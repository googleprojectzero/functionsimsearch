#include <spii/solver.h>
#include "gtest/gtest.h"
#include "functionsimhash.hpp"
#include "sgdsolver.hpp"
#include "simhashtrainer.hpp"
#include "testutil.hpp"
#include "util.hpp"
#include <array>

void DumpFloatVector(const std::vector<float>& vector) {
  printf("(");
  for (float value : vector) {
    printf("%f, ", value);
  }
  printf(")\n");
}

void DumpDelta(const std::vector<float>& vec1, const std::vector<float>& vec2) {
  printf("(");
  for (uint32_t i = 0; i < vec1.size(); ++i) {
    float val = vec1.at(i) - vec2.at(i);
    printf("%f, ", val);
  }
  printf("(\n");
}

void RunSimpleAttractionTest(const std::string& pathname) {
  std::vector<FunctionFeatures> all_functions;
  std::vector<FeatureHash> all_features_vector;
  std::vector<std::pair<uint32_t, uint32_t>> attractionset;
  std::vector<std::pair<uint32_t, uint32_t>> repulsionset;

  ASSERT_TRUE(LoadTrainingData(pathname, &all_functions, &all_features_vector,
    &attractionset, &repulsionset));

  SimHashTrainer trainer(
    &all_functions,
    &all_features_vector,
    &attractionset,
    &repulsionset);
  std::vector<double> weights;

  spii::LBFGSSolver sgd;
  //spii::SGDSolver sgd;
  trainer.Train(&weights, &sgd);

  std::map<uint64_t, float> hash_to_weight;

  for (uint32_t index = 0; index < all_features_vector.size(); ++index) {
    printf("Feature %16lx%16lx weight %f\n", all_features_vector[index].first,
      all_features_vector[index].second, weights[index]);
    hash_to_weight[all_features_vector[index].first] = weights[index];
  }

  // Instantiate two simhashers - one without the trained weights and one with
  // the trained weights - to ensure that the hamming distance between the
  // hashes is reduced by the training procedure.
  FunctionSimHasher hasher_untrained("");
  FunctionSimHasher hasher_trained(&hash_to_weight);

  std::vector<FeatureHash> functionA;
  for (uint32_t index : all_functions[0]) {
    functionA.push_back(all_features_vector[index]);
  }
  std::vector<FeatureHash> functionB;
  for (uint32_t index : all_functions[1]) {
    functionB.push_back(all_features_vector[index]);
  }

  std::vector<float> hash_trained_A_floats;
  std::vector<float> hash_untrained_A_floats;
  std::vector<float> hash_trained_B_floats;
  std::vector<float> hash_untrained_B_floats;

  std::vector<uint64_t> hash_trained_A;
  std::vector<uint64_t> hash_untrained_A;
  std::vector<uint64_t> hash_trained_B;
  std::vector<uint64_t> hash_untrained_B;

  hasher_untrained.CalculateFunctionSimHash(&functionA, &hash_untrained_A,
    &hash_untrained_A_floats);
  hasher_trained.CalculateFunctionSimHash(&functionA, &hash_trained_A,
    &hash_trained_A_floats);
  hasher_untrained.CalculateFunctionSimHash(&functionB, &hash_untrained_B,
    &hash_untrained_B_floats);
  hasher_trained.CalculateFunctionSimHash(&functionB, &hash_trained_B,
    &hash_trained_B_floats);

  printf("Hash of A is %16.16lx%16.16lx untrained\n", hash_untrained_A[0],
    hash_untrained_A[1]);
  printf("Hash of B is %16.16lx%16.16lx untrained\n", hash_untrained_B[0],
    hash_untrained_B[1]);

  uint32_t untrained_hamming = HammingDistance(
    hash_untrained_A[0], hash_untrained_A[1], hash_untrained_B[0],
    hash_untrained_B[1]);

  printf("Hash of A is %16.16lx%16.16lx trained\n", hash_trained_A[0],
    hash_trained_A[1]);
  printf("Hash of B is %16.16lx%16.16lx trained\n", hash_trained_B[0],
    hash_trained_B[1]);

  uint32_t trained_hamming = HammingDistance(
    hash_trained_A[0], hash_trained_A[1], hash_trained_B[0],
    hash_trained_B[1]);

  printf("Untrained hamming distance is %d\n", untrained_hamming);
  printf("Trained hamming distance is %d\n", trained_hamming);

  // Pushing the distance down to zero is expected.
  EXPECT_EQ(trained_hamming, 0);
}

TEST(simhashtrainer, simple_attraction2) {
  RunSimpleAttractionTest("../testdata/train_simple_attraction2");
}

TEST(simhashtrainer, simple_attraction) {
  RunSimpleAttractionTest("../testdata/train_simple_attraction");
}

TEST(simhashtrainer, simple_attraction3) {
  RunSimpleAttractionTest("../testdata/train_simple_attraction3");
}

// A test to validate that two functions from an attractionset will indeed have
// their distance reduced by the training procedure.
TEST(simhashtrainer, attractionset) {
  // Initialize an untrained hasher.
  FunctionSimHasher hasher_untrained("");

  // Train weights on the testing data, and save them to the temp directory.
  ASSERT_EQ(TrainSimHashFromDataDirectory("../testdata/train_attraction_only",
    "/tmp/attraction_weights.txt", true), true);

  // Initialize a trained hasher.
  FunctionSimHasher hasher_trained("/tmp/attraction_weights.txt");

  printf("[!] Ran training\n");
  // Get the hashes for all functions above, with both hashers.
  std::map<std::pair<uint64_t, uint64_t>, FeatureHash> hashes_untrained;
  std::map<std::pair<uint64_t, uint64_t>, FeatureHash> hashes_trained;

  for (const auto& hash_addr : id_to_address_function_1) {
    uint64_t file_hash = hash_addr.first;
    uint64_t address = hash_addr.second;

    FeatureHash untrained = GetHashForFileAndFunction(hasher_untrained,
      id_to_filename[file_hash], id_to_mode[file_hash], address);
    FeatureHash trained = GetHashForFileAndFunction(hasher_trained,
      id_to_filename[file_hash], id_to_mode[file_hash], address);

    ASSERT_TRUE((untrained.first != 0) || (untrained.second !=0));
    ASSERT_TRUE((trained.first != 0) || (trained.second !=0));

    hashes_untrained[hash_addr] = untrained;
    hashes_trained[hash_addr] = trained;
  }

  printf("[!] Calculated the hashes!\n");

  // All the untrained and trained hashes are available now.
  // Calculate a similarity matrix using both and dump it out.
  printf("[!] Untrained hamming distances:\n");

  for (std::map<std::pair<uint64_t, uint64_t>, FeatureHash>::const_iterator
    untrained_iter = hashes_untrained.begin();
    untrained_iter != hashes_untrained.end(); ++untrained_iter) {

    auto next_iter = untrained_iter;
    next_iter++;
    FeatureHash hash_A = untrained_iter->second;
    while (next_iter != hashes_untrained.end()) {
      FeatureHash hash_B = next_iter->second;
      printf("%02.02d ", HammingDistance(hash_A, hash_B));
      ++next_iter;
    }
    printf("...\n");
  }
  for (const auto& hash_addr_A : hashes_untrained) {
    for (const auto& hash_addr_B : hashes_untrained) {
      FeatureHash hash_A = hash_addr_A.second;
      FeatureHash hash_B = hash_addr_B.second;

      printf("%02.02d ", HammingDistance(hash_A, hash_B));
    }
    printf("\n");
  }
  printf("[!] Trained hamming distances:\n");
  for (const auto& hash_addr_A : hashes_trained) {
    for (const auto& hash_addr_B : hashes_trained) {
      FeatureHash hash_A = hash_addr_A.second;
      FeatureHash hash_B = hash_addr_B.second;

      printf("%02.02d ", HammingDistance(hash_A, hash_B));
    }
    printf("\n");
  }

}

