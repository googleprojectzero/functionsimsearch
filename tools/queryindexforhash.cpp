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

#include "searchbackend/simhashsearchindex.hpp"
#include "util/util.hpp"

DEFINE_string(index, "./similarity.index", "Index file.");
DEFINE_string(hash, "deadbeefABCD12340BADCAFEABCD1234", "Hash to query.");
DEFINE_uint64(max_matches, 5, "Maximum number of matches to show.");

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
    "Query the DB for a given hash.");
  ParseCommandLineFlags(&argc, &argv, true);

  FeatureHash hash = StringToFeatureHash(FLAGS_hash);
  std::string index_file(FLAGS_index);
  uint64_t max_matches = FLAGS_max_matches;

  // Load the search index.
  SimHashSearchIndex search_index(index_file, false);
  printf("[!] Loaded search index.\n");

  printf("[!] Querying for %16.16lx %16.16lx\n", hash.first, hash.second);
  std::vector<std::pair<float, SimHashSearchIndex::FileAndAddress>> results;
  search_index.QueryTopN(
    hash.first, hash.second, max_matches, &results);
  for (const auto& result : results) {
    printf("[!] %f %16.16lx %16.16lx\n", result.first, result.second.first,
      result.second.second);
  }
}
