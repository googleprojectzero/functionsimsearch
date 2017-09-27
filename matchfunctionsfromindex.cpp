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

#include "disassembly.hpp"
#include "dyninstfeaturegenerator.hpp"
#include "flowgraph.hpp"
#include "flowgraphutil.hpp"
#include "functionsimhash.hpp"
#include "functionmetadata.hpp"
#include "simhashsearchindex.hpp"
#include "pecodesource.hpp"
#include "threadpool.hpp"
#include "util.hpp"

using namespace std;
using namespace Dyninst;
using namespace ParseAPI;
using namespace InstructionAPI;

// Simple POD class to aggregate the information from a given search.
class SearchResult {
public:
  SearchResult() {};
  SearchResult(SimHashSearchIndex::FileID source,
    Address source_address, uint32_t index,
    uint32_t source_function_bnodes, float similarity,
    SimHashSearchIndex::FileID match,
    Address match_address) :
    source_file_(source), source_function_(source_address),
    source_function_index_(index),
    source_function_bnodes_(source_function_bnodes),
    similarity_(similarity), matching_file_(match),
    matching_function_(match_address) {};
  SimHashSearchIndex::FileID source_file_;
  Address source_function_;
  uint32_t source_function_index_;
  uint32_t source_function_bnodes_; // Number of branching nodes in source.
  float similarity_;
  SimHashSearchIndex::FileID matching_file_;
  Address matching_function_;
};

int main(int argc, char** argv) {
  if (argc != 7) {
    printf("Match simhashes from a binary against a search index\n");
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
  printf("[!] Executable id is %16.16lx\n", file_id);

  // Load the metadata file.
  FunctionMetadataStore metadata(index_file + ".meta");

  // Load the search index.
  SimHashSearchIndex search_index(index_file, false);

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

  threadpool::SynchronizedQueue<SearchResult> resultqueue;
  threadpool::ThreadPool pool(std::thread::hardware_concurrency());
  std::atomic_ulong atomic_processed_functions(0);
  std::atomic_ulong* processed_functions = &atomic_processed_functions;
  uint64_t number_of_functions = functions.size();
  FunctionSimHasher hasher("weights.txt");

  // Push the consumer thread into the threadpool.
  for (Function* function : functions) {
    // Push the producer threads into the threadpool.
    pool.Push(
      [&resultqueue, &search_index, &metadata, processed_functions, file_id,
      function, minimum_size, max_matches, minimum_percentage,
      number_of_functions, &hasher]
        (int threadid) {
      Flowgraph graph;
      Address function_address = function->addr();
      BuildFlowgraph(function, &graph);
      (*processed_functions)++;

      uint64_t branching_nodes = graph.GetNumberOfBranchingNodes();

      if (graph.GetNumberOfBranchingNodes() <= minimum_size) {
       return;
      }

      std::vector<uint64_t> hashes;

      // TODO(thomasdullien): Investigate if we need locking for the constructor
      // of the DyninstFeatureGenerator.
      DyninstFeatureGenerator generator(function);
      hasher.CalculateFunctionSimHash(&generator, 128, &hashes);
      uint64_t hash_A = hashes[0];
      uint64_t hash_B = hashes[1];

      std::vector<std::pair<float, SimHashSearchIndex::FileAndAddress>> results;
      search_index.QueryTopN(
        hash_A, hash_B, max_matches, &results);
      for (const auto& result : results) {
        if (result.first > minimum_percentage) {
          resultqueue.Push(
            SearchResult(file_id, function_address,
              processed_functions->load(), branching_nodes, result.first,
              result.second.first, result.second.second));
        }
      }
    });
  }

  // Run as long as there is still work to do in the pool.
  while (!pool.AllIdle()) {
    SearchResult result;
    if (resultqueue.Pop(result)) {
      printf("[!] (%lu/%lu - %d branching nodes) %f: %lx.%lx matches "
        "%lx.%lx ", result.source_function_index_, number_of_functions,
        result.source_function_bnodes_, result.similarity_,
        result.source_file_, result.source_function_,
        result.matching_file_, result.matching_function_);

      std::string function_name;
      std::string file_name;
      if (metadata.GetFileName(result.matching_file_, &file_name)) {
        printf("%s ", file_name.c_str());
      }
      if (metadata.GetFunctionName(result.matching_file_,
        result.matching_function_, &function_name)) {
        printf("%s ", function_name.c_str());
      }
      printf("\n");
    }
  }
  pool.Stop(true);
}
