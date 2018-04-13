#ifndef FUNCTIONSIMHASHFEATUREDUMP_HPP
#define FUNCTIONSIMHASHFEATUREDUMP_HPP

#include "disassembly/functionfeaturegenerator.hpp"
#include "disassembly/flowgraph.hpp"
#include "searchbackend/functionsimhashfeaturedump.hpp"

// Writes a DOT file for a given graphlet.
void WriteFeatureDictionaryEntry(uint64_t hashA, uint64_t hashB,
  const Flowgraph& graphlet);

// Writes a JSON file for a given MnemTuple.
void WriteFeatureDictionaryEntry(uint64_t hashA, uint64_t hashB, 
  const MnemTuple& tuple);

#endif // FUNCTIONSIMHASHFEATUREDUMP_HPP
