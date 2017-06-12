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
#include "threadpool.hpp"

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
  if (argc != 7) {
    printf("Match minhash vectors from a binary against a search index\n");
    printf("Usage: %s <PE/ELF> <binary path> <index file> <minimum function size> <max_matches> <minimum_percentage>\n", argv[0]);
    return -1;
  }

  std::string mode(argv[1]);
  std::string binary_path_string(argv[2]);
  std::string index_file(argv[3]);
  uint64_t minimum_size = strtoul(argv[4], nullptr, 10);
  uint64_t max_matches = strtoul(argv[5], nullptr, 10);
  float minimum_percentage = strtod(argv[6], nullptr);

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

  printf("[!] Done disassembling, beginning search.\n");
  Instruction::Ptr instruction;

  threadpool::SynchronizedQueue<
    std::tuple<Address, float, MinHashSearchIndex::FileAndAddress>> resultqueue;
  threadpool::ThreadPool pool(std::thread::hardware_concurrency());
  std::atomic_ulong atomic_processed_functions(0);
  std::atomic_ulong* processed_functions = &atomic_processed_functions;
  uint64_t number_of_functions = functions.size();

  // Push the consumer thread into the threadpool.
  for (Function* function : functions) {
    // Push the producer threads into the threadpool.
    pool.Push(
      [&resultqueue, &search_index, processed_functions, file_id, function,
      minimum_size, max_matches, minimum_percentage, number_of_functions]
        (int threadid) {
      Flowgraph graph;
      Address function_address = function->addr();
      uint64_t block_count = 0;
      for (const auto& block : function->blocks()) {
        ++block_count;
      }
      if (block_count <= minimum_size) {
        return;
      }

      BuildFlowgraph(function, &graph);
      std::vector<uint32_t> minhash_vector;
      CalculateFunctionFingerprint(function, 200, 200, 32, &minhash_vector);

      std::vector<std::pair<float, MinHashSearchIndex::FileAndAddress>> results;
      search_index.QueryTopN(
        minhash_vector, max_matches, &results);
      for (const auto& result : results) {
        if (result.first > minimum_percentage) {
          resultqueue.Push(std::make_tuple(
            function_address, result.first, 
            result.second));

            printf("[!] (%lu/%lu) %f: %lx.%lx matches %lx.%lx \n",
              processed_functions->load(), number_of_functions, result.first,
              file_id, function_address, result.second.first, 
              result.second.second);
        }
      }
      (*processed_functions)++;
      //printf("[!] Thread %d processed %lu/%lu function\n", threadid,
      //  processed_functions->load(), number_of_functions);
    });
  }
  // Process all the things.
  pool.Stop(true);
}
