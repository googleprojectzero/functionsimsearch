#include <map>
#include <fstream>
#include <string>
#include <vector>

#include "disassembly/flowgraph.hpp"
#include "disassembly/flowgraphwithinstructions.hpp"
#include "third_party/json/src/json.hpp"

// Copy constructor. Somewhat hideously expensive.
FlowgraphWithInstructions::FlowgraphWithInstructions(
  const FlowgraphWithInstructions& original) : Flowgraph(original) {
  instructions_ = original.instructions_;
}

FlowgraphWithInstructions::FlowgraphWithInstructions() {
}

bool FlowgraphWithInstructions::AddInstructions(address node_address,
  const std::vector<Instruction>& instructions) {
  instructions_[node_address] = instructions;
  return true;
}

bool FlowgraphWithInstructions::ParseNodeJSON(const nlohmann::json& node) {
  if ((node.find("address") == node.end()) ||
    (node.find("instructions") == node.end())) {
    return false;
  }
  uint64_t address = node["address"].get<uint64_t>();
  std::vector<Instruction> instructions;
  for (const auto& instruction : node["instructions"]) {
    if (instruction.find("mnemonic") == instruction.end() ||
      instruction.find("operands") == instruction.end()) {
      return false;
    }
    std::string mnemonic = instruction["mnemonic"].get<std::string>();
    std::vector<std::string> operands;
    for (const auto& operand : instruction["operands"]) {
      operands.push_back(operand.get<std::string>());
    }
    instructions.emplace_back(mnemonic, operands);
  }
  AddNode(address);
  AddInstructions(address, instructions);
  return true;
}

bool FlowgraphWithInstructions::ParseEdgeJSON(const nlohmann::json& edge) {
  if ((edge.find("source") == edge.end()) ||
    (edge.find("destination") == edge.end())) {
    return false;
  }
  uint64_t source = edge["source"].get<uint64_t>();
  uint64_t destination = edge["destination"].get<uint64_t>();
  AddEdge(source, destination);
  return true;
}

bool FlowgraphWithInstructions::ParseJSON(const nlohmann::json& json_graph) {
  if ((json_graph.find("nodes") == json_graph.end()) ||
      (json_graph.find("edges") == json_graph.end())) {
    return false;
  }
  for (const auto& node : json_graph["nodes"]) {
    if (ParseNodeJSON(node) == false) {
      return false;
    }
  }
  for (const auto& edge : json_graph["edges"]) {
    if (ParseEdgeJSON(edge) == false) {
      return false;
    }
  }
  return true;
}

bool FlowgraphWithInstructionsFromJSON(const char* json,
  FlowgraphWithInstructions* graph) {
  auto json_graph = nlohmann::json::parse(json);

  if ((json_graph.find("nodes") == json_graph.end()) ||
      (json_graph.find("edges") == json_graph.end())) {
    return false;
  }

  return graph->ParseJSON(json_graph);
}

bool FlowgraphWithInstructionsFromJSONFile(const std::string& filename,
  FlowgraphWithInstructions* graph) {
  std::ifstream t(filename);
  std::string str((std::istreambuf_iterator<char>(t)),
    std::istreambuf_iterator<char>());
  return FlowgraphWithInstructionsFromJSON(str.c_str(), graph);
}

std::string FlowgraphWithInstructions::GetDisassembly() const {
  std::stringstream output;
  // TODO(thomasdullien): The code takes the lowest address in a function as the
  // beginning address here. There has to be a better way?
  output << "\n[!] Function at " << std::hex << instructions_.begin()->first;

  for (const auto& block : instructions_) {
    output << "\t\tBlock at " << std::hex << block.first;
    output << " (" << std::dec << static_cast<size_t>(block.second.size()) 
      << ")\n";
    for (const auto& instruction : block.second) {
      output << "\t\t\t " << instruction.AsString() << "\n";
    }
  }
  return output.str();
}
