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

load("@protobuf_git//:protobuf.bzl", "cc_proto_library")

cc_test(
    name = "http_template_test",
    size = "small",
    srcs = [
        "http_template_test.cc",
    ],
    linkstatic = 1,
    deps = [
        "//external:googletest_main",
        "//src:http_template",
    ],
)

cc_test(
    name = "path_matcher_test",
    size = "small",
    srcs = [
        "path_matcher_test.cc",
    ],
    linkstatic = 1,
    deps = [
        "//external:googletest_main",
        "//src:path_matcher",
    ],
)

cc_test(
    name = "path_matcher_utility_test",
    size = "small",
    srcs = [
        "path_matcher_utility_test.cc",
    ],
    linkstatic = 1,
    deps = [
        "//external:googletest_main",
        "//src:path_matcher_utility",
    ],
)

cc_test(
    name = "prefix_writer_test",
    size = "small",
    srcs = [
        "prefix_writer_test.cc",
    ],
    deps = [
        "//external:googletest_main",
        "//src:prefix_writer",
    ],
)

cc_test(
    name = "request_weaver_test",
    size = "small",
    srcs = [
        "request_weaver_test.cc",
    ],
    deps = [
        "//external:googletest_main",
        "//src:request_weaver",
    ],
)

cc_proto_library(
    name = "bookstore_test_proto",
    testonly = 1,
    srcs = ["bookstore.proto"],
    include = ".",
    default_runtime = "//external:protobuf",
    protoc = "//external:protoc",
    deps = [
        "//external:cc_wkt_protos",
    ],
)

cc_test(
    name = "type_helper_test",
    size = "small",
    srcs = [
        "type_helper_test.cc",
    ],
    data = [
        "testdata/bookstore_service.pb.txt",
    ],
    deps = [
        ":test_common",
        "//external:googletest_main",
        "//external:service_config",
        "//src:type_helper",
    ],
)

cc_library(
    name = "test_common",
    testonly = 1,
    srcs = ["test_common.cc"],
    hdrs = ["test_common.h"],
    deps = [
        "//external:googletest",
        "//external:protobuf",
        "//external:service_config",
        "//src:transcoder_input_stream",
    ],
)

cc_library(
    name = "request_translator_test_base",
    testonly = 1,
    srcs = [
        "proto_stream_tester.cc",
        "proto_stream_tester.h",
        "request_translator_test_base.cc",
    ],
    hdrs = [
        "request_translator_test_base.h",
    ],
    deps = [
        ":bookstore_test_proto",
        ":test_common",
        "//external:googletest",
        "//external:protobuf",
        "//external:service_config",
        "//src:request_message_translator",
        "//src:type_helper",
    ],
)

cc_test(
    name = "request_message_translator_test",
    size = "small",
    srcs = [
        "request_message_translator_test.cc",
    ],
    data = [
        "testdata/bookstore_service.pb.txt",
    ],
    deps = [
        ":bookstore_test_proto",
        ":request_translator_test_base",
        ":test_common",
        "//external:googletest_main",
        "//src:request_message_translator",
    ],
)

cc_test(
    name = "request_stream_translator_test",
    size = "small",
    srcs = [
        "request_stream_translator_test.cc",
    ],
    data = [
        "testdata/bookstore_service.pb.txt",
    ],
    deps = [
        ":bookstore_test_proto",
        ":request_translator_test_base",
        "//external:googletest_main",
        "//src:request_stream_translator",
    ],
)

cc_test(
    name = "json_request_translator_test",
    size = "small",
    srcs = [
        "json_request_translator_test.cc",
    ],
    data = [
        "testdata/bookstore_service.pb.txt",
    ],
    deps = [
        ":bookstore_test_proto",
        ":request_translator_test_base",
        ":test_common",
        "//external:googletest_main",
        "//src:json_request_translator",
    ],
)

cc_test(
    name = "message_reader_test",
    size = "small",
    srcs = [
        "message_reader_test.cc",
    ],
    deps = [
        ":test_common",
        "//external:googletest_main",
        "//src:message_reader",
    ],
)

cc_test(
    name = "response_to_json_translator_test",
    size = "small",
    srcs = [
        "response_to_json_translator_test.cc",
    ],
    data = [
        "testdata/bookstore_service.pb.txt",
    ],
    deps = [
        ":bookstore_test_proto",
        ":test_common",
        "//external:googletest_main",
        "//src:message_reader",
        "//src:response_to_json_translator",
        "//src:type_helper",
    ],
)

cc_test(
    name = "message_stream_test",
    size = "small",
    srcs = [
        "message_stream_test.cc",
    ],
    deps = [
        ":test_common",
        "//external:googletest_main",
        "//src:message_stream",
    ],
)
