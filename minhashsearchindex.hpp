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

#ifndef MINHASHSEARCHINDEX_HPP
#define MINHASHSEARCHINDEX_HPP

#include <cstdint>
#include <map>
#include <vector>
#include "persistentmap.hpp"

class MinHashSearchIndex {
public:
  typedef uint16_t VectorIndex;
  typedef uint32_t HashValue;
  typedef uint64_t FunctionID;
  typedef uint64_t FileID;
  typedef uint64_t Address;

  typedef std::tuple<VectorIndex, HashValue, FunctionID> IndexEntry;
  typedef std::pair<FileID, Address> FileAndAddress;
  MinHashSearchIndex(const std::string& indexname, bool create);

  uint64_t QueryTopN(const std::vector<uint32_t>& features,
    uint32_t how_many, std::vector<std::pair<float, FileAndAddress>>* results);

  // Add the given function to the search index.
  void AddFunction(const std::vector<uint32_t>& features, FileID file_id,
    Address address);

private:
  PersistentMap<FunctionID, FileAndAddress> id_to_file_and_address_;
  PersistentSet<IndexEntry> search_index_;
};

#endif // MINHASHSEARCHINDEX_HPP

