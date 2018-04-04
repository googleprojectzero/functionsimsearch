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

#include <map>
#include <unordered_map>
#include <vector>

#include "util/buffertokeniterator.hpp"
#include "learning/trainingdata.hpp"

TrainingData::TrainingData(const std::string& directory) :
  directory_(directory), loaded_(false) {};

std::vector<FeatureHash>* TrainingData::GetFeaturesVector() {
  return &all_features_vector_;
}

std::vector<std::pair<uint32_t, uint32_t>>* TrainingData::GetAttractionSet() {
  return &attractionset_;
}

std::vector<std::pair<uint32_t, uint32_t>>* TrainingData::GetRepulsionSet() {
  return &repulsionset_;
}

std::vector<FunctionFeatures>* TrainingData::GetFunctions() {
  return &all_functions_;
}

bool TrainingData::Load() {
  bool result = LoadTrainingData(directory_,
    &all_functions_,
    &all_features_vector_,
    &attractionset_,
    &repulsionset_);

  if (result) {
    printf("# Loaded %ld functions (%ld unique features)\n",
      all_functions_.size(), all_features_vector_.size());
    printf("# Attraction-Set: %ld pairs\n", attractionset_.size());
    printf("# Repulsion-Set: %ld pairs\n", repulsionset_.size());
    loaded_ = true;
    return true;
  } else {
    return false;
  }
}

// Re-write of LoadTrainingData to allow very large training data files (dozens
// of GBs).
//
// Parses a functions.txt file (which consists of lines with function-IDs and
// feature hashes).
bool LoadTrainingData(const std::string& directory,
  std::vector<FunctionFeatures>* all_functions,
  std::vector<FeatureHash>* all_features_vector,
  std::vector<std::pair<uint32_t, uint32_t>>* attractionset,
  std::vector<std::pair<uint32_t, uint32_t>>* repulsionset) {

  // Map functions.txt into memory.
  printf("[!] Mapping functions.txt\n");
  MappedTextFile functions_txt(directory + "/functions.txt");

  // Run through the file and obtain a set of all feature hashes.
  printf("[!] About to count the entire feature set.\n");
  std::set<FeatureHash> all_features;
  uint32_t lines = ReadFeatureSet(&functions_txt, &all_features);
  printf("[!] Processed %d lines, total features are %d\n", lines,
    all_features.size());

  std::map<FeatureHash, uint32_t> features_to_vector_index;
  uint32_t index = 0;
  // Fill the all_features_vector while also building a map.
  for (const FeatureHash& feature : all_features) {
    all_features_vector->push_back(feature);
    features_to_vector_index[feature] = index;
    ++index;
  }

  // FunctionFeatures is typedef'd to a vector of uint32_t. so
  // all_functions is a vector of vectors. The following code iterates over
  // all lines in functions.txt, and fills all_functions by putting indices
  // to the actual hashes into the per-function FunctionFeatures vector.
  printf("[!] Iterating over input data for the 2nd time.\n");
  std::unordered_map<std::string, uint32_t> function_to_index;
  index = 0;
  all_functions->resize(lines);

  BufferTokenIterator line_iterator = functions_txt.GetLineIterator();
  while (line_iterator.HasMore()) {
    BufferTokenIterator word_iterator = functions_txt.GetWordIterator(
      line_iterator);
    function_to_index[word_iterator.Get()] = index;
    ++word_iterator;
    while (word_iterator.HasMore()) {
      FeatureHash hash = StringToFeatureHash(word_iterator.Get());
      (*all_functions)[index].push_back(features_to_vector_index[hash]);
      ++word_iterator;
    }
    ++index;
    ++line_iterator;
  }

  // Feature vector and function vector have been loaded. Now load the attraction
  // and repulsion set.
  std::vector<std::vector<std::string>> attract_file_contents;
  if (!FileToLineTokens(directory + "/attract.txt", &attract_file_contents)) {
    return false;
  }

  std::vector<std::vector<std::string>> repulse_file_contents;
  if (!FileToLineTokens(directory + "/repulse.txt", &repulse_file_contents)) {
    return false;
  }

  for (const std::vector<std::string>& line : attract_file_contents) {
    attractionset->push_back(std::make_pair(
      function_to_index[line[0]],
      function_to_index[line[1]]));
  }

  for (const std::vector<std::string>& line : repulse_file_contents) {
    repulsionset->push_back(std::make_pair(
      function_to_index[line[0]],
      function_to_index[line[1]]));
  }

  printf("[!] Loaded %ld functions (%ld unique features)\n",
    all_functions->size(), all_features_vector->size());
  printf("[!] Attraction-Set: %ld pairs\n", attractionset->size());
  printf("[!] Repulsion-Set: %ld pairs\n", repulsionset->size());

  return true;
}


