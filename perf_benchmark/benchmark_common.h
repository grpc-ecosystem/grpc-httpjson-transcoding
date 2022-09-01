//
// Created by hongrux on 8/30/22.
//

#ifndef PERF_BENCHMARK_BENCHMARK_COMMON_H_
#define PERF_BENCHMARK_BENCHMARK_COMMON_H_

#include <string>

#include "google/api/service.pb.h"
#include "grpc_transcoding/transcoder_input_stream.h"
#include "absl/strings/string_view.h"
#include "benchmark/benchmark.h"

namespace google {
namespace grpc {
namespace transcoding {

namespace perf_benchmark {

// An implementation of ZeroCopyInputStream for benchmarking.
// This stream stores the entire input message and constant return the same
// pointer to the message. This is useful during benchmark since the same input
// message will be read multiple times without introducing a large overhead.
//
// When streaming == true, the same json message will be streamed by stream_size
// times. Each call to Next() will yield a single message from the streaming,
// which means Next() will return true exactly `stream_size` times.
// stream_size won't be used if streaming == false.
class BenchmarkZeroCopyInputStream : public TranscoderInputStream {
 public:
  explicit BenchmarkZeroCopyInputStream(std::string msg, bool streaming,
                                        int stream_size);
  ~BenchmarkZeroCopyInputStream() override = default;

  int64_t BytesAvailable() const override;
  bool Finished() const override;

  bool Next(const void** data, int* size) override;
  void BackUp(int count) override {}; // Not Implemented
  bool Skip(int count) override { return false; }; // Not Implemented
  int64_t ByteCount() const override { return 0; }; // Not Implemented

  // Reset the input stream back to the original start state.
  // This should be called after one iteration of benchmark.
  void Reset();

 private:
  bool finished_;
  const std::string msg_;

  const bool streaming_;
  // only used when streaming_ == true
  const int stream_size_;
  int msg_sent;
  std::string header_;
  std::string body_;
  std::string tail_;

};

// Load service from a proto text file. Returns true if loading succeeds;
// otherwise returns false.
bool LoadService(const std::string& config_pb_txt_file,
                 const std::string& benchmark_path,
                 ::google::api::Service* service);
bool LoadService(const std::string& config_pb_txt_file,
                 ::google::api::Service* service);

// Return the given percentile of the vector v.
double GetPercentile(const std::vector<double>& v, double perc);

// Return a random alphanumeric string of the given length.
std::string GetRandomString(int64_t length);

} // namespace perf_benchmark

} // namespace transcoding
} // namespace grpc
} // namespace google

// Macros

// Macro for running a benchmark with p25, p75, p90, p99, p999 percentiles.
// Other statistics - mean, median, standard deviation, coefficient of variation
// are automatically captured.
// This will also set iteration to 1 because the aggregate function gets
// statistics from each benchmark repetition instead of iterations.
// Running with 1000 iterations only gives 1 data point. Therefore, it is
// recommended to run 1000 repetition of the benchmark with each repetition only
// has 1 iteration instead.
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
    })                                                                        \
    ->Iterations(1)

#endif //PERF_BENCHMARK_BENCHMARK_COMMON_H_
