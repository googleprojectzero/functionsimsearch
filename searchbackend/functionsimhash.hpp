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

#ifndef FUNCTIONSIMHASH_HPP
#define FUNCTIONSIMHASH_HPP

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <vector>

#include "CodeObject.h"
#include "disassembly/flowgraph.hpp"
#include "disassembly/flowgraphutil.hpp"
#include "disassembly/functionfeaturegenerator.hpp"
#include "util/util.hpp"

// A class to perform per-function SimHash calculation.
//
// SimHash was introduced in the paper "Similarity Estimation Techniques from
// Rounding Algorithms" (Moses S. Charikar).
//
// The basic idea proceeds as follows:
//   1) Cut your input into pieces, possibly of heterogenous type.
//   2) Keep a vector of N floats, initialized to all zeroes initially.
//   3) For each piece, obtain a weight.
//   4) Hash the piece into N bits; for each 1-bit, add the weight to the float
//      at the right index, for each 0-bit, subtract the weight.
//   5) Convert the vector of floats to a vector of zeroes and ones again.
//
// The weights themselves are given in a flat text file. The keys in this
// map are the hash values of the feature using the first hash function of the
// hash family (subsequent hash functions are used for the calculations).
class FunctionSimHasher {
public:
  // The weight_file is a simple memory-mapped map that maps uint64_t IDs for
  // a feature to float weights. The second argument is used to obtain the
  // IDs for features used in the calculation of the SimHash, and mainly used
  // for debugging.
  FunctionSimHasher(const std::string& weight_file,
    bool disable_graphs = false,
    bool disable_mnemonic = false,
    bool dump_graphlet_dictionary = false,
    bool dump_mnem_tuple_dictionary = false,
    double default_mnemomic_weight = 0.05,
    double default_graphlet_weight = 1.0);

  FunctionSimHasher(std::map<uint64_t, float>* weights);

  // Calculate a simhash value for a given function. Outputs a vector of 64-bit
  // values, number_of_outputs describes how many bits of SimHash should be
  // calculated. Reasonable use is usually 128.
  void CalculateFunctionSimHash(
    FunctionFeatureGenerator* generator, uint64_t number_of_outputs,
    std::vector<uint64_t>* output_simhash_values,
    std::vector<FeatureHash>* feature_hashes=nullptr);

  void CalculateFunctionSimHash(
    std::vector<FeatureHash>* features, std::vector<uint64_t>* output,
    std::vector<float>* optional_state = nullptr);

  static uint64_t FloatsToBits(const std::vector<float>& floats);
  static bool FloatsToBits(const std::vector<float>& floats,
    std::vector<uint64_t>* outputs);

  // Primarily exposed for testing.
  const std::map<uint64_t, float>* GetWeights() const {
    return &weights_;
  }
private:
  // Process one subgraph and hash it into the output vector.
  void ProcessSubgraph(std::unique_ptr<Flowgraph>& graph, float graphlet_weight,
    address node, uint64_t bits, uint64_t cardinality,
    std::vector<float>* output_simhash_floats, std::vector<FeatureHash>*
    feature_hashes = nullptr) const;

  // Process one mnemonic n-gram and hash it into the output vector.
  void ProcessMnemTuple(const MnemTuple &tup, float weight, uint64_t bits,
    uint64_t hash_index, std::vector<float>* output_simhash_floats,
    std::vector<FeatureHash>* feature_hashes = nullptr) const;

  // Given an n-bit hash and a weight, hash the weight into the output vector
  // with positive sign for 1's and negative sign for 0's.
  void AddWeightsInHashToOutput(const std::vector<uint64_t>& hash, uint64_t bits,
    float weight, std::vector<float>* output_simhash_floats) const;

  // Helper function to obtain seed values for a given hash index.
  inline uint64_t SeedXForHashY(uint64_t seed_index, uint64_t hash_index) const;

  // Hash a tuple of mnemonics into a 64-bit value, using a hash family index.
  uint64_t HashMnemTuple(const MnemTuple& tup, uint64_t hash_index) const;

  // Hash a graph into a 64-bit value, using a hash family index.
  uint64_t HashGraph(std::unique_ptr<Flowgraph>& graph, address node,
    uint64_t hash_index, uint64_t counter) const;

  // Extend a 64-bit graph hash family to a N-bit hash family by just increasing
  // the hash function index.
  void CalculateNBitGraphHash(std::unique_ptr<Flowgraph>& graph, address node,
    uint64_t bits, uint64_t hash_index, std::vector<uint64_t>* output) const;

  // Extend a 64-bit mnemonic tuple hash family to a N-bit hash family by just
  // increasing the hash function index.
  void CalculateNBitMnemTupleHash(const MnemTuple& tup, uint64_t bits,
    uint64_t hash_index, std::vector<uint64_t>* output) const;

  // Return a weight for a given key.
  float GetWeight(uint64_t key, float standard) const;

  // Obtain graphlet or mnemonic IDs with or without occurrence.
  uint64_t GetGraphletIdOccurrence(std::unique_ptr<Flowgraph>& graph,
    uint32_t occurrence, address node) const;
  uint64_t GetGraphletIdNoOccurrence(std::unique_ptr<Flowgraph>& graph,
    address node) const;
  uint64_t GetMnemonicIdOccurrence(const MnemTuple& tuple,
    uint32_t occurrence) const;
  uint64_t GetMnemonicIdNoOccurrence(const MnemTuple& tuple) const;

  inline bool GetNthBit(const std::vector<uint64_t>& nbit_hash,
    uint64_t bitindex) const;

  void DumpFloatState(std::vector<float>* output_floats);

  std::map<uint64_t, float> weights_;

  double default_mnemonic_weight_;
  double default_graphlet_weight_;

  bool disable_graph_hashes_;
  bool disable_mnemonic_hashes_;
  bool dump_graphlet_dictionary_;
  bool dump_mnem_tuple_dictionary_;

  // Some primes between 2^63 and 2^64 from CityHash.
  static constexpr uint64_t seed0_ = 0xc3a5c85c97cb3127ULL;
  static constexpr uint64_t seed1_ = 0xb492b66fbe98f273ULL;
  static constexpr uint64_t seed2_ = 0x9ae16a3b2f90404fULL;
};

#endif // FUNCTIONSIMHASH_HPP
