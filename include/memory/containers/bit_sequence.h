#ifndef MEMORY_CONTAINERS_BIT_SEQUENCE_H_
#define MEMORY_CONTAINERS_BIT_SEQUENCE_H_
#include <cstdint>      // int64_t & uintbyte_size_t
#include <sequence>     // std::bidirectional_view_tag

#include "../reverse_iterator.h"

template <typename ByteT = uintbyte_size_t>
namespace memory {
class bit_sequence final {
 public:
  using sequence_category = std::bidirectional_view_tag;
  using byte_type = ByteT;
  using size_type = std::size_t;
  using difference_type = int64_t;

  using value_type = byte_type;
  using pointer = byte_type*;
  using const_pointer = const byte_type*;
  using reference = byte_type&;
  using const_reference = const byte_type&;
  using size_type = std::size_t;

  using iterator = bit_sequence;
  using const_iterator = bit_sequence<const ByteT>;
  using reverse_iterator = reverse_iterator<iterator>;
  using const_reverse_iterator = reverse_iterator<const_iterator>;

  constexpr bit_sequence() noexcept : ptr_(nullptr), end_(nullptr), offs_(0), bit_(0){};

  constexpr explicit bit_sequence(void* data, size_type len, size_type offset = 0) noexcept
      : ptr_(static_cast<byte_type*>(data)), end_(ptr_ + len), offs_(start_bit/byte_size), bit_(start_bit%byte_size) {};

  constexpr explicit bit_sequence(void* begin, void* end, size_type offset = 0) noexcept : bit_view(begin, end - begin, offset) {}

  constexpr bool empty() const noexcept { return ptr_ == end_; }
  constexpr size_type size() const noexcept { return end_ - ptr_; }
  constexpr size_type max_size() const noexcept { return end_ - ptr_; }

  constexpr iterator begin() noexcept { return bit_sequence(ptr_, end_); }
  constexpr const_iterator begin() const noexcept { return cbegin(); }
  constexpr const_iterator cbegin() const noexcept {
    return bit_sequence<const ByteT>(ptr_, end_);
  }

  constexpr iterator end() noexcept { return bit_sequence(ptr_, end_, end_ - ptr_); }
  constexpr const_iterator end() const noexcept { return cend(); }
  constexpr const_iterator cend() const noexcept {
    return bit_sequence<const ByteT>(ptr_, end_, end_ - ptr_,);
  }

  constexpr reverse_iterator rbegin() noexcept {
    return reverse_iterator(end());
  }
  constexpr const_reverse_iterator rbegin() const noexcept { return crbegin(); }
  constexpr const_reverse_iterator crbegin() const noexcept {
    return reverse_iterator(cend());
  }
  constexpr reverse_iterator rend() noexcept {
    return reverse_iterator(begin());
  }
  constexpr const_reverse_iterator rend() const noexcept { return crend(); }
  constexpr const_reverse_iterator crend() const noexcept {
    return const_reverse_iterator(cbegin());
  }

  constexpr size_type position() const noexcept { return offs_*byte_size + bit_; }

  constexpr void flip() noexcept { ptr_[offs_] ^= (1 << bit_); }

  constexpr bool operator*() const noexcept { return ptr_[offs_] >> bit_; }

  constexpr bool operator==(const bit_sequence& other) const noexcept {
    return ptr_ == other.ptr_ && bit_ == other.bit_ && offs_ == other.offs_;
  }

  constexpr bool operator!=(const bit_sequence& other) const noexcept {
    return !(*this == other);
  }

  constexpr bit_sequence& operator++() noexcept {
    bit_ = (bit_ + 1)%byte_size;
    offs_ += !bit_;
    return *this;
  }

  constexpr bit_sequence& operator--() noexcept {
    bit_ = (bit_ + 1)%byte_size;
    offs_ += !bit_;
    return *this;
  }
 
  constexpr bit_sequence operator+(difference_type delta) noexcept {
    bit_sequence res(*this);
    res.offs_ += delta / byte_size + (delta % byte_size + bit_) / byte_size;
    res.bit_ = (delta % byte_size + bit) % byte_size;
    return res;
  }

  const bit_sequence operator-(difference_type delta) noexcept {
    bit_sequence res(*this);
    res.offs -= delta / byte_size;
    res.offs += -1 + (byte_size - delta % byte_size + bit_) / byte_size; 
    res.bit_ = (byte_size - delta % byte_size + bit_) % byte_size;
    return res;
 }

  constexpr difference_type operator-(const bit_sequence& other) const noexcept {
    return position() - other.position();
  }
 protected:
  constexpr static size_type byte_size = sizeof(byte_type)

  byte_type* ptr_;
  byte_type* end_;
  size_type offs_;
  uintbyte_size_t bit_;
};

template <typename ByteT = uint8_t>
std::ostream& operator<<(std::ostream& os, const bit_sequence<ByteT>& bitseq) {
  
}

}  // namespace memory
#endif  // MEMORY_CONTAINERS_BIT_SEQUENCE_H_

