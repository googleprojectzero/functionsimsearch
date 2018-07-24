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

  bool contains_function = (disassembly.GetIndexByAddress(address) 
    != std::numeric_limits<uint32_t>::max());

  if (!contains_function) {
    // Make sure the function-to-index is in fact getting indexed.
    printf("[!] Warning: Did not find %lx during auto-analysis of %s, adding.\n",
      address, filename.c_str());
    disassembly.DisassembleFromAddress(address, true);
  }

  if (disassembly.GetNumberOfFunctions() == 0) {
    return std::make_pair(0,0);
  }

  uint32_t index = disassembly.GetIndexByAddress(address);
  std::unique_ptr<Flowgraph> graph = disassembly.GetFlowgraph(index);
  std::unique_ptr<FunctionFeatureGenerator> generator =
    disassembly.GetFeatureGenerator(index);
  std::vector<uint64_t> hashes;
  hasher.CalculateFunctionSimHash(generator.get(), 128, &hashes, feature_hashes);
  return std::make_pair(hashes.at(0), hashes.at(1));
}

