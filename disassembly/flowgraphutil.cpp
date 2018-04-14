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

#include "CodeObject.h"
#include "InstructionDecoder.h"

#include "disassembly/disassembly.hpp"
#include "disassembly/flowgraph.hpp"
#include "disassembly/pecodesource.hpp"
#include "util/util.hpp"

#include "disassembly/flowgraphutil.hpp"

uint64_t BuildFlowgraph(Dyninst::ParseAPI::Function* function,
  Flowgraph* graph) {
  uint64_t count = 0;
  for (const auto& block : function->blocks()) {
    graph->AddNode(block->start());
    for (const Dyninst::ParseAPI::Edge* edge : block->targets()) {
      if ((edge->type() == Dyninst::ParseAPI::CALL) ||
          (edge->type() == Dyninst::ParseAPI::RET)) {
        // Do not add edges for calls and RETs.
        continue;
      }
      graph->AddEdge(edge->src()->start(), edge->trg()->start());
    }
    count++;
  }
  return count;
}

InstructionGetter FlowgraphWithInstructionInstructionGetter(
  FlowgraphWithInstructions* flowgraph) {
  InstructionGetter getter = [flowgraph](uint64_t address,
    std::vector<Instruction>* results) -> bool {
    const auto& iter = flowgraph->GetInstructions().find(address);
    if (iter == flowgraph->GetInstructions().end()) {
      return false;
    }
    *(results) = iter->second;
    return true;
  };
  return getter;
}

// A helper function to provide an InstructionGetter that abstracts away the
// DynInst API.
InstructionGetter MakeDyninstInstructionGetter(
  Dyninst::ParseAPI::CodeObject* codeobject) {
  InstructionGetter getter =
    [codeobject](uint64_t address, std::vector<Instruction>* results) -> bool {
    Dyninst::ParseAPI::CodeSource* codesource = codeobject->cs();
    for (Dyninst::ParseAPI::CodeRegion* region : codesource->regions()) {
      Dyninst::ParseAPI::Block* block = codeobject->findBlockByEntry(
        region, address);
      if (!block) {
        continue;
      }
      results->clear();
      Dyninst::ParseAPI::Block::Insns block_instructions;
      block->getInsns(block_instructions);
      for (const auto& instruction : block_instructions) {
        std::vector<std::string> operand_strings;
        std::vector<Dyninst::InstructionAPI::Operand> operands;
        instruction.second->getOperands(operands);
        for (const auto& operand : operands) {
          operand_strings.emplace_back(
            operand.format(
              instruction.second->getArch(),
              instruction.first));
        }
        results->emplace_back(Instruction(
          instruction.second->getOperation().format(), operand_strings));
      }
      if (!results->empty()) {
        return true;
      }
    }
    return false;
  };
  return getter;
}

// TODO(thomasdullien): The following two functions share more than a little
// bit of code and should probably be consolidated (e.g. the common code should
// be factored out).
bool GetCFGFromBinaryAsJSON(const std::string& format, const std::string
  &inputfile, uint64_t address, std::string* result) {

  Disassembly disassembly(format, inputfile);
  if (!disassembly.Load(false)) {
    return false;
  }
  disassembly.DisassembleFromAddress(address, true);

  Dyninst::ParseAPI::CodeObject* code_object = disassembly.getCodeObject();
  // Obtain the list of all functions in the binary.
  const Dyninst::ParseAPI::CodeObject::funclist &functions = code_object->funcs();
  if (functions.size() == 0) {
    printf("No functions found.\n");
    return false;
  }

  Dyninst::InstructionAPI::Instruction::Ptr instruction;
  InstructionGetter get_block = MakeDyninstInstructionGetter(code_object);
  for (Dyninst::ParseAPI::Function* function : functions) {

    Flowgraph graph;
    Dyninst::Address function_address = function->addr();
    if (function_address == address) {
      BuildFlowgraph(function, &graph);
      std::stringstream json_data;
      graph.WriteJSON(&json_data, get_block);
      *result = json_data.str();
      return true;
    }
  }
  return false;
}

FlowgraphWithInstructions* GetCFGWithInstructionsFromBinary(
  const std::string& format, const std::string &inputfile,
  uint64_t func_address) {

  Disassembly disassembly(format, inputfile);
  if (!disassembly.Load(false)) {
    return nullptr;
  }
  disassembly.DisassembleFromAddress(func_address, true);

  Dyninst::ParseAPI::CodeObject* code_object = disassembly.getCodeObject();
  // Obtain the list of all functions in the binary.
  const Dyninst::ParseAPI::CodeObject::funclist &functions = code_object->funcs();
  if (functions.size() == 0) {
    printf("No functions found.\n");
    return nullptr;
  }

  Dyninst::InstructionAPI::Instruction::Ptr instruction;
  InstructionGetter get_block = MakeDyninstInstructionGetter(code_object);
  for (Dyninst::ParseAPI::Function* function : functions) {
    Dyninst::Address function_address = function->addr();
    if (function_address == func_address) {
      FlowgraphWithInstructions* graph = new FlowgraphWithInstructions();
      BuildFlowgraph(function, graph);
      std::vector<address> nodes;
      graph->GetNodes(&nodes);
      for (const address& node_address : nodes) {
        std::vector<Instruction> instructions;
        get_block(node_address, &instructions);
        graph->AddInstructions(node_address, instructions);
      }
      return graph;
    }
  }
  return nullptr;
}


