#ifndef FLOWGRAPHUTIL_DYNINST_HPP
#define FLOWGRAPHUTIL_DYNINST_HPP

#include "CodeObject.h"
#include "InstructionDecoder.h"

#include "disassembly/flowgraphwithinstructions.hpp"
#include "disassembly/flowgraphutil.hpp"
#include "disassembly/flowgraph.hpp"

// Builds a CFG using a Dyninst Function* as input.
uint64_t BuildFlowgraph(Dyninst::ParseAPI::Function* function,
  Flowgraph* graph);

// Return a std::function that retrieves instruction strings from DynInst.
InstructionGetter MakeDyninstInstructionGetter(
  Dyninst::ParseAPI::CodeObject* codeobject);

// Get a single CFG as JSON.
bool GetCFGFromBinaryAsJSON(const std::string& format, const std::string
  &inputfile, uint64_t address, std::string* result);

std::unique_ptr<FlowgraphWithInstructions> GetCFGWithInstructionsFromBinary(
  const std::string& format, const std::string &inputfile,
  uint64_t func_address);

#endif // FLOWGRAPHUTIL_DYNINST_HPP

