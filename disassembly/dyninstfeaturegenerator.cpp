#include "disassembly/flowgraphutil_dyninst.hpp"
#include "disassembly/dyninstfeaturegenerator.hpp"

DyninstFeatureGenerator::DyninstFeatureGenerator(
  Dyninst::ParseAPI::Function* function) {
  BuildFlowgraph(function, &graph_);
  std::vector<address> nodes;
  graph_.GetNodes(&nodes);
  for (const auto& node : nodes) {
    nodes_and_distance_.push(std::make_pair(node, 1));
  }
  for (const auto& node : nodes) {
    nodes_and_distance_.push(std::make_pair(node, 2));
  }
  for (const auto& node : nodes) {
    nodes_and_distance_.push(std::make_pair(node, 3));
  }

  std::vector<MnemTuple> tuples;
  BuildMnemonicNgrams(function, &tuples);
  for (const auto& tuple : tuples) {
    mnem_tuples_.push(tuple);
  }
}

// Given a Dyninst function, create the set of instruction 3-grams.
void DyninstFeatureGenerator::BuildMnemonicNgrams(
  Dyninst::ParseAPI::Function* function,
  std::vector<MnemTuple>* tuples) const {
  std::vector<std::pair<address, std::string>> sequence;
  // Make sure that reading the mnemonics from Dyninst is synchronized.
  // TODO(thomasdullien): Latest DynInst should allow concurrent reading, so
  // the mutex is gone here. If crashes or problems come up, we need to re-
  // introduce it.
  {
    // Iterate over each block and extract the instructions.
    for (const auto& block : function->blocks()) {
      Dyninst::ParseAPI::Block::Insns block_instructions;
      block->getInsns(block_instructions);
      for (const auto& instruction : block_instructions) {
        auto& op = instruction.second->getOperation();
        sequence.push_back(std::make_pair(instruction.first, op.format()));
      }
    }
  }
  // Sort instructions by address;
  std::sort(sequence.begin(), sequence.end());
  // Construct the 3-tuples.
  for (uint64_t index = 0; index + 2 < sequence.size(); ++index) {
    tuples->emplace_back(std::make_tuple(
      sequence[index].second,
      sequence[index+1].second,
      sequence[index+2].second));
  }
}

bool DyninstFeatureGenerator::HasMoreSubgraphs() const {
  return !nodes_and_distance_.empty();
}

std::pair<Flowgraph*, address> DyninstFeatureGenerator::GetNextSubgraph() {
  address node = nodes_and_distance_.front().first;
  uint32_t distance = nodes_and_distance_.front().second;
  nodes_and_distance_.pop();
  return std::make_pair(graph_.GetSubgraph(node, distance, 30), node);
}

bool DyninstFeatureGenerator::HasMoreMnemonics() const {
  return !mnem_tuples_.empty();
}

MnemTuple DyninstFeatureGenerator::GetNextMnemTuple() {
  MnemTuple tuple = mnem_tuples_.front();
  mnem_tuples_.pop();
  return tuple;
}
