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

#ifndef PECODESOURCE_HPP
#define PECODESOURCE_HPP

#include "CodeSource.h"

class PECodeRegion : public Dyninst::ParseAPI::CodeRegion {
public:
  PECodeRegion(uint8_t* buffer, size_t length, Dyninst::Address section_base, 
    const std::string& name, bool is_amd64);
  ~PECodeRegion();

  Dyninst::Address low() const { return base_; }
  Dyninst::Address high() const { return base_ + size_; }

  virtual bool isValidAddress( const Dyninst::Address addr ) const {
    return addr > base_ && (addr < (base_ + size_));
  }

  virtual void* getPtrToInstruction( const Dyninst::Address addr ) const {
    return &data_[ addr - base_ ];
  }

  virtual void* getPtrToData( const Dyninst::Address addr ) const {
    return &data_[ addr - base_ ];
  }

  virtual unsigned int getAddressWidth() const {
    return is_64_bit_ ? 8 : 4;
  }

  virtual bool isCode(Dyninst::Address addr) const {
    if (is_code_ && addr >= low() && addr <= high())
      return true;
    return false;
  }

  virtual bool isData(Dyninst::Address addr) const {
    return is_data_;
  }

  virtual bool isReadOnly(Dyninst::Address addr) const {
    return is_read_only_;
  }

  Dyninst::Address offset() const {
    return base_;
  }

  Dyninst::Address length() const {
    return size_;
  }

  Dyninst::Architecture getArch() const {
    return is_64_bit_ ? Dyninst::Architecture::Arch_x86_64 : 
      Dyninst::Architecture::Arch_x86;
  }

private:
  bool is_code_ = true;
  bool is_data_ = false;
  bool is_read_only_ = true;
  bool is_64_bit_ = false;
  uint64_t base_;
  size_t size_;
  uint8_t *data_;
};

class PECodeSource : public Dyninst::ParseAPI::CodeSource {
public:
  PECodeSource(const std::string& filename);

  Dyninst::Address baseAddress() { return 0x40000; }
  Dyninst::Address loadAddress() { return 0x40000; }

  bool isValidAddress( const Dyninst::Address addr ) const;
  void* getPtrToInstruction(const Dyninst::Address) const;
  void* getPtrToData(const Dyninst::Address) const;
  unsigned int getAddressWidth() const;
  bool isCode(const Dyninst::Address) const;
  bool isData(const Dyninst::Address) const;
  bool isReadOnly(const Dyninst::Address) const;
  Dyninst::Address offset() const;
  Dyninst::Address length() const;
  Dyninst::Architecture getArch() const;

  bool parsed() const { return parsed_; }
  bool isAmd64() const { return is_amd64_; }
private:
  Dyninst::ParseAPI::CodeRegion* getRegion(const Dyninst::Address addr) const;
  bool parsed_ = false;
  bool is_amd64_ = false;
};

#endif // PECODESOURCE_HPP
