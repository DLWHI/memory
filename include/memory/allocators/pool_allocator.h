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
  using byte_type = uint8_t;
  
  template <typename U>
  friend class pool_allocator;

  struct trace_type {
    std::size_t allocd;
    std::size_t limit;
    std::size_t ref_count;
    byte_type* state;
    byte_type* pool;
  };

 public:
  using value_type = T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using propagate_on_container_copy_assignment = std::false_type;
  using propagate_on_container_move_assignment = std::false_type;
  using propagate_on_container_swap = std::true_type;

  constexpr explicit pool_allocator(size_type size) 
      : trace_(alloc_trace(size)) {
    trace_->state =  reinterpret_cast<byte_type*>(trace_ + 3*sizeof(std::size_t));
    trace_->pool = trace_->state + (size + 8)/8;
    trace_->allocd = 0;
    trace_->limit = size;
    trace_->ref_count = 1;
  }

  template <typename U>
  constexpr pool_allocator(const pool_allocator<U>& other) noexcept
      : trace_(reinterpret_cast<trace_type*>(other.trace_)) {
    ++trace_->ref_count;
  }

  template <typename U>
  constexpr pool_allocator(pool_allocator<U>&& other) noexcept
      : pool_allocator(other) {} 

  constexpr pool_allocator(const pool_allocator& other) noexcept
      : trace_(reinterpret_cast<trace_type*>(other.trace_)) {
    ++trace_->ref_count;
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
    --trace_->ref_count;
    if (!trace_->ref_count) {
      operator delete(trace_);
    }
  };

  //==============================================================================

  constexpr size_type max_size() const noexcept {
    return trace_->limit / sizeof(T);
  }

  constexpr size_type allocd() const noexcept {
    return trace_->allocd;
  }

  constexpr size_type remaining() const noexcept {
    return trace_->limit - trace_->allocd;
  }
  //==============================================================================

  constexpr void swap(pool_allocator& other) noexcept {
    if (trace_ != other.trace_) {
      std::swap(trace_, other.trace_);
    }
  }

  constexpr T* allocate(size_type count) {
    size_type chunk_size = count * sizeof(T);
    bit_iterator first(trace_->state);
    bit_iterator last = first;
    bit_iterator end(trace_->state, trace_->limit);
    for (; last != end && last.position() - first.position() < chunk_size; ++last) {
      if (*last) {
        first = last;
      }
    }
    if (last.position() - first.position() < chunk_size) {
      throw std::bad_alloc();  // write own bad_alloc?
    }
    for (bit_iterator i = first; i != last; i.flip(), ++i) {}
    trace_->allocd += chunk_size;
    std::cout << "allocd ptr " << trace_->pool + first.position() << "of size " << last.position() - first.position() << std::endl; 
    return reinterpret_cast<T*>(trace_->pool + first.position());
  }

  constexpr void deallocate(T* pointer, size_type count) noexcept {
    size_type chunk_size = count * sizeof(T);
    byte_type* bptr = reinterpret_cast<byte_type*>(pointer);
    if (bptr - trace_->pool > trace_->limit) { return;}
    bit_iterator first(trace_->state, bptr - trace_->pool);
    for (size_type i = 0; i < chunk_size; first.flip(), ++first, ++i) {}
    trace_->state -= chunk_size;
    std::cout << "deallocd ptr: " << bptr << std::endl; 
   }

  constexpr bool operator==(const pool_allocator& other) const noexcept {
    return trace_ == other.trace_;
  }

  constexpr bool operator!=(const pool_allocator& other) const noexcept {
    return trace_ != other.trace_;
  }

 private:

  trace_type* alloc_trace(std::size_t size) {
    return (size) ? reinterpret_cast<trace_type*>(operator new(3*sizeof(std::size_t) + (size + 8)/8 + size)) : throw std::bad_alloc();
  }

  trace_type* trace_;
};

template <typename T>
void swap(pool_allocator<T>& lhs, pool_allocator<T>& rhs) noexcept {
  lhs.swap(rhs);
}
}  // namespace memory
#endif  // MEMORY_ALLOCATORS_POOL_ALLOCATOR_H_

