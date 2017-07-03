
typedef std::tuple<std::string, std::string, std::string> MnemTuple;

// Calculates an output vector of floats from the underlying function.
// SimHash works by hashing individual elements, and adding up weights in
// vectors with the same bits.
void FunctionSimHasher::CalculateFunctionSimHash(
  Dyninst::ParseAPI::Function* function, uint64_t number_of_outputs,
  uint64_t simhash_index, std::vector<float>* output_simhash_floats) {
  output_simhash_floats.resize(number_of_outputs);

  Flowgraph graph;
  BuildFlowgraph(function, &graph);

  // Process subgraphs.
  for (const address node : nodes) {
    // Calculate a number_of_outputs-bit-hash of the subgraphs.
    std::unique_ptr<Flowgraph> distance_2(graph.GetSubgraph(node, 2, 30));
    std::unique_ptr<Flowgraph> distance_3(graph.GetSubgraph(node, 3, 30));

    ProcessSubgraph(distance_2, node, number_of_outputs, simhash_index,
      output_simhash_floats);
    ProcessSubgraph(distance_3, node, number_of_outputs, simhash_index,
      output_simhash_floats);
  }

  // Process mnemonic n-grams.
  std::vector<MnemTuple> tuples;
  BuildMnemonicNgrams(function, &tuples);
  for (const MnemTuple& tuple : tuples) {
    ProcessTuple(tuple, number_of_outputs, output_simhash_floats);
  }
}

// Add a given subgraph into the vector of floats.
void FunctionSimHasher::ProcessSubgraph(std::unique_ptr<Flowgraph>& graph,
  address node, uint64_t bits, std::vector<float>* output_simhash_floats) {
  // Retrieve the saved weight of the subgraph graphlet.
  float graphlet_weight = GetGraphletWeight(graph, node);

  // Calculate n-bit hash of the subgraph.
  std::vector<uint64_t> hash;
  CalculateNBitGraphHash(graph, node, bits, hash_index, &hash);

  // Iterate over the bits in the hash.
  AddWeightsInHashToOutput(hash, bits, graphlet_weight, output_simhash_floats);
}

// Add a given mnemonic tuple into the vector of floats.
void FunctionSimHasher::ProcessMnemTuple(const MnmemTuple tup&, uint64_t bits,
  uint64_t hash_index, std::vector<float>* output_simhash_floats) {
  float mnem_tuple_weight = GetMnemTupleWeight(tup);

  std::vector<uint64_t> hash;
  CalculateNBitMnemTupleHash(tup, bits, hash_index, output_simhash_floats);

  AddWeightsInHashToOutput(hash, bits, mnem_tuple_weight, hash_index,
    output_simhash_floats);
}

// Iterate over an n-bit hash; add weights into the vector for each one,
// subtract the weights from the vector for each zero.
void FunctionSimHasher::AddWeightsInHashToOutput(
  const std::vector<uint64_t>& hash, uint64_t bits, float weight,
  std::vector<float>* output_simhash_floats) {

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
    return rotl64(seed0_, hash_index % 7);
  } else if (seed_index == 1) {
    return rotl64(seed1_, hash_index % 11);
  } else if (seed_index == 2) {
    return rotl64(seed2_, hash_index % 13);
  }
}

// Hash a tuple of mnemonics into a 64-bit value, using a hash family index.
uint64_t FunctionSimHasher::HashMnemTuple(const MnemTuple& tup,
  uint64_t hash_index) {
  size_t value1 = SeedXForHashY(0, hash_index) ^ SeedXForHashY(1, hash_index) ^
    SeedXForHashY(2, hash_index);
  value1 *= std::hash<std::string>{}(std::get<0>(tup));
  value1 = rotl64(value1, 7);
  value1 *= std::hash<std::string>{}(std::get<1>(tup));
  value1 = rotl64(value1, 7);
  value1 *= std::hash<std::string>{}(std::get<2>(tup));
  value1 = rotl64(value1, 7);
  value1 *= (k2 * hash_index);
  return value1;
}

// Hash a graph into a 64-bit value, using a hash family index.
uint64_t FunctionSimHasher::HashGraph(std::unique_ptr<Flowgraph>& graph,
  address node, uint64_t hash_index, uint64_t counter) {
  return graph->CalculateHash(node,
    SeedXForHashY(0, hash_index) * (counter + 1),
    SeedXForHashY(1, hash_index) * (counter + 1),
    SeedXForHashY(2, hash_index) * (counter + 1));
}

// Extend a 64-bit graph hash family to a N-bit hash family by just increasing
// the hash function index.
void FunctionSimHasher::CalculateNBitGraphHash(
  std::unique_ptr<Flowgraph>& graph, address node, uint64_t bits,
  uint64_t hash_index, std::vector<uint64_t>* output) {
  output->clear();

  for (uint64_t counter = 0; counter < bits; counter += 64) {
    output->push_back(HashGraph(graph, node, hash_index, counter));
  }
}

// Extend a 64-bit mnemonic tuple hash family to a N-bit hash family by just
// increasing the hash function index.
void FunctionSimHasher::CalculateNBitMnemTupleHash(
  const MnemTuple& tup, uint64_t bits, uint64_t hash_index,
  std::vector<uint64_t>* output) {
  output->clear();

  for (uint64_t counter = 0; counter < bits; counter += 64) {
    output->push_back(HashMnemTuple(tup, hash_index + (counter + 1)));
  }
}

// Return a weight for a given key, default to 1.0.
float FunctionSimHasher::GetWeight(uint64_t key) {
  const auto& iter = weights_.find(key);
  return (iter == weights_.end()) ? 1.0 : *iter;
}

// Hash the graph under the first hash function of the family, then see if a
// pre-computed weight is loaded.
float FunctionSimHasher::GetGraphWeight(std::unique_ptr<Flowgraph>& graph,
  address node) {
  uint64_t graphlet_id = graph->CalculateHash(node,
    SeedXForHashY(0, 0), SeedXForHashY(0, 1), SeedXForHashY(0, 2));
  return GetWeight(graphlet_id);
}

float FunctionSimHasher::GetMnemTupleWeight(const MnemTuple& tup) {
  uint64_t mnem_id = HashMnemTuple(tup, hash_index);
  return GetWeight(mnem_id);
}

// Given a vector of 64-bit values, retrieve the n-th bit.
inline bool FunctionSimHasher::GetNthBit(const std::vector<uint64_t>& nbit_hash,
  uint64_t bitindex) {
  uint64_t index = bitindex / 64;
  uint64_t value = nbit_hash[index];
  uint64_t sub_word_index = bitindex % 64;
  return (value >> sub_word_index) & 1;
}

// Given a Dyninst function, create the set of instruction 3-grams.
void FunctionSimHasher::BuildMnemonicNgrams(Dyninst::ParseAPI::Function* function,
  std::vector<MnemTuple>* tuples) {
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
  // Construct the 3-tuples.
  for (uint64_t index = 0; index + 3 < sequence.size(); ++index) {
    tuples->emplace_back(std::make_tuple(
      sequence[index].second,
      sequence[index+1].second,
      sequence[index+2].second));
  }
}

FunctionSimHasher::FunctionSimHasher(const std::string& weight_file) {


}
