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
        "prefix_writer.h",
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
        "request_weaver.h",
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
        "message_stream.h",
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
        "request_message_translator.h",
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
        "request_stream_translator.h",
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
        "json_request_translator.h",
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
        "message_reader.h",
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
        "response_to_json_translator.h",
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
        "transcoder_input_stream.h",
    ],
    deps = [
        "//external:protobuf",
    ],
)

cc_library(
    name = "transcoding",
    hdrs = [
        "transcoder.h",
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
        "type_helper.h",
    ],
    deps = [
        "//external:protobuf",
    ],
)
