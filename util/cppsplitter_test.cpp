#include "gtest/gtest.h"
#include "util/cppsplitter.hpp"

TEST(cppsplitter, constant) {
  std::vector<std::string> stay_constant = {
    "Array<unsigned char>::Addr(unsigned long)",
    "StringList::AddString(wchar_t const*)",
    "Array<unsigned char>::Add(unsigned long)"
  };

  for (const auto& example : stay_constant) {
    std::deque<std::string> result;
    CppSplitter(example, result);
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], example);
  }
}

TEST(cppsplitter, return_value) {
  std::string example =
    "public HRESULT GetCurrentAddrExclusionList(const struct _GUID &, void * *)";
  std::deque<std::string> result;
  CppSplitter(example, result);
  EXPECT_EQ(result.size(), 3);
  EXPECT_EQ(result[0], "public");
  EXPECT_EQ(result[1], "HRESULT");
  EXPECT_EQ(result[2], "GetCurrentAddrExclusionList(const struct _GUID &, void * *)");
}

