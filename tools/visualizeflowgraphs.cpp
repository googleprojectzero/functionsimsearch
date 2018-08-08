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
#include <map>
#include <string>
#include <gflags/gflags.h>

#include "disassembly/disassembly.hpp"
#include "disassembly/flowgraph.hpp"
#include "disassembly/flowgraphutil_dyninst.hpp"
#include "disassembly/pecodesource.hpp"
#include "util/util.hpp"

// A command-line tool that writes HTML files that allow the side-by-side
// visualisation of up to 6 different CFGs.
//
// The input string specifies triples of file-format, filename, address.
//
DEFINE_string(input, "ELF,/bin/tar,0x8BD0,ELF,/bin/gzip,0x2320", "...");
DEFINE_string(output, "/var/tmp/results.html", "Output html file.");

// The google namespace is there for compatibility with legacy gflags and will
// be removed eventually.
#ifndef gflags
using namespace google;
#else
using namespace gflags;
#endif

using namespace std;

int main(int argc, char** argv) {
  SetUsageMessage(
    "Writes interactive visualisation of CFGs into an HTML file.");
  ParseCommandLineFlags(&argc, &argv, true);

  printf("%s\n", FLAGS_input.c_str());
  // Read the visualization HTML template into an std::string.
  // TODO(thomasdullien): Make sure the executable can find the right
  // path?
  std::ifstream templatefile("../testdata/graph_template.html");
  std::string template_string((std::istreambuf_iterator<char>(templatefile)),
    std::istreambuf_iterator<char>());

  // Now obtain the JSON description of each of the functions specified in the
  // input string.
  std::vector<std::string> tokens = Tokenize(FLAGS_input.c_str(), ',');
  if ((tokens.size() % 3) != 0) {
    printf("[!] Invalid input command line argument (%d tokens).\n",
      tokens.size());
    exit(1);
  }

  std::vector<std::string> jsons;
  uint32_t tokens_index = 0;
  while (tokens_index < tokens.size()) {
    uint64_t address = strtoull(tokens[tokens_index + 2].c_str(), nullptr, 16);
    std::string result;
    bool res = GetCFGFromBinaryAsJSON(tokens[tokens_index],
      tokens[tokens_index+1], address, &result);
    if (!res) {
      printf("[!] Failed to load %s:%s:%s\n", tokens[tokens_index].c_str(),
        tokens[tokens_index+1].c_str(), tokens[tokens_index+2].c_str());
      exit(1);
    }
    printf("Got JSON for function!\n");
    tokens_index += 3;
    jsons.push_back(result);
    if (jsons.size() >= 6) {
      break;
    }
  }
  while (jsons.size() != 6) {
    jsons.push_back("");
  }

  // We have 6 JSON descriptions of the graph. Insert it into the template.
  std::vector<std::string> replacements( {
    "<!-- %%GRAPH0%% -->", "<!-- %%GRAPH1%% -->", "<!-- %%GRAPH2%% -->",
    "<!-- %%GRAPH3%% -->", "<!-- %%GRAPH4%% -->", "<!-- %%GRAPH5%% -->" } );
  for (uint32_t index = 0; index < 6; ++index) {
    size_t search_index = template_string.find(replacements[index]);
    if (search_index == std::string::npos) {
      printf("[!] Failed to find replacement string %s in template?\n",
        replacements[index].c_str());
    }
    template_string.replace(search_index, strlen(replacements[index].c_str()),
      jsons[index]);
  }

  std::ofstream html_file;
  html_file.open(FLAGS_output);
  html_file << template_string;
  html_file.close();
}
