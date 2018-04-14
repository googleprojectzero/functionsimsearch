#ifndef FLOWGRAPHWITHINSTRUCTIONS_HPP
#define FLOWGRAPHWITHINSTRUCTIONS_HPP

#include <map>
#include <queue>
#include <vector>

#include "disassembly/flowgraph.hpp"
#include "disassembly/functionfeaturegenerator.hpp"

// A class that keeps both the CFG and associated vectors of mnemonics for the
// instructions. This is not used much internally, but gets exposed through the
// external Python bindings to make interactions from other tools easier.
class FlowgraphWithInstructions : public Flowgraph {
private:
  std::map<address, std::vector<Instruction>> instructions_;
public:
  bool AddInstructions(address node_address,
    const std::vector<Instruction>& instructions);
  const std::map<address, std::vector<Instruction>>& GetInstructions() const {
    return instructions_; };
};

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
};

#endif // FLOWGRAPHWITHINSTRUCTIONS_HPP
