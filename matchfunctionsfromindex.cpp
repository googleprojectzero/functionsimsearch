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

#include "CodeObject.h"
#include "InstructionDecoder.h"
#include "third_party/PicoSHA2/picosha2.h"

#include "disassembly.hpp"
#include "flowgraph.hpp"
#include "flowgraphutil.hpp"
#include "functionminhash.hpp"
#include "minhashsearchindex.hpp"
#include "pecodesource.hpp"

using namespace std;
using namespace Dyninst;
using namespace ParseAPI;
using namespace InstructionAPI;

// Obtain the first 64 bits of the input file's SHA256 hash.
uint64_t GenerateExecutableID(const std::string& filename) {
  std::ifstream ifs(filename.c_str(), std::ios::binary);
  std::vector<unsigned char> hash(32);
  picosha2::hash256(std::istreambuf_iterator<char>(ifs),
      std::istreambuf_iterator<char>(), hash.begin(), hash.end());

  uint64_t *temp = reinterpret_cast<uint64_t*>(&hash[0]);
  return __builtin_bswap64(*temp);
}

int main(int argc, char** argv) {
  if (argc != 6) {
    printf("Match minhash vectors from a binary against a search index\n");
    printf("Usage: %s <PE/ELF> <binary path> <index file> <minimum function size> <max_matches>\n", argv[0]);
    return -1;
  }

  std::string mode(argv[1]);
  std::string binary_path_string(argv[2]);
  std::string index_file(argv[3]);
  uint64_t minimum_size = strtoul(argv[4], nullptr, 10);
  uint64_t max_matches = strtoul(argv[5], nullptr, 10);

  uint64_t file_id = GenerateExecutableID(binary_path_string);
  printf("[!] Executable id is %lx\n", file_id);

  // Load the search index.
  MinHashSearchIndex search_index(index_file, false);

  printf("[!] Loaded search index, starting disassembly.\n");

  Disassembly disassembly(mode, binary_path_string);
  if (!disassembly.Load()) {
    exit(1);
  }
  CodeObject* code_object = disassembly.getCodeObject();

  // Obtain the list of all functions in the binary.
  const CodeObject::funclist &functions = code_object->funcs();
  if (functions.size() == 0) {
    printf("No functions found.\n");
    return -1;
  }

  Instruction::Ptr instruction;
  for (Function* function : functions) {
    Flowgraph graph;
    Address function_address = function->addr();

    BuildFlowgraph(function, &graph);
    printf("....");
    if (graph.GetSize() > minimum_size) {
      std::vector<uint32_t> minhash_vector;
      printf("[!] FileID %lx: Fingerprinting function %lx (%lu nodes)\n",
        file_id, function_address, graph.GetSize());

      CalculateFunctionFingerprint(function, 200, 200, 32, &minhash_vector);

      printf("[!] Searching for matches...\n");

      std::vector<std::pair<float, MinHashSearchIndex::FileAndAddress>> results;
      search_index.QueryTopN(
        minhash_vector, max_matches, &results);
      for (const auto& result : results) {
        printf("[!] %f -- FileID %lx Address %lx\n", result.first,
          result.second.first, result.second.second);
      }
    } else {
      printf("[!] FileID %lx: Skipping function %lx, only %lu nodes\n", 
        file_id, function_address, graph.GetSize());
    }
  }
}
