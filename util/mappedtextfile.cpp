// Copyright 2017 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "util/buffertokeniterator.hpp"
#include "util/mappedtextfile.hpp"

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
