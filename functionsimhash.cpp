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

#include "InstructionDecoder.h"
#include "util.hpp"
#include "functionsimhash.hpp"

uint64_t FunctionSimHasher::FloatsToBits(const std::vector<float>& floats) {
  std::vector<uint64_t> temp;
  FloatsToBits(floats, &temp);
  return temp[0];
}

bool FunctionSimHasher::FloatsToBits(const std::vector<float>& floats,
  std::vector<uint64_t>* outputs) {
  uint64_t index = 0;
  outputs->resize((floats.size() / 64) + 1);
  for (float entry : floats) {
    uint64_t vector_index = index / 64;
    uint64_t bit_index = index % 64;
    if (entry >= 0) {
      (*outputs)[vector_index] |= (1ULL << bit_index);
    }
    ++index;
  }
}

void FunctionSimHasher::DumpFloatState(std::vector<float>* output_floats) {
  for (float value : *output_floats) {
    printf("%03.02f ", value);
  }
  printf("\n");
  std::vector<uint64_t> outputs;
  FloatsToBits(*output_floats, &outputs);
  for (uint64_t hash : outputs) {
    printf("%16.16lx ", hash);
  }
  printf("\n");
}

// Calculates an output vector of floats from the underlying function.
// SimHash works by hashing individual elements, and adding up weights in
// vectors with the same bits. Unfortunately, the construction of SimHash is
// such that repeated entries in a multiset will "overwhelm" the hash, so
// care has to be taken to add cardinalities into the construction.
//
// TODO(thomasdullien): Refactor this code to get rid of the Dyninst dependency
// in the function prototype -- the function should be callable with anything
// that allows iteration over features.
void FunctionSimHasher::CalculateFunctionSimHash(
  Dyninst::ParseAPI::Function* function, uint64_t number_of_outputs,
  std::vector<uint64_t>* output_simhash_values) {

  std::vector<float> output_simhash_floats;
  output_simhash_floats.resize(number_of_outputs);

  Flowgraph graph;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    BuildFlowgraph(function, &graph);
  }

  std::vector<address> nodes;
  graph.GetNodes(&nodes);

  // A map to keep track of how often each feature has been seen during the
  // calculation of this SimHash. Needed to deal with multisets -- if one simply
  // adds the same per-feature-hash into the calculation the resulting SimHash
  // will with high likelihood end up being just the per-feature hash.
  std::map<uint64_t, uint64_t> feature_cardinalities;

  // Process subgraphs.
  for (const address node : nodes) {
    // Calculate a number_of_outputs-bit-hash of the subgraphs.
    std::unique_ptr<Flowgraph> distance_2(graph.GetSubgraph(node, 2, 30));
    std::unique_ptr<Flowgraph> distance_3(graph.GetSubgraph(node, 3, 30));

    if (distance_2) {
      if (verbose_) {
        printf("g2.");
      }

     // Calculate IDs for the graphs.
     uint64_t distance_2_graph_id = HashGraph(distance_2, node, 0, 0);

     // Increment cardinality for how often this feature has been seen.
     uint64_t cardinality = feature_cardinalities[distance_2_graph_id]++;

     ProcessSubgraph(distance_2, node, number_of_outputs, cardinality,
       &output_simhash_floats);
    }
    if (distance_3) {
      if (verbose_) {
        printf("g3.");
      }

     // Calculate IDs for the graphs.
     uint64_t distance_3_graph_id = HashGraph(distance_3, node, 0, 0);

     // Increment cardinality for how often this feature has been seen.
     uint64_t cardinality = feature_cardinalities[distance_3_graph_id]++;

     ProcessSubgraph(distance_3, node, number_of_outputs, cardinality,
       &output_simhash_floats);
    }
  }

  // Process mnemonic n-grams.
  std::vector<MnemTuple> tuples;
  BuildMnemonicNgrams(function, &tuples);
  for (const MnemTuple& tuple : tuples) {
    if (verbose_) {
      printf("mn.");
    }
    uint64_t tuple_id = HashMnemTuple(tuple, 0);
    uint64_t cardinality = feature_cardinalities[tuple_id]++;

    ProcessMnemTuple(tuple, number_of_outputs, cardinality,
      &output_simhash_floats);
  }

  if (verbose_) {
    printf("\n");
  }

  FloatsToBits(output_simhash_floats, output_simhash_values);
}

// TODO(thomasdullien): When the refactoring of the main implementation (above)
// is complete, remove this code - if arbitrary iterators can be used as input,
// this code will serve no purpose any more.
void FunctionSimHasher::CalculateFunctionSimHash(
  std::vector<FeatureHash>* features, std::vector<uint64_t>* output) {

  std::vector<float> floats(128);
  for (const auto& feature : *features) {
    std::vector<uint64_t> hash = { feature.first, feature.second };
    double weight = 1.0;
    if (weights_.find(feature.first) != weights_.end()) {
      weight = weights_.at(feature.first);
    }
    AddWeightsInHashToOutput(hash, 128, weight, &floats);
  }
  FloatsToBits(floats, output);
}

// Add a given subgraph into the vector of floats.
void FunctionSimHasher::ProcessSubgraph(std::unique_ptr<Flowgraph>& graph,
  address node, uint64_t bits, uint64_t hash_index,
  std::vector<float>* output_simhash_floats) const {
  // Retrieve the saved weight of the subgraph graphlet.
  float graphlet_weight = GetGraphletWeight(graph, node);

  // Calculate n-bit hash of the subgraph.
  std::vector<uint64_t> hash;
  CalculateNBitGraphHash(graph, node, bits, hash_index, &hash);

  // Iterate over the bits in the hash.
  AddWeightsInHashToOutput(hash, bits, graphlet_weight, output_simhash_floats);
}

// Add a given mnemonic tuple into the vector of floats.
void FunctionSimHasher::ProcessMnemTuple(const MnemTuple &tup, uint64_t bits,
  uint64_t hash_index, std::vector<float>* output_simhash_floats) const {
  float mnem_tuple_weight = GetMnemTupleWeight(tup);

  std::vector<uint64_t> hash;
  CalculateNBitMnemTupleHash(tup, bits, hash_index, &hash);

  AddWeightsInHashToOutput(hash, bits, mnem_tuple_weight, output_simhash_floats);
}

// Iterate over an n-bit hash; add weights into the vector for each one,
// subtract the weights from the vector for each zero.
void FunctionSimHasher::AddWeightsInHashToOutput(
  const std::vector<uint64_t>& hash, uint64_t bits, float weight,
  std::vector<float>* output_simhash_floats) const {

  if (verbose_) {
    for (const uint64_t hash_value : hash) {
      printf("%16.16lx", hash_value);
    }
    printf(" ");
  }

  for (uint64_t bit_index = 0; bit_index < bits; ++bit_index) {
    if (GetNthBit(hash, bit_index)) {
      (*output_simhash_floats)[bit_index] += weight;
    } else {
      (*output_simhash_floats)[bit_index] -= weight;
    }
  }
}

// In order to facilitate the creation of a hash function family, create a
// different hash function given a particular hash index.
inline uint64_t FunctionSimHasher::SeedXForHashY(uint64_t seed_index,
  uint64_t hash_index) const {
  if (seed_index == 0) {
    return rotl64(seed0_, hash_index % 7) * (hash_index + 1);
  } else if (seed_index == 1) {
    return rotl64(seed1_, hash_index % 11) * (hash_index + 1);
  } else if (seed_index == 2) {
    return rotl64(seed2_, hash_index % 13) * (hash_index + 1);
  }
}

// Hash a tuple of mnemonics into a 64-bit value, using a hash family index.
uint64_t FunctionSimHasher::HashMnemTuple(const MnemTuple& tup,
  uint64_t hash_index) const {
  size_t value1 = SeedXForHashY(0, hash_index) ^ SeedXForHashY(1, hash_index) ^
    SeedXForHashY(2, hash_index);
  value1 *= std::hash<std::string>{}(std::get<0>(tup));
  value1 = rotl64(value1, 7);
  value1 *= std::hash<std::string>{}(std::get<1>(tup));
  value1 = rotl64(value1, 7);
  value1 *= std::hash<std::string>{}(std::get<2>(tup));
  value1 = rotl64(value1, 7);
  value1 *= (k2 * (hash_index + 1));
  return value1;
}

// Hash a graph into a 64-bit value, using a hash family index and a counter.
// The hash family index is used for choosing a hash family (clearly ;), the
// counter is used for generating multi-word hash outputs.
uint64_t FunctionSimHasher::HashGraph(std::unique_ptr<Flowgraph>& graph,
  address node, uint64_t hash_index, uint64_t counter) const {
  return graph->CalculateHash(node,
    SeedXForHashY(0, hash_index) * (counter + 1),
    SeedXForHashY(1, hash_index) * (counter + 1),
    SeedXForHashY(2, hash_index) * (counter + 1));
}

// Extend a 64-bit graph hash family to a N-bit hash family by just increasing
// the hash function index.
void FunctionSimHasher::CalculateNBitGraphHash(
  std::unique_ptr<Flowgraph>& graph, address node, uint64_t bits,
  uint64_t hash_index, std::vector<uint64_t>* output) const {
  output->clear();

  for (uint64_t counter = 0; counter < bits; counter += 64) {
    output->push_back(HashGraph(graph, node, hash_index, counter));
  }
}

// Extend a 64-bit mnemonic tuple hash family to a N-bit hash family by just
// increasing the hash function index.
void FunctionSimHasher::CalculateNBitMnemTupleHash(
  const MnemTuple& tup, uint64_t bits, uint64_t hash_index,
  std::vector<uint64_t>* output) const {
  output->clear();

  for (uint64_t counter = 0; counter < bits; counter += 64) {
    output->push_back(HashMnemTuple(tup, hash_index + (counter + 1)));
  }
}

// Return a weight for a given key, default to 1.0.
float FunctionSimHasher::GetWeight(uint64_t key, float standard = 1.0) const {
  const auto& iter = weights_.find(key);
  return (iter == weights_.end()) ? standard : iter->second;
}

// Hash the graph under the first hash function of the family, then see if a
// pre-computed weight is loaded.
float FunctionSimHasher::GetGraphletWeight(std::unique_ptr<Flowgraph>& graph,
  address node) const {
  uint64_t graphlet_id = graph->CalculateHash(node,
    SeedXForHashY(0, 0), SeedXForHashY(0, 1), SeedXForHashY(0, 2));
  return GetWeight(graphlet_id, default_graphlet_weight_);
}

float FunctionSimHasher::GetMnemTupleWeight(const MnemTuple& tup) const {
  uint64_t mnem_id = HashMnemTuple(tup, 0);
  // In the absence of proper weight learning, use 1/20th the weight of
  // a graphlet.
  return GetWeight(mnem_id, default_mnemonic_weight_);
}

// Given a vector of 64-bit values, retrieve the n-th bit.
inline bool FunctionSimHasher::GetNthBit(const std::vector<uint64_t>& nbit_hash,
  uint64_t bitindex) const {
  uint64_t index = bitindex / 64;
  uint64_t value = nbit_hash[index];
  uint64_t sub_word_index = bitindex % 64;
  return (value >> sub_word_index) & 1;
}

// Given a Dyninst function, create the set of instruction 3-grams.
void FunctionSimHasher::BuildMnemonicNgrams(Dyninst::ParseAPI::Function* function,
  std::vector<MnemTuple>* tuples) const {
  std::vector<std::pair<address, std::string>> sequence;
  // Make sure that reading the mnemonics from Dyninst is synchronized.
  {
    std::lock_guard<std::mutex> lock(mutex_);
    // Perform the calculation of the mnemonic-n-gram-hashes.
    for (const auto& block : function->blocks()) {
      Dyninst::ParseAPI::Block::Insns block_instructions;
      block->getInsns(block_instructions);
      for (const auto& instruction : block_instructions) {
        auto& op = instruction.second->getOperation();
        sequence.push_back(std::make_pair(instruction.first, op.format()));
      }
    }
  }
  // Sort instructions by address;
  std::sort(sequence.begin(), sequence.end());
  // Construct the 3-tuples.
  for (uint64_t index = 0; index + 2 < sequence.size(); ++index) {
    tuples->emplace_back(std::make_tuple(
      sequence[index].second,
      sequence[index+1].second,
      sequence[index+2].second));
  }
}

FunctionSimHasher::FunctionSimHasher(const std::string& weight_file,
  bool verbose, double default_mnemonic_weight, double default_graphlet_weight) :
    verbose_(verbose), default_mnemonic_weight_(default_mnemonic_weight),
    default_graphlet_weight_(default_graphlet_weight) {
  std::vector<std::vector<std::string>> tokenized_lines;

  if (weight_file == "") {
    return;
  }
  if (FileToLineTokens(weight_file, &tokenized_lines)) {
    for (const std::vector<std::string>& line : tokenized_lines) {
      FeatureHash hash = StringToFeatureHash(line[0]);
      double weight = strtod(line[1].c_str(), nullptr);
      weights_[hash.first] = weight;
    }
  }
}

FunctionSimHasher::FunctionSimHasher(std::map<uint64_t, float>* weights) :
  weights_(*weights), verbose_(false), default_mnemonic_weight_(0.05),
  default_graphlet_weight_(1.0) {
  weights_ = *weights;
}


