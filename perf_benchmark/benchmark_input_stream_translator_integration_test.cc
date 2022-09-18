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
#include "perf_benchmark/benchmark_input_stream.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_format.h"
#include "google/api/service.pb.h"
#include "grpc_transcoding/json_request_translator.h"
#include "grpc_transcoding/type_helper.h"
#include "gtest/gtest.h"
#include "perf_benchmark/benchmark.pb.h"
#include "perf_benchmark/utils.h"

namespace google {
namespace grpc {
namespace transcoding {

namespace perf_benchmark {
namespace {
constexpr absl::string_view kServiceConfigTextProtoFile =
    "benchmark_service.textproto";

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

std::string ParseJsonMessageToProtoMessage(absl::string_view json_msg,
                                           absl::string_view msg_type,
                                           uint64_t num_checks,
                                           RequestInfo request_info = {}) {
  BenchmarkZeroCopyInputStream is(std::string(json_msg), num_checks);
  // Get message type from the global TypeHelper
  const TypeHelper& type_helper = GetBenchmarkTypeHelper();
  const google::protobuf::Type* type = type_helper.Info()->GetTypeByTypeUrl(
      absl::StrFormat("type.googleapis.com/%s", msg_type));

  // body field path used in this benchmark are all "*"
  request_info.body_field_path = "*";
  request_info.message_type = type;

  std::string message;
  JsonRequestTranslator translator(type_helper.Resolver(), &is, request_info,
                                   false, false);
  MessageStream& out = translator.Output();
  EXPECT_TRUE(out.Status().ok());

  // Read the message
  while (out.NextMessage(&message)) {
  }
  return message;
}

// class T protobuf class payload field type needs to support parsing 0 such as
// int32, string, and double.
template <class T>
void IntegrationWithJsonRequestTranslatorArrayProtoHelper(
    absl::string_view msg_type) {
  // JSON message containing an array of 3 zeros
  absl::string_view json_msg = R"({"payload":["0","0","0"]})";
  const uint64_t arr_length = 3;

  std::string proto_str = ParseJsonMessageToProtoMessage(json_msg, msg_type, 1);

  // Verification - array length should be 3
  T actual_proto;
  actual_proto.ParseFromString(proto_str);
  EXPECT_EQ(actual_proto.payload().size(), arr_length);
}

uint64_t GetNestedProtoLayer(std::string proto_msg) {
  NestedPayload actual_proto;
  actual_proto.ParseFromString(proto_msg);

  uint64_t actual_layers = 0;
  const NestedPayload* it = &actual_proto;
  while (it->has_nested()) {
    ++actual_layers;
    it = &(it->nested());
  }
  return actual_layers;
}

uint64_t GetStructProtoLayer(std::string proto_msg, std::string field_name) {
  ::google::protobuf::Struct actual_proto;
  actual_proto.ParseFromString(proto_msg);

  uint64_t actual_layers = 0;
  const ::google::protobuf::Struct* it = &actual_proto;
  while (it->fields().contains(field_name)) {
    ++actual_layers;
    it = &(it->fields().at(field_name).struct_value());
  }
  return actual_layers;
}

}  // namespace

TEST(BenchmarkInputStreamTest, IntegrationWithJsonRequestTranslatorBytesProto) {
  // JSON message containing "Hello World!" encoded in base64 string.
  absl::string_view json_msg = R"({"payload":"SGVsbG8gV29ybGQh"})";
  absl::string_view expected_decoded_payload = "Hello World!";

  const std::string proto_str =
      ParseJsonMessageToProtoMessage(json_msg, "BytesPayload", 1);

  // Verification - decoded message should equal the encoded one.
  BytesPayload actual_proto;
  actual_proto.ParseFromString(proto_str);
  EXPECT_EQ(expected_decoded_payload, actual_proto.payload());
}

TEST(BenchmarkInputStreamTest, IntegrationWithJsonRequestTranslatorArrayProto) {
  IntegrationWithJsonRequestTranslatorArrayProtoHelper<Int32ArrayPayload>(
      "Int32ArrayPayload");
  IntegrationWithJsonRequestTranslatorArrayProtoHelper<DoubleArrayPayload>(
      "DoubleArrayPayload");
  IntegrationWithJsonRequestTranslatorArrayProtoHelper<StringArrayPayload>(
      "StringArrayPayload");
}

TEST(BenchmarkInputStreamTest,
     IntegrationWithJsonRequestTranslatorNestedProto) {
  absl::string_view nested_field_name = "nested";
  uint64_t num_nested_layer_input[] = {0, 1, 2, 4, 8, 16, 32};
  for (uint64_t num_nested_layer : num_nested_layer_input) {
    const std::string json_msg = GetNestedJsonString(
        num_nested_layer, nested_field_name, "payload", "Hello World!");
    const std::string proto_str =
        ParseJsonMessageToProtoMessage(json_msg, "NestedPayload", 1);

    EXPECT_EQ(GetNestedProtoLayer(proto_str), num_nested_layer);
  }
}

TEST(BenchmarkInputStreamTest,
     IntegrationWithJsonRequestTranslatorStructProto) {
  absl::string_view nested_field_name = "nested";
  uint64_t num_nested_layer_input[] = {0, 1, 2, 4, 8, 16, 32};
  for (uint64_t num_nested_layer : num_nested_layer_input) {
    const std::string json_msg = GetNestedJsonString(
        num_nested_layer, nested_field_name, "payload", "Hello World!");
    const std::string proto_str =
        ParseJsonMessageToProtoMessage(json_msg, "google.protobuf.Struct", 1);

    EXPECT_EQ(GetStructProtoLayer(proto_str, std::string(nested_field_name)),
              num_nested_layer);
  }
}

TEST(BenchmarkInputStreamTest,
     IntegrationWithJsonRequestTranslatorChunkMessage) {
  // JSON message containing "Hello World!"
  absl::string_view expected_payload = "Hello World!";
  const std::string json_msg =
      absl::StrFormat(R"({"payload":"%s"})", expected_payload);
  uint64_t num_checks_input[] = {1, 2, 4, 8};

  for (uint64_t num_checks : num_checks_input) {
    std::string proto_str =
        ParseJsonMessageToProtoMessage(json_msg, "StringPayload", num_checks);

    // Verification - decoded message should equal the encoded one.
    StringPayload actual_proto;
    actual_proto.ParseFromString(proto_str);
    EXPECT_EQ(expected_payload, actual_proto.payload());
  }
}

TEST(BenchmarkInputStreamTest,
     IntegrationWithJsonRequestTranslatorNestedVariableBinding) {
  absl::string_view nested_field_name = "nested";
  uint64_t num_nested_layer_input[] = {0, 1, 2, 4, 8, 16, 32};
  for (uint64_t num_nested_layer : num_nested_layer_input) {
    // Variable value comes from binding, so an empty string is fine.
    const std::string json_msg = "{}";

    // Build the field_path bindings.
    // First, build the dot delimited binding string based on the number of
    // layers
    std::string field_path_str;
    for (uint64_t i = 0; i < num_nested_layer; ++i) {
      // Append the nested field name and a dot delimiter for each layer
      absl::StrAppend(&field_path_str, nested_field_name, ".");
    }
    // Append the actual payload field name
    absl::StrAppend(&field_path_str, "payload");

    // Second, parse the field_path object from the string
    const TypeHelper& type_helper = GetBenchmarkTypeHelper();
    const google::protobuf::Type* type = type_helper.Info()->GetTypeByTypeUrl(
        "type.googleapis.com/NestedPayload");
    auto field_path =
        ParseFieldPath(*type, *type_helper.Info(), field_path_str);

    // Finally, construct the RequestInfo object containing the binding.
    // We only need to fill in variable_bindings, other fields are filled in
    // by BenchmarkJsonTranslation().
    RequestInfo request_info;
    request_info.variable_bindings = {
        RequestWeaver::BindingInfo{field_path, "Hello World!"}};

    const std::string proto_str = ParseJsonMessageToProtoMessage(
        json_msg, "NestedPayload", 1, request_info);

    EXPECT_EQ(GetNestedProtoLayer(proto_str), num_nested_layer);
  }
}

}  // namespace perf_benchmark

}  // namespace transcoding
}  // namespace grpc
}  // namespace google