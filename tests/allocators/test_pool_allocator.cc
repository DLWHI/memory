#include <gtest/gtest.h>

#include "memory/allocators/pool_allocator.h"
#include "../test_helpers.h"

TEST(PoolAlloc, ctor) {
  constexpr int64_t size = 1024;
  memory::pool_allocator<uint8_t> al(size);

  ASSERT_EQ(al.max_size(), size);
  al.deallocate(al.allocate(size), size);
  ASSERT_THROW(al.allocate(size + 1), std::bad_alloc);
}

TEST(PoolAlloc, ctor_copy) {
  constexpr int64_t size = 20*sizeof(subject);
  constexpr int64_t count = 20;
  memory::pool_allocator<subject> al(size);
  memory::pool_allocator<subject> cpy(al);

  ASSERT_EQ(al.max_size(), cpy.max_size());
  ASSERT_EQ(al, cpy);
  subject* ptr = al.allocate(count);
  cpy.deallocate(ptr, count);
  ASSERT_THROW(cpy.allocate(count + 1), std::bad_alloc);
}

TEST(PoolAlloc, ctor_move) {
  constexpr int64_t size = 20*sizeof(subject);
  constexpr int64_t count = 10;
  memory::pool_allocator<subject> al(size);
  memory::pool_allocator<subject> mv(std::move(al));

  ASSERT_EQ(mv.max_size(), size/sizeof(subject));
  mv.deallocate(mv.allocate(count), count);
  ASSERT_THROW(mv.allocate(size), std::bad_alloc);
}

TEST(PoolAlloc, swap) {
  constexpr int64_t size = 1023;
  memory::pool_allocator<subject> lhs(size);
  memory::pool_allocator<subject> rhs(size * 2);

  memory::pool_allocator<subject> lhs_cpy(lhs);
  memory::pool_allocator<subject> rhs_cpy(rhs);

  using std::swap;
  swap(rhs, lhs);

  ASSERT_EQ(rhs, lhs_cpy);
  ASSERT_EQ(lhs, rhs_cpy);
}

TEST(PoolAlloc, rebind) {
  using traits = typename std::allocator_traits<memory::pool_allocator<subject>>;
  using rebind = typename traits::template rebind_alloc<large>;
  using rebind_traits = typename std::allocator_traits<rebind>;
  using rebind_rebind = typename rebind_traits::template rebind_alloc<subject>;

  constexpr int64_t size = 20*sizeof(subject) + 10*sizeof(large);

  memory::pool_allocator<subject> al(size);
  rebind al_rebind(al);
  rebind_rebind al_rebind_rebind(al_rebind);
  
  EXPECT_EQ(al, al_rebind_rebind);

  EXPECT_NO_THROW(al.deallocate(al_rebind_rebind.allocate(1), 1));
  EXPECT_NO_THROW(al_rebind.deallocate(al_rebind.allocate(1), 1));

  rebind rbd(size);
  rebind_rebind nrm(rbd);
  rebind rbd_rbd(nrm);

  EXPECT_EQ(rbd, rbd_rbd);
  EXPECT_NO_THROW(rbd_rbd.deallocate(rbd.allocate(1), 1));
  EXPECT_NO_THROW(nrm.deallocate(nrm.allocate(1), 1));
}

TEST(PoolAlloc, alloc_chunk) {
  constexpr int64_t size = 20*sizeof(subject);
  constexpr int64_t alloc_size = 7;
  memory::pool_allocator<subject> al(size);

  subject* ptr;
  ASSERT_NO_THROW(ptr = al.allocate(alloc_size));
  ASSERT_NE(ptr, nullptr);
  al.deallocate(ptr, alloc_size);
}

TEST(PoolAlloc, alloc_from_diff) {
  constexpr int64_t size = 20*sizeof(subject);
  constexpr int64_t alloc_size = 7;
  memory::pool_allocator<subject> al1(size);
  memory::pool_allocator<subject> al2(size);
  ASSERT_NE(al1, al2);

  subject* ptr1;
  subject* ptr2;
  ASSERT_NO_THROW(ptr1 = al1.allocate(alloc_size));
  ASSERT_NE(ptr1, nullptr);
  ASSERT_NO_THROW(ptr2 = al2.allocate(alloc_size));
  ASSERT_NE(ptr2, nullptr);
  al1.deallocate(ptr1, alloc_size);
  al2.deallocate(ptr2, alloc_size);
}

TEST(PoolAlloc, alloc_zero) {
  constexpr int64_t size = 20;
  memory::pool_allocator<subject> al(size);

  subject* ptr;
  ASSERT_NO_THROW(ptr = al.allocate(0));
  ASSERT_NE(ptr, nullptr);
  al.deallocate(ptr, 0);
}

TEST(PoolAlloc, alloc_almost_all) {
  constexpr int64_t size = 20*sizeof(subject);
  constexpr int64_t count = 19;
  memory::pool_allocator<subject> al(size);

  subject* ptr;
  ASSERT_NO_THROW(ptr = al.allocate(count));
  ASSERT_NE(ptr, nullptr);
  al.deallocate(ptr, count);
}

TEST(PoolAlloc, alloc_multiple) {
  constexpr int64_t size = 20*sizeof(subject);
  constexpr int64_t alloc_size = 4;
  memory::pool_allocator<subject> al(size);

  for (int i = 0; i < size / alloc_size; ++i) {
    subject* ptr;
    ASSERT_NO_THROW(ptr = al.allocate(alloc_size)); 
    ASSERT_NE(ptr, nullptr);
    al.deallocate(ptr, alloc_size);
  }
}

TEST(PoolAlloc, alloc_continious) {
  constexpr int64_t count = 20;
  constexpr int64_t size = count*sizeof(subject);
  constexpr int64_t alloc_size = 4;
  memory::pool_allocator<subject> al(size);

  subject* ptr_array[count / alloc_size];
  for (int i = 0; i < count / alloc_size; ++i) {
    ASSERT_NO_THROW(ptr_array[i] = al.allocate(alloc_size));
    ASSERT_NE(ptr_array[i], nullptr);
  }
  ASSERT_EQ(al.remaining(), 0);
  for (int i = 0; i < count / alloc_size; ++i) {
    al.deallocate(ptr_array[i], alloc_size);
  }
}

TEST(PoolAlloc, alloc_continious_race) {
  constexpr int64_t count = 20;
  constexpr int64_t size = count*sizeof(subject);
  constexpr int64_t alloc_size = 4;
  memory::pool_allocator<subject> al(size);

  subject* ptr_array[count / alloc_size];

  ASSERT_NO_THROW(ptr_array[0] = al.allocate(alloc_size));
  ASSERT_NE(ptr_array[0], nullptr);
  for (int i = 1; i < count / alloc_size; ++i) {
    ASSERT_NO_THROW(ptr_array[i] = al.allocate(alloc_size));
    ASSERT_NE(ptr_array[i], nullptr);
    al.deallocate(ptr_array[i - 1], alloc_size);
    ASSERT_EQ(al.remaining() + al.allocd(), size);
  }
}

TEST(PoolAlloc, alloc_continious_race_non_uniform) {
  constexpr int64_t size = 20*sizeof(subject);
  constexpr int64_t allocs[] = {2, 7, 4, 8, 10};
  constexpr int64_t allocs_size = sizeof(allocs) / sizeof(int64_t);
  memory::pool_allocator<subject> al(size);

  subject* ptr_array[allocs_size];

  ASSERT_NO_THROW(ptr_array[0] = al.allocate(allocs[0]));
  for (int i = 1; i < allocs_size; ++i) {
    ASSERT_NO_THROW(ptr_array[i] = al.allocate(allocs[i]));
    ASSERT_NE(ptr_array[i], nullptr);
    al.deallocate(ptr_array[i - 1], allocs[i - 1]);
    ASSERT_EQ(al.remaining() + al.allocd(), size);
  }
  al.deallocate(ptr_array[allocs_size - 1], allocs[allocs_size - 1]);
}

TEST(PoolAlloc, alloc_rebind) {
  constexpr int64_t size = 20*sizeof(subject) + 10*sizeof(large);
  constexpr int64_t sallocs[] = {2, 7, 4};
  constexpr int64_t sallocs_size = sizeof(sallocs) / sizeof(int64_t);

  constexpr int64_t lallocs[] = {4, 1, 2};
  constexpr int64_t lallocs_size = sizeof(lallocs) / sizeof(int64_t);


  memory::pool_allocator<subject> al(size);
  std::allocator_traits<memory::pool_allocator<subject>>::template rebind_alloc<large> al_rebind(al);

  subject* subjects[sallocs_size];
  large* larges[lallocs_size];

  ASSERT_NO_THROW(subjects[0] = al.allocate(sallocs[0]));
  ASSERT_NO_THROW(larges[0] = al_rebind.allocate(lallocs[0]));
  for (int i = 1; i < sallocs_size; ++i) {
    ASSERT_NO_THROW(subjects[i] = al.allocate(sallocs[i]));
    ASSERT_NE(subjects[i], nullptr);
    al.deallocate(subjects[i - 1], sallocs[i - 1]);
    ASSERT_EQ(al.remaining() + al.allocd(), size);
  }
  for (int i = 1; i < lallocs_size; ++i) {
    ASSERT_NO_THROW(larges[i] = al_rebind.allocate(lallocs[i]));
    ASSERT_NE(larges[i], nullptr);
    al_rebind.deallocate(larges[i - 1], lallocs[i - 1]);
    ASSERT_EQ(al_rebind.remaining() + al_rebind.allocd(), size);
  }
  al.deallocate(subjects[sallocs_size - 1], sallocs[sallocs_size - 1]);
  al_rebind.deallocate(larges[lallocs_size - 1], lallocs[lallocs_size - 1]);
}

TEST(PoolAlloc, alloc_continious_exceed) {
  constexpr int64_t size = 20*sizeof(subject);
  constexpr int64_t allocs[] = {2, 7, 4};
  constexpr int64_t allocs_size = sizeof(allocs) / sizeof(int64_t);
  constexpr int64_t extra_size = 10;
  memory::pool_allocator<subject> al(size);

  subject* ptr_array[allocs_size];

  for (int i = 0; i < allocs_size; ++i) {
    ASSERT_NO_THROW(ptr_array[i] = al.allocate(allocs[i]));
    ASSERT_NE(ptr_array[i], nullptr);
  }
  ASSERT_THROW(al.allocate(extra_size), std::bad_alloc);
  for (int i = 0; i < allocs_size; ++i) {
    al.deallocate(ptr_array[i], allocs[i]);
  }
}

TEST(PoolAlloc, dealloc_nullptr) {
  constexpr int64_t count = 20;
  constexpr int64_t size = count*sizeof(subject);
  memory::pool_allocator<subject> al(size);

  al.deallocate(nullptr, 0);

  subject* ptr = al.allocate(count);
  al.deallocate(ptr, count);
}

TEST(PoolAlloc, alloc_empty) {
  constexpr int64_t size = 20*sizeof(subject);
  memory::pool_allocator<subject> al(size);

  ASSERT_NO_THROW(al.deallocate(al.allocate(0), 0));
}

