// Copyright 2016 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
////////////////////////////////////////////////////////////////////////////////
//
#include "perf_benchmark/utils.h"
#include <fstream>
#include <limits>
#include <sstream>
#include "absl/random/random.h"
#include "absl/status/statusor.h"
#include "absl/strings/escaping.h"
#include "google/protobuf/text_format.h"
#include "nlohmann/json.hpp"

namespace google {
namespace grpc {
namespace transcoding {

namespace perf_benchmark {

namespace pb = ::google::protobuf;

absl::StatusOr<std::string> LoadFile(absl::string_view file_name) {
  std::ifstream ifs(file_name.data(), std::ifstream::in);
  if (!ifs) {
    return absl::InvalidArgumentError(
        absl::StrCat("Could not open ", file_name));
  }
  std::ostringstream ss;
  ss << ifs.rdbuf();
  return ss.str();
}

absl::Status LoadService(absl::string_view config_pb_txt_file,
                         ::google::api::Service* service) {
  static const char kBenchmarkData[] = "perf_benchmark/";
  return LoadService(config_pb_txt_file, kBenchmarkData, service);
}

absl::Status LoadService(absl::string_view config_pb_txt_file,
                         absl::string_view benchmark_path,
                         ::google::api::Service* service) {
  auto config = LoadFile(absl::StrCat(benchmark_path, config_pb_txt_file));
  if (!config.ok()) {
    return config.status();
  }

  if (!pb::TextFormat::ParseFromString(*config, service)) {
    return absl::InvalidArgumentError(absl::StrCat(
        "Could not parse service config from ", config_pb_txt_file));
  } else {
    return absl::OkStatus();
  }
}
double GetPercentile(const std::vector<double>& v, double perc) {
  if (perc < 0) {
    perc = 0;
  }
  if (perc > 100) {
    perc = 100;
  }
  // Making a copy since std::nth_element mutates the vector
  auto copy = std::vector<double>(v);
  size_t rough_position = copy.size() * perc / 100;
  std::nth_element(copy.begin(), copy.begin() + rough_position, copy.end());
  return copy[rough_position];
}

std::string GetRandomBytesString(uint64_t length, bool base64) {
  static absl::BitGen bitgen;
  std::string ret;
  ret.reserve(length);

  for (int i = 0; i < length; ++i) {
    // Randomly generate ASCII character.
    ret += char(absl::Uniform(bitgen, 0u, 128u));
  }
  return base64 ? absl::Base64Escape(ret) : ret;
}

std::string GetRandomAlphanumericString(uint64_t length) {
  static absl::BitGen bitgen;
  static const char charset[] =
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz";
  std::string ret;
  ret.reserve(length);
  for (int i = 0; i < length; ++i) {
    // sizeof(charset) - 1 to exclude trailing NULL char
    ret += charset[absl::Uniform(bitgen, 0u, sizeof(charset) - 1)];
  }
  return ret;
}

std::string GetRandomInt32ArrayString(uint64_t length) {
  static absl::BitGen bitgen;
  std::ostringstream os;
  os << '[';
  for (int i = 0; i < length; ++i) {
    os << int32_t(absl::Uniform(bitgen, std::numeric_limits<int32_t>::min(),
                                std::numeric_limits<int32_t>::max()));
    if (i != length - 1) {
      os << ',';
    }
  }
  os << ']';
  return os.str();
}

std::string GetRepeatedValueArrayString(absl::string_view val,
                                        uint64_t length) {
  std::ostringstream os;
  os << '[';
  for (int i = 0; i < length; ++i) {
    os << '"' << val << '"';
    if (i != length - 1) {
      os << ',';
    }
  }
  os << ']';
  return os.str();
}

nlohmann::json GetNestedJson(uint64_t layers,
                             absl::string_view nested_field_name,
                             nlohmann::json inner) {
  if (layers == 0) {
    return inner;
  }
  nlohmann::json outer;
  outer[std::string(nested_field_name)] =
      GetNestedJson(layers - 1, nested_field_name, inner);
  return outer;
}

std::string GetNestedJsonString(uint64_t layers,
                                absl::string_view nested_field_name,
                                absl::string_view inner_key,
                                absl::string_view inner_val) {
  nlohmann::json inner;
  inner[inner_key.data()] = inner_val;
  return to_string(GetNestedJson(layers, nested_field_name, inner));
}

std::string GetStreamedJson(absl::string_view json_msg, uint64_t stream_size) {
  std::stringstream ss("");
  ss << '[';
  for (uint64_t i = 0; i < stream_size; ++i) {
    ss << json_msg;
    if (i != stream_size - 1) {
      ss << ",";
    }
  }
  ss << ']';
  return ss.str();
}

// Copied from "test/test_common.h".
std::string SizeToDelimiter(unsigned size) {
  unsigned char delimiter[5];
  // Byte 0 is the compression bit - set to 0 (no compression)
  delimiter[0] = 0;
  // Bytes 1-4 are big-endian 32-bit message size
  delimiter[4] = 0xFF & size;
  size >>= 8;
  delimiter[3] = 0xFF & size;
  size >>= 8;
  delimiter[2] = 0xFF & size;
  size >>= 8;
  delimiter[1] = 0xFF & size;

  return std::string(reinterpret_cast<const char*>(delimiter),
                     sizeof(delimiter));
}
}  // namespace perf_benchmark

}  // namespace transcoding
}  // namespace grpc
}  // namespace google
