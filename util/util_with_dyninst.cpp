// DynInst headers.
#include "CodeObject.h"
#include "InstructionDecoder.h"

#include "disassembly/disassembly.hpp"
#include "disassembly/dyninstfeaturegenerator.hpp"
#include "disassembly/flowgraph.hpp"
#include "disassembly/flowgraphutil_dyninst.hpp"
#include "searchbackend/functionsimhash.hpp"
#include "util/mappedtextfile.hpp"
#include "util/util.hpp"
#include "util/util_with_dyninst.hpp"

// A file with utility functions, formerly from util.cpp, that have DynInst
// dependencies.

FeatureHash GetHashForFileAndFunction(FunctionSimHasher& hasher,
  const std::string& filename, const std::string& mode, uint64_t address,
  std::vector<FeatureHash>* feature_hashes) {
  Disassembly disassembly(mode, filename);
  if (!disassembly.Load()) {
    printf("Failure to load %s\n", filename.c_str());
    return std::make_pair(0, 0);
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

