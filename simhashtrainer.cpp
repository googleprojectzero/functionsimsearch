#include <map>
#include <random>
#include <vector>

#include <spii/auto_diff_term.h>
#include <spii/dynamic_auto_diff_term.h>
#include <spii/large_auto_diff_term.h>
#include <spii/function.h>
#include <spii/solver.h>
#include <spii/term.h>

#include "sgdsolver.hpp"
#include "simhashweightslossfunctor.hpp"
#include "util.hpp"
#include "simhashtrainer.hpp"

// Helper function to load the contents of a data directory and populate the
// necessary data structures to perform training.




SimHashTrainer::SimHashTrainer(
  const std::vector<FunctionFeatures>* all_functions,
  const std::vector<FeatureHash>* all_features,
  const std::vector<std::pair<uint32_t, uint32_t>>* attractionset,
  const std::vector<std::pair<uint32_t, uint32_t>>* repulsionset) :
  all_functions_(all_functions),
  all_features_(all_features),
  attractionset_(attractionset),
  repulsionset_(repulsionset) {};

void SimHashTrainer::AddPairLossTerm(const std::pair<uint32_t, uint32_t>& pair,
  spii::Function* function,
  const std::vector<FunctionFeatures>* all_functions,
  const std::vector<FeatureHash>* all_features_vector,
  std::vector<std::vector<double>>* weights,
  uint32_t set_size,
  bool attract) {

  std::vector<double*> weights_in_this_pair;
  std::vector<int> dimensions;
  std::set<uint32_t> weights_for_both;

  const FunctionFeatures* functionA = &all_functions->at(pair.first);
  const FunctionFeatures* functionB = &all_functions->at(pair.second);

  // Calculate the union of features present in both functions. The loss term
  // will only include variables from these features.
  weights_for_both.insert(functionA->begin(), functionA->end());
  weights_for_both.insert(functionB->begin(), functionB->end());

  // A map is needed to map the global feature indices to the indices used
  // inside this loss term.
  std::map<uint32_t, uint32_t> global_index_to_pair_index;

  // Add the weights that are important for this loss term, and their
  // dimensionality (always 1). Also make sure we have a mapping from the
  // global weight index to the term-local index.
  uint32_t index = 0;
  for (uint32_t weight_index : weights_for_both) {
    global_index_to_pair_index[weight_index] = index++;
    weights_in_this_pair.push_back(weights->at(weight_index).data());
    dimensions.push_back(1);
  }

  // Add the term to the function.
  function->add_term(
    std::make_shared<spii::LargeAutoDiffTerm<SimHashPairLossTerm>>(
    dimensions, all_features_vector, functionA, functionB,
    attract, set_size, global_index_to_pair_index),
      weights_in_this_pair);
}

void SimHashTrainer::Train(std::vector<double>* output_weights,
  spii::Solver* solver) {
  // Begin constructing the loss function for the training.
  spii::Function function;

  // Each feature has a specific weight.
  uint32_t number_of_weights = all_features_->size();
  std::vector<std::vector<double>> weights(number_of_weights, {0.0});
  output_weights->resize(number_of_weights);

  // We need some random numbers to perturb the initial problem randomly.
  std::mt19937 rng(std::random_device{}());
  std::normal_distribution<float> normal(0, 0.01);

  for (int index = 0; index < number_of_weights; ++index) {
    function.add_variable(weights[index].data(), 1);
    // Initialize the weights to 1.0 + random epsilon.
    weights[index][0] = 1.0 + normal(rng);
  }

  // Create a loss term for each pair from the attraction set.
  for (const auto& pair : *attractionset_) {
    AddPairLossTerm(pair,
      &function, all_functions_, all_features_, &weights,
      attractionset_->size(), true);
  }

  // Create loss terms for the repulsionset.
  for (const auto& pair : *repulsionset_) {
    AddPairLossTerm(pair,
      &function, all_functions_, all_features_, &weights,
      repulsionset_->size(), false);
  }

  // TODO(thomasdullien): Loss terms should be created to maximize entropy on
  // the functions that have not been labeled.

  spii::SolverResults results;
  solver->solve(function, &results);
  std::cout << results << std::endl << std::endl;

  for (uint32_t index = 0; index < number_of_weights; ++index) {
    (*output_weights)[index] = weights[index][0];
  }
}

bool LoadTrainingData(const std::string& directory,
  std::vector<FunctionFeatures>* all_functions,
  std::vector<FeatureHash>* all_features_vector,
  std::vector<std::pair<uint32_t, uint32_t>>* attractionset,
  std::vector<std::pair<uint32_t, uint32_t>>* repulsionset) {

  // Read the contents of functions.txt.
  std::string functionsfilename = directory + "/functions.txt";
  std::vector<std::vector<std::string>> file_contents;
  if (!FileToLineTokens(functionsfilename, &file_contents)) {
    return false;
  }

  // Read all the features, place them in a vector, and populate a map that maps
  // FeatureHash to vector index.
  std::set<FeatureHash> all_features;
  ReadFeatureSet(file_contents, &all_features);
  std::map<FeatureHash, uint32_t> feature_to_vector_index;
  uint32_t index = 0;
  for (const FeatureHash& feature : all_features) {
    all_features_vector->push_back(feature);
    feature_to_vector_index[feature] = index;
    ++index;
  }

  // FunctionFeatures are a vector of uint32_t.
  std::map<std::string, uint32_t> function_to_index;
  index = 0;
  all_functions->resize(file_contents.size());
  for (const std::vector<std::string>& line : file_contents) {
    function_to_index[line[0]] = index;
    for (uint32_t i = 1; i < line.size(); ++i) {
      FeatureHash hash = StringToFeatureHash(line[i]);
      (*all_functions)[index].push_back(feature_to_vector_index[hash]);
    }
    ++index;
  }
  // Feature vector and function vector have been loaded. Now load the attraction
  // and repulsion set.
  std::vector<std::vector<std::string>> attract_file_contents;
  if (!FileToLineTokens(directory + "/attract.txt", &attract_file_contents)) {
    return false;
  }

  std::vector<std::vector<std::string>> repulse_file_contents;
  if (!FileToLineTokens(directory + "/repulse.txt", &repulse_file_contents)) {
    return false;
  }

  for (const std::vector<std::string>& line : attract_file_contents) {
    attractionset->push_back(std::make_pair(
      function_to_index[line[0]],
      function_to_index[line[1]]));
  }

  for (const std::vector<std::string>& line : repulse_file_contents) {
    repulsionset->push_back(std::make_pair(
      function_to_index[line[0]],
      function_to_index[line[1]]));
  }

  printf("[!] Loaded %ld functions (%ld unique features)\n",
    all_functions->size(), all_features_vector->size());
  printf("[!] Attraction-Set: %ld pairs\n", attractionset->size());
  printf("[!] Repulsion-Set: %ld pairs\n", repulsionset->size());

  return true;
}

bool TrainSimHashFromDataDirectory(const std::string& directory, const
  std::string& outputfile, bool use_lbfgs, uint32_t max_steps) {
  std::vector<FunctionFeatures> all_functions;
  std::vector<FeatureHash> all_features_vector;
  std::vector<std::pair<uint32_t, uint32_t>> attractionset;
  std::vector<std::pair<uint32_t, uint32_t>> repulsionset;

  if (!LoadTrainingData(directory,
    &all_functions,
    &all_features_vector,
    &attractionset,
    &repulsionset)) {
    return false;
  }

  printf("[!] Training data parsed, beginning the training process.\n");

  SimHashTrainer trainer(
    &all_functions,
    &all_features_vector,
    &attractionset,
    &repulsionset);

  std::unique_ptr<spii::Solver> solver;
  if (use_lbfgs) {
    solver.reset(new spii::LBFGSSolver);
    solver->maximum_iterations = max_steps;
  } else {
    solver.reset(new spii::SGDSolver);
  }

  std::vector<double> weights;
  trainer.Train(&weights, solver.get());

  // Write the weights file.
  {
    std::ofstream outfile(outputfile);
    if (!outfile) {
      printf("Failed to open outputfile %s.\n", outputfile.c_str());
      return false;
    }
    for (uint32_t i = 0; i < all_features_vector.size(); ++i) {
      char buf[512];
      FeatureHash& hash = all_features_vector[i];
      sprintf(buf, "%16.16lx%16.16lx %f", hash.first, hash.second,
        weights[i]);
      outfile << buf << std::endl;
    }
  }
  return true;
}


