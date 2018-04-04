#include "gtest/gtest.h"
#include "util/mappedtextfile.hpp"

TEST(mappedtextfile, count_lines) {
  MappedTextFile mapped("../testdata/training/functions.txt");
  auto lines = mapped.GetLineIterator();
  int line_count = 0;
  while (lines.HasMore()) {
    ++line_count;
    ++lines;
  }
  EXPECT_EQ(line_count, 166);
}

TEST(mappedtextfile, count_words) {
  MappedTextFile mapped("../testdata/training/functions.txt");
  auto lines = mapped.GetLineIterator();
  int word_count = 0;
  while (lines.HasMore()) {
    auto words = mapped.GetWordIterator(lines);
    while (words.HasMore()) {
      const char *wordstart, *wordend;
      words.Get(&wordstart, &wordend);
      std::string word(wordstart, wordend);
      ++words;
      ++word_count;
    }
    ++lines;
  }
  EXPECT_EQ(word_count, 49411);
}


