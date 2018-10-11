#include "gtest/gtest.h"
#include "util/testutil.hpp"
#include "disassembly/extractimmediate.hpp"

TEST(extract_immediate, x86registers) {
  std::vector<uint64_t> results;
  ExtractImmediateFromString("EAX", &results);
  EXPECT_EQ(results.size(), 0);

  ExtractImmediateFromString("EBX", &results);
  EXPECT_EQ(results.size(), 0);

  ExtractImmediateFromString("ECX", &results);
  EXPECT_EQ(results.size(), 0);

  ExtractImmediateFromString("EDX", &results);
  EXPECT_EQ(results.size(), 0);

  ExtractImmediateFromString("EDI", &results);
  EXPECT_EQ(results.size(), 0);

  ExtractImmediateFromString("ESI", &results);
  EXPECT_EQ(results.size(), 0);

  ExtractImmediateFromString("AH", &results);
  EXPECT_EQ(results.size(), 0);

  ExtractImmediateFromString("EBP", &results);
  EXPECT_EQ(results.size(), 0);
}

TEST(extract_immediate, ecx_negative_offset) {
  std::vector<uint64_t> results;
  ExtractImmediateFromString("RCX + ffffffffffffffc0", &results);
  EXPECT_EQ(results.size(), 1);
  if (results.size() > 0) {
    EXPECT_EQ(results[0], 0xffffffffffffffc0);
  }
}

TEST(extract_immediate, scaled_index) {
  std::vector<uint64_t> results;
  ExtractImmediateFromString("R13 + RCX * 8 + ffffffffffffffd0", &results);
  EXPECT_EQ(results.size(), 2);
  if (results.size() == 2) {
    EXPECT_EQ(results[0], 8);
    EXPECT_EQ(results[1], 0xffffffffffffffd0);
  }
}


