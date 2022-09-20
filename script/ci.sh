#!/bin/bash
#
# Copyright 2021 Google LLC. All Rights Reserved.
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

bazel build //...
bazel test //... --test_output=errors

# Push benchmark binary image to cloudesf-testing GCR.
gcloud config set core/project cloudesf-testing
bazel run //perf_benchmark:benchmark_main_image_push --define=PUSH_REGISTRY=gcr.io --define=PUSH_PROJECT=cloudesf-testing --define=PUSH_TAG=github-latest
