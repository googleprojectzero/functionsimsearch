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
