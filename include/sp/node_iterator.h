#ifndef SP_MEMORY_NODE_ITERATOR_H_
#define SP_MEMORY_NODE_ITERATOR_H_
#include <iterator>
#include <cstdint>

namespace sp {

// Container - is not used inside of class, but allows different containers
//             with same template type produce different iterators
template <typename T, typename Node, typename Container>
class node_iterator {
 public:
  using iterator_category = std::bidirectional_iterator_tag;
  using node_type = T;
  using value_type = typename Container::value_type;
  using pointer = typename Container::pointer;
  using reference = typename Container::reference;
  using difference_type = int64_t;

  constexpr node_iterator() noexcept : node_(nullptr){};
  constexpr explicit node_iterator(T* data) noexcept : node_(data){};
  constexpr virtual ~node_iterator() = default;

  constexpr T* base() const noexcept { return node_; }

  // static type checking?
  constexpr decltype(T().value()) operator*() noexcept { return node_->value(); }
  constexpr decltype(&T().value()) operator->() noexcept { return &node_->value(); }

  constexpr bool operator==(const node_iterator& other) const noexcept {
    return node_ == other.node_;
  }

  constexpr bool operator!=(const node_iterator& other) const noexcept {
    return node_ != other.node_;
  }

//   constexpr bool operator>(const node_iterator& other) const noexcept {
//     return (node_ - other.node_) > 0;
//   }

//   constexpr bool operator<(const node_iterator& other) const noexcept {
//     return (node_ - other.node_) < 0;
//   }

//   constexpr bool operator>=(const node_iterator& other) const noexcept {
//     return (node_ - other.node_) >= 0;
//   }

//   constexpr bool operator<=(const node_iterator& other) const noexcept {
//     return (node_ - other.node_) <= 0;
//   }

//   constexpr node_iterator operator+(difference_type delta) const noexcept {
//     return node_iterator(node_ + delta);
//   }

//   constexpr node_iterator operator-(difference_type delta) const noexcept {
//     return node_iterator(node_ - delta);
//   }

//   constexpr difference_type operator-(const node_iterator& other) const noexcept {
//     return node_ - other.node_;
//   }

//   constexpr node_iterator& operator+=(difference_type delta) noexcept {
//     node_ += delta;
//     return *this;
//   }

//   constexpr node_iterator& operator-=(difference_type delta) noexcept {
//     node_ -= delta;
//     return *this;
//   }

  constexpr node_iterator operator++(int) noexcept { 
    return node_iterator(node_ = node_->next()); 
  }

  constexpr node_iterator operator--(int) noexcept { 
    return node_iterator(node_ = node_->prev()); 
  }

  constexpr node_iterator& operator++() noexcept {
    node_ = node_->next();
    return *this;
  }

  constexpr node_iterator& operator--() noexcept {
    node_ = node_->prev();
    return *this;
  }

  constexpr operator node_iterator<const T, Container>() const noexcept {
    return node_iterator<const T, Container>(node_);
  }

 protected:
  node_type* node_;
};
}  // namespace s21
#endif  // SP_MEMORY_NODE_ITERATOR_H_
