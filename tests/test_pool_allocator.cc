#include <gtest/gtest.h>

#include "sp/pool_allocator.h"
#include "test_helpers.h"

TEST(PoolAlloc, ctor) {
  constexpr int64_t size = 20;
  sp::pool_allocator<safe> al(size);

  ASSERT_EQ(al.max_size(), size);
  ASSERT_NO_THROW(al.deallocate(al.allocate(size), size));
  ASSERT_THROW(al.allocate(size + 1), std::bad_alloc);
}

TEST(PoolAlloc, ctor_size_neg) {
  constexpr int64_t size = -20;
  ASSERT_THROW(sp::pool_allocator<safe> al(size), std::invalid_argument);
}

TEST(PoolAlloc, ctor_copy) {
  constexpr int64_t size = 20;
  sp::pool_allocator<safe> al(size);
  sp::pool_allocator<safe> cpy(al);

  ASSERT_EQ(al.max_size(), cpy.max_size());
  ASSERT_EQ(al, cpy);
  safe* ptr = al.allocate(size);
  ASSERT_NO_THROW(cpy.deallocate(ptr, size));
  ASSERT_THROW(cpy.allocate(size + 1), std::bad_alloc);
}

TEST(PoolAlloc, ctor_move) {
  constexpr int64_t size = 20;
  sp::pool_allocator<safe> al(size);
  sp::pool_allocator<safe> mv(std::move(al));

  ASSERT_EQ(mv.max_size(), size);
  ASSERT_NE(al, mv);
  ASSERT_THROW(al.allocate(10), std::bad_alloc);
  ASSERT_NO_THROW(mv.deallocate(mv.allocate(size), size));
  ASSERT_THROW(mv.allocate(size + 1), std::bad_alloc);
}

TEST(PoolAlloc, ctor_assign_copy) {
  constexpr int64_t size = 20;
  sp::pool_allocator<safe> al(size * 2);
  sp::pool_allocator<safe> cpy(size);

  cpy = al;

  ASSERT_EQ(al.max_size(), cpy.max_size());
  ASSERT_EQ(al, cpy);
  safe* ptr = al.allocate(size);
  ASSERT_NO_THROW(cpy.deallocate(ptr, size));
  ASSERT_THROW(cpy.allocate(size * 2 + 1), std::bad_alloc);
}

TEST(PoolAlloc, ctor_assign_move) {
  constexpr int64_t size = 20;
  sp::pool_allocator<safe> al(size);
  sp::pool_allocator<safe> mv(size * 2);

  mv = std::move(al);

  ASSERT_EQ(mv.max_size(), size);
  ASSERT_NE(al, mv);
  ASSERT_THROW(al.allocate(10), std::bad_alloc);
  ASSERT_NO_THROW(mv.deallocate(mv.allocate(size), size));
  ASSERT_THROW(mv.allocate(size + 1), std::bad_alloc);
}

TEST(PoolAlloc, swap) {
  constexpr int64_t size = 20;
  sp::pool_allocator<safe> lhs(size);
  sp::pool_allocator<safe> rhs(size * 2);

  sp::pool_allocator<safe> lhs_cpy(lhs);
  sp::pool_allocator<safe> rhs_cpy(rhs);

  std::swap(rhs, lhs);

  ASSERT_EQ(rhs, lhs_cpy);
  ASSERT_EQ(lhs, rhs_cpy);
}

TEST(PoolAlloc, alloc_chunk) {
  constexpr int64_t size = 20;
  constexpr int64_t alloc_size = 7;
  sp::pool_allocator<safe> al(size);

  safe* ptr;
  ASSERT_NO_THROW(ptr = al.allocate(alloc_size));
  ASSERT_NE(ptr, nullptr);
  al.deallocate(ptr, alloc_size);
}

TEST(PoolAlloc, alloc_almost_all) {
  constexpr int64_t size = 20;
  sp::pool_allocator<safe> al(size);

  safe* ptr;
  ASSERT_NO_THROW(ptr = al.allocate(size - 1));
  ASSERT_NE(ptr, nullptr);
  al.deallocate(ptr, size - 1);
}

TEST(PoolAlloc, alloc_multiple) {
  constexpr int64_t size = 20;
  constexpr int64_t alloc_size = 4;
  sp::pool_allocator<safe> al(size);

  for (int i = 0; i < size / alloc_size; ++i) {
    safe* ptr;
    ASSERT_NO_THROW(ptr = al.allocate(alloc_size));
    ASSERT_NE(ptr, nullptr);
    al.deallocate(ptr, alloc_size);
  }
}

TEST(PoolAlloc, alloc_continious) {
  constexpr int64_t size = 20;
  constexpr int64_t alloc_size = 4;
  sp::pool_allocator<safe> al(size);

  safe* ptr_array[size / alloc_size];
  for (int i = 0; i < size / alloc_size; ++i) {
    ASSERT_NO_THROW(ptr_array[i] = al.allocate(alloc_size));
    ASSERT_NE(ptr_array[i], nullptr);
  }
  ASSERT_EQ(al.leftover(), 0);
  for (int i = 0; i < size / alloc_size; ++i) {
    al.deallocate(ptr_array[i], alloc_size);
  }
}

TEST(PoolAlloc, alloc_continious_race) {
  constexpr int64_t size = 20;
  constexpr int64_t alloc_size = 4;
  sp::pool_allocator<safe> al(size);

  safe* ptr_array[size / alloc_size];

  ASSERT_NO_THROW(ptr_array[0] = al.allocate(alloc_size));
  ASSERT_NE(ptr_array[0], nullptr);
  for (int i = 1; i < size / alloc_size; ++i) {
    ASSERT_NO_THROW(ptr_array[i] = al.allocate(alloc_size));
    ASSERT_NE(ptr_array[i], nullptr);
    al.deallocate(ptr_array[i - 1], alloc_size);
    ASSERT_EQ(al.leftover(), size - alloc_size);
  }
}

TEST(PoolAlloc, alloc_continious_race_non_uniform) {
  constexpr int64_t size = 20;
  constexpr int64_t allocs[] = {2, 7, 4, 8, 10};
  sp::pool_allocator<safe> al(size);

  safe* ptr_array[sizeof(allocs) / sizeof(int64_t)];

  ASSERT_NO_THROW(ptr_array[0] = al.allocate(allocs[0]));
  for (size_t i = 1; i < sizeof(allocs) / sizeof(int64_t); ++i) {
    ASSERT_NO_THROW(ptr_array[i] = al.allocate(allocs[i]));
    ASSERT_NE(ptr_array[i], nullptr);
    al.deallocate(ptr_array[i - 1], allocs[i - 1]);
    ASSERT_EQ(al.leftover(), size - allocs[i]);
  }
}

TEST(PoolAlloc, alloc_continious_exceed) {
  constexpr int64_t size = 20;
  constexpr int64_t allocs[] = {2, 7, 4};
  constexpr int64_t extra_size = 10;
  sp::pool_allocator<safe> al(size);

  safe* ptr_array[sizeof(allocs) / sizeof(int64_t)];

  for (size_t i = 0; i < sizeof(allocs) / sizeof(int64_t); ++i) {
    ASSERT_NO_THROW(ptr_array[i] = al.allocate(allocs[i]));
    ASSERT_NE(ptr_array[i], nullptr);
  }
  ASSERT_THROW(al.allocate(extra_size), std::bad_alloc);
  for (size_t i = 0; i < sizeof(allocs) / sizeof(int64_t); ++i) {
    al.deallocate(ptr_array[i], allocs[i]);
  }
}