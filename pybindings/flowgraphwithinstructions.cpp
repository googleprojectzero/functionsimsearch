
bool FlowgraphWithInstructions::AddInstructions(address node_address,
  const std::vector<std::string>& mnemonics) {
  mnemonics_[nodes_address] = mnemonics;
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





};

FLowgraphWithInstructionsFeatureGenerator::BuildMnemonicNgrams() {



}
