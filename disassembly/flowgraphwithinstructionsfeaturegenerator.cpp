#include <map>
#include <string>
#include <vector>

#include "disassembly/flowgraph.hpp"
#include "disassembly/flowgraphwithinstructions.hpp"
#include "disassembly/extractimmediate.hpp"

#include "disassembly/flowgraphwithinstructionsfeaturegenerator.hpp"

FlowgraphWithInstructionsFeatureGenerator::FlowgraphWithInstructionsFeatureGenerator(
  const FlowgraphWithInstructions& flowgraph) :
  flowgraph_(new FlowgraphWithInstructions(flowgraph)) {
  init();
}

// Takes ownership of the passed-in unique_ptr<FlowgraphWithInstructions>.
FlowgraphWithInstructionsFeatureGenerator::FlowgraphWithInstructionsFeatureGenerator(
  std::unique_ptr<FlowgraphWithInstructions> flowgraph) :
    flowgraph_(std::move(flowgraph)) {
  init();
}

void FlowgraphWithInstructionsFeatureGenerator::reinit() {
  // Fill the internal queues with the features again.
  init();
}

void FlowgraphWithInstructionsFeatureGenerator::init() {
  std::vector<address> nodes;
  flowgraph_->GetNodes(&nodes);
  for (const auto& node : nodes) {
    nodes_and_distance_.push(std::make_pair(node, 1));
  }
  for (const auto& node : nodes) {
    nodes_and_distance_.push(std::make_pair(node, 2));
  }
  for (const auto& node : nodes) {
    nodes_and_distance_.push(std::make_pair(node, 3));
  }

  BuildMnemonicNgrams();
  FindImmediateValues();
}

void FlowgraphWithInstructionsFeatureGenerator::FindImmediateValues() {
  // Run through all the instructions again.
  for (const auto& pair : flowgraph_->GetInstructions()) {
    // Some disassemblers give us basic blocks with no instructions (DynInst).
    if (pair.second.size() == 0) {
      continue;
    }
    // Always skip the last instruction in a basic block, as it is a branch
    // and hence does not contain a useful operand.
    for (uint64_t index = 0; index < pair.second.size()-1; ++index) {
      const Instruction& instruction = pair.second[index];
      for (const std::string& operand : instruction.GetOperands()) {
        std::vector<uint64_t> immediates;
        ExtractImmediateFromString(operand, &immediates);
        for (uint64_t immediate : immediates) {
          // Only consider immediates as useful that are either greater than
          // 0x4000 or (not divisible by 4 and greater 10). This should remove
          // most stack offsets.
          //
          // These are precisely the heuristics that should be removed by the
          // machine-learning step, but since the baseline is supposed to work
          // reasonably well even without the learning step, we need such stuff
          // here.
          //
          // Also removes data structure offsets, though.
          if ((abs(static_cast<int64_t>(immediate)) > 0x4000) ||
            ((immediate % 4) && (immediate > 10))) {
            immediates_.push(immediate);
          }
        }
      }
    }
  }
}

void FlowgraphWithInstructionsFeatureGenerator::BuildMnemonicNgrams() {
  std::vector<std::string> sequence;

  // The map returned from GetInstructions is already sorted by address, so
  // the instructions will be sorted by address, too.
  for (const auto& pair : flowgraph_->GetInstructions()) {
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
  return std::make_pair(flowgraph_->GetSubgraph(node, distance, 30), node);
}

bool FlowgraphWithInstructionsFeatureGenerator::HasMoreMnemonics() const {
  return !mnem_tuples_.empty();
}

MnemTuple FlowgraphWithInstructionsFeatureGenerator::GetNextMnemTuple() {
  MnemTuple tuple = mnem_tuples_.front();
  mnem_tuples_.pop();
  return tuple;
}

bool FlowgraphWithInstructionsFeatureGenerator::HasMoreImmediates() const {
  return !immediates_.empty();
}

uint64_t FlowgraphWithInstructionsFeatureGenerator::GetNextImmediate() {
  uint64_t val = immediates_.front();
  immediates_.pop();
  return val;
}
