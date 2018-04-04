#include "gtest/gtest.h"
#include "util/bitpermutation.hpp"
#include <array>

TEST(bitpermutation, is_permutation) {
  uint32_t index = 0;

  for (uint32_t shift = 0; shift < 128; ++shift) {
    uint128_t value = static_cast<uint128_t>(1) << shift;
    uint128_t permuted = permute_128_bit(value);
    while ((permuted & 1) == 0) {
      permuted = permuted >> 1;
      ++index;
    }
  }
  EXPECT_EQ(index, 8128);
}

TEST(bitpermutation, in_and_out) {
  std::array<uint64_t, 12> constantarray = {
    0xba5eba11bedabb1eUL,
    0xbe5077edb0a710adUL,
    0xb01dfacecab005e0UL,
    0xca11ab1eca55e77eUL,
    0xdeadbea700defec8UL,
    0xf01dab1ef005ba11UL,
    0x0ddba115ca1ab1e0UL,
    0x7e1eca57deadbeefUL,
    0xca5cadab1ef00d50UL,
    0x0b501e7edecea5edUL,
    0x7e55e118df00d500UL,
    0x0e1ec7edba11a575UL };
  std::vector<uint128_t> results;

  for (uint32_t i = 0; i < constantarray.size()-1; ++i) {
    uint128_t val = to128(constantarray[i], constantarray[i+1]);
    results.push_back(permute_128_bit(val));
  }

  for (uint32_t i = 0; i < constantarray.size()-1; ++i) {
    uint128_t val = to128(constantarray[i], constantarray[i+1]);
    EXPECT_EQ(permute_128_bit(val), results[i]);
  }
}
