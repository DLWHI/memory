#ifndef MEMORY_ITERATORS_REVERSE_ITERATOR_H_
#define MEMORY_ITERATORS_REVERSE_ITERATOR_H_
#include <cstdint>  // int64_t

namespace memory {
// BidIt satisfies BidirectionalIterator
template <typename BidIt>
class reverse_iterator final {
 public:
  using iterator_category = typename BidIt::iterator_category;
  using value_type = typename BidIt::value_type;
  using pointer = typename BidIt::pointer;
  using reference = typename BidIt::reference;
  using difference_type = typename BidIt::difference_type;

  constexpr explicit reverse_iterator(const BidIt& it) : fwd_it_(it){};

  constexpr pointer* base() const noexcept { return fwd_it_; }

  constexpr value_type& operator*() const { return *(fwd_it_ - 1); }
  constexpr pointer* operator->() const { return (fwd_it_ - 1)->operator->(); }

  constexpr bool operator==(const reverse_iterator& other) const {
    return fwd_it_ == other.fwd_it_;
  }

  constexpr bool operator!=(const reverse_iterator& other) const {
    return fwd_it_ != other.fwd_it_;
  }

  constexpr bool operator>(const reverse_iterator& other) const {
    return (fwd_it_ - other.ptr_) < 0;
  }

  constexpr bool operator<(const reverse_iterator& other) const {
    return (fwd_it_ - other.ptr_) > 0;
  }

  constexpr bool operator>=(const reverse_iterator& other) const {
    return (fwd_it_ - other.ptr_) <= 0;
  }

  constexpr bool operator<=(const reverse_iterator& other) const {
    return (fwd_it_ - other.ptr_) >= 0;
  }

  constexpr reverse_iterator operator+(difference_type delta) const {
    return reverse_iterator(fwd_it_ - delta);
  }

  constexpr reverse_iterator operator-(difference_type delta) const {
    return reverse_iterator(fwd_it_ + delta);
  }

  constexpr difference_type operator-(const reverse_iterator& other) const {
    return other.fwd_it_ - fwd_it_;
  }

  constexpr reverse_iterator& operator+=(difference_type delta) {
    fwd_it_ += delta;
    return *this;
  }

  constexpr reverse_iterator& operator-=(difference_type delta) {
    fwd_it_ -= delta;
    return *this;
  }

  constexpr reverse_iterator operator++(int) {
    return reverse_iterator(fwd_it_--);
  }
  constexpr reverse_iterator operator--(int) {
    return reverse_iterator(fwd_it_++);
  }

  constexpr reverse_iterator& operator++() {
    --fwd_it_;
    return *this;
  }

  constexpr reverse_iterator& operator--() {
    ++fwd_it_;
    return *this;
  }

  constexpr operator reverse_iterator<const BidIt>() const {
    return reverse_iterator<const BidIt>(const_cast<const BidIt>(fwd_it_));
  }

 protected:
  BidIt fwd_it_;
};
}  // namespace memory
#endif  // MEMORY_ITERATORS_REVERSE_ITERATOR_H_

