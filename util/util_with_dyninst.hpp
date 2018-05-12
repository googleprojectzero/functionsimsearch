#ifndef UTIL_WITH_DYNINST_HPP
#define UTIL_WITH_DYNINST_HPP

// A helper function mostly for the benefit of testing. Iterates through the
// entire disassembly, so this should not be used in a loop.
FeatureHash GetHashForFileAndFunction(FunctionSimHasher& hasher,
  const std::string& filename, const std::string& mode, uint64_t address,
  std::vector<FeatureHash>* feature_hashes = nullptr);

#endif // UTIL_WITH_DYNINST_HPP

