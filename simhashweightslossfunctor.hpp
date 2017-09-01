#ifndef SIMHASHWEIGHTSLOSSFUNCTOR_HPP
#define SIMHASHWEIGHTSLOSSFUNCTOR_HPP

#include <cmath>
#include <map>
#include <vector>

//
// The functor that calculates the loss function for the SimHash weights. This
// is somewhat mathematically involved, hence the lengthy explanation below:
//
// During the calculation of the SimHash, each "feature" is hashed into an N-bit
// number (in our case, 128 bits). So each function is really just a collection
// of 128-bit vectors.
// The SimHash itself is calculated by starting from a 128-entry float vector
// full of zeros. One then iterates over the per-feature hashes, and for each
// 1-bit one adds "+1" to the relevant float, and for each 0-bit, one adds "-1"
// to the relevant float. The final SimHash is calculated by replacing each
// positive float in the array with a 1-bit, and each negative float with a
// 0-bit. The distance between two SimHashes is then simply popcount(a^b).
//
// What we wish to obtain from a loss function is the following:
//  1) Labeled functions that should be the same should have their distance
//     minimized.
//  2) Labeled functions that should be different should have their distance
//     maximized.
//  3) The hash function should maximize entropy on the unlabeled samples.
//
// So the loss function should consist of:
//
//    sum_{(f1, f2) in attractionset} SimHash(f1)
//

// The hash of an individual feature.
typedef std::pair<uint64_t, uint64_t> FeatureHash;
// A vector of indices into a FeatureHash vector represents a function.
typedef std::vector<uint32_t> FunctionFeatures;

class SimHashWeightLossFunction {
public:
  SimHashWeightsLossFunction(
    std::vector<FeatureHash>* all_features,
    std::vector<FunctionFeatures>* all_functions,
    std::vector<std::pair<uint32_t, uint32_t>>* attractionset,
    std::vector<std::pair<uint32_t, uint32_t>>* repulsionset,
    std::vector<uint32_t>* unlabeledset) :
      all_features_(all_features),
      attractionset_(attractionset),
      repulsionset_(repulsionset),
      unlabeledset_(unlabeledset) {};

  bool GetBit(const FeatureHash& hash, uint32_t bit) {
    uint64_t value = (bit > 63) ? hash.second : hash.first;
    bit = bit % 64;
    return (1ULL << bit) & value;
  }

  template <typename R>
  void calculateSimHashFloats(const FunctionFeatures& features,
    std::vector<R>& out, const R* weights) {
    for (uint32_t i = 0; i < 128; ++i) {
      for (uint32_t feature_index : features) {
        bool bit = GetBit((*all_features_)[feature_index], i);
        if (bit) {
          out[feature_index] += weights[feature_index];
        } else {
          out[feature_index] -= weights[feature_index];
        }
      }
    }
  }

  template <typename R>
  R piecewise_linear(R argument) {
    if (argument > 1.0) {
      return 1.0;
    } else if (argument < -1.0) {
      return -1.0;
    }
    return argument;
  }

  template <typename R>
  R calculatePairLoss(const std::vector<std::pair<uint32_t, uint32_t>>* pairs,
    const R* weights, bool attract=true) {
    R loss = 0.0;

    for (std::pair<uint32_t, uint32_t> pair : *pairs) {
      std::vector<R> functionA(128);
      std::vector<R> functionB(128);
      const FunctionFeatures& first_function = all_functions_->at(pair.first);
      const FunctionFeatures& second_function = all_functions_->at(pair.second);
      calculateSimHashFloats(first_function, functionA, weights);
      calculateSimHashFloats(second_function, functionB, weights);

      // Calculate (functionA[i] * functionB[i]) and feed the result through
      // a piecewise linear function. If both entries have the same sign
      // (which is desired), the result will be positive, otherwise negative.
      // Invert the sign of the product, then feed through a piecewise linear
      // function.
      for (uint32_t i = 0; i < 128; ++i) {
        if (attract) {
          loss += -piecewise_linear(functionA[i] * functionB[i]);
        } else {
          loss += piecewise_linear(functionA[i] * functionB[i]);
        }
      }
    }
    loss /= pairs.size();
    return loss;
  }

  template <typename R>
  R operator()(const R* weights) const {
    R attraction_loss = calculatePairLoss(attractionset_, weights, true);
    R repulsion_loss = calculatePairLoss(repulsionset_, weights, false);

    // The last thing we wish to do is to maximize entropy on the unlabeled
    // set. Since maximizing the entropy directly seems cumbersome, it should
    // be enough to ask for the hash function being nearly balanced on the
    // unlabeled set.
    R entropy_loss = 0.0;
    std::vector<R> sum_of_floats(128);
    for (uint32_t function_index : *unlabeledset_) {
      const FunctionFeatures& function = all_functions_->at(function_index);
      std::vector<R> function_floats(128);
      calculateSimHashFloats(function, function_floats, weights);
      for (uint32_t i = 0; i < 128; ++i) {
        sum_of_floats[i] += piecewise_linear(function_floats[i]);
      }
    }
    // Calculate the L2 norm.
    for (uint32_t i = 0; i < 128; ++i) {
      entropy_loss += sum_of_floats[i] * sum_of_floats[i];
    }
    entropy_loss = std::sqrt(entropy_loss);
    return attraction_loss + repulsion_loss + entropy_loss;
  }
private:
  // A pointer to a vector containing all features that can be encountered.
  std::vector<FeatureHash>* all_features_;
  // A pointer to a vector containing all
  std::vector<FunctionFeatures>* all_functions_;
  std::vector<std::pair<uint32_t, uint32_t>>* attractionset_;
  std::vector<std::pair<uint32_t, uint32_t>>* repulsionset_;
  std::vector<uint32_t>* unlabeledset_;
}


#endif // SIMHASHWEIGHTSLOSSFUNCTOR_HPP
