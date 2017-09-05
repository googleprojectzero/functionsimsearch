// Copyright 2017 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SIMHASHWEIGHTSLOSSFUNCTOR_HPP
#define SIMHASHWEIGHTSLOSSFUNCTOR_HPP

#include <cmath>
#include <map>
#include <vector>

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

bool GetBit(const FeatureHash& hash, uint32_t bit) {
  uint64_t value = (bit > 63) ? hash.second : hash.first;
  bit = bit % 64;
  return (1ULL << bit) & value;
}

template <typename R>
void calculateSimHashFloats(const FunctionFeatures& features,
  const std::vector<FeatureHash>* all_features,
  std::vector<R>& out, const R* weights) {
  for (uint32_t i = 0; i < 128; ++i) {
    for (uint32_t feature_index : features) {
      bool bit = GetBit((*all_features)[feature_index], i);
      if (bit) {
        out[i] += weights[feature_index];
      } else {
        out[i] -= weights[feature_index];
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
R calculatePairLoss(
  const std::vector<std::pair<uint32_t, uint32_t>>* pairs,
  const std::vector<FunctionFeatures>* all_functions,
  const std::vector<FeatureHash>* all_features, uint32_t pair_index,
  const R* weights, bool attract=true) {
  R loss = 0.0;
  // Load the relevant pair.
  std::pair<uint32_t, uint32_t> pair = pairs->at(pair_index);
  // Initialize the two vectors of double/float/sums of weights.
  std::vector<R> functionA(128);
  std::vector<R> functionB(128);
  // Obtain the set of features for each function in the pair.
  const FunctionFeatures& first_function = all_functions->at(pair.first);
  const FunctionFeatures& second_function = all_functions->at(pair.second);
  // Sum the features with their weights into the relevant vector.
  calculateSimHashFloats(first_function, all_features, functionA, weights);
  calculateSimHashFloats(second_function, all_features, functionB, weights);

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
  loss /= pairs->size();
  return loss;
}

class SimHashPairLossTerm {
public:
  SimHashPairLossTerm(
    std::vector<FeatureHash>* all_features,
    std::vector<FunctionFeatures>* all_functions,
    std::vector<std::pair<uint32_t, uint32_t>>* attractionset,
    uint32_t relevant_pair_index, bool attract) :
      all_features_(all_features),
      all_functions_(all_functions),
      pairset_(attractionset),
      relevant_pair_index_(relevant_pair_index),
      attract_(attract) {};

  template <typename R>
  R operator()(const R* weights) const {
    R pair_loss = calculatePairLoss(
      pairset_, all_functions_, all_features_, relevant_pair_index_,
      weights, attract_);
    return pair_loss;
  }

private:
  std::vector<FeatureHash>* all_features_;
  std::vector<FunctionFeatures>* all_functions_;
  std::vector<std::pair<uint32_t, uint32_t>>* pairset_;
  uint32_t relevant_pair_index_;
  bool attract_;
};

#endif // SIMHASHWEIGHTSLOSSFUNCTOR_HPP
