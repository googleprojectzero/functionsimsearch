#include <sstream>

#include "third_party/PicoSHA2/picosha2.h"

#include "disassembly/disassembly.hpp"
#include "disassembly/dyninstfeaturegenerator.hpp"
#include "disassembly/flowgraph.hpp"
#include "disassembly/flowgraphutil.hpp"
#include "searchbackend/functionsimhash.hpp"
#include "util/mappedtextfile.hpp"
#include "util/util.hpp"

std::vector<std::string> Tokenize(const char *str, const char c) {
  std::vector<std::string> result;
  do {
    const char *begin = str;
    while(*str != c && *str) {
      str++;
    }
    result.push_back(std::string(begin, str));
  } while (0 != *str++);
  return result;
}

// Obtain the first 64 bits of the input file's SHA256 hash.
uint64_t GenerateExecutableID(const std::string& filename) {
  std::ifstream ifs(filename.c_str(), std::ios::binary);
  std::vector<unsigned char> hash(32);

  if (!ifs.is_open()) {
    printf("[E] Could not open executable %s\n", filename.c_str());
    exit(-1);
  }

  picosha2::hash256(
    std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>(), 
    hash.begin(), hash.end());

  uint64_t *temp = reinterpret_cast<uint64_t*>(&hash[0]);
  return __builtin_bswap64(*temp);
}

uint32_t HammingDistance(uint64_t A1, uint64_t A2, uint64_t B1, uint64_t B2) {
  uint32_t distance =
    __builtin_popcountl(A1 ^ B1) +
    __builtin_popcountl(A2 ^ B2);
  return distance;
}

uint32_t HammingDistance(FeatureHash A, FeatureHash B) {
  return HammingDistance(A.first, A.second, B.first, B.second);
}

uint32_t ReadFeatureSet(MappedTextFile* input, std::set<FeatureHash>* result) {
  uint32_t linecount = 0;
  auto lines = input->GetLineIterator();
  while (lines.HasMore()) {
    auto words = input->GetWordIterator(lines);
    ++words;
    while (words.HasMore()) {
      result->insert(StringToFeatureHash(words.Get()));
      ++words;
    }
    ++lines;
    ++linecount;
    if ((linecount % 1000) == 0) {
      printf("[!] Parsed %d lines, saw %ld features ...\n", linecount,
        result->size());
    }
  }
  return linecount;
}

void ReadFeatureSet(const std::vector<std::vector<std::string>>& inputlines,
  std::set<FeatureHash>* result) {
  for (const std::vector<std::string>& line : inputlines) {
    for (uint32_t index = 1; index < line.size(); ++index) {
      result->insert(StringToFeatureHash(line[index]));
    }
  }
}

bool FileToLineTokens(const std::string& filename,
  std::vector<std::vector<std::string>>* tokenized_lines) {
  std::ifstream inputfile(filename);
  if (!inputfile) {
    return false;
  }

  std::string line;
  while (std::getline(inputfile, line)) {
    std::vector<std::string> tokens;
    split(line, ' ', std::back_inserter(tokens));
    tokenized_lines->push_back(tokens);
  }
  return true;
}

FeatureHash StringToFeatureHash(const std::string& hash_as_string) {
  const std::string& token = hash_as_string;
  std::string first_half_string;
  std::string second_half_string;
  if ((token.size() != 32) && (token.size() != 35)) {
    printf("[E] Broken token: '%s'\n", token.c_str());
    return std::make_pair(0, 0);
  }

  if (token.c_str()[2] == '.') {
    first_half_string = token.substr(3, 16);
    second_half_string = token.substr(16+3, std::string::npos);
  } else {
    first_half_string = token.substr(0, 16);
    second_half_string = token.substr(16, std::string::npos);
  }
  const char* first_half = first_half_string.c_str();
  const char* second_half = second_half_string.c_str();
  uint64_t hashA = strtoull(first_half, nullptr, 16);
  uint64_t hashB = strtoull(second_half, nullptr, 16);

  return std::make_pair(hashA, hashB);
}


