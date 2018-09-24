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


#ifndef SIMHASHSEARCHINDEX_HPP
#define SIMHASHSEARCHINDEX_HPP

#include <cstdint>
#include <mutex>
#include "util/persistentmap.hpp"

// Pretend uint128_t was a standard type already.
typedef __uint128_t uint128_t;

// A simple memory-mapping backed index for querying a 128-bit SimHash.
// Permutations of the full 128-bit value are used for turning the SimHash
// into a family of hashes.
//
// Determining the concrete number of buckets is a somewhat tricky trade-
// off between memory consumption, query performance, and desired accuracy, so
// I will describe an example calculation in the following:
//
// There is a trade-off where the similarity search deteriorates quickly
// if you include things that are too dissimilar with your target.
// So the first thing that we need to decide "for what maximum hamming dist-
// ance of the SimHash vector do we want the search to work well".
//
// First, let us gather all the data we need to perform our calculation:
//
// A: Approximate number of functions in our index: 50 million
// B: Maximum "distance" from the query that we wish to find: 20% ~ 0.2
// C: Maximum acceptable odds of missing an item when querying: 5% ~ 0.05
// D: Maximum number of items we wish to "touch" on each query: ~5 million
//    (this will determine per-item query latency)
//
// The value B leads us to 0.2 * 128 ~= 26 bits difference -- so we need to
// make sure we can efficiently find SimHash fingerprints that differ in up
// to 26 places.
//
// The probability of a given item to fall inside a randomly picked bucket
// extracted from N bits of the SimHash is the same as the probability of
// obtaining a N-bit run of white balls when drawing IID from an urn with
// 102 white and 26 black balls.
//
// The probability for a run of a given length can be calculated using the
// following Python function (warning, likely to contain subtle errors):
//
// def probability_for_run(N, B, W, n):
//   """ Calculates the probability of obtaining a run of n white balls
//   when iid drawing from an urn with N balls of which B are black """
//   result = 1.0
//   B = float(B)
//   n = float(n)
//   N = float(N)
//   for i in range(int(n)):
//      result *= ((N - i - B)/(N - i))
//   result *= (B / (N - n - 1))
//   return result
//
// Summing up the probabilities for white runs of 9 or more balls, we obtain
// that the odds of getting 9 or more white balls in a row is about ~15.47%
//
// So we want to achieve (1.0 - 0.1547)^buckets < 0.05
// We find that (1.0 - 0.1547)^18 ~= 0.049 < 0.05, so we see that with 18
// buckets the odds of missing one item goes below 5%. If we wanted the odds
// of missing one item to go below 1%, we will need 28 buckets.
//
// The 50 million entries will be split using the 8-bit bucket IDs, leading to
// expected 50m / 256.0 elements per bucket (about 200k). If we look at 18
// buckets per query, we expect the code to have to examine about 3.6m
// elements per query; if we look at 28 buckets, the code will have to ex-
// amine about 5.5m elements per query.
//
// Examining an element should be comparatively cheap, so we should still
// be able to get acceptable latency out of this.

class SimHashSearchIndex {
public:
  // Uniquely identifies a bitwise permutation of the 128-bit value.
  typedef uint8_t PermutationIndex;
  // Upper 64 bits of the hash.
  typedef uint64_t HashValueA;
  // Lower 64 bits of the hash.
  typedef uint64_t HashValueB;
  // Unique ID for the function.
  typedef uint64_t FunctionID;
  typedef std::tuple<PermutationIndex, HashValueA, HashValueB, FunctionID>
    IndexEntry;

  typedef uint64_t FileID;
  typedef uint64_t Address;
  typedef std::pair<FileID, Address> FileAndAddress;

  SimHashSearchIndex(const std::string& indexname,
    bool create, uint8_t buckets = 50);

  uint64_t QueryTopN(uint64_t hash_A, uint64_t hash_B, uint32_t how_many,
    std::vector<std::pair<float, FileAndAddress>>* results);

  uint64_t AddFunction(uint64_t hash_A, uint64_t hash_B, FileID file_id,
    Address address);

  uint64_t GetIndexFileSize();
  uint64_t GetIndexFileFreeSpace();
  uint64_t GetIndexSetSize() const;
  uint64_t GetNumberOfIndexedFunctions() const;
  uint8_t GetNumberOfBuckets() const;
  double GetOddsOfRandomHit(uint32_t count) const;

  void DumpIndexToStdout(bool all) const;
private:

  // TODO(thomasdullien): As soon as the codebase is ported to C++14,
  // replace the following mutex with a shared_mutex to allow concurrent
  // reading from the index.
  mutable std::mutex mutex_;
  PersistentMap<FunctionID, FileAndAddress> id_to_file_and_address_;
  PersistentSet<IndexEntry> search_index_;
  uint8_t buckets_;
};

#endif // SIMHASHSEARCHINDEX_HPP
