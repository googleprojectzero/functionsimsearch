#include <array>
#include <map>
#include <set>

#include "gtest/gtest.h"
#include "disassembly/flowgraphwithinstructions.hpp"
#include "disassembly/flowgraphwithinstructionsfeaturegenerator.hpp"
#include "searchbackend/functionsimhash.hpp"
#include "util/testutil.hpp"
#include "util/util_with_dyninst.hpp"

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

// Test whether loading and using weights from an input text file works. This
// is done by loading a bunch of zero-weights, which should lead to all functions
// having hashes of hamming distance zero.
TEST(functionsimhash, zero_weight_hasher) {
  FunctionSimHasher hasher_trained(
    "../testdata/train_zero_weights/weights.txt");
  const std::map<uint64_t, float>* weights = hasher_trained.GetWeights();

  std::map<std::pair<uint64_t, uint64_t>, FeatureHash> hashes_trained;

  // Goes through all functions in the id_to_address_function_1 map and checks
  // if their hashes with the zero weights evaluate to all-bits-set, and that
  // weights for every hash that is detected are set to zero in the incoming
  // weights.txt file.
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
        // A feature was found in the set of features for the function that
        // did not have a corresponding weight in the weights.txt that was
        // loaded. This should not happen, and can be investigated by dumping
        // the features extracted from that function using the
        // dumpsinglefunctionfeature tool.
        printf("Unknown feature for %16.16lx %16.16lx at index %d %16.16lx\n",
          file_hash, address, index, feature_id);
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

// Test that zero weights for mnemonic tuples for two identical CFGs hashes
// to the same value.
TEST(functionsimhash, zero_weight_for_mnemonics) {
  FunctionSimHasher hasher_weight_zero("",
    false, // Do not disable graphs.
    false, // Do not disable mnemonics.
    false, // Do not dump the graphlet dictionary.
    false, // Do not dump the mnemonic dictionary.
    0.0, // Mnemonic weight set to zero.
    1.0 // Graphlet weight set to one.
  );
  FunctionSimHasher hasher_ignore_mnemonics("",
    false, // Do not disable graphs.
    true, // Disable mnemonics.
    false, // Do not dump the graphlet dictionary.
    false, // Do not dump the mnemonic dictionary.
    0.0, // Mnemonic weight set to zero.
    1.0 // Graphlet weight set to one.
  );

  FlowgraphWithInstructions graph1;
  FlowgraphWithInstructions graph2;
  FlowgraphWithInstructionsFromJSONFile(
    "../testdata/vp9_set_target_rate.clang.nothumb.json", &graph1);
  FlowgraphWithInstructionsFromJSONFile(
    "../testdata/vp9_set_target_rate.clang.with.thumb.json", &graph2);

  FlowgraphWithInstructionsFeatureGenerator featuregen1(graph1);
  FlowgraphWithInstructionsFeatureGenerator featuregen2(graph2);

  std::vector<uint64_t> hash1;
  hasher_weight_zero.CalculateFunctionSimHash(&featuregen1, 128, &hash1);

  std::vector<uint64_t> hash2;
  hasher_weight_zero.CalculateFunctionSimHash(&featuregen2, 128, &hash2);

  std::vector<uint64_t> hash3;
  hasher_ignore_mnemonics.CalculateFunctionSimHash(&featuregen1, 128, &hash3);

  std::vector<uint64_t> hash4;
  hasher_ignore_mnemonics.CalculateFunctionSimHash(&featuregen2, 128, &hash4);

  // All hashes should be the same.
  EXPECT_EQ(hash1[0], hash2[0]);
  EXPECT_EQ(hash1[1], hash2[1]);
  EXPECT_EQ(hash1[0], hash3[0]);
  EXPECT_EQ(hash1[1], hash3[1]);
  EXPECT_EQ(hash1[0], hash4[0]);
  EXPECT_EQ(hash1[1], hash4[1]);

  EXPECT_EQ(hash1[0], 0xb74b6fbed1a325ea);
  EXPECT_EQ(hash1[1], 0xc6b79fb0afe3a8fd);
}



