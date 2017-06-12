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

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>

using namespace boost::interprocess;

int main(int argc, char** argv) {
  if (argc != 3) {
    printf("Grow function search index file.\n");
    printf("Usage: %s <index file> <size_to_grow_in_MB>\n", argv[0]);
    return -1;
  }
  uint64_t grow_size = strtoul(argv[2], nullptr, 10);
  grow_size *= (1024*1024);
  managed_mapped_file::grow(argv[1], grow_size);
}
