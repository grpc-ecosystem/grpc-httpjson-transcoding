# Performance Benchmark

We use [Google Benchmark](https://github.com/google/benchmark) library to build
our performance benchmark. Variable being tested

- Number of nested JSON layer
- JSON array length
- JSON value data type
- JSON body length
- Message chunk size

## How to run

```bash
bazelisk run //perf_benchmark:benchmark_main -- \
  --benchmark_min_warmup_time=3 \
  --benchmark_repetitions=1000 \
  --benchmark_format=console \
  --benchmark_counters_tabular=true \
  --benchmark_filter=BM_*
```

Options meaning:

- `benchmark_min_warmup_time=<int>`: the amount of time for which the warmup should be run 
- `benchmark_repetitions=<int>`: the test will automatically run several
  iterations, but only one data point is captured per run. Setting repetition to
  1000 gives us 1000 data points, which would make percentiles and standard
  deviation more meaningful.
- `benchmark_format=<console|json|csv>`: where to output benchmark results.
- `benchmark_counters_tabular=<true|false>`: it's useful when outputting to console.
- `benchmark_filter=<regex>`: it can be used to only run the benchmarks that match
  the specified <regex>.

## Captured data

- Elapsed time and CPU time
- Byte latency and throughput
- Message latency and throughput
- Request latency and throughput

We also capture p25, p50, p75, p90, p99, and p999 for each test,
but `--benchmark_repetitions=1000` is recommended for the results to be
meaningful.