#ifndef FUNCTIONSIMHASH_HPP
#define FUNCTIONSIMHASH_HPP

#include <map>
#include <memory>
#include <string>
#include <vector>

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
// The weights themselves are given in a persistent std::map. The keys in this
// map are the hash values of the feature using the first hash function of the
// hash family (subsequent hash functions are used for the calculations).
class FunctionSimHasher {
public:
  // The weight_file is a simple memory-mapped map that maps uint64_t IDs for
  // a feature to float weights.
  FunctionSimHasher(const std::string& weight_file);

  // Calculate a simhash value for a given function. Return a vector of floats.
  // The reason why this does not return the compressed vector of bits is that
  // returning the floats allows easy sim-hashing of groups of functions, too,
  // and combining the sim-hash with other tihngs that are not derived from the
  // function itself.
  //
  // simhash_index is an index that allows the calculation of many ent
  // sim-hashes.
  void CalculateFunctionSimHash(
    Dyninst::ParseAPI::Function* function, uint64_t number_of_outputs,
    uint64_t simhash_index, std::vector<float>* output_simhash_floats);

private:
  // Process one subgraph and hash it into the output vector.
  void ProcessSubgraph(std::unique_ptr<Flowgraph>& graph, address node,
    uint64_t bits, std::vector<float>* output_simhash_floats) const;

  // Process one mnemonic n-gram and hash it into the output vector.
  void ProcessMnemTuple(const MnmemTuple tup&, uint64_t bits,
    uint64_t hash_index, std::vector<float>* output_simhash_floats) const;

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
  float GetWeight(uint64_t key) const;
  float GetGraphWeight(std::unique_ptr<Flowgraph>& graph, address node) const;
  float GetMnemTupleWeight(const MnemTuple& tup) const;

  inline bool GetNthBit(const std::vector<uint64_t>& nbit_hash,
    uint64_t bitindex) const;

  // Given a Dyninst function, create the set of instruction 3-grams.
  void BuildMnemonicNgrams(Dyninst::ParseAPI::Function* function,
    int64_t HashMnemTuple(const MnemTuple& tup, uint64_t hash_index) const;

  std::map<uint64_t, float> weights_;

  // Some primes between 2^63 and 2^64 from CityHash.
  static constexpr uint64_t seed0_ = 0xc3a5c85c97cb3127ULL;
  static constexpr uint64_t seed1_ = 0xb492b66fbe98f273ULL;
  static constexpr uint64_t seed2_ = 0x9ae16a3b2f90404fULL;
}

#endif FUNCTIONSIMHASH_HPP
