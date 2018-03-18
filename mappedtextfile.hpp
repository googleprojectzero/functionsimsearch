#ifndef MAPPEDTEXTFILE
#define MAPPEDTEXTFILE

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include "buffertokeniterator.hpp"

// A class to allow convenient zero-copy parsing of large text files.
class MappedTextFile {
public:
  MappedTextFile(const std::string& inputfile);

  BufferTokenIterator GetLineIterator() const;
  BufferTokenIterator GetWordIterator(const BufferTokenIterator& line) const;
  const char* GetStart() const { return data_buffer_; };
  const char* GetEnd() const { return data_end_; };
private:
  boost::interprocess::file_mapping inputfile_;
  boost::interprocess::mapped_region region_;
  const char* data_buffer_;
  const char* data_end_;
};

#endif // MAPPEDTEXTFILE


