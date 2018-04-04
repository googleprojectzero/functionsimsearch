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

#ifndef PERSISTENTMAP_HPP
#define PERSISTENTMAP_HPP

#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/set.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <functional>
#include <utility>

using namespace boost::interprocess;

// From the boost documentation.
typedef allocator<char, managed_mapped_file::segment_manager> CharAllocator;
typedef basic_string<char, std::char_traits<char>, CharAllocator> MyShmString;
typedef allocator<MyShmString, managed_mapped_file::segment_manager>
  StringAllocator;

template <typename ValueType>
class PersistentVector {
  typedef allocator<ValueType, managed_mapped_file::segment_manager>
    PersistentVectorAllocator;
  typedef set<ValueType, std::less<ValueType>, PersistentVectorAllocator>
    InnerPersistentVector;
  public:
  PersistentVector(
    const std::string& vectorname, std::shared_ptr<managed_mapped_file>& segment,
    bool create) :
    vectorname_(vectorname),
    segment_(segment),
    allocator_(segment_->get_segment_manager()) {
    Init(create);
  }

  void Init(bool create) {
    if (create) {
      vector_ = segment_->construct<InnerPersistentVector>(vectorname_.c_str())
        (std::less<ValueType>(), allocator_);
    } else {
      vector_ = segment_->find<InnerPersistentVector>(vectorname_.c_str()).first;
    }
  }
  InnerPersistentVector* getVector() const { return vector_; };

private:
  std::string vectorname_;
  std::shared_ptr<managed_mapped_file> segment_;
  PersistentVectorAllocator allocator_;
  vector<ValueType, PersistentVectorAllocator>* vector_;
};

template <typename ValueType>
class PersistentSet {
  typedef allocator<ValueType, managed_mapped_file::segment_manager>
    PersistentSetAllocator;
  typedef set<ValueType, std::less<ValueType>, PersistentSetAllocator>
    InnerPersistentSet;

public:
  PersistentSet(
    const std::string& setname, std::shared_ptr<managed_mapped_file>& segment,
    bool create) :
    setname_(setname),
    segment_(segment),
    allocator_(segment_->get_segment_manager()) {
    Init(create);
  }

  void Init(bool create) {
    if (create) {
      set_ = segment_->construct<InnerPersistentSet>(setname_.c_str())
        (std::less<ValueType>(), allocator_);
    } else {
      set_ = segment_->find<InnerPersistentSet>(setname_.c_str()).first;
    }
  }
  InnerPersistentSet* getSet() const { return set_; };

private:
  std::string setname_;
  std::shared_ptr<managed_mapped_file> segment_;
  PersistentSetAllocator allocator_;
  set<ValueType, std::less<ValueType>, PersistentSetAllocator>* set_;
};

template <typename KeyType, typename ValueType>
class PersistentMap {
  typedef std::pair<const KeyType, ValueType> PairType;
  typedef allocator<PairType, managed_mapped_file::segment_manager>
    PersistentMapAllocator;
  typedef map<KeyType, ValueType, std::less<KeyType>, PersistentMapAllocator>
    InnerPersistentMap;
public:
  PersistentMap(const std::string& filename, bool create) :
    filename_(filename),
    mapname_("map"),
    segment_(new managed_mapped_file(open_or_create, filename.c_str(),
      1ul << 24)),
    allocator_(segment_->get_segment_manager()) {
    Init(create);
  }

  PersistentMap(
    const std::string& mapname, std::shared_ptr<managed_mapped_file>& segment,
      bool create) :
      filename_(""),
      mapname_(mapname),
      segment_(segment),
      allocator_(segment_->get_segment_manager()) {
    Init(create);
  }

  void Init(bool create) {
    if (create) {
      map_ = segment_->construct<InnerPersistentMap>(mapname_.c_str())
        (std::less<KeyType>(), allocator_);
    } else {
      map_ = segment_->find<InnerPersistentMap>(mapname_.c_str()).first;
    }
  }

  InnerPersistentMap* getMap() const { return map_; }

  std::shared_ptr<managed_mapped_file>& getSegment() { return segment_; }
private:
  std::string filename_;
  std::string mapname_;
  std::shared_ptr<managed_mapped_file> segment_;
  PersistentMapAllocator allocator_;
  map<KeyType, ValueType, std::less<KeyType>,
    PersistentMapAllocator>* map_;
};


#endif // PERSISTENTMAP_HPP
