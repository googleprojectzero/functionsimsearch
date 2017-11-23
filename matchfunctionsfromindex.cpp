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
#include "functionmetadata.hpp"
#include "simhashsearchindex.hpp"
#include "pecodesource.hpp"
#include "threadpool.hpp"
#include "util.hpp"

DEFINE_string(format, "PE", "Executable format: PE or ELF");
DEFINE_string(input, "", "File to disassemble");
DEFINE_string(index, "./similarity.index", "Index file");
DEFINE_string(weights, "", "Feature weights file");
DEFINE_uint64(minimum_function_size, 5, "Minimum size of a function to be added");
DEFINE_uint64(max_matches, 5, "Maximum number of matches per query");
DEFINE_double(minimum_percentage, 0.8, "Minimum similarity");
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
  uint64_t source_function_index_;
  uint32_t source_function_bnodes_; // Number of branching nodes in source.
  float similarity_;
  SimHashSearchIndex::FileID matching_file_;
  Address matching_function_;
};

int main(int argc, char** argv) {
  SetUsageMessage(
    "Match simhashes from a binary against a search index");
  ParseCommandLineFlags(&argc, &argv, true);

  std::string mode(FLAGS_format);
  std::string binary_path_string(FLAGS_input);
  std::string index_file(FLAGS_index);
  uint64_t minimum_size = FLAGS_minimum_function_size;
  uint64_t max_matches = FLAGS_max_matches;
  float minimum_percentage = FLAGS_minimum_percentage;

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

  std::mutex search_index_mutex;
  std::mutex* mutex_pointer = &search_index_mutex; 
  threadpool::SynchronizedQueue<SearchResult> resultqueue;
  threadpool::ThreadPool pool(std::thread::hardware_concurrency());
  std::atomic_ulong atomic_processed_functions(0);
  std::atomic_ulong* processed_functions = &atomic_processed_functions;
  uint64_t number_of_functions = functions.size();
  FunctionSimHasher hasher(FLAGS_weights);

  uint64_t function_index = 0;

  // Push the consumer thread into the threadpool.
  for (Function* function : functions) {
    // Push the producer threads into the threadpool.
    pool.Push(
      [&resultqueue, function_index, mutex_pointer, &search_index, &metadata,
      file_id, function, minimum_size, max_matches, minimum_percentage,
      number_of_functions, &hasher]
        (int threadid) {
      Flowgraph graph;
      Address function_address = function->addr();
      BuildFlowgraph(function, &graph);

      uint64_t branching_nodes = graph.GetNumberOfBranchingNodes();
      if (graph.GetNumberOfBranchingNodes() <= minimum_size) {
       return;
      }

      std::vector<uint64_t> hashes;
      // Dyninst cannot tolerate multi-threaded access, so construction of the
      // DyninstFeatureGenerator needs to be synched accross threads.
      mutex_pointer->lock();
      DyninstFeatureGenerator generator(function);
      mutex_pointer->unlock();

      hasher.CalculateFunctionSimHash(&generator, 128, &hashes);
      uint64_t hash_A = hashes[0];
      uint64_t hash_B = hashes[1];

      std::vector<std::pair<float, SimHashSearchIndex::FileAndAddress>> results;
      search_index.QueryTopN(
        hash_A, hash_B, max_matches, &results);

      for (const auto& result : results) {
        if (result.first > minimum_percentage) {
          resultqueue.Push(
            SearchResult(file_id, function_address, function_index,
              branching_nodes, result.first, result.second.first,
              result.second.second));
        }
      }
    });
    ++function_index;
  }

  // Run as long as there is still work to do in the pool.
  while (!pool.AllIdle()) {
    SearchResult result;
    if (resultqueue.Pop(result)) {
      printf("[!] (%lu/%lu - %d branching nodes) %f: %lx.%lx matches "
        "%lx.%lx ", result.source_function_index_, number_of_functions,
        result.source_function_bnodes_, result.similarity_ / 128.0,
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
