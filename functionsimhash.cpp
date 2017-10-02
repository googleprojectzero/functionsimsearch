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
void FunctionSimHasher::CalculateFunctionSimHash(
  FunctionFeatureGenerator* generator, uint64_t number_of_outputs,
  std::vector<uint64_t>* output_simhash_values, std::vector<FeatureHash>*
  feature_hashes) {

  std::vector<float> output_simhash_floats;
  output_simhash_floats.resize(number_of_outputs);

  // A map to keep track of how often each feature has been seen during the
  // calculation of this SimHash. Needed to deal with multisets -- if one simply
  // adds the same per-feature-hash into the calculation the resulting SimHash
  // will with high likelihood end up being just the per-feature hash.
  std::map<uint64_t, uint64_t> feature_cardinalities;

  // Process subgraphs.
  while (generator->HasMoreSubgraphs()) {
    std::pair<Flowgraph*, address> graphlet_and_node =
      generator->GetNextSubgraph();
    std::unique_ptr<Flowgraph> graphlet(graphlet_and_node.first);
    address node = graphlet_and_node.second;
    if (graphlet) {
      uint64_t graphlet_id = GetGraphletIdNoOccurrence(graphlet, node);

      uint64_t cardinality = feature_cardinalities[graphlet_id]++;

      uint64_t graphlet_id_with_cardinality = GetGraphletIdOccurrence(
        graphlet, cardinality, node);

      // Get the weight for the graphlet.
      float graphlet_weight = GetWeight(graphlet_id_with_cardinality,
        default_graphlet_weight_);

      ProcessSubgraph(graphlet, graphlet_weight, node, number_of_outputs,
        cardinality, &output_simhash_floats, feature_hashes);
    }
  }
  while (generator->HasMoreMnemonics()) {
    MnemTuple tuple = generator->GetNextMnemTuple();
    uint64_t tuple_id = GetMnemonicIdNoOccurrence(tuple);
    uint64_t cardinality = feature_cardinalities[tuple_id]++;

    uint64_t mnemonic_id_with_cardinality = GetMnemonicIdOccurrence(tuple,
      cardinality);
    float mnemonic_tuple_weight = GetWeight(mnemonic_id_with_cardinality,
      default_mnemonic_weight_);

    ProcessMnemTuple(tuple, mnemonic_tuple_weight, number_of_outputs, cardinality,
      &output_simhash_floats, feature_hashes);
  }

  FloatsToBits(output_simhash_floats, output_simhash_values);
}

void FunctionSimHasher::CalculateFunctionSimHash(
  std::vector<FeatureHash>* features, std::vector<uint64_t>* output,
  std::vector<float>* optional_state) {

  std::vector<float> floats(128);
  for (const auto& feature : *features) {
    std::vector<uint64_t> hash = { feature.first, feature.second };
    double weight = 1.0;
    if (weights_.find(feature.first) != weights_.end()) {
      weight = weights_.at(feature.first);
    }
    AddWeightsInHashToOutput(hash, 128, weight, &floats);
  }
  if (optional_state) {
    optional_state->clear();
    optional_state->insert(optional_state->begin(), floats.begin(), floats.end());
  }
  FloatsToBits(floats, output);
}

// Add a given subgraph into the vector of floats.
void FunctionSimHasher::ProcessSubgraph(std::unique_ptr<Flowgraph>& graph,
  float graphlet_weight, address node, uint64_t bits, uint64_t hash_index,
  std::vector<float>* output_simhash_floats, std::vector<FeatureHash>*
  feature_hashes) const {

  // Calculate n-bit hash of the subgraph.
  std::vector<uint64_t> hash;
  CalculateNBitGraphHash(graph, node, bits, hash_index, &hash);

  if (feature_hashes) {
    feature_hashes->push_back(std::make_pair(hash[0], hash[1]));
  }
  // Iterate over the bits in the hash.
  AddWeightsInHashToOutput(hash, bits, graphlet_weight, output_simhash_floats);
}

// Add a given mnemonic tuple into the vector of floats.
void FunctionSimHasher::ProcessMnemTuple(const MnemTuple &tup,
  float mnem_tuple_weight, uint64_t bits, uint64_t hash_index,
  std::vector<float>* output_simhash_floats, std::vector<FeatureHash>*
  feature_hashes) const {

  std::vector<uint64_t> hash;
  CalculateNBitMnemTupleHash(tup, bits, hash_index, &hash);

  if (feature_hashes) {
    feature_hashes->push_back(std::make_pair(hash[0], hash[1]));
  }
  AddWeightsInHashToOutput(hash, bits, mnem_tuple_weight, output_simhash_floats);
}

// Iterate over an n-bit hash; add weights into the vector for each one,
// subtract the weights from the vector for each zero.
void FunctionSimHasher::AddWeightsInHashToOutput(
  const std::vector<uint64_t>& hash, uint64_t bits, float weight,
  std::vector<float>* output_simhash_floats) const {

  for (uint64_t bit_index = 0; bit_index < bits; ++bit_index) {
    if (GetNthBit(hash, bit_index)) {
      (*output_simhash_floats)[bit_index] += weight;
    } else {
      (*output_simhash_floats)[bit_index] -= weight;
    }
  }
}

// In order to facilitate the creation of a hash function family, create a
// different hash function seed given a particular hash index.
inline uint64_t FunctionSimHasher::SeedXForHashY(uint64_t seed_index,
  uint64_t hash_index) const {
  if (seed_index == 0) {
    return rotl64(seed0_, hash_index % 7) * (hash_index + 1);
  } else if (seed_index == 1) {
    return rotl64(seed1_, hash_index % 11) * (hash_index + 1);
  } else if (seed_index == 2) {
    return rotl64(seed2_, hash_index % 13) * (hash_index + 1);
  }
  printf("This function should never be called with a seed_index exceeding 2.\n");
  exit(-1);
}

// Hash a tuple of mnemonics into a 64-bit value, using a hash family index.
uint64_t FunctionSimHasher::HashMnemTuple(const MnemTuple& tup,
  uint64_t hash_index) const {
  uint64_t value1 = SeedXForHashY(0, hash_index) ^ SeedXForHashY(1, hash_index) ^
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
  if (iter == weights_.end()) {
    return standard;
  }
  return iter->second;
}

// Calculates an ID for a graphlet, taking the occurrence count of the graphlet
// into account. This is used for weight lookup.
uint64_t FunctionSimHasher::GetGraphletIdOccurrence(
  std::unique_ptr<Flowgraph>& graph, uint32_t occurrence, address node) const {
  uint64_t graphlet_id = graph->CalculateHash(node, SeedXForHashY(0, occurrence),
    SeedXForHashY(1, occurrence), SeedXForHashY(2, occurrence));

  return graphlet_id;
}

// Calculates an ID for a graphlet without taking the occurrence into account.
// This is needed so that we can count how often a given graphlet has shown up.
uint64_t FunctionSimHasher::GetGraphletIdNoOccurrence(
  std::unique_ptr<Flowgraph>& graph, address node) const {
  return HashGraph(graph, node, 0, 0);
}

uint64_t FunctionSimHasher::GetMnemonicIdNoOccurrence(const MnemTuple& tuple)
  const {
  std::vector<uint64_t> hash;
  CalculateNBitMnemTupleHash(tuple, 128, 0, &hash);
  return hash[0];
}

uint64_t FunctionSimHasher::GetMnemonicIdOccurrence(const MnemTuple& tuple,
  uint32_t occurrence) const {
  std::vector<uint64_t> hash;
  CalculateNBitMnemTupleHash(tuple, 128, occurrence, &hash);

  return hash[0];
}

// Given a vector of 64-bit values, retrieve the n-th bit.
inline bool FunctionSimHasher::GetNthBit(const std::vector<uint64_t>& nbit_hash,
  uint64_t bitindex) const {
  uint64_t index = bitindex / 64;
  uint64_t value = nbit_hash[index];
  uint64_t sub_word_index = bitindex % 64;
  return (value >> sub_word_index) & 1;
}

FunctionSimHasher::FunctionSimHasher(const std::string& weight_file,
  double default_mnemonic_weight, double default_graphlet_weight) :
    default_mnemonic_weight_(default_mnemonic_weight),
    default_graphlet_weight_(default_graphlet_weight) {
  std::vector<std::vector<std::string>> tokenized_lines;

  if (weight_file == "") {
    return;
  }
  if (FileToLineTokens(weight_file, &tokenized_lines)) {
    for (const std::vector<std::string>& line : tokenized_lines) {
      if (line.size() < 2) {
        printf("[!] Truncated line found!\n");
        continue;
      }
      double weight = strtod(line[1].c_str(), nullptr);
      if ((line[0].size() == 32) || (line[0].size() == 35)) {
        FeatureHash hash = StringToFeatureHash(line[0]);
        weights_[hash.first] = weight;
      } else if (line[0].size() == 16) {
        uint64_t value = strtoull(line[0].c_str(), nullptr, 16);
        weights_[value] = weight;
      }
    }
  }
}

FunctionSimHasher::FunctionSimHasher(std::map<uint64_t, float>* weights) :
  weights_(*weights), default_mnemonic_weight_(0.05),
  default_graphlet_weight_(1.0) {
  weights_ = *weights;
}


