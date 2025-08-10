#ifndef MEMORY_ALLOCATORS_POOL_ALLOCATOR_H_
#define MEMORY_ALLOCATORS_POOL_ALLOCATOR_H_
#include <memory>
#include <type_traits>
#include <stdexcept>

#include "../iterators/bit_iterator.h"

namespace memory {
// No general requirements on type T
template <typename T>
class pool_allocator {
  template <typename U>
  friend class pool_allocator;

  using byte_t = uint8_t;

 public:
  using value_type = T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using propagate_on_container_copy_assignment = std::false_type;
  using propagate_on_container_move_assignment = std::false_type;
  using propagate_on_container_swap = std::true_type;

  constexpr explicit pool_allocator(size_type size) 
      : trace_(new size_type[3 + (size + 8) / 8]{}),
        pool_(reinterpret_cast<byte_t*>(operator new(size))) {
    trace_[kLimitInd] = size;
    trace_[kRefInd] = 1;
  }

  template <typename U>
  constexpr pool_allocator(const pool_allocator<U>& other) noexcept
      : trace_(other.trace_), pool_(other.pool_) {
    ++trace_[kRefInd];
  }

  template <typename U>
  constexpr pool_allocator(pool_allocator<U>&& other) noexcept
      : pool_allocator(other) {}

  constexpr pool_allocator(const pool_allocator& other) noexcept
      : trace_(other.trace_), pool_(other.pool_) {
    ++trace_[kRefInd];
  }

  constexpr pool_allocator(pool_allocator&& other) noexcept
      : pool_allocator(other) {}

  template <typename U>
  pool_allocator& operator=(const pool_allocator<U>& other) = delete;

  template <typename U>
  pool_allocator& operator=(pool_allocator<U>&&) = delete;

  pool_allocator& operator=(const pool_allocator& other) = delete;

  pool_allocator& operator=(pool_allocator&&) = delete;

  constexpr virtual ~pool_allocator() noexcept {
    --trace_[kRefInd];
    if (!trace_[kRefInd]) {
      operator delete(pool_);
      delete[] trace_;
    }
  };

  //==============================================================================

  constexpr size_type max_size() const noexcept {
    if (trace_) {
      return trace_[kLimitInd] / sizeof(T);
    }
    return 0;
  }

  constexpr size_type allocd() const noexcept {
    if (trace_) {
      return trace_[kStateInd];
    }
    return 0;
  }

  constexpr size_type remaining() const noexcept {
    if (trace_) {
      return trace_[kLimitInd] - trace_[kStateInd];
    }
    return 0;
  }
  //==============================================================================

  constexpr void swap(pool_allocator& other) noexcept {
    if (trace_ != other.trace_) {
      std::swap(trace_, other.trace_);
      std::swap(pool_, other.pool_);
    }
  }

  constexpr T* allocate(size_type count) {
    size_type chunk_size = count * sizeof(T);
    bit_iterator first(trace_ + kByteStateStart);
    bit_iterator last = first;
    bit_iterator end(trace_ + kByteStateStart, trace_[kLimitInd]);
    for (; last != end && last.position() - first.position() < chunk_size; ++last) {
      if (*last) {
        first = last;
      }
    }
    if (last.position() - first.position() < chunk_size) {
      throw std::bad_alloc();  // write own bad_alloc?
    }
    for (bit_iterator i = first; i != last; i.flip(), ++i) {}
    trace_[kStateInd] += chunk_size;
    return reinterpret_cast<T*>(pool_ + first.position());
  }

  constexpr void deallocate(T* pointer, size_type count) noexcept {
    size_type chunk_size = count * sizeof(T);
    byte_t* bptr = reinterpret_cast<byte_t*>(pointer);
    bit_iterator first(trace_ + kByteStateStart, bptr - pool_);
    for (size_type i = 0; i < chunk_size; first.flip(), ++first, ++i) {}
    trace_[kStateInd] -= chunk_size;
  }

  constexpr bool operator==(const pool_allocator& other) const noexcept {
    return trace_ == other.trace_;
  }

  constexpr bool operator!=(const pool_allocator& other) const noexcept {
    return trace_ != other.trace_;
  }

 private:
  static constexpr size_type kLimitInd = 0;
  static constexpr size_type kRefInd = 1;
  static constexpr size_type kStateInd = 2;
  static constexpr size_type kByteStateStart = 3;

  

  size_type* trace_;
  byte_t* pool_;
};

template <typename T>
void swap(pool_allocator<T>& lhs, pool_allocator<T>& rhs) noexcept {
  lhs.swap(rhs);
}
}  // namespace memory
#endif  // MEMORY_ALLOCATORS_POOL_ALLOCATOR_H_
