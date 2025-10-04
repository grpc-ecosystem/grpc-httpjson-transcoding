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

def absl_repositories():
    http_archive(
        name = "com_google_absl",
        sha256 = "f49929d22751bf70dd61922fb1fd05eb7aec5e7a7f870beece79a6e28f0a06c1",
        strip_prefix = "abseil-cpp-4a2c63365eff8823a5221db86ef490e828306f9d",  # Abseil LTS 20240116.0
        urls = ["https://github.com/abseil/abseil-cpp/archive/4a2c63365eff8823a5221db86ef490e828306f9d.zip"],
    )

PROTOBUF_COMMIT = "b407e8416e3893036aee5af9a12bd9b6a0e2b2e6"  # v29.3: Oct 2, 2025
PROTOBUF_SHA256 = "55912546338433f465a552e9ef09930c63b9eb697053937416890cff83a8622d"

def protobuf_repositories():
    http_archive(
        name = "com_google_protobuf",
        strip_prefix = "protobuf-" + PROTOBUF_COMMIT,
        urls = [
            "https://github.com/google/protobuf/archive/" + PROTOBUF_COMMIT + ".tar.gz",
        ],
        sha256 = PROTOBUF_SHA256,
    )

GOOGLETEST_COMMIT = "f8d7d77c06936315286eb55f8de22cd23c188571"  # v1.14.0: Aug 2, 2023
GOOGLETEST_SHA256 = "7ff5db23de232a39cbb5c9f5143c355885e30ac596161a6b9fc50c4538bfbf01"

def googletest_repositories():
    http_archive(
        name = "com_google_googletest",
        strip_prefix = "googletest-" + GOOGLETEST_COMMIT,
        url = "https://github.com/google/googletest/archive/" + GOOGLETEST_COMMIT + ".tar.gz",
        sha256 = GOOGLETEST_SHA256,
    )

GOOGLEAPIS_COMMIT = "a92cee399e0fc8afa2d460373b1085f543bc8d3f"  # Aug 26, 2025
GOOGLEAPIS_SHA256 = "468056c3244b7a4f6a575a135b6b6dde280a3f219203a01c4a09d0cf504a4ba6"

def googleapis_repositories():
    http_archive(
        name = "com_google_googleapis",
        strip_prefix = "googleapis-" + GOOGLEAPIS_COMMIT,
        url = "https://github.com/googleapis/googleapis/archive/" + GOOGLEAPIS_COMMIT + ".tar.gz",
        sha256 = GOOGLEAPIS_SHA256,
    )

GOOGLEBENCHMARK_COMMIT = "1.8.3"  # Jan 11, 2024
GOOGLEBENCHMARK_SHA256 = "6bc180a57d23d4d9515519f92b0c83d61b05b5bab188961f36ac7b06b0d9e9ce"

def googlebenchmark_repositories():
    http_archive(
        name = "com_google_benchmark",
        strip_prefix = "benchmark-" + GOOGLEBENCHMARK_COMMIT,
        url = "https://github.com/google/benchmark/archive/v" + GOOGLEBENCHMARK_COMMIT + ".tar.gz",
        sha256 = GOOGLEBENCHMARK_SHA256,
    )

def nlohmannjson_repositories():
    http_archive(
        name = "com_github_nlohmann_json",
        strip_prefix = "json-3.11.3",
        urls = ["https://github.com/nlohmann/json/archive/v3.11.3.tar.gz"],
        sha256 = "0d8ef5af7f9794e3263480193c491549b2ba6cc74bb018906202ada498a79406",
    )

RULES_DOCKER_COMMIT = "0.25.0"  # Jun 22, 2022
RULES_DOCKER_SHA256 = "b1e80761a8a8243d03ebca8845e9cc1ba6c82ce7c5179ce2b295cd36f7e394bf"

def io_bazel_rules_docker():
    http_archive(
        name = "io_bazel_rules_docker",
        sha256 = RULES_DOCKER_SHA256,
        urls = ["https://github.com/bazelbuild/rules_docker/releases/download/v" + RULES_DOCKER_COMMIT + "/rules_docker-v" + RULES_DOCKER_COMMIT + ".tar.gz"],
    )

def protoconverter_repositories():
    http_archive(
        name = "com_google_protoconverter",
        sha256 = "17c49df769dcd505a57fc99136075f1a1f8418ab03914faf7af692772ce54d54",
        strip_prefix = "proto-converter-2458ed8ea405b47c1960f0b0af211efdf0e057a0",
        urls = ["https://github.com/mmorel-35/proto-converter/archive/2458ed8ea405b47c1960f0b0af211efdf0e057a0.zip"],
    )
