#ifndef UTIL_HPP
#define UTIL_HPP

#include <cstdint>
#include <fstream>
#include <set>
#include <string>
#include <sstream>
#include <vector>

class FunctionSimHasher;

// The hash of an individual feature.
typedef std::pair<uint64_t, uint64_t> FeatureHash;

// A vector of indices into a FeatureHash vector represents a function.
typedef std::vector<uint32_t> FunctionFeatures;

uint64_t GenerateExecutableID(const std::string& filename);
uint32_t HammingDistance(uint64_t A1, uint64_t A2, uint64_t B1, uint64_t B2);
uint32_t HammingDistance(FeatureHash A, FeatureHash B);

// String split functionality.
template<typename Out>
void split(const std::string &s, char delim, Out result) {
  std::stringstream ss;
  ss.str(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    *(result++) = item;
  }
}

bool FileToLineTokens(const std::string& filename,
  std::vector<std::vector<std::string>>* tokenized_lines);

FeatureHash StringToFeatureHash(const std::string& hash_as_string);

void ReadFeatureSet(const std::vector<std::vector<std::string>>& inputlines,
  std::set<FeatureHash>* result);

// A helper function mostly for the benefit of testing. Iterates through the
// entire disassembly, so this should not be used in a loop.
FeatureHash GetHashForFileAndFunction(FunctionSimHasher& hasher,
  const std::string& filename, const std::string& mode, uint64_t address,
  std::vector<FeatureHash>* feature_hashes = nullptr);

#endif // UTIL_HPP
