#ifndef MEMORY_CONTAINERS_VECTOR_H_
#define MEMORY_CONTAINERS_VECTOR_H_

#include "../iterators/pointer_iterator.h"  // iterator and std::distance
#include "../iterators/reverse_iterator.h"

#include <cstdint>      // types
#include <ostream>      // operator<<
#include <stdexcept>    // exceptions
#include <type_traits>  // as name suggests
#include <utility>      // std::forward, std::swap, std::move

#if __cplusplus >= 202002L
#define MEMORY_CPP20CONSTEXPR constexpr 
#else
#define MEMORY_CPP20CONSTEXPR
#endif  // 202002L

namespace memory {
// use it at your own risk
//
// T is Erasable
// Allocator is Allocator
// Methods may have additional requirements on types
template <typename T, class Allocator = std::allocator<T>>
class vector {
 public:
  using value_type = T;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

  using allocator_type = Allocator;

  using iterator = memory::pointer_iterator<T, vector>;
  using const_iterator = memory::pointer_iterator<const T, vector>;
  using reverse_iterator = memory::reverse_iterator<iterator>;
  using const_reverse_iterator = memory::reverse_iterator<const_iterator>;

  static constexpr size_type kCapMul = 2;

  // Allocator is DefaultConstructible
  MEMORY_CPP20CONSTEXPR vector() noexcept(
      std::is_nothrow_default_constructible<Allocator>::value)
      : size_(0), cap_(0), ptr_(nullptr) {}

  MEMORY_CPP20CONSTEXPR explicit vector(const Allocator& al) noexcept
      : size_(0), cap_(0), al_(al), ptr_(nullptr) {}

  // T is DefaultInsertable into *this
  MEMORY_CPP20CONSTEXPR explicit vector(size_type size, const Allocator& al = Allocator())
      : size_(size), cap_(size), al_(al), ptr_(alloc(size_)) {
    try {
      construct(ptr_, size_);
    } catch(...) {
      dealloc(ptr_, size_);
      throw;
    }
  }

  // T is CopyInsertable into *this
  MEMORY_CPP20CONSTEXPR explicit vector(size_type size, const_reference value,
                            const Allocator& al = Allocator())
      : size_(size), cap_(size), al_(al), ptr_(alloc(size_)) {
    try {
      construct(ptr_, size_, value);
    } catch(...) {
      dealloc(ptr_, size_);
      throw;
    }
  };

  // T is EmplaceConstructible and MoveInsertable (if InputIterator does not meet the FwdIt requirements) into *this
  template <typename InputIterator>
  MEMORY_CPP20CONSTEXPR vector(InputIterator first, InputIterator last,
                   const Allocator& al = Allocator())
      : al_(al) {
    if constexpr (std::is_base_of<std::forward_iterator_tag, typename std::iterator_traits<InputIterator>::iterator_category>::value) {
      size_ = cap_ = std::distance(first, last);
      ptr_ = alloc(size_);
      try {
        fill(ptr_, first, last);
      } catch(...) {
        dealloc(ptr_, size_);
        throw;
      }
    } else {
      size_ = cap_ = 0;
      ptr_ = nullptr;
      try {
        for (; first != last; ++first) {
          emplace_back(*first);
        }
      } catch(...) {
        destroy(ptr_, size_);
        dealloc(ptr_, cap_);
        throw;
      }
    }
  }

  // T is EmplaceConstructible
  MEMORY_CPP20CONSTEXPR vector(std::initializer_list<T>  values,
                   const Allocator& al = Allocator())
      : vector(values.begin(), values.end(), al) {}
  
  // T is CopyInsertable into *this
  MEMORY_CPP20CONSTEXPR vector(const vector& other)
      : vector(other.ptr_, other.ptr_ + other.size_, std::allocator_traits<Allocator>::select_on_container_copy_construction(other.al_)) {}

  // T is CopyInsertable into *this
  MEMORY_CPP20CONSTEXPR vector(const vector& other, const Allocator& al)
      : vector(other.ptr_, other.ptr_ + other.size_, al) {};

  // No additional requirements on template types
  MEMORY_CPP20CONSTEXPR vector(vector&& other) noexcept
      : size_(other.size_),
        cap_(other.cap_),
        al_(std::move(other.al_)),
        ptr_(other.ptr_) {
    other.ptr_ = nullptr;
    other.cap_ = 0;
    other.size_ = 0;
  }

  // T is MoveInsertable into *this
  MEMORY_CPP20CONSTEXPR vector(vector&& other, const Allocator& al)
      noexcept(std::allocator_traits<Allocator>::is_always_equal::value)
      : vector(al) {
    if (al == other.al_) {
      swap(other);
    } else {
      ptr_ = alloc(other.size_);
      try {
        move(ptr_, other.ptr_, other.ptr_ + other.size_);
      } catch(...) {
        dealloc(ptr_, other.size_);
        if constexpr (!std::allocator_traits<Allocator>::is_always_equal::value) throw;
      }
      cap_ = size_ = other.size_;
    }
  }

  // T is CopyInsertable and CopyAssignable into *this
  MEMORY_CPP20CONSTEXPR vector& operator=(const vector& other) {
    if (ptr_ == other.ptr_) {
      return *this;
    }
    if constexpr (std::allocator_traits<Allocator>::propagate_on_container_copy_assignment::value) {
      pointer p = ptr_;
      if (al_ != other.al_) {
        p = other.alloc(other.size_);
        try {
          other.fill(p,  other.ptr_, other.ptr_ + other.size_);
        } catch(...) {
          other.dealloc(p,  other.size_);
          throw;
        }
        swap_out_buffer(p,  other.size);
        al_ = other.al_;
      }
    } else {
      copy_assign(other.size_, other.ptr_, other.ptr_ + other.size_);      
    }
    return *this;
  }

  // T is  MoveInsertable into *this and MoveAssignable if
  //  allocator_traits<Allocator>::propagate_on_container_move_assignment::value is false
  MEMORY_CPP20CONSTEXPR vector& operator=(vector&& other) noexcept(
      std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value ||
      std::allocator_traits<Allocator>::is_always_equal::value) {
    if (ptr_ == other.ptr_) {
      return *this;
    }
    if constexpr (std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value || std::allocator_traits<Allocator>::is_always_equal::value) {
      swap(other);
    } else {
      if (al_ != other.al_) {
        move_assign(other.size_, other.ptr_, other.ptr_ + other.size_);      
      } else {
        swap(other); 
      }
    }
    return *this;   
  }
  
  MEMORY_CPP20CONSTEXPR vector& operator=(std::initializer_list<T>  ilist) {
    assign(ilist.begin(), ilist.end());
    return *this;   
  }
  
  // No additional requirements on template types
  MEMORY_CPP20CONSTEXPR virtual ~vector() noexcept { 
    clear(); 
    dealloc(ptr_, cap_);
  }

  //============================================================================
  // No additional requirements on template types for all methods below

  MEMORY_CPP20CONSTEXPR allocator_type get_allocator() const noexcept { return al_; }

  MEMORY_CPP20CONSTEXPR reference at(size_type pos) {
    return (pos < size_)
               ? ptr_[pos]
               : throw std::out_of_range("Accessing element out of bounds");
  }

  MEMORY_CPP20CONSTEXPR const_reference at(size_type pos) const {
    return (pos < size_)
               ? ptr_[pos]
               : throw std::out_of_range("Accessing element out of bounds");
  }

  MEMORY_CPP20CONSTEXPR reference front() noexcept { return ptr_[0]; }
  MEMORY_CPP20CONSTEXPR reference back() noexcept { return ptr_[size_ - 1]; }
  MEMORY_CPP20CONSTEXPR pointer data() noexcept { return ptr_; }

  MEMORY_CPP20CONSTEXPR const_reference front() const noexcept { return ptr_[0]; }
  MEMORY_CPP20CONSTEXPR const_reference back() const noexcept {
    return ptr_[size_ - 1];
  }
  MEMORY_CPP20CONSTEXPR const_pointer data() const noexcept { return ptr_; }

  MEMORY_CPP20CONSTEXPR iterator begin() noexcept { return iterator(ptr_); }
  MEMORY_CPP20CONSTEXPR const_iterator begin() const noexcept {
    return const_iterator(ptr_);
  }
  MEMORY_CPP20CONSTEXPR const_iterator cbegin() const noexcept {
    return const_iterator(ptr_);
  }

  MEMORY_CPP20CONSTEXPR iterator end() noexcept { return iterator(ptr_ + size_); }
  MEMORY_CPP20CONSTEXPR const_iterator end() const noexcept {
    return const_iterator(ptr_ + size_);
  }
  MEMORY_CPP20CONSTEXPR const_iterator cend() const noexcept {
    return const_iterator(ptr_ + size_);
  }

  MEMORY_CPP20CONSTEXPR reverse_iterator rbegin() noexcept {
    return reverse_iterator(end());
  }
  MEMORY_CPP20CONSTEXPR const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator(end());
  }
  MEMORY_CPP20CONSTEXPR const_reverse_iterator crbegin() const noexcept {
    return const_reverse_iterator(cend());
  }

  MEMORY_CPP20CONSTEXPR reverse_iterator rend() noexcept {
    return reverse_iterator(begin());
  }
  MEMORY_CPP20CONSTEXPR const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator(begin());
  }
  MEMORY_CPP20CONSTEXPR const_reverse_iterator crend() const noexcept {
    return const_reverse_iterator(begin());
  }

  MEMORY_CPP20CONSTEXPR bool empty() const noexcept { return !size_; }
  MEMORY_CPP20CONSTEXPR size_type size() const noexcept { return size_; }
  MEMORY_CPP20CONSTEXPR size_type capacity() const noexcept { return cap_; }
  MEMORY_CPP20CONSTEXPR size_type max_size() const noexcept {
    return std::allocator_traits<Allocator>::max_size(al_);
  }
  //============================================================================

  // T is CopyAssignable and CopyInsertable into *this
  MEMORY_CPP20CONSTEXPR void assign(size_type count, const_reference value) {
    if (count > max_size()) {
      throw std::length_error("Invalid count provided");
    }
    pointer p = ptr_;
    if (cap_ < count) {
      p = create_buffer(count, count, 0, value);
      swap_out_buffer(p, count);
    } else {
      pointer end = ptr_ + size_;
      size_type i = count;
      for (; p != end && i; --i, ++p) {
        *p = value;
      }
      if (p != end) {
        destroy(p, end - p);
      } else {
        construct(p, i, value);
      }
    }
    size_ = count;
  }

  // T must meet additional requirement of EmplaceConstructible into *this
  //  and assignable from InputIterator (I guess CopyAssignable would be okay)
  //  and MoveInsertable if InputIterator does not satisfy FwdIt
  template <typename InputIterator>
  MEMORY_CPP20CONSTEXPR void assign(InputIterator first, InputIterator last) {
    size_type count = std::distance(first, last);
    if (count > max_size()) {
      throw std::length_error("Too big range provided");
    }
    copy_assign(count, first, last);
  }

  // Same as assign(values.begin(), values.end())
  MEMORY_CPP20CONSTEXPR void assign(std::initializer_list<T>  values) {
    return assign(values.begin(), values.end());
  }

  // T must meet additional requirements of MoveInsertable into *this
  MEMORY_CPP20CONSTEXPR void reserve(size_type count) {
    if (count > max_size()) {
      throw std::length_error("Cannot reserve space more than max_size()");
    } else if (count > cap_) {
      pointer p = create_buffer(count);
      swap_out_buffer(p, count);
    }
  }

  // T must meet additional requirements of MoveInsertable into *this
  MEMORY_CPP20CONSTEXPR void shrink_to_fit() {
    if (cap_ > size_) {
      pointer p = create_buffer(size_);
      swap_out_buffer(p, size_);
    }
  }

  // No additional requirements on template types
  MEMORY_CPP20CONSTEXPR void clear() noexcept {
    destroy(ptr_, size_);
    size_ = 0;
  }

  // T must meet additional requirements of
  //  MoveInsertable and DefaultInsertable into *this
  MEMORY_CPP20CONSTEXPR void resize(size_type count) {
    if (count > max_size()) {
      throw std::length_error("Cannot resize more than max_size()");
    }
    if (count == size_) {
      return;
    } else if (count > cap_) {
      pointer p = create_buffer(count);
      try {
        construct(p + size_, count - size_);
      } catch(...) {
        destroy(p, size_);
        dealloc(p, count);
        throw;
      }
      swap_out_buffer(p, count);
    } else if (count > size_){
      construct(ptr_ + size_, count - size_);
    } else {
      destroy(ptr_ + count, size_ - count);
    }
    size_ = count;
  }

  // T must meet additional requirements of CopyInsertable into *this
  MEMORY_CPP20CONSTEXPR void resize(size_type count, const_reference value) {
    if (count > max_size()) {
      throw std::length_error("Cannot resize more than max_size()");
    }
    if (count == size_) {
      return;
    } else if (count > cap_) {
      pointer p = create_buffer(count);
      try {
        construct(p + size_, count - size_, value);
      } catch(...) {
        destroy(p, size_);
        dealloc(p, count);
        throw;
      }
      swap_out_buffer(p, count);
    } else if (count > size_){
      construct(ptr_ + size_, count - size_, value);
    } else {
      destroy(ptr_ + count, size_ - count);
    }
    size_ = count;
  }

//==============================================================================

  // T must meet additional requirements of CopyInsertable
  MEMORY_CPP20CONSTEXPR void push_back(const_reference value) { emplace_back(value); }

  // T must meet additional requirements of MoveInsertable
  MEMORY_CPP20CONSTEXPR void push_back(value_type&& value) {
    emplace_back(std::forward<T> (value));
  }

  // No additional requirements on template types
  MEMORY_CPP20CONSTEXPR void pop_back() noexcept(std::is_nothrow_destructible<T>::value) {
    std::allocator_traits<Allocator>::destroy(al_, ptr_ + size_ - 1);
    --size_;
  }

  // T is CopyAssignable and CopyInsertable into *this
  MEMORY_CPP20CONSTEXPR iterator insert(const_iterator pos, const_reference value) {
    return emplace(pos, value);
  }

  // T must meet additional requirements of MoveAssignable
  //   and MoveInsertable into *this
  MEMORY_CPP20CONSTEXPR iterator insert(const_iterator pos, value_type&& value) {
    return emplace(pos, std::forward<T>(value));
  }

  // T is CopyAssignable and CopyInsertable into *this
  MEMORY_CPP20CONSTEXPR iterator insert(const_iterator pos, size_type count, const_reference value) {
    size_type ind = pos - begin();
    size_type nsize = cap_;
    if (size_ + count >= cap_) {
      nsize = std::max(cap_*kCapMul + 1, size_ + count);
    }
    size_type copied = 0;
    pointer p = create_buffer(nsize, count, ind, value);
    try {
      safe_move(p, ptr_, ptr_ + ind);
      copied = ind;
      safe_move(p + ind + count, ptr_ + ind, ptr_ + size_);
    } catch (...) {
      destroy(p + ind, count);
      destroy(p, copied);
      dealloc(p, nsize);
      throw;
    }
    swap_out_buffer(p, nsize);    
    size_ += count;
    return begin() + ind;
  }

  // T is Swappable, MoveAssignable, MoveConstructible, EmplaceConstructible and MoveInsertable into *this
  template <typename InputIt>
  MEMORY_CPP20CONSTEXPR iterator insert(const_iterator pos, InputIt first, InputIt last) {
    size_type ind = pos - begin();
    size_type count = 0;
    if constexpr (std::is_base_of<std::forward_iterator_tag, typename std::iterator_traits<InputIt>::iterator_category>::value) {
      count = std::distance(first, last);
      if (size_ + count >= cap_ || !std::is_nothrow_swappable<T>::value) {
        size_type nsize = std::max(cap_*kCapMul + 1, size_ + count);
        size_type copied = 0;
        pointer p = create_buffer(nsize, ind, first, last);
        try {
          safe_move(p, ptr_, ptr_ + ind);
          copied = ind;
          safe_move(p + ind + count, ptr_ + ind, ptr_ + size_);
        } catch (...) {
          destroy(p + ind, count);
          destroy(p, copied);
          dealloc(p, nsize);
          throw;
        }
        swap_out_buffer(p, nsize);   
      } else if constexpr (std::is_nothrow_swappable<T>::value) {
        fill(ptr_ + size_, first, last);
        std::rotate(ptr_ + ind, ptr_ + size_, ptr_ + size_ + count);
      }
    } else {
      try {
        for (; first != last; ++first, ++count) {
          emplace_back(*first);
        }
        std::rotate(ptr_ + ind, ptr_ + size_, ptr_ + size_ + count);
      } catch (...) {
        destroy(ptr_ + size_, count);
        throw;
      }
    }
    size_ += count;
    return begin() + ind;
  }

  // Same as insert(pos, values.begin(), values.end())
  MEMORY_CPP20CONSTEXPR iterator insert(const_iterator pos,
                            std::initializer_list<T> values) {
    return insert(pos, values.begin(), values.end());
  }

  // T must meet additional requirements of MoveAssignable
  MEMORY_CPP20CONSTEXPR iterator erase(const_iterator pos) noexcept(
      std::is_nothrow_move_assignable<T>::value) {
    size_type ind = pos - begin();
    for (size_type i = ind; i < size_ - 1; ++i) {
      if constexpr (std::is_move_assignable<T>::value) {
        ptr_[i] = std::move(ptr_[i + 1]);
      } else {
        ptr_[i] = ptr_[i + 1];
      }
    }
    std::allocator_traits<Allocator>::destroy(al_, ptr_ + size_ - 1);
    --size_;
    return begin() + ind;
  }

  // T must meet additional requirements of MoveAssignable
  MEMORY_CPP20CONSTEXPR iterator erase(const_iterator first, const_iterator last) noexcept(std::is_nothrow_move_assignable<T>::value) {
    size_type start = first - begin();
    size_type finish = last - begin();
    size_type count = finish - start;
    for (size_type i = start; i < size_ - count; ++i) {
      if constexpr (std::is_move_assignable<T>::value) {
        ptr_[i] = std::move(ptr_[i + count]);
      } else {
        ptr_[i] = ptr_[i + count];
      }
    }
    destroy(ptr_ + size_ - count, count);
    size_ -= count;
    return begin() + start;
  }

  // T is EmplaceConstrutible from args, MoveInsertable into *this and MoveAssignable
  template <typename... Args>
  MEMORY_CPP20CONSTEXPR iterator emplace(const_iterator pos, Args&&... args) {
    size_type ind = pos - begin();
    if (size_ >= cap_ || !std::is_nothrow_move_constructible<T>::value || !std::is_nothrow_move_assignable<T>::value) {
      pointer p = create_buffer(cap_*kCapMul + 1, 1, ind, std::forward<Args>(args)...);
      size_type copied = 0;
      try {
        safe_move(p, ptr_, ptr_ + ind);
        copied = ind;
        safe_move(p + ind + 1, ptr_ + ind, ptr_ + size_);
      } catch (...) {
        destroy(p + ind, 1);
        destroy(p, copied);
        dealloc(p, cap_*kCapMul + 1);
        throw;
      }
      swap_out_buffer(p, cap_*kCapMul + 1);
   } else if constexpr (std::is_nothrow_move_constructible<T>::value && std::is_nothrow_move_assignable<T>::value){
      T val(std::forward<Args>(args)...);
      std::allocator_traits<Allocator>::construct(al_, ptr_ + size_, std::move(ptr_[size_ - 1]));
      for (size_type i = size_ - 1; i > ind; --i) {
        ptr_[i] = std::move(ptr_[i - 1]);
      }
      ptr_[ind] = std::move(val);
    }
    ++size_;
    return begin() + ind;
  }

  // T is EmplaceConstrutible from args and MoveInsertable into *this
  template <typename... Args>
  MEMORY_CPP20CONSTEXPR T& emplace_back(Args&&... args) {
    if (size_ >= cap_) {
      pointer p = create_buffer(cap_*kCapMul + 1, 1, size_, std::forward<Args>(args)...);
      try {
        safe_move(p, ptr_, ptr_ + size_);
      } catch (...) {
        destroy(p + size_, 1);
        dealloc(p, cap_*kCapMul + 1);
        throw;
      }
      swap_out_buffer(p, cap_*kCapMul + 1);
    } else {
      std::allocator_traits<Allocator>::construct(al_, ptr_ + size_, std::forward<Args>(args)...);
    }
    ++size_;
    return ptr_[size_ - 1];
  }

  // No additional requirements on types
  MEMORY_CPP20CONSTEXPR void swap(vector& other) 
      noexcept(
        std::allocator_traits<Allocator>::propagate_on_container_swap::value ||
        std::allocator_traits<Allocator>::is_always_equal::value
      ) {
    if (ptr_ == other.ptr_) {
      return;
    }
    if constexpr (std::allocator_traits<Allocator>::propagate_on_container_swap::value) {
       using std::swap;
       swap(al_, other.al_);
    }
    std::swap(ptr_, other.ptr_);
    std::swap(size_, other.size_);
    std::swap(cap_, other.cap_);
  }

  // T is EqualityComparable
  MEMORY_CPP20CONSTEXPR bool operator==(const vector& other) const {
    if (size_ != other.size_) return false;
    for (size_type i = 0; i < size_; ++i)
      if (ptr_[i] != other.ptr_[i]) return false;
    return true;
  }

  // T is EqualityComparable
  MEMORY_CPP20CONSTEXPR bool operator!=(const vector& other) const {
    return !(*this == other);
  }

  // No additional requirements on types
  MEMORY_CPP20CONSTEXPR reference operator[](size_type index) noexcept {
    return ptr_[index];
  }

  // No additional requirements on types
  MEMORY_CPP20CONSTEXPR const_reference operator[](size_type index) const noexcept {
    return ptr_[index];
  }

  // I guess os << T must be valid
  friend std::ostream& operator<<(std::ostream& os, const vector& vec) {
    for (size_type i = 0; i < vec.size_ - 1; ++i) os << vec.ptr_[i] << ' ';
    os << vec.ptr_[vec.size_ - 1];
    return os;
  }

 private:
  // No additional requirements on template types
  MEMORY_CPP20CONSTEXPR pointer alloc(size_type count) {
    return (count) ?
      std::allocator_traits<Allocator>::allocate(al_, count) :
      nullptr;
  }

  // No additional requirements on template types
  MEMORY_CPP20CONSTEXPR void dealloc(pointer p,  size_type count) {
    std::allocator_traits<Allocator>::deallocate(al_, p,  count);
  }

  // T is EmplaceConstructible from Args...
  template <typename... Args>
  MEMORY_CPP20CONSTEXPR void construct(pointer dst, size_type count, Args&&... args)
      noexcept(std::is_nothrow_constructible<T, Args...>::value) {
    size_type i = 0;
    try {
      for (; i < count; ++i) {
        std::allocator_traits<Allocator>::construct(
          al_,
          dst + i,
          std::forward<Args>(args)...
        );
      }
    } catch (...) {
      for (; i; --i) {
        std::allocator_traits<Allocator>::destroy(al_, dst + i - 1);
      }
      if constexpr (!std::is_nothrow_constructible<T, Args...>::value) throw;
    }
  }

  // T is EmplaceConstructible
  // FwdIt is ForwardIterator
  template <typename FwdIt>
  MEMORY_CPP20CONSTEXPR void fill(pointer dest, FwdIt first, FwdIt last)
      noexcept(std::is_nothrow_copy_constructible<T>::value) {
    size_type count = std::distance(first, last);
    size_type i = 0;
    try {
      for (; i < count; ++i, ++first) {
        std::allocator_traits<Allocator>::construct(al_, dest + i, *first);
      }
    } catch (...) {
      for (; i; --i) {
        std::allocator_traits<Allocator>::destroy(al_, dest + i - 1);
      }
      if constexpr (!std::is_nothrow_copy_constructible<T>::value) throw;
    }
  }

  // T is MoveInsertable
  // FwdIt is ForwardIterator
  template <typename FwdIt>
  MEMORY_CPP20CONSTEXPR void move(pointer dest, FwdIt first, FwdIt last) noexcept(std::is_nothrow_move_constructible<T>::value) {
    size_type count = std::distance(first, last);
    size_type i = 0;
    try {
      for (; i < count; ++i, ++first) {
        std::allocator_traits<Allocator>::construct(al_, dest + i, std::move(*first));
      }
    } catch (...) {
      for (; i; --i) {
        std::allocator_traits<Allocator>::destroy(al_, dest + i - 1);
      }
      if constexpr (!std::is_nothrow_move_constructible<T>::value)  throw;
    }    
  }

  // No additional requirements on template types
  MEMORY_CPP20CONSTEXPR void destroy(pointer p, size_type count)
      noexcept(std::is_nothrow_destructible<T>::value) {
    for (; count; --count) {
      std::allocator_traits<Allocator>::destroy(al_, p + count - 1);
    }
  }

  template <typename FwdIt>
  MEMORY_CPP20CONSTEXPR void copy_assign(size_type count, FwdIt first, FwdIt last) {
    pointer p = ptr_;
    if (cap_ < count) {
      p = create_buffer(count, 0, first, last);
      swap_out_buffer(p, count);
    } else {
      pointer end = ptr_ + size_;
      for (; p != end && first != last; ++first, ++p) {
        *p = *first;
      }
      if (p != end) {
        destroy(p, end - p);
      } else if (first != last) {
        fill(p, first, last);
      }
    }
    size_ = count;
  }

  template <typename FwdIt>
  MEMORY_CPP20CONSTEXPR void move_assign(size_type count, FwdIt first, FwdIt last) {
    pointer p = ptr_;
    if (cap_ < count) {
      p = create_buffer(count, 0, first, last);
      swap_out_buffer(p, count);
    } else {
      pointer end = ptr_ + size_;
      for (; p != end && first != last; ++first, ++p) {
        *p = std::move(*first);
      }
      if (p != end) {
        destroy(p, end - p);
      } else if (first != last) {
        move(p, first, last);
      }
    }
    size_ = count;
  } 

  // T is MoveInsertable
  MEMORY_CPP20CONSTEXPR pointer create_buffer(size_type size) {
    pointer p = alloc(size);
    try {
      safe_move(p, ptr_, ptr_ + size_);
    } catch(...) {
      dealloc(p, size);
      throw;
    }
    return p;
  }

  // T is MoveInsertable
  template <typename FwdIt>
  MEMORY_CPP20CONSTEXPR typename std::enable_if<std::is_base_of<std::forward_iterator_tag, typename std::iterator_traits<FwdIt>::iterator_category>::value, pointer>::type create_buffer(size_type size, size_type ind, FwdIt first, FwdIt last) {
    pointer p = alloc(size);
    try {
      fill(p + ind, first, last);
    } catch(...) {
      dealloc(p, size);
      throw;
    }
    return p;
  }

  // T is Emplaceonstructible
  template <typename... Args>
  MEMORY_CPP20CONSTEXPR pointer create_buffer(size_type size, size_type count, size_type ind, Args&&... args) {
    pointer p = alloc(size);
    try {
      construct(p + ind, count, std::forward<Args>(args)...);
    } catch(...) {
      dealloc(p, count);
      throw;
    }
    return p;
  }

  // T is MoveInsertable into *this
  MEMORY_CPP20CONSTEXPR void safe_move(pointer dest, pointer first, pointer last) noexcept(std::is_nothrow_move_constructible<T>::value) {
    if constexpr (std::is_nothrow_move_constructible<T>::value) {
      move(dest, first, last);
    } else if constexpr (std::is_copy_constructible<T>::value) {
      fill(dest, first, last);
    } else {
      move(dest, first, last);
    }
  }

  // No additional requirements on template types
  MEMORY_CPP20CONSTEXPR void swap_out_buffer(pointer new_buf, size_type size) {
    if (new_buf == ptr_) {
      return;
    }
    std::swap(new_buf, ptr_);
    std::swap(size, cap_);
    destroy(new_buf, size_);
    dealloc(new_buf, size);
  }

  // T is MoveInsertable and MoveAssingable
  MEMORY_CPP20CONSTEXPR void shift_right(size_type index, size_type offset) noexcept {
    size_type i = size_;
    for (; i > size_ - offset; --i) {
      std::allocator_traits<Allocator>::construct(al_, ptr_ + size_ + offset - 1, std::move(ptr_[size_ - offset]));
    }
    for (; i > index; --i) {
      ptr_[i] = std::move(ptr_[i - offset]);
    }  
  }

  size_type size_;
  size_type cap_;
  allocator_type al_;
  pointer ptr_;
};
}  // namespace memory

#undef MEMORY_CPP20_CONSTEXPR

#endif  // MEMORY_CONTAINERS_VECTOR_H_

