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
#include <fstream>
#include <string>
#include <boost/tokenizer.hpp>

#include "searchbackend/functionmetadata.hpp"

std::vector<std::string> split(const char* string, char c = ' ') {
  std::vector<std::string> result;
  do {
    const char *begin = string;
    while (*string != c && *string) {
      ++string;
    }
    result.push_back(std::string(begin, string));
  } while (0 != *string++);
  return result;
}

FunctionMetadataStore::FunctionMetadataStore(const std::string& csvfile) {
  std::ifstream infile;
  infile.open(csvfile);

  while (infile) {
    std::string line;
    if (!getline( infile, line )) {
      break;
    }
    std::vector<std::string> tokens = split(line.c_str(), ' ');
    if (tokens.size() != 5) {
      break;
    }
    uint64_t file_id = strtoul(tokens[0].c_str(), nullptr, 16);
    std::string& filename = tokens[1];
    uint64_t function_address = strtoul(tokens[2].c_str(), nullptr, 16);
    std::string& functionname = tokens[3];
    std::string& vulnerable = tokens[4];
    file_id_to_name_[file_id] = filename;
    // Function names are read in base64-encoded format.
    function_to_name_[std::make_pair(file_id, function_address)] =
      functionname;
    function_to_vuln_[std::make_pair(file_id, function_address)] =
      vulnerable == "true" ? true : false;
  }
}

bool FunctionMetadataStore::GetFunctionName(uint64_t file_id,
  uint64_t address, std::string* out) {
  auto pair = std::make_pair(file_id, address);
  if (function_to_name_.find(pair) != function_to_name_.end()) {
    *out = function_to_name_.at(pair);
    return true;
  }
  return false;
}

bool FunctionMetadataStore::GetFileName(uint64_t file_id, std::string* out) {
  if (file_id_to_name_.find(file_id) != file_id_to_name_.end()) {
    *out = file_id_to_name_.at(file_id);
    return true;
  }
  return false;
}

bool FunctionMetadataStore::FunctionHasVulnerability(uint64_t file_id,
  uint64_t address) {
  auto pair = std::make_pair(file_id, address);
  if (function_to_vuln_.find(pair) != function_to_vuln_.end()) {
    return function_to_vuln_.at(pair);
  }
  return false;
}

void FunctionMetadataStore::AddFunctionName(uint64_t file_id,
  uint64_t address, const std::string& function_name) {

}

void FunctionMetadataStore::SetFunctionIsVulnerable(uint64_t file_id,
  uint64_t address, bool val) {
  function_to_vuln_[std::make_pair(file_id, address)] = val;
}

void FunctionMetadataStore::AddFileName(uint64_t file_id,
  const std::string& file_name) {
  file_id_to_name_[file_id] = file_name;
}
