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


#include "InstructionDecoder.h"

#include "flowgraphutil.hpp"

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
