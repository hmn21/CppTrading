#include "spscqueue.h"

#include <benchmark/benchmark.h>

#include <iostream>
#include <thread>

static void pinThread(int cpu) {
  if (cpu < 0) {
    return;
  }
  ::cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(cpu, &cpuset);
  if (::pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == -1) {
    std::perror("pthread_setaffinity_rp");
    std::exit(EXIT_FAILURE);
  }
}

constexpr bool is_power_of_2(int v) {
  return v && ((v & (v - 1)) == 0);
}

constexpr auto cpu1 = 1;
constexpr auto cpu2 = 2;

template<template<typename> class SPSCQueueT>
void BM_SPSC(benchmark::State &state) {
  using spsc_type = SPSCQueueT<std::int_fast64_t>;
  using value_type = typename spsc_type::value_type;

  constexpr auto spsc_size = 4096;
  static_assert(is_power_of_2(spsc_size), "not power of 2");
  spsc_type spsc(spsc_size);

  auto t = std::jthread([&] {
    pinThread(cpu1);
    for (auto i = value_type{};; ++i) {
      value_type val;

      while (not spsc.pop(val)) { ;
      }
      benchmark::DoNotOptimize(val);

      if (val == -1) {
        break;
      }

      if (val != i) {
        throw std::runtime_error("invalid value");
      }
    }
  });

  auto value = value_type{};
  pinThread(cpu2);
  for (auto _ : state) {

    while (auto again = not spsc.push(value)) {
      benchmark::DoNotOptimize(again);
    }

    ++value;

    while (auto again = not spsc.empty()) {
      benchmark::DoNotOptimize(again);
    }
  }
  state.counters["ops/sec"] = benchmark::Counter(double(value), benchmark::Counter::kIsRate);
  state.PauseTiming();
  spsc.push(-1);
}

BENCHMARK_TEMPLATE(BM_SPSC, SPSCQueue);

BENCHMARK_MAIN();