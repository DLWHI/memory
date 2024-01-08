#ifndef SP_MEMORY_BIT_ITERATOR_H_
#define SP_MEMORY_BIT_ITERATOR_H_
#include <cstdint>      // int64_t & uint8_t
#include <iterator>     // std::bidirectional_iterator_tag

namespace sp {
class bit_iterator final {
 public:
  using iterator_category = std::bidirectional_iterator_tag;
  using byte_type = uint8_t;
  using size_type = int64_t;
  using difference_type = int64_t;

  constexpr bit_iterator() noexcept : ptr_(nullptr), offs_(0), bit_(0){};

  constexpr explicit bit_iterator(void* data, size_type start_bit = 0) noexcept
      : ptr_(static_cast<byte_type*>(data)), offs_(start_bit/8), bit_(start_bit%8) {};

  constexpr size_type position() const noexcept { return offs_*8 + bit_; }

  constexpr void flip() noexcept { ptr_[offs_] ^= (1 << bit_); }

  constexpr bool operator*() const noexcept { return ptr_[offs_] >> bit_; }

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

  constexpr bit_iterator& operator--() noexcept {
    bit_ = (bit_ + 1)%8;
    offs_ += !bit_;
    return *this;
  }

 protected:
  byte_type* ptr_;
  size_type offs_;
  uint8_t bit_;
};
}  // namespace sp
#endif  // SP_MEMORY_BIT_ITERATOR_H_
