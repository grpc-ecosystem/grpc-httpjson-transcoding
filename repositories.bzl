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

def absl_repositories(bind = True):
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

PROTOBUF_COMMIT = "3.10.1"  # Oct 29, 2019
PROTOBUF_SHA256 = "6adf73fd7f90409e479d6ac86529ade2d45f50494c5c10f539226693cb8fe4f7"

RULES_PROTO_SHA = "97d8af4dc474595af3900dd85cb3a29ad28cc313"
RULES_PROTO_SHA256 = "602e7161d9195e50246177e7c55b2f39950a9cf7366f74ed5f22fd45750cd208"

def protobuf_repositories(bind = True):
    http_archive(
        name = "com_google_protobuf",
        strip_prefix = "protobuf-" + PROTOBUF_COMMIT,
        url = "https://github.com/google/protobuf/archive/v" + PROTOBUF_COMMIT + ".tar.gz",
        sha256 = PROTOBUF_SHA256,
    )

    http_archive(
        name = "rules_proto",
        sha256 = RULES_PROTO_SHA256,
        strip_prefix = "rules_proto-" + RULES_PROTO_SHA,
        urls = [
            "https://mirror.bazel.build/github.com/bazelbuild/rules_proto/archive/" + RULES_PROTO_SHA + ".tar.gz",
            "https://github.com/bazelbuild/rules_proto/archive/" + RULES_PROTO_SHA + ".tar.gz",
        ],
    )

GOOGLETEST_COMMIT = "43863938377a9ea1399c0596269e0890b5c5515a"
GOOGLETEST_SHA256 = "7c8ece456ad588c30160429498e108e2df6f42a30888b3ec0abf5d9792d9d3a0"

def googletest_repositories(bind = True):
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

GOOGLEAPIS_COMMIT = "1d5522ad1056f16a6d593b8f3038d831e64daeea"  # Sept 03, 2020
GOOGLEAPIS_SHA256 = "cd13e547cffaad217c942084fd5ae0985a293d0cce3e788c20796e5e2ea54758"

def googleapis_repositories(bind = True):
    http_archive(
        name = "com_google_googleapis",
        strip_prefix = "googleapis-" + GOOGLEAPIS_COMMIT,
        url = "https://github.com/googleapis/googleapis/archive/" + GOOGLEAPIS_COMMIT + ".tar.gz",
        sha256 = GOOGLEAPIS_SHA256,
    )
