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

#include "gtest/gtest.h"
#include "simhashsearchindex.hpp"

TEST(simhashsearchindex, initialize) {
  // Run the constructor.
  {
    SimHashSearchIndex index("./testindex.index", true, 28);
  }
  // Should've run the destructor.
  EXPECT_EQ(unlink("./testindex.index"), 0);
}

TEST(simhashsearchindex, addfunction) {
  SimHashSearchIndex index("./testindex.index", true, 28);
  index.AddFunction(0xDEADBEEF0BADBABE, 0x0BADFEEDBA551055, 0x1,
    0x400000);
  EXPECT_EQ(index.GetIndexSetSize(), 28);
  EXPECT_EQ(unlink("./testindex.index"), 0);
}

TEST(simhashsearchindex, persistence) {
  {
    SimHashSearchIndex index("./testindex.index", true, 28);
    index.AddFunction(0xDEADBEEF0BADBABE, 0x0BADFEEDBA551055, 0x1,
      0x400000);
    EXPECT_EQ(index.GetIndexSetSize(), 28);
  }
  SimHashSearchIndex index2("./testindex.index", false, 28);
  EXPECT_EQ(index2.GetIndexSetSize(), 28);
  EXPECT_EQ(unlink("./testindex.index"), 0);
}

TEST(simhashsearchindex, querytopn_precise) {
  SimHashSearchIndex index("./testindex.index", true, 28);
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

  for (uint32_t i = 0; i < constantarray.size()-1; ++i) {
    index.AddFunction(constantarray[i], constantarray[i+1],
      static_cast<uint64_t>(i), static_cast<uint64_t>(i));
  }
  EXPECT_EQ(index.GetIndexSetSize(), 28 * (constantarray.size()-1));

  for (uint32_t i = 0; i < constantarray.size()-1; ++i) {
    uint64_t hash_a = constantarray[i];
    uint64_t hash_b = constantarray[i+1];

    std::vector<std::pair<float, SimHashSearchIndex::FileAndAddress>> results;
    index.QueryTopN(hash_a, hash_b, 5, &results);
    EXPECT_EQ(results[0].second.first, i);
  }

  EXPECT_EQ(unlink("./testindex.index"), 0);

}

TEST(simhashsearchindex, querytopn) {
  SimHashSearchIndex index("./testindex.index", true, 28);
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

  for (uint32_t i = 0; i < constantarray.size()-1; ++i) {
    index.AddFunction(constantarray[i], constantarray[i+1],
      static_cast<uint64_t>(i), static_cast<uint64_t>(i));
  }
  EXPECT_EQ(index.GetIndexSetSize(), 28 * (constantarray.size()-1));

  std::array<uint64_t, 3> distortions = {
    0x0180018001800180UL,
    0x0101010101010101UL,
    0x8080808080808080UL };

  for (uint32_t i = 0; i < constantarray.size()-1; ++i) {
    uint64_t hash_a = constantarray[i];
    uint64_t hash_b = constantarray[i+1];

    for (uint32_t distortion_index = 0; distortion_index < distortions.size();
      ++distortion_index) {
      hash_a ^= distortions[distortion_index];
      hash_b ^= distortions[distortion_index];

      std::vector<std::pair<float, SimHashSearchIndex::FileAndAddress>> results;
      index.QueryTopN(hash_a, hash_b, 5, &results);
      EXPECT_EQ(results[0].second.first, i);
    }
  }

  EXPECT_EQ(unlink("./testindex.index"), 0);
}
