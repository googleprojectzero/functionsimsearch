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

  // The code will perform two iterations over functions.txt, the first to
  // extract all the features, the second to build the data structures required
  // for the SimHashWeightLossFunction.

  std::string directory(argv[1]);
  std::ifstream functionsfile(directory + "/functions.txt");
  if (!functionsfile) {
    printf("Failed to open functions.txt in data directory.\n");
    return -1;
  }

  // We want features sorted and de-duplicated in the end, so use a set.
  std::set<FeatureHash> all_features;
  std::string line;
  while (std::getline(functionsfile, line)) {
    std::vector<std::string> tokens;
    split(line, ' ', std::back_inserter(tokens));
    for (uint32_t index = 1; index < tokens.size(); ++index) {
      std::string& token = tokens[index];
      std::string first_half_string;
      std::string second_half_string;
      if (token.c_str()[2] == '.') {
        std::string first_half_string = token.substr(3, 16+3);
        std::string second_half_string = token.substr(16+3, string::npos);
      } else {
        std::string first_half_string = token.substr(0, 16);
        std::string second_half_string = token.substr(16, string::npos);
      }
      const char* first_half = first_half_string.c_str();
      const char* second_half = second_half_string.c_str();
      uint64_t hashA = strtoull(first_half, nullptr, 16);
      uint64_t hashB = strtoull(second_half, nullptr, 16);
      printf("%s - %16.16lx%16.16lx\n", token.c_str(), hashA, hashB);
    }
  }
}
