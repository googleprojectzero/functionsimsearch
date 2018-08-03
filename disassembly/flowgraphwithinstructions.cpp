#include <map>
#include <string>
#include <vector>

#include "disassembly/flowgraph.hpp"
#include "disassembly/flowgraphwithinstructions.hpp"
#include "third_party/json/src/json.hpp"

// Copy constructor. Somewhat hideously expensive.
FlowgraphWithInstructions::FlowgraphWithInstructions(
  const FlowgraphWithInstructions& original) {
  instructions_ = original.instructions_;
}

FlowgraphWithInstructions::FlowgraphWithInstructions() {
}

bool FlowgraphWithInstructions::AddInstructions(address node_address,
  const std::vector<Instruction>& instructions) {
  instructions_[node_address] = instructions;
  return true;
}

bool FlowgraphWithInstructions::ParseNodeJSON(const nlohmann::json& node) {
  if ((node.find("address") == node.end()) ||
    (node.find("instructions") == node.end())) {
    return false;
  }
  uint64_t address = node["address"].get<uint64_t>();
  std::vector<Instruction> instructions;
  for (const auto& instruction : node["instructions"]) {
    if (instruction.find("mnemonic") == instruction.end() ||
      instruction.find("operands") == instruction.end()) {
      return false;
    }
    std::string mnemonic = instruction["mnemonic"].get<std::string>();
    std::vector<std::string> operands;
    for (const auto& operand : instruction["operands"]) {
      operands.push_back(operand.get<std::string>());
    }
    instructions.emplace_back(mnemonic, operands);
  }
  AddNode(address);
  AddInstructions(address, instructions);
  return true;
}

bool FlowgraphWithInstructions::ParseEdgeJSON(const nlohmann::json& edge) {
  if ((edge.find("source") == edge.end()) || 
    (edge.find("destination") == edge.end())) {
    return false;
  }
  uint64_t source = edge["source"].get<uint64_t>();
  uint64_t destination = edge["destination"].get<uint64_t>();
  AddEdge(source, destination);
  return true;
}

bool FlowgraphWithInstructions::ParseJSON(const nlohmann::json& json_graph) {
  if ((json_graph.find("nodes") == json_graph.end()) ||
      (json_graph.find("edges") == json_graph.end())) {
    return false;
  }
  for (const auto& node : json_graph["nodes"]) {
    if (ParseNodeJSON(node) == false) {
      return false;
    }
  }
  for (const auto& edge : json_graph["edges"]) {
    if (ParseEdgeJSON(edge) == false) {
      return false;
    }
  }
  return true;
}


FlowgraphWithInstructionsFeatureGenerator::FlowgraphWithInstructionsFeatureGenerator(
  const FlowgraphWithInstructions& flowgraph) : flowgraph_(flowgraph) {
  std::vector<address> nodes;
  flowgraph_.GetNodes(&nodes);
  for (const auto& node : nodes) {
    nodes_and_distance_.push(std::make_pair(node, 2));
  }
  for (const auto& node : nodes) {
    nodes_and_distance_.push(std::make_pair(node, 3));
  }

  std::vector<MnemTuple> tuples;
  BuildMnemonicNgrams();
  for (const auto& tuple : tuples) {
    mnem_tuples_.push(tuple);
  }
}

void FlowgraphWithInstructionsFeatureGenerator::BuildMnemonicNgrams() {
  std::vector<std::string> sequence;
  for (const auto& pair : flowgraph_.GetInstructions()) {
    for (const Instruction& instruction : pair.second) {
      sequence.push_back(instruction.GetMnemonic());
    }
  }

  // Construct the 3-tuples.
  for (uint64_t index = 0; index + 2 < sequence.size(); ++index) {
    mnem_tuples_.push(std::make_tuple(
      sequence[index],
      sequence[index+1],
      sequence[index+2]));
  }
}

bool FlowgraphWithInstructionsFeatureGenerator::HasMoreSubgraphs() const {
  return !nodes_and_distance_.empty();
}

std::pair<Flowgraph*, address> FlowgraphWithInstructionsFeatureGenerator
  ::GetNextSubgraph() {
  address node = nodes_and_distance_.front().first;
  uint32_t distance = nodes_and_distance_.front().second;
  nodes_and_distance_.pop();
  return std::make_pair(flowgraph_.GetSubgraph(node, distance, 30), node);
}

bool FlowgraphWithInstructionsFeatureGenerator::HasMoreMnemonics() const {
  return !mnem_tuples_.empty();
}

MnemTuple FlowgraphWithInstructionsFeatureGenerator::GetNextMnemTuple() {
  MnemTuple tuple = mnem_tuples_.front();
  mnem_tuples_.pop();
  return tuple;
}

bool FlowgraphWithInstructionsFromJSON(const char* json,
  FlowgraphWithInstructions* graph) {
  auto json_graph = nlohmann::json::parse(json);

  if ((json_graph.find("nodes") == json_graph.end()) ||
      (json_graph.find("edges") == json_graph.end())) {
    return false;
  }

  return graph->ParseJSON(json_graph);
}
