#include "gtest/gtest.h"
#include "functionsimhash.hpp"
#include "simhashtrainer.hpp"
#include "util.hpp"
#include <array>

// This test calculates the similarity between RarVM::ExecuteStandardFilter
// in various optimization settings, both pre- and post-training.
std::map<uint64_t, const std::string> id_to_filename = {
  { 0xf89b73cc72cd02c7ULL, "../testdata/unrar.5.5.3.builds/unrar.x64.O0" },
  { 0x5ae018cfafb410f5ULL, "../testdata/unrar.5.5.3.builds/unrar.x64.O2" },
  { 0x51f3962ff93c1c1eULL, "../testdata/unrar.5.5.3.builds/unrar.x64.O3" },
  { 0x584e2f1630b21cfaULL, "../testdata/unrar.5.5.3.builds/unrar.x64.Os" },
  { 0xf7f94f1cdfbe0f98ULL, "../testdata/unrar.5.5.3.builds/unrar.x86.O0" },
  { 0x83fe3244c90314f4ULL, "../testdata/unrar.5.5.3.builds/unrar.x86.O2" },
  { 0x396063026eaac371ULL, "../testdata/unrar.5.5.3.builds/unrar.x86.O3" },
  { 0x924daa0b17c6ae64ULL, "../testdata/unrar.5.5.3.builds/unrar.x86.Os" }};

std::map<uint64_t, const std::string> id_to_mode = {
  { 0xf89b73cc72cd02c7ULL, "ELF" },
  { 0x5ae018cfafb410f5ULL, "ELF" },
  { 0x51f3962ff93c1c1eULL, "ELF" },
  { 0x584e2f1630b21cfaULL, "ELF" },
  { 0xf7f94f1cdfbe0f98ULL, "ELF" },
  { 0x83fe3244c90314f4ULL, "ELF" },
  { 0x396063026eaac371ULL, "ELF" },
  { 0x924daa0b17c6ae64ULL, "ELF" }};

// The addresses of RarVM::ExecuteStandardFilter in the various executables
// listed above.
std::map<uint64_t, uint64_t> id_to_address_function_1 = {
  { 0xf89b73cc72cd02c7, 0x0000000000418400 },
  { 0x5ae018cfafb410f5, 0x0000000000411890 },
  { 0x51f3962ff93c1c1e, 0x0000000000416f60 },
  { 0x584e2f1630b21cfa, 0x000000000040e092 },
  { 0xf7f94f1cdfbe0f98, 0x000000000805e09c },
  { 0x83fe3244c90314f4, 0x0000000008059f70 },
  { 0x396063026eaac371, 0x000000000805e910 },
  { 0x924daa0b17c6ae64, 0x00000000080566fc } };


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
  trainer.Train(&weights);

  std::map<uint64_t, float> hash_to_weight;

  for (uint32_t index = 0; index < all_features_vector.size(); ++index) {
    printf("Feature %16lx%16lx weight %f\n", all_features_vector[index].first,
      all_features_vector[index].second, weights[index]);
    hash_to_weight[all_features_vector[index].first] = weights[index];
  }

  // We expect that the weight of the first two features (which are not shared)
  // should be close to zero, and the weight of the last two features (which
  // are shared) should be close to one.

  EXPECT_LE(weights[0], 0.05);
  EXPECT_LE(weights[1], 0.05);
  EXPECT_GE(weights[2], 0.95);
  EXPECT_GE(weights[3], 0.95);

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

  printf("Untrained floats:\n");
  DumpFloatVector(hash_untrained_A_floats);
  DumpFloatVector(hash_untrained_B_floats);
  DumpDelta(hash_untrained_A_floats, hash_untrained_B_floats);

  printf("Trained floats:\n");
  DumpFloatVector(hash_trained_A_floats);
  DumpFloatVector(hash_trained_B_floats);
  DumpDelta(hash_trained_A_floats, hash_trained_B_floats);

  printf("Untrained Hamming distance is %d\n", HammingDistance(
    hash_untrained_A[0], hash_untrained_A[1], hash_untrained_B[0],
    hash_untrained_B[1]));

  printf("Hash of A is %16.16lx%16.16lx trained\n", hash_trained_A[0],
    hash_trained_A[1]);
  printf("Hash of B is %16.16lx%16.16lx trained\n", hash_trained_B[0],
    hash_trained_B[1]);

  uint32_t trained_hamming = HammingDistance(
    hash_trained_A[0], hash_trained_A[1], hash_trained_B[0],
    hash_trained_B[1]);

  printf("Trained hamming distance is %d\n", trained_hamming);

  EXPECT_EQ(trained_hamming, 0);
}

TEST(simhashtrainer, simple_attraction) {
  RunSimpleAttractionTest("../testdata/train_simple_attraction");
}

TEST(simhashtrainer, simple_attraction2) {
  RunSimpleAttractionTest("../testdata/train_simple_attraction2");
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
    "/tmp/attraction_weights.txt"), true);

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
  for (const auto& hash_addr_A : hashes_untrained) {
    for (const auto& hash_addr_B : hashes_untrained) {
      FeatureHash hash_A = hash_addr_A.second;
      FeatureHash hash_B = hash_addr_B.second;

      printf("%d ", HammingDistance(hash_A, hash_B));
    }
    printf("\n");
  }
  printf("[!] Trained hamming distances:\n");
  for (const auto& hash_addr_A : hashes_trained) {
    for (const auto& hash_addr_B : hashes_trained) {
      FeatureHash hash_A = hash_addr_A.second;
      FeatureHash hash_B = hash_addr_B.second;

      printf("%d ", HammingDistance(hash_A, hash_B));
    }
    printf("\n");
  }

}

