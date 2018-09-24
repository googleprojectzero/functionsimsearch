#include "gtest/gtest.h"
#include "util/testutil.hpp"
#include "disassembly/disassembly.hpp"

TEST(disassembly, load_elf_with_dyninst) {
  uint64_t id = 0xf89b73cc72cd02c7ULL;
  const std::string& filename = id_to_filename[id];

  Disassembly dis("ELF", filename);
  dis.Load();
  EXPECT_EQ(dis.GetNumberOfFunctions(), 948);
}

TEST(disassembly, load_pe_with_dyninst) {
  uint64_t id = 0x38ec33c1d2961e79ULL;
  const std::string& filename = id_to_filename[id];

  Disassembly dis("PE", filename);
  dis.Load();
  EXPECT_EQ(dis.GetNumberOfFunctions(), 1139);
}

TEST(disassembly, load_json) {
  Disassembly dis("JSON",
    "../testdata/PE/unrar.5.5.3.builds/VS2015/unrar32/Release/unrar.exe.json");
  dis.Load();
  EXPECT_EQ(dis.GetNumberOfFunctions(), 1139);
}

