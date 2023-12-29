#ifndef SP_MEMORY_MEMORY_BUFFER_H_
#define SP_MEMORY_MEMORY_BUFFER_H_
#include <cstdint>
#include <type_traits>

namespace sp {
template <typename T, typename Allocator>
class allocator_buffer {
  using al_traits = std::allocator_traits<Allocator>;

 public:
  using value_type = T;
  using allocator_type = T;
  using pointer = al_traits::pointer;
  using size_type = int64_t;

  constexpr explicit allocator_buffer(Allocator& al)
      : alc_(al), ptr_(nullptr), cap_(0) {}
  constexpr allocator_buffer(size_type size, Allocator& al,
                             pointer hint = nullptr)
      : alc_(al), cap_(size) {
    if (cap_ < 0) {
      throw std::invalid_argument("Invalid memory buffer length");
    }
    ptr_ = (cap_) ? al_traits::allocate(alc_, size, hint) : nullptr;
  }
  constexpr allocator_buffer(const allocator_buffer&) = delete;

  constexpr allocator_buffer(allocator_buffer&& other) noexcept
      : alc_(other.alc_), ptr_(other.ptr_), cap_(other.cap_) {
    other.ptr_ = nullptr;
    other.cap_ = 0;
  }

  constexpr allocator_buffer& operator=(const allocator_buffer&) = delete;

  constexpr allocator_buffer& operator=(allocator_buffer&& other) noexcept {
    if (alc_ == other.alc_) {
      std::swap(ptr_, other.ptr_);
      std::swap(cap_, other.cap_);
    }
  }

  constexpr ~allocator_buffer() { al_traits::deallocate(alc_, ptr_, cap_); }

  constexpr pointer get() const noexcept { return ptr_; }
  constexpr size_type capacity() const noexcept { return cap_; }

  constexpr void swap_out(allocator_buffer& other) noexcept {
    *this = std::move(other);
  }

 private:
  allocator_type& alc_;
  pointer ptr_;
  size_type cap_;
};
}  // namespace sp
#endif  // SP_MEMORY_MEMORY_BUFFER_H_
