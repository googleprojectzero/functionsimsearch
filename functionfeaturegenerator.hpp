#ifndef FUNCTIONFEATUREGENERATOR_HPP
#define FUNCTIONFEATUREGENERATOR_HPP

typedef std::tuple<std::string, std::string, std::string> MnemTuple;

// In order to calculate a per-function SimHash, we need something that provides
// graphlets and mnemonic-3-grams to the calculation. This file defines a simple
// interface to do so. Deriving from this interface for whatever disassembly
// infrastructure you are operating on allows de-coupling the disassembler from
// the hash calculation.

class FunctionFeatureGenerator {
public:
  virtual bool HasMoreSubgraphs() const = 0;
  virtual std::pair<Flowgraph*, address> GetNextSubgraph() = 0;
  virtual bool HasMoreMnemonics() const = 0;
  virtual MnemTuple GetNextMnemTuple() = 0;
};

#endif // FUNCTIONFEATUREGENERATOR_HPP

