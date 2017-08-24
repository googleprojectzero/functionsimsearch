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
#include "CodeObject.h"
#include "InstructionDecoder.h"

#include "disassembly.hpp"
#include "flowgraph.hpp"
#include "flowgraphutil.hpp"
#include "pecodesource.hpp"

using namespace std;
using namespace Dyninst;
using namespace ParseAPI;
using namespace InstructionAPI;

int main(int argc, char** argv) {
  if ((argc < 4) || (argc > 5)) {
    printf("Dumps all CFGs in the target binary as .dot files\n");
    printf("Usage: %s <PE/ELF> <binary path> <output dir> "
      "<optional: function_address>\n", argv[0]);
    return -1;
  }
  std::string mode(argv[1]);
  std::string binary_path_string(argv[2]);
  // Optional argument: A single function that should be explicitly disassembled
  // to make sure it is in the disassembly.
  uint64_t function_address = 0;
  if (argc == 5) {
    function_address = strtoul(argv[4], nullptr, 16);
  }

  Disassembly disassembly(mode, binary_path_string);
  if (!disassembly.Load(function_address == 0)) {
    exit(1);
  }
  if (function_address) {
    disassembly.DisassembleFromAddress(function_address, true);
  }

  std::string output_path_string(argv[3]);

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
