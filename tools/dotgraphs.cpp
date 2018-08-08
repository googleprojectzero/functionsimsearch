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
DEFINE_string(output, "/var/tmp", "Output directory to dump .dot files to");
DEFINE_string(function_address, "", "Address of function (optional)");
DEFINE_bool(json, false, "Also dump .json CFGs with instructions");
DEFINE_bool(no_shared_blocks, false, "Skip functions with shared blocks.");

// The google namespace is there for compatibility with legacy gflags and will
// be removed eventually.
#ifndef gflags
using namespace google;
#else
using namespace gflags;
#endif

using namespace std;

int main(int argc, char** argv) {
  SetUsageMessage(
    "Dumps the CFGs in the target binary as .dot files to the output directory.");
  ParseCommandLineFlags(&argc, &argv, true);

  std::string mode(FLAGS_format);
  std::string binary_path_string(FLAGS_input);
  // Optional argument: A single function that should be explicitly disassembled
  // to make sure it is in the disassembly.
  uint64_t function_address = 0;
  if (FLAGS_function_address != "") {
    function_address = strtoul(FLAGS_function_address.c_str(), nullptr, 16);
  }

  Disassembly disassembly(mode, binary_path_string);
  if (!disassembly.Load(function_address == 0)) {
    exit(1);
  }
  if (function_address) {
    disassembly.DisassembleFromAddress(function_address, true);
  }

  std::string output_path_string(FLAGS_output);

  if (disassembly.GetNumberOfFunctions() == 0) {
    printf("No functions found.\n");
    return -1;
  }

  InstructionGetter get_block = disassembly.GetInstructionGetter();
  for (uint32_t index = 0; index < disassembly.GetNumberOfFunctions(); ++index) {
    std::unique_ptr<Flowgraph> graph = disassembly.GetFlowgraph(index);
    uint64_t function_address = disassembly.GetAddressOfFunction(index);

    // Skip functions that contain shared basic blocks.
    if (FLAGS_no_shared_blocks && disassembly.ContainsSharedBasicBlocks(index)) {
      continue;
    }

    char buf[200];
    sprintf(buf, "sub_%lx.dot", function_address);
    std::string filename = output_path_string + "/" + std::string(buf);
    graph->WriteDot(filename);

    if (FLAGS_json) {
      sprintf(buf, "sub_%lx.json", function_address);
      filename = output_path_string + "/" + std::string(buf);
      graph->WriteJSON(filename, get_block);
    }
  }
}
