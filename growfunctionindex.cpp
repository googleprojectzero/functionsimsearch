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

#include <gflags/gflags.h>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>

DEFINE_string(index, "./similarity.index", "Index file");
DEFINE_uint64(size_to_grow, 500, "Size in MB to grow the index");
// The google namespace is there for compatibility with legacy gflags and will
// be removed eventually.
#ifndef gflags
using namespace google;
#else
using namespace gflags;
#endif

using namespace boost::interprocess;

int main(int argc, char** argv) {
  SetUsageMessage(
    "Grow function search index file by specified number of megabytes.");
  ParseCommandLineFlags(&argc, &argv, true);

  uint64_t grow_size = FLAGS_size_to_grow;
  grow_size *= (1024*1024);
  managed_mapped_file::grow(FLAGS_index.c_str(), grow_size);
}
