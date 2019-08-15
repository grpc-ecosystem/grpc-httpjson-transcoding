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
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

ABSEIL_COMMIT = "99477fa9f1e89a7d8253c8aeee331864710d080c"
ABSEIL_SHA256 = "495e8e1c481018126b2a84bfe36e273907ce282b135e7d161e138e463d295f3d"

def absl_repositories(bind=True):
    http_archive(
        name = "com_google_absl",
        strip_prefix = "abseil-cpp-" + ABSEIL_COMMIT,
        url = "https://github.com/abseil/abseil-cpp/archive/" + ABSEIL_COMMIT + ".tar.gz",
        sha256 = ABSEIL_SHA256,
    )

def zlib_repositories(bind = True):
    BUILD = """
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
licenses(["notice"])
exports_files(["README"])
cc_library(
    name = "zlib",
    srcs = [
        "adler32.c",
        "crc32.c",
        "crc32.h",
        "deflate.c",
        "deflate.h",
        "infback.c",
        "inffast.c",
        "inffast.h",
        "inffixed.h",
        "inflate.c",
        "inflate.h",
        "inftrees.c",
        "inftrees.h",
        "trees.c",
        "trees.h",
        "zconf.h",
        "zutil.c",
        "zutil.h",
    ],
    hdrs = [
        "zlib.h",
    ],
    copts = [
        "-Wno-shift-negative-value",
        "-Wno-unknown-warning-option",
    ],
    defines = [
        "Z_SOLO",
    ],
    visibility = [
        "//visibility:public",
    ],
)
"""
    http_archive(
        name = "zlib",
        strip_prefix = "zlib-1.2.11",
        urls = ["https://github.com/madler/zlib/archive/v1.2.11.tar.gz"],
        sha256 = "629380c90a77b964d896ed37163f5c3a34f6e6d897311f1df2a7016355c45eff",
        build_file_content = BUILD,
    )


BAZEL_SKYLIB_RELEASE = "0.8.0"
BAZEL_SKYLIB_SHA256 = "2ef429f5d7ce7111263289644d233707dba35e39696377ebab8b0bc701f7818e"

PROTOBUF_COMMIT = "3.9.0"  # July 10, 2019
PROTOBUF_SHA256 = "2ee9dcec820352671eb83e081295ba43f7a4157181dad549024d7070d079cf65"

def protobuf_repositories(bind=True):
    zlib_repositories(bind)

    http_archive(
        name = "bazel_skylib",
        urls = ["https://github.com/bazelbuild/bazel-skylib/releases/download/" + BAZEL_SKYLIB_RELEASE + "/bazel-skylib." + BAZEL_SKYLIB_RELEASE + ".tar.gz"],
        sha256 = BAZEL_SKYLIB_SHA256,
    )

    http_archive(
        name = "protobuf_git",
        strip_prefix = "protobuf-" + PROTOBUF_COMMIT,
        url = "https://github.com/google/protobuf/archive/v" + PROTOBUF_COMMIT + ".tar.gz",
        sha256 = PROTOBUF_SHA256,
    )

    if bind:
        native.bind(
            name = "protoc",
            actual = "@protobuf_git//:protoc",
        )

        native.bind(
            name = "protobuf",
            actual = "@protobuf_git//:protobuf",
        )

        native.bind(
            name = "cc_wkt_protos",
            actual = "@protobuf_git//:cc_wkt_protos",
        )

        native.bind(
            name = "cc_wkt_protos_genproto",
            actual = "@protobuf_git//:cc_wkt_protos_genproto",
        )

        native.bind(
            name = "protobuf_compiler",
            actual = "@protobuf_git//:protoc_lib",
        )

        native.bind(
            name = "protobuf_clib",
            actual = "@protobuf_git//:protoc_lib",
        )

GOOGLETEST_COMMIT = "43863938377a9ea1399c0596269e0890b5c5515a"
GOOGLETEST_SHA256 = "7c8ece456ad588c30160429498e108e2df6f42a30888b3ec0abf5d9792d9d3a0"

def googletest_repositories(bind=True):
    BUILD = """
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

cc_library(
    name = "googletest",
    srcs = [
        "googletest/src/gtest-all.cc",
        "googlemock/src/gmock-all.cc",
    ],
    hdrs = glob([
        "googletest/include/**/*.h",
        "googlemock/include/**/*.h",
        "googletest/src/*.cc",
        "googletest/src/*.h",
        "googlemock/src/*.cc",
    ]),
    includes = [
        "googlemock",
        "googletest",
        "googletest/include",
        "googlemock/include",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "googletest_main",
    srcs = ["googlemock/src/gmock_main.cc"],
    visibility = ["//visibility:public"],
    linkopts = [
        "-lpthread",
    ],
    deps = [":googletest"],
)

cc_library(
    name = "googletest_prod",
    hdrs = [
        "googletest/include/gtest/gtest_prod.h",
    ],
    includes = [
        "googletest/include",
    ],
    visibility = ["//visibility:public"],
)
"""
    http_archive(
        name = "googletest_git",
        strip_prefix = "googletest-" + GOOGLETEST_COMMIT,
        build_file_content = BUILD,
        url = "https://github.com/google/googletest/archive/" + GOOGLETEST_COMMIT + ".tar.gz",
        sha256 = GOOGLETEST_SHA256,
    )

    if bind:
        native.bind(
            name = "googletest",
            actual = "@googletest_git//:googletest",
        )

        native.bind(
            name = "googletest_main",
            actual = "@googletest_git//:googletest_main",
        )

        native.bind(
            name = "googletest_prod",
            actual = "@googletest_git//:googletest_prod",
        )

GOOGLEAPIS_COMMIT = "32a10f69e2c9ce15bba13ab1ff928bacebb25160" # May 20, 2019
GOOGLEAPIS_SHA256 = "6861efa8619579e06e70dd4765cdf6cef1ecad6a1a2026ad750541e99552bf71"

def googleapis_repositories(protobuf_repo="@protobuf_git//", bind=True):
    BUILD = """
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

licenses(["notice"])

load("{0}:protobuf.bzl", "cc_proto_library")

exports_files(glob(["google/**"]))

cc_proto_library(
    name = "http_api_protos",
    srcs = [
        "google/api/annotations.proto",
        "google/api/http.proto",
    ],
    default_runtime = "//external:protobuf",
    protoc = "//external:protoc",
    visibility = ["//visibility:public"],
    deps = ["{0}:cc_wkt_protos"],
)

cc_proto_library(
    name = "service_config",
    srcs = [
        "google/api/auth.proto",
        "google/api/backend.proto",
        "google/api/billing.proto",
        "google/api/consumer.proto",
        "google/api/context.proto",
        "google/api/control.proto",
        "google/api/documentation.proto",
        "google/api/endpoint.proto",
        "google/api/label.proto",
        "google/api/launch_stage.proto",
        "google/api/log.proto",
        "google/api/logging.proto",
        "google/api/metric.proto",
        "google/api/experimental/experimental.proto",
        "google/api/experimental/authorization_config.proto",
        "google/api/monitored_resource.proto",
        "google/api/monitoring.proto",
        "google/api/resource.proto",
        "google/api/quota.proto",
        "google/api/service.proto",
        "google/api/source_info.proto",
        "google/api/system_parameter.proto",
        "google/api/usage.proto",
    ],
    include = ".",
    visibility = ["//visibility:public"],
    deps = [
        ":http_api_protos",
        "//external:cc_wkt_protos",
    ],
    protoc = "//external:protoc",
    default_runtime = "//external:protobuf",
)
""".format(protobuf_repo)

    http_archive(
        name = "googleapis_git",
        patch_cmds = ["find . -type f -name '*BUILD*' | xargs rm"],
        strip_prefix = "googleapis-" + GOOGLEAPIS_COMMIT,
        url = "https://github.com/googleapis/googleapis/archive/" + GOOGLEAPIS_COMMIT + ".tar.gz",
        build_file_content = BUILD,
        sha256 = GOOGLEAPIS_SHA256,
    )

    if bind:
        native.bind(
            name = "service_config",
            actual = "@googleapis_git//:service_config",
        )

        native.bind(
            name = "http_api_protos",
            actual = "@googleapis_git//:http_api_protos",
        )
