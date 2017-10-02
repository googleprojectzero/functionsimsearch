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

#include <spii/auto_diff_term.h>
#include <spii/dynamic_auto_diff_term.h>
#include <spii/large_auto_diff_term.h>
#include <spii/function.h>
#include <spii/solver.h>
#include <spii/term.h>

#include "util.hpp"
#include "functionsimhash.hpp"
#include "simhashtrainer.hpp"
#include "trainingdata.hpp"

using namespace std;

void CalculateHashStats(TrainingData* data, FunctionSimHasher* not_trained,
  FunctionSimHasher* trained, std::vector<std::pair<uint32_t, uint32_t>>* pairs,
  double* trained_mean, double* untrained_mean) {
  // Calculate the hashes for all elements in the attraction set.
  for (const auto& pair : *pairs) {
    std::vector<FeatureHash> features_A;
    std::vector<FeatureHash> features_B;

    for (uint32_t index : data->GetFunctions()->at(pair.first)) {
      features_A.push_back(data->GetFeaturesVector()->at(index));
    }
    for (uint32_t index : data->GetFunctions()->at(pair.second)) {
      features_B.push_back(data->GetFeaturesVector()->at(index));
    }
    std::vector<uint64_t> function_A_hash_trained;
    std::vector<uint64_t> function_B_hash_trained;
    std::vector<uint64_t> function_A_hash_untrained;
    std::vector<uint64_t> function_B_hash_untrained;

    // Calculate the hashes for the trained version.
    trained->CalculateFunctionSimHash(
      &features_A, &function_A_hash_trained);
    trained->CalculateFunctionSimHash(
      &features_B, &function_B_hash_trained);

    // Calculate the hashes for the untrained version.
    not_trained->CalculateFunctionSimHash(
      &features_A, &function_A_hash_untrained);
    not_trained->CalculateFunctionSimHash(
      &features_B, &function_B_hash_untrained);

    uint32_t hamming_trained = HammingDistance(function_A_hash_trained[0],
      function_A_hash_trained[1], function_B_hash_trained[0],
      function_B_hash_trained[1]);
    uint32_t hamming_untrained = HammingDistance(function_A_hash_untrained[0],
      function_A_hash_untrained[1], function_B_hash_untrained[0],
      function_B_hash_untrained[1]);

    *trained_mean += hamming_trained;
    *untrained_mean += hamming_untrained;
  }
  *trained_mean /= pairs->size();
  *untrained_mean /= pairs->size();
}

// The code expects the following files to be present inside the data directory
// (which is passed in as first argument):
//
//  functions.txt - a text file formed by concatenating the output of the
//                  functionfingerprints tool in verbose mode. Each line starts
//                  with [file_id]:[address], and is followed by the various
//                  features that make up the function.
//  attract.txt   - a file with pairs of [file_id]:[address] [file_id]:[address]
//                  indicating which functions should be the same.
//  repulse.txt   - a file with pairs of [file_id]:[address] [file_id]:[address]
//                  indicating which functions should NOT be the same.
//
// It also expects a weights file to evaluate against the data in question.

int main(int argc, char** argv) {
  if (argc != 3) {
    printf("Evaluate a weights file against labelled data.\n");
    printf("Usage: %s <data directory> <weights file>\n", argv[0]);
    printf("Refer to the source code of this utility for detailed usage.\n");
    return -1;
  }

  std::string directory(argv[1]);
  std::string outputfile(argv[2]);

  TrainingData data(directory);
  if (!data.Load()) {
    printf("[!] Failure to load the training data, exiting.\n");
    return -1;
  }
  printf("[!] Training data parsed, beginning the evaluation process.\n");

  // Training has been performed. Instantiate two FunctionSimHasher, one with
  // the new weights, one without.
  FunctionSimHasher hash_no_weight("", false);
  FunctionSimHasher hash_weights(outputfile, false);

  double attraction_mean_trained = 0;
  double attraction_mean_untrained = 0;

  CalculateHashStats(&data, &hash_no_weight, &hash_weights,
    data.GetAttractionSet(), &attraction_mean_trained,
    &attraction_mean_untrained);

  double repulsion_mean_trained = 0;
  double repulsion_mean_untrained = 0;

  CalculateHashStats(&data, &hash_no_weight, &hash_weights,
    data.GetAttractionSet(), &repulsion_mean_trained,
    &repulsion_mean_untrained);

  printf("Attraction mean trained: %.10e\n", attraction_mean_trained);
  printf("Attraction mean untrained: %.10e\n", attraction_mean_untrained);
  printf("Repulsion mean trained: %.10e\n", repulsion_mean_trained);
  printf("Repulsion mean untrained: %.10e\n", repulsion_mean_untrained);

}

