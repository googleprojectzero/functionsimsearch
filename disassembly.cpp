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
#include "pecodesource.hpp"

#include "disassembly.hpp"

using namespace std;
using namespace Dyninst;
using namespace ParseAPI;
using namespace InstructionAPI;

Disassembly::Disassembly(const std::string& filetype,
  const std::string& inputfile) : type_(filetype), inputfile_(inputfile) {
  code_object_ = nullptr;
  code_source_ = nullptr;
}

Disassembly::~Disassembly() {
  delete code_object_;
  if (type_ == "ELF") {
    SymtabCodeSource* symtab_code_source =
      static_cast<SymtabCodeSource*>(code_source_);
    delete symtab_code_source;
  } else if (type_ == "PE") {
    PECodeSource* pe_code_source = static_cast<PECodeSource*>(code_source_);
    delete pe_code_source;
  }
}

bool Disassembly::Load(bool perform_parsing) {
  Instruction::Ptr instruction;

  if (type_ == "ELF") {
    SymtabAPI::Symtab* sym_tab = nullptr;
    SymtabCodeSource* symtab_code_source = nullptr;
    bool is_parseable = SymtabAPI::Symtab::openFile(sym_tab, inputfile_);
    if (is_parseable == false) {
      printf("Error: ELF File cannot be parsed.\n");
      return false;
    }
    // Brutal C-style cast because SymtabCodeSource for some reason wants a
    // char * instead of a const char*.
    symtab_code_source = new SymtabCodeSource((char *)inputfile_.c_str());
    code_source_ = static_cast<CodeSource*>(symtab_code_source);
  } else if (type_ == "PE") {
    PECodeSource* pe_code_source = new PECodeSource(inputfile_);
    if (pe_code_source->parsed() == false) {
      printf("Error: PE File cannot be parsed.\n");
      return false;
    }
    code_source_ = static_cast<CodeSource*>(pe_code_source);
  } else {
    printf("Error: Unknown filetype specified.\n");
    return false;
  }

  code_object_ = new CodeObject(code_source_);

  if (perform_parsing) {
    // Parse the obvious function entries.
    code_object_->parse();

    // Parse the gaps.
    for (CodeRegion* region : code_source_->regions()) {
      code_object_->parseGaps(region, GapParsingType::IdiomMatching);
      code_object_->parseGaps(region, GapParsingType::PreambleMatching);
    }
  }
  return true;
}

void Disassembly::DisassembleFromAddress(uint64_t address, bool recursive) {
  code_object_->parse(address, recursive);
}

