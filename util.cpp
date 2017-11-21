// DynInst headers.
#include "CodeObject.h"
#include "InstructionDecoder.h"
#include "third_party/PicoSHA2/picosha2.h"

#include "disassembly.hpp"
#include "flowgraph.hpp"
#include "flowgraphutil.hpp"
#include "functionsimhash.hpp"
#include "dyninstfeaturegenerator.hpp"
#include "util.hpp"

// Obtain the first 64 bits of the input file's SHA256 hash.
uint64_t GenerateExecutableID(const std::string& filename) {
  std::ifstream ifs(filename.c_str(), std::ios::binary);
  std::vector<unsigned char> hash(32);

  if (!ifs.is_open()) {
    printf("[E] Could not open executable %s\n", filename.c_str());
    exit(-1);
  }
  picosha2::hash256(std::istreambuf_iterator<char>(ifs),
      std::istreambuf_iterator<char>(), hash.begin(), hash.end());

  uint64_t *temp = reinterpret_cast<uint64_t*>(&hash[0]);
  return __builtin_bswap64(*temp);
}

uint32_t HammingDistance(uint64_t A1, uint64_t A2, uint64_t B1, uint64_t B2) {
  uint32_t distance =
    __builtin_popcount(A1 ^ B1) +
    __builtin_popcount(A2 ^ B2);
  return distance;
}

uint32_t HammingDistance(FeatureHash A, FeatureHash B) {
  return HammingDistance(A.first, A.second, B.first, B.second);
}

void ReadFeatureSet(const std::vector<std::vector<std::string>>& inputlines,
  std::set<FeatureHash>* result) {
  for (const std::vector<std::string>& line : inputlines) {
    for (uint32_t index = 1; index < line.size(); ++index) {
      FeatureHash foo = StringToFeatureHash(line[index]);
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

  uint32_t line_index = 0;
  // We want features sorted and de-duplicated in the end, so use a set.
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
    printf("[E] Broken token: %s\n", token.c_str());
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

FeatureHash GetHashForFileAndFunction(FunctionSimHasher& hasher,
  const std::string& filename, const std::string& mode, uint64_t address,
  std::vector<FeatureHash>* feature_hashes) {
  Disassembly disassembly(mode, filename);
  if (!disassembly.Load()) {
    exit(1);
  }
  Dyninst::ParseAPI::CodeObject* code_object = disassembly.getCodeObject();

  // Obtain the list of all functions in the binary.
  const Dyninst::ParseAPI::CodeObject::funclist &functions =
    code_object->funcs();

  bool contains_function = false;
  for (Dyninst::ParseAPI::Function* function : functions) {
    Dyninst::Address function_address = function->addr();
    if (function_address == address) {
      contains_function = true;
      break;
    }
  }

  if (!contains_function) {
    // Make sure the function-to-index is in fact getting indexed.
    printf("[!] Warning: Did not find %lx during auto-analysis of %s, adding.\n",
      address, filename.c_str());
    disassembly.DisassembleFromAddress(address, true);
  }

  if (functions.size() == 0) {
    return std::make_pair(0,0);
  }

  for (Dyninst::ParseAPI::Function* function : functions) {
    Flowgraph graph;
    Dyninst::Address function_address = function->addr();
    if (function_address == address) {
      BuildFlowgraph(function, &graph);

      std::vector<uint64_t> hashes;
      DyninstFeatureGenerator generator(function);
      hasher.CalculateFunctionSimHash(&generator, 128, &hashes, feature_hashes);
      uint64_t hash_A = hashes[0];
      uint64_t hash_B = hashes[1];
      return std::make_pair(hash_A, hash_B);
    }
  }
  return std::make_pair(0,0);
}

