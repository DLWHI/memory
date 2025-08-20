#include <gtest/gtest.h>

#include <random>

#include "memory/iterators/bit_iterator.h"

static std::random_device ran_dev;
static std::mt19937 gen(ran_dev());

TEST(BitIteratorTest, init_byte) {
  char bits = 0xF0;
  memory::bit_iterator it(&bits);
  for (std::size_t i = 0; i < 8; ++i, ++it) {
     ASSERT_EQ((bits >> i) & 1, *it);
  }
}

TEST(BitIteratorTest, init_word) {
  std::uniform_int_distribution<uint32_t> uid(0, 0xFFFFFFFFu);
  uint32_t bits = uid(gen);
  memory::bit_iterator it(&bits);
  for (std::size_t i = 0; i < 32; ++i, ++it) {
    ASSERT_EQ((bits >> i) & 1, *it);
  }
}

TEST(BitIteratorTest, pos) {
  std::uniform_int_distribution<uint32_t> uid(0, 0xFFFFFFFFu);
  uint32_t bits = uid(gen);
  memory::bit_iterator it(&bits);
  for (std::size_t i = 0; i < 32; ++i, ++it) {
    ASSERT_EQ(i, it.position());
  }
}

TEST(BitIteratorTest, distance) {
  std::uniform_int_distribution<uint32_t> uid(0, 0xFFFFFFFFu);
  uint32_t bits = uid(gen);
  memory::bit_iterator first(&bits), last(first);
  for (std::size_t i = 0; i < 32; ++i, ++last) {
    ASSERT_EQ(std::distance(first, last), last - first);
  }
}

TEST(BitIteratorTest, shift) {
  uint32_t bits = 0x8D8AC;
  memory::bit_iterator it(&bits);
  it += 11;
  ASSERT_EQ(it.position(), 11);
  ASSERT_TRUE(*it);
  it -= 5;
  ASSERT_EQ(it.position(), 6);
  ASSERT_FALSE(*it);
}

TEST(BitIteratorTest, shift_whole) {
  uint32_t bits = 0x8D8AC;
  memory::bit_iterator it(&bits, 7);
  ASSERT_TRUE(*it);
  it += 8;
  ASSERT_EQ(it.position(), 15);
  ASSERT_TRUE(*it);
}

TEST(BitIteratorTest, shift_left_1) {
  uint32_t bits = 0x8D8AC;
  memory::bit_iterator it(&bits, 7);
  ASSERT_TRUE(*it);
  it += 5;
  ASSERT_EQ(it.position(), 12);
  ASSERT_TRUE(*it);
}

TEST(BitIteratorTest, shift_left_2) {
  uint32_t bits = 0x8D8AC;
  memory::bit_iterator it(&bits, 7);
  ASSERT_TRUE(*it);
  it += 6;
  ASSERT_EQ(it.position(), 13);
  ASSERT_FALSE(*it);
}

TEST(BitIteratorTest, shift_left_3) {
  uint64_t bits = 0x3F5EB5D32;
  memory::bit_iterator it(&bits, 3);
  ASSERT_FALSE(*it);
  it += 14;
  ASSERT_EQ(it.position(), 17);
  ASSERT_TRUE(*it);
}

TEST(BitIteratorTest, shift_right_1) {
  uint32_t bits = 0x8D8AC;
  memory::bit_iterator it(&bits, 7);
  ASSERT_TRUE(*it);
  it -= 5;
  ASSERT_EQ(it.position(), 2);
  ASSERT_TRUE(*it);
}

TEST(BitIteratorTest, shift_right_2) {
  uint32_t bits = 0x8D8AC;
  memory::bit_iterator it(&bits, 7);
  ASSERT_TRUE(*it);
  it -= 6;
  ASSERT_EQ(it.position(), 1);
  ASSERT_FALSE(*it);
}

TEST(BitIteratorTest, shift_right_3) {
  uint64_t bits = 0x3F5EB5D32;
  memory::bit_iterator it(&bits, 12);
  ASSERT_TRUE(*it);
  it -= 5;
  ASSERT_EQ(it.position(), 7);
  ASSERT_FALSE(*it);
}

TEST(BitIteratorTest, flip_1) {
  uint8_t bits = 0xF0;
  memory::bit_iterator it(&bits);
  for (std::size_t i = 0; i < 8; ++i, ++it) {
    it.flip();
  }
  ASSERT_EQ(bits, 0x0F);
}

TEST(BitIteratorTest, flip_2) {
  uint8_t bits = 0xF0;
  memory::bit_iterator it(&bits, 3);
  ASSERT_FALSE(*it);
  it.flip();
  ASSERT_TRUE(*it);
  it.flip();
  ASSERT_FALSE(*it);
}

