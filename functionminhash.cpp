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

#include <functional>

#include "CodeObject.h"
#include "InstructionDecoder.h"

#include "flowgraph.hpp"
#include "flowgraphutil.hpp"
#include "functionminhash.hpp"

uint32_t TruncateValue(uint32_t value, char bits) {
  uint32_t mask = (1UL << bits)-1;
  return value & mask;
}

typedef std::tuple<std::string, std::string, std::string> MnemTuple;
typedef std::tuple<MnemTuple, uint64_t, uint64_t> HashableTuple;

uint32_t HashMnemTuple(const MnemTuple& tup, uint64_t hash_index) {
  size_t value1 = std::hash<std::string>{}(std::get<0>(tup));
  value1 = rotl64(value1, 7);
  value1 *= std::hash<std::string>{}(std::get<1>(tup));
  value1 = rotl64(value1, 7);
  value1 *= std::hash<std::string>{}(std::get<2>(tup));
  value1 = rotl64(value1, 7);
  value1 *= (k2 * hash_index);
  return value1;
}

void CalculateFunctionFingerprint(Dyninst::ParseAPI::Function* function,
  uint64_t cfg_hashes, uint64_t mnem_hashes, uint32_t bits_per_hash,
  std::vector<uint32_t>* output) {

  // Build the flowgraph.
  Flowgraph graph;
  BuildFlowgraph(function, &graph);

  // Resize output appropriately.
  output->resize(cfg_hashes + mnem_hashes, 0xFFFFFFFFL);

  std::vector<address> nodes;
  graph.GetNodes(&nodes);

  // Begin the calculation of the cfg hashes. Treat every node in the graph as
  // center for graphlets, once with distance 2, once with distance 3. Note that
  // this code treats the graph patches of the CFG as set, not as multiset.
  // This may be a mistake and could be rectified easily, but it is not yet
  // clear whether it has much of an impact.
  for (const address node : nodes) {
    std::unique_ptr<Flowgraph> distance_2(graph.GetSubgraph(node, 2, 30));
    std::unique_ptr<Flowgraph> distance_3(graph.GetSubgraph(node, 3, 30));
    // Hash with each of the hash functions and update the entries in the vector.
    for (uint64_t hash_index = 0; hash_index < cfg_hashes; ++hash_index) {
      if (distance_2.get() != nullptr) {
        (*output)[hash_index] = std::min((*output)[hash_index],
          TruncateValue(
            static_cast<uint32_t>(
              distance_2->CalculateHash(node, k0, k1, hash_index*k2)),
          bits_per_hash));
      }
      if (distance_3.get() != nullptr) {
        (*output)[hash_index] = std::min((*output)[hash_index],
          TruncateValue(
            static_cast<uint32_t>(
              distance_3->CalculateHash(node, k0, k1, hash_index*k2)),
          bits_per_hash));
      }
    }
  }

  // Perform the calculation of the mnemonic-n-gram-hashes.
  std::vector<std::pair<address, std::string>> sequence;
  for (const auto& block : function->blocks()) {
    Dyninst::ParseAPI::Block::Insns block_instructions;
    block->getInsns(block_instructions);
    for (const auto& instruction : block_instructions) {
      auto& op = instruction.second->getOperation();
      sequence.push_back(std::make_pair(instruction.first, op.format()));
    }
  }
  // Sort instructions by address;
  std::sort(sequence.begin(), sequence.end());
  // Count how often each 3-tuple occurs in the function.
  std::map<MnemTuple, uint64_t> occurrence;
  for (uint64_t index = 0; index + 3 < sequence.size(); ++index) {
    MnemTuple tup = std::make_tuple(
      sequence[index].second,
      sequence[index+1].second,
      sequence[index+2].second);
    occurrence[tup]++;
  }
  // Now calculate the minhash entries.
  for (const auto& entry : occurrence) {
    for (uint64_t hash_index = 0; hash_index < mnem_hashes; ++hash_index) {
      uint32_t value = TruncateValue(
        HashMnemTuple(entry.first, hash_index), bits_per_hash);
      uint64_t output_index = hash_index + cfg_hashes;
      (*output)[output_index] = std::min((*output)[output_index],
        value);
    }
  }
}

