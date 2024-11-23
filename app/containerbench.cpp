#include "benchmark/benchmark.h"
#include "absl/log/absl_check.h"
#include "absl/random/random.h"
#include "absl/container/node_hash_map.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/btree_map.h"

#include <list>
#include <map>

template<typename T>
void BM_LookupSpeed(benchmark::State& state) {
  using ValueType = T::value_type;

  const int size = std::max<int>(1, state.range(0) / sizeof(ValueType));

  // Choose size random integers, then populate our container with it.
  absl::BitGen rng;
  std::vector<std::remove_cv_t<typename ValueType::first_type>> v;
  v.reserve(size);
  for (int i = 0; i < size; ++i) {
    v.push_back(absl::Uniform(rng, std::numeric_limits<typename ValueType::first_type>::min(),
                              std::numeric_limits<typename ValueType::first_type>::max()));
  }

  T container;
  for (auto u : v) {
    if constexpr (std::is_same<T, std::list<ValueType>>::value) {
      container.emplace_back(u, u);
    } else {
      container.emplace(u, u);
    }
  }

  const auto begin = container.begin();
  const auto end = container.end();

  // Use a small state PRNG for selecting indices to avoid confounding access
  // counts that might occur with larger state RNGs (like Mersenne Twister).
  std::minstd_rand lcg;
  uint32_t last = 0;
  uint64_t sum = 0;
  for (auto _ : state) {
    const size_t index = (lcg() + last) % size;
    const auto value = v[index];

    typename T::const_iterator it;
    if constexpr (std::is_same<T, std::list<ValueType>>::value) {
      it = std::find(begin, end, std::make_pair(value, value));
    } else {
      it = container.find(value);
    }
    ABSL_DCHECK(it != end);
    auto found = it->second;
    sum += found;

    // Introduce a deliberate dependency from this lookup on the key for the
    // next value.
    last = found;
  }

  ABSL_CHECK_NE(sum, 0);
}

// std::list is O(N), so reduce the data set size 100x for reasonable running
// time.  The reported numbers are per operation, so they are still comparable.
BENCHMARK_TEMPLATE(BM_LookupSpeed, std::list<std::pair<uint32_t, uint32_t>>)
->Range(1, benchmark::CPUInfo::Get().caches.back().size / 100);
BENCHMARK_TEMPLATE(BM_LookupSpeed, std::map<uint32_t, uint32_t>)
->Range(1, benchmark::CPUInfo::Get().caches.back().size);
BENCHMARK_TEMPLATE(BM_LookupSpeed, absl::btree_map<uint32_t, uint32_t>)
->Range(1, benchmark::CPUInfo::Get().caches.back().size);
BENCHMARK_TEMPLATE(BM_LookupSpeed, absl::node_hash_map<uint32_t, uint32_t>)
->Range(1, benchmark::CPUInfo::Get().caches.back().size);
BENCHMARK_TEMPLATE(BM_LookupSpeed, absl::flat_hash_map<uint32_t, uint32_t>)
->Range(1, benchmark::CPUInfo::Get().caches.back().size);
BENCHMARK_MAIN();