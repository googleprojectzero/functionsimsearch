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

#include <tuple>
#include <vector>

#include "bitpermutation.hpp"
#include "simhashsearchindex.hpp"
#include "threadtimer.hpp"
#include "util.hpp"

SimHashSearchIndex::SimHashSearchIndex(const std::string& indexname,
  bool create, uint8_t buckets) :
    id_to_file_and_address_(indexname, create),
    search_index_("index", id_to_file_and_address_.getSegment(), create),
    buckets_(buckets) {
  if (id_to_file_and_address_.getMap() == nullptr) {
    throw std::runtime_error("Loading search index map failed!");
  }
  if (search_index_.getSet() == nullptr) {
    throw std::runtime_error("Loading search index set failed!");
  }
}

uint64_t SimHashSearchIndex::QueryTopN(uint64_t hash_A, uint64_t hash_B,
  uint32_t how_many, std::vector<std::pair<float, FileAndAddress>>* results) {
  std::chrono::time_point<std::chrono::high_resolution_clock> timepoint;
  std::map<FunctionID, uint64_t> candidate_and_distance;

  // Get the full hash for the query.
  uint128_t full_hash = to128(hash_A, hash_B);
  std::vector<uint128_t> permuted_values;
  get_n_permutations(full_hash, buckets_, &permuted_values);

  // Identify the N different buckets that need to be checked.
  for (uint8_t bucket_count = 0; bucket_count < buckets_; ++bucket_count) {
    // Permute the input hash, then mask off all but 8 bits to identify the
    // hash bucket to use.
    uint128_t permuted = permuted_values[bucket_count];

    uint64_t hash_component_A = getHigh64(permuted);
    uint64_t hash_component_A_masked = hash_component_A & 0xFF00000000000000ULL;
    uint64_t hash_component_B = getLow64(permuted);

    // Build a synthetic index entry to perform the search.
    IndexEntry search_entry = std::make_tuple(
      bucket_count, hash_component_A_masked, 0ULL, 0);

    profile::ResetClock();
    // Find the relevant index entry.
    auto iter = search_index_.getSet()->lower_bound(search_entry);
    profile::ClockCheckpoint("Searched lower_bound for bucket %d\n",
      bucket_count);

    {
      // TODO(thomasdullien): Replace this with a shared_lock as soon as
      // the codebase is moved to C++14.
      std::lock_guard<std::mutex> lock(mutex_);
      profile::ResetClock();

      // Run through all entries until the end of the 'hash bucket' (really
      // just a range of elements in the set) is reached.
      while (iter != search_index_.getSet()->end()) {
        IndexEntry current_entry = *iter;

        // Check if we have processed the entire bucket.
        uint64_t entry_component_A = std::get<1>(current_entry);
        PermutationIndex index = std::get<0>(current_entry);
        if (((entry_component_A & 0xFF00000000000000ULL) !=
              hash_component_A_masked) || (index != bucket_count)) {
          break;
        }
        // Retrieve the second component of the full hash.
        uint64_t entry_component_B = std::get<2>(current_entry);
        // Compute the hamming distance of the full hash.
        uint64_t distance = HammingDistance(
          hash_component_A, hash_component_B, entry_component_A,
          entry_component_B);
        candidate_and_distance[std::get<3>(current_entry)] = distance;
        ++iter;
      }
      profile::ClockCheckpoint("Obtained candidates\n");
    }
  }

  profile::ResetClock();
  // Convert the candidate map to a vector.
  std::vector<std::pair<uint64_t, FunctionID>> distance_and_candidate;
  for (const auto& element : candidate_and_distance) {
    distance_and_candidate.push_back(std::make_pair(element.second,
      element.first));
  }
  profile::ClockCheckpoint("Converted map to vector with %d elements\n",
    distance_and_candidate.size());

  // Sort the candidate vector to provide the top-N results.
  std::sort(distance_and_candidate.begin(), distance_and_candidate.end());

  profile::ClockCheckpoint("Sorted the vector.\n");

  uint64_t counter = 0;
  const auto& innermap = id_to_file_and_address_.getMap();

  for (const auto& element : distance_and_candidate) {
    const FileAndAddress& file_address = innermap->at(element.second);
    results->push_back(std::make_pair(
      128.0 - (static_cast<float>(element.first)),
      file_address));
    ++counter;
    if (counter >= how_many) {
      break;
    }
  }

  profile::ClockCheckpoint("Returning with results.\n");
  return counter;
}

uint64_t SimHashSearchIndex::AddFunction(uint64_t hash_A, uint64_t hash_B,
  SimHashSearchIndex::FileID file_id, SimHashSearchIndex::Address address) {
  // Obtain a new function ID and insert the mapping from function ID to
  // target file and address into the corresponding map.
  FunctionID function_id = id_to_file_and_address_.getMap()->size() + 1;
  (*id_to_file_and_address_.getMap())[function_id] = std::make_pair(
    file_id, address);

  // Now generate 'buckets' many new hashes from the SimHash by simply
  // permuting the 128-bit value a few times.
  uint128_t full_hash = to128(hash_A, hash_B);
  std::vector<uint128_t> permuted_values;
  get_n_permutations(full_hash, buckets_, &permuted_values);

  for (uint8_t bucket_count = 0; bucket_count < buckets_; ++bucket_count) {
    uint128_t permuted = permuted_values[bucket_count];
    uint64_t hash_component_A = getHigh64(permuted);
    uint64_t hash_component_B = getLow64(permuted);

    {
      // TODO(thomasdullien): When the codebase is moved to C++14, change this
      // to be a unique_lock.
      std::lock_guard<std::mutex> lock(mutex_);
      search_index_.getSet()->insert(std::make_tuple(
        bucket_count, hash_component_A, hash_component_B, function_id));
    }
  }
  return 0; // TODO(thomasdullien): Why return anything at all?
}

uint64_t SimHashSearchIndex::GetIndexFileSize() {
  std::lock_guard<std::mutex> lock(mutex_);
  const std::shared_ptr<managed_mapped_file> segment =
    id_to_file_and_address_.getSegment();
  return segment->get_size();
}

uint64_t SimHashSearchIndex::GetIndexFileFreeSpace() {
  std::lock_guard<std::mutex> lock(mutex_);
  const std::shared_ptr<managed_mapped_file> segment =
    id_to_file_and_address_.getSegment();
  return segment->get_free_memory();
}

uint64_t SimHashSearchIndex::GetIndexSetSize() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return search_index_.getSet()->size();
}

uint64_t SimHashSearchIndex::GetNumberOfIndexedFunctions() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return id_to_file_and_address_.getMap()->size();
}

void SimHashSearchIndex::DumpIndexToStdout(bool all = false) const {
  std::lock_guard<std::mutex> lock(mutex_);
  const auto map = id_to_file_and_address_.getMap();
  const auto index = search_index_.getSet();
  for (const IndexEntry& entry : *index) {
    FileAndAddress file_and_address = map->at(std::get<3>(entry));

    if ((!all) && (std::get<0>(entry) != 0)) {
      break;
    }
    printf("%d %16.16lx %16.16lx %16.16lx %16.16lx\n",
      std::get<0>(entry), std::get<1>(entry), std::get<2>(entry),
      file_and_address.first, file_and_address.second);
  }
}
