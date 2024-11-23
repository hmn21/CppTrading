#pragma once

#include <atomic>
#include <emmintrin.h>

class SpinLock {
 public:

  void lock() noexcept {
    for (;;) {
      if (!flag_.test_and_set(std::memory_order_acquire)) {
        return;
      }
      while (flag_.test(std::memory_order_relaxed)) {
        _mm_pause();
      }
    }
  }

  bool try_lock() noexcept {
    return !flag_.test(std::memory_order_relaxed) && !flag_.test_and_set(std::memory_order_acquire);
  }

  void unlock() noexcept {
    flag_.clear(std::memory_order_release);
  }

 private:
  std::atomic_flag flag_{};
};