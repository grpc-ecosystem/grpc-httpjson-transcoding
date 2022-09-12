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
#include "gtest/gtest.h"
#include "perf_benchmark/benchmark_input_stream.h"
#include "perf_benchmark/utils.h"
#include "grpc_transcoding/json_request_translator.h"
#include "grpc_transcoding/type_helper.h"
#include "perf_benchmark/benchmark.pb.h"
#include "google/api/service.pb.h"
#include "absl/strings/str_format.h"
#include "absl/strings/escaping.h"

namespace google {
namespace grpc {
namespace transcoding {

namespace perf_benchmark {
namespace {
constexpr absl::string_view
    kServiceConfigTextProtoFile = "benchmark_service.textproto";

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
                                           uint64_t chunk_size) {
  BenchmarkZeroCopyInputStream is(std::string(json_msg), chunk_size);
  // Get message type from the global TypeHelper
  const TypeHelper& type_helper = GetBenchmarkTypeHelper();
  const google::protobuf::Type* type = type_helper.Info()->GetTypeByTypeUrl(
      absl::StrFormat("type.googleapis.com/%s", msg_type));

  RequestInfo request_info;
  // body field path used in this benchmark are all "*"
  request_info.body_field_path = "*";
  request_info.variable_bindings = std::vector<RequestWeaver::BindingInfo>();
  request_info.message_type = type;

  std::string message;
  JsonRequestTranslator translator
      (type_helper.Resolver(), &is, request_info, false, false);
  MessageStream& out = translator.Output();
  EXPECT_TRUE(out.Status().ok());

  // Read the message
  while (out.NextMessage(&message)) {
  }
  return message;
}

// class T protobuf class payload field type needs to support parsing 0 such as
// int32, string, and double.
template<class T>
void IntegrationWithJsonRequestTranslatorArrayProtoHelper(absl::string_view msg_type) {
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

} // namespace

TEST(BenchmarkInputStreamTest, BenchmarkZeroCopyInputStreamSimple
) {
absl::string_view json_msg_input[] =
    {R"({"Hello":"World!"})",
     R"([{"Hello":"World!"}])",
     R"([{"Hello":"World!"},{"Hello":"World, Again!"}])"};

for (
auto& json_msg
: json_msg_input) {
BenchmarkZeroCopyInputStream is(std::string(json_msg), 1);

// TotalBytes and BytesAvailable should equal to json_msg.
EXPECT_EQ(is
.
TotalBytes(), json_msg
.
size()
);
EXPECT_EQ(is
.
BytesAvailable(), json_msg
.
size()
);
// Stream should not be finished.
EXPECT_FALSE(is
.
Finished()
);

// Reading data.
const void* data = nullptr;
int size;
is.
Next(& data, & size
);
EXPECT_EQ(size, json_msg
.
size()
);
EXPECT_EQ(std::string(static_cast<const char*>(data)), json_msg
);

// Stream should be finished
EXPECT_TRUE(is
.
Finished()
);

// Reset should reset everything as if Next() is not called.
is.
Reset();
EXPECT_EQ(is
.
TotalBytes(), json_msg
.
size()
);
EXPECT_EQ(is
.
BytesAvailable(), json_msg
.
size()
);
EXPECT_FALSE(is
.
Finished()
);
}
}

TEST(BenchmarkInputStreamTest, BenchmarkZeroCopyInputStreamChunk
) {
absl::string_view json_msg = R"({"Hello":"World!"})";
const uint64_t
    chunk_per_msg_input[] = {1, 2, 4, json_msg.size() - 1, json_msg.size()};

for (
uint64_t chunk_per_msg
: chunk_per_msg_input) {
BenchmarkZeroCopyInputStream is(std::string(json_msg), chunk_per_msg);
uint64_t expected_chunk_size = json_msg.size() / chunk_per_msg;

// Reading data.
const void* data = nullptr;
int size;
uint64_t total_bytes_read = 0;
std::string str_read;
while (!is.
Finished()
) {
// TotalBytes should equal to json_msg.
EXPECT_EQ(is
.
TotalBytes(), json_msg
.
size()
);
if (json_msg.
size()
- total_bytes_read >= expected_chunk_size) {
// BytesAvailable should equal to the chunk size unless we are reading
// the last message.
EXPECT_EQ(is
.
BytesAvailable(), expected_chunk_size
);
}

is.
Next(& data, & size
);
total_bytes_read +=
size;
str_read += std::string(static_cast
<const char*>(data), size
);

if (json_msg.
size()
- total_bytes_read >= expected_chunk_size) {
// size should equal to the expected_chunk_size unless it's the last
// message.
EXPECT_EQ(size, expected_chunk_size
);
}
if (total_bytes_read == json_msg.
size()
) {
// Stream should be finished
EXPECT_TRUE(is
.
Finished()
);
}
}
EXPECT_EQ(total_bytes_read, json_msg
.
size()
);
EXPECT_EQ(str_read, json_msg
);

// Reset should reset everything as if Next() is not called.
is.
Reset();
EXPECT_EQ(is
.
TotalBytes(), json_msg
.
size()
);
EXPECT_EQ(is
.
BytesAvailable(), expected_chunk_size
);
EXPECT_FALSE(is
.
Finished()
);
}
}

TEST(BenchmarkInputStreamTest, IntegrationWithJsonRequestTranslatorBytesProto
) {
// JSON message containing "Hello World!" encoded in base64 string.
absl::string_view json_msg = R"({"payload":"SGVsbG8gV29ybGQh"})";
absl::string_view expected_decoded_payload = "Hello World!";

const std::string proto_str =
    ParseJsonMessageToProtoMessage(json_msg, "BytesPayload", 1);

// Verification - decoded message should equal the encoded one.
BytesPayload actual_proto;
actual_proto.
ParseFromString(proto_str);
EXPECT_EQ(expected_decoded_payload, actual_proto
.
payload()
);
}

TEST(BenchmarkInputStreamTest, IntegrationWithJsonRequestTranslatorArrayProto
) {
IntegrationWithJsonRequestTranslatorArrayProtoHelper<Int32ArrayPayload>(
"Int32ArrayPayload");
IntegrationWithJsonRequestTranslatorArrayProtoHelper<DoubleArrayPayload>(
"DoubleArrayPayload");
IntegrationWithJsonRequestTranslatorArrayProtoHelper<StringArrayPayload>(
"StringArrayPayload");
}

TEST(BenchmarkInputStreamTest,
    IntegrationWithJsonRequestTranslatorNestedProto
) {
// JSON message containing 0 and 2 layers of nesetd payload message
absl::string_view expected_payload = "Hello World!";
absl::string_view nested_field_name = "nested";
const std::string zero_layer_json_msg =
    GetNestedJsonString(0, nested_field_name, "payload", expected_payload);
const std::string two_layers_json_msg =
    GetNestedJsonString(2, nested_field_name, "payload", expected_payload);

const std::string zero_layer_proto_str =
    ParseJsonMessageToProtoMessage(zero_layer_json_msg, "NestedPayload", 1);
const std::string two_layer_proto_str =
    ParseJsonMessageToProtoMessage(two_layers_json_msg, "NestedPayload", 1);

EXPECT_EQ(GetNestedProtoLayer(zero_layer_proto_str),
0);
EXPECT_EQ(GetNestedProtoLayer(two_layer_proto_str),
2);
}

TEST(BenchmarkInputStreamTest,
    IntegrationWithJsonRequestTranslatorSstructProto
) {
absl::string_view expected_payload = "Hello World!";
absl::string_view nested_field_name = "nested";
const std::string zero_layer_json_msg =
    GetNestedJsonString(0, nested_field_name, "payload", expected_payload);
const std::string two_layers_json_msg =
    GetNestedJsonString(2, nested_field_name, "payload", expected_payload);

const std::string zero_layer_proto_str =
    ParseJsonMessageToProtoMessage(zero_layer_json_msg,
                                   "google.protobuf.Struct",
                                   1);
const std::string two_layer_proto_str =
    ParseJsonMessageToProtoMessage(two_layers_json_msg,
                                   "google.protobuf.Struct",
                                   1);

EXPECT_EQ(GetStructProtoLayer(zero_layer_proto_str,
                              std::string(nested_field_name)),
0);
EXPECT_EQ(GetStructProtoLayer(two_layer_proto_str,
                              std::string(nested_field_name)),
2);
}

TEST(BenchmarkInputStreamTest,
    IntegrationWithJsonRequestTranslatorChunkMessage
) {
// JSON message containing "Hello World!"
absl::string_view expected_payload = "Hello World!";
const std::string
    json_msg = absl::StrFormat(R"({"payload":"%s"})", expected_payload);
uint64_t chunk_size_input[] = {1, 2, 4, 8};

for (
uint64_t chunk_size
: chunk_size_input) {
std::string proto_str =
    ParseJsonMessageToProtoMessage(json_msg, "StringPayload", chunk_size);

// Verification - decoded message should equal the encoded one.
StringPayload actual_proto;
actual_proto.
ParseFromString(proto_str);
EXPECT_EQ(expected_payload, actual_proto
.
payload()
);
}
}

} // namespace perf_benchmark

} // namespace transcoding
} // namespace grpc
} // namespace google