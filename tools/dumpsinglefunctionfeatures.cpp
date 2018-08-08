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
#include "disassembly/flowgraphutil.hpp"
#include "searchbackend/functionsimhash.hpp"
#include "searchbackend/simhashsearchindex.hpp"
#include "disassembly/pecodesource.hpp"
#include "util/threadpool.hpp"

DEFINE_string(format, "PE", "Executable format: PE,ELF,JSON");
DEFINE_string(input, "", "File to disassemble");
DEFINE_string(function_address, "", "Address of function");
DEFINE_bool(no_shared_blocks, false, "Skip functions with shared blocks.");

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
    "Dump feature hashes for a given function to stdout.");
  ParseCommandLineFlags(&argc, &argv, true);

  std::string mode(FLAGS_format);
  std::string binary_path_string(FLAGS_input);
  uint64_t target_address = strtoul(FLAGS_function_address.c_str(), nullptr, 16);

  uint64_t file_id = GenerateExecutableID(binary_path_string);
  printf("%16.16lx:%16.16lx ", file_id, target_address);

  Disassembly disassembly(mode, binary_path_string);
  if (!disassembly.Load()) {
    exit(1);
  }
  // Make sure the function-to-index is in fact getting indexed.
  disassembly.DisassembleFromAddress(target_address, true);

  // Find the function that has the right address.
  uint32_t index = disassembly.GetIndexByAddress(target_address);

  // The weights are not used, so ignore them (and use hardcoded weights.txt.
  FunctionSimHasher hasher("weights.txt");

  if (FLAGS_no_shared_blocks && disassembly.ContainsSharedBasicBlocks(index)) {
    printf("[!] Error: --no_shared_blocks was specified but function has them.");
    exit(1);
  }

  std::unique_ptr<FunctionFeatureGenerator> generator =
    disassembly.GetFeatureGenerator(index);

  std::vector<FeatureHash> feature_ids;
  std::vector<uint64_t> hashes;
  hasher.CalculateFunctionSimHash(generator.get(), 128, &hashes, &feature_ids);
  for (FeatureHash feature : feature_ids) {
    printf("%16.16lx%16.16lx ", feature.first, feature.second);
  }
}
