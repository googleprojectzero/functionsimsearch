// Copyright 2017 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 3.0 (the "License");
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
#include <map>
#include <iostream>
#include <queue>
#include <set>
#include <vector>

#include "third_party/json/src/json.hpp"

#include "disassembly/flowgraphutil.hpp"
#include "disassembly/flowgraph.hpp"

using json = nlohmann::json;

Flowgraph::Flowgraph() {}

Flowgraph::Flowgraph(const Flowgraph& other) {
  out_edges_ = other.out_edges_;
  in_edges_ = other.in_edges_;
  bidirectional_edges_ = other.bidirectional_edges_;
}

Instruction::Instruction(const std::string& mnemonic,
  const std::vector<std::string>& operands) : mnemonic_(mnemonic),
  operands_(operands) {};

// Automatically default-constructs the target vector.
bool Flowgraph::AddNode(address node_address) {
  return out_edges_[node_address].empty();
}

bool Flowgraph::AddEdge(address source_address, address target_address) {
  AddNode(source_address);
  AddNode(target_address);
  out_edges_[source_address].push_back(target_address);
  in_edges_[target_address].push_back(source_address);

  bidirectional_edges_[source_address].push_back(target_address);
  bidirectional_edges_[target_address].push_back(source_address);
  return true;
}

void Flowgraph::GetNodes(std::vector<address>* nodes) const {
  for (const auto& entry : out_edges_) {
    nodes->push_back(entry.first);
  }
}

void Flowgraph::WriteDot(const std::string& output_file) const {
  std::ofstream dotfile;
  dotfile.open(output_file);
  dotfile << "digraph G {\n";
  for (const auto& edges : out_edges_) {
    address source = edges.first;
    for (const address& target : edges.second) {
      dotfile << "\tblk_" << std::hex << source << " -> blk_" << target << ";\n";
    }
  }
  dotfile << "}\n";
}

void Flowgraph::WriteJSON(std::ostream* output, InstructionGetter block_getter) const {
  std::vector<address> nodes;
  json out_graph = {
    {"name", "CFG"},
    {"nodes", json::array()},
    {"edges", json::array()}};

  GetNodes(&nodes);
  for (const auto& block_address : nodes) {
    json node = {
      { "address", static_cast<uint64_t>(block_address) },
      { "instructions", json::array() } };

    std::vector<Instruction> instructions;
    block_getter(block_address, &instructions);
    for (const Instruction& instruction : instructions) {
      node["instructions"].push_back(
        {{"mnemonic", instruction.GetMnemonic()},
        {"operands", instruction.GetOperands() }});
    }
    out_graph["nodes"].emplace_back(node);
  }

  // Add the relevant edges.
  for (const auto& edge : out_edges_) {
    for (address destination : edge.second) {
      out_graph["edges"].push_back(
        {{ "source", edge.first}, { "destination", destination }});
    }
  }

  (*output) << out_graph;
}

void Flowgraph::WriteJSON(const std::string& output_file,
  InstructionGetter block_getter) const {
  std::ofstream jsonfile;
  jsonfile.open(output_file);
  WriteJSON(&jsonfile, block_getter);
}

bool Flowgraph::HasNode(address node) {
  return (out_edges_.find(node) != out_edges_.end());
}

void Flowgraph::GetTopologicalOrder(std::map<address, std::vector<address>>*
  edges, address startnode, std::map<address, int32_t>* order) {
  // Begin by calculating the topological order of each node.
  std::queue<std::pair<address, uint32_t>> worklist;

  worklist.push(std::make_pair(startnode, 0));
  (*order)[startnode] = 0;

  address current_node;
  uint32_t current_order;
  while (!worklist.empty()) {
    std::tie(current_node, current_order) = worklist.front();
    worklist.pop();
    std::vector<address>& targets = (*edges)[current_node];
    for (const address& target : targets) {
      if (order->find(target) != order->end()) {
        uint32_t target_order = (*order)[target];
        if (target_order <= current_order + 1) {
          continue;
        }
      }
      // Found a shorter path to this node, reprocess it.
      worklist.push(std::make_pair(target, current_order + 1));
      (*order)[target] = current_order + 1;
    }
  }
  for (const auto& entry : (*edges)) {
    if (order->find(entry.first) == order->end()) {
      (*order)[entry.first] = -1;
    }
  }
}

// The hash is calculated as follows:
//  - Each edge in the graph is associated with a tuple of numbers:
//    - Topological order forward of source
//    - Topological order backward of source
//    - Topological order bidirectional of source
//    - indegree of source
//    - outdegree of source
//    - Same for target of the edge
// The result is a set of ten-tuples. Each element in the tuple is multiplied
// with a constant and added into the hash value, which is then rotated.
//
uint64_t Flowgraph::CalculateHash(address startnode,
  uint64_t k0, uint64_t k1, uint64_t k2) {
  std::map<address, int32_t> order_forward, order_backward, order_both;
  GetTopologicalOrder(&out_edges_, startnode, &order_forward);
  GetTopologicalOrder(&in_edges_, startnode, &order_backward);
  GetTopologicalOrder(&bidirectional_edges_, startnode, &order_both);

  std::map<address, uint32_t> indegrees, outdegrees;
  for (const auto& element : out_edges_) {
    outdegrees[element.first] = element.second.size();
    for (const address target : element.second) {
      indegrees[target]++;
    }
  }

  // Arbitrarily chosen seed value;
  uint64_t hash_result = 0x0BADDEED600DDEEDL;
  for (const auto& element : out_edges_) {
    address source = element.first;
    uint64_t per_edge_hash = 0x600DDEED0BADDEEDL;
    for (const address target : element.second) {
      per_edge_hash += k0 * order_forward[source];
      per_edge_hash = rotl64( per_edge_hash, 7 );
      per_edge_hash += k1 * order_backward[source];
      per_edge_hash = rotl64( per_edge_hash, 7 );
      per_edge_hash += k2 * order_both[source];
      per_edge_hash = rotl64( per_edge_hash, 7 );
      per_edge_hash += k0 * indegrees[source];
      per_edge_hash = rotl64( per_edge_hash, 7 );
      per_edge_hash += k1 * outdegrees[source];
      per_edge_hash = rotl64( per_edge_hash, 7 );

      per_edge_hash += k2 * order_forward[target];
      per_edge_hash = rotl64( per_edge_hash, 7 );
      per_edge_hash += k0 * order_backward[target];
      per_edge_hash = rotl64( per_edge_hash, 7 );
      per_edge_hash += k1 * order_both[target];
      per_edge_hash = rotl64( per_edge_hash, 7 );
      per_edge_hash += k2 * indegrees[target];
      per_edge_hash = rotl64( per_edge_hash, 7 );
      per_edge_hash += k0 * outdegrees[target];
      per_edge_hash = rotl64( per_edge_hash, 7 );
    }
    // Combine the per-edge-hashes with a commutative operation.
    hash_result += per_edge_hash;
  }
  return hash_result;
}

Flowgraph* Flowgraph::GetSubgraph(address node, uint32_t distance, uint32_t
  max_size = 0xFFFFFFFF) {
  Flowgraph* subgraph = new Flowgraph();

  // This code proceeds in two iterations: It first identifies all nodes within
  // the specified distance and adds them o the new graph. It then performs a
  // iteration where it adds all the edges that fall within the subgraph.

  // FIFO queue so we get BFS iteration.
  std::queue<std::pair<address, uint32_t>> worklist;
  std::set<address> visited;

  worklist.push(std::make_pair(node, 0));
  visited.insert(node);
  subgraph->AddNode(node);

  address current_node;
  uint32_t current_distance;
  while (!worklist.empty()) {
    std::tie(current_node, current_distance) = worklist.front();
    worklist.pop();

    if (current_distance < distance) {
      std::vector<address>* in = &(in_edges_[current_node]);
      std::vector<address>* out = &(out_edges_[current_node]);

      std::vector<std::vector<address>*> in_and_out = { in, out };

      for (std::vector<address>* edges : in_and_out) {
        for (const address& target : *edges) {
          if (visited.find(target) == visited.end()) {
            visited.insert(target);
            subgraph->AddNode(target);
            worklist.push(std::make_pair(target, current_distance + 1));
            if (subgraph->GetSize() > max_size) {
              delete subgraph;
              return nullptr;
            }
          }
        }
      }
    }
  }
  // We should have all nodes within the right distance.
  std::vector<address> nodes;
  subgraph->GetNodes(&nodes);
  for (const address& node : nodes) {
    // Get the outgoing edges for this node.
    for (const address& target : *(GetOutEdges(node))) {
      if (subgraph->HasNode(target)) {
        subgraph->AddEdge(node, target);
      }
    }
  }
  return subgraph;
}

const std::vector<address>* Flowgraph::GetOutEdges(address node) {
  auto iter = out_edges_.find(node);
  if (iter != out_edges_.end()) {
    return &(iter->second);
  }
  return nullptr;
}

const std::vector<address>* Flowgraph::GetInEdges(address node) {
  auto iter = in_edges_.find(node);
  if (iter != in_edges_.end()) {
    return &(iter->second);
  }
  return nullptr;
}

// Returns the number of nodes that have more than one successor in the graph.
// Required to measure graph complexity to decide what to index.
uint64_t Flowgraph::GetNumberOfBranchingNodes() const {
  uint64_t result = 0;
  for (const auto& edge : out_edges_) {
    if (edge.second.size() > 1) {
      ++result;
    }
  }
  return result;
}

std::string Instruction::AsString() const {
  std::ostringstream result;
  result << mnemonic_ << " ";
  for (uint32_t index = 0; index < operands_.size(); ++index) {
    result << operands_[index];
    if (index != operands_.size() -1) {
      result << ", ";
    }
  }
  return result.str();
}
