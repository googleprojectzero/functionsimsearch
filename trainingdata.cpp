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

#include "trainingdata.hpp"

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
  // Read the contents of functions.txt.
  std::string functionsfilename = directory_ + "/functions.txt";
  std::vector<std::vector<std::string>> file_contents;
  if (!FileToLineTokens(functionsfilename, &file_contents)) {
    return false;
  }

  // Read all the features, place them in a vector, and populate a map that maps
  // FeatureHash to vector index.
  ReadFeatureSet(file_contents, &all_features_);
  uint32_t index = 0;
  for (const FeatureHash& feature : all_features_) {
    all_features_vector_.push_back(feature);
    feature_to_vector_index_[feature] = index;
    ++index;
  }

  index = 0;
  all_functions_.resize(file_contents.size());
  for (const std::vector<std::string>& line : file_contents) {
    function_to_index_[line[0]] = index;
    for (uint32_t i = 1; i < line.size(); ++i) {
      FeatureHash hash = StringToFeatureHash(line[i]);
      all_functions_[index].push_back(feature_to_vector_index_[hash]);
    }
    ++index;
  }
  // Feature vector and function vector have been loaded. Now load the attraction
  // and repulsion set.
  std::vector<std::vector<std::string>> attract_file_contents;
  if (!FileToLineTokens(directory_ + "/attract.txt", &attract_file_contents)) {
    return false;
  }

  std::vector<std::vector<std::string>> repulse_file_contents;
  if (!FileToLineTokens(directory_ + "/repulse.txt", &repulse_file_contents)) {
    return false;
  }

  for (const std::vector<std::string>& line : attract_file_contents) {
    attractionset_.push_back(std::make_pair(
      function_to_index_[line[0]],
      function_to_index_[line[1]]));
  }

  std::vector<std::pair<uint32_t, uint32_t>> repulsionset;
  for (const std::vector<std::string>& line : repulse_file_contents) {
    repulsionset_.push_back(std::make_pair(
      function_to_index_[line[0]],
      function_to_index_[line[1]]));
  }

  printf("[!] Loaded %ld functions (%ld unique features)\n",
    all_functions_.size(), all_features_vector_.size());
  printf("[!] Attraction-Set: %ld pairs\n", attractionset_.size());
  printf("[!] Repulsion-Set: %ld pairs\n", repulsionset_.size());
  loaded_ = true;
  return true;
}
