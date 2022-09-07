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
#include <fstream>
#include <sstream>
#include <utility>
#include <limits>
#include "perf_benchmark/benchmark_common.h"
#include "google/protobuf/text_format.h"
#include "absl/strings/escaping.h"
#include "absl/random/random.h"
#include "absl/status/statusor.h"
#include "nlohmann/json.hpp"

namespace google {
namespace grpc {
namespace transcoding {

namespace perf_benchmark {

namespace pb = ::google::protobuf;

absl::StatusOr<std::string> LoadFile(absl::string_view file_name) {
  std::ifstream ifs(file_name.data(), std::ifstream::in);
  if (!ifs) {
    return absl::InvalidArgumentError(absl::StrCat("Could not open ",
                                                   file_name));
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
        "Could not parse service config from ",
        config_pb_txt_file));
  } else {
    return absl::OkStatus();
  }
}
double GetPercentile(const std::vector<double>& v, double perc) {
  if (perc < 0) { perc = 0; }
  if (perc > 100) { perc = 100; }
  // Making a copy since std::nth_element mutates the vector
  auto copy = std::vector<double>(v);
  size_t rough_position = copy.size() * perc / 100;
  std::nth_element(copy.begin(), copy.begin() + rough_position, copy.end());
  return copy[rough_position];
}

std::string GetRandomBytesString(int64_t length, bool base64) {
  static absl::BitGen bitgen;
  std::string ret;
  ret.reserve(length);

  for (int i = 0; i < length; ++i) {
    // Randomly generate ASCII character.
    ret += char(absl::Uniform(bitgen, 0u, 128u));
  }
  return base64 ? absl::Base64Escape(ret) : ret;
}

std::string GetRandomAlphanumericString(int64_t length) {
  static absl::BitGen bitgen;
  static const char charset[] =
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz";
  std::string ret;
  ret.reserve(length);
  for (int i = 0; i < length; ++i) {
    ret += charset[absl::Uniform(bitgen, 0u, sizeof(charset))];
  }
  return ret;
}

std::string GetRandomInt32ArrayString(int64_t length) {
  static absl::BitGen bitgen;
  std::ostringstream os;
  os << '[';
  for (int i = 0; i < length; ++i) {
    os << int32_t(absl::Uniform(bitgen,
                                std::numeric_limits<int32_t>::min(),
                                std::numeric_limits<int32_t>::max()));
    if (i != length - 1) {
      os << ',';
    }
  }
  os << ']';
  return os.str();
}

std::string GetRepeatedValueArrayString(std::string val, int64_t length) {
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

nlohmann::json GetNestedJson(int64_t layers,
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

std::string GetNestedJsonString(int64_t layers,
                                absl::string_view nested_field_name,
                                std::string inner_key,
                                std::string inner_val) {
  nlohmann::json inner;
  inner[std::move(inner_key)] = inner_val;
  return to_string(GetNestedJson(layers, nested_field_name, inner));
}

BenchmarkZeroCopyInputStream::BenchmarkZeroCopyInputStream(std::string msg,
                                                           bool streaming,
                                                           int stream_size,
                                                           int chunk_per_msg)
    : finished_(false),
      msg_(std::move(msg)),
      chunk_size_(msg_.size() / chunk_per_msg),
      pos_(0),
      streaming_(streaming),
      stream_size_(stream_size),
      msg_sent(0) {
  if (streaming_) {
    if (stream_size_ == 1) {
      // Edge case, we only need to set header_
      header_ = "[" + msg_ + "]";
    } else {
      header_ = "[" + msg_ + ", ";
      body_ = msg_ + ", ";
      tail_ = msg_ + "]";
    }
  }
}

int64_t BenchmarkZeroCopyInputStream::BytesAvailable() const {
  if (finished_) {
    return 0;
  }
  if (!streaming_) {
    // non-streaming
    // check if we are at the last chunk
    if (pos_ + chunk_size_ >= msg_.size()) {
      return msg_.size() - pos_;
    }
    return chunk_size_;
  } else {
    // streaming
    // no message sent -> next message is header_
    if (msg_sent == 0) {
      // header has 3 chars overhead
      if (pos_ + chunk_size_ + 3 >= header_.size()) {
        return header_.size() - pos_;
      }
      return chunk_size_;
    }
    // last message to be sent -> next message is tail_
    if (msg_sent + 1 == stream_size_) {
      // tail has 1 char overhead
      if (pos_ + chunk_size_ + 1 >= tail_.size()) {
        return tail_.size() - pos_;
      }
      return chunk_size_;
    }
    // otherwise -> next message is body_
    // body has 2 chars overhead
    if (pos_ + chunk_size_ + 2 >= body_.size()) {
      return body_.size() - pos_;
    }
    return chunk_size_;
  }
}

bool BenchmarkZeroCopyInputStream::Finished() const {
  return finished_;
}

bool BenchmarkZeroCopyInputStream::Next(const void** data, int* size) {
  if (finished_) {
    *size = 0;
    return false;
  }
  if (!streaming_) {
    // non-streaming
    *data = msg_.data() + pos_;
    if (pos_ + chunk_size_ >= msg_.size()) { // last message
      *size = msg_.size() - pos_;
      pos_ = 0; // reset pos after sending the last message
      finished_ = true;
    } else {
      *size = chunk_size_;
      pos_ += chunk_size_;
    }
  } else {
    // streaming
    std::string* msg_ptr = nullptr;
    int overhead = 0; // character overhead due to [,] characters.
    if (msg_sent == 0) {
      // no message sent -> next message is header_
      msg_ptr = &header_;
      overhead = 3;
    } else if (msg_sent + 1 == stream_size_) {
      // last message to be sent -> next message is tail_
      msg_ptr = &tail_;
      overhead = 1;
    } else {
      msg_ptr = &body_;
      overhead = 2;
    }

    *data = msg_ptr->data() + pos_;
    if (pos_ + chunk_size_ + overhead >= msg_ptr->size()) { // last message
      *size = msg_ptr->size() - pos_;
      pos_ = 0; // reset pos after sending the last message
      ++msg_sent;
    } else {
      *size = chunk_size_;
      pos_ += chunk_size_;
    }
    if (msg_sent == stream_size_) {
      finished_ = true;
    }
  }
  return true;
}

void BenchmarkZeroCopyInputStream::Reset() {
  finished_ = false;
  msg_sent = 0;
}
int64_t BenchmarkZeroCopyInputStream::TotalBytes() const {
  if (!streaming_) {
    return msg_.size();
  }

  if (stream_size_ == 0) {
    return 0;
  }
  if (stream_size_ == 1) {
    return header_.size();
  }
  return header_.size() + tail_.size()
      + body_.size() * static_cast<size_t>(stream_size_ - 2);
}

} // namespace perf_benchmark

} // namespace transcoding
} // namespace grpc
} // namespace google

