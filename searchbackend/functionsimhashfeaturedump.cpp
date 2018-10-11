#include <iostream>
#include <fstream>

#include "disassembly/flowgraph.hpp"
#include "disassembly/functionfeaturegenerator.hpp"
#include "searchbackend/functionsimhashfeaturedump.hpp"

// Writes a DOT file for a given graphlet.
void WriteFeatureDictionaryEntry(uint64_t hashA, uint64_t hashB,
  const Flowgraph& graphlet) {
  char buf[256];
  sprintf(buf, "/var/tmp/%16.16lx%16.16lx.dot", hashA, hashB);
  graphlet.WriteDot(std::string(buf));
}

// Writes a JSON file for a given MnemTuple.
void WriteFeatureDictionaryEntry(uint64_t hashA, uint64_t hashB,
  const MnemTuple& tuple) {
  char buf[256];
  sprintf(buf, "/var/tmp/%16.16lx%16.16lx.json", hashA, hashB);
  std::ofstream jsonfile;
  jsonfile.open(std::string(buf));
  jsonfile << "[ " << std::get<0>(tuple) << ", " << std::get<1>(tuple) << ", "
    << std::get<2>(tuple) << " ]" << std::endl;
}

// Writes an immediate that was encountered.
void WriteFeatureDictionaryEntry(uint64_t hashA, uint64_t hashB, uint64_t
  immediate) {
  std::ofstream immediates;
  immediates.open("/var/tmp/immediates.txt",
    std::ofstream::out | std::ofstream::app);
  immediates << std::hex << hashA << " " << hashB << " " << immediate
    << std::endl;
}
