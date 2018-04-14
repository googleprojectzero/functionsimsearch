#include <map>
#include <string>
#include <vector>

#include "disassembly/flowgraph.hpp"
#include "pybindings/flowgraphwithinstructions.hpp"

bool FlowgraphWithInstructions::AddInstructions(address node_address,
  const std::vector<Instruction>& instructions) {
  instructions_[node_address] = instructions;
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
