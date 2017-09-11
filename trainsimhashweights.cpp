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
#include "simhashtrainer.hpp"

using namespace std;

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
//                  indicating which functions should NOT be the same
//

int main(int argc, char** argv) {
  if (argc != 3) {
    printf("Use labelled and unlablled data to train a weights.txt file.\n");
    printf("Usage: %s <data directory> <weights file>\n", argv[0]);
    printf("Refer to the source code of this utility for detailed usage.\n");
    return -1;
  }

  printf("[!] Parsing training data.\n");
  std::string directory(argv[1]);
  std::string outputfile(argv[2]);
  // Read the contents of functions.txt.
  std::string functionsfilename = directory + "/functions.txt";
  std::vector<std::vector<std::string>> file_contents;
  if (!FileToLineTokens(functionsfilename, &file_contents)) {
    return -1;
  }

  // Read all the features, place them in a vector, and populate a map that maps
  // FeatureHash to vector index.
  std::set<FeatureHash> all_features;
  ReadFeatureSet(file_contents, &all_features);
  std::vector<FeatureHash> all_features_vector;
  std::map<FeatureHash, uint32_t> feature_to_vector_index;
  uint32_t index = 0;
  for (const FeatureHash& feature : all_features) {
    all_features_vector.push_back(feature);
    feature_to_vector_index[feature] = index;
    ++index;
  }

  // FunctionFeatures are a vector of uint32_t.
  std::vector<FunctionFeatures> all_functions;
  std::map<std::string, uint32_t> function_to_index;
  index = 0;
  all_functions.resize(file_contents.size());
  for (const std::vector<std::string>& line : file_contents) {
    function_to_index[line[0]] = index;
    for (uint32_t i = 1; i < line.size(); ++i) {
      FeatureHash hash = StringToFeatureHash(line[i]);
      all_functions[index].push_back(feature_to_vector_index[hash]);
    }
    ++index;
  }
  // Feature vector and function vector have been loaded. Now load the attraction
  // and repulsion set.
  std::vector<std::vector<std::string>> attract_file_contents;
  if (!FileToLineTokens(directory + "/attract.txt", &attract_file_contents)) {
    return -1;
  }

  std::vector<std::vector<std::string>> repulse_file_contents;
  if (!FileToLineTokens(directory + "/repulse.txt", &repulse_file_contents)) {
    return -1;
  }

  std::vector<std::pair<uint32_t, uint32_t>> attractionset;
  for (const std::vector<std::string>& line : attract_file_contents) {
    attractionset.push_back(std::make_pair(
      function_to_index[line[0]],
      function_to_index[line[1]]));
  }

  std::vector<std::pair<uint32_t, uint32_t>> repulsionset;
  for (const std::vector<std::string>& line : repulse_file_contents) {
    repulsionset.push_back(std::make_pair(
      function_to_index[line[0]],
      function_to_index[line[1]]));
  }

  printf("[!] Loaded %ld functions (%ld unique features)\n",
    all_functions.size(), all_features_vector.size());
  printf("[!] Attraction-Set: %ld pairs\n", attractionset.size());
  printf("[!] Repulsion-Set: %ld pairs\n", repulsionset.size());

  printf("[!] Training data parsed, beginning the training process.\n");

  SimHashTrainer trainer(
    &all_functions,
    &all_features_vector,
    &attractionset,
    &repulsionset);
  std::vector<double> weights;
  trainer.Train(&weights);

  // Write the weights file.
  {
    std::ofstream outfile(outputfile);
    if (!outfile) {
      printf("Failed to open outputfile %s.\n", outputfile.c_str());
      return false;
    }
    for (uint32_t i = 0; i < all_features_vector.size(); ++i) {
      char buf[512];
      FeatureHash& hash = all_features_vector[i];
      sprintf(buf, "%16.16lx%16.16lx %f", hash.first, hash.second,
        weights[i]);
      outfile << buf << std::endl;
    }
  }

  /*
  // Training has been performed. Instantiate two FunctionSimHasher, one with
  // the new weights, one without.
  FunctionSimHasher hash_no_weight("", false);
  FunctionSimHasher hash_weights(outputfile, false);

  for (const auto& pair : attractionset) {
    std::vector<FeatureHash> features_A;
    std::vector<FeatureHash> features_B;
    for (uint32_t index : all_functions[pair.first]) {
      features_A.push_back(all_features_vector[index]);
    }
    for (uint32_t index : all_functions[pair.second]) {
      features_B.push_back(all_features_vector[index]);
    }
    std::vector<uint64_t> function_A_hash;
    std::vector<uint64_t> function_B_hash;
    hash_no_weight.CalculateFunctionSimHash(
      &features_A, &function_A_hash);
    hash_no_weight.CalculateFunctionSimHash(
      &features_B, &function_B_hash);
    
    FunctionFeaturesi* functionA = &all_functions[pair.first];

  }*/
}

