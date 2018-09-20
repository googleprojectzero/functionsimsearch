#ifndef FLOWGRAPHWITHINSTRUCTIONS_HPP
#define FLOWGRAPHWITHINSTRUCTIONS_HPP

#include <map>
#include <queue>
#include <vector>

#include "disassembly/flowgraph.hpp"
#include "disassembly/functionfeaturegenerator.hpp"
#include "third_party/json/src/json.hpp"

// A class that keeps both the CFG and associated vectors of mnemonics for the
// instructions. This is not used much internally, but gets exposed through the
// external Python bindings to make interactions from other tools easier.
class FlowgraphWithInstructions : public Flowgraph {
private:
  std::map<address, std::vector<Instruction>> instructions_;
  bool ParseNodeJSON(const nlohmann::json& node);
  bool ParseEdgeJSON(const nlohmann::json& edge);
public:
  bool ParseJSON(const nlohmann::json& json_graph);
  bool AddInstructions(address node_address,
    const std::vector<Instruction>& instructions);
  const std::map<address, std::vector<Instruction>>& GetInstructions() const {
    return instructions_; };
  FlowgraphWithInstructions(const FlowgraphWithInstructions& original);\
  FlowgraphWithInstructions();
  std::string GetDisassembly() const;
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

bool FlowgraphWithInstructionsFromJSON(const char* json,
  FlowgraphWithInstructions* graph);

bool FlowgraphWithInstructionsFromJSONFile(const std::string& filename,
  FlowgraphWithInstructions* graph);

#endif // FLOWGRAPHWITHINSTRUCTIONS_HPP
