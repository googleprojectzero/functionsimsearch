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

DEFINE_string(data, "./data", "Directory for sourcing data");
DEFINE_string(weights, "weights.txt", "Feature weights file");
// The google namespace is there for compatibility with legacy gflags and will
// be removed eventually.
#ifndef gflags
using namespace google;
#else
using namespace gflags;
#endif


using namespace std;

void CalculateHashStats(TrainingData* data, FunctionSimHasher* not_trained,
  FunctionSimHasher* trained, std::vector<std::pair<uint32_t, uint32_t>>* pairs,
  double* trained_mean, double* untrained_mean, std::map<uint32_t, uint32_t>*
  trained_histogram, std::map<uint32_t, uint32_t>* untrained_histogram) {

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

    (*untrained_histogram)[hamming_untrained]++;
    (*trained_histogram)[hamming_trained]++;
    *trained_mean += hamming_trained;
    *untrained_mean += hamming_untrained;
  }
  *trained_mean /= pairs->size();
  *untrained_mean /= pairs->size();
}

// Write a histogram to stdout in gnuplot format.
void DumpHistogram(const std::string& title, const std::map<uint32_t, uint32_t>&
    histogram) {
  printf("# Histogram %s\n", title.c_str());
  for (const auto& entry : histogram) {
    printf("%d %d\n", entry.first, entry.second);
  }
  printf("\n\n");
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
  SetUsageMessage(
    "Evaluate a weights file against labelled data. Refer to the source file "
    "for a detailed description of what data the tool expects to find.");
  ParseCommandLineFlags(&argc, &argv, true);

  std::string directory(FLAGS_data);
  std::string outputfile(FLAGS_weights);

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
  std::map<uint32_t, uint32_t> attraction_trained_histogram;
  std::map<uint32_t, uint32_t> attraction_untrained_histogram;

  CalculateHashStats(&data, &hash_no_weight, &hash_weights,
    data.GetAttractionSet(), &attraction_mean_trained,
    &attraction_mean_untrained, &attraction_trained_histogram,
    &attraction_untrained_histogram);

  double repulsion_mean_trained = 0;
  double repulsion_mean_untrained = 0;
  std::map<uint32_t, uint32_t> repulsion_trained_histogram;
  std::map<uint32_t, uint32_t> repulsion_untrained_histogram;

  CalculateHashStats(&data, &hash_no_weight, &hash_weights,
    data.GetRepulsionSet(), &repulsion_mean_trained,
    &repulsion_mean_untrained, &repulsion_trained_histogram,
    &repulsion_untrained_histogram);

  DumpHistogram("attraction untrained", attraction_untrained_histogram);
  DumpHistogram("repulsion untrained", repulsion_untrained_histogram);
  DumpHistogram("attraction trained", attraction_trained_histogram);
  DumpHistogram("repulsion trained", repulsion_trained_histogram);

  printf("Attraction mean trained: %.10e\n", attraction_mean_trained);
  printf("Attraction mean untrained: %.10e\n", attraction_mean_untrained);
  printf("Repulsion mean trained: %.10e\n", repulsion_mean_trained);
  printf("Repulsion mean untrained: %.10e\n", repulsion_mean_untrained);
}

