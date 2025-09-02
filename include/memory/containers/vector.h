#ifndef MEMORY_CONTAINERS_VECTOR_H_
#define MEMORY_CONTAINERS_VECTOR_H_

#include "../iterators/pointer_iterator.h"  // iterator and std::distance
#include "../iterators/reverse_iterator.h"

#include <cstdint>      // types
#include <ostream>      // operator<<
#include <stdexcept>    // exceptions
#include <type_traits>  // as name suggests
#include <utility>      // std::forward, std::swap, std::move

namespace memory {
// use it at your own risk
//
// T type must meet requirements of Erasable
// Allocator type must meet requirements of Allocator
// Methods may have additional reuirements on types
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

  // T has no additional requirements
  // Allocator type must meet additional requirements of DefaultConstructible
  constexpr vector() noexcept(
      std::is_nothrow_default_constructible<Allocator>::value)
      : size_(0), cap_(0), ptr_(nullptr) {}

  // No additional requirements on template types
  constexpr explicit vector(const Allocator& al) noexcept
      : size_(0), cap_(0), al_(al), ptr_(nullptr) {}

  // T must meet additional requirements of DefaultInsertable into *this
  constexpr explicit vector(size_type size, const Allocator& al = Allocator())
      : size_(size), cap_(size), al_(al), ptr_(alloc(size_)) {
    try {
      construct(ptr_, size_);
    } catch(...) {
      dealloc(ptr_, size_);
      throw;
    }
  }

  // T must meet additional requirements of CopyInsertable into *this
  constexpr explicit vector(size_type size, const_reference value,
                            const Allocator& al = Allocator())
      : size_(size), cap_(size), al_(al), ptr_(alloc(size_)) {
    try {
      construct(ptr_, size_, value);
    } catch(...) {
      dealloc(ptr_, size_);
      throw;
    }
  };

  // T must meet additional requirements of
  // EmplaceConstructible and
  // MoveInsertable (if InputIterator does not meet
  // the ForwardIterator requirements) into *this
  template <typename InputIterator>
  constexpr vector(InputIterator first, InputIterator last,
                   const Allocator& al = Allocator())
      : size_(std::distance(first, last)),
        cap_(size_),
        al_(al),
        ptr_(alloc(size_)) {
    try {
      fill(ptr_, first, last);
    } catch(...) {
      dealloc(ptr_, size_);
      throw;
    }
  }

  // T must meet additional requirements of EmplaceConstructible
  constexpr vector(std::initializer_list<value_type> values,
                   const Allocator& al = Allocator())
      : vector(values.begin(), values.end(), al) {}
  
  constexpr vector(const vector& other)
      : vector(other.ptr_, other.ptr_ + other.size_, std::allocator_traits<Allocator>::select_on_container_copy_construction(other.al_)) {}

  // T must meet additional requirements of CopyInsertable into *this
  constexpr vector(const vector& other, const Allocator& al)
      : vector(other.ptr_, other.ptr_ + other.size_, al) {};

  // No additional requirements on template types
  constexpr vector(vector&& other) noexcept
      : size_(other.size_),
        cap_(other.cap_),
        al_(std::move(other.al_)),
        ptr_(other.ptr_) {
    other.ptr_ = nullptr;
    other.cap_ = 0;
    other.size_ = 0;
  }

  // T must meet additional requirements of MoveInsertable into *this
  constexpr vector(vector&& other, const Allocator& al)
      noexcept(std::allocator_traits<Allocator>::is_always_equal::value)
      : vector(al) {
    if (al == other.al_) {
      swap(other);
    } else {
      ptr_ = alloc(other.size_);
      move(ptr_, other.ptr_, other.ptr_ + other.size_);
      cap_ = size_ = other.size_;
    }
  }

  // T must meet additional requirements of
  //  CopyInsertable and CopyAssignable into *this
  constexpr vector& operator=(const vector& other) {
    if (ptr_ == other.ptr_) {
      return *this;
    }
    if constexpr (std::allocator_traits<Allocator>::propagate_on_container_copy_assignment::value) {
      pointer ptr = ptr_;
      if (al_ != other.al_) {
        ptr = other.alloc(other.size_);
        other.fill(ptr, other.ptr_, other.ptr_ + other.size_);
        swap_out_buffer(ptr, other.size);
        al_ = other.al_;
      }
    } else {
      copy_assign(other.size_, other.ptr_, other.ptr_ + other.size_);      
    }
    return *this;
  }

  // T must meet additional requirements of
  //  MoveInsertable into *this and MoveAssignable if
  //  allocator_traits<Allocator>
  //  ::propagate_on_container_move_assignment::value is false
  constexpr vector& operator=(vector&& other) noexcept(
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
  
  constexpr vector& operator=(std::initializer_list<value_type> ilist) {
    assign(ilist.begin(), ilist.end());
    return *this;   
  }
  
  // No additional requirements on template types
  constexpr virtual ~vector() noexcept { 
    clear(); 
    dealloc(ptr_, cap_);
  }

  //============================================================================
  // No additional requirements on template types for all methods below

  constexpr allocator_type get_allocator() const noexcept { return al_; }

  constexpr reference at(size_type pos) {
    return (pos < size_)
               ? ptr_[pos]
               : throw std::out_of_range("Accessing element out of bounds");
  }

  constexpr const_reference at(size_type pos) const {
    return (pos < size_)
               ? ptr_[pos]
               : throw std::out_of_range("Accessing element out of bounds");
  }

  constexpr reference front() noexcept { return ptr_[0]; }
  constexpr reference back() noexcept { return ptr_[size_ - 1]; }
  constexpr pointer data() noexcept { return ptr_; }

  constexpr const_reference front() const noexcept { return ptr_[0]; }
  constexpr const_reference back() const noexcept {
    return ptr_[size_ - 1];
  }
  constexpr const_pointer data() const noexcept { return ptr_; }

  constexpr iterator begin() noexcept { return iterator(ptr_); }
  constexpr const_iterator begin() const noexcept {
    return const_iterator(ptr_);
  }
  constexpr const_iterator cbegin() const noexcept {
    return const_iterator(ptr_);
  }

  constexpr iterator end() noexcept { return iterator(ptr_ + size_); }
  constexpr const_iterator end() const noexcept {
    return const_iterator(ptr_ + size_);
  }
  constexpr const_iterator cend() const noexcept {
    return const_iterator(ptr_ + size_);
  }

  constexpr reverse_iterator rbegin() noexcept {
    return reverse_iterator(end());
  }
  constexpr const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator(end());
  }
  constexpr const_reverse_iterator crbegin() const noexcept {
    return const_reverse_iterator(cend());
  }

  constexpr reverse_iterator rend() noexcept {
    return reverse_iterator(begin());
  }
  constexpr const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator(begin());
  }
  constexpr const_reverse_iterator crend() const noexcept {
    return const_reverse_iterator(begin());
  }

  constexpr bool empty() const noexcept { return !size_; }
  constexpr size_type size() const noexcept { return size_; }
  constexpr size_type capacity() const noexcept { return cap_; }
  constexpr size_type max_size() const noexcept {
    return std::allocator_traits<Allocator>::max_size(al_);
  }
  //============================================================================

  // T must meet additional requirement of CopyAssignable and
  //  CopyInsertable into *this
  constexpr void assign(size_type count, const_reference value) {
    if (count > max_size()) {
      throw std::length_error("Invalid count provided");
    }
    pointer p = ptr_;
    if (cap_ < count) {
      p = copy_buffer(count, value);
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
  //  and MoveInsertable if InputIterator does not satisfy ForwardIterator
  template <typename InputIterator>
  constexpr void assign(InputIterator first, InputIterator last) {
    size_type count = std::distance(first, last);
    if (count > max_size()) {
      throw std::length_error("Too big range provided");
    }
    copy_assign(count, first, last);
  }

  // Same as assign(values.begin(), values.end())
  constexpr void assign(std::initializer_list<value_type> values) {
    return assign(values.begin(), values.end());
  }

  // T must meet additional requirements of MoveInsertable into *this
  constexpr void reserve(size_type count) {
    // if (count > max_size() || count < 0) {
    //   throw std::length_error("Invalid reserve MEMORYace");
    // } else if (count > buf_.cap) {
    //   resize_buffer(count);
    // }
  }

  // T must meet additional requirements of MoveInsertable into *this
  constexpr void shrink_to_fit() {
    // if (buf_.cap > size_) {
    //   resize_buffer(size_);
    // }
  }

  // No additional requirements on template types
  constexpr void clear() noexcept {
    destroy(ptr_, size_);
  }

  // T must meet additional requirements of
  //  MoveInsertable and DefaultInsertable into *this
  constexpr void resize(size_type count) {
    // if (count > max_size() || count < 0) {
    //   throw std::length_error("Invalid count provided");
    // }
    // if (count == size_) {
    //   return;
    // } else if (count >= buf_.cap) {
    //   pointer_buffer temp(count, &al_, ptr_);
    //   move_from(temp.ptr, ptr_, size_);
    //   try {
    //     fill(temp.ptr + size_, count - size_);
    //   } catch (...) {
    //     destroy_content(temp.ptr, size_);
    //     throw;
    //   }
    //   buf_.swap(temp);
    //   destroy_content(temp.ptr, size_);
    //   size_ = count;
    // } else {
    //   move_end(count - size_);
    // }
  }

  // T must meet additional requirements of CopyInsertable into *this
  constexpr void resize(size_type count, const_reference value) {
    // if (count > max_size() || count < 0) {
    //   throw std::length_error("Invalid count provided");
    // }
    // if (count == size_) {
    //   return;
    // } else if (count >= buf_.cap) {
    //   pointer_buffer temp(count, &al_, ptr_);
    //   move_from(temp.ptr, ptr_, size_);
    //   try {
    //     fill(temp.ptr + size_, count - size_, value);
    //   } catch (...) {
    //     destroy_content(temp.ptr, size_);
    //     throw;
    //   }
    //   buf_.swap(temp);
    //   destroy_content(temp.ptr, size_);
    //   size_ = count;
    // } else {
    //   move_end(count - size_, value);
    // }
  }
  //==============================================================================

  // T must meet additional requirements of CopyInsertable
  constexpr void push_back(const_reference value) { emplace_back(value); }

  // T must meet additional requirements of MoveInsertable
  constexpr void push_back(value_type&& value) {
    emplace_back(std::forward<value_type>(value));
  }

  // No additional requirements on template types
  constexpr void pop_back() noexcept(std::is_nothrow_destructible<T>::value) {
    std::allocator_traits<Allocator>::destroy(al_, ptr_ + size_ - 1);
    --size_;
  }

  // T must meet additional requirements of CopyAssignable
  //   and CopyInsertable into *this
  constexpr iterator insert(const_iterator pos, const_reference value) {
    return insert(pos, 1, value);
  }

  // T must meet additional requirements of MoveAssignable
  //   and MoveInsertable into *this
  constexpr iterator insert(const_iterator pos, value_type&& value) {
    return emplace(pos, std::forward<value_type>(value));
  }

  // T must meet additional requirements of CopyAssignable
  //   and CopyInsertable into *this
  constexpr iterator insert(const_iterator pos, size_type count,
                            const_reference value) {
    // size_type ind = pos - begin();
    // if (size_ + count >= buf_.cap || !std::is_nothrow_swappable<T>::value) {
    //   pointer_buffer temp(std::max(kCapMul * buf_.cap, size_ + count), &al_,
    //                       ptr_);
    //   fill(temp.ptr, count, value);
    //   try {
    //     split_buffer(temp.ptr, ind, count);
    //   } catch (...) {
    //     destroy_content(temp.ptr, count);
    //     throw;
    //   }
    //   buf_.swap(temp);
    //   destroy_content(temp.ptr, size_);
    // } else if constexpr (std::is_nothrow_swappable<T>::value) {
    //   fill(ptr_ + size_, count, value);
    //   std::reverse(ptr_, ptr_ + size_ + count);
    //   std::reverse(ptr_, ptr_ + count);
    //   std::reverse(ptr_ + count, ptr_ + size_ + count);
    // }
    // size_ += count;
    return begin();
  }

  // T must meet additional requirements of Swappable, MoveAssignable,
  //   MoveConstructible, EmplaceConstructible and MoveInsertable into *this
  template <typename InputIt>
  constexpr iterator insert(const_iterator pos, InputIt first, InputIt last) {
    // size_type ind = pos - begin();
    // size_type count = std::distance(first, last);
    // if (count + size_ >= buf_.cap || !std::is_nothrow_swappable<T>::value) {
    //   pointer_buffer temp(std::max(kCapMul * buf_.cap, size_ + count), &al_,
    //                       ptr_);
    //   assign(temp.ptr, first, last);
    //   try {
    //     split_buffer(temp.ptr, ind, count);
    //   } catch (...) {
    //     destroy_content(temp.ptr, count);
    //     throw;
    //   }
    //   buf_.swap(temp);
    //   destroy_content(temp.ptr, size_);
    // } else if constexpr (std::is_nothrow_swappable<T>::value) {
    //   assign(ptr_ + size_, first, last);
    //   std::reverse(ptr_, ptr_ + size_ + count);
    //   std::reverse(ptr_, ptr_ + count);
    //   std::reverse(ptr_ + count, ptr_ + size_ + count);
    // }
    // size_ += count;
    return begin();
  }

  // Same as insert(pos, values.begin(), values.end())
  constexpr iterator insert(const_iterator pos,
                            std::initializer_list<T> values) {
    return insert(pos, values.begin(), values.end());
  }

  // T must meet additional requirements of MoveAssignable
  constexpr iterator erase(const_iterator pos) noexcept(
      std::is_nothrow_move_assignable<T>::value) {
    // size_type ind = pos - begin();
    // if constexpr (std::is_nothrow_move_assignable<T>::value) {
    //   for (size_type i = ind; i < size_ - 1; ++i) {
    //     ptr_[i] = std::move(ptr_[i + 1]);
    //   }
    //   al_traits::destroy(al_, ptr_ + size_ - 1);
    // } else {
    //   pointer_buffer temp(buf_.cap, &al_, ptr_);
    //   split_buffer(temp.ptr, ind + 1, -1);
    //   buf_.swap(temp);
    //   destroy_content(temp.ptr, size_);
    // }
    // --size_;
    return begin();
  }

  // T must meet additional requirements of MoveAssignable
  constexpr iterator erase(const_iterator first, const_iterator last) noexcept(
      std::is_nothrow_move_assignable<T>::value) {
    // size_type start = first - begin();
    // size_type finish = last - begin();
    // size_type count = finish - start;
    // if constexpr (std::is_nothrow_move_assignable<T>::value) {
    //   for (size_type i = start; i < size_ - count; ++i) {
    //     ptr_[i] = std::move(ptr_[i + count]);
    //   }
    //   destroy_content(ptr_ + size_ - count, count);
    // } else {
    //   pointer_buffer temp(buf_.cap, &al_, ptr_);
    //   split_buffer(temp.ptr, finish, -count);
    //   buf_.swap(temp);
    //   destroy_content(temp.ptr, size_);
    // }
    // size_ -= count;
    return begin();
  }

  // T must meet additional requirements of EmplaceConstrutible from args,
  //  MoveAssignable and MoveInsertable into *this
  template <typename... Args>
  constexpr iterator emplace(const_iterator pos, Args&&... args) {
    // size_type ind = pos - begin();
    // if (size_ == buf_.cap || !std::is_nothrow_move_assignable<T>::value) {
    //   pointer_buffer temp(std::max(kCapMul * buf_.cap, size_type(1)), &al_,
    //                       ptr_);
    //   al_traits::construct(al_, temp.ptr, std::forward<Args>(args)...);
    //   try {
    //     split_buffer(temp.ptr, ind, 1);
    //   } catch (...) {
    //     al_traits::destroy(al_, temp.ptr);
    //     throw;
    //   }
    //   buf_.swap(temp);
    //   destroy_content(temp.ptr, size_);
    // } else if constexpr (std::is_nothrow_move_assignable<T>::value) {
    //   T emp(std::forward<Args>(args)...);
    //   al_traits::construct(al_, ptr_ + size_,
    //                        std::move(ptr_[size_ - 1]));
    //   for (size_type i = size_ - 1; i > ind; --i) {
    //     ptr_[i] = std::move(ptr_[i - 1]);
    //   }
    //   ptr_[ind] = std::move(emp);
    // }
    // ++size_;
    return begin();
  }

  // T must meet additional requirements of EmplaceConstrutible from args
  //   and MoveInsertable into *this
  template <typename... Args>
  constexpr T& emplace_back(Args&&... args) {
    // if (size_ >= buf_.cap) {
    //   pointer_buffer temp(std::max(kCapMul * buf_.cap, size_type(1)), &al_,
    //                       ptr_);
    //   al_traits::construct(al_, temp.ptr + size_, std::forward<Args>(args)...);
    //   try {
    //     move_from(temp.ptr, ptr_, size_);
    //   } catch (...) {
    //     al_traits::destroy(al_, temp.ptr + size_);
    //     throw;
    //   }
    //   buf_.swap(temp);
    //   destroy_content(temp.ptr, size_);
    // } else {
    //   al_traits::construct(al_, ptr_ + size_, std::forward<Args>(args)...);
    // }
    // ++size_;
    return ptr_[size_ - 1];
  }

  // No additional requirements on types
  constexpr void swap(vector& other) 
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

  // T must meet additional requirements of EqualityComparable
  constexpr bool operator==(const vector& other) const noexcept {
    if (size_ != other.size_) return false;
    for (size_type i = 0; i < size_; ++i)
      if (ptr_[i] != other.ptr_[i]) return false;
    return true;
  }

  // T must meet additional requirements of EqualityComparable
  constexpr bool operator!=(const vector& other) const noexcept {
    return !(*this == other);
  }

  // No additional requirements on types
  constexpr reference operator[](size_type index) noexcept {
    return ptr_[index];
  }

  // No additional requirements on types
  constexpr const_reference operator[](size_type index) const noexcept {
    return ptr_[index];
  }

  // I guess os << T must be valid
  friend std::ostream& operator<<(std::ostream& os, const vector& vec) {
    for (size_type i = 0; i < vec.size_ - 1; ++i) os << vec.ptr_[i] << ' ';
    os << vec.ptr_[vec.size_ - 1];
    return os;
  }

 private:
  constexpr pointer alloc(size_type count) {
    return (count) ?
      std::allocator_traits<Allocator>::allocate(al_, count) :
      nullptr;
  }

  constexpr void dealloc(pointer ptr, size_type count) {
    std::allocator_traits<Allocator>::deallocate(al_, ptr, count);
  }

  template <typename... Args>
  constexpr void construct(pointer dst, size_type count, Args&&... args)
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

  template <typename InIt>
  constexpr void fill(pointer dest, InIt first, InIt last)
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

  template <typename InIt>
  constexpr void move(pointer dest, InIt first, InIt last) noexcept(std::is_nothrow_move_constructible<T>::value) {
    size_type count = std::distance(first, last);
    size_type i =0;
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

  constexpr void destroy(pointer ptr, size_type count)
      noexcept(std::is_nothrow_destructible<T>::value) {
    for (; count; --count) {
      std::allocator_traits<Allocator>::destroy(al_, ptr + count - 1);
    }
  }

  template <typename InIt>
  constexpr void copy_assign(size_type count, InIt first, InIt last) {
    pointer p = ptr_;
    if (cap_ < count) {
      p = copy_buffer(count, first, last);
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

  template <typename InIt>
  constexpr void move_assign(size_type count, InIt first, InIt last) {
    pointer p = ptr_;
    if (cap_ < count) {
      p = move_buffer(count, first, last);
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

  template <typename InIt>
  constexpr pointer copy_buffer(size_type size, InIt first, InIt last) {
    pointer p = alloc(size);
    try {
      fill(p, first, last);
    } catch(...) {
      dealloc(p, size);
      throw;
    }
    return p;
  } 

  template <typename InIt>
  constexpr pointer move_buffer(size_type size, InIt first, InIt last) {
    pointer p = alloc(size);
    try {
      move(p, first, last);
    } catch(...) {
      dealloc(p, size);
      throw;
    }
    return p;
  }

  constexpr pointer copy_buffer(size_type count, value_type value) {
    pointer p = alloc(count);
    try {
      construct(p, count, value);
    } catch(...) {
      dealloc(p, count);
      throw;
    }
    return p;
  }

  constexpr void swap_out_buffer(pointer new_buf, size_type size) {
    if (new_buf == ptr_) {
      return;
    }
    std::swap(new_buf, ptr_);
    std::swap(size, cap_);
    destroy(new_buf, size);
    dealloc(new_buf, size);
  }

  template <typename... Args>
  constexpr void move_end(size_type offset, Args&&... append_args) {
    // if (offset < 0) {
    //   destroy_content(ptr_ + size_ + offset, -offset);
    // } else {
    //   fill(ptr_ + size_, offset, std::forward<Args>(append_args)...);
    // }
    // size_ += offset;
  }

  constexpr void split_buffer(
      pointer dest, size_type ind,
      size_type offset) noexcept(std::is_nothrow_move_constructible<T>::value) {
    // size_type lim = std::min(ind, ind + offset);
    // move_from(dest, ptr_, lim);
    // try {
    //   move_from(dest + offset, ptr_, size_ - ind);
    // } catch (...) {
    //   destroy_content(dest, lim);
    //   if constexpr (!std::is_nothrow_move_constructible<T>::value) throw;
    // }
  }

  size_type size_;
  size_type cap_;
  allocator_type al_;
  pointer ptr_;
};
}  // namespace memory
#endif  // MEMORY_CONTAINERS_VECTOR_H_

