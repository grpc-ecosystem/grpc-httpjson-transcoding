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
#ifndef PERF_BENCHMARK_BENCHMARK_COMMON_H_
#define PERF_BENCHMARK_BENCHMARK_COMMON_H_

#include <string>

#include "google/api/service.pb.h"
#include "grpc_transcoding/transcoder_input_stream.h"
#include "absl/strings/string_view.h"
#include "absl/status/status.h"
#include "benchmark/benchmark.h"

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

// Load service from a proto text file. Returns true if loading succeeds;
// otherwise returns false.
absl::Status LoadService(absl::string_view config_pb_txt_file,
                         absl::string_view benchmark_path,
                         ::google::api::Service* service);
absl::Status LoadService(absl::string_view config_pb_txt_file,
                         ::google::api::Service* service);

// Return the given percentile of the vector v.
double GetPercentile(const std::vector<double>& v, double perc);

// Return a random string of the given length.
// length - Length of the returned string. If base64 == true, the actual
//          returned string length is 33–37% larger due to the encoding.
// base64 - True if the returned string should be base64 encoded. This is
//          required for bytes proto message.
std::string GetRandomBytesString(uint64_t length, bool base64);

// Return a random alphanumeric string of the given length.
// length - Length of the returned string.
std::string GetRandomAlphanumericString(uint64_t length);

// Return a random string representing an array of int32, e.g. "[1,2,3]"
// length - Length of the integer array.
std::string GetRandomInt32ArrayString(uint64_t length);

// Return an array string of the given length with repeated values,
// e.g. "[0, 0, 0]" for GetRepeatedValueArrayString("0", 3).
// val - Unescaped string value to be put in the array.
// length - Length of the integer array.
std::string GetRepeatedValueArrayString(absl::string_view val, uint64_t length);

// Return a nested JSON string with the innermost value being a payload string,
// e.g. "{"nested": {"nested": {"inner_key": "inner_val"}}}"
// layers - Number of nested layer. The value needs >= 0. 0 is a flat JSON.
// nested_field_name - JSON key name for the nested field.
// inner_key - Field name for the innermost json field.
// payload_msg - String value for the innermost json field.
std::string GetNestedJsonString(uint64_t layers,
                                absl::string_view nested_field_name,
                                std::string inner_key,
                                std::string inner_val);

} // namespace perf_benchmark

} // namespace transcoding
} // namespace grpc
} // namespace google

// Macros

// Macro for running a benchmark with p25, p75, p90, p99, p999 percentiles.
// Other statistics - mean, median, standard deviation, coefficient of variation
// are automatically captured.
// Note that running with 1000 iterations only gives 1 data point. Therefore,
// it is recommended to run with --benchmark_repetitions=1000 CLI argument to
// get comparable results.
// Use this marco the same way as BENCHMARK macro.
#define BENCHMARK_WITH_PERCENTILE(func)                                       \
    BENCHMARK(func)                                                           \
    ->ComputeStatistics("p25", [](const std::vector<double>& v) -> double {   \
      return GetPercentile(v, 25);                                            \
    })                                                                        \
    ->ComputeStatistics("p75", [](const std::vector<double>& v) -> double {   \
      return GetPercentile(v, 75);                                            \
    })                                                                        \
    ->ComputeStatistics("p90", [](const std::vector<double>& v) -> double {   \
      return GetPercentile(v, 90);                                            \
    })                                                                        \
    ->ComputeStatistics("p99", [](const std::vector<double>& v) -> double {   \
      return GetPercentile(v, 99);                                            \
    })                                                                        \
    ->ComputeStatistics("p999", [](const std::vector<double>& v) -> double {  \
      return GetPercentile(v, 99.9);                                          \
    })

#define BENCHMARK_STREAMING_WITH_PERCENTILE(func)                             \
    BENCHMARK_WITH_PERCENTILE(func)                                           \
    ->Arg(1)                                                                  \
    ->Arg(1 << 2)                                                             \
    ->Arg(1 << 4)                                                             \
    ->Arg(1 << 6)

#endif //PERF_BENCHMARK_BENCHMARK_COMMON_H_
