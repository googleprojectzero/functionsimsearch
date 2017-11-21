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

#include "CodeObject.h"
#include "InstructionDecoder.h"

#include "disassembly.hpp"
#include "flowgraph.hpp"
#include "flowgraphutil.hpp"
#include "pecodesource.hpp"

DEFINE_string(format, "PE", "Executable format: PE or ELF");
DEFINE_string(input, "", "File to disassemble");
DEFINE_string(output, "/var/tmp", "Output directory to dump .dot files to");
DEFINE_string(function_address, "", "Address of function (optional)");
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

  CodeObject* code_object = disassembly.getCodeObject();

  // Obtain the list of all functions in the binary.
  const CodeObject::funclist &functions = code_object->funcs();
  if (functions.size() == 0) {
    printf("No functions found.\n");
    return -1;
  }

  Instruction::Ptr instruction;
  for (Function* function : functions) {
    Flowgraph graph;
    Address function_address = function->addr();
    BuildFlowgraph(function, &graph);

    char buf[200];
    sprintf(buf, "sub_%lx.dot", function_address);
    std::string filename = output_path_string + "/" + std::string(buf);
    graph.WriteDot(filename);
  }
}
