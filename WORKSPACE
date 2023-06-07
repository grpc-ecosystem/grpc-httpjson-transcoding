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

load(
    "//:repositories.bzl",
    "absl_repositories",
    "googleapis_repositories",
    "googlebenchmark_repositories",
    "googletest_repositories",
    "io_bazel_rules_docker",
    "nlohmannjson_repositories",
    "protobuf_repositories",
)

# See
# https://github.com/bazelbuild/rules_fuzzing/blob/master/README.md#configuring-the-workspace.
# The fuzzing rules must be first because if they are not, bazel will
# pull in incompatible versions of absl and rules_python.
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "rules_fuzzing",
    sha256 = "d9002dd3cd6437017f08593124fdd1b13b3473c7b929ceb0e60d317cb9346118",
    strip_prefix = "rules_fuzzing-0.3.2",
    urls = ["https://github.com/bazelbuild/rules_fuzzing/archive/v0.3.2.zip"],
)

http_archive(
    name = "com_google_absl",
    sha256 = "ea1d31db00eb37e607bfda17ffac09064670ddf05da067944c4766f517876390",
    strip_prefix = "abseil-cpp-c2435f8342c2d0ed8101cb43adfd605fdc52dca2",
    urls = ["https://github.com/abseil/abseil-cpp/archive/c2435f8342c2d0ed8101cb43adfd605fdc52dca2.zip"],
)

load("@rules_fuzzing//fuzzing:repositories.bzl", "rules_fuzzing_dependencies")

rules_fuzzing_dependencies()

load("@rules_fuzzing//fuzzing:init.bzl", "rules_fuzzing_init")

rules_fuzzing_init()

protobuf_repositories()

load("@com_google_protobuf//:protobuf_deps.bzl", "protobuf_deps")

protobuf_deps()

http_archive(
    name = "com_google_protoconverter",
    sha256 = "1bfb2db800c5d339687dfcdb96740d296d8a1bb9ea06ab8f48a81981d3d8bba9",
    strip_prefix = "proto-converter-2c4192cf3bdd2ccdd5a812293df135cdbe0baae5",
    urls = ["https://github.com/grpc-ecosystem/proto-converter/archive/2c4192cf3bdd2ccdd5a812293df135cdbe0baae5.zip"],
)

googletest_repositories()

googleapis_repositories()

googlebenchmark_repositories()

nlohmannjson_repositories()

# Followed https://github.com/bazelbuild/rules_docker#setup and
# https://github.com/bazelbuild/rules_docker#cc_image.
# BEGIN io_bazel_rules_docker
io_bazel_rules_docker()

load(
    "@io_bazel_rules_docker//repositories:repositories.bzl",
    container_repositories = "repositories",
)

container_repositories()

load(
    "@io_bazel_rules_docker//cc:image.bzl",
    _cc_image_repos = "repositories",
)

_cc_image_repos()
# END io_bazel_rules_docker

load("@rules_proto//proto:repositories.bzl", "rules_proto_dependencies", "rules_proto_toolchains")

rules_proto_dependencies()

rules_proto_toolchains()

load("@com_google_googleapis//:repository_rules.bzl", "switched_rules_by_language")

switched_rules_by_language(
    name = "com_google_googleapis_imports",
    cc = True,
)
