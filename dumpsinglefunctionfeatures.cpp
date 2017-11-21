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

#include "CodeObject.h"
#include "InstructionDecoder.h"

#include "disassembly.hpp"
#include "dyninstfeaturegenerator.hpp"
#include "flowgraph.hpp"
#include "flowgraphutil.hpp"
#include "functionsimhash.hpp"
#include "simhashsearchindex.hpp"
#include "pecodesource.hpp"
#include "threadpool.hpp"

DEFINE_string(format, "PE", "Executable format: PE or ELF");
DEFINE_string(input, "", "File to disassemble");
DEFINE_string(function_address, "", "Address of function");
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
  CodeObject* code_object = disassembly.getCodeObject();

  const CodeObject::funclist &functions = code_object->funcs();
  if (functions.size() == 0) {
    printf("No functions found.\n");
    return -1;
  }

  std::mutex dyninst_mutex;
  std::mutex* mutex_pointer = &dyninst_mutex;
  threadpool::ThreadPool pool(std::thread::hardware_concurrency());
  uint64_t number_of_functions = functions.size();
  FunctionSimHasher hasher("weights.txt");

  // Given that we are only indexing one function, the following code is clearly
  // overkill.
  for (Function* function : functions) {
    pool.Push(
      [mutex_pointer, &binary_path_string, &hasher, file_id, function,
        target_address](int threadid) {
      Address function_address = function->addr();
      if (function_address != target_address) {
        return;
      }
      std::vector<uint64_t> hashes;
      mutex_pointer->lock();
      DyninstFeatureGenerator generator(function);
      mutex_pointer->unlock();

      std::vector<FeatureHash> feature_ids;
      hasher.CalculateFunctionSimHash(&generator, 128, &hashes, &feature_ids);
      for (FeatureHash feature : feature_ids) {
        printf("%16.16lx%16.16lx ", feature.first, feature.second);
      }
      uint64_t hash_A = hashes[0];
      uint64_t hash_B = hashes[1];
    });
  }
  pool.Stop(true);
  printf("\n");
}
