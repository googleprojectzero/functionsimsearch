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

#ifndef FLOWGRAPH_HPP
#define FLOWGRAPH_HPP

#include <functional>
#include <map>
#include <vector>

/* A Flowgraph class designed to allow fast extraction of small subgraphs
 A set of target nodes is kept for each node allowing easy retrieval.
 This file is kept intentionally relatively free of external dependencies;
 the goal is that implementing this interface should eventually be enough to
 swap disassembly engines.
*/

typedef uint64_t address;

enum Direction {
  direction_up,
  direction_down,
  direction_bidirectional
};

class Instruction {
public:
  Instruction(const std::string& mnemonic, const std::vector<std::string>&
    operands);
  const std::string& GetMnemonic() const { return mnemonic_; };
  const std::vector<std::string>& GetOperands() const { return operands_; };
  std::string AsString() const;
private:
  std::string mnemonic_;
  std::vector<std::string> operands_;
};

// A callback to get the instructions for a basic block at a given address.
typedef std::function<bool(uint64_t, std::vector<Instruction>*)>
  InstructionGetter;

class Flowgraph {
private:
  // For each node address, store a vector of node addresses of target nodes.
  std::map<address, std::vector<address>> out_edges_;
  std::map<address, std::vector<address>> in_edges_;
  std::map<address, std::vector<address>> bidirectional_edges_;
public:
  Flowgraph();
  Flowgraph(const Flowgraph& other);
  virtual ~Flowgraph() {};
  bool AddNode(address node_adress);
  bool AddEdge(address source_address, address target_address);
  bool HasNode(address node);
  const std::vector<address>* GetOutEdges(address node);
  const std::vector<address>* GetInEdges(address node);

  uint64_t GetSize() const { return out_edges_.size(); };
  uint64_t GetNumberOfBranchingNodes() const;

  Flowgraph* GetSubgraph(address node, uint32_t distance, uint32_t max_size);
  void GetNodes(std::vector<address>* nodes) const;
  // Fills a map with topological distance from the startnode, or -1 for no
  // path found.
  void GetTopologicalOrder(std::map<address, std::vector<address>>* edges,
    address startnode, std::map<address, int32_t>* order);

  uint64_t CalculateHash(address node, uint64_t k0 = 0xc3a5c85c97cb3127ULL,
    uint64_t k1 = 0xb492b66fbe98f273ULL,
    uint64_t k2 = 0x9ae16a3b2f90404fULL);

  void WriteDot(const std::string& output_file) const;
  void WriteJSON(const std::string& output_file,
    InstructionGetter block_getter) const;
  void WriteJSON(std::ostream* output, InstructionGetter block_getter) const;
};

#endif // FLOWGRAPH_HPP
