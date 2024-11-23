// From https://github.com/CharlesFrasch/cppcon2023

#pragma once

#include <atomic>
#include <cassert>
#include <cstring>
#include <memory>
#include <new>
#include <type_traits>

/// A trait used to optimize the number of bytes copied. Specialize this
/// on the type used to parameterize the SPSCQueue to implement the
/// optimization. The general template returns `sizeof(T)`.
template<typename T>
struct ValueSizeTraits {
  using value_type = T;
  static std::size_t size(value_type const &value) { return sizeof(value_type); }
};

template<typename T, typename Alloc = std::allocator<T>> requires std::is_trivial_v<T>
class SPSCQueue : private Alloc {
 public:
  using value_type = T;
  using allocator_traits = std::allocator_traits<Alloc>;
  using size_type = typename allocator_traits::size_type;

  explicit SPSCQueue(size_type capacity, Alloc const &alloc = Alloc{})
      : Alloc{alloc}, mask_{capacity - 1}, ring_{allocator_traits::allocate(*this, capacity)} {
    assert((capacity & mask_) == 0);
  }

  ~SPSCQueue() {
    allocator_traits::deallocate(*this, ring_, capacity());
  }

  /// Returns the number of elements in the spsc
  auto size() const noexcept {
    auto pushCursor = pushCursor_.load(std::memory_order_relaxed);
    auto popCursor = popCursor_.load(std::memory_order_relaxed);

    assert(popCursor <= pushCursor);
    return pushCursor - popCursor;
  }

  /// Returns whether the container has no elements
  bool empty() const noexcept { return size() == 0; }

  /// Returns whether the container has capacity_() elements
  bool full() const noexcept { return size() == capacity(); }

  /// Returns the number of elements that can be held in the spsc
  size_type capacity() const noexcept { return mask_ + 1; }

  /// An RAII proxy object returned by push(). Allows the caller to
  /// manipulate value_type's members directly in the spsc's ring. The
  /// actual push happens when the pusher goes out of scope.
  class pusher_t {
   public:
    pusher_t() = default;
    explicit pusher_t(SPSCQueue *spsc, size_type cursor) noexcept: spsc_{spsc}, cursor_{cursor} {}

    pusher_t(pusher_t const &) = delete;
    pusher_t &operator=(pusher_t const &) = delete;

    pusher_t(pusher_t &&other) noexcept
        : spsc_{std::move(other.spsc_)}, cursor_{std::move(other.cursor_)} {
      other.release();
    }
    pusher_t &operator=(pusher_t &&other) noexcept {
      spsc_ = std::move(other.spsc_);
      cursor_ = std::move(other.cursor_);
      other.release();
      return *this;
    }

    ~pusher_t() {
      if (spsc_) {
        spsc_->pushCursor_.store(cursor_ + 1, std::memory_order_release);
      }
    }

    /// If called the actual push operation will not be called when the
    /// pusher_t goes out of scope. Operations on the pusher_t instance
    /// after release has been called are undefined.
    void release() noexcept { spsc_ = {}; }

    /// Return whether or not the pusher_t is active.
    explicit operator bool() const noexcept { return spsc_; }

    /// @name Direct access to the spsc's ring
    ///@{
    value_type *get() noexcept { return spsc_->element(cursor_); }
    const value_type *get() const noexcept { return spsc_->element(cursor_); }

    value_type &operator*() noexcept { return *get(); }
    const value_type &operator*() const noexcept { return *get(); }

    value_type *operator->() noexcept { return get(); }
    const value_type *operator->() const noexcept { return get(); }
    ///@}

    /// Copy-assign a `value_type` to the pusher. Prefer to use this
    /// form rather than assigning directly to a value_type&. It takes
    /// advantage of ValueSizeTraits.
    pusher_t &operator=(const value_type &value) noexcept {
      std::memcpy(get(), std::addressof(value), ValueSizeTraits<value_type>::size(value));
      return *this;
    }

   private:
    SPSCQueue *spsc_{};
    size_type cursor_;
  };
  friend class pusher_t;

  /// Optionally push one object onto a file via a pusher.
  /// @return a pointer to pusher_t.
  pusher_t push() noexcept {
    auto pushCursor = pushCursor_.load(std::memory_order_relaxed);
    if (full(pushCursor, popCursorCached_)) {
      popCursorCached_ = popCursor_.load(std::memory_order_acquire);
      if (full(pushCursor, popCursorCached_)) {
        return pusher_t{};
      }
    }
    return pusher_t(this, pushCursor);
  }

  /// Push one object onto the spsc.
  /// @return `true` if the operation is successful; `false` if spsc is full.
  bool push(T const &value) noexcept {
    if (auto pusher = push(); pusher) {
      pusher = value;
      return true;
    }
    return false;
  }

  /// An RAII proxy object returned by pop(). Allows the caller to
  /// manipulate value_type members directly in the spsc's ring. The
  // /actual pop happens when the popper goes out of scope.
  class popper_t {
   public:
    popper_t() = default;
    explicit popper_t(SPSCQueue *spsc, size_type cursor) noexcept: spsc_{spsc}, cursor_{cursor} {}

    popper_t(popper_t const &) = delete;
    popper_t &operator=(popper_t const &) = delete;

    popper_t(popper_t &&other) noexcept
        : spsc_{std::move(other.spsc_)}, cursor_{std::move(other.cursor_)} {
      other.release();
    }
    popper_t &operator=(popper_t &&other) noexcept {
      spsc_ = std::move(other.spsc_);
      cursor_ = std::move(other.cursor_);
      other.release();
      return *this;
    }

    ~popper_t() {
      if (spsc_) {
        spsc_->popCursor_.store(cursor_ + 1, std::memory_order_release);
      }
    }

    /// If called the actual pop operation will not be called when the
    /// popper_t goes out of scope. Operations on the popper_t instance
    /// after release has been called are undefined.
    void release() noexcept { spsc_ = {}; }

    /// Return whether or not the popper_t is active.
    explicit operator bool() const noexcept { return spsc_; }

    /// @name Direct access to the spsc's ring
    ///@{
    value_type *get() noexcept { return spsc_->element(cursor_); }
    const value_type *get() const noexcept { return spsc_->element(cursor_); }

    value_type &operator*() noexcept { return *get(); }
    const value_type &operator*() const noexcept { return *get(); }

    value_type *operator->() noexcept { return get(); }
    const value_type *operator->() const noexcept { return get(); }
    ///@}

   private:
    SPSCQueue *spsc_{};
    size_type cursor_;
  };
  friend popper_t;

  auto pop() noexcept {
    auto popCursor = popCursor_.load(std::memory_order_relaxed);
    if (empty(pushCursorCached_, popCursor)) {
      pushCursorCached_ = pushCursor_.load(std::memory_order_acquire);
      if (empty(pushCursorCached_, popCursor)) {
        return popper_t{};
      }
    }
    return popper_t(this, popCursor);
  };

  /// Pop one object from the spsc.
  /// @return `true` if the pop operation is successful; `false` if spsc is empty.
  auto pop(T &value) noexcept {
    if (auto popper = pop(); popper) {
      value = *popper;
      return true;
    }
    return false;
  }

 private:
  bool full(size_type pushCursor, size_type popCursor) const noexcept {
    assert(popCursor <= pushCursor);
    return (pushCursor - popCursor) == capacity();
  }
  static bool empty(size_type pushCursor, size_type popCursor) noexcept {
    return pushCursor == popCursor;
  }

  T *element(size_type cursor) noexcept { return &ring_[cursor & mask_]; }
  const T *element(size_type cursor) const noexcept { return &ring_[cursor % mask_]; }

 private:
  size_type mask_;
  T *ring_;

  using cursor_type = std::atomic<size_type>;
  static_assert(cursor_type::is_always_lock_free);

  // https://stackoverflow.com/questions/39680206/understanding-stdhardware-destructive-interference-size-and-stdhardware-cons
  // suppress warning: use of ‘std::hardware_destructive_interference_size’ [-Winterference-size]
  static constexpr auto hardware_destructive_interference_size = size_type{64};

  /// Loaded and stored by the push thread; loaded by the pop thread
  alignas(hardware_destructive_interference_size) cursor_type pushCursor_;

  /// Exclusive to the push thread
  alignas(hardware_destructive_interference_size) size_type popCursorCached_{};

  /// Loaded and stored by the pop thread; loaded by the push thread
  alignas(hardware_destructive_interference_size) cursor_type popCursor_;

  /// Exclusive to the pop thread
  alignas(hardware_destructive_interference_size) size_type pushCursorCached_{};

  // Padding to avoid false sharing with adjacent objects
  char padding_[hardware_destructive_interference_size - sizeof(size_type)];
};