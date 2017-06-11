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
  if (argc != 3) {
    printf("Dumps hashes of all CFGs in the target binary to stdout\n");
    printf("Usage: %s <PE/ELF> <binary path>\n", argv[0]);
    return -1;
  }
  std::string mode(argv[1]);
  std::string binary_path_string(argv[2]);
  Disassembly disassembly(mode, binary_path_string);
  if (!disassembly.Load()) {
    exit(1);
  }
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

    printf("%16.16lx %16.16lx\n", graph.CalculateHash(function_address),
      function_address);
  }
}
