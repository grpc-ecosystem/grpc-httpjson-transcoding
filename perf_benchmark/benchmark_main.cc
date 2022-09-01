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
// TODO Generalize this method
static void BM_SinglePayloadFromJson(::benchmark::State& state) {
  // Load service config proto into Service object
  google::api::Service service;
  LoadService(std::string(kServiceConfigTextProtoFile), &service);

  // Create a TypeHelper based on the service config
  TypeHelper type_helper(service.types(), service.enums());

  // Get message type
  absl::string_view message_type = kBytePayloadMessageType;
  const google::protobuf::Type* type = type_helper.Info()->GetTypeByTypeUrl(
      absl::StrFormat("type.googleapis.com/%s", message_type));
  if (nullptr == type) {
    std::cerr << "Could not resolve the message type " << message_type
              << std::endl;
  }

  // Generate ZeroCopyInputStream from a random string payload
  // Using heap instead of stack because heap space is more suitable to hold large data chunks
  int64_t payload_length = state.range(0);
  auto json_msg =
      absl::make_unique<std::string>(absl::StrFormat(R"({"payload" : "%s"})",
                                                     GetRandomString(
                                                         payload_length)));
  auto is = absl::make_unique<BenchmarkZeroCopyInputStream>(*json_msg);

  // Populate request info
  RequestInfo request_info;
  request_info.message_type = type;
  request_info.body_field_path = "*";
  request_info.variable_bindings = std::vector<RequestWeaver::BindingInfo>();

  // Benchmark the transcoding process
  for (auto s: state) {
    JsonRequestTranslator
        translator
        (type_helper.Resolver(), is.get(), request_info, false, false);
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
  auto message_processed = static_cast<int64_t>(state.iterations());
  auto bytes_processed =
      static_cast<int64_t>(state.iterations()) * state.range(0);
  state.counters["byte_throughput"] =
      Counter(static_cast<double>(bytes_processed),
              Counter::kIsRate,
              Counter::kIs1024);
  state.counters["byte_latency"] = Counter(static_cast<double>(bytes_processed),
                                           Counter::kIsRate | Counter::kInvert,
                                           Counter::kIs1024);
  state.counters["message_throughput"] =
      Counter(static_cast<double>(message_processed), Counter::kIsRate);
  state.counters["message_latency"] =
      Counter(static_cast<double>(message_processed),
              Counter::kIsRate | Counter::kInvert);
}

BENCHMARK(BM_SinglePayloadFromJson)
    ->ComputeStatistics("p25", [](const std::vector<double>& v) -> double {
      return GetPercentile(v, 25);
    })
    ->ComputeStatistics("p75", [](const std::vector<double>& v) -> double {
      return GetPercentile(v, 75);
    })
    ->ComputeStatistics("p90", [](const std::vector<double>& v) -> double {
      return GetPercentile(v, 90);
    })
    ->ComputeStatistics("p99", [](const std::vector<double>& v) -> double {
      return GetPercentile(v, 99);
    })
    ->ComputeStatistics("p999", [](const std::vector<double>& v) -> double {
      return GetPercentile(v, 99.9);
    })
    ->Iterations(1) // Run for 1000 iterations for all cases
    ->Arg(1) // 1 byte
    ->Arg(1 << 10) // 1 KiB
    ->Arg(1 << 20) // 1 MiB
    ->Arg(1 << 25); // 32 MiB
BENCHMARK_MAIN();

} // namespace perf_benchmark

} // namespace transcoding
} // namespace grpc
} // namespace google

