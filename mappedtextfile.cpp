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

#include "mappedtextfile.hpp"

MappedTextFile::MappedTextFile(const std::string& inputfile,
  const char separator) :
  separator_(separator),
  inputfile_(inputfile.c_str(), boost::interprocess::read_only),
  region_(inputfile_, boost::interprocess::read_only),
  data_buffer_(static_cast<const char*>(region_.get_address())),
  data_end_(static_cast<const char*>(region_.get_address()) + region_.get_size()),
  current_line_start_(data_buffer_),
  current_line_end_(nullptr), current_token_(data_buffer_),
  current_token_end_(nullptr) {
  updateLineEnd();
  updateTokenEnd();
}

void MappedTextFile::updateLineEnd() {
  const char* next_newline = static_cast<const char*>(
    memchr( current_line_start_, '\n', data_end_ - current_line_start_ ));
  if (next_newline == nullptr) {
    // No more newlines in this file, the end of the current line is hence
    // data_end_.
    current_line_end_ = data_end_;
  } else {
    current_line_end_ = next_newline;
  }
}

void MappedTextFile::updateTokenEnd() {
  const char* next_separator = static_cast<const char*>(
    memchr(current_token_, separator_, current_line_end_ - current_token_));
  if (next_separator == nullptr) {
    // No more tokens in this line.
    current_token_end_ = current_line_end_;
  } else {
    current_token_end_ = next_separator;
  }
}

bool MappedTextFile::GetToken(const char** begin, const char** end) const {
  *begin = current_token_;
  *end = current_token_end_;
  return true;
}

// Advances the cursor until a new line is encountered.
bool MappedTextFile::AdvanceLine() {
  if (current_line_end_ != data_end_) {
    current_line_start_ = current_line_end_ + 1;
    updateLineEnd();
    current_token_ = current_line_start_;
    updateTokenEnd();
    return true;
  } else {
    return false;
  }
}

bool MappedTextFile::AdvanceToken() {
  if (current_token_end_ != current_line_end_) {
    current_token_ = current_token_end_ + 1;
    while (isspace(*current_token_) && (current_token_ != current_line_end_)) {
      current_token_++;
    }
    updateTokenEnd();
    return (current_token_end_ != current_token_);
  } else {
    return false;
  }
}

bool MappedTextFile::Reset() {
  data_buffer_ = static_cast<const char*>(region_.get_address());
  data_end_ =
    static_cast<const char*>(region_.get_address()) + region_.get_size();
  current_line_start_ = data_buffer_;
  current_token_ = data_buffer_;
  updateLineEnd();
  updateTokenEnd();
}

std::string MappedTextFile::GetToken() const {
  return std::string(current_token_, current_token_end_ - current_token_);
}
