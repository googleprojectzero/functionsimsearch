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

#include "third_party/pe-parse/pe-parser-library/include/parser-library/parse.h"
#include "third_party/pe-parse/pe-parser-library/include/parser-library/nt-headers.h"

#include "pecodesource.hpp"

PECodeRegion::PECodeRegion(uint8_t* buffer, size_t length, Dyninst::Address
  section_base, const std::string& name, bool is_amd64) : base_(section_base),
  size_(length) {
  data_ = static_cast<uint8_t*>(malloc(length));
  memcpy(data_, buffer, length);
  is_code_ = true;
  is_64_bit_ = is_amd64;
}

PECodeRegion::~PECodeRegion() {
  free(data_);
}

bool PECodeSource::isValidAddress( const Dyninst::Address addr ) const {
  Dyninst::ParseAPI::CodeRegion* region = getRegion(addr);
  if (region != NULL) {
    return true;
  } else {
    return false;
  }
}

PECodeSource::PECodeSource(const std::string& filename) : is_amd64_(false) {
  peparse::parsed_pe* parsed_file = peparse::ParsePEFromFile(filename.c_str());

  if (parsed_file == 0) {
    printf("[!] Failure to parse PE file!\n");
    return;
  }

  // Determine if this is a AMD64 or i386 file.
  uint32_t machine = parsed_file->peHeader.nt.FileHeader.Machine;
  if (machine == peparse::IMAGE_FILE_MACHINE_AMD64) {
    is_amd64_ = true;
  } else if (machine != peparse::IMAGE_FILE_MACHINE_I386) {
    printf("[!] PE file is neither i386 nor AMD64, aborting!\n");
    peparse::DestructParsedPE(parsed_file);
    return;
  }

  peparse::iterSec section_callback = [](void* N,
    peparse::VA section_base, std::string& section_name,
    peparse::image_section_header s, peparse::bounded_buffer* data) -> int {
    PECodeSource* pe_code_source = static_cast<PECodeSource*>(N);

    PECodeRegion* new_region =  new PECodeRegion(
      data->buf, data->bufLen, static_cast<Dyninst::Address>(section_base),
      section_name, pe_code_source->isAmd64());
    // Debug print the new section?
    pe_code_source->_regions.push_back(new_region);
    pe_code_source->_region_tree.insert(new_region);
    return 1; // TODO(thomasdullien): Unclear what the right return value is?
  };
  peparse::IterSec( parsed_file, section_callback, static_cast<void*>(this) );
  parsed_ = true;

  peparse::DestructParsedPE(parsed_file);
}

Dyninst::ParseAPI::CodeRegion* PECodeSource::getRegion(
  const Dyninst::Address addr) const {
  std::set<Dyninst::ParseAPI::CodeRegion*> regions;
  _region_tree.find(addr, regions);
  for (Dyninst::ParseAPI::CodeRegion* region : regions) {
    return region;
  }
  return NULL;
}

void* PECodeSource::getPtrToInstruction(const Dyninst::Address addr) const {
  return getRegion(addr)->getPtrToInstruction(addr);
}

void* PECodeSource::getPtrToData(const Dyninst::Address addr) const {
  return getPtrToInstruction(addr);
}

unsigned int PECodeSource::getAddressWidth() const {
  return _regions[0]->getArch();
}

bool PECodeSource::isCode(const Dyninst::Address addr) const {
  Dyninst::ParseAPI::CodeRegion* region = getRegion(addr);
  if (region && region->isCode(addr)) {
    return true;
  }
  return false;
}

bool PECodeSource::isData(const Dyninst::Address addr) const {
  Dyninst::ParseAPI::CodeRegion* region = getRegion(addr);
  if (region && region->isData(addr)) {
    return true;
  }
  return false;
}

bool PECodeSource::isReadOnly(const Dyninst::Address addr) const {
  Dyninst::ParseAPI::CodeRegion* region = getRegion(addr);
  if (region && region->isReadOnly(addr)) {
    return true;
  }
  return false;
}

Dyninst::Address PECodeSource::offset() const {
  return 0;
}

Dyninst::Address PECodeSource::length() const {
  return 0;
}

Dyninst::Architecture PECodeSource::getArch() const {
  return _regions[0]->getArch();
}


