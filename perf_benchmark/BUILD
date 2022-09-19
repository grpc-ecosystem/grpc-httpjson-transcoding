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
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

cc_proto_library(
    name = "benchmark_cc_proto",
    deps = [":benchmark_proto"],
)

cc_proto_library(
    name = "com_google_protobuf_struct_cc_proto",
    deps = ["@com_google_protobuf//:struct_proto"],
)

proto_library(
    name = "benchmark_proto",
    srcs = ["benchmark.proto"],
    deps = [
        "@com_google_googleapis//google/api:annotations_proto",
        "@com_google_protobuf//:struct_proto",
    ],
)

cc_binary(
    name = "benchmark_main",
    srcs = ["benchmark_main.cc"],
    data = [
        "benchmark_service.textproto",
    ],
    deps = [
        "benchmark_cc_proto",
        ":benchmark_input_stream",
        ":utils",
        "//src:json_request_translator",
        "//src:response_to_json_translator",
        "//src:type_helper",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/random",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_benchmark//:benchmark",
        "@com_google_benchmark//:benchmark_main",
        "@com_google_googleapis//google/api:service_cc_proto",
    ],
)

cc_library(
    name = "utils",
    srcs = ["utils.cc"],
    hdrs = ["utils.h"],
    deps = [
        "benchmark_cc_proto",
        "//src:type_helper",
        "@com_github_nlohmann_json//:json",
        "@com_google_absl//absl/random",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_googleapis//google/api:service_cc_proto",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "benchmark_input_stream",
    srcs = ["benchmark_input_stream.cc"],
    hdrs = ["benchmark_input_stream.h"],
    deps = [
        "//src:transcoder_input_stream",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "utils_test",
    srcs = [
        "utils_test.cc",
    ],
    deps = [
        ":utils",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "benchmark_input_stream_test",
    srcs = [
        "benchmark_input_stream_test.cc",
    ],
    data = [
        "benchmark_service.textproto",
    ],
    deps = [
        ":benchmark_input_stream",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "benchmark_input_stream_translator_integration_test",
    srcs = [
        "benchmark_input_stream_translator_integration_test.cc",
    ],
    data = [
        "benchmark_service.textproto",
    ],
    deps = [
        "com_google_protobuf_struct_cc_proto",
        ":benchmark_cc_proto",
        ":benchmark_input_stream",
        ":utils",
        "//src:json_request_translator",
        "//src:response_to_json_translator",
        "//src:type_helper",
        "@com_github_nlohmann_json//:json",
        "@com_google_googleapis//google/api:service_cc_proto",
        "@com_google_googletest//:gtest_main",
    ],
)
