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
#include "perf_benchmark/benchmark_common.h"
#include "google/protobuf/text_format.h"
#include "absl/strings/escaping.h"
#include "nlohmann/json.hpp"

namespace google {
namespace grpc {
namespace transcoding {

namespace perf_benchmark {

namespace pb = ::google::protobuf;

std::string LoadFile(const std::string& file_name) {
  std::ifstream ifs(file_name);
  if (!ifs) {
    std::cerr << "Could not open " << file_name.c_str() << std::endl;
    return {};
  }
  std::ostringstream ss;
  ss << ifs.rdbuf();
  return ss.str();
}

bool LoadService(const std::string& config_pb_txt_file,
                 ::google::api::Service* service) {
  static const char kBenchmarkData[] = "perf_benchmark/";
  return LoadService(config_pb_txt_file, kBenchmarkData, service);
}

bool LoadService(const std::string& config_pb_txt_file,
                 const std::string& benchmark_path,
                 ::google::api::Service* service) {
  auto config = LoadFile(benchmark_path + config_pb_txt_file);
  if (config.empty()) {
    return false;
  }

  if (!pb::TextFormat::ParseFromString(config, service)) {
    std::cerr << "Could not parse service config from "
              << config_pb_txt_file.c_str() << std::endl;
    return false;
  } else {
    return true;
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

std::string GetRandomString(int64_t length, bool base64) {
  static const char charset[] =
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
       21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38,
       39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56,
       57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74,
       75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92,
       93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108,
       109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122,
       123, 124, 125, 126, 127};
  std::string ret;
  ret.reserve(length);

  for (int i = 0; i < length; ++i) {
    ret += charset[rand() % (sizeof(charset) - 1)];
  }
  return base64 ? absl::Base64Escape(ret) : ret;
}

std::string GetRandomInt32ArrayString(int64_t length) {
  std::ostringstream os;
  os << '[';
  for (int i = 0; i < length; ++i) {
    os << int32_t(rand());
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
    os << val;
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
                                std::string payload_msg) {
  return to_string(GetNestedJson(layers, nested_field_name, payload_msg));
}

BenchmarkZeroCopyInputStream::BenchmarkZeroCopyInputStream(std::string msg,
                                                           bool streaming,
                                                           int stream_size)
    : finished_(false),
      msg_(std::move(msg)),
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
    return msg_.size();
  } else {
    // streaming
    if (msg_sent == 0) {
      // no message sent -> next message is header_
      return header_.size();
    }
    if (msg_sent + 1 == stream_size_) {
      // last message to be sent -> next message is tail_
      return tail_.size();
    }
    return body_.size();
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
    *data = msg_.data();
    *size = msg_.size();
    finished_ = true;
  } else {
    // streaming
    if (msg_sent == 0) {
      // no message sent -> next message is header_
      *data = header_.data();
      *size = header_.size();
    } else if (msg_sent + 1 == stream_size_) {
      // last message to be sent -> next message is tail_
      *data = tail_.data();
      *size = tail_.size();
    } else {
      *data = body_.data();
      *size = body_.size();
    }
    ++msg_sent;
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

