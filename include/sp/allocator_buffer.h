#ifndef SP_MEMORY_MEMORY_BUFFER_H_
#define SP_MEMORY_MEMORY_BUFFER_H_
#include <cstdint>
#include <type_traits>

namespace sp {
template <typename T, typename Allocator>
struct pointer_buffer {
    using al_traits = std::allocator_traits<Allocator>;
    using value_type = T;
    using pointer = al_traits::pointer;
    using size_type = int64_t;
    using difference_type = int64_t;

    constexpr explicit pointer_buffer(Allocator* al = nullptr)
        : alc(al), ptr(nullptr), cap(0) {}
    constexpr pointer_buffer(size_type size, Allocator* al,
                             pointer hint = nullptr)
        : alc(al), cap(size) {
      if (cap < 0) {
        throw std::invalid_argument("Invalid memory buffer length");
      }
      ptr = (cap) ? al_traits::allocate(*alc, size, hint) : nullptr;
    }
    constexpr pointer_buffer(const pointer_buffer&) = delete;
    constexpr pointer_buffer(pointer_buffer&& other) = delete;
    constexpr pointer_buffer& operator=(const pointer_buffer&) = delete;
    constexpr pointer_buffer& operator=(pointer_buffer&& other) = delete;
    constexpr ~pointer_buffer() { al_traits::deallocate(*alc, ptr, cap); }

    Allocator* const alc;
    pointer const ptr;
    const size_type cap;
  };
}  // namespace sp
#endif  // SP_MEMORY_MEMORY_BUFFER_H_
