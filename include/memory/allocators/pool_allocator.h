#ifndef MEMORY_ALLOCATORS_POOL_ALLOCATOR_H_
#define MEMORY_ALLOCATORS_POOL_ALLOCATOR_H_
#include <cstring>
#include <memory>
#include <type_traits>
#include <stdexcept>

#include "../iterators/bit_iterator.h"

#if __cplusplus >= 202002L
#define MEMORY_CPP20CONSTEXPR constexpr 
#else
#define MEMORY_CPP20CONSTEXPR
#endif  // 202002L

namespace memory {
// No general requirements on type T
template <typename T>
class pool_allocator {
  template <typename U>
  friend class pool_allocator;

  struct trace_type {
    std::size_t allocd;
    std::size_t limit;
    std::size_t ref_count;
  };

 public:
  using value_type = T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using propagate_on_container_copy_assignment = std::false_type;
  using propagate_on_container_move_assignment = std::false_type;
  using propagate_on_container_swap = std::true_type;

  MEMORY_CPP20CONSTEXPR explicit pool_allocator(size_type size) 
      : trace_(alloc_trace(size)) {
    pool_ = state() + (size + 7)/8;
    trace_->allocd = 0;
    trace_->limit = size;
    trace_->ref_count = 1;
  }

  template <typename U>
  MEMORY_CPP20CONSTEXPR pool_allocator(const pool_allocator<U>& other) noexcept
      : pool_(other.pool_), trace_(reinterpret_cast<trace_type*>(other.trace_)) {
    ++trace_->ref_count;
  }

  template <typename U>
  MEMORY_CPP20CONSTEXPR pool_allocator(pool_allocator<U>&& other) noexcept
      : pool_allocator(other) {} 

  MEMORY_CPP20CONSTEXPR pool_allocator(const pool_allocator& other) noexcept
      : pool_(other.pool_), trace_(reinterpret_cast<trace_type*>(other.trace_)) {
    ++trace_->ref_count;
  }

  MEMORY_CPP20CONSTEXPR pool_allocator(pool_allocator&& other) noexcept
      : pool_allocator(other) {}

  template <typename U>
  pool_allocator& operator=(const pool_allocator<U>& other) = delete;

  template <typename U>
  pool_allocator& operator=(pool_allocator<U>&&) = delete;

  pool_allocator& operator=(const pool_allocator& other) = delete;

  pool_allocator& operator=(pool_allocator&&) = delete;

  MEMORY_CPP20CONSTEXPR virtual ~pool_allocator() noexcept(false) {
    --trace_->ref_count;
    if (!trace_->ref_count) {
      for (uint8_t *p = state(); p != pool_; ++p) {
      if (*p) {
        operator delete(trace_);
        throw std::runtime_error("Memory leak detected: attempting to destroy pool allocator that has memory being used and not dealloc'd'");  // AOAOOOAOAOAOOAOAOAOAAOAO
      }
    }
      operator delete(trace_);
    }
  };

  //==============================================================================

  MEMORY_CPP20CONSTEXPR size_type max_size() const noexcept {
    return trace_->limit / sizeof(T);
  }

  MEMORY_CPP20CONSTEXPR size_type allocd() const noexcept {
    return trace_->allocd;
  }

  MEMORY_CPP20CONSTEXPR size_type remaining() const noexcept {
    return trace_->limit - trace_->allocd;
  }
  //==============================================================================

  MEMORY_CPP20CONSTEXPR void swap(pool_allocator& other) noexcept {
    if (trace_ != other.trace_) {
      std::swap(trace_, other.trace_);
      std::swap(pool_, other.pool_);
    }
  }

  MEMORY_CPP20CONSTEXPR T* allocate(size_type count) {
    size_type chunk_size = count * sizeof(T);
    bit_iterator first(state());
    bit_iterator last = first;
    bit_iterator end(state(), trace_->limit);
    for (; last != end && last.position() - first.position() < chunk_size; ++last) {
      if (*last) {
        ++first;
      }
    }
    if (last.position() - first.position() < chunk_size) {
      throw std::bad_alloc();  // write own bad_alloc?
    }
    for (bit_iterator i = first; i != last; i.flip(), ++i) {}
    trace_->allocd += chunk_size;
    return reinterpret_cast<T*>(pool_ + first.position());
  }

  MEMORY_CPP20CONSTEXPR void deallocate(T* ptr, size_type count) noexcept {
    size_type chunk_size = count * sizeof(T);
    int64_t offs = reinterpret_cast<uint8_t*>(ptr) - pool_;
    if (offs > trace_->limit) { return;}
    bit_iterator first(state(), offs);
    for (size_type i = 0; i < chunk_size; first.flip(), ++first, ++i) {}
    trace_->allocd -= chunk_size;
   }

  MEMORY_CPP20CONSTEXPR bool operator==(const pool_allocator& other) const noexcept {
    return trace_ == other.trace_;
  }

  MEMORY_CPP20CONSTEXPR bool operator!=(const pool_allocator& other) const noexcept {
    return trace_ != other.trace_;
  }

 private:
  MEMORY_CPP20CONSTEXPR uint8_t* state() noexcept {
    return reinterpret_cast<uint8_t*>(trace_ + 1);
  }

  trace_type* alloc_trace(std::size_t size) {
    if (!size) throw std::bad_alloc();
    std::size_t trace_size = sizeof(trace_type) + (size + 7)/8 + size;
    trace_type* ptr = reinterpret_cast<trace_type*>(operator new(trace_size));
    std::memset(ptr, 0, trace_size);
    return ptr;
  }

  uint8_t* pool_;
  trace_type* trace_;
};

template <typename T>
void swap(pool_allocator<T>& lhs, pool_allocator<T>& rhs) noexcept {
  lhs.swap(rhs);
}
}  // namespace memory

#undef MEMORY_CPP20CONSTEXPR

#endif  // MEMORY_ALLOCATORS_POOL_ALLOCATOR_H_

