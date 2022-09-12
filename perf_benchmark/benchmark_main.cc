// Copyright 2016 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
////////////////////////////////////////////////////////////////////////////////
//
#include "grpc_transcoding/request_message_translator.h"
#include "grpc_transcoding/json_request_translator.h"
#include "grpc_transcoding/type_helper.h"
#include "benchmark/benchmark.h"
#include "google/protobuf/text_format.h"
#include "absl/strings/string_view.h"
#include "absl/strings/str_format.h"
#include "absl/strings/escaping.h"
#include "absl/status/statusor.h"
#include "absl/memory/memory.h"
#include "google/api/service.pb.h"

#include "perf_benchmark/utils.h"
#include "perf_benchmark/benchmark_input_stream.h"
#include "perf_benchmark/benchmark.pb.h"

namespace google {
namespace grpc {
namespace transcoding {

namespace perf_benchmark {
namespace {
using namespace benchmark;

constexpr absl::string_view
    kServiceConfigTextProtoFile = "benchmark_service.textproto";
constexpr absl::string_view kNestedFieldName = "nested";
constexpr absl::string_view kInnerMostNestedFieldName = "payload";
constexpr absl::string_view kBytesPayloadMessageType = "BytesPayload";
constexpr absl::string_view kStringPayloadMessageType = "StringPayload";
constexpr absl::string_view kNestedPayloadMessageType = "NestedPayload";
constexpr absl::string_view kInt32ArrayPayloadMessageType = "Int32ArrayPayload";
constexpr absl::string_view
    kStructPayloadMessageType = "google.protobuf.Struct";
constexpr absl::string_view
    kDoubleArrayPayloadMessageType = "DoubleArrayPayload";
constexpr absl::string_view
    kStringArrayPayloadMessageType = "StringArrayPayload";

// Used for NestedPayload and StructPayload
constexpr uint64_t kNumNestedLayersForStreaming = 32;
// Used for ArrayPayload
constexpr uint64_t kArrayPayloadLengthForStreaming = 1 << 10; // 1024
// Used for BytesPayload
constexpr uint64_t kBytesPayloadLengthForStreaming = 1 << 20; // 1 MiB
// Used for Int32ArrayPayload
constexpr uint64_t kInt32ArrayPayloadLengthForStreaming = 1 << 14; // 16384
// Used for Segmented StringPayload
constexpr uint64_t kSegmentedStringPayloadLength = 1 << 20; // 1 MiB
constexpr uint64_t kSegmentedStringStreamingNumChunksPerMsg = 1 << 12; // 4096

// Global type helper containing the type information of the benchmark_service
// service config object.
[[nodiscard]] const TypeHelper& GetBenchmarkTypeHelper() {
  static const auto* const kTypeHelper = [] {
    // Load service config proto into Service object.
    // Construct object on the heap using new without calling its dtor to
    // avoid destruction issue with static variables. However, this can cause
    // unnecessary heap allocations and create minor performance concerns.
    // For a small benchmark script, this is okay.
    auto* service = new google::api::Service();
    GOOGLE_CHECK_OK(
        LoadService(std::string(kServiceConfigTextProtoFile), service));

    // Create a TypeHelper based on the service config.
    // Construct object on the heap for the same reason as the Service config.
    auto* type_helper = new TypeHelper(service->types(), service->enums());
    return type_helper;
  }();
  return *kTypeHelper;
}
} // namespace

// Helper method to run Json Translation benchmark.
//
// error.
// state - ::benchmark::State& variable used for collecting metrics.
// msg_type - Protobuf message name for translation.
// json_msg - Complete input json message.
// streaming - Flag for streaming testing. When true, a stream of `stream_size`
//             number of `json_msg` will be fed into translation.
// stream_size - Number of streaming messages.
// chunk_per_msg - Number of data chunks per message.
absl::Status BenchmarkJsonTranslation(::benchmark::State& state,
                                      absl::string_view msg_type,
                                      absl::string_view json_msg,
                                      bool streaming,
                                      uint64_t stream_size,
                                      uint64_t chunk_per_msg) {
  // Retrieve global type helper
  const TypeHelper& type_helper = GetBenchmarkTypeHelper();

  // Get message type
  const google::protobuf::Type* type = type_helper.Info()->GetTypeByTypeUrl(
      absl::StrFormat("type.googleapis.com/%s", msg_type));
  if (nullptr == type) {
    return absl::InvalidArgumentError(absl::StrCat(
        "Could not resolve the message type ",
        msg_type));
  }

  RequestInfo request_info;
  // body field path used in this benchmark are all "*"
  request_info.body_field_path = "*";
  request_info.variable_bindings = std::vector<RequestWeaver::BindingInfo>();
  request_info.message_type = type;

  // Wrap json_msg inside BenchmarkZeroCopyInputStream.
  std::unique_ptr<BenchmarkZeroCopyInputStream> is;
  if (streaming) {
    std::string streaming_msg = GetStreamedJson(json_msg, stream_size);
    is = absl::make_unique<BenchmarkZeroCopyInputStream>(streaming_msg,
                                                         chunk_per_msg);
  } else {
    is = absl::make_unique<BenchmarkZeroCopyInputStream>(std::string(json_msg),
                                                         chunk_per_msg);
  }

  // Benchmark the transcoding process
  std::string message;
  for (auto s: state) {
    JsonRequestTranslator translator
        (type_helper.Resolver(), is.get(), request_info, streaming, false);
    MessageStream& out = translator.Output();

    if (!out.Status().ok()) {
      return absl::InternalError(out.Status().ToString());
    }

    while (out.NextMessage(&message)) {
    }
    is->Reset(); // low overhead.
  }

  // Add custom benchmark counters
  auto request_processed = static_cast<double>(state.iterations());
  auto message_processed =
      static_cast<double>(state.iterations() * (streaming ? stream_size : 1));
  auto bytes_processed =
      static_cast<double>(state.iterations() * is->TotalBytes());
  state.counters["byte_throughput"] = Counter(bytes_processed, Counter::kIsRate,
                                              Counter::kIs1024);
  state.counters["byte_latency"] =
      Counter(bytes_processed,
              Counter::kIsRate | Counter::kInvert,
              Counter::kIs1024);
  state.counters["request_throughput"] =
      Counter(request_processed, Counter::kIsRate);
  state.counters["request_latency"] =
      Counter(request_processed, Counter::kIsRate | Counter::kInvert);
  state.counters["message_throughput"] =
      Counter(message_processed, Counter::kIsRate);
  state.counters["message_latency"] =
      Counter(message_processed, Counter::kIsRate | Counter::kInvert);

  return absl::OkStatus();
}

// Helper function for benchmarking single bytes payload translation from JSON.
void BM_SinglePayloadFromJson(::benchmark::State& state,
                              uint64_t payload_length,
                              bool streaming,
                              uint64_t stream_size) {
  std::string json_msg = absl::StrFormat(
      R"({"payload" : "%s"})", GetRandomBytesString(payload_length, true));
  absl::Status status = BenchmarkJsonTranslation(state,
                                                 kBytesPayloadMessageType,
                                                 json_msg,
                                                 streaming,
                                                 stream_size,
                                                 1);
  if (!status.ok()) {
    state.SkipWithError(status.ToString().c_str());
  }
}

static void BM_SinglePayloadFromJsonNonStreaming(::benchmark::State& state) {
  BM_SinglePayloadFromJson(state, state.range(0), false, 0);
}

static void BM_SinglePayloadFromJsonStreaming(::benchmark::State& state) {
  BM_SinglePayloadFromJson(state,
                           kBytesPayloadLengthForStreaming,
                           true,
                           state.range(0));
}

// Helper function for benchmarking int32 array payload translation from JSON.
void BM_Int32ArrayPayloadFromJson(::benchmark::State& state,
                                  uint64_t array_length,
                                  bool streaming,
                                  uint64_t stream_size) {
  std::string json_msg = absl::StrFormat(R"({"payload" : %s})",
                                         GetRandomInt32ArrayString(array_length));
  absl::Status status = BenchmarkJsonTranslation(state,
                                                 kInt32ArrayPayloadMessageType,
                                                 json_msg,
                                                 streaming,
                                                 stream_size,
                                                 1);
  if (!status.ok()) {
    state.SkipWithError(status.ToString().c_str());
  }
}

static void BM_Int32ArrayPayloadFromJsonNonStreaming(::benchmark::State& state) {
  BM_Int32ArrayPayloadFromJson(state, state.range(0), false, 0);
}

static void BM_Int32ArrayPayloadFromJsonStreaming(::benchmark::State& state) {
  BM_Int32ArrayPayloadFromJson(state,
                               kInt32ArrayPayloadLengthForStreaming,
                               true,
                               state.range(0));
}

// Helper function for benchmarking translation from JSON to payload of
// different types.
template<class T>
void BM_ArrayPayloadFromJson(::benchmark::State& state,
                             absl::string_view msg_type,
                             bool streaming,
                             uint64_t stream_size) {
  auto json_msg = absl::StrFormat(
      R"({"payload" : %s})",
      GetRepeatedValueArrayString("0", kArrayPayloadLengthForStreaming));
  absl::Status status = BenchmarkJsonTranslation(state,
                                                 msg_type,
                                                 json_msg,
                                                 streaming,
                                                 stream_size,
                                                 1);
  if (!status.ok()) {
    state.SkipWithError(status.ToString().c_str());
  }
}

static void BM_Int32ArrayTypePayloadFromJsonNonStreaming(::benchmark::State& state) {
  BM_ArrayPayloadFromJson<Int32ArrayPayload>(state,
                                             kInt32ArrayPayloadMessageType,
                                             false,
                                             0);
}
static void BM_DoubleArrayTypePayloadFromJsonNonStreaming(::benchmark::State& state) {
  BM_ArrayPayloadFromJson<DoubleArrayPayload>(state,
                                              kDoubleArrayPayloadMessageType,
                                              false,
                                              0);
}
static void BM_StringArrayTypePayloadFromJsonNonStreaming(::benchmark::State& state) {
  BM_ArrayPayloadFromJson<StringArrayPayload>(state,
                                              kStringArrayPayloadMessageType,
                                              false,
                                              0);
}

// Helper function for benchmarking translation from nested JSON values.
void BM_NestedPayloadFromJson(::benchmark::State& state,
                              uint64_t layers,
                              bool streaming,
                              uint64_t stream_size,
                              absl::string_view msg_type) {
  const std::string json_msg = GetNestedJsonString(
      layers, kNestedFieldName, std::string(kInnerMostNestedFieldName), "buzz");
  absl::Status status = BenchmarkJsonTranslation(state,
                                                 msg_type,
                                                 json_msg,
                                                 streaming,
                                                 stream_size,
                                                 1);
  if (!status.ok()) {
    state.SkipWithError(status.ToString().c_str());
  }
}

static void BM_NestedProtoPayloadFromJsonNonStreaming(::benchmark::State& state) {
  BM_NestedPayloadFromJson(state,
                           state.range(0),
                           false,
                           0,
                           kNestedPayloadMessageType);
}

static void BM_NestedProtoPayloadFromJsonStreaming(::benchmark::State& state) {
  BM_NestedPayloadFromJson(state,
                           kNumNestedLayersForStreaming,
                           true,
                           state.range(0),
                           kNestedPayloadMessageType);
}

static void BM_StructProtoPayloadFromJsonNonStreaming(::benchmark::State& state) {
  BM_NestedPayloadFromJson(state,
                           state.range(0),
                           false,
                           0,
                           kStructPayloadMessageType);
}

static void BM_StructProtoPayloadFromJsonStreaming(::benchmark::State& state) {
  BM_NestedPayloadFromJson(state,
                           kNumNestedLayersForStreaming,
                           true,
                           state.range(0),
                           kStructPayloadMessageType);
}

// Helper function for benchmarking translation from segmented JSON input
void BM_SegmentedStringPayloadFromJson(::benchmark::State& state,
                                       uint64_t payload_length,
                                       bool streaming,
                                       uint64_t stream_size,
                                       uint64_t chunk_per_msg) {
  // We are using GetRandomAlphanumericString instead of GetRandomBytesString
  // because JSON format reserves characters such as `"` and `\`.
  // We could generate `"` and `\` and escape them, but for simplicity, we are
  // only using alphanumeric characters.
  // This would also be a more common for string proto.
  const std::string json_msg =
      absl::StrFormat(R"({"payload" : "%s"})",
                      GetRandomAlphanumericString(payload_length));
  absl::Status status = BenchmarkJsonTranslation(state,
                                                 kStringPayloadMessageType,
                                                 json_msg,
                                                 streaming,
                                                 stream_size,
                                                 chunk_per_msg);
  if (!status.ok()) {
    state.SkipWithError(status.ToString().c_str());
  }
}

static void BM_SegmentedStringPayloadFromJsonNonStreaming(::benchmark::State& state) {
  BM_SegmentedStringPayloadFromJson(state,
                                    kSegmentedStringPayloadLength,
                                    false,
                                    0,
                                    state.range(0));
}

static void BM_SegmentedStringPayloadFromJsonStreaming(::benchmark::State& state) {
  BM_SegmentedStringPayloadFromJson(state,
                                    kSegmentedStringPayloadLength,
                                    true,
                                    state.range(0),
                                    1 << 8);
}

//
// Independent benchmark variable: JSON body length.
//
BENCHMARK_WITH_PERCENTILE(BM_SinglePayloadFromJsonNonStreaming)
    ->Arg(1) // 1 byte
    ->Arg(1 << 10) // 1 KiB
    ->Arg(1 << 20) // 1 MiB
    ->Arg(1 << 25); // 32 MiB
BENCHMARK_STREAMING_WITH_PERCENTILE(BM_SinglePayloadFromJsonStreaming, 1);

//
// Independent benchmark variable: JSON array length.
//
BENCHMARK_WITH_PERCENTILE(BM_Int32ArrayPayloadFromJsonNonStreaming)
    ->Arg(1) // 1 val
    ->Arg(1 << 8) // 256 vals
    ->Arg(1 << 10) // 1024 vals
    ->Arg(1 << 14); // 16384 vals
BENCHMARK_STREAMING_WITH_PERCENTILE(BM_Int32ArrayPayloadFromJsonStreaming, 1);

//
// Independent benchmark variable: JSON value data type.
// E.g. "0" can be parsed as int32, double, or string.
// Only non-streaming is benchmarked since the JSON is already an array.
// Benchmarks for array typed JSON streaming is tested with the JSON array
// length benchmark variable.
//
BENCHMARK_WITH_PERCENTILE(BM_Int32ArrayTypePayloadFromJsonNonStreaming);
BENCHMARK_WITH_PERCENTILE(BM_DoubleArrayTypePayloadFromJsonNonStreaming);
BENCHMARK_WITH_PERCENTILE(BM_StringArrayTypePayloadFromJsonNonStreaming);

//
// Independent benchmark variable: Number of nested JSON layer.
//
BENCHMARK_WITH_PERCENTILE(BM_NestedProtoPayloadFromJsonNonStreaming)
    ->Arg(0) // flat JSON
    ->Arg(1) // nested with 1 layer
    ->Arg(8) // nested with 8 layers
    ->Arg(32); // nested with 32 layers
BENCHMARK_WITH_PERCENTILE(BM_StructProtoPayloadFromJsonNonStreaming)
    ->Arg(0) // flat JSON
    ->Arg(1) // nested with 1 layer
    ->Arg(8) // nested with 8 layers
        // More than 32 layers would fail the parsing for struct proto.
    ->Arg(32); // nested with 32 layers
BENCHMARK_STREAMING_WITH_PERCENTILE(BM_NestedProtoPayloadFromJsonStreaming, 1);
BENCHMARK_STREAMING_WITH_PERCENTILE(BM_StructProtoPayloadFromJsonStreaming, 1);

//
// Independent benchmark variable: Message chunk per message
//
BENCHMARK_WITH_PERCENTILE(BM_SegmentedStringPayloadFromJsonNonStreaming)
    ->Arg(1) // 1 chunk per message
    ->Arg(1 << 4) // 16 chunks per message
    ->Arg(1 << 8) // 256 chunks per message
    ->Arg(1 << 12); // 4096 chunks per message
BENCHMARK_STREAMING_WITH_PERCENTILE(BM_SegmentedStringPayloadFromJsonStreaming,
                                    kSegmentedStringStreamingNumChunksPerMsg);

// Benchmark Main function
BENCHMARK_MAIN();

} // namespace perf_benchmark

} // namespace transcoding
} // namespace grpc
} // namespace google

