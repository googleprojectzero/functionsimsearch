#include <map>
#include <random>
#include <vector>

#include <spii/auto_diff_term.h>
#include <spii/dynamic_auto_diff_term.h>
#include <spii/large_auto_diff_term.h>
#include <spii/function.h>
#include <spii/solver.h>
#include <spii/term.h>

#include "util/mappedtextfile.hpp"
#include "sgdsolver.hpp"
#include "simhashweightslossfunctor.hpp"
#include "util/util.hpp"
#include "learning/simhashtrainer.hpp"
#include "learning/trainingdata.hpp"

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

bool WriteWeightsFile(const std::string& outputfile,
  const std::vector<FeatureHash>& all_features_vector,
  const std::vector<double>& weights) {
  std::ofstream outfile(outputfile);
  if (!outfile) {
    printf("Failed to open outputfile %s.\n", outputfile.c_str());
    return false;
  }
  for (uint32_t i = 0; i < all_features_vector.size(); ++i) {
    char buf[512];
    const FeatureHash& hash = all_features_vector[i];
    sprintf(buf, "%16.16lx%16.16lx %f", hash.first, hash.second,
      weights[i]);
    outfile << buf << std::endl;
  }
  return true;
}

void SimHashTrainer::Train(std::vector<double>* output_weights,
  spii::Solver* solver, const std::string& snapshot_directory) {
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

  // Disable calculation of the Hessian for the function (since L-BFGS does
  // not need it.
  function.hessian_is_enabled = false;

  // TODO(thomasdullien): Loss terms should be created to maximize entropy on
  // the functions that have not been labeled.

  // If the snapshot directory is non-empty, set a callback from the solver to
  // save the intermediate results every 20 steps.
  uint32_t iteration = 0;
  if (snapshot_directory != "") {
    solver->callback_function =
      [&](const spii::CallbackInformation& info) -> bool {
        ++iteration;
        if ((iteration % 20) == 0) {
          function.copy_global_to_user(*info.x);
          char buf[256];
          sprintf(buf, "%d.snapshot", iteration);
          std::string snapshotfile = snapshot_directory + buf;
          for (uint32_t index = 0; index < number_of_weights; ++index) {
            (*output_weights)[index] = weights[index][0];
          }
          WriteWeightsFile(snapshotfile, *all_features_, *output_weights);
        }
        return true;
      };
  }

  spii::SolverResults results;
  solver->solve(function, &results);
  std::cout << results << std::endl << std::endl;

  for (uint32_t index = 0; index < number_of_weights; ++index) {
    (*output_weights)[index] = weights[index][0];
  }
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

  return WriteWeightsFile(outputfile, all_features_vector, weights);
}


