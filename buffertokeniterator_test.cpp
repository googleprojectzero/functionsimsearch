#include "gtest/gtest.h"
#include "buffertokeniterator.hpp"

TEST(buffertokeniterator, iterate_words) {
  const char buf[] = 
    "This is a multi-word string that we wish to iterate over...";
  BufferTokenIterator token(buf, buf + strlen(buf), ' ');

  while (token.HasMore()) {
    const char* start_token;
    const char* end_token;
    token.Get(&start_token, &end_token);

    std::string result(start_token, end_token);
    printf("'%s'\n", result.c_str());
    ++token;
  }
}

