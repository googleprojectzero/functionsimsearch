#include <array>
#include <map>
#include <set>

#include "gtest/gtest.h"
#include "functionsimhash.hpp"
#include "testutil.hpp"
#include "util.hpp"

TEST(functionsimhash, check_feature_uniqueness) {
  FunctionSimHasher hasher("");
  for (const auto& hash_addr : id_to_address_function_1) {
    uint64_t file_hash = hash_addr.first;
    uint64_t address = hash_addr.second;

    std::vector<FeatureHash> feature_hashes;
    FeatureHash trained = GetHashForFileAndFunction(hasher,
      id_to_filename[file_hash], id_to_mode[file_hash], address, &feature_hashes);

    std::set<uint64_t> feature_id_set;
    for (const auto& feature : feature_hashes) {
      feature_id_set.insert(feature.first);
    }
    ASSERT_EQ(feature_hashes.size(), feature_id_set.size());
  }
}

TEST(functionsimhash, check_feature_counts) {
  FunctionSimHasher hasher("");

  std::map<uint64_t, uint32_t> feature_counts = {
    { 0x019beb40ff26b418ULL, 961 },
    { 0x22b6ae5553ee8881ULL, 627 },
    { 0x396063026eaac371ULL, 716 },
    { 0x51f3962ff93c1c1eULL, 674 },
    { 0x584e2f1630b21cfaULL, 521 },
    { 0x5ae018cfafb410f5ULL, 587 },
    { 0x720c272a7261ec7eULL, 917 },
    { 0x83fe3244c90314f4ULL, 627 },
    { 0x924daa0b17c6ae64ULL, 544 },
    { 0xb9b4ecb0faefd8cfULL, 659 },
    { 0xf7f94f1cdfbe0f98ULL, 811 },
    { 0xf89b73cc72cd02c7ULL, 819 }};

  for (const auto& hash_addr : id_to_address_function_1) {
    uint64_t file_hash = hash_addr.first;
    uint64_t address = hash_addr.second;

    std::vector<FeatureHash> feature_hashes;
    FeatureHash trained = GetHashForFileAndFunction(hasher,
      id_to_filename[file_hash], id_to_mode[file_hash], address, &feature_hashes);

    std::set<uint64_t> feature_id_set;
    for (const auto& feature : feature_hashes) {
      feature_id_set.insert(feature.first);
    }
    EXPECT_EQ(feature_id_set.size(), feature_counts[file_hash]);
  }
}

// Test whether loading and using weights from an input text file works. This
// is done by loading a bunch of zero-weights, which should lead to all functions
// having hashes of hamming distance zero.
TEST(functionsimhash, zero_weight_hasher) {
  FunctionSimHasher hasher_trained(
    "../testdata/train_zero_weights/weights.txt");
  const std::map<uint64_t, float>* weights = hasher_trained.GetWeights();

  std::map<std::pair<uint64_t, uint64_t>, FeatureHash> hashes_trained;
  for (const auto& hash_addr : id_to_address_function_1) {
    uint64_t file_hash = hash_addr.first;
    uint64_t address = hash_addr.second;
    std::vector<FeatureHash> feature_hashes;

    FeatureHash trained = GetHashForFileAndFunction(hasher_trained,
      id_to_filename[file_hash], id_to_mode[file_hash], address, &feature_hashes);

    if ((trained.first == 0) && (trained.second == 0)) {
      printf("Failed to open %s and get function at %llx\n",
        id_to_filename[file_hash].c_str(), address);
    }

    // With weights all equal zero, the SimHashes should have all bits set.
    EXPECT_EQ(trained.first, 0xFFFFFFFFFFFFFFFFULL);
    EXPECT_EQ(trained.second, 0xFFFFFFFFFFFFFFFFULL);

    for (uint32_t index = 0; index < feature_hashes.size(); ++index) {
      uint64_t feature_id = feature_hashes[index].first;
      // Assume that each feature_id has an associated weight.
      EXPECT_TRUE(weights->find(feature_id) != weights->end());
      if (weights->find(feature_id) == weights->end()) {
        printf("Bizarre, unknown feature at index %d %16.16lx\n",
          index, feature_id);
        printf("Check with:\n");
        printf("./dumpsinglefunctionfeature %s %s %llx\n",
          id_to_mode[file_hash].c_str(), id_to_filename[file_hash].c_str(),
          address);
      } else {
        EXPECT_EQ(weights->at(feature_id), 0.0);
      }
    }
    ASSERT_TRUE((trained.first != 0) || (trained.second !=0));

    hashes_trained[hash_addr] = trained;
  }

  for (const auto& hash_addr_A : hashes_trained) {
    for (const auto& hash_addr_B : hashes_trained) {
      FeatureHash hash_A = hash_addr_A.second;
      FeatureHash hash_B = hash_addr_B.second;

      uint32_t hamming = HammingDistance(hash_A, hash_B);
      ASSERT_EQ(hamming, 0);
    }
  }
}



