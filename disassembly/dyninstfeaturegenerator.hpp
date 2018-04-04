#ifndef DYNINSTFEATUREGENERATOR_HPP
#define DYNINSTFEATUREGENERATOR_HPP

#include <queue>
#include <map>

#include "CodeObject.h"
#include "disassembly/flowgraph.hpp"
#include "disassembly/flowgraphutil.hpp"
#include "disassembly/functionfeaturegenerator.hpp"
#include "InstructionDecoder.h"
#include "util/util.hpp"

class DyninstFeatureGenerator : public FunctionFeatureGenerator {
public:
  DyninstFeatureGenerator(Dyninst::ParseAPI::Function* function);
  // Implementation of the FunctionFeatureGenerator interface.
  bool HasMoreSubgraphs() const;
  std::pair<Flowgraph*, address> GetNextSubgraph();
  bool HasMoreMnemonics() const;
  MnemTuple GetNextMnemTuple();

private:
  void BuildMnemonicNgrams(Dyninst::ParseAPI::Function* function,
    std::vector<MnemTuple>* tuples) const;
  Flowgraph graph_;
  std::queue<std::pair<address, uint32_t>> nodes_and_distance_;
  std::queue<MnemTuple> mnem_tuples_;
};

#endif // DYNINSTFEATUREGENERATOR_HPP
