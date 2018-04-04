#include "util/buffertokeniterator.hpp"
#include <cstring>

BufferTokenIterator::BufferTokenIterator( const char* start,
  const char* end, const char separator ) : start_(start), end_(end), 
  separator_(separator), current_(start) {
}

BufferTokenIterator& BufferTokenIterator::operator++() {
  // Advance current_ to the next token.
  const char* next_separator =
    static_cast<const char*>( memchr( static_cast<const void*>(current_), separator_,
      end_ - current_));
  if (next_separator == nullptr) {
    current_ = end_;
    return *this;
  }
  current_ = next_separator + 1;
  return *this;
}

BufferTokenIterator& BufferTokenIterator::operator--() {
  const char* next_separator =
    static_cast<const char*>( memrchr( static_cast<const void*>(current_), separator_,
      current_ - start_));
  if (next_separator == nullptr) {
    current_ = start_;
    return *this;
  }
  current_ = next_separator + 1;
  return *this;
}

void BufferTokenIterator::Get(const char** tokenstart, const char** tokenend)
  const {
  *tokenstart = current_;
  const char* next_separator =
    static_cast<const char*>( memchr( static_cast<const void*>(current_), separator_,
      end_ - current_));

  if (next_separator == nullptr) {
    *tokenend = end_;
    return;
  }
  *tokenend = next_separator;
  return;
}

std::string BufferTokenIterator::Get() const {
  const char* begin, *end;
  Get(&begin, &end);
  std::string result(begin, end);
  return result;
}
