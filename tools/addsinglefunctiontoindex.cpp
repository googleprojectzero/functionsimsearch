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

#include <fstream>
#include <iostream>
#include <map>
#include <gflags/gflags.h>

#include "disassembly/disassembly.hpp"
#include "disassembly/dyninstfeaturegenerator.hpp"
#include "disassembly/flowgraph.hpp"
#include "disassembly/flowgraphutil_dyninst.hpp"
#include "searchbackend/functionsimhash.hpp"
#include "searchbackend/simhashsearchindex.hpp"
#include "disassembly/pecodesource.hpp"
#include "util/threadpool.hpp"
#include "util/util.hpp"

DEFINE_string(format, "PE", "Executable format: PE,ELF,JSON");
DEFINE_string(input, "", "File to disassemble");
DEFINE_string(index, "./similarity.index", "Index file");
DEFINE_string(weights, "weights.txt", "Feature weights file");
DEFINE_string(function_address, "", "Address of the function");
// The google namespace is there for compatibility with legacy gflags and will
// be removed eventually.
#ifndef gflags
using namespace google;
#else
using namespace gflags;
#endif

using namespace std;

int main(int argc, char** argv) {
  SetUsageMessage(
    "Add a single functions of the input executable to the search index.");
  ParseCommandLineFlags(&argc, &argv, true);

  std::string mode(FLAGS_format);
  std::string binary_path_string(FLAGS_input);
  std::string index_file(FLAGS_index);
  uint64_t target_address = strtoul(FLAGS_function_address.c_str(), nullptr, 16);

  uint64_t file_id = GenerateExecutableID(binary_path_string);
  printf("[!] Executable id is %16.16lx\n", file_id);

  // Load the search index.
  SimHashSearchIndex search_index(index_file, false);

  Disassembly disassembly(mode, binary_path_string);
  if (!disassembly.Load()) {
    exit(1);
  }
  // Make sure the function-to-index is in fact getting indexed.
  disassembly.DisassembleFromAddress(target_address, true);

  if (disassembly.GetNumberOfFunctions() == 0) {
    printf("No functions found.\n");
    return -1;
  }

  FunctionSimHasher hasher("weights.txt");
  uint32_t index = disassembly.GetIndexByAddress(target_address);
  if (index == std::numeric_limits<uint32_t>::max()) {
    printf("Specified function not found.\n");
    return -1;
  }
  std::unique_ptr<Flowgraph> graph = disassembly.GetFlowgraph(index);

  if (search_index.GetIndexFileFreeSpace() < (1ULL << 14)) {
    printf("[!] (1/1) %s FileID %lx: Skipping function %lx. Index file "
      "full.\n", binary_path_string.c_str(), file_id, target_address);
    return -1;
  }
  uint64_t branching_nodes = graph->GetNumberOfBranchingNodes();

  printf("[!] (1/1) %s FileID %lx: Adding function %lx (%lu branching "
    "nodes)\n", binary_path_string.c_str(), file_id, target_address,
    branching_nodes);

  std::vector<uint64_t> hashes;
  std::unique_ptr<FunctionFeatureGenerator> generator =
    disassembly.GetFeatureGenerator(index);
  hasher.CalculateFunctionSimHash(generator.get(), 128, &hashes);
  uint64_t hash_A = hashes[0];
  uint64_t hash_B = hashes[1];
  try {
    search_index.AddFunction(hash_A, hash_B, file_id, target_address);
  } catch (boost::interprocess::bad_alloc& out_of_space) {
    printf("[!] boost::interprocess::bad_alloc - no space in index file left!\n");
  }
}
