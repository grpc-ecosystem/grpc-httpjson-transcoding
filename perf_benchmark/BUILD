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
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

cc_proto_library(
    name = "benchmark_cc_proto",
    deps = [":benchmark_proto"],
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
        ":benchmark_cc_proto",
        ":benchmark_common",
        "//src:json_request_translator",
        "//src:type_helper",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/memory",
        "@com_google_benchmark//:benchmark",
        "@com_google_benchmark//:benchmark_main",
        "@com_google_googleapis//google/api:service_cc_proto",
    ],
    copts = ["-BENCHMARK_ENABLE_LIBPFM"]
)

cc_library(
    name = "benchmark_common",
    srcs = ["benchmark_common.cc"],
    hdrs = ["benchmark_common.h"],
    deps = [
        "//src:transcoder_input_stream",
        "@com_google_absl//absl/strings",
        "@com_google_googleapis//google/api:service_cc_proto",
    ],
)
