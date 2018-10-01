#include <memory>
#include "gtest/gtest.h"
#include "third_party/json/src/json.hpp"

#include "disassembly/flowgraphutil_dyninst.hpp"
#include "searchbackend/functionsimhash.hpp"
#include "disassembly/flowgraphwithinstructions.hpp"
#include "disassembly/flowgraphwithinstructionsfeaturegenerator.hpp"

TEST(flowgraphwithinstructions, creategraph) {
  FlowgraphWithInstructions fg;

  // Get function 0x806C811 from ./testdata/ELF/unrar.5.5.3.builds/unrar.x86.Os
  std::unique_ptr<FlowgraphWithInstructions> fg_dyninst(
    GetCFGWithInstructionsFromBinary(
        "ELF", "../testdata/ELF/unrar.5.5.3.builds/unrar.x86.Os.ELF",
        0x806C811));

  // Expect that the loading of the disassembly worked.
  EXPECT_FALSE(fg_dyninst == nullptr);

  // The graph nodes:
  std::vector<std::pair<address, std::vector<Instruction>>> nodes = {
    { 0x806C811, {
                   { "sub", {} },
                   { "lea", {} },
                   { "push", {} },
                   { "call", {}}}},
    { 0x806C820, {
                   { "add", {} },
                   { "jmp", {} }}},
    { 0x806C825, { { "mov", {} }}},
    { 0x806C827, {
                   { "sub", {} },
                   { "lea", {} },
                   { "push", {} },
                   { "call", {} }}},
    { 0x806c836, {
                   { "mov", {} },
                   { "call", {} }}}};

  // The graph edges:
  std::vector<std::pair<address, address>> edges = {
    { 0x806C811, 0x806C820 },
    { 0x806c820, 0x806c825 },
    { 0x806c820, 0x806c827 },
    { 0x806c825, 0x806c827 },
    { 0x806c827, 0x806c836 }};

  for (const auto& n : nodes) {
    fg.AddNode(n.first);
    fg.AddInstructions(n.first, n.second);
  }
  for (const auto& e : edges) {
    fg.AddEdge(e.first, e.second);
  }

  // The Dyninst output for the simhash for the function is
  // 9c5b458691685f5e76a3474775cba64e.

  // Check fg.
  {
    FunctionSimHasher sim_hasher("", false, false, false, false);
    std::vector<FeatureHash> feature_hashes;
    std::vector<uint64_t> hashes;
    FlowgraphWithInstructionsFeatureGenerator generator(fg);

    sim_hasher.CalculateFunctionSimHash(&generator, 128, &hashes,
      &feature_hashes);
    uint64_t hash1 = hashes[0];
    uint64_t hash2 = hashes[1];
    EXPECT_EQ(hash1, 0xACEB07449170DFCF);
    EXPECT_EQ(hash2, 0x56df46c771e9a4df);
  }
  // Check fg_dyninst
  {
    FunctionSimHasher sim_hasher("", false, false, false, false);
    std::vector<FeatureHash> feature_hashes;
    std::vector<uint64_t> hashes;
    FlowgraphWithInstructionsFeatureGenerator generator(*fg_dyninst);

    sim_hasher.CalculateFunctionSimHash(&generator, 128, &hashes,
      &feature_hashes);
    uint64_t hash1 = hashes[0];
    uint64_t hash2 = hashes[1];

    EXPECT_EQ(hash1, 0x9c5b458691685f5e);
    EXPECT_EQ(hash2, 0x76a3474775cba64e); 
  }
}

TEST(flowgraphwithinstructions, parsejson) {
  const char* json_string =
  R"json({"edges":[{"destination":1518838580,"source":1518838565},{"destination":1518838572,"source":1518838565},{"destination":1518838578,"source":1518838572},{"destination":1518838574,"source":1518838572},{"destination":1518838580,"source":1518838574},{"destination":1518838578,"source":1518838574},{"destination":1518838580,"source":1518838578}],"name":"CFG","nodes":[{"address":1518838565,"instructions":[{"mnemonic":"xor","operands":["EAX","EAX"]},{"mnemonic":"cmp","operands":["[ECX + 4]","EAX"]},{"mnemonic":"jnle","operands":["5a87a334"]}]},{"address":1518838572,"instructions":[{"mnemonic":"jl","operands":["5a87a332"]}]},{"address":1518838574,"instructions":[{"mnemonic":"cmp","operands":["[ECX]","EAX"]},{"mnemonic":"jnb","operands":["5a87a334"]}]},{"address":1518838578,"instructions":[{"mnemonic":"mov","operands":["AL","1"]}]},{"address":1518838580,"instructions":[{"mnemonic":"ret near","operands":["[ESP]"]}]}]})json";

  std::string expected_disassembly =
    "\n[!] Function at 5a87a325\t\tBlock at 5a87a325 (3)\n"
    "\t\t\t xor EAX, EAX\n"
    "\t\t\t cmp [ECX + 4], EAX\n"
    "\t\t\t jnle 5a87a334\n"
    "\t\tBlock at 5a87a32c (1)\n"
    "\t\t\t jl 5a87a332\n"
    "\t\tBlock at 5a87a32e (2)\n"
    "\t\t\t cmp [ECX], EAX\n"
    "\t\t\t jnb 5a87a334\n"
    "\t\tBlock at 5a87a332 (1)\n"
    "\t\t\t mov AL, 1\n"
    "\t\tBlock at 5a87a334 (1)\n"
    "\t\t\t ret near [ESP]\n";

  std::unique_ptr<FlowgraphWithInstructions> ptr(new FlowgraphWithInstructions());
  FlowgraphWithInstructions* temp = ptr.get();
  EXPECT_TRUE(FlowgraphWithInstructionsFromJSON(json_string, temp));

  EXPECT_TRUE(ptr->HasNode(1518838565));
  EXPECT_TRUE(ptr->HasNode(1518838572));

  std::string disasm = temp->GetDisassembly();
  EXPECT_EQ(disasm, expected_disassembly);
}
