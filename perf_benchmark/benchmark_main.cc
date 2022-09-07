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
#include "google/protobuf/struct.pb.h"
#include "absl/strings/string_view.h"
#include "absl/strings/str_format.h"
#include "absl/strings/escaping.h"
#include "absl/memory/memory.h"
#include "google/api/service.pb.h"

#include "perf_benchmark/benchmark_common.h"
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
constexpr absl::string_view
    kStructPayloadMessageType = "google.protobuf.Struct";
constexpr absl::string_view kInt32ArrayPayloadMessageType = "Int32ArrayPayload";
constexpr absl::string_view
    kDoubleArrayPayloadMessageType = "DoubleArrayPayload";
constexpr absl::string_view
    kStringArrayPayloadMessageType = "StringArrayPayload";
}

// Helper method to run Json Translation benchmark.
//
// state - ::benchmark::State& variable used for collecting metrics.
// msg_type - Protobuf message name for translation.
// request_info - Json-Protobuf request information, body_field_path and
//                variable_bindings need to be set before it is passed in.
// json_msg_ptr - Pointer to a complete json message.
// streaming - Flag for streaming testing. When true, a stream of `stream_size`
//             number of `json_msg_ptr` will be fed into translation.
// stream_size - Number of streaming messages.
// chunk_per_msg - Number of data chunks per message.
std::string BenchmarkJsonTranslation(::benchmark::State& state,
                                     absl::string_view msg_type,
                                     const std::string* json_msg_ptr,
                                     bool streaming,
                                     int64_t stream_size,
                                     int chunk_per_msg) {
  // Load service config proto into Service object
  google::api::Service service;
  LoadService(std::string(kServiceConfigTextProtoFile), &service);

  // Create a TypeHelper based on the service config
  TypeHelper type_helper(service.types(), service.enums());

  // Get message type
  const google::protobuf::Type* type = type_helper.Info()->GetTypeByTypeUrl(
      absl::StrFormat("type.googleapis.com/%s", msg_type));
  if (nullptr == type) {
    std::cerr << "Could not resolve the message type " << msg_type << std::endl;
    return "";
  }

  RequestInfo request_info;
  // body field path used in this benchmark are all "*"
  request_info.body_field_path = "*";
  request_info.variable_bindings = std::vector<RequestWeaver::BindingInfo>();
  request_info.message_type = type;

  // Wrap json_msg_ptr inside ZeroCopyInputStream.
  // Using heap instead of stack because heap space is more suitable to hold
  // large data chunks.
  auto is = absl::make_unique<BenchmarkZeroCopyInputStream>(*json_msg_ptr,
                                                            streaming,
                                                            stream_size,
                                                            chunk_per_msg);

  // Benchmark the transcoding process
  std::string message;
  for (auto s: state) {
    JsonRequestTranslator
        translator
        (type_helper.Resolver(), is.get(), request_info, streaming, false);
    MessageStream& out = translator.Output();

    if (!out.Status().ok()) {
      std::cerr << "Error: " << out.Status().message().as_string().c_str();
      return "";
    }

    while (out.NextMessage(&message)) {
    }
    is->Reset();
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
      Counter(bytes_processed, Counter::kIsRate | Counter::kInvert,
              Counter::kIs1024);
  state.counters["request_throughput"] =
      Counter(request_processed, Counter::kIsRate);
  state.counters["request_latency"] =
      Counter(request_processed, Counter::kIsRate | Counter::kInvert);
  state.counters["message_throughput"] =
      Counter(message_processed, Counter::kIsRate);
  state.counters["message_latency"] =
      Counter(message_processed, Counter::kIsRate | Counter::kInvert);
  return message;
}

//
// Benchmark variable: JSON body length.
//
// Helper function for benchmarking single bytes payload translation from JSON.
void BM_SinglePayloadFromJson(::benchmark::State& state,
                              int64_t payload_length,
                              bool streaming,
                              int64_t stream_size) {
  auto msg =
      absl::make_unique<std::string>(GetRandomBytesString(payload_length,
                                                          true));
  auto json_msg =
      absl::make_unique<std::string>(absl::StrFormat(R"({"payload" : "%s"})",
                                                     *msg));
  std::string proto_str = BenchmarkJsonTranslation(state,
                                                   kBytesPayloadMessageType,
                                                   json_msg.get(),
                                                   streaming,
                                                   stream_size,
                                                   1);
  // Verification
  std::string msg_decoded;
  absl::Base64Unescape(*msg, &msg_decoded);
  BytesPayload actual_proto;
  actual_proto.ParseFromString(proto_str);
  if (actual_proto.payload() != msg_decoded) {
    state.SkipWithError(("JSON parsing error, parsed proto: "
        + actual_proto.DebugString()).c_str());
  }
}

static void BM_SinglePayloadFromJsonNonStreaming(::benchmark::State& state) {
  BM_SinglePayloadFromJson(state, state.range(0), false, 0);
}
BENCHMARK_WITH_PERCENTILE(BM_SinglePayloadFromJsonNonStreaming)
    ->Arg(12) // 1 byte
    ->Arg(1 << 10) // 1 KiB
    ->Arg(1 << 20) // 1 MiB
    ->Arg(1 << 25); // 32 MiB

static void BM_SinglePayloadFromJsonStreaming(::benchmark::State& state) {
  int64_t byte_length = 1 << 20; // 1 MiB
  BM_SinglePayloadFromJson(state, byte_length, true, state.range(0));
}
BENCHMARK_STREAMING_WITH_PERCENTILE(BM_SinglePayloadFromJsonStreaming);

//
// Benchmark variable: JSON array length.
//
// Helper function for benchmarking int32 array payload translation from JSON.
void BM_Int32ArrayPayloadFromJson(::benchmark::State& state,
                                  int64_t array_length,
                                  bool streaming,
                                  int64_t stream_size) {
  auto json_msg =
      absl::make_unique<std::string>(absl::StrFormat(R"({"payload" : %s})",
                                                     GetRandomInt32ArrayString(
                                                         array_length)));
  std::string proto_str = BenchmarkJsonTranslation(state,
                                                   kInt32ArrayPayloadMessageType,
                                                   json_msg.get(),
                                                   streaming,
                                                   stream_size,
                                                   1);

  // Verification
  Int32ArrayPayload actual_proto;
  actual_proto.ParseFromString(proto_str);
  if (actual_proto.payload().size() != array_length) {
    state.SkipWithError(("JSON parsing error, parsed proto: "
        + actual_proto.DebugString()).c_str());
  }
}

static void BM_Int32ArrayPayloadFromJsonNonStreaming(::benchmark::State& state) {
  BM_Int32ArrayPayloadFromJson(state, state.range(0), false, 0);
}
BENCHMARK_WITH_PERCENTILE(BM_Int32ArrayPayloadFromJsonNonStreaming)
    ->Arg(1) // 1 val
    ->Arg(1 << 8) // 256 vals
    ->Arg(1 << 10) // 1024 vals
    ->Arg(1 << 14); // 16384 vals

static void BM_Int32ArrayPayloadFromJsonStreaming(::benchmark::State& state) {
  int64_t array_length = 1 << 14; // 16384 int values
  BM_Int32ArrayPayloadFromJson(state, array_length, true, state.range(0));
}
BENCHMARK_STREAMING_WITH_PERCENTILE(BM_Int32ArrayPayloadFromJsonStreaming);

//
// Benchmark variable: JSON value data type.
// E.g. "0" can be parsed as int32, double, or string.
// Only non-streaming is benchmarked since the JSON is already an array.
// Benchmarks for array typed JSON streaming is tested with the JSON array
// length benchmark variable.
//
// Helper function for benchmarking translation from JSON to payload of
// different types.
template<class T>
void BM_ArrayPayloadFromJson(::benchmark::State& state,
                             absl::string_view msg_type,
                             bool streaming,
                             int64_t stream_size) {
  int64_t array_length = 1 << 10; // 1024
  auto json_msg =
      absl::make_unique<std::string>(absl::StrFormat(R"({"payload" : %s})",
                                                     GetRepeatedValueArrayString(
                                                         "0",
                                                         array_length)));
  std::string proto_str = BenchmarkJsonTranslation(state,
                                                   msg_type,
                                                   json_msg.get(),
                                                   streaming,
                                                   stream_size,
                                                   1);

  // Verification
  T actual_proto;
  actual_proto.ParseFromString(proto_str);
  if (actual_proto.payload().size() != array_length) {
    state.SkipWithError(("JSON parsing error, parsed proto: "
        + actual_proto.DebugString()).c_str());
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
BENCHMARK_WITH_PERCENTILE(BM_Int32ArrayTypePayloadFromJsonNonStreaming);
BENCHMARK_WITH_PERCENTILE(BM_DoubleArrayTypePayloadFromJsonNonStreaming);
BENCHMARK_WITH_PERCENTILE(BM_StringArrayTypePayloadFromJsonNonStreaming);

//
// Benchmark variable: Number of nested JSON layer.
//
// Helper function for benchmarking translation from nested JSON values.
void BM_NestedPayloadFromJson(::benchmark::State& state,
                              int64_t layers,
                              bool streaming,
                              int64_t stream_size,
                              absl::string_view msg_type) {
  auto json_msg =
      absl::make_unique<std::string>(GetNestedJsonString(layers,
                                                         kNestedFieldName,
                                                         std::string(
                                                             kInnerMostNestedFieldName),
                                                         "buzz"));
  std::string proto_str = BenchmarkJsonTranslation(state,
                                                   msg_type,
                                                   json_msg.get(),
                                                   streaming,
                                                   stream_size,
                                                   1);

  // Verification
  int actual_layers = 0;
  if (msg_type == kNestedPayloadMessageType) {
    NestedPayload actual_proto;
    actual_proto.ParseFromString(proto_str);

    while (actual_proto.has_nested()) {
      ++actual_layers;
      // avoid stackoverflow due to copy
      NestedPayload tmp(actual_proto.nested());
      actual_proto.clear_nested();
      actual_proto.clear_payload();
      actual_proto = tmp;
    }
    // This is because actual_proto.has_nested() will be true for
    // every first call.
    actual_layers -= 1;

  } else if (msg_type == kStructPayloadMessageType) {
    google::protobuf::Struct actual_proto;
    actual_proto.ParseFromString(proto_str);

    std::string field_name = std::string(kNestedFieldName);
    while (actual_proto.fields().contains(field_name)) {
      ++actual_layers;
      // avoid stackoverflow due to copy
      google::protobuf::Struct
          tmp = actual_proto.fields().at(field_name).struct_value();
      actual_proto.clear_fields();
      actual_proto = tmp;
    }
  }

  if (actual_layers != layers) {
    state.SkipWithError(absl::StrFormat(
        "Actual layer is not as expected. Actual layers: %d, Expected layers: %d",
        actual_layers,
        layers).c_str());
  }
}

static void BM_NestedProtoPayloadFromJsonNonStreaming(::benchmark::State& state) {
  BM_NestedPayloadFromJson(state,
                           state.range(0),
                           false,
                           0,
                           kNestedPayloadMessageType);
}
BENCHMARK_WITH_PERCENTILE(BM_NestedProtoPayloadFromJsonNonStreaming)
    ->Arg(0) // flat JSON
    ->Arg(1) // nested with 1 layer
    ->Arg(8) // nested with 8 layers
    ->Arg(32) // nested with 32 layers
    ->Arg(64); // nested with 64 layers

static void BM_NestedProtoPayloadFromJsonStreaming(::benchmark::State& state) {
  BM_NestedPayloadFromJson(state,
                           64,
                           true,
                           state.range(0),
                           kNestedPayloadMessageType);
}
BENCHMARK_STREAMING_WITH_PERCENTILE(BM_NestedProtoPayloadFromJsonStreaming);

static void BM_StructProtoPayloadFromJsonNonStreaming(::benchmark::State& state) {
  BM_NestedPayloadFromJson(state,
                           state.range(0),
                           false,
                           0,
                           kStructPayloadMessageType);
}
BENCHMARK_WITH_PERCENTILE(BM_StructProtoPayloadFromJsonNonStreaming)
    ->Arg(0) // flat JSON
    ->Arg(1) // nested with 1 layer
    ->Arg(8) // nested with 8 layers
    ->Arg(32); // nested with 32 layers
    // More than 32 layers would fail the parsing

static void BM_StructProtoPayloadFromJsonStreaming(::benchmark::State& state) {
  BM_NestedPayloadFromJson(state,
                           64,
                           true,
                           state.range(0),
                           kStructPayloadMessageType);
}
BENCHMARK_STREAMING_WITH_PERCENTILE(BM_StructProtoPayloadFromJsonStreaming);

//
// Benchmark variable: Message chunk per message
//
// Helper function for benchmarking translation from segmented JSON input
void BM_SegmentedStringPayloadFromJson(::benchmark::State& state,
                                       int64_t payload_length,
                                       bool streaming,
                                       int64_t stream_size,
                                       int chunk_per_msg) {
  auto msg =
      absl::make_unique<std::string>(GetRandomString(payload_length));
  auto json_msg =
      absl::make_unique<std::string>(absl::StrFormat(R"({"payload" : "%s"})",
                                                     *msg));
  std::string proto_str = BenchmarkJsonTranslation(state,
                                                   kStringPayloadMessageType,
                                                   json_msg.get(),
                                                   streaming,
                                                   stream_size,
                                                   chunk_per_msg);
  // Verification
  StringPayload actual_proto;
  actual_proto.ParseFromString(proto_str);
  if (actual_proto.payload() != *msg) {
    state.SkipWithError(("JSON parsing error, parsed proto: "
        + actual_proto.DebugString()).c_str());
  }
}

static void BM_SegmentedStringPayloadFromJsonNonStreaming(::benchmark::State& state) {
  int64_t payload_length = 1 << 20; // 1 MiB
  BM_SegmentedStringPayloadFromJson(state,
                                    payload_length,
                                    false,
                                    0,
                                    state.range(0));
}
BENCHMARK_WITH_PERCENTILE(BM_SegmentedStringPayloadFromJsonNonStreaming)
    ->Arg(1) // 1 chunk per message
    ->Arg(1 << 4) // 16 chunks per message
    ->Arg(1 << 8) // 256 chunks per message
    ->Arg(1 << 12); // 4096 chunks per message

static void BM_SegmentedStringPayloadFromJsonStreaming(::benchmark::State& state) {
  int64_t payload_length = 1 << 20; // 1 MiB
  BM_SegmentedStringPayloadFromJson(state,
                                    payload_length,
                                    true,
                                    state.range(0),
                                    1 << 8);
}
BENCHMARK_STREAMING_WITH_PERCENTILE(BM_SegmentedStringPayloadFromJsonStreaming);

// Benchmark Main function
BENCHMARK_MAIN();

} // namespace perf_benchmark

} // namespace transcoding
} // namespace grpc
} // namespace google

