#ifndef SIMHASHTRAINER_HPP
#define SIMHASHTRAINER_HPP

#include <spii/function.h>

#include "util.hpp"

// The code expects the following files to be present inside the data directory
// (which is passed in as first argument):
//
//  functions.txt - a text file formed by concatenating the output of the
//                  functionfingerprints tool in verbose mode. Each line starts
//                  with [file_id]:[address], and is followed by the various
//                  features that make up the function.
//  attract.txt   - a file with pairs of [file_id]:[address] [file_id]:[address]
//                  indicating which functions should be the same.
//  repulse.txt   - a file with pairs of [file_id]:[address] [file_id]:[address]
//                  indicating which functions should NOT be the same
bool LoadTrainingData(const std::string& directory,
   std::vector<FunctionFeatures>* all_functions,
   std::vector<FeatureHash>* all_features_vector,
   std::vector<std::pair<uint32_t, uint32_t>>* attractionset,
   std::vector<std::pair<uint32_t, uint32_t>>* repulsionset);

// Convenience function. Expects the data described in LoadTrainingData, outputs
// a file full of weights.
bool TrainSimHashFromDataDirectory(const std::string& directory,
  const std::string& weights_filename, bool use_lbfgs=true, 
  uint32_t max_steps=100);

class SimHashTrainer {
public:
  SimHashTrainer(
    const std::vector<FunctionFeatures>* all_functions,
    const std::vector<FeatureHash>* all_features,
    const std::vector<std::pair<uint32_t, uint32_t>>* attractionset,
    const std::vector<std::pair<uint32_t, uint32_t>>* repulsionset);

  void Train(std::vector<double>* weights, spii::Solver* solver,
    const std::string& snapshot_directory = "");
private:
  void AddPairLossTerm(const std::pair<uint32_t, uint32_t>& pair,
    spii::Function* function,
    const std::vector<FunctionFeatures>* all_functions,
    const std::vector<FeatureHash>* all_features_vector,
    std::vector<std::vector<double>>* weights,
    uint32_t set_size,
    bool attract);
  const std::vector<FunctionFeatures>* all_functions_;
  const std::vector<FeatureHash>* all_features_;
  const std::vector<std::pair<uint32_t, uint32_t>>* attractionset_;
  const std::vector<std::pair<uint32_t, uint32_t>>* repulsionset_;
};

#endif // SIMHASHTRAINER_HPP
