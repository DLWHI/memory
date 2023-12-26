#ifndef SP_MEMORY_LIMIT_ALLOCATOR_H_
#define SP_MEMORY_LIMIT_ALLOCATOR_H_
#include <cstdint>  // int64_t
#include <memory>
#include <type_traits>
#include <stdexcept>
#include <exception>

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

  explicit pool_allocator(size_type pool_size)
      : trace_(new size_type[3]{pool_size, 0, 1}),
        pool_head_(new pool_node{nullptr, 0, nullptr, nullptr}) {
    if (pool_size < 0) {
      throw std::invalid_argument("pool_allocator: negative pool size");
    }
    pool_ = static_cast<T*>(operator new(pool_size * sizeof(T)));
    pool_node* pl = new pool_node{pool_, trace_[kLimitInd], nullptr, nullptr};
    pool_head_->bind(pl, pl);
  }

  pool_allocator(const pool_allocator& other) noexcept
      : trace_(other.trace_), pool_(other.pool_), pool_head_(other.pool_head_) {
    if (trace_) {
      ++trace_[kRefInd];
    }
  }

  pool_allocator(pool_allocator&& other) noexcept
      : trace_(other.trace_), pool_(other.pool_), pool_head_(other.pool_head_) {
    other.trace_ = nullptr;
    other.pool_ = nullptr;
    other.pool_head_ = nullptr;
  }

  pool_allocator& operator=(const pool_allocator& other) noexcept {
    trace_ = other.trace_;
    pool_ = other.pool_;
    pool_head_ = other.pool_head_;
    if (trace_) {
      ++trace_[kRefInd];
    }
    return *this;
  }

  pool_allocator& operator=(pool_allocator&& other) noexcept {
    trace_ = std::exchange(other.trace_, nullptr);
    pool_ = std::exchange(other.pool_, nullptr);
    pool_head_ = std::exchange(other.pool_head_, nullptr);
    return *this;
  }

  virtual ~pool_allocator() noexcept {
    if (trace_) {
      --trace_[kRefInd];
      if (!trace_[kRefInd]) {
        operator delete(pool_, trace_[kLimitInd] * sizeof(T));
        delete trace_;
        delete pool_head_->next;
        delete pool_head_;
      }
    }
  };

  //==============================================================================

  size_type max_size() const noexcept {
    if (trace_) {
      return trace_[kLimitInd];
    }
    return 0;
  }

  size_type leftover() const noexcept {
    if (trace_) {
      return trace_[kLimitInd] - trace_[kStateInd];
    }
    return 0;
  }
  //==============================================================================

  void swap(pool_allocator& other) noexcept {
    std::swap(trace_, other.trace_);
    std::swap(pool_, other.pool_);
    std::swap(pool_head_, other.pool_head_);
  }

  T* allocate(size_type count) {
    if (!trace_ || !pool_head_->next->ptr ||
        trace_[kStateInd] + count > trace_[kLimitInd]) {
      throw std::bad_alloc();  // write own bad_alloc?
    }
    pool_node* pl = pool_head_->next;
    for (; pl != pool_head_ && pl->size < count; pl = pl->next) {
    }
    T* ptr = pl->ptr;
    pl->size -= count;
    pl->ptr += count;
    if (!pl->size) {
      pl->unbind();
      delete pl;
    }
    trace_[kStateInd] += count;
    return ptr;
  }

  void deallocate(T* pointer, size_type count) noexcept {
    if (!trace_) {
      return;
    }
    pool_node* pl = pool_head_->next;
    for (; pointer < pl->ptr; pl = pl->next) {
    }
    T* end = pl->ptr + pl->size;
    if (end == pointer) {
      pl->size += count;
    } else {
      pool_node* return_node = new pool_node{pointer, count, nullptr, nullptr};
      return_node->bind(pl, pl->next);
      pl = return_node;
    }
    if (pointer + count == pl->next->ptr) {
      pl->size += pl->next->size;
      pl = pl->next;
      pl->unbind();
      delete pl;
    }
    trace_[kStateInd] -= count;
  }

  bool operator==(const pool_allocator& other) const noexcept {
    return trace_ == other.trace_;
  }

  bool operator!=(const pool_allocator& other) const noexcept {
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
  };

  static constexpr size_type kLimitInd = 0;
  static constexpr size_type kStateInd = 1;
  static constexpr size_type kRefInd = 2;

  size_type* trace_;
  T* pool_;
  pool_node* pool_head_;
};

template <typename T>
void swap(pool_allocator<T>& lhs, pool_allocator<T>& rhs) noexcept {
  lhs.swap(rhs);
}
}  // namespace sp
#endif  // SP_MEMORY_LIMIT_ALLOCATOR_H_
