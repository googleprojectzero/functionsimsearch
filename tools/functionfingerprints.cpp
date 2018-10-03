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
#include "disassembly/flowgraph.hpp"
#include "disassembly/flowgraphutil_dyninst.hpp"
#include "searchbackend/functionsimhash.hpp"
#include "disassembly/pecodesource.hpp"
#include "util/util.hpp"

DEFINE_string(format, "PE", "Executable format: PE,ELF,JSON");
DEFINE_string(input, "", "File to disassemble");
DEFINE_uint64(minimum_function_size, 5, "Minimum size of a function to be added.");
DEFINE_bool(verbose, false, "Verbose output");
DEFINE_string(weights, "", "Feature weights file");
DEFINE_string(function_address, "", "Address of function");
DEFINE_bool(disable_graphs, false, "Disable graphs as features");
DEFINE_bool(disable_instructions, false, "Disable instructions as features");
DEFINE_bool(no_shared_blocks, false, "Skip functions with shared blocks.");
DEFINE_bool(dump_graphlet_features, false, "Dump graphlet features into /var/tmp.");
DEFINE_bool(dump_instruction_features, false, "Dump instruction features into /var/tmp.");

//
// The google namespace is there for compatibility with legacy gflags and will
// be removed eventually.
#ifndef gflags
using namespace google;
#else
using namespace gflags;
#endif

using namespace std;
using namespace Dyninst;
using namespace ParseAPI;
using namespace InstructionAPI;

int main(int argc, char** argv) {
  SetUsageMessage(
    "Dump simhash values from a binary to stdout. If verbose is true, "
    "dump the feature IDs for the simhash values. This is used to curate "
    "training data for training feature weights.");
  ParseCommandLineFlags(&argc, &argv, true);

  uint64_t target_address = strtoul(FLAGS_function_address.c_str(), nullptr, 16);

  std::string mode(FLAGS_format);
  std::string binary_path_string(FLAGS_input);
  uint64_t minimum_size = FLAGS_minimum_function_size;
  bool verbose = FLAGS_verbose;

  uint64_t file_id = GenerateExecutableID(binary_path_string);

  Disassembly disassembly(mode, binary_path_string);
  if (!disassembly.Load()) {
    exit(1);
  }

  if (disassembly.GetNumberOfFunctions() == 0) {
    printf("No functions found.\n");
    return -1;
  }

  FunctionSimHasher sim_hasher(FLAGS_weights, FLAGS_disable_graphs,
    FLAGS_disable_instructions, FLAGS_dump_graphlet_features,
    FLAGS_dump_instruction_features);

  for (uint32_t index = 0; index < disassembly.GetNumberOfFunctions(); ++index) {
    std::unique_ptr<FlowgraphWithInstructions> graph =
      disassembly.GetFlowgraphWithInstructions(index);
    Address function_address = disassembly.GetAddressOfFunction(index);

    // Skip functions that contain shared basic blocks.
    if (FLAGS_no_shared_blocks && disassembly.ContainsSharedBasicBlocks(index)) {
      continue;
    }

    if ((target_address != 0) && (target_address != function_address)) {
      continue;
    }

    uint64_t branching_nodes = graph->GetNumberOfBranchingNodes();

    if (branching_nodes <= minimum_size) {
      continue;
    }

    // Dump out the file ID and address.
    printf("%16.16lx:%16.16lx ", file_id, function_address);

    // If we are in verbose mode, the following lines will dump out the
    // individual feature hashes.
    std::vector<FeatureHash> feature_hashes;
    std::vector<uint64_t> hashes;
    std::unique_ptr<FunctionFeatureGenerator> generator =
      disassembly.GetFeatureGenerator(index);
    sim_hasher.CalculateFunctionSimHash(generator.get(), 128, &hashes,
      &feature_hashes);

    uint64_t hash1 = hashes[0];
    uint64_t hash2 = hashes[1];

    // Dump out the final simhash of the function only if we are in non-verbose
    // mode.
    if (!verbose) {
      printf("%16.16lx%16.16lx\n", hash1, hash2);
    } else {
      for (const auto& feature_hash : feature_hashes) {
        printf("%16.16lx%16.16lx ", feature_hash.first, feature_hash.second);
      }
      printf("\n");
    }
  }
}
