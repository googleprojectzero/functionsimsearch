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

#ifndef DISASSEMBLY_HPP
#define DISASSEMBLY_HPP

#include <mutex>
#include <string>
#include "CodeObject.h"
#include "disassembly/flowgraph.hpp"
#include "disassembly/functionfeaturegenerator.hpp"

class FlowgraphWithInstructions;

// A thin wrapper class around Dyninst to deal with some boilerplate code
// required for switching between PE and ELF files and other inputs. Also helps
// hiding the details of DynInst and whatever else produces disassemblies from
// the rest of the codebase (the external interface is free of pollution from
// disassembly libraries.
class Disassembly {
public:
  Disassembly(const std::string& filetype, const std::string& inputfile);
  virtual ~Disassembly();

  bool Load(bool perform_parsing = true);
  void DisassembleFromAddress(uint64_t address, bool recursive);

  // Allow users of this class to iterate through all functions in the binary.
  std::unique_ptr<FunctionFeatureGenerator> GetFeatureGenerator(
    uint32_t function_index) const;
  std::unique_ptr<Flowgraph> GetFlowgraph(uint32_t function_index) const;
  std::unique_ptr<FlowgraphWithInstructions> GetFlowgraphWithInstructions(
    uint32_t function_index) const;
  InstructionGetter GetInstructionGetter() const;
  uint64_t GetAddressOfFunction(uint32_t function_index) const;
  uint32_t GetNumberOfFunctions() const;
  std::string GetDisassembly(uint32_t function_index) const;
  uint32_t GetIndexByAddress(uint64_t address) const;
  // A helper function used to detect functions which have shared basic blocks
  // with other functions. This is mainly used to detect situations where 
  // Dyninst failed to properly disassembly a binary.
  bool ContainsSharedBasicBlocks(uint32_t function_index) const;
private:
  void RefreshFunctionVector();
  void LoadFromJSONStream(std::istream& jsondata);
  const std::string type_;
  const std::string inputfile_;
  // Dyninst-specific data members.
  bool uses_dyninst_;
  mutable std::mutex dyninst_api_mutex_;
  std::vector<Dyninst::ParseAPI::Function*> dyninst_functions_;
  Dyninst::ParseAPI::CodeObject* code_object_;
  Dyninst::ParseAPI::CodeSource* code_source_;
  // Data members for the JSON input.
  std::vector<std::unique_ptr<FlowgraphWithInstructions>> json_functions_;
  std::map<uint64_t, uint32_t> blocks_to_flowgraph_;
};

#endif // DISASSEMBLY_HPP
