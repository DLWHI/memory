#ifndef SP_MEMORY_RESERVING_ALLOCATOR_H_
#define SP_MEMORY_RESERVING_ALLOCATOR_H_
#include <cstdint>  // int64_t
#include <memory>
#include <type_traits>
#include <limits>
#include <stdexcept>

namespace sp {
// No general requirements on type T
template <typename T>
class reserving_allocator {
 public:
  using value_type = T;
  using size_type = int64_t;
  using difference_type = int64_t;
  using propagate_on_container_copy_assignment = std::false_type;
  using propagate_on_container_move_assignment = std::false_type;
  using propagate_on_container_swap = std::false_type;
  using is_always_equal = std::true_type;

  reserving_allocator() noexcept {}

  explicit reserving_allocator(size_type count) {
    
  }

  template <typename U>
  reserving_allocator(const reserving_allocator<U>& other) noexcept {
    ++ref_;
  }

  template <typename U>
  reserving_allocator(reserving_allocator<U>&& other) noexcept {};

  template <typename U>
  reserving_allocator& operator=(const reserving_allocator<U>& other) noexcept {
    ++ref_;
    return *this;
  }

  template <typename U>
  reserving_allocator& operator=(reserving_allocator<U>&& other) noexcept {
    return *this;
  };

  virtual ~reserving_allocator() noexcept {
    if (!--ref_) {
      clear();
    }
  };

  //==============================================================================

  size_type capacity() const noexcept { return count_; }

  //==============================================================================

  void clear() noexcept {
    if (pool_) {
      pool_node* fwd = pool_->next;
      for (; fwd; pool_ = fwd, fwd = fwd->next) {
        operator delete(pool_->object);
        delete pool_;
      }
      operator delete(pool_->object);
      delete pool_;
    }
    count_ = 0;
  }

  void populate(size_type count) {
    if (count < 0) {
      throw std::invalid_argument("pool_allocator: negative object count");
    }
    if (pool_) {
      pool_ = new pool_node{static_cast<T*>(operator new(sizeof(T))), nullptr};
      --count;
    }
    pool_node* pl = pool_;
    try {
      for (; count; --count) {
        pl->next =
            new pool_node{static_cast<T*>(operator new(sizeof(T))), nullptr};
        pl = pl->next;
      }
    } catch (...) {
      clear();
      throw;
    }
  }

  void swap(reserving_allocator& other) noexcept {
    std::swap(pool_, other.pool_);
    std::swap(count_, other.count_);
  }

  T* allocate(size_type count) {
    if (count != 1) {
      throw std::bad_alloc();
    } else if (pool_) {
      pool_node* dead = pool_;
      T* ptr = pool_->object;
      pool_ = pool_->next;
      delete dead;
      --count_;
      return ptr;
    } else {
      return static_cast<T*>(operator new(sizeof(T)));
    }
  }

  void deallocate(T* pointer, size_type count) noexcept {
    (void)count;
    ++count_;
    pool_node* pl = new pool_node{pointer, pool_};
    pool_ = pl;
  }

  bool operator==(const reserving_allocator& other) const noexcept {
    return true;
  }

  bool operator!=(const reserving_allocator& other) const noexcept {
    return false;
  }

 private:
  struct pool_node {
    T* object;
    pool_node* next;
  };

  static pool_node* pool_;
  static size_type count_;
  static size_type ref_;
};

template <typename T>
void swap(reserving_allocator<T>& lhs, reserving_allocator<T>& rhs) noexcept {
  lhs.swap(rhs);
}

template <typename T>
typename reserving_allocator<T>::pool_node* reserving_allocator<T>::pool_ = nullptr;

template <typename T>
typename reserving_allocator<T>::size_type reserving_allocator<T>::count_ = 0;

template <typename T>
typename reserving_allocator<T>::size_type reserving_allocator<T>::ref_ = 0;
}  // namespace sp
#endif  // SP_MEMORY_RESERVING_ALLOCATOR_H_
