// A small helper class to assist in iterating over the individual words and
// lines of memory-mapped text files (can be used for CSV files etc.).

#ifndef BUFFERTOKENITERATOR_HPP
#define BUFFERTOKENITERATOR_HPP

#include <string>

class BufferTokenIterator {
public:
  BufferTokenIterator(const char* start, const char* end, const char separator);
  BufferTokenIterator& operator++();
  BufferTokenIterator& operator--();
//  const BufferToken& InnerIterator(const char inner_seperator);
  void Get(const char** tokenstart, const char** tokenend) const;
  std::string Get() const;
  bool HasMore() const { return current_ != end_; };
private:
  const char* start_;
  const char* end_;
  const char separator_;
  const char* current_;
};

#endif // BUFFERTOKENITERATOR_HPP
