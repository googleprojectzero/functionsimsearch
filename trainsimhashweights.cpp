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
#include <spii/function.h>
#include <spii/solver.h>
#include <spii/term.h>

#include "simhashweightslossfunctor.hpp"

using namespace std;

// String split functionality.
template<typename Out>
void split(const std::string &s, char delim, Out result) {
  std::stringstream ss;
  ss.str(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    *(result++) = item;
  }
}

bool FileToLineTokens(const std::string& filename,
  std::vector<std::vector<std::string>>* tokenized_lines) {
  std::ifstream inputfile(filename);
  if (!inputfile) {
    printf("Failed to open inputfile %s.\n", filename.c_str());
    return false;
  }

  uint32_t line_index = 0;
  // We want features sorted and de-duplicated in the end, so use a set.
  std::string line;
  while (std::getline(inputfile, line)) {
    std::vector<std::string> tokens;
    split(line, ' ', std::back_inserter(tokens));
    tokenized_lines->push_back(tokens);
  }
  return true;
}

FeatureHash StringToFeatureHash(const std::string& hash_as_string) {
  const std::string& token = hash_as_string;
  std::string first_half_string;
  std::string second_half_string;
  if (token.c_str()[2] == '.') {
    first_half_string = token.substr(3, 16);
    second_half_string = token.substr(16+3, string::npos);
  } else {
    first_half_string = token.substr(0, 16);
    second_half_string = token.substr(16, string::npos);
  }
  const char* first_half = first_half_string.c_str();
  const char* second_half = second_half_string.c_str();
  uint64_t hashA = strtoull(first_half, nullptr, 16);
  uint64_t hashB = strtoull(second_half, nullptr, 16);

  return std::make_pair(hashA, hashB);
}

void ReadFeatureSet(const std::vector<std::vector<std::string>>& inputlines,
  std::set<FeatureHash>* result) {
  for (const std::vector<std::string>& line : inputlines) {
    for (uint32_t index = 1; index < line.size(); ++index) {
      FeatureHash foo = StringToFeatureHash(line[index]);
      result->insert(StringToFeatureHash(line[index]));
    }
  }
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
//                  indicating which functions should NOT be the same
//

int main(int argc, char** argv) {
  if (argc != 2) {
    printf("Use labelled and unlablled data to train a weights.txt file.\n");
    printf("Usage: %s <data directory>\n", argv[0]);
    printf("Refer to the source code of this utility for detailed usage.\n");
    return -1;
  }

  printf("[!] Parsing training data.\n");
  std::string directory(argv[1]);
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

  spii::Function function;

  std::vector<uint32_t> unlabeledset;

  size_t number_of_features = all_features_vector.size();
  // Hard-code an upper limit of 400k unique features for the moment. This will
  // almost certainly need to be raised.

  std::vector<double> weights(number_of_features, 1.0);

  // Add a term for each example in the attraction set.
  for (uint32_t pair_index = 0; pair_index < attractionset.size(); ++pair_index) {
    printf("[!] Adding term for similar pair %d (%d, %d) to the loss function.\n",
      pair_index, attractionset[pair_index].first,
      attractionset[pair_index].second);
    function.add_term(std::make_shared<
      spii::AutoDiffTerm<SimHashPairLossTerm, spii::Dynamic>>(
        number_of_features,
        &all_features_vector,
        &all_functions,
        &attractionset,
        pair_index,
        true), weights.data());
  }

  // Add a term for each example in the repulsionset.
  for (uint32_t pair_index = 0; pair_index < repulsionset.size(); ++pair_index) {
    printf("[!] Adding term for dissimilar pair %d (%d, %d) to the loss function.\n",
      pair_index, repulsionset[pair_index].first,
      repulsionset[pair_index].second);
    function.add_term(
      std::make_shared<spii::AutoDiffTerm<SimHashPairLossTerm, spii::Dynamic>>(
        number_of_features,
        &all_features_vector,
        &all_functions,
        &repulsionset,
        pair_index,
        true), weights.data());
  }

  spii::LBFGSSolver solver;
  solver.maximum_iterations = 50;
  spii::SolverResults results;
  solver.solve(function, &results);
  cout << results << std::endl << std::endl;

}

