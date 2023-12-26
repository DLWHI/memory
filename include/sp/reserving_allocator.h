#ifndef SP_MEMORY_LIMIT_ALLOCATOR_H_
#define SP_MEMORY_LIMIT_ALLOCATOR_H_
#include <cstdint>  // int64_t
#include <memory>
#include <type_traits>
#include <stdexcept>

namespace sp {
// No general requirements on type T
template <typename T>
class reseving_allocator {
 public:
  using value_type = T;
  using size_type = int64_t;
  using difference_type = int64_t;
  using propagate_on_container_copy_assignment = std::false_type;
  using propagate_on_container_move_assignment = std::false_type;
  using propagate_on_container_swap = std::true_type;

  constexpr explicit reseving_allocator()
      : trace_(new size_type[3]{0, 0, 0}), pool_(nullptr) {}

  constexpr explicit reseving_allocator(size_type count, size_type limit = 0)
      : trace_(new size_type[2]{limit, count, 0}) {
    if (limit < 0) {
      throw std::invalid_argument("pool_allocator: negative pool size");
    } else if (count < 0) {
      throw std::invalid_argument("pool_allocator: negative object count");
    }
    pool_ = new pool_node{static_cast<T*>(operator new(sizeof(T)), nullptr)};
    pool_node* pl = pool_;
    for (--count; count; --count) {
      pl->next = new pool_node{static_cast<T*>(operator new(sizeof(T)), nullptr)};
      pl = pl->next;
    }
  }

  constexpr reseving_allocator(const reseving_allocator& other) noexcept
      : trace_(other.trace_), pool_(other.pool_) {
    ++trace_[kRefInd];
  }

  constexpr reseving_allocator(reseving_allocator&& other) noexcept
      : trace_(other.trace_), pool_(other.pool_) {
    other.trace_ = nullptr;
    other.pool_ = nullptr;
  }

  constexpr reseving_allocator& operator=(
      const reseving_allocator& other) noexcept {
    trace_ = other.trace_;
    pool_ = other.pool_;
    ++ref_count_[kRefInd];
    return *this;
  }

  constexpr reseving_allocator& operator=(reseving_allocator&& other) noexcept {
    trace_ = std::exchange(other.trace_, nullptr);
    pool_ = std::exchange(other.pool_, nullptr);
    return *this;
  }

  constexpr virtual ~reseving_allocator() noexcept {
    --trace_[kRefInd];
    if (!trace_[kRefInd]) {
      delete trace_;
      pool_node* fwd = pool_->next;
      for (; fwd; pool_ = fwd, fwd = fwd->next) {
        delete pool_;
      }
      delete pool_;
    }
  };

  //==============================================================================

  constexpr size_type capacity() const noexcept {
    return trace_[kCapInd];
  }

  //==============================================================================

  constexpr void swap(reseving_allocator& other) noexcept {
    std::swap(trace_, other.trace_);
    std::swap(pool_, other.pool_);
  }

  constexpr T* allocate(size_type count) {
    if (!trace_) {
      throw std::bad_alloc("pool_allocator: using moved-out allocator");
    } else if (pool_) {
      pool_node dead = pool_;
      T* ptr = pool_->object;
      pool_ = pool_->next;
      delete dead;
      return ptr;
    } else {
      return static_cast<T*>(operator new(count*sizeof(T)));
    }
  }

  constexpr void deallocate(T* pointer) noexcept {
    if (!trace_) {
      return;  // or throw?
    } else if ()
    pool_node* pl = new pool_node{pointer, pool_};
    pool_ = pl;
  }

  constexpr bool operator==(const reseving_allocator& other) const noexcept {
    return trace_ == other.trace_;
  }

  constexpr bool operator!=(const reseving_allocator& other) const noexcept {
    return trace_ != other.trace_;
  }

 private:
  struct pool_node {
    T* object;
    pool_node* next;
  }

  static constexpr kLimitInd = 0;
  static constexpr kCapInd = 1;
  static constexpr kRefInd = 2;

  size_type* trace_;
  pool_node* pool_;
};

template <typename T>
constexpr void swap(reseving_allocator<T>& lhs,
                    reseving_allocator<T>& rhs) noexcept {
  lhs.swap(rhs);
}
}  // namespace sp
#endif  // SP_MEMORY_LIMIT_ALLOCATOR_H_
