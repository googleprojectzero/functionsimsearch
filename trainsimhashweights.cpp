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
//                  indicating which functions should NOT be the same.
//

int main(int argc, char** argv) {
  if (argc != 3) {
    printf("Use labelled and unlabelled data to train a weights.txt file.\n");
    printf("Usage: %s <data directory> <weights file>\n", argv[0]);
    printf("Refer to the source code of this utility for detailed usage.\n");
    return -1;
  }

  printf("[!] Parsing training data.\n");
  std::string directory(argv[1]);
  std::string outputfile(argv[2]);

  if(!TrainSimHashFromDataDirectory(directory, outputfile, true, 400)) {
    printf("[!] Training failed, exiting.\n");
    return -1;
  }
}
