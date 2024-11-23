// From the talk CppCon 2017: Chandler Carruth “Going Nowhere Faster”
// https://www.youtube.com/watch?v=2EWejmkKlxs

#include <random>
#include <limits>
#include "benchmark/benchmark.h"

static void cacheBench(benchmark::State& s) {
  int bytes = 1 << s.range(0);
  int count = (bytes / sizeof(int)) / 2;
  std::vector<int> v;
  std::random_device rd;  // a seed source for the random number engine
  std::mt19937 gen(rd()); // mersenne_twister_engine seeded with rd()
  std::uniform_int_distribution<> distrib(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
  for (int i = 0; i < count; ++i) {
    v.push_back(distrib(gen));
  }
  std::uniform_int_distribution<> distrib2(0, count - 1);
  std::vector<int> indices;
  for (int i = 0; i < count; ++i) {
    indices.push_back(distrib2(gen));
  }
  while (s.KeepRunning()) {
    long sum = 0;
    for (int i: indices) {
      sum += v[i];
    }
    benchmark::DoNotOptimize(sum);
  }
  s.SetBytesProcessed(long(s.iterations()) * long(bytes));
  s.SetLabel(std::to_string(bytes / 1024) + "kb");
}

BENCHMARK(cacheBench)->DenseRange(13, 26)->ReportAggregatesOnly(true);
BENCHMARK_MAIN();