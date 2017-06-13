# Copyright 2016 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
################################################################################
#
package(default_visibility = ["//visibility:public"])

cc_library(
    name = "prefix_writer",
    srcs = [
        "prefix_writer.cc",
    ],
    hdrs = [
        "include/grpc_transcoding/prefix_writer.h",
    ],
    includes = [
        "include/"
    ],
    deps = [
        "//external:protobuf",
    ],
)

cc_library(
    name = "request_weaver",
    srcs = [
        "request_weaver.cc",
    ],
    hdrs = [
        "include/grpc_transcoding/request_weaver.h",
    ],
    includes = [
        "include/"
    ],
    deps = [
        "//external:protobuf",
    ],
)

cc_library(
    name = "message_stream",
    srcs = [
        "message_stream.cc",
    ],
    hdrs = [
        "include/grpc_transcoding/message_stream.h",
    ],
    includes = [
        "include/"
    ],
    deps = [
        ":transcoder_input_stream",
        "//external:protobuf",
    ],
)

cc_library(
    name = "request_message_translator",
    srcs = [
        "request_message_translator.cc",
    ],
    hdrs = [
        "include/grpc_transcoding/request_message_translator.h",
    ],
    includes = [
        "include/"
    ],
    deps = [
        ":message_stream",
        ":prefix_writer",
        ":request_weaver",
        "//external:protobuf",
    ],
)

cc_library(
    name = "request_stream_translator",
    srcs = [
        "request_stream_translator.cc",
    ],
    hdrs = [
        "include/grpc_transcoding/request_stream_translator.h",
    ],
    includes = [
        "include/"
    ],
    deps = [
        ":request_message_translator",
        "//external:protobuf",
    ],
)

cc_library(
    name = "path_matcher",
    srcs = [
        "path_matcher_node.cc",
        "path_matcher_node.h",
    ],
    hdrs = [
        "path_matcher.h",
    ],
    deps = [
        ":http_template",
    ],
)

cc_library(
    name = "http_template",
    srcs = [
        "http_template.cc",
    ],
    hdrs = [
        "http_template.h",
    ],
)

cc_library(
    name = "json_request_translator",
    srcs = [
        "json_request_translator.cc",
    ],
    hdrs = [
        "include/grpc_transcoding/json_request_translator.h",
    ],
    includes = [
        "include/"
    ],
    deps = [
        ":request_message_translator",
        ":request_stream_translator",
        "//external:protobuf",
    ],
)

cc_library(
    name = "message_reader",
    srcs = [
        "message_reader.cc",
    ],
    hdrs = [
        "include/grpc_transcoding/message_reader.h",
    ],
    includes = [
        "include/"
    ],
    deps = [
        ":transcoder_input_stream",
        "//external:protobuf",
    ],
)

cc_library(
    name = "response_to_json_translator",
    srcs = [
        "response_to_json_translator.cc",
    ],
    hdrs = [
        "include/grpc_transcoding/response_to_json_translator.h",
    ],
    includes = [
        "include/"
    ],
    deps = [
        ":message_reader",
        ":message_stream",
        "//external:protobuf",
    ],
)

cc_library(
    name = "transcoder_input_stream",
    srcs = [
        "include/grpc_transcoding/transcoder_input_stream.h",
    ],
    includes = [
        "include/"
    ],
    deps = [
        "//external:protobuf",
    ],
)

cc_library(
    name = "transcoding",
    hdrs = [
        "include/grpc_transcoding/transcoder.h",
    ],
    includes = [
        "include/"
    ],
    deps = [
        ":json_request_translator",
        ":message_stream",
        ":response_to_json_translator",
        ":type_helper",
        "//external:protobuf",
    ],
)

cc_library(
    name = "type_helper",
    srcs = [
        "type_helper.cc",
    ],
    hdrs = [
        "include/grpc_transcoding/type_helper.h",
    ],
    includes = [
        "include/"
    ],
    deps = [
        "//external:protobuf",
    ],
)
