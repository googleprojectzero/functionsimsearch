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
#include <mutex>
#include "CodeObject.h"
#include "InstructionDecoder.h"

#include "disassembly/flowgraphutil_dyninst.hpp"
#include "disassembly/functionfeaturegenerator.hpp"
#include "disassembly/dyninstfeaturegenerator.hpp"
#include "disassembly/pecodesource.hpp"
#include "disassembly/disassembly.hpp"

using namespace std;

Disassembly::Disassembly(const std::string& filetype,
  const std::string& inputfile) : type_(filetype), inputfile_(inputfile) {
  code_object_ = nullptr;
  code_source_ = nullptr;
  if ((type_ == "ELF") || (type_ == "PE")) {
    uses_dyninst_ = true;
  } else if (type_ == "JSON") {
    uses_dyninst_ = false;
  }
}

Disassembly::~Disassembly() {
  delete code_object_;
  if (type_ == "ELF") {
    Dyninst::ParseAPI::SymtabCodeSource* symtab_code_source =
      static_cast<Dyninst::ParseAPI::SymtabCodeSource*>(code_source_);
    delete symtab_code_source;
  } else if (type_ == "PE") {
    PECodeSource* pe_code_source = static_cast<PECodeSource*>(code_source_);
    delete pe_code_source;
  }
}

void Disassembly::LoadFromJSONStream(std::istream& jsondata) {
  nlohmann::json incoming_data;
  incoming_data << jsondata;
  // Check that we have the mandatory "functions" field.
  for (const auto& flowgraph : incoming_data["flowgraphs"]) {
    json_functions_.emplace_back(new FlowgraphWithInstructions());
    json_functions_.back()->ParseJSON(flowgraph);
    for (const auto& instruction : json_functions_.back()->GetInstructions()) {
      uint64_t address = instruction.first;
      blocks_to_flowgraph_[address] = json_functions_.size()-1;
    }
  }
}

bool Disassembly::Load(bool perform_parsing) {
  if (uses_dyninst_) {
    std::scoped_lock<std::mutex> lock(dyninst_api_mutex_);

    Dyninst::InstructionAPI::Instruction::Ptr instruction;

    if (type_ == "ELF") {
      Dyninst::SymtabAPI::Symtab* sym_tab = nullptr;
      Dyninst::ParseAPI::SymtabCodeSource* symtab_code_source = nullptr;
      bool is_parseable = Dyninst::SymtabAPI::Symtab::openFile(sym_tab,
        inputfile_);
      if (is_parseable == false) {
        printf("Error: ELF File cannot be parsed.\n");
        return false;
      }
      // Brutal const_cast because SymtabCodeSource for some reason wants a
      // char * instead of a const char*.
      symtab_code_source = new Dyninst::ParseAPI::SymtabCodeSource(
        const_cast<char*>(inputfile_.c_str()));
      code_source_ = static_cast<Dyninst::ParseAPI::CodeSource*>(
        symtab_code_source);
      Dyninst::ParseAPI::SymtabCodeSource::addNonReturning("__stack_chk_fail");
    } else if (type_ == "PE") {
      PECodeSource* pe_code_source = new PECodeSource(inputfile_);
      if (pe_code_source->parsed() == false) {
        printf("Error: PE File cannot be parsed.\n");
        return false;
      }
      code_source_ = static_cast<Dyninst::ParseAPI::CodeSource*>(pe_code_source);
    } else {
      printf("Error: Unknown filetype specified.\n");
      return false;
    }

    code_object_ = new Dyninst::ParseAPI::CodeObject(code_source_);

    if (perform_parsing) {
      // Parse the obvious function entries.
      code_object_->parse();

      // Parse the gaps.
      for (Dyninst::ParseAPI::CodeRegion* region : code_source_->regions()) {
        code_object_->parseGaps(region,
          Dyninst::ParseAPI::GapParsingType::IdiomMatching);
        code_object_->parseGaps(region, 
          Dyninst::ParseAPI::GapParsingType::PreambleMatching);
      }
      RefreshFunctionVector();
    }
    return true;
  } else if (type_ == "JSON") {
    // Perform the JSON input parsing.
    std::ifstream inputfile(inputfile_);
    LoadFromJSONStream(inputfile);
    if (GetNumberOfFunctions() != 0) {
      return true;
    }
  }
  return false;
}

void Disassembly::RefreshFunctionVector() {
  if (uses_dyninst_) {
    dyninst_functions_.clear();
    for (Dyninst::ParseAPI::Function* function : code_object_->funcs()) {
      dyninst_functions_.push_back(function);
    }
  }
}

void Disassembly::DisassembleFromAddress(uint64_t address, bool recursive) {
  if (uses_dyninst_) {
    scoped_lock<std::mutex> lock(dyninst_api_mutex_);
    code_object_->parse(address, recursive);

    RefreshFunctionVector();
  }
}

std::unique_ptr<FunctionFeatureGenerator> Disassembly::GetFeatureGenerator(
  uint32_t function_index) const {
  if (uses_dyninst_) {
    std::scoped_lock lock(dyninst_api_mutex_);
    Dyninst::ParseAPI::Function* function = dyninst_functions_.at(
      function_index);
    std::unique_ptr<FunctionFeatureGenerator> generator(
      static_cast<FunctionFeatureGenerator *>(new DyninstFeatureGenerator(
      function)));
    return generator;
  } else {
    return std::unique_ptr<FunctionFeatureGenerator>(
      new FlowgraphWithInstructionsFeatureGenerator(
      *json_functions_[function_index]));
  }
  return std::unique_ptr<FunctionFeatureGenerator>(nullptr);
}

std::unique_ptr<FlowgraphWithInstructions>
  Disassembly::GetFlowgraphWithInstructions(
  uint32_t function_index) const {
  if (uses_dyninst_) {
    std::scoped_lock lock(dyninst_api_mutex_);
    Dyninst::ParseAPI::Function* function = dyninst_functions_.at(
      function_index); 
    std::unique_ptr<FlowgraphWithInstructions> flowgraph(
      new FlowgraphWithInstructions());
    BuildFlowgraph(function, flowgraph.get());
    InstructionGetter get_block = GetInstructionGetter();
    std::vector<address> nodes;
    flowgraph->GetNodes(&nodes);
    for (const address& node_address : nodes) {
      std::vector<Instruction> instructions;
      get_block(node_address, &instructions);
      flowgraph->AddInstructions(node_address, instructions);
    }
    return flowgraph;
  } else if (function_index < GetNumberOfFunctions()) {
    FlowgraphWithInstructions* flowgraph = json_functions_[function_index].get();
    return std::unique_ptr<FlowgraphWithInstructions>(
      new FlowgraphWithInstructions(*flowgraph));
  }
  return std::unique_ptr<FlowgraphWithInstructions>(nullptr);
}

std::unique_ptr<Flowgraph> Disassembly::GetFlowgraph(uint32_t function_index)
  const {
  return GetFlowgraphWithInstructions(function_index);
}

uint64_t Disassembly::GetAddressOfFunction(uint32_t function_index) const {
  if (uses_dyninst_) {
    Dyninst::ParseAPI::Function* function = dyninst_functions_.at(function_index);
    return function->addr();
  } else {
    // In a rather inelegant move, the node with the lowest address is declared
    // to be the address of the function.
    // TODO(thomasdullien): Find a performant better variant of this.
    return json_functions_[function_index]->GetInstructions().begin()->first;
  }
}

uint32_t Disassembly::GetNumberOfFunctions() const {
  if (uses_dyninst_) {
    return dyninst_functions_.size();
  }
  return json_functions_.size();
}

std::string Disassembly::GetDisassembly(uint32_t function_index) const {
  if (uses_dyninst_) {
    std::scoped_lock<std::mutex> lock(dyninst_api_mutex_);
    Dyninst::ParseAPI::Function* function = dyninst_functions_.at(
      function_index);

    std::stringstream output;
    Dyninst::InstructionAPI::InstructionDecoder decoder(
      function->isrc()->getPtrToInstruction(function->addr()),
      Dyninst::InstructionAPI::InstructionDecoder::maxInstructionLength,
      function->region()->getArch());

    output << "\n[!] Function at " << std::hex << function->addr();

    for (const auto& block : function->blocks()) {
      output << "\t\tBlock at " << std::hex << block->start();

      Dyninst::ParseAPI::Block::Insns block_instructions;
      block->getInsns(block_instructions);

      output << "(" << std::dec << static_cast<size_t>(block_instructions.size())
        << ")\n";

      for (const auto& instruction : block_instructions) {
        std::string rendered = instruction.second->format(instruction.first);
        output << "\t\t\t " << std::hex << instruction.first << ": " <<
        rendered << "\n";
      }
    }
    return output.str();
  } else {
    return json_functions_[function_index]->GetDisassembly();
  }
}

bool Disassembly::ContainsSharedBasicBlocks(uint32_t index) const {
  if (uses_dyninst_) {
    std::scoped_lock<std::mutex> lock(dyninst_api_mutex_);
    Dyninst::ParseAPI::Function* function = dyninst_functions_.at(index);

    bool has_shared_blocks = false;
    for (const auto& block : function->blocks()) {
      std::vector<Dyninst::ParseAPI::Function *> functions_for_block;
      block->getFuncs(functions_for_block);
      if (functions_for_block.size() > 1) {
        return true;
      }
    }
  }
  return false;
}

InstructionGetter Disassembly::GetInstructionGetter() const {
  if (uses_dyninst_) {
    return MakeDyninstInstructionGetter(code_object_);
  } else {
    InstructionGetter getter = [this](uint64_t address,
      std::vector<Instruction>* results) -> bool {
      const auto& entry = this->blocks_to_flowgraph_.find(address);
      if (entry == this->blocks_to_flowgraph_.end()) {
        return false;
      }
      *(results) = json_functions_[entry->second]->GetInstructions().at(address);
      return true;
    };
    return getter;
  }
}

uint32_t Disassembly::GetIndexByAddress(uint64_t address) const {
  // TODO(thomasdullien): Implement.
  if (uses_dyninst_) {
    for (uint32_t index = 0; index < dyninst_functions_.size(); ++index) {
      if (dyninst_functions_.at(index)->addr() == address) {
        return index;
      }
    }
  }
  return std::numeric_limits<uint32_t>::max();
}

