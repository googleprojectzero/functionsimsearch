#ifndef __BITPERMUTATION_HPP
#define __BITPERMUTATION_HPP

#include <cstdint>
#include <vector>

// A 128-bit bitwise random permutation that should take ~65 cycles. Generated
// using the absolutely fantastic permutation generator code that can be found
// under http://programming.sirrida.de/calcperm.php

typedef __uint128_t uint128_t;

uint128_t bit_permute_step(uint128_t x, uint128_t m, uint128_t shift);

uint128_t permute_128_bit(uint128_t x);

void get_n_permutations(uint128_t value, uint32_t n, std::vector<uint128_t>*
  results);

inline uint128_t to128(uint64_t a, uint64_t b) {
  uint128_t result = a;
  result = result << 64;
  result |= b;
  return result;
}

inline uint64_t getHigh64(__uint128_t val) {
  return (val >> 64);
}

inline uint64_t getLow64(__uint128_t val) {
  return val;
}

#endif // BITPERMUTATION_HPP
