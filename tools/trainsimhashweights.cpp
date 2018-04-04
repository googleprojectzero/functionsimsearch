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

#include "util/util.hpp"
#include "learning/simhashtrainer.hpp"

DEFINE_string(data, "./data", "Data directory");
DEFINE_string(weights, "weights.txt", "Feature weights file");
DEFINE_uint64(train_steps, 500, "Number of training steps");
// The google namespace is there for compatibility with legacy gflags and will
// be removed eventually.
#ifndef gflags
using namespace google;
#else
using namespace gflags;
#endif

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
  SetUsageMessage(
    "Use labelled and unlabelled data to train a weights.txt file. Refer to the "
    "source code for details regarding the data format in the data directory.");
  ParseCommandLineFlags(&argc, &argv, true);

  printf("[!] Parsing training data.\n");
  std::string directory(FLAGS_data);
  std::string outputfile(FLAGS_weights);
  uint32_t train_steps = FLAGS_train_steps;

  if(!TrainSimHashFromDataDirectory(directory, outputfile, true, train_steps)) {
    printf("[!] Training failed, exiting.\n");
    return -1;
  }
}
