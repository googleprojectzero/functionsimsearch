#include "gtest/gtest.h"
#include "util/buffertokeniterator.hpp"

TEST(buffertokeniterator, iterate_words) {
  const char buf[] =
    "This is a multi-word string that we wish to iterate over...";
  std::vector<std::string> control = {
    "This", "is", "a", "multi-word", "string", "that", "we", "wish", "to",
    "iterate", "over..." };

  BufferTokenIterator token(buf, buf + strlen(buf), ' ');
  int index = 0;
  while (token.HasMore()) {
    const char* start_token;
    const char* end_token;
    token.Get(&start_token, &end_token);

    std::string result(start_token, end_token);
    EXPECT_EQ(result, control[index]);
    ++index;
    ++token;
  }
}


TEST(buffertokeniterator, iterate_lines_and_words) {
  const char buf[] =
    "This is a multi-word string that we wish to iterate over...\n"
    "in a data set that consists of multiple lines\n"
    "that we also expect to iterate over.";

  BufferTokenIterator lines(buf, buf + strlen(buf), '\n');
  int line_index = 0;
  int word_index = 0;
  while (lines.HasMore()) {
    const char* start_line, *end_line;
    lines.Get(&start_line, &end_line);
    BufferTokenIterator words(start_line, end_line, ' ');
    while (words.HasMore()) {
      const char* start_word, *end_word;
      words.Get(&start_word, &end_word);
      std::string word(start_word, end_word);
      ++words;
      ++word_index;
    }
    ++line_index;
    ++lines;
  }
  EXPECT_EQ(line_index, 3);
  EXPECT_EQ(word_index, 27);
}

