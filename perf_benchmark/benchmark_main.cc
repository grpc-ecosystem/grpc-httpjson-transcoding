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
#include "benchmark/benchmark.h"
#include "grpc_transcoding/type_helper.h"
#include "google/protobuf/text_format.h"
#include "absl/strings/string_view.h"
#include "absl/strings/str_format.h"
#include "google/api/service.pb.h"
#include "perf_benchmark/benchmark_common.h"
#include "absl/memory/memory.h"

namespace google {
namespace grpc {
namespace transcoding {

namespace perf_benchmark {
namespace {
using namespace benchmark;

constexpr absl::string_view
    kServiceConfigTextProtoFile = "benchmark_service.textproto";
constexpr absl::string_view kBytePayloadMessageType = "BytePayload";
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
static void BenchmarkJsonTranslation(::benchmark::State& state,
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
  auto message_processed = static_cast<double>(state.iterations() * (streaming ? stream_size : 1));
  auto bytes_processed =
      static_cast<double>(state.iterations() * state.range(0));
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

static void BM_SinglePayloadFromJson(::benchmark::State& state,
                                     int64_t payload_length,
                                     bool streaming,
                                     int64_t stream_size) {
  // Populate request info except for message_type which will be populated
  // inside BenchmarkJsonTranslation.
  RequestInfo request_info;
  request_info.body_field_path = "*";
  request_info.variable_bindings = std::vector<RequestWeaver::BindingInfo>();

  // Using heap instead of stack because heap space is more suitable to hold large data chunks
  auto json_msg =
      absl::make_unique<std::string>(absl::StrFormat(R"({"payload" : "%s"})",
                                                     GetRandomString(
                                                         payload_length)));
  BenchmarkJsonTranslation(state,
                           kBytePayloadMessageType,
                           request_info,
                           json_msg.get(),
                           streaming,
                           stream_size);
}

static void BM_SinglePayloadFromJsonNonStreaming(::benchmark::State& state) {
  BM_SinglePayloadFromJson(state, state.range(0), false, 0);
}
BENCHMARK_WITH_PERCENTILE(BM_SinglePayloadFromJsonNonStreaming)
    ->Arg(1) // 1 byte
    ->Arg(1 << 10) // 1 KiB
    ->Arg(1 << 20) // 1 MiB
    ->Arg(1 << 25); // 32 MiB

static void BM_SinglePayloadFromJsonStreaming(::benchmark::State& state) {
  BM_SinglePayloadFromJson(state, state.range(0), true, state.range(1));
}
BENCHMARK_WITH_PERCENTILE(BM_SinglePayloadFromJsonStreaming)
    ->ArgPair(1 << 20, 1) // 1 MiB, 1 message
    ->ArgPair(1 << 20, 1 << 2) // 1 MiB, 2 messages
    ->ArgPair(1 << 20, 1 << 4) // 1 MiB, 16 messages
    ->ArgPair(1 << 20, 1 << 6); // 1 MiB, 64 messages

BENCHMARK_MAIN();

} // namespace perf_benchmark

} // namespace transcoding
} // namespace grpc
} // namespace google

