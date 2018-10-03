#ifndef FLOWGRAPHWITHINSTRUCTIONSFEATUREGENERATOR_HPP
#define FLOWGRAPHWITHINSTRUCTIONSFEATUREGENERATOR_HPP

#include <map>
#include <queue>
#include <vector>

#include "disassembly/flowgraph.hpp"
#include "disassembly/functionfeaturegenerator.hpp"

class FlowgraphWithInstructionsFeatureGenerator :
  public FunctionFeatureGenerator {
public:
  // Careful: This constructor takes ownership of the provided pointer.
  FlowgraphWithInstructionsFeatureGenerator(
    std::unique_ptr<FlowgraphWithInstructions> flowgraph);
  FlowgraphWithInstructionsFeatureGenerator(const FlowgraphWithInstructions&
    flowgraph); 
  bool HasMoreSubgraphs() const;
  std::pair<Flowgraph*, address> GetNextSubgraph();
  bool HasMoreMnemonics() const;
  MnemTuple GetNextMnemTuple();
private:
  void init();
  void BuildMnemonicNgrams();
  std::unique_ptr<FlowgraphWithInstructions> flowgraph_;
  std::queue<std::pair<address, uint32_t>> nodes_and_distance_;
  std::queue<MnemTuple> mnem_tuples_;
  std::queue<uint64_t> immediates_;
};

#endif // FLOWGRAPHWITHINSTRUCTIONSFEATUREGENERATOR_HPP
