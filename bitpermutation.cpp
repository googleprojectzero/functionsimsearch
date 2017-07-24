#include "bitpermutation.hpp"

// A 128-bit bitwise random permutation that should take ~65 cycles. Generated
// using the absolutely fantastic permutation generator code that can be found
// under http://programming.sirrida.de/calcperm.php

uint128_t bit_permute_step(uint128_t x, uint128_t m, uint128_t shift) {
  uint128_t t;
  t = ((x >> shift) ^ x) & m;
  x = (x ^ t) ^ (t << shift);
  return x;
}

// A somewhat ugly workaround for the fact that there is no way to specify
// 128-bit constants in GCC.
constexpr uint128_t m128(uint64_t a, uint64_t b) {
  return (static_cast<uint128_t>(a) << 64) | b;
}

uint128_t permute_128_bit(uint128_t x) {
  x = bit_permute_step(x, m128(0x1110444400100110UL, 0x4501005550510550UL), 1);
  x = bit_permute_step(x, m128(0x2123333100302210UL, 0x3110101001022210UL), 2);
  x = bit_permute_step(x, m128(0x0c010c0d09040901UL, 0x0a0c01010a040b00UL), 4);
  x = bit_permute_step(x, m128(0x0052006700fe0041UL, 0x00ab002900330000UL), 8);
  x = bit_permute_step(x, m128(0x0000678f0000bd05UL, 0x0000236300000000UL), 16);
  x = bit_permute_step(x, m128(0x0000000004c30052UL, 0x0000000000000000UL), 32);
  x = bit_permute_step(x, m128(0x0000000000000000UL, 0x3782ccfd884d7006UL), 64);
  x = bit_permute_step(x, m128(0x0000000096739fb5UL, 0x00000000f20085beUL), 32);
  x = bit_permute_step(x, m128(0x000076f900004a91UL, 0x000018220000ecf9UL), 16);
  x = bit_permute_step(x, m128(0x00bf001d00720029UL, 0x001300c40053007cUL), 8);
  x = bit_permute_step(x, m128(0x06040a09050f0a04UL, 0x0d0202050c010c0fUL), 4);
  x = bit_permute_step(x, m128(0x1231022131331030UL, 0x2232233111022130UL), 2);
  x = bit_permute_step(x, m128(0x0140545111411015UL, 0x0010144154505040UL), 1);
  return x;
}

void get_n_permutations(uint128_t value, uint32_t n, std::vector<uint128_t>*
  results) {
  for (uint32_t index = 0; index < n; ++index) {
    value = permute_128_bit(value);
    results->push_back(value);
  }
}
