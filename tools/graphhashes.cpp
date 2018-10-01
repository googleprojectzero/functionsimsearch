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

#include <iostream>
#include <map>
#include <gflags/gflags.h>

#include "disassembly/disassembly.hpp"
#include "disassembly/flowgraph.hpp"
#include "disassembly/flowgraphutil_dyninst.hpp"
#include "disassembly/pecodesource.hpp"

DEFINE_string(format, "PE", "Executable format: PE,ELF,JSON");
DEFINE_string(input, "", "File to disassemble");
// The google namespace is there for compatibility with legacy gflags and will
// be removed eventually.
#ifndef gflags
using namespace google;
#else
using namespace gflags;
#endif

using namespace std;
using namespace Dyninst;
using namespace ParseAPI;
using namespace InstructionAPI;

int main(int argc, char** argv) {
  SetUsageMessage(
    "Dumps (non-fuzzy) hashes of all CFGs in the target binary to stdout.");
  ParseCommandLineFlags(&argc, &argv, true);

  std::string mode(FLAGS_format);
  std::string binary_path_string(FLAGS_input);
  Disassembly disassembly(mode, binary_path_string);
  if (!disassembly.Load()) {
    exit(1);
  }

  if (disassembly.GetNumberOfFunctions() == 0) {
    printf("No functions found.\n");
    return -1;
  }

  for (uint32_t index = 0; index < disassembly.GetNumberOfFunctions(); ++index) {
    std::unique_ptr<FlowgraphWithInstructions> graph =
      disassembly.GetFlowgraphWithInstructions(index);
    Address function_address = disassembly.GetAddressOfFunction(index);

    printf("%16.16lx %16.16lx\n", graph->CalculateHash(function_address),
      function_address);
  }
}
