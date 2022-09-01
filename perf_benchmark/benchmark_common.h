//
// Created by hongrux on 8/30/22.
//

#ifndef PERF_BENCHMARK_BENCHMARK_COMMON_H_
#define PERF_BENCHMARK_BENCHMARK_COMMON_H_

#include <string>

#include "google/api/service.pb.h"
#include "grpc_transcoding/transcoder_input_stream.h"
#include "absl/strings/string_view.h"

namespace google {
namespace grpc {
namespace transcoding {

namespace perf_benchmark {

// An implementation of ZeroCopyInputStream for benchmarking.
// This stream stores the entire input message and constant return the same
// pointer to the message. This is  useful during benchmark since the same input
// message will be read multiple times without introducing a large overhead.
class BenchmarkZeroCopyInputStream : public TranscoderInputStream {
 public:
  explicit BenchmarkZeroCopyInputStream(absl::string_view msg)
      : finished_(false), msg_(msg) {};
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
  std::string msg_;
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

}

}
}
}

#endif //PERF_BENCHMARK_BENCHMARK_COMMON_H_
