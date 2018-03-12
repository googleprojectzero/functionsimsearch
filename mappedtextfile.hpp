#ifndef MAPPEDTEXTFILE
#define MAPPEDTEXTFILE

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

// A class to allow convenient zero-copy parsing of large text files.
class MappedTextFile {
public:
  MappedTextFile(const std::string& inputfile, const char separator=' ');
  bool GetToken(const char** begin, const char** end) const;
  std::string GetToken() const;
  bool AdvanceLine();
  bool AdvanceToken();
  bool Reset();
private:
  void updateLineEnd();
  void updateTokenEnd();
  boost::interprocess::file_mapping inputfile_;
  boost::interprocess::mapped_region region_;
  const char separator_;
  const char* data_buffer_;
  const char* data_end_;
  const char* current_line_start_;
  const char* current_line_end_;
  const char* current_token_;
  const char* current_token_end_;
};

#endif // MAPPEDTEXTFILE


