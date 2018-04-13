#ifndef FLOWGRAPHWITHINSTRUCTIONS_HPP
#include "disassembly/flowgraph.hpp"
#include "disassembly/functionfeaturegenerator.hpp"

// A class that keeps both the CFG and associated vectors of mnemonics for the
// instructions. This is not used much internally, but gets exposed through the
// external Python bindings to make interactions from other tools easier.
class FlowgraphWithInstructions : public Flowgraph {
private:
  std::map<address, std::vector<std::string>> mnemonics_;
public:
  bool AddInstructions(address node_address,
    const std::vector<std::string>& mnemonics);
}

class FlowgraphWithInstructionsFeatureGenerator :
  public FunctionFeatureGenerator {
public:
  FlowgraphWithInstructionsFeatureGenerator(const FlowgraphWithInstructions&
    flowgraph);
  bool HasMoreSubgraphs() const;
  std::pair<Flowgraph*, address> GetNextSubgraph();
  bool HasMoreMnemonics() const;
  MnemTuple GetNextMnemTuple();
private:
  void BuildMnemonicNgrams();
  FlowgraphWithInstructions flowgraph_;
  std::queue<std::pair<address, uint32_t>> nodes_and_distance_;
  std::queue<MnemTuple> mnem_tuples_;
}


#define FLOWGRAPHWITHINSTRUCTIONS_HPP
