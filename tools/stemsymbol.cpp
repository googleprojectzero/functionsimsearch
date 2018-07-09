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

#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <deque>
#include <string>

#include "util/cppsplitter.hpp"
#include "util/util.hpp"

using namespace std;

void replaceAll(std::string& str, const std::string& from,
  const std::string& to) {
  if(from.empty()) {
    return;
  }
  size_t start_pos = 0;
  while((start_pos = str.find(from, start_pos)) != std::string::npos) {
    str.replace(start_pos, from.length(), to);
    start_pos += to.length();
    // In case 'to' contains 'from', like replacing 'x' with 'yx'
  }
}


// At tiny command line tool used to "stem" demangled symbols from visual-studio
// compiled executables so they can be compared to GCC-generated symbols. It is
// written in entirely the wrong way and ugly as hell. I would love to have a
// better way to do this.

int main(int argc, char** argv) {
  std::string input;
  std::getline(std::cin, input);
  std::string temp;

  replaceAll(input, "struct", "");
  replaceAll(input, "class", "");
  replaceAll(input, "enum", "");
  replaceAll(input, "(void)", "()");
  replaceAll(input, "& ", "&");
  replaceAll(input, ",", ", ");
  replaceAll(input, "bool&", "bool &");
  replaceAll(input, " *", "*");
  replaceAll(input, " __ptr64", "");
  replaceAll(input, "__ptr64", "");
  replaceAll(input, "__int64", "long");
  replaceAll(input, " &", "&");
  replaceAll(input, " ,", ",");
  replaceAll(input, " )", ")");
  //replaceAll(input, "const*", "const *");

  // Now we need to stem off the return type.
  std::deque<std::string> tokens;
  CppSplitter(input, tokens);
  //split(input, ' ', std::back_inserter(tokens));
  std::deque<std::string> tokens2(tokens);

  // Remove tokens that describe the return value.
  while(tokens2.size() > 0) {
    if (tokens2[0].find("(") == string::npos) {
      tokens2.pop_front();
    } else {
      break;
    }
  }
  // If the last token is enclosed in [..], remove it too.
  if (tokens2.size() > 0) {
    if ((tokens2[tokens2.size()-1][0] == '[') && 
      tokens2[tokens.size()-1].size() > 2) {
      tokens2.pop_back();
    }
  }
  std::stringstream ss;
  while (tokens2.size() > 0) {
    if (tokens2.size() > 1) {
      if (tokens2[0].c_str()[tokens2[0].size()-1] == '(') {
        ss << tokens2[0];
      } else {
        if (tokens2.size() > 2) {
          if (tokens2[1].c_str()[0] == '&') {
            ss << tokens2[0];
          } else {
            ss << tokens2[0] << " ";
          }
        } else {
          ss << tokens2[0] << " ";
        }
      }
    } else {
      ss << tokens2[0];
    }
    tokens2.pop_front();
  }
  std::string result = ss.str();
  replaceAll(result, "  ", " ");
  printf("%s", result.c_str());
}

