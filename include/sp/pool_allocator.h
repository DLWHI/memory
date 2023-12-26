#ifndef SP_MEMORY_LIMIT_ALLOCATOR_H_
#define SP_MEMORY_LIMIT_ALLOCATOR_H_
#include <cstdint>  // int64_t
#include <memory>
#include <type_traits>
#include <stdexcept>

namespace sp {
// No general requirements on type T
template <typename T>
class pool_allocator {
 public:
  using value_type = T;
  using size_type = int64_t;
  using difference_type = int64_t;
  using propagate_on_container_copy_assignment = std::false_type;
  using propagate_on_container_move_assignment = std::false_type;
  using propagate_on_container_swap = std::true_type;

  constexpr explicit pool_allocator(size_type pool_size)
      : trace_(new size_type[2]{pool_size, 0}),
        pool_head_(new pool_node{nullptr, 0, nullptr, nullptr}) {
    if (pool_size < 0) {
      throw std::invalid_argument("pool_allocator: negative pool size");
    }
    pool_ = operator new(pool_size);
    pool_node* pl = new pool_node{pool_, trace_[kLimitInd], nullptr, nullptr};
    pool_head_->bind(pl, pl);
  }

  constexpr pool_allocator(const pool_allocator& other) noexcept
      : trace_(other.trace_), pool_(other.pool_), pool_head_(other.pool_head_) {
    ++trace_[kRefInd];
  }
  
  constexpr pool_allocator(pool_allocator&& other) noexcept
      : trace_(other.trace_), pool_(other.pool_), pool_head_(other.pool_head_) {
    other.trace_ = nullptr;
    other.pool_ = nullptr;
    other.pool_head_ = nullptr;
  }

  constexpr pool_allocator& operator=(const pool_allocator& other) noexcept {
    trace_ = other.trace_;
    pool_ = other.pool_;
    pool_head_ = other.pool_head_;
    ++ref_count_[kRefInd];
    return *this;
  }

  constexpr pool_allocator& operator=(pool_allocator&& other) noexcept {
    trace_ = std::exchange(other.trace_, nullptr);
    pool_ = std::exchange(other.pool_, nullptr);
    pool_head_ = std::exchange(other.pool_head_, nullptr);
    return *this;
  }

  constexpr virtual ~pool_allocator() noexcept {
    --trace_[kRefInd];
    if (!trace_[kRefInd]) {
      operator delete(pool_, trace_[kLimitInd]);
      delete trace_;
      delete pool_head_->next;
      delete pool_head_;
    }
  };

//==============================================================================

constexpr size_type max_size() const noexcept { return trace_[kLimitInd]; }

constexpr size_type leftover() const noexcept { 
  pool_node* pl = pool_head_->next;
  size_type acc = 0;
  for (; pl != pool_head_; pl = pl->next) {
    acc += pl->size;
  }
  return acc;
}

//==============================================================================

  constexpr void swap(pool_allocator& other) noexcept {
    std::swap(trace_, other.trace_);
    std::swap(pool_, other.pool_);
    std::swap(pool_head_, other.pool_head_);
  }

  constexpr T* allocate(size_type count) {
    if (!trace_) {
      throw std::bad_alloc("pool_allocator: using moved-out allocator");
    } else if (!pool_head_->ptr) {
      throw std::bad_alloc("pool_allocator: limit exceeded");
    }
    pool_node* pl = pool_head_->next;
    for (; pl != pool_head_ && pl->size < count; pl = pl->next) {
    }
    T* ptr = pl->ptr;
    pl->size -= count;
    if (!pl->size) {
      pl->unbind();
      delete pl;
    }
    return ptr;
  }

  constexpr void deallocate(T* pointer, size_type count) noexcept {
    if (!trace_) {
      return;  // or throw?
    } else if (pointer > pool_ || pointer >= pool_ + trace_[kLimitInd]) {
      return;
    }
    pool_node* pl = pool_head_->next;
    for (; pointer < pl->ptr; pl = pl->next) {
    }
    T* end = pl->ptr + pl->size;
    if (end > pointer || pl->next->ptr < pointer + count) {
      return;  // double free, must throw or make ub?
    } else if (pointer > end) {
      pool_node* return_node = new pool_node{pointer, count, nullptr, nullptr};
      return_node->bind(pl, pl->next);
    } else {
      pl->size += count;
    }
  }

  constexpr bool operator==(const pool_allocator& other) const noexcept {
    return trace_ == other.trace_;
  }

  constexpr bool operator!=(const pool_allocator& other) const noexcept {
    return trace_ != other.trace_;
  }

 private:
  struct pool_node {
    void bind(pool_node* prv, pool_node* nxt) noexcept {
      prev = prv;
      next = nxt;
      prv->next = this;
      nxt->prev = this;
    }

    void unbind() noexcept {
      next->prev = prev;
      prev->next = next;
      prev = nullptr;
      next = nullptr;
    }

    T* ptr;
    size_type size;
    pool_node* next;
    pool_node* prev;
  }

  static constexpr kLimitInd = 0;
  static constexpr kRefInd = 1;

  size_type* trace_;
  T* pool_;
  pool_node* pool_head_;
};

template <typename T>
constexpr void swap(pool_allocator<T>& lhs, pool_allocator<T>& rhs) noexcept {
  lhs.swap(rhs);
}
}  // namespace sp
#endif  // SP_MEMORY_LIMIT_ALLOCATOR_H_
