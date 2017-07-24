// Copyright 2017 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef FLOWGRAPHUTIL_HPP
#define FLOWGRAPHUTIL_HPP

#include "CodeObject.h"

#include "flowgraph.hpp"

// Some primes between 2^63 and 2^64 from CityHash.
static constexpr uint64_t k0 = 0xc3a5c85c97cb3127ULL;
static constexpr uint64_t k1 = 0xb492b66fbe98f273ULL;
static constexpr uint64_t k2 = 0x9ae16a3b2f90404fULL;

uint64_t BuildFlowgraph(Dyninst::ParseAPI::Function* function, Flowgraph* graph);

inline uint64_t rotl64 ( uint64_t x, int8_t r ) {
  return (x << r) | (x >> (64 - r));
}

// GCC supports 128-bit integers.
inline __uint128_t rotl128 ( __uint128_t x, int8_t r ) {
  return (x << r) | (x >> (128 - r));
}

#endif
