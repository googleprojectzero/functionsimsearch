#include "gtest/gtest.h"
#include "functionsimhash.hpp"
#include "simhashtrainer.hpp"
#include "util.hpp"
#include <array>

TEST(simhashtrainer, simple_attraction) {
  std::vector<FunctionFeatures> all_functions;
  std::vector<FeatureHash> all_features_vector;
  std::vector<std::pair<uint32_t, uint32_t>> attractionset;
  std::vector<std::pair<uint32_t, uint32_t>> repulsionset;

  ASSERT_TRUE(LoadTrainingData("../testdata/train_simple_attraction",
    &all_functions, &all_features_vector, &attractionset, &repulsionset));

  SimHashTrainer trainer(
    &all_functions,
    &all_features_vector,
    &attractionset,
    &repulsionset);
  std::vector<double> weights;
  trainer.Train(&weights);

  for (uint32_t index = 0; index < all_features_vector.size(); ++index) {
    printf("Feature %16lx%16lx weight %f\n", all_features_vector[index].first,
      all_features_vector[index].second, weights[index]);

  }
}

// their distance reduced by the training procedure.
TEST(simhashtrainer, attractionset) {
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

