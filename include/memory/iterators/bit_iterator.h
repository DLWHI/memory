#ifndef MEMORY_ITERATORS_BIT_ITERATOR_H_
#define MEMORY_ITERATORS_BIT_ITERATOR_H_
#include <cstdint>      // int64_t & uint8_t
#include <iterator>     // std::bidirectional_iterator_tag
#include <iostream>
namespace memory {
class bit_iterator final {
 public:
  using iterator_category = std::bidirectional_iterator_tag;
  using byte_type = uint8_t;
  using value_type = bool;
  using reference = bool&;
  using size_type = std::size_t;
  using difference_type = int64_t;

  constexpr bit_iterator() noexcept : ptr_(nullptr), offs_(0), bit_(0){};

  constexpr explicit bit_iterator(void* data, size_type start_bit = 0) noexcept
      : ptr_(static_cast<byte_type*>(data)), offs_(start_bit/8), bit_(start_bit%8) {};

  constexpr size_type position() const noexcept { return offs_*8 + bit_; }

  constexpr void flip() noexcept { ptr_[offs_] ^= (1 << bit_); }

  constexpr bool value() const noexcept { return (ptr_[offs_] >> bit_) & 1;}

  constexpr bool operator*() const noexcept { return value(); }

  constexpr bool operator==(const bit_iterator& other) const noexcept {
    return ptr_ == other.ptr_ && bit_ == other.bit_ && offs_ == other.offs_;
  }

  constexpr bool operator!=(const bit_iterator& other) const noexcept {
    return !(*this == other);
  }

  constexpr bit_iterator& operator++() noexcept {
    bit_ = (bit_ + 1)%8;
    offs_ += !bit_;
    return *this;
  }

  constexpr bit_iterator& operator++(int) noexcept {
    bit_iterator res(*this);
    return ++res;
  }

  constexpr bit_iterator& operator--() noexcept {
    bit_ = (bit_ + 1)%8;
    offs_ += !bit_;
    return *this;
  }
  
  constexpr bit_iterator& operator--(int) noexcept {
    bit_iterator res(*this);
    return --res;
  }

  constexpr bit_iterator& operator+=(difference_type delta) noexcept {
    offs_ += delta / 8;
    bit_ = (delta % 8 + bit_);
    offs_ += ((delta > 0) - (delta < 0))*(bit_ > 8);
    bit_ %= 8;
    return *this;
  }

  constexpr bit_iterator& operator-=(difference_type delta) noexcept {
    return *this += -delta;
  }

  constexpr bit_iterator operator+(difference_type delta) const noexcept {
    bit_iterator res(*this);
    res += delta;
    return res;
  }

  const bit_iterator operator-(difference_type delta) const noexcept {
    bit_iterator res(*this);
    res -= delta;
    return res;
  }

  constexpr difference_type operator-(const bit_iterator& other) const noexcept {
    return position() - other.position();
  }
 protected:
  byte_type* ptr_;
  size_type offs_;
  uint8_t bit_;
};
}  // namespace memory
#endif  // MEMORY_ITERATORS_BIT_ITERATOR_H_
