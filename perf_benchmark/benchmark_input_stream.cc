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

#include "perf_benchmark/benchmark_input_stream.h"

namespace google {
namespace grpc {
namespace transcoding {

namespace perf_benchmark {

BenchmarkZeroCopyInputStream::BenchmarkZeroCopyInputStream(std::string msg,
                                                           uint64_t chunk_per_msg)
    : finished_(false),
      msg_(std::move(msg)),
      chunk_size_(msg_.size() / chunk_per_msg) {}

UnaryBenchmarkZeroCopyInputStream::UnaryBenchmarkZeroCopyInputStream(std::string msg,
                                                                     uint64_t chunk_per_msg)
    : BenchmarkZeroCopyInputStream(std::move(msg), chunk_per_msg), pos_(0) {}

int64_t UnaryBenchmarkZeroCopyInputStream::BytesAvailable() const {
  if (finished_) {
    return 0;
  }
  // check if we are at the last chunk
  if (pos_ + chunk_size_ >= msg_.size()) {
    return msg_.size() - pos_;
  }
  return chunk_size_;
}

bool UnaryBenchmarkZeroCopyInputStream::Next(const void** data, int* size) {
  if (finished_) {
    *size = 0;
    return false;
  }
  *data = msg_.data() + pos_;
  if (pos_ + chunk_size_ >= msg_.size()) { // last message
    *size = msg_.size() - pos_;
    pos_ = 0; // reset pos after sending the last message
    finished_ = true;
  } else {
    *size = chunk_size_;
    pos_ += chunk_size_;
  }
  return true;
}

void UnaryBenchmarkZeroCopyInputStream::Reset() {
  finished_ = false;
}

uint64_t UnaryBenchmarkZeroCopyInputStream::TotalBytes() const {
  return msg_.size();
}

StreamingBenchmarkZeroCopyInputStream::StreamingBenchmarkZeroCopyInputStream(std::string msg,
                                                                             uint64_t chunk_per_msg,
                                                                             uint64_t stream_size)
    : BenchmarkZeroCopyInputStream(std::move(msg), chunk_per_msg),
      pos_(0),
      stream_size_(stream_size),
      msg_sent(0) {
  if (stream_size_ == 1) {
    // Edge case, we only need to set header_
    header_ = "[" + msg_ + "]";
  } else {
    header_ = "[" + msg_ + ", ";
    body_ = msg_ + ", ";
    tail_ = msg_ + "]";
  }
}
int64_t StreamingBenchmarkZeroCopyInputStream::BytesAvailable() const {
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
bool StreamingBenchmarkZeroCopyInputStream::Next(const void** data, int* size) {
  std::string* msg_ptr;
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
  return true;
}
void StreamingBenchmarkZeroCopyInputStream::Reset() {
  finished_ = false;
  msg_sent = 0;
}
uint64_t StreamingBenchmarkZeroCopyInputStream::TotalBytes() const {
  if (stream_size_ == 0) {
    return 0;
  }
  if (stream_size_ == 1) {
    return header_.size();
  }
  return header_.size() + tail_.size() + body_.size() * (stream_size_ - 2);
}

} // namespace perf_benchmark

} // namespace transcoding
} // namespace grpc
} // namespace google