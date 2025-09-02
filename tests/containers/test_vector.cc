#include <chrono>
#include <iostream>
#include <list>
#include <random>
#include <sstream>
#include <vector>

#include <gtest/gtest.h>
#include "memory/allocators/pool_allocator.h"
#include "memory/containers/vector.h"
#include "../test_helpers.h"

static std::random_device ran_dev;
static std::mt19937 gen(ran_dev());

#ifdef MAX_SIZE
static std::uniform_int_distribution<std::size_t> uid(1, MAX_SIZE);
#else
static std::uniform_int_distribution<std::size_t> uid(1, 100);
#endif

#ifdef LOOP_COUNT
constexpr static std::size_t loop = LOOP_COUNT;
#else
constexpr static std::size_t loop = 15;
#endif

// constexpr std::size_t constexpr_check(std::size_t val) {
//   memory::vector<std::size_t> vec = {1, 2, 3, 4, 5};
//   vec.emplace(vec.end(), 7);
//   vec.reserve(100);
//   vec.push_back(6);
//   vec.erase(--vec.end());
//   vec.insert(vec.begin(), 0);
//   vec.insert(vec.begin() + 1, 0);
//   vec.insert(vec.begin() + 3, -1);
//   vec.insert(--vec.end(), -1);
//   vec.shrink_to_fit();
//   vec.assign({6, 6, 6, 7, 7, 7});
//   vec.push_back(val);
//   return *(vec.end() - 1);
// }

//==============================================================================
// ctors
TEST(VectorTest, ctor_default) {
  const memory::vector<no_def> vec;
  ASSERT_EQ(vec.size(), 0);
  ASSERT_EQ(vec.capacity(), 0);
  ASSERT_EQ(vec.data(), nullptr);
  ASSERT_THROW(vec.at(uid(gen)), std::out_of_range);
}

TEST(VectorTest, ctor_size_alloc_not_default) {
  std::size_t size = uid(gen);
  memory::pool_allocator<not_safe> not_default(size * 3 / 2* sizeof(not_safe));
  const memory::vector<not_safe, memory::pool_allocator<not_safe>> vec(size, not_default);
  ASSERT_EQ(vec.size(), size);
  ASSERT_EQ(vec.capacity(), size);
  ASSERT_TRUE(vec.get_allocator() == not_default);
  ASSERT_EQ(not_default.allocd(), size*sizeof(not_safe));
  ASSERT_NE(vec.data(), nullptr);
  ASSERT_EQ(vec.front(), not_safe());
  ASSERT_EQ(vec.back(), not_safe());
}

TEST(VectorTest, ctor_size_bad_alloc) {
  std::size_t size = uid(gen);
  memory::pool_allocator<not_safe> not_default(size);

  try {
    const memory::vector<not_safe, memory::pool_allocator<not_safe>> vec(size * 2, not_default);
    FAIL();
  } catch (std::bad_alloc &) {
  }
}

TEST(VectorTest, ctor_size) {
  std::size_t size = uid(gen);
  const memory::vector<not_safe> vec(size);

  ASSERT_EQ(vec.size(), size);
  ASSERT_EQ(vec.capacity(), size);
  ASSERT_NE(vec.data(), nullptr);
  ASSERT_EQ(vec.front(), safe());
  ASSERT_EQ(vec.back(), safe());
}

TEST(VectorTest, ctor_size_zero) {
  const memory::vector<not_safe> vec(0);

  ASSERT_EQ(vec.size(), 0);
  ASSERT_EQ(vec.capacity(), 0);
  ASSERT_EQ(vec.data(), nullptr);
  ASSERT_THROW(vec.at(uid(gen)), std::out_of_range);
}

TEST(VectorTest, ctor_size_value) {
  std::size_t size = uid(gen);
  const memory::vector<no_def> vec(size, no_def("not default"));

  ASSERT_EQ(vec.size(), size);
  ASSERT_EQ(vec.capacity(), size);
  ASSERT_NE(vec.data(), nullptr);
  ASSERT_EQ(vec.front(), no_def("not default"));
  ASSERT_EQ(vec.back(), no_def("not default"));
}

TEST(VectorTest, ctor_size_value_zero) {
  const memory::vector<no_def> vec(0, no_def("not default"));

  ASSERT_EQ(vec.size(), 0);
  ASSERT_EQ(vec.capacity(), 0);
  ASSERT_EQ(vec.data(), nullptr);
}

TEST(VectorTest, ctor_size_value_throwing) {
  throwing::count = 0;
  std::size_t size = uid(gen) + 5;
  std::allocator<throwing> not_default;
  ASSERT_ANY_THROW(
      memory::vector<throwing> vec(size, throwing("not default"), not_default));
}

TEST(VectorTest, ctor_from_stl_vector) {
  std::size_t size = uid(gen);
  std::vector<safe> from(size, safe("not default"));

  const memory::vector<safe> vec(from.begin(), from.end());
  ASSERT_EQ(vec.size(), from.size());
  ASSERT_EQ(vec.capacity(), from.size());
  ASSERT_NE(vec.data(), nullptr);
  ASSERT_EQ(vec.front(), from.front());
  ASSERT_EQ(vec.back(), from.back());
  ASSERT_EQ(vec.front().birth, constructed::kCopy);
  ASSERT_EQ(vec.back().birth, constructed::kCopy);
}

TEST(VectorTest, ctor_from_stl_vector_strings) {
  std::size_t size = uid(gen);
  std::vector<std::string> from(size, "not default");

  const memory::vector<safe> vec(from.begin(), from.end());
  ASSERT_EQ(vec.size(), from.size());
  ASSERT_NE(vec.data(), nullptr);
  ASSERT_EQ(vec.front(), safe(from.front()));
  ASSERT_EQ(vec.back(), safe(from.back()));
}

TEST(VectorTest, ctor_from_stl_list) {
  std::size_t size = uid(gen);
  std::list<safe> from(size, safe("not default"));

  const memory::vector<safe> vec(from.begin(), from.end());

  ASSERT_EQ(vec.size(), from.size());
  ASSERT_EQ(vec.capacity(), from.size());
  ASSERT_NE(vec.data(), nullptr);
  ASSERT_EQ(vec.front(), from.front());
  ASSERT_EQ(vec.back(), from.back());
  ASSERT_EQ(vec.front().birth, constructed::kCopy);
  ASSERT_EQ(vec.back().birth, constructed::kCopy);
}

TEST(VectorTest, ctor_range_empty) {
  const std::allocator<safe> not_default;
  const safe range[] = {safe("first"), safe("second"), safe("last")};
  const memory::vector<safe> vec(&range[0], &range[0], not_default);
  ASSERT_EQ(vec.size(), 0);
  ASSERT_EQ(vec.capacity(), 0);
  ASSERT_EQ(vec.data(), nullptr);
}

TEST(VectorTest, ctor_range_throwing) {
  throwing::count = 0;
  const std::allocator<throwing> not_default;
  const throwing range[] = {throwing("first"), throwing("second"),
                            throwing("last"), throwing("last"),
                            throwing("last")};
  ASSERT_ANY_THROW(const memory::vector<throwing> vec(
      std::begin(range), std::end(range), not_default));
}

TEST(VectorTest, ctor_init_list) {
  std::initializer_list<std::string> list{"Karen",   "Anastasia", "Alice",
                                          "Natalie", "Leyla",     "Victoria"};

  const memory::vector<std::string> vec(list);
  ASSERT_EQ(vec.size(), list.size());
  ASSERT_EQ(vec.capacity(), list.size());
  ASSERT_NE(vec.data(), nullptr);
  ASSERT_EQ(vec.front(), "Karen");
}

TEST(VectorTest, ctor_init_list_implicit) {
  const memory::vector<std::string> vec{"Karen",   "Anastasia", "Alice",
                                      "Natalie", "Leyla",     "Victoria"};

  ASSERT_EQ(vec.size(), 6);
  ASSERT_EQ(vec.capacity(), 6);
  ASSERT_NE(vec.data(), nullptr);
  ASSERT_EQ(vec.front(), "Karen");
}

TEST(VectorTest, ctor_init_list_empty) {
  const memory::vector<std::string> vec{};

  ASSERT_EQ(vec.size(), 0);
  ASSERT_EQ(vec.capacity(), 0);
  ASSERT_EQ(vec.data(), nullptr);
}

TEST(VectorTest, ctor_init_list_throwing) {
  throwing::count = 0;
  ASSERT_ANY_THROW(memory::vector<throwing> vec(
      {throwing(), throwing(), throwing(), throwing(), throwing()}));
}

TEST(VectorTest, ctor_copy) {
  std::size_t size = uid(gen);
  const memory::vector<safe> vec1(size, safe("not default"));
  const memory::vector<safe> vec2(vec1);

  ASSERT_EQ(vec1.size(), vec2.size());
  ASSERT_EQ(vec1.capacity(), vec2.capacity());
  ASSERT_NE(vec1.data(), vec2.data());
  ASSERT_EQ(vec1.front(), vec2.front());
  ASSERT_EQ(vec1.back(), vec2.front());
  ASSERT_EQ(vec2.front().birth, constructed::kCopy);
  ASSERT_EQ(vec2.back().birth, constructed::kCopy);
}

TEST(VectorTest, ctor_copy_alloc) {
  std::size_t size = 10;
  memory::pool_allocator<safe> al(size*sizeof(safe)*2);
  const memory::vector<safe, memory::pool_allocator<safe>> vec1(
      size, safe("not default"), al);
  const memory::vector<safe, memory::pool_allocator<safe>> vec2(vec1);
  ASSERT_EQ(vec1.size(), vec2.size());
  ASSERT_EQ(vec1.capacity(), vec2.capacity());
  ASSERT_NE(vec1.data(), vec2.data());
  ASSERT_EQ(vec1.front(), vec2.front());
  ASSERT_EQ(vec1.back(), vec2.front());
  ASSERT_EQ(vec1.get_allocator(), vec2.get_allocator());
  ASSERT_EQ(al.remaining(), 0);
  ASSERT_EQ(vec2.front().birth, constructed::kCopy);
  ASSERT_EQ(vec2.back().birth, constructed::kCopy);}

TEST(VectorTest, ctor_copy_size_zero) {
  const memory::vector<safe> vec1(0);
  const memory::vector<safe> vec2(vec1);

  ASSERT_EQ(vec1.size(), vec2.size());
  ASSERT_EQ(vec1.capacity(), vec2.capacity());
  ASSERT_EQ(vec1.data(), vec2.data());
}

TEST(VectorTest, ctor_copy_safe_alloc_always_equal) {
  std::size_t size = uid(gen);
  std::allocator<safe> not_default;
  memory::vector<safe> vec1(size);
  const memory::vector<safe> vec2(vec1, not_default);

  ASSERT_EQ(vec1.size(), vec2.size());
  ASSERT_EQ(vec1.capacity(), vec2.capacity());
  ASSERT_NE(vec1.data(), vec2.data());
  ASSERT_EQ(vec1.front(), vec2.front());
  ASSERT_EQ(vec1.back(), vec2.front());
  ASSERT_EQ(vec2.front().birth, constructed::kCopy);
  ASSERT_EQ(vec2.back().birth, constructed::kCopy);
}

TEST(VectorTest, ctor_copy_safe_alloc_not_equal) {
  std::size_t size = uid(gen) + 10;
  memory::pool_allocator<safe> al1(size*sizeof(safe));
  memory::pool_allocator<safe> al2(size * 2*sizeof(safe));
  memory::vector<safe, memory::pool_allocator<safe>> vec1(size, al1);

  const memory::vector<safe, memory::pool_allocator<safe>> vec2(vec1, al2);
  ASSERT_EQ(vec1.size(), vec2.size());
  ASSERT_EQ(vec1.capacity(), vec2.capacity());
  ASSERT_NE(vec1.data(), vec2.data());
  ASSERT_EQ(vec1.front(), vec2.front());
  ASSERT_EQ(vec1.back(), vec2.front());
  ASSERT_TRUE(vec1.get_allocator() != vec2.get_allocator());
  ASSERT_TRUE(vec1.get_allocator() == al1);
  ASSERT_TRUE(vec2.get_allocator() == al2);
  ASSERT_TRUE(al2.allocd() == al1.allocd());
  ASSERT_EQ(vec2.front().birth, constructed::kCopy);
  ASSERT_EQ(vec2.back().birth, constructed::kCopy);
}

TEST(VectorTest, ctor_copy_throwing_alloc_not_default) {
  throwing::count = 0;
  std::size_t size = uid(gen) + 5;
  memory::pool_allocator<throwing> al1(size*sizeof(throwing));
  memory::pool_allocator<throwing> al2(size*sizeof(throwing));
  memory::vector<throwing, memory::pool_allocator<throwing>> vec1(size, al1);
  try {
    memory::vector<throwing, memory::pool_allocator<throwing>> vec2(vec1, al2);
    FAIL();
  } catch (...) {
  }

  ASSERT_EQ(vec1.size(), size);
  ASSERT_NE(vec1.data(), nullptr);
  for (auto i = 0; i < size; ++i) {
    ASSERT_NO_THROW(vec1.at(i));
  }
}

TEST(VectorTest, ctor_copy_throwing) {
  throwing::count = 0;
  std::size_t size = uid(gen);
  const memory::vector<throwing> vec1(size + 5);
  ASSERT_ANY_THROW(memory::vector<throwing> vec2(vec1));
}

TEST(VectorTest, ctor_move) {
  std::size_t size = uid(gen);
  memory::vector<not_safe> vec1(size);
  std::size_t cap = vec1.capacity();
  not_safe *ptr = vec1.data();
  const memory::vector<not_safe> vec2(std::move(vec1));

  ASSERT_EQ(vec2.size(), size);
  ASSERT_EQ(vec2.capacity(), cap);
  ASSERT_EQ(vec2.data(), ptr);
  ASSERT_EQ(vec2.front(), not_safe());
  ASSERT_EQ(vec2.back(), not_safe());
  ASSERT_EQ(vec2.front().birth, constructed::kDef);
  ASSERT_EQ(vec2.back().birth, constructed::kDef);
}

TEST(VectorTest, ctor_move_throwing) {
  std::size_t size = uid(gen);
  memory::vector<throwing> vec1(size);

  ASSERT_NO_THROW(const memory::vector<throwing> vec2(std::move(vec1)););
}

TEST(VectorTest, ctor_move_safe_alloc_always_equal) {
  std::size_t size = uid(gen);
  std::allocator<safe> not_default;
  memory::vector<safe> vec1(size);
  std::size_t cap = vec1.capacity();
  safe *ptr = vec1.data();
  const memory::vector<safe> vec2(std::move(vec1), not_default);

  ASSERT_EQ(vec2.size(), size);
  ASSERT_EQ(vec2.capacity(), cap);
  ASSERT_EQ(vec2.data(), ptr);
  ASSERT_EQ(vec2.front().birth, constructed::kDef);
  ASSERT_EQ(vec2.back().birth, constructed::kDef);
}

TEST(VectorTest, ctor_move_safe_alloc_equal) {
  std::size_t size = uid(gen);
  memory::pool_allocator<safe> al(size * sizeof(safe));
  memory::vector<safe, memory::pool_allocator<safe>> vec1(size, al);
  std::size_t cap = vec1.capacity();
  const memory::vector<safe, memory::pool_allocator<safe>> vec2(std::move(vec1), al);

  ASSERT_EQ(vec2.size(), size);
  ASSERT_EQ(vec2.capacity(), cap);
  ASSERT_NE(vec1.data(), vec2.data());
  ASSERT_EQ(vec2.front(), safe());
  ASSERT_EQ(vec2.back(), safe());
  ASSERT_TRUE(vec1.get_allocator() == vec2.get_allocator());
  ASSERT_TRUE(vec2.get_allocator() == al);
  ASSERT_EQ(al.remaining(), 0);
  ASSERT_EQ(vec2.front().birth, constructed::kDef);
  ASSERT_EQ(vec2.back().birth, constructed::kDef);
}

TEST(VectorTest, ctor_move_safe_alloc) {
  std::size_t size = uid(gen);
  memory::pool_allocator<safe> al1(size*sizeof(safe));
  memory::pool_allocator<safe> al2(size*sizeof(safe));
  memory::vector<safe, memory::pool_allocator<safe>> vec1(size, safe("not default"),
                                                    al1);
  std::size_t cap = vec1.capacity();
  const memory::vector<safe, memory::pool_allocator<safe>> vec2(std::move(vec1), al2);
  ASSERT_EQ(vec2.size(), size);
  ASSERT_EQ(vec1.capacity(), cap);
  ASSERT_NE(vec1.data(), vec2.data());
  ASSERT_EQ(vec2.front(), safe("not default"));
  ASSERT_EQ(vec2.back(), safe("not default"));
  ASSERT_TRUE(vec2.get_allocator() == al2);
  ASSERT_TRUE(vec1.get_allocator() != vec2.get_allocator());
  ASSERT_TRUE(vec1.get_allocator() == al1);
  ASSERT_EQ(al1.remaining(), 0);
  ASSERT_EQ(al2.remaining(), 0);
  ASSERT_EQ(vec2.front().birth, constructed::kMove);
  ASSERT_EQ(vec2.back().birth, constructed::kMove);
}

TEST(VectorTest, ctor_move_not_safe_alloc) {
  std::size_t size = uid(gen);
  memory::pool_allocator<not_safe> al1(size*sizeof(safe));
  memory::pool_allocator<not_safe> al2(size*sizeof(safe));
  memory::vector<not_safe, memory::pool_allocator<not_safe>> vec1(
      size, not_safe("not default"), al1);
  std::size_t cap = vec1.capacity();
  const memory::vector<not_safe, memory::pool_allocator<not_safe>> vec2(
      std::move(vec1), al2);

  ASSERT_EQ(vec2.size(), size);
  ASSERT_EQ(vec1.capacity(), cap);
  ASSERT_NE(vec1.data(), vec2.data());
  ASSERT_EQ(vec2.front(), not_safe("not default"));
  ASSERT_EQ(vec2.back(), not_safe("not default"));
  ASSERT_TRUE(vec2.get_allocator() == al2);
  ASSERT_TRUE(vec1.get_allocator() != vec2.get_allocator());
  ASSERT_EQ(al1.remaining(), 0);
  ASSERT_EQ(al2.remaining(), 0);
  ASSERT_EQ(vec2.front().birth, constructed::kCopy);
  ASSERT_EQ(vec2.back().birth, constructed::kCopy);
}

TEST(VectorTest, ctor_move_throwing_alloc_not_default) {
  throwing::count = 0;
  std::size_t size = uid(gen) + 5;
  memory::pool_allocator<throwing> al1(size*sizeof(throwing));
  memory::pool_allocator<throwing> al2(size*sizeof(throwing));
  memory::vector<throwing, memory::pool_allocator<throwing>> vec1(size, al1);
  try {
    memory::vector<throwing, memory::pool_allocator<throwing>> vec2(std::move(vec1),
                                                              al2);
    FAIL();
  } catch (...) {
  }

  ASSERT_EQ(vec1.size(), size);
  ASSERT_NE(vec1.data(), nullptr);
  for (auto i = 0; i < size; ++i) {
    ASSERT_NO_THROW(vec1.at(i));
  }
}

TEST(VectorTest, assignment_copy) {
  std::size_t size = uid(gen);
  const memory::vector<safe> vec1(size, safe("not default"));
  memory::vector<safe> vec2;

  vec2 = vec1;

  ASSERT_EQ(vec1.size(), vec2.size());
  ASSERT_EQ(vec1.capacity(), vec2.capacity());
  ASSERT_NE(vec2.data(), nullptr);
  ASSERT_EQ(vec1.front(), vec2.front());
  ASSERT_EQ(vec1.back(), vec2.front());
  ASSERT_EQ(vec2.front().birth, constructed::kCopy);
  ASSERT_EQ(vec2.back().birth, constructed::kCopy);
}

TEST(VectorTest, assignment_copy_self) {
  std::size_t size = uid(gen);
  memory::vector<safe> vec1(size, safe("not default"));

  vec1 = vec1;

  ASSERT_EQ(vec1.size(), vec1.size());
  ASSERT_EQ(vec1.capacity(), vec1.capacity());
  ASSERT_NE(vec1.data(), nullptr);
  ASSERT_EQ(vec1.front(), vec1.front());
  ASSERT_EQ(vec1.back(), vec1.front());
  ASSERT_EQ(vec1.front().birth, constructed::kCopy);
  ASSERT_EQ(vec1.back().birth, constructed::kCopy);
}

TEST(VectorTest, assignment_copy_with_alloc) {
  std::size_t size = uid(gen);
  memory::pool_allocator<safe> al1(sizeof(safe)*size*2);
  memory::pool_allocator<safe> al2(sizeof(safe)*size*2);
  memory::vector<safe, memory::pool_allocator<safe>> vec1(size, safe("not default"), al1);
  memory::vector<safe, memory::pool_allocator<safe>> vec2(size/2, safe("not default"), al2);
  
  vec2 = vec1;

  ASSERT_EQ(vec1.size(), vec2.size());
  ASSERT_EQ(vec1.front(), vec2.front());
  ASSERT_EQ(vec1.back(), vec2.front());
  ASSERT_EQ(al1.allocd(), al2.allocd());
}

TEST(VectorTest, assignment_copy_with_alloc_grow) {
  std::size_t size = uid(gen);
  memory::pool_allocator<safe> al1(sizeof(safe)*size*2);
  memory::pool_allocator<safe> al2(sizeof(safe)*size*2);
  memory::vector<safe, memory::pool_allocator<safe>> vec1(size/2, safe("not default"), al1);
  memory::vector<safe, memory::pool_allocator<safe>> vec2(size, safe("not default"), al2);
  
  vec2 = vec1;

  ASSERT_EQ(vec1.size(), vec2.size());
  ASSERT_EQ(vec1.front(), vec2.front());
  ASSERT_EQ(vec1.back(), vec2.front());
  ASSERT_EQ(vec2.capacity(), size);
  ASSERT_EQ(al2.allocd(), size*sizeof(safe));
}

TEST(VectorTest, assignment_copy_throwing) {
  throwing::count = 0;
  std::size_t size = uid(gen) + 5;
  const memory::vector<throwing> vec1(size);
  memory::vector<throwing> vec2(size + 7);

  ASSERT_ANY_THROW(vec2 = vec1);

  ASSERT_EQ(vec1.size() + 7, vec2.size());
  ASSERT_EQ(vec1.capacity() + 7, vec2.capacity());
  ASSERT_NE(vec2.data(), nullptr);
  ASSERT_NE(vec1.data(), nullptr);
  ASSERT_EQ(vec1.front(), vec2.front());
  ASSERT_EQ(vec1.back(), vec2.front());
  for (std::size_t i = 0; i < size; ++i) {
    ASSERT_NO_THROW(vec1.at(i));
    ASSERT_NO_THROW(vec2.at(i));
  }
}

TEST(VectorTest, assignment_move) {
  std::size_t size = uid(gen);
  std::vector<not_safe> vec1(size);
  std::vector<not_safe> vec2;

  vec2 = std::move(vec1);

  ASSERT_EQ(vec2.size(), size);
  ASSERT_EQ(vec2.front(), not_safe());
  ASSERT_EQ(vec2.back(), not_safe());
}

TEST(VectorTest, assignment_move_self) {
  std::size_t size = uid(gen);
  memory::vector<not_safe> vec1(size);
  std::size_t cap = vec1.capacity();
  not_safe *ptr = vec1.data();

  vec1 = std::move(vec1);

  ASSERT_EQ(vec1.size(), size);
  ASSERT_EQ(vec1.capacity(), cap);
  ASSERT_EQ(vec1.data(), ptr);
  ASSERT_EQ(vec1.front(), not_safe());
  ASSERT_EQ(vec1.back(), not_safe());
}

TEST(VectorTest, assignment_move_with_alloc) {
  std::size_t size = uid(gen);
  memory::pool_allocator<safe> al1(size*sizeof(safe));
  memory::pool_allocator<safe> al2(size*sizeof(safe));
  memory::vector<safe, memory::pool_allocator<safe>> vec1(size, al1);
  std::size_t cap = vec1.capacity();
  memory::vector<safe, memory::pool_allocator<safe>> vec2(al2);

  vec2 = std::move(vec1);

  ASSERT_EQ(vec2.size(), size);
  ASSERT_EQ(vec2.capacity(), cap);
  ASSERT_NE(vec2.data(), nullptr);
  ASSERT_NE(vec2.get_allocator(), vec1.get_allocator());
  ASSERT_EQ(vec2.front(), safe());
  ASSERT_EQ(vec2.back(), safe());
}

TEST(VectorTest, assignment_move_with_alloc_no_realloc) {
  memory::pool_allocator<safe> al1(10*sizeof(safe));
  memory::pool_allocator<safe> al2(43*sizeof(safe));
  memory::vector<safe, memory::pool_allocator<safe>> vec1(10, al1);

  memory::vector<safe, memory::pool_allocator<safe>> vec2(33, al2);
  safe *ptr = vec2.data();

  vec2 = std::move(vec1);

  ASSERT_EQ(vec2.size(), 10);
  ASSERT_EQ(vec1.capacity(), 10);
  ASSERT_EQ(vec2.capacity(), 33);
  ASSERT_EQ(vec2.data(), ptr);
  ASSERT_NE(vec2.get_allocator(), vec1.get_allocator());
  ASSERT_EQ(vec2.front(), safe());
  ASSERT_EQ(vec2.back(), safe());
}

TEST(VectorTest, swap) {
  memory::vector<not_safe> vec1(10, not_safe("one"));

  memory::vector<not_safe> vec2(33, not_safe("two"));

  memory::vector<not_safe> exp1(vec2);
  memory::vector<not_safe> exp2(vec1);

  not_safe *ptr1 = vec1.data();
  not_safe *ptr2 = vec2.data();

  vec1.swap(vec2);

  ASSERT_EQ(vec1, exp1);
  ASSERT_EQ(vec2, exp2);
  ASSERT_EQ(vec1.data(), ptr2);
  ASSERT_EQ(vec2.data(), ptr1);
}

TEST(VectorTest, swap_with_propagation) {
  memory::pool_allocator<not_safe> al1(20*sizeof(not_safe));
  memory::pool_allocator<not_safe> al2(66*sizeof(not_safe));

  memory::vector<not_safe, memory::pool_allocator<not_safe>> vec1(10, not_safe("one"),
                                                            al1);
  memory::vector<not_safe, memory::pool_allocator<not_safe>> vec2(33, not_safe("two"),
                                                            al2);

  memory::vector<not_safe, memory::pool_allocator<not_safe>> exp1(vec2);
  memory::vector<not_safe, memory::pool_allocator<not_safe>> exp2(vec1);

  not_safe *ptr1 = vec1.data();
  not_safe *ptr2 = vec2.data();

  vec1.swap(vec2);

  ASSERT_EQ(al1.remaining(), 0);
  ASSERT_EQ(al2.remaining(), 0);
  ASSERT_EQ(vec1, exp1);
  ASSERT_EQ(vec2, exp2);
  ASSERT_EQ(vec1.data(), ptr2);
  ASSERT_EQ(vec2.data(), ptr1);
  ASSERT_EQ(vec1.get_allocator(), al2);
  ASSERT_EQ(vec2.get_allocator(), al1);
}

//==============================================================================
// cmp and random access
TEST(VectorTest, comparison_1) {
  std::size_t size = uid(gen);
  const memory::vector<not_safe> vec1(size, not_safe("equal"));
  const memory::vector<not_safe> vec2(size, not_safe("equal"));

  ASSERT_TRUE(vec1 == vec2);
  ASSERT_FALSE(vec1 != vec2);
  ASSERT_TRUE(vec2 == vec1);
  ASSERT_FALSE(vec2 != vec1);
}

TEST(VectorTest, comparison_2) {
  const memory::vector<not_safe> vec1(uid(gen), not_safe("not equal"));
  const memory::vector<not_safe> vec2(uid(gen), not_safe("equal"));

  ASSERT_FALSE(vec1 == vec2);
  ASSERT_TRUE(vec1 != vec2);
  ASSERT_FALSE(vec2 == vec1);
  ASSERT_TRUE(vec2 != vec1);
}

TEST(VectorTest, comparison_3) {
  const memory::vector<not_safe> vec1(uid(gen), not_safe("not equal"));
  const memory::vector<not_safe> vec2;

  ASSERT_FALSE(vec1 == vec2);
  ASSERT_TRUE(vec1 != vec2);
  ASSERT_FALSE(vec2 == vec1);
  ASSERT_TRUE(vec2 != vec1);
}

TEST(VectorTest, comparison_self) {
  const memory::vector<not_safe> vec(uid(gen), not_safe("not equal"));

  ASSERT_TRUE(vec == vec);
  ASSERT_FALSE(vec != vec);
}

TEST(VectorTest, comparison_empty) {
  const memory::vector<not_safe> vec1;
  const memory::vector<not_safe> vec2;

  ASSERT_TRUE(vec1 == vec2);
  ASSERT_FALSE(vec1 != vec2);
}

TEST(VectorTest, random_access) {
  memory::vector<std::size_t> vec(uid(gen));
  for (std::size_t i = 0; i < vec.size(); ++i) {
    ASSERT_EQ(vec[i], 0);
    vec[i] = i + 1;
  }
  for (std::size_t i = 0; i < vec.size(); ++i) {
    ASSERT_EQ(vec[i], i + 1);
  }
}

TEST(VectorTest, random_access_bounds) {
  memory::vector<std::size_t> vec(uid(gen));
  for (std::size_t i = 0; i < vec.size(); ++i) {
    ASSERT_EQ(vec.at(i), 0);
    vec.at(i) = i + 1;
  }
  for (std::size_t i = 0; i < vec.size(); ++i) {
    ASSERT_EQ(vec.at(i), i + 1);
  }
  ASSERT_THROW(vec.at(-1), std::out_of_range);
  ASSERT_THROW(vec.at(vec.size()), std::out_of_range);
}

TEST(VectorTest, front_back_access) {
  memory::vector<std::size_t> vec(uid(gen));
  for (std::size_t i = 0; i < vec.size(); ++i) {
    ASSERT_EQ(vec[i], 0);
    vec[i] = i + 1;
  }
  ASSERT_EQ(vec.front(), 1);
  ASSERT_EQ(vec.back(), vec.size());
}

TEST(VectorTest, data_access) {
  const memory::vector<std::size_t> vec(uid(gen), 666);
  const std::size_t *ptr = vec.data();
  for (std::size_t i = 0; i < vec.size(); ++i) ASSERT_EQ(ptr[i], 666);
}

TEST(VectorTest, iterators) {
  memory::vector<std::size_t> vec{0, 1, 2, 3, 4, 5};
  memory::vector<std::size_t>::iterator it = vec.begin();
  for (std::size_t i = 0; it != vec.end(); ++it, ++i) ASSERT_EQ(*it, vec[i]);
  it = vec.begin() + 3;
  *it = 10;
  ASSERT_EQ(vec[3], 10);
}

TEST(VectorTest, reverse_iterators) {
  memory::vector<std::size_t> vec{0, 1, 2, 3, 4, 5};
  memory::vector<std::size_t>::reverse_iterator it = vec.rbegin();
  for (std::size_t i = 5; it != vec.rend(); ++it, --i) ASSERT_EQ(*it, vec[i]);
  it = vec.rbegin() + 3;
  *it = 10;
  ASSERT_EQ(vec[2], 10);
}

//==============================================================================
// capacity
// TEST(VectorTest, assign_gt_not_safe_1) {
//   std::size_t size = 322;
//   memory::vector<not_safe> vec(10);

//   vec.assign(size, not_safe("tm"));

//   ASSERT_EQ(vec.size(), size);
//   for (const not_safe &ob : vec) {
//     ASSERT_EQ(ob, not_safe("tm"));
//   }
// }

// TEST(VectorTest, assign_gt_not_safe_2) {
//   std::size_t size = 13;
//   memory::vector<not_safe> vec(10);

//   vec.assign(size, not_safe("nm"));

//   ASSERT_EQ(vec.size(), size);
//   for (const not_safe &ob : vec) {
//     ASSERT_EQ(ob, not_safe("nm"));
//   }
// }

// TEST(VectorTest, assign_lt_not_safe_1) {
//   std::size_t size = 10;
//   memory::vector<not_safe> vec(322);
//   not_safe *ptr = vec.data();

//   vec.assign(size, not_safe("tm"));

//   ASSERT_EQ(vec.size(), size);
//   ASSERT_EQ(vec.data(), ptr);
//   for (const not_safe &ob : vec) {
//     ASSERT_EQ(ob, not_safe("tm"));
//     ASSERT_EQ(ob.birth, constructed::kDef);
//   }
// }

// TEST(VectorTest, assign_lt_not_safe_2) {
//   std::size_t size = 10;
//   memory::vector<not_safe> vec(13);
//   not_safe *ptr = vec.data();

//   vec.assign(size, not_safe("nm"));

//   ASSERT_EQ(vec.size(), size);
//   ASSERT_EQ(vec.data(), ptr);

//   for (const not_safe &ob : vec) {
//     ASSERT_EQ(ob, not_safe("nm"));
//   }
// }

// TEST(VectorTest, assign_gt_no_realloc) {
//   memory::vector<safe> vec(10);

//   vec.assign(322, safe("tm"));
//   safe *ptr = vec.data();
//   vec.assign(10, safe("tm"));
//   vec.assign(16, safe("tm"));

//   ASSERT_EQ(vec.size(), 16);
//   ASSERT_EQ(vec.capacity(), 322);
//   ASSERT_EQ(vec.data(), ptr);
//   for (const safe &ob : vec) {
//     ASSERT_EQ(ob, safe("tm"));
//   }
// }

// TEST(VectorTest, assign_throwing) {
//   throwing::count = 0;
//   std::size_t size = uid(gen);
//   memory::vector<throwing> vec(size);

//   ASSERT_ANY_THROW(vec.assign(7, throwing()));

//   ASSERT_EQ(vec.size(), size);
//   for (const throwing &ob : vec) {
//     ASSERT_EQ(ob, throwing());
//   }
// }

// TEST(VectorTest, assign_list_safe) {
//   memory::vector<safe> vec(uid(gen));

//   vec.assign({safe(), safe(), safe(), safe(), safe()});

//   for (const safe &ob : vec) {
//     ASSERT_EQ(ob, safe());
//   }
// }

// TEST(VectorTest, assign_random) {
//   for (std::size_t i = 0; i < loop; ++i) {
//     memory::vector<safe> vec(uid(gen));

//     vec.assign(uid(gen), safe("ass"));

//     for (const safe &ob : vec) {
//       ASSERT_EQ(ob, safe("ass"));
//     }
//   }
// }

// TEST(VectorTest, assign_list_not_safe) {
//   memory::vector<not_safe> vec(uid(gen));

//   vec.assign({not_safe(), not_safe(), not_safe(), not_safe(), not_safe()});

//   for (const not_safe &ob : vec) {
//     ASSERT_EQ(ob, not_safe());
//   }
// }

// TEST(VectorTest, assign_list_throwing) {
//   throwing::count = 0;
//   std::size_t size = uid(gen);
//   memory::vector<throwing> vec(size);

//   ASSERT_ANY_THROW(vec.assign({throwing("not default"), throwing("not default"),
//                                throwing("not default"), throwing("not default"),
//                                throwing("not default")}));

//   ASSERT_EQ(vec.size(), size);
//   for (std::size_t i = 0; i < size; ++i) {
//     ASSERT_NO_THROW(vec.at(i));
//   }
// }

// TEST(VectorTest, assign_invalid_count) {
//   memory::vector<safe> vec(uid(gen));

//   ASSERT_THROW(vec.assign(-uid(gen), safe()), std::length_error);
// }

// TEST(VectorTest, assign_big_count) {
//   memory::vector<safe> vec(uid(gen));
//   std::allocator<safe> al;
//   std::size_t max = std::allocator_traits<std::allocator<safe>>::max_size(al);
//   ASSERT_THROW(vec.assign(max + 1, safe()), std::length_error);
// }

// TEST(VectorTest, reserve_expand_safe) {
//   std::size_t size = 10;
//   std::size_t re_cap = 40;

//   memory::vector<safe> vec(size);

//   vec.reserve(re_cap);
//   ASSERT_EQ(size, vec.size());
//   ASSERT_EQ(re_cap, vec.capacity());
//   for (const safe &ob : vec) {
//     ASSERT_EQ(ob, safe());
//     ASSERT_EQ(ob.birth, constructed::kMove);
//   }
// }

// TEST(VectorTest, reserve_expand_not_safe) {
//   std::size_t size = 10;
//   std::size_t re_cap = 40;

//   memory::vector<not_safe> vec(size);

//   vec.reserve(re_cap);
//   ASSERT_EQ(size, vec.size());
//   ASSERT_EQ(re_cap, vec.capacity());
//   for (const not_safe &ob : vec) {
//     ASSERT_EQ(ob, not_safe());
//     ASSERT_EQ(ob.birth, constructed::kCopy);
//   }
// }

// TEST(VectorTest, reserve_expand_throwing) {
//   throwing::count = 0;
//   std::size_t size = 5;
//   memory::vector<throwing> vec(size);

//   std::size_t old_cap = vec.capacity();

//   ASSERT_ANY_THROW(vec.reserve(17));

//   ASSERT_EQ(size, vec.size());
//   ASSERT_EQ(old_cap, vec.capacity());
//   ASSERT_EQ(vec.back().birth, constructed::kDef);
//   ASSERT_EQ(vec.front().birth, constructed::kDef);
// }

// TEST(VectorTest, reserve_shrink) {
//   throwing::count = 0;

//   std::size_t size = 10;
//   memory::vector<throwing> vec(size);
//   throwing *ptr = vec.data();
//   vec.reserve(7);

//   ASSERT_EQ(vec.size(), size);
//   ASSERT_EQ(vec.capacity(), size);
//   ASSERT_EQ(vec.data(), ptr);
//   ASSERT_EQ(vec.back().birth, constructed::kDef);
//   ASSERT_EQ(vec.front().birth, constructed::kDef);
// }

// TEST(VectorTest, reserve_invalid) {
//   memory::vector<safe> vec1(uid(gen));
//   ASSERT_THROW(vec1.reserve(-1), std::length_error);

//   memory::vector<not_safe> vec2(uid(gen));
//   ASSERT_THROW(vec2.reserve(-1), std::length_error);

//   memory::vector<throwing> vec3(uid(gen));
//   ASSERT_THROW(vec3.reserve(-1), std::length_error);
// }

// TEST(VectorTest, reserve_random) {
//   for (std::size_t i = 0; i < loop; ++i) {
//     std::size_t size = uid(gen);
//     std::size_t re_cap = uid(gen);

//     memory::vector<not_safe> vec(size, not_safe("rand"));

//     vec.reserve(re_cap);

//     ASSERT_EQ(size, vec.size());
//     ASSERT_LE(re_cap, vec.capacity());
//     for (const not_safe &ob : vec) {
//       ASSERT_EQ(ob, not_safe("rand"));
//     }
//   }
// }

// TEST(VectorTest, stf_safe) {
//   for (std::size_t i = 0; i < loop; ++i) {
//     memory::vector<safe> vec(uid(gen));

//     vec.reserve(vec.size() + uid(gen));
//     vec.shrink_to_fit();
//     ASSERT_EQ(vec.size(), vec.capacity());
//     ASSERT_EQ(vec.back().birth, constructed::kMove);
//     ASSERT_EQ(vec.front().birth, constructed::kMove);
//   }
// }

// TEST(VectorTest, stf_not_safe) {
//   for (std::size_t i = 0; i < loop; ++i) {
//     memory::vector<not_safe> vec(uid(gen));

//     vec.reserve(vec.size() + uid(gen));
//     vec.shrink_to_fit();
//     ASSERT_EQ(vec.size(), vec.capacity());
//     ASSERT_EQ(vec.back().birth, constructed::kCopy);
//     ASSERT_EQ(vec.front().birth, constructed::kCopy);
//   }
// }

// TEST(VectorTest, stf_throwing) {
//   throwing::count = 0;
//   memory::vector<throwing> vec(4);

//   vec.reserve(10);
//   std::size_t old_cap = vec.capacity();
//   ASSERT_ANY_THROW(vec.shrink_to_fit());
//   ASSERT_EQ(vec.capacity(), old_cap);
// }

// TEST(VectorTest, clear) {
//   for (std::size_t i = 0; i < loop; ++i) {
//     memory::vector<not_safe> vec(uid(gen), not_safe("dirty"));

//     vec.clear();
//     ASSERT_EQ(vec.size(), 0);
//   }
// }

// TEST(VectorTest, resize_expand_not_safe) {
//   std::size_t size = 10;
//   std::size_t re_size = 15;
//   memory::vector<not_safe> vec(size, not_safe("default"));

//   vec.resize(re_size, not_safe("appended"));

//   ASSERT_EQ(re_size, vec.size());
//   for (std::size_t j = 0; j < size; ++j) {
//     ASSERT_EQ(vec[j], not_safe("default"));
//     ASSERT_EQ(vec[j].birth, constructed::kCopy);
//   }
//   for (std::size_t j = size; j < re_size; ++j) {
//     ASSERT_EQ(vec[j], not_safe("appended"));
//     ASSERT_EQ(vec[j].birth, constructed::kCopy);
//   }
// }

// TEST(VectorTest, resize_expand_safe) {
//   std::size_t size = 10;
//   std::size_t re_size = 77;
//   memory::vector<safe> vec(size, safe("default"));

//   vec.resize(re_size, safe("appended"));

//   ASSERT_EQ(re_size, vec.size());
//   for (std::size_t j = 0; j < size; ++j) {
//     ASSERT_EQ(vec[j], safe("default"));
//     ASSERT_EQ(vec[j].birth, constructed::kMove);
//   }
//   for (std::size_t j = size; j < re_size; ++j) {
//     ASSERT_EQ(vec[j], safe("appended"));
//     ASSERT_EQ(vec[j].birth, constructed::kCopy);
//   }
// }

// TEST(VectorTest, resize_expand_throwing) {
//   throwing::count = 0;
//   std::size_t size = 3;
//   memory::vector<throwing> vec(size);
//   ASSERT_ANY_THROW(vec.resize(10, throwing("appended")));
//   ASSERT_EQ(size, vec.size());
//   for (const throwing &ob : vec) {
//     ASSERT_EQ(ob, throwing());
//     ASSERT_EQ(ob.birth, constructed::kDef);
//   }

//   throwing::count = 3;
//   ASSERT_ANY_THROW(vec.resize(10, throwing("appended")));
//   ASSERT_EQ(size, vec.size());
//   for (const throwing &ob : vec) {
//     ASSERT_EQ(ob, throwing());
//     ASSERT_EQ(ob.birth, constructed::kDef);
//   }
// }

// TEST(VectorTest, resize_shrink) {
//   std::size_t size = 77;
//   std::size_t re_size = 23;
//   memory::vector<not_safe> vec(size);
//   std::size_t old_cap = vec.capacity();
//   not_safe *ptr = vec.data();

//   vec.resize(re_size);

//   ASSERT_EQ(re_size, vec.size());
//   ASSERT_EQ(old_cap, vec.capacity());
//   ASSERT_EQ(ptr, vec.data());

//   for (const not_safe &ob : vec) {
//     ASSERT_EQ(ob, not_safe());
//     ASSERT_EQ(ob.birth, constructed::kDef);
//   }
// }

// TEST(VectorTest, resize_shrink_throwing) {
//   throwing::count = 0;
//   std::size_t re_size = 5;
//   memory::vector<throwing> vec(17);
//   std::size_t old_cap = vec.capacity();

//   ASSERT_NO_THROW(vec.resize(re_size));
//   ASSERT_EQ(re_size, vec.size());
//   ASSERT_EQ(old_cap, vec.capacity());

//   for (const throwing &ob : vec) {
//     ASSERT_EQ(ob, throwing());
//     ASSERT_EQ(ob.birth, constructed::kDef);
//   }
// }

// TEST(VectorTest, resize_expand_no_realloc) {
//   memory::vector<safe> vec(40);
//   safe *ptr = vec.data();

//   vec.resize(10, safe("appended"));
//   vec.resize(30, safe("appended"));

//   ASSERT_EQ(vec.size(), 30);
//   ASSERT_EQ(vec.capacity(), 40);
//   ASSERT_EQ(vec.data(), ptr);

//   for (std::size_t j = 0; j < 10; ++j) {
//     ASSERT_EQ(vec[j], safe("default"));
//     ASSERT_EQ(vec[j].birth, constructed::kDef);
//   }
//   for (std::size_t j = 10; j < 30; ++j) {
//     ASSERT_EQ(vec[j], safe("appended"));
//   }
// }

// TEST(VectorTest, resize_zero) {
//   memory::vector<not_safe> vec(uid(gen));
//   std::size_t old_cap = vec.capacity();
//   not_safe *ptr = vec.data();
//   vec.resize(0);
//   ASSERT_EQ(0, vec.size());
//   ASSERT_EQ(old_cap, vec.capacity());
//   ASSERT_EQ(ptr, vec.data());
// }

// TEST(VectorTest, resize_invalid) {
//   memory::vector<not_safe> vec(uid(gen));
//   ASSERT_THROW(vec.resize(-1, not_safe("dolbaeb")), std::length_error);
// }

// TEST(VectorTest, resize_random) {
//   for (std::size_t i = 0; i < loop; ++i) {
//     std::size_t size = uid(gen);
//     std::size_t re_size = uid(gen);
//     memory::vector<not_safe> vec(size, not_safe("default"));

//     vec.resize(re_size, not_safe("appended"));

//     ASSERT_EQ(re_size, vec.size());
//     for (std::size_t j = 0; j < size && j < re_size; ++j) {
//       ASSERT_EQ(vec[j], not_safe("default"));
//     }
//     for (std::size_t j = std::min(size, re_size); j < re_size; ++j) {
//       ASSERT_EQ(vec[j], not_safe("appended"));
//     }
//   }
// }
// //==============================================================================
// // modifiers

// TEST(VectorTest, insert_continious) {
//   not_safe insert_val("inserted");
//   memory::vector<not_safe> vec(60);
//   std::size_t insert = 0;
//   for (std::size_t i = 0; i < loop; ++i, insert += 4) {
//     auto pos = vec.insert(vec.begin() + insert, insert_val);
//     ASSERT_EQ(pos, vec.begin() + insert);
//     ASSERT_EQ(*pos, insert_val);
//   }
// }

// TEST(VectorTest, insert_not_safe) {
//   memory::vector<not_safe> vec(60);
//   std::size_t insert = 8;
//   auto pos = vec.insert(vec.begin() + insert, not_safe("inserted"));
//   ASSERT_EQ(pos, vec.begin() + insert);
//   ASSERT_EQ(*pos, not_safe("inserted"));
//   for (auto i = vec.begin(); i != pos; ++i) {
//     ASSERT_NE(*i, not_safe("inserted"));
//     ASSERT_EQ(i->birth, constructed::kCopy);
//   }
//   for (auto i = pos + 1; i != vec.end(); ++i) {
//     ASSERT_NE(*i, not_safe("inserted"));
//     ASSERT_EQ(i->birth, constructed::kCopy);
//   }
// }

// TEST(VectorTest, insert_safe) {
//   memory::vector<safe> vec(60);
//   std::size_t insert = 47;
//   auto pos = vec.insert(vec.begin() + insert, safe("inserted"));
//   ASSERT_EQ(pos, vec.begin() + insert);
//   ASSERT_EQ(*pos, safe("inserted"));
//   for (auto i = vec.begin(); i != pos; ++i) {
//     ASSERT_NE(*i, safe("inserted"));
//     ASSERT_EQ(i->birth, constructed::kMove);
//   }
//   for (auto i = pos + 1; i != vec.end(); ++i) {
//     ASSERT_NE(*i, safe("inserted"));
//     ASSERT_EQ(i->birth, constructed::kMove);
//   }
// }

// TEST(VectorTest, insert_empty) {
//   safe insert_val("inserted");
//   memory::vector<safe> vec;
//   auto insert_pos = vec.insert(vec.begin(), insert_val);
//   ASSERT_EQ(vec.size(), 1);
//   ASSERT_EQ(vec.capacity(), 1);
//   ASSERT_NE(vec.data(), nullptr);
//   ASSERT_EQ(insert_pos, vec.begin());
//   ASSERT_EQ(*insert_pos, insert_val);
// }

// TEST(VectorTest, insert_throwing) {
//   throwing::count = 0;
//   std::size_t size = 4;
//   throwing insert_val("inserted");
//   memory::vector<throwing> vec(size);
//   ASSERT_ANY_THROW(vec.insert(vec.begin(), throwing("face")));
//   ASSERT_EQ(vec.size(), size);
//   for (auto i = 0; i < size; ++i) {
//     ASSERT_NO_THROW(vec.at(i));
//   }

//   throwing::count = 4;
//   ASSERT_ANY_THROW(vec.insert(vec.begin(), throwing("in your")));
//   ASSERT_EQ(vec.size(), size);
//   for (auto i = 0; i < size; ++i) {
//     ASSERT_NO_THROW(vec.at(i));
//   }
// }

// TEST(VectorTest, insert_safe_counted) {
//   std::size_t insert = 47;
//   std::size_t size = 60;
//   std::size_t count = 8;
//   memory::vector<safe> vec(size);

//   auto pos = vec.insert(vec.begin() + insert, count, safe("inserted"));

//   ASSERT_EQ(vec.size(), size + count);
//   ASSERT_EQ(vec.capacity(), size * 2);
//   ASSERT_EQ(pos, vec.begin() + insert);
//   ASSERT_EQ(*pos, safe("inserted"));
//   auto i = vec.begin();

//   for (; i != pos; ++i) {
//     ASSERT_NE(*i, safe("inserted"));
//     ASSERT_EQ(i->birth, constructed::kMove);
//   }
//   for (; i != pos + count; ++i) {
//     ASSERT_EQ(*i, safe("inserted"));
//   }
//   for (; i != vec.end(); ++i) {
//     ASSERT_NE(*i, safe("inserted"));
//     ASSERT_EQ(i->birth, constructed::kMove);
//   }
// }

// TEST(VectorTest, insert_not_safe_counted) {
//   std::size_t insert = 8;
//   std::size_t size = 60;
//   std::size_t count = 67;
//   memory::vector<not_safe> vec(size);

//   auto pos = vec.insert(vec.begin() + insert, count, not_safe("inserted"));

//   ASSERT_EQ(vec.size(), size + count);
//   ASSERT_EQ(vec.capacity(), size + count);
//   ASSERT_EQ(pos, vec.begin() + insert);
//   ASSERT_EQ(*pos, not_safe("inserted"));
//   auto i = vec.begin();

//   for (; i != pos; ++i) {
//     ASSERT_NE(*i, not_safe("inserted"));
//     ASSERT_EQ(i->birth, constructed::kCopy);
//   }
//   for (; i != pos + count; ++i) {
//     ASSERT_EQ(*i, not_safe("inserted"));
//   }
//   for (; i != vec.end(); ++i) {
//     ASSERT_NE(*i, not_safe("inserted"));
//     ASSERT_EQ(i->birth, constructed::kCopy);
//   }
// }

// TEST(VectorTest, insert_counted_no_realloc) {
//   memory::vector<safe> vec(77);
//   safe *ptr = vec.data();
//   vec.resize(25);

//   std::size_t insert = 8;
//   std::size_t count = 9;

//   auto pos = vec.insert(vec.begin() + insert, count, safe("inserted"));

//   ASSERT_EQ(vec.size(), 25 + count);
//   ASSERT_EQ(vec.capacity(), 77);
//   ASSERT_EQ(vec.data(), ptr);
//   ASSERT_EQ(pos, vec.begin() + insert);
//   ASSERT_EQ(*pos, safe("inserted"));
//   auto i = vec.begin();

//   for (; i != pos; ++i) {
//     ASSERT_NE(*i, safe("inserted"));
//     ASSERT_EQ(i->birth, constructed::kDef);
//   }
//   for (; i != pos + count; ++i) {
//     ASSERT_EQ(*i, safe("inserted"));
//   }
// }

// TEST(VectorTest, insert_counted_throwing) {
//   throwing::count = 0;
//   std::size_t size = 4;
//   throwing insert_val("inserted");
//   memory::vector<throwing> vec(size);
//   throwing *ptr = vec.data();

//   ASSERT_ANY_THROW(vec.insert(vec.begin(), 4, throwing("face")));

//   ASSERT_EQ(vec.size(), size);
//   ASSERT_EQ(vec.capacity(), size);
//   ASSERT_EQ(vec.data(), ptr);
//   for (auto i = 0; i < size; ++i) {
//     ASSERT_NO_THROW(vec.at(i));
//   }

//   throwing::count = 4;

//   ASSERT_ANY_THROW(vec.insert(vec.begin(), 4, throwing("in your")));

//   ASSERT_EQ(vec.size(), size);
//   ASSERT_EQ(vec.capacity(), size);
//   ASSERT_EQ(vec.data(), ptr);
//   for (auto i = 0; i < size; ++i) {
//     ASSERT_NO_THROW(vec.at(i));
//   }
// }

// TEST(VectorTest, insert_range_safe) {
//   std::size_t size = 60;
//   memory::vector<safe> vec(size);
//   std::vector<safe> range(size, safe("inserted"));

//   std::size_t start = 8;
//   std::size_t finish = 24;
//   std::size_t count = finish - start;

//   std::size_t insert = 16;

//   auto pos = vec.insert(vec.begin() + insert, range.begin() + start,
//                         range.begin() + finish);

//   ASSERT_EQ(vec.size(), size + count);
//   ASSERT_EQ(pos, vec.begin() + insert);

//   auto i = vec.begin();

//   for (; i != pos; ++i) {
//     ASSERT_NE(*i, safe("inserted"));
//     ASSERT_EQ(i->birth, constructed::kMove);
//   }
//   for (; i != pos + count; ++i) {
//     ASSERT_EQ(*i, safe("inserted"));
//   }
//   for (; i != vec.end(); ++i) {
//     ASSERT_NE(*i, safe("inserted"));
//     ASSERT_EQ(i->birth, constructed::kMove);
//   }
// }

// TEST(VectorTest, insert_range_not_safe) {
//   std::size_t size = 60;
//   memory::vector<not_safe> vec(size);
//   std::vector<not_safe> range(size, not_safe("inserted"));

//   std::size_t start = 8;
//   std::size_t finish = 24;
//   std::size_t count = finish - start;

//   std::size_t insert = 16;

//   auto pos = vec.insert(vec.begin() + insert, range.begin() + start,
//                         range.begin() + finish);

//   ASSERT_EQ(vec.size(), size + count);
//   ASSERT_EQ(pos, vec.begin() + insert);

//   auto i = vec.begin();

//   for (; i != pos; ++i) {
//     ASSERT_NE(*i, not_safe("inserted"));
//     ASSERT_EQ(i->birth, constructed::kCopy);
//   }
//   for (; i != pos + count; ++i) {
//     ASSERT_EQ(*i, not_safe("inserted"));
//   }
//   for (; i != vec.end(); ++i) {
//     ASSERT_NE(*i, not_safe("inserted"));
//     ASSERT_EQ(i->birth, constructed::kCopy);
//   }
// }

// TEST(VectorTest, insert_range_no_realloc) {
//   std::size_t size = 60;
//   memory::vector<safe> vec(size);
//   std::vector<safe> range(size, safe("inserted"));

//   vec.resize(10);

//   std::size_t start = 8;
//   std::size_t finish = 24;
//   std::size_t count = finish - start;

//   std::size_t insert = 6;

//   auto pos = vec.insert(vec.begin() + insert, range.begin() + start,
//                         range.begin() + finish);

//   ASSERT_EQ(vec.size(), 10 + count);
//   ASSERT_EQ(pos, vec.begin() + insert);

//   auto i = vec.begin();

//   for (; i != pos; ++i) {
//     ASSERT_NE(*i, not_safe("inserted"));
//     ASSERT_EQ(i->birth, constructed::kDef);
//   }
//   for (; i != pos + count; ++i) {
//     ASSERT_EQ(*i, not_safe("inserted"));
//   }
// }

// TEST(VectorTest, insert_range_throwing) {
//   throwing::count = 0;
//   std::size_t size = 4;
//   memory::vector<throwing> vec(size);
//   std::vector<throwing> range(10);
//   std::size_t start = 2;
//   std::size_t finish = start + 5;
//   ASSERT_ANY_THROW(
//       vec.insert(vec.begin(), range.begin() + start, range.begin() + finish));
//   ASSERT_EQ(vec.size(), size);
//   for (auto i = 0; i < size; ++i) {
//     ASSERT_NO_THROW(vec.at(i));
//   }

//   throwing::count = 0;
//   ASSERT_ANY_THROW(
//       vec.insert(vec.begin(), range.begin() + start, range.begin() + finish));
//   ASSERT_EQ(vec.size(), size);
//   for (auto i = 0; i < size; ++i) {
//     ASSERT_NO_THROW(vec.at(i));
//   }
// }

// TEST(VectorTest, insert_list_safe) {
//   for (std::size_t i = 0; i < loop; ++i) {
//     std::size_t size = uid(gen) + 3;
//     memory::vector<safe> vec(size);

//     std::size_t insert = 2;

//     auto pos =
//         vec.insert(vec.begin() + insert,
//                    {safe("inserted"), safe("inserted"), safe("inserted")});
//     ASSERT_EQ(vec.size(), size + 3);
//     ASSERT_EQ(pos, vec.begin() + insert);
//     auto it = vec.begin();

//     for (; it != pos; ++it) {
//       ASSERT_NE(*it, safe("inserted"));
//       ASSERT_EQ(it->birth, constructed::kMove);
//     }
//     for (; it != pos + 3; ++it) {
//       ASSERT_EQ(*it, safe("inserted"));
//     }
//     for (; it != vec.end(); ++it) {
//       ASSERT_NE(*it, safe("inserted"));
//       ASSERT_EQ(it->birth, constructed::kMove);
//     }
//   }
// }

// TEST(VectorTest, insert_list_not_safe) {
//   for (std::size_t i = 0; i < loop; ++i) {
//     std::size_t size = uid(gen);
//     memory::vector<not_safe> vec(size);
//     std::uniform_int_distribution<std::size_t> uid_vec(1, size);

//     std::size_t insert = uid_vec(gen);

//     auto pos = vec.insert(
//         vec.begin() + insert,
//         {not_safe("inserted"), not_safe("inserted"), not_safe("inserted")});
//     ASSERT_EQ(vec.size(), size + 3);
//     ASSERT_EQ(pos, vec.begin() + insert);
//     auto it = vec.begin();

//     for (; it != pos; ++it) {
//       ASSERT_NE(*it, not_safe("inserted"));
//       ASSERT_EQ(it->birth, constructed::kCopy);
//     }
//     for (; it != pos + 3; ++it) {
//       ASSERT_EQ(*it, not_safe("inserted"));
//     }
//     for (; it != vec.end(); ++it) {
//       ASSERT_NE(*it, not_safe("inserted"));
//       ASSERT_EQ(it->birth, constructed::kCopy);
//     }
//   }
// }

// TEST(VectorTest, erase_not_safe) {
//   for (std::size_t i = 0; i < loop; ++i) {
//     memory::vector<not_safe> vec(uid(gen));
//     std::uniform_int_distribution<std::size_t> uid_vec(0, vec.size() - 1);
//     auto pos = vec.begin() + uid_vec(gen);
//     pos = vec.insert(pos, not_safe("inserted"));
//     vec.erase(pos);
//     for (const safe &ob : vec) {
//       ASSERT_NE(ob, not_safe("inserted"));
//       ASSERT_EQ(ob.birth, constructed::kCopy);
//     }
//   }
// }

// TEST(VectorTest, erase_safe) {
//   for (std::size_t i = 0; i < loop; ++i) {
//     memory::vector<safe> vec(uid(gen));
//     std::uniform_int_distribution<std::size_t> uid_vec(0, vec.size() - 1);
//     auto pos = vec.begin() + uid_vec(gen);
//     pos = vec.insert(pos, safe("inserted"));
//     vec.erase(pos);
//     for (const safe &ob : vec) {
//       ASSERT_NE(ob, safe("inserted"));
//       ASSERT_NE(ob.birth, constructed::kCopy);
//     }
//   }
// }

// TEST(VectorTest, erase_throwing) {
//   throwing::count = 0;
//   std::size_t size = uid(gen) + 10;
//   memory::vector<throwing> vec(size);
//   ASSERT_ANY_THROW(vec.erase(vec.begin() + 7));
//   ASSERT_EQ(vec.size(), size);
//   for (auto i = 0; i < size; ++i) {
//     ASSERT_NO_THROW(vec.at(i));
//   }

//   ASSERT_ANY_THROW(vec.erase(vec.begin() + 2));
//   ASSERT_EQ(vec.size(), size);
//   for (auto i = 0; i < size; ++i) {
//     ASSERT_NO_THROW(vec.at(i));
//   }
// }

// TEST(VectorTest, erase_to_empty) {
//   std::size_t size = uid(gen);
//   memory::vector<safe> vec(size);
//   for (std::size_t i = 0; i < size; ++i) {
//     vec.erase(vec.begin());
//   }
//   ASSERT_EQ(vec.size(), 0);
// }

// TEST(VectorTest, erase_last) {
//   std::size_t size = uid(gen);
//   memory::vector<safe> vec(size);
//   auto pos = vec.end() - 1;
//   pos = vec.erase(pos);
//   ASSERT_EQ(pos, vec.end());
//   ASSERT_EQ(vec.size(), size - 1);
// }

// TEST(VectorTest, erase_range_safe) {
//   memory::vector<safe> vec(10);
//   std::size_t start = 2;
//   std::size_t finish = 6;
//   std::size_t n_size = vec.size() - (finish - start);
//   std::size_t cap = vec.capacity();

//   auto it = vec.erase(vec.begin() + start, vec.begin() + finish);

//   ASSERT_EQ(n_size, vec.size());
//   ASSERT_EQ(vec.capacity(), cap);
//   ASSERT_EQ(it, vec.begin() + start);
//   for (const safe &ob : vec) {
//     ASSERT_NE(ob.birth, constructed::kCopy);
//   }
// }

// TEST(VectorTest, erase_range_not_safe) {
//   memory::vector<not_safe> vec{not_safe("no erase"), not_safe("no erase"),
//                              not_safe("erase"),    not_safe("erase"),
//                              not_safe("erase"),    not_safe("erase"),
//                              not_safe("no erase"), not_safe("no erase")};
//   std::size_t start = 2;
//   std::size_t finish = 6;
//   std::size_t n_size = vec.size() - (finish - start);
//   std::size_t cap = vec.capacity();

//   auto it = vec.erase(vec.begin() + start, vec.begin() + finish);

//   ASSERT_EQ(n_size, vec.size());
//   ASSERT_EQ(vec.capacity(), cap);
//   ASSERT_EQ(it, vec.begin() + start);
//   for (const not_safe &ob : vec) {
//     ASSERT_NE(ob, not_safe("erase"));
//   }
// }

// TEST(VectorTest, erase_range_throwing) {
//   memory::vector<throwing> vec(10);
//   std::size_t start = 2;
//   std::size_t finish = 4;
//   std::size_t cap = vec.capacity();

//   ASSERT_ANY_THROW(vec.erase(vec.begin() + start, vec.begin() + finish));

//   ASSERT_EQ(vec.size(), 10);
//   ASSERT_EQ(vec.capacity(), cap);
// }

// TEST(VectorTest, erase_range_end) {
//   for (std::size_t i = 0; i < loop; ++i) {
//     memory::vector<safe> vec(uid(gen), safe("default"));
//     std::uniform_int_distribution<std::size_t> uid_vec(0, vec.size());
//     auto pos = vec.begin() + uid_vec(gen);
//     std::size_t n_size = vec.size() - (vec.end() - pos);

//     pos = vec.erase(pos, vec.end());

//     ASSERT_EQ(pos, vec.end());
//     ASSERT_EQ(n_size, vec.size());
//   }
// }

// TEST(VectorTest, erase_range_entire) {
//   memory::vector<safe> vec(uid(gen));
//   vec.erase(vec.begin(), vec.end());
//   ASSERT_EQ(vec.size(), 0);
// }

// TEST(VectorTest, erase_range_empty) {
//   for (std::size_t i = 0; i < loop; ++i) {
//     std::size_t size = uid(gen) + 1;
//     memory::vector<safe> vec(size);
//     auto pos = vec.erase(vec.begin() + 1, vec.begin() + 1);
//     ASSERT_EQ(pos, vec.begin() + 1);
//     ASSERT_EQ(vec.size(), size);
//   }
// }

// TEST(VectorTest, push_back_not_safe) {
//   not_safe push_value("pushed");
//   memory::vector<not_safe> vec(uid(gen));

//   vec.push_back(push_value);

//   ASSERT_EQ(vec.back(), push_value);
//   for (std::size_t i = 0; i < vec.size(); ++i) {
//     ASSERT_EQ(vec[i].birth, constructed::kCopy);
//   }
// }

// TEST(VectorTest, push_back_safe) {
//   memory::vector<safe> vec(uid(gen));

//   vec.push_back(safe("pushed"));

//   ASSERT_EQ(vec.back(), safe("pushed"));
//   for (std::size_t i = 0; i < vec.size(); ++i) {
//     ASSERT_EQ(vec[i].birth, constructed::kMove);
//   }
// }

// TEST(VectorTest, push_back_continious) {
//   memory::vector<safe> vec(uid(gen));

//   for (std::size_t i = 0; i < loop; ++i) {
//     vec.push_back(safe("pushed"));
//     ASSERT_EQ(vec.back(), safe("pushed"));
//     ASSERT_EQ(vec.back().birth, constructed::kMove);
//   }
// }

// TEST(VectorTest, push_back_throwing) {
//   throwing::count = 0;
//   std::size_t size = 4;
//   throwing push_value("pushed");
//   memory::vector<throwing> vec(size);
//   ASSERT_ANY_THROW(vec.push_back(push_value));
//   ASSERT_EQ(vec.size(), size);
//   for (auto i = 0; i < size; ++i) {
//     ASSERT_NO_THROW(vec.at(i));
//   }

//   throwing::count = 4;
//   ASSERT_ANY_THROW(vec.push_back(push_value));
//   ASSERT_EQ(vec.size(), size);
//   for (auto i = 0; i < size; ++i) {
//     ASSERT_NO_THROW(vec.at(i));
//   }
// }

// TEST(VectorTest, pop_back_same_vector) {
//   memory::vector<safe> vec(77, safe("default"));
//   for (std::size_t i = 0; i < 45; ++i) {
//     vec.pop_back();
//   }
//   ASSERT_EQ(vec.size(), 77 - 45);
// }

// TEST(VectorTest, pop_back_to_empty) {
//   std::size_t size = uid(gen);
//   memory::vector<safe> vec(size);
//   for (std::size_t i = 0; i < size; ++i) {
//     vec.pop_back();
//   }
//   ASSERT_EQ(vec.size(), 0);
// }

// TEST(VectorTest, emplace_not_safe) {
//   memory::vector<not_safe> vec(54);
//   not_safe value("inserted");
//   std::size_t insert = 31;

//   auto pos = vec.emplace(vec.begin() + insert, value);

//   ASSERT_EQ(pos, vec.begin() + insert);
//   ASSERT_EQ(*pos, not_safe("inserted"));
//   ASSERT_EQ(pos->birth, constructed::kCopy);

//   for (auto i = vec.begin(); i != pos; ++i) {
//     ASSERT_NE(*i, not_safe("inserted"));
//     ASSERT_EQ(i->birth, constructed::kCopy);
//   }
//   for (auto i = pos + 1; i != vec.end(); ++i) {
//     ASSERT_NE(*i, not_safe("inserted"));
//     ASSERT_EQ(i->birth, constructed::kCopy);
//   }
// }

// TEST(VectorTest, emplace_safe) {
//   memory::vector<safe> vec(54);
//   std::size_t insert = 31;

//   auto pos = vec.emplace(vec.begin() + insert, safe("inserted"));

//   ASSERT_EQ(pos, vec.begin() + insert);
//   ASSERT_EQ(*pos, safe("inserted"));
//   ASSERT_EQ(pos->birth, constructed::kMove);

//   for (auto i = vec.begin(); i != pos; ++i) {
//     ASSERT_NE(*i, safe("inserted"));
//     ASSERT_EQ(i->birth, constructed::kMove);
//   }
//   for (auto i = pos + 1; i != vec.end(); ++i) {
//     ASSERT_NE(*i, safe("inserted"));
//     ASSERT_EQ(i->birth, constructed::kMove);
//   }
// }

// TEST(VectorTest, emplace_no_realloc) {
//   memory::vector<safe> vec(54);
//   safe *ptr = vec.data();
//   vec.resize(50);

//   std::size_t insert = 31;

//   auto pos = vec.emplace(vec.begin() + insert, "inserted");

//   ASSERT_EQ(vec.data(), ptr);
//   ASSERT_EQ(pos, vec.begin() + insert);
//   ASSERT_EQ(*pos, safe("inserted"));
//   ASSERT_EQ(pos->birth, constructed::kDef);

//   for (auto i = vec.begin(); i != pos; ++i) {
//     ASSERT_NE(*i, safe("inserted"));
//     ASSERT_EQ(i->birth, constructed::kDef);
//   }
//   ASSERT_EQ(vec.back().birth, constructed::kMove);
// }

// TEST(VectorTest, emplace_no_def) {
//   memory::vector<no_def> vec;
//   for (std::size_t i = 0; i < loop; ++i) {
//     auto insert_pos = vec.emplace(vec.begin(), "inserted");
//     ASSERT_EQ(*insert_pos, no_def("inserted"));
//     ASSERT_EQ(insert_pos->birth, constructed::kParam);
//   }
// }

// TEST(VectorTest, emplace_throwing) {
//   throwing::count = 0;
//   std::size_t size = 4;
//   memory::vector<throwing> vec(size);
//   throwing value("pushed");
//   ASSERT_ANY_THROW(vec.emplace(vec.begin(), value));
//   ASSERT_EQ(vec.size(), size);
//   for (auto i = 0; i < size; ++i) {
//     ASSERT_NO_THROW(vec.at(i));
//   }

//   throwing::count = 4;
//   ASSERT_ANY_THROW(vec.emplace(vec.begin(), value));
//   ASSERT_EQ(vec.size(), size);
//   for (auto i = 0; i < size; ++i) {
//     ASSERT_NO_THROW(vec.at(i));
//   }
// }

// TEST(VectorTest, emplace_back_from_begin_safe) {
//   memory::vector<safe> vec(uid(gen), safe("default"));
//   vec.emplace_back(std::move(vec.front()));
//   ASSERT_EQ(vec.back(), safe("default"));
//   ASSERT_EQ(vec.back().birth, constructed::kMove);
//   ASSERT_EQ(vec.front().birth, constructed::kMove);
//   ASSERT_NE(vec.front(), safe());
// }

// TEST(VectorTest, emplace_back_continious_safe) {
//   std::size_t size = uid(gen);
//   memory::vector<safe> vec(size, safe("default"));
//   for (std::size_t i = 0; i < size; ++i) {
//     vec.emplace_back(std::move(vec[i]));
//   }
//   ASSERT_EQ(vec.size(), size * 2);
//   for (std::size_t i = 0; i < size; ++i) {
//     ASSERT_NE(vec[i], safe("default"));
//   }
//   for (std::size_t i = size; i < 2 * size; ++i) {
//     ASSERT_EQ(vec[i], safe("default"));
//   }
// }

// TEST(VectorTest, emplace_back_safe) {
//   memory::vector<safe> vec(uid(gen));

//   safe &val = vec.emplace_back("default");
//   ASSERT_EQ(val, safe("default"));
//   ASSERT_EQ(val.birth, constructed::kParam);
//   for (std::size_t i = 0; i < vec.size() - 1; ++i) {
//     ASSERT_EQ(vec[i].birth, constructed::kMove);
//   }
// }

// TEST(VectorTest, emplace_back_not_safe) {
//   memory::vector<not_safe> vec(uid(gen));

//   not_safe &val = vec.emplace_back("default");
//   ASSERT_EQ(val, not_safe("default"));
//   ASSERT_EQ(val.birth, constructed::kParam);
//   for (std::size_t i = 0; i < vec.size() - 1; ++i) {
//     ASSERT_EQ(vec[i].birth, constructed::kCopy);
//   }
// }

// TEST(VectorTest, emplace_back_no_realloc) {
//   memory::vector<safe> vec(56);
//   vec.resize(55);

//   safe &val = vec.emplace_back("default");
//   ASSERT_EQ(val, safe("default"));
//   ASSERT_EQ(val.birth, constructed::kParam);
//   for (std::size_t i = 0; i < vec.size() - 1; ++i) {
//     ASSERT_EQ(vec[i].birth, constructed::kDef);
//   }
// }

// TEST(VectorTest, emplace_back_throwing) {
//   throwing::count = 0;
//   std::size_t size = 7;
//   memory::vector<throwing> vec(size);
//   ASSERT_ANY_THROW(vec.emplace(vec.begin(), "pushed"));
//   ASSERT_EQ(vec.size(), size);
//   for (auto i = 0; i < size; ++i) {
//     ASSERT_NO_THROW(vec.at(i));
//   }

//   throwing::count = 4;
//   vec.resize(6);

//   ASSERT_ANY_THROW(vec.emplace(vec.begin(), throwing("pushed")));
//   ASSERT_EQ(vec.size(), size - 1);
//   for (auto i = 0; i < size - 1; ++i) {
//     ASSERT_NO_THROW(vec.at(i));
//   }
// }

// TEST(VectorTest, stream) {
//   memory::vector<safe> vec{
//       safe("Aileen"), safe("Anna"), safe("Louie"), safe("Noel"),
//       safe("Grace"),
//   };
//   std::stringstream stream;
//   std::string expected("Aileen Anna Louie Noel Grace");
//   stream << vec;
//   EXPECT_EQ(expected, stream.str());
// }

// TEST(VectorTest, valid_constexpr) {
//   constexpr std::size_t cexper = constexpr_check(0);
//   ASSERT_EQ(cexper, 0);
// }
