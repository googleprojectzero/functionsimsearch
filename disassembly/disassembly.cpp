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
    }
    return true;
  }
  return false;
}

void Disassembly::DisassembleFromAddress(uint64_t address, bool recursive) {
  if (uses_dyninst_) {
    scoped_lock<std::mutex> lock(dyninst_api_mutex_);
    code_object_->parse(address, recursive);
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
  }
  return std::unique_ptr<FunctionFeatureGenerator>(nullptr);
}

std::unique_ptr<Flowgraph> Disassembly::GetFlowgraph(uint32_t function_index)
  const {
  if (uses_dyninst_) {
    std::scoped_lock lock(dyninst_api_mutex_);
    Dyninst::ParseAPI::Function* function = dyninst_functions_.at(
      function_index); 
    std::unique_ptr<Flowgraph> flowgraph(new Flowgraph());
    BuildFlowgraph(function, flowgraph.get());
    return flowgraph;
  }
  return std::unique_ptr<Flowgraph>(nullptr);
}

uint64_t Disassembly::GetAddressOfFunction(uint32_t function_index) const {
  if (uses_dyninst_) {
    Dyninst::ParseAPI::Function* function = dyninst_functions_.at(function_index);
    return function->addr();
  }
  return 0;
}

uint32_t Disassembly::GetNumberOfFunctions() const {
  if (uses_dyninst_) {
    return dyninst_functions_.size();
  }
  return 0;
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
  }
  return "";
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
  }
}

uint32_t Disassembly::GetIndexByAddress(uint64_t address) const {
  // TODO(thomasdullien): Implement.
  return 0;

}

