#include "buffertokeniterator.hpp"
#include "mappedtextfile.hpp"

MappedTextFile::MappedTextFile(const std::string& inputfile) :
  inputfile_(inputfile.c_str(), boost::interprocess::read_only),
  region_(inputfile_, boost::interprocess::read_only),
  data_buffer_(static_cast<const char*>(region_.get_address())),
  data_end_(static_cast<const char*>(region_.get_address()) + region_.get_size())
{}

BufferTokenIterator MappedTextFile::GetLineIterator() const {
  return BufferTokenIterator(data_buffer_, data_end_, '\n');
}

BufferTokenIterator MappedTextFile::GetWordIterator(const BufferTokenIterator&
  line) const {
  const char* line_begin, *line_end;
  line.Get(&line_begin, &line_end);
  return BufferTokenIterator(line_begin, line_end, ' ');
}

