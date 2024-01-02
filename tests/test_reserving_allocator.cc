#include <gtest/gtest.h>

#include "sp/reserving_allocator.h"
#include "test_helpers.h"

TEST(ReserveAlloc, ctor_def) {
  sp::reserving_allocator<safe> al;

  ASSERT_EQ(al.capacity(), 0);
}

// TEST(ReserveAlloc, ctor_size) {
//   constexpr int64_t size = 20;
//   sp::reserving_allocator<safe> al(size);

//   ASSERT_EQ(al.capacity(), size);
//   safe* ptr = al.allocate(1);
//   ASSERT_EQ(al.capacity(), size - 1);
//   ASSERT_NO_THROW(al.deallocate(ptr, 1));
// }

// TEST(ReserveAlloc, ctor_size_neg) {
//   constexpr int64_t size = -20;
//   ASSERT_THROW(sp::reserving_allocator<safe> al(size), std::invalid_argument);
// }

TEST(ReserveAlloc, ctor_copy) {
  constexpr int64_t size = 20;
  sp::reserving_allocator<safe> al(size);
  sp::reserving_allocator<safe> cpy(al);

  for (int i = 0; i < size; ++i) {
    ASSERT_NO_THROW(cpy.deallocate(al.allocate(1), 1));
  }

  ASSERT_EQ(al.capacity(), 0);
  ASSERT_EQ(cpy.capacity(), size);
}

TEST(ReserveAlloc, ctor_move) {
  constexpr int64_t size = 20;
  sp::reserving_allocator<safe> al(size);
  sp::reserving_allocator<safe> mv(std::move(al));

  ASSERT_EQ(mv.capacity(), size);

  safe* ptr[size];

  for (int i = 0; i < size; ++i) {
    ASSERT_NO_THROW(ptr[i] = al.allocate(1));
  }

  for (int i = 0; i < size; ++i) {
    al.deallocate(ptr[i], 1);
  }

  ASSERT_EQ(mv.capacity(), al.capacity());
}

TEST(ReserveAlloc, ctor_assign_copy) {
  constexpr int64_t size = 20;
  sp::reserving_allocator<safe> al(size);
  sp::reserving_allocator<safe> cpy(size * 2);

  cpy = al;

  for (int i = 0; i < size; ++i) {
    ASSERT_NO_THROW(cpy.deallocate(al.allocate(1), 1));
  }

  ASSERT_EQ(al.capacity(), 0);
  ASSERT_EQ(cpy.capacity(), size);
}

TEST(ReserveAlloc, ctor_assign_move) {
  constexpr int64_t size = 20;
  sp::reserving_allocator<safe> al(size);
  sp::reserving_allocator<safe> mv(size * 2);

  mv = std::move(al);

  ASSERT_EQ(mv.capacity(), size);
}

TEST(ReserveAlloc, swap) {
  constexpr int64_t size = 20;
  sp::reserving_allocator<safe> lhs(size);
  sp::reserving_allocator<safe> rhs(size * 2);

  int64_t lhs_cap = lhs.capacity();
  int64_t rhs_cap = rhs.capacity();

  using std::swap;
  swap(rhs, lhs);

  ASSERT_EQ(rhs.capacity(), lhs_cap);
  ASSERT_EQ(lhs.capacity(), rhs_cap);
}

TEST(ReserveAlloc, alloc) {
  constexpr int64_t alloc_count = 20;
  sp::reserving_allocator<safe> al;

  safe* ptr[alloc_count];

  for (int i = 0; i < alloc_count; ++i) {
    ASSERT_NO_THROW(ptr[i] = al.allocate(1));
  }

  for (int i = 0; i < alloc_count; ++i) {
    al.deallocate(ptr[i], 1);
  }

  ASSERT_EQ(al.capacity(), alloc_count);
}

TEST(ReserveAlloc, alloc_zero) {
  constexpr int64_t size = 20;
  sp::reserving_allocator<safe> al(size);

  ASSERT_THROW(al.allocate(0), std::bad_alloc);

  ASSERT_EQ(al.capacity(), size);
}

TEST(ReserveAlloc, alloc_existing) {
  constexpr int64_t alloc_count = 20;
  constexpr int64_t size = 20;
  sp::reserving_allocator<safe> al(size);

  for (int i = 0; i < alloc_count; ++i) {
    ASSERT_NO_THROW(al.deallocate(al.allocate(1), 1));
  }

  ASSERT_EQ(al.capacity(), size);
}

TEST(ReserveAlloc, alloc_invalid) {
  constexpr int64_t alloc_count = 20;
  sp::reserving_allocator<safe> al;

  safe* ptr[alloc_count];

  for (int i = 0; i < alloc_count; ++i) {
    ASSERT_NO_THROW(ptr[i] = al.allocate(1));
  }

  for (int i = 0; i < alloc_count; ++i) {
    al.deallocate(ptr[i], 1);
  }

  ASSERT_EQ(al.capacity(), alloc_count);
  ASSERT_THROW(al.allocate(2), std::bad_alloc);
  ASSERT_THROW(al.allocate(-2), std::bad_alloc);
}
