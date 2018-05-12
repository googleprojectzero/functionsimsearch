#ifndef UTIL_HPP
#define UTIL_HPP

#include <cstdint>
#include <fstream>
#include <set>
#include <string>
#include <sstream>
#include <vector>

#include "util/mappedtextfile.hpp"

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

uint32_t ReadFeatureSet(MappedTextFile* input, std::set<FeatureHash>* result);
void ReadFeatureSet(const std::vector<std::vector<std::string>>& inputlines,
  std::set<FeatureHash>* result);

std::vector<std::string> Tokenize(const char *str, const char c);

#endif // UTIL_HPP
