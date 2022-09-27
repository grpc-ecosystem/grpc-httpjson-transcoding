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

PROTOBUF_COMMIT = "3.16.0"  # Mar 04, 2021
PROTOBUF_SHA256 = "7892a35d979304a404400a101c46ce90e85ec9e2a766a86041bb361f626247f5"

RULES_PROTO_SHA = "97d8af4dc474595af3900dd85cb3a29ad28cc313"  # Oct 31, 2019
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

GOOGLETEST_COMMIT = "703bd9caab50b139428cea1aaff9974ebee5742e"  # v1.10.0: Oct 2, 2019
GOOGLETEST_SHA256 = "d17b1b83a57b3933565a6d0616fe261107326d47de20288d0949ed038e1c342d"

def googletest_repositories(bind = True):
    http_archive(
        name = "com_google_googletest",
        strip_prefix = "googletest-" + GOOGLETEST_COMMIT,
        url = "https://github.com/google/googletest/archive/" + GOOGLETEST_COMMIT + ".tar.gz",
        sha256 = GOOGLETEST_SHA256,
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

GOOGLEBENCHMARK_COMMIT = "1.7.0"  # Jul 25, 2022
GOOGLEBENCHMARK_SHA256 = "3aff99169fa8bdee356eaa1f691e835a6e57b1efeadb8a0f9f228531158246ac"

def googlebenchmark_repositories(bind = True):
    http_archive(
        name = "com_google_benchmark",
        strip_prefix = "benchmark-" + GOOGLEBENCHMARK_COMMIT,
        url = "https://github.com/google/benchmark/archive/v" + GOOGLEBENCHMARK_COMMIT + ".tar.gz",
        sha256 = GOOGLEBENCHMARK_SHA256,
    )

def nlohmannjson_repositories(bind = True):
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
package(default_visibility = ["//visibility:public"])
cc_library(
    name = "json",
    hdrs = [
        "single_include/nlohmann/json.hpp",
    ],
    strip_include_prefix = "single_include/",
)
"""
    http_archive(
        name = "com_github_nlohmann_json",
        strip_prefix = "json-3.11.2",
        urls = ["https://github.com/nlohmann/json/archive/v3.11.2.tar.gz"],
        sha256 = "d69f9deb6a75e2580465c6c4c5111b89c4dc2fa94e3a85fcd2ffcd9a143d9273",
        build_file_content = BUILD,
    )

RULES_DOCKER_COMMIT = "0.25.0"  # Jun 22, 2022
RULES_DOCKER_SHA256 = "b1e80761a8a8243d03ebca8845e9cc1ba6c82ce7c5179ce2b295cd36f7e394bf"

def io_bazel_rules_docker(bind = True):
    http_archive(
        name = "io_bazel_rules_docker",
        sha256 = RULES_DOCKER_SHA256,
        urls = ["https://github.com/bazelbuild/rules_docker/releases/download/v" + RULES_DOCKER_COMMIT + "/rules_docker-v" + RULES_DOCKER_COMMIT + ".tar.gz"],
    )
