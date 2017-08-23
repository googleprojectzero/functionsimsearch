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
#include "pecodesource.hpp"

using namespace std;
using namespace Dyninst;
using namespace ParseAPI;
using namespace InstructionAPI;

int main(int argc, char** argv) {
  if ((argc < 3) || (argc > 4)) {
    printf("Usage: %s <PE/ELF> <binary path> <optional: function_address>\n",
      argv[0]);
    return -1;
  }
  std::string mode(argv[1]);
  std::string binary_path_string(argv[2]);

  // Optional argument: A single function that should be explicitly disassembled
  // to make sure it is in the disassembly.
  uint64_t function_address = 0;
  if (argc == 4) {
    function_address = strtoul(argv[3], nullptr, 16);
  }

  Disassembly disassembly(mode, binary_path_string);
  if (!disassembly.Load((function_address == 0))) {
    exit(1);
  }
  if (function_address) {
    disassembly.DisassembleFromAddress(function_address, false);
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
    InstructionDecoder decoder(function->isrc()->getPtrToInstruction(
      function->addr()), InstructionDecoder::maxInstructionLength,
      function->region()->getArch());
    Address function_address = function->addr();
    int instruction_count = 0;
    printf("\n[!] Function at %lx\n", function_address);
    for (const auto& block : function->blocks()) {
      printf("     Block at %lx", block->start());

      Dyninst::ParseAPI::Block::Insns block_instructions;
      block->getInsns(block_instructions);
      printf(" (%lu instructions)\n", static_cast<size_t>(
        block_instructions.size()));
      for (const auto& instruction : block_instructions) {
        std::string rendered = instruction.second->format(instruction.first);
        printf("          %lx: %s\n", instruction.first, rendered.c_str());
      }
    }
  }

}
