#include <stdexcept>

#include "minhashsearchindex.hpp"

MinHashSearchIndex::MinHashSearchIndex(
  const std::string& indexname, bool create) :
  id_to_file_and_address_(indexname, create),
  search_index_("index", id_to_file_and_address_.getSegment(), create) {

  if (id_to_file_and_address_.getMap() == nullptr) {
    throw std::runtime_error("Loading search index map failed!");
  }
  if (search_index_.getSet() == nullptr) {
    throw std::runtime_error("Loading search index set failed!");
  }
}

void MinHashSearchIndex::AddFunction(const std::vector<uint32_t>& features,
  FileID file_id, Address address) {
  uint64_t function_id = id_to_file_and_address_.getMap()->size() + 1;
  // Set the new function ID.
  (*id_to_file_and_address_.getMap())[function_id] = std::make_pair(
    file_id, address);

  for (uint16_t index = 0; index < features.size(); ++index) {
    uint32_t hash_value = features[index];
    search_index_.getSet()->insert(
      std::make_tuple(index, hash_value, function_id));
  }
}

uint64_t MinHashSearchIndex::QueryTopN(const std::vector<uint32_t>& features,
  uint32_t how_many, std::vector<std::pair<float, FileAndAddress>>* results) {
  std::map<FunctionID, uint64_t> score_keeping;

  for (uint16_t index = 0; index < features.size(); ++index) {
    uint32_t hash_value = features[index];
    auto iter = search_index_.getSet()->lower_bound(
      std::make_tuple(index, hash_value, 0));

    while ((std::get<0>(*iter) == index) && (std::get<1>(*iter) == hash_value)) {
      score_keeping[std::get<2>(*iter)]++;
      ++iter;
    }
  }
  // Scores for each element have been calculated. Fill the resulting vector
  // and sort it.

  for (const auto& element : score_keeping) {
    results->push_back(
      std::make_pair(
        static_cast<float>(element.second) / features.size(),
        (*id_to_file_and_address_.getMap())[element.first]));
  }
  std::sort(results->rbegin(), results->rend());

  uint64_t candidates = results->size();
  results->resize(how_many);
  return candidates;
}

uint64_t MinHashSearchIndex::GetIndexFileSize() {
  const std::shared_ptr<managed_mapped_file> segment =
    id_to_file_and_address_.getSegment();
  return segment->get_size();
}

uint64_t MinHashSearchIndex::GetIndexFileFreeSpace() {
  const std::shared_ptr<managed_mapped_file> segment =
    id_to_file_and_address_.getSegment();
  return segment->get_free_memory();
}

uint64_t MinHashSearchIndex::GetIndexSetSize() const {
  return search_index_.getSet()->size();
}

uint64_t MinHashSearchIndex::GetNumberOfIndexedFunctions() const {
  return id_to_file_and_address_.getMap()->size();
}

