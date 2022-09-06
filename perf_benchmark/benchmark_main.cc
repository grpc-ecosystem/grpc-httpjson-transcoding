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
#include "absl/memory/memory.h"
#include "google/api/service.pb.h"

#include <utility>
#include "perf_benchmark/benchmark_common.h"

namespace google {
namespace grpc {
namespace transcoding {

namespace perf_benchmark {
namespace {
using namespace benchmark;

constexpr absl::string_view
    kServiceConfigTextProtoFile = "benchmark_service.textproto";
constexpr absl::string_view kInnerMostNestedFieldName = "payload";
constexpr absl::string_view kBytesPayloadMessageType = "BytesPayload";
constexpr absl::string_view kNestedPayloadMessageType = "NestedPayload";
constexpr absl::string_view kInt32ArrayPayloadMessageType = "Int32ArrayPayload";
constexpr absl::string_view kBoolArrayPayloadMessageType = "BoolArrayPayload";
constexpr absl::string_view kBytesArrayPayloadMessageType = "BytesArrayPayload";
constexpr absl::string_view
    kDoubleArrayPayloadMessageType = "DoubleArrayPayload";
constexpr absl::string_view
    kStringArrayPayloadMessageType = "StringArrayPayload";
}
// TODO Add memory manager
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
void BenchmarkJsonTranslation(::benchmark::State& state,
                              absl::string_view msg_type,
                              RequestInfo request_info,
                              const std::string* json_msg_ptr,
                              bool streaming,
                              int64_t stream_size) {
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
    return;
  }
  request_info.message_type = type;

  // Wrap json_msg_ptr inside ZeroCopyInputStream.
  // Using heap instead of stack because heap space is more suitable to hold
  // large data chunks.
  auto is = absl::make_unique<BenchmarkZeroCopyInputStream>(*json_msg_ptr,
                                                            streaming,
                                                            stream_size);

  // Benchmark the transcoding process
  for (auto s: state) {
    JsonRequestTranslator
        translator
        (type_helper.Resolver(), is.get(), request_info, streaming, false);
    MessageStream& out = translator.Output();

    if (!out.Status().ok()) {
      std::cerr << "Error: " << out.Status().message().as_string().c_str();
      return;
    }

    std::string message;
    while (out.NextMessage(&message)) {
//      BytePayload actual_proto;
//      actual_proto.ParseFromString(message);
//      std::cout << "Message=" << actual_proto.DebugString() << std::endl;
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
}

// Create and populate request info with the given body_field_path and an empty
// variable_bindings.
RequestInfo CreateRequestInfo(std::string body_field_path) {
  RequestInfo request_info;
  request_info.body_field_path = std::move(body_field_path);
  request_info.variable_bindings = std::vector<RequestWeaver::BindingInfo>();
  return request_info;
}

//
// Benchmark variable: JSON body length.
//
// Helper function for benchmarking single bytes payload translation from JSON.
void BM_SinglePayloadFromJson(::benchmark::State& state,
                              int64_t payload_length,
                              bool streaming,
                              int64_t stream_size) {
  RequestInfo request_info = CreateRequestInfo("*");
  auto json_msg =
      absl::make_unique<std::string>(absl::StrFormat(R"({"payload" : "%s"})",
                                                     GetRandomString(
                                                         payload_length,
                                                         true)));
  BenchmarkJsonTranslation(state,
                           kBytesPayloadMessageType,
                           request_info,
                           json_msg.get(),
                           streaming,
                           stream_size);
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
  RequestInfo request_info = CreateRequestInfo("*");
  auto json_msg =
      absl::make_unique<std::string>(absl::StrFormat(R"({"payload" : %s})",
                                                     GetRandomInt32ArrayString(
                                                         array_length)));
  BenchmarkJsonTranslation(state,
                           kInt32ArrayPayloadMessageType,
                           request_info,
                           json_msg.get(),
                           streaming,
                           stream_size);
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
// Only non-streaming is benchmarked since the JSON is already an array.
// Benchmarks for array typed JSON streaming is tested with the JSON array
// length benchmark variable.
//
// Helper function for benchmarking translation from JSON to payload of
// different types.
void BM_ArrayPayloadFromJson(::benchmark::State& state,
                             absl::string_view msg_type,
                             bool streaming,
                             int64_t stream_size) {
  RequestInfo request_info = CreateRequestInfo("*");
  int64_t array_length = 1 << 10; // 1024
  auto json_msg =
      absl::make_unique<std::string>(absl::StrFormat(R"({"payload" : %s})",
                                                     GetRepeatedValueArrayString(
                                                         "0",
                                                         array_length)));
  BenchmarkJsonTranslation(state,
                           msg_type,
                           request_info,
                           json_msg.get(),
                           streaming,
                           stream_size);
}

static void BM_Int32ArrayTypePayloadFromJsonNonStreaming(::benchmark::State& state) {
  BM_ArrayPayloadFromJson(state, kInt32ArrayPayloadMessageType, false, 0);
}
static void BM_DoubleArrayTypePayloadFromJsonNonStreaming(::benchmark::State& state) {
  BM_ArrayPayloadFromJson(state, kDoubleArrayPayloadMessageType, false, 0);
}
static void BM_BoolArrayTypePayloadFromJsonNonStreaming(::benchmark::State& state) {
  BM_ArrayPayloadFromJson(state, kBoolArrayPayloadMessageType, false, 0);
}
static void BM_StringArrayTypePayloadFromJsonNonStreaming(::benchmark::State& state) {
  BM_ArrayPayloadFromJson(state, kStringArrayPayloadMessageType, false, 0);
}
static void BM_BytesArrayTypePayloadFromJsonNonStreaming(::benchmark::State& state) {
  BM_ArrayPayloadFromJson(state, kBytesArrayPayloadMessageType, false, 0);
}
BENCHMARK_WITH_PERCENTILE(BM_Int32ArrayTypePayloadFromJsonNonStreaming);
BENCHMARK_WITH_PERCENTILE(BM_DoubleArrayTypePayloadFromJsonNonStreaming);
BENCHMARK_WITH_PERCENTILE(BM_BoolArrayTypePayloadFromJsonNonStreaming);
BENCHMARK_WITH_PERCENTILE(BM_StringArrayTypePayloadFromJsonNonStreaming);
BENCHMARK_WITH_PERCENTILE(BM_BytesArrayTypePayloadFromJsonNonStreaming);

//
// Benchmark variable: Number of nested JSON layer.
//
// Helper function for benchmarking translation from nested JSON values.
void BM_NestedPayloadFromJson(::benchmark::State& state,
                              int64_t layers,
                              bool streaming,
                              int64_t stream_size) {
  RequestInfo request_info = CreateRequestInfo("*");
  auto json_msg =
      absl::make_unique<std::string>(GetNestedJsonString(layers,
                                                         kInnerMostNestedFieldName,
                                                         "buzz"));
  BenchmarkJsonTranslation(state,
                           kNestedPayloadMessageType,
                           request_info,
                           json_msg.get(),
                           streaming,
                           stream_size);
}

static void BM_NestedPayloadFromJsonNonStreaming(::benchmark::State& state) {
  BM_NestedPayloadFromJson(state, state.range(0), false, 0);
}
BENCHMARK_WITH_PERCENTILE(BM_NestedPayloadFromJsonNonStreaming)
    ->Arg(0) // flat JSON
    ->Arg(1) // nested with 1 layer
    ->Arg(8) // nested with 8 layers
    ->Arg(32) // nested with 32 layers
    ->Arg(64); // nested with 64 layers

static void BM_NestedPayloadFromJsonStreaming(::benchmark::State& state) {
  BM_NestedPayloadFromJson(state, 64, true, state.range(0));
}
BENCHMARK_STREAMING_WITH_PERCENTILE(BM_NestedPayloadFromJsonStreaming);

// Benchmark Main function
BENCHMARK_MAIN();

} // namespace perf_benchmark

} // namespace transcoding
} // namespace grpc
} // namespace google

