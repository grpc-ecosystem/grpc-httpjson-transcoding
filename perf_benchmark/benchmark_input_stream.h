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
#ifndef PERF_BENCHMARK_BENCHMARK_INPUT_STREAM_H_
#define PERF_BENCHMARK_BENCHMARK_INPUT_STREAM_H_

#include "grpc_transcoding/transcoder_input_stream.h"
#include "absl/strings/string_view.h"

namespace google {
namespace grpc {
namespace transcoding {

namespace perf_benchmark {
// An implementation of ZeroCopyInputStream for benchmarking.
// Subclasses of this should store the entire input message and return pointer
// to the stored message for each round of Next(). This is useful during
// benchmark since the same input message will be read multiple times without
// introducing a large runtime overhead.
//
// After each benchmark iteration, Reset() needs to be called.
class BenchmarkZeroCopyInputStream : public TranscoderInputStream {
 public:
  // Pre-Conditions:
  // - chunk_per_msg <= msg.size()
  //
  // Note: chunk_per_msg could be off by a few chunks due to int rounding.
  explicit BenchmarkZeroCopyInputStream(std::string msg,
                                        uint64_t chunk_per_msg);
  ~BenchmarkZeroCopyInputStream() override = default;

  int64_t BytesAvailable() const override = 0;
  bool Finished() const override { return finished_; };

  bool Next(const void** data, int* size) override = 0;
  void BackUp(int count) override {}; // Not Implemented
  bool Skip(int count) override { return false; }; // Not Implemented
  int64_t ByteCount() const override { return 0; }; // Not Implemented

  // Reset the input stream back to the original start state.
  // This should be called after one iteration of benchmark.
  virtual void Reset() = 0;

  // Return the total number of bytes of the entire JSON message.
  virtual uint64_t TotalBytes() const = 0;

 protected:
  bool finished_;
  const std::string msg_;
  const uint64_t chunk_size_;
};

// BenchmarkZeroCopyInputStream implementation for non-streaming benchmarks.
class UnaryBenchmarkZeroCopyInputStream : public BenchmarkZeroCopyInputStream {
 public:
  // Pre-Conditions:
  // - chunk_per_msg <= msg.size()
  //
  // Note: chunk_per_msg could be off by a few chunks due to int rounding.
  explicit UnaryBenchmarkZeroCopyInputStream(std::string msg,
                                             uint64_t chunk_per_msg);
  ~UnaryBenchmarkZeroCopyInputStream() override = default;

  int64_t BytesAvailable() const override;
  bool Next(const void** data, int* size) override;
  void Reset() override;
  uint64_t TotalBytes() const override;

 private:
  uint64_t pos_;
};

// BenchmarkZeroCopyInputStream for streaming message benchmarks.
// The same json message will be streamed by stream_size times. Each call to
// Next() will yield a single message from the streaming, which means Next()
// will return true exactly `stream_size` times.
class StreamingBenchmarkZeroCopyInputStream
    : public BenchmarkZeroCopyInputStream {
 public:
  // Pre-Conditions:
  // - chunk_per_msg <= msg.size()
  //
  // Note: chunk_per_msg could be off by a few chunks due to int rounding.
  explicit StreamingBenchmarkZeroCopyInputStream(std::string msg,
                                                 uint64_t chunk_per_msg,
                                                 uint64_t stream_size);
  ~StreamingBenchmarkZeroCopyInputStream() override = default;

  int64_t BytesAvailable() const override;
  bool Next(const void** data, int* size) override;
  void Reset() override;
  uint64_t TotalBytes() const override;

 private:
  uint64_t pos_;
  const uint64_t stream_size_;
  int msg_sent;
  std::string header_;
  std::string body_;
  std::string tail_;
};
} // namespace perf_benchmark

} // namespace transcoding
} // namespace grpc
} // namespace google

#endif //PERF_BENCHMARK_BENCHMARK_INPUT_STREAM_H_
