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

#include "util.hpp"

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

bool GetBit(const FeatureHash& hash, uint32_t bit) {
  uint64_t value = (bit > 63) ? hash.second : hash.first;
  bit = bit % 64;
  return (1ULL << bit) & value;
}

template <typename R>
void calculateSimHashFloats(
  const FunctionFeatures& features,
  const std::vector<FeatureHash>* all_features,
  std::vector<R>& out,
  const R* const* const weights,
  const std::map<uint32_t, uint32_t>* global_to_local) {
  for (uint32_t i = 0; i < 128; ++i) {
    for (uint32_t feature_index : features) {
      bool bit = GetBit((*all_features)[feature_index], i);
      uint32_t weight_index = global_to_local->at(feature_index);
      if (bit) {
        out[i] += weights[weight_index][0];
      } else {
        out[i] -= weights[weight_index][0];
      }
    }
  }
}

template <typename R>
R piecewise_linear(R argument) {
  if (argument > 1.01) {
    return 1.01;
  } else if (argument < -1.01) {
    return -1.01;
  }
  return argument;
}

template <typename R>
R sigmoid_shifted(R argument) {
  return ((1.0 / (1 + exp(-argument))) - 0.5) * 2;
}

// A function that returns 1 if the sign of x is different than the sign of
// y, and 0 otherwise.
template <typename R>
R hard_edged_sign_punish(R x, R y) {
  R x_times_y = x * y;
  R x_square = x * x;
  R y_square = y * y;
  return 0.5*(-(x_times_y / sqrt(x_square * y_square))+1);
}

template <typename R>
R punish_wrong_sign(R x, R y) {
  R x_times_y = x * y;
  R x_square = x * x;
  R y_square = y * y;
  return (-(x_times_y / sqrt(x_square * y_square)));
}


template <typename R>
R calculatePairLoss(
  const std::vector<FeatureHash>* all_features,
  const FunctionFeatures* first_function,
  const FunctionFeatures* second_function,
  const R* const* const weights,
  const std::map<uint32_t, uint32_t>* global_to_local,
  bool attract=true) {
  R loss = 0.0;
  // Initialize the two vectors of double/float/sums of weights.
  std::vector<R> functionA(128);
  std::vector<R> functionB(128);
  // Sum the features with their weights into the relevant vector.
  calculateSimHashFloats(*first_function, all_features, functionA, weights,
    global_to_local);
  calculateSimHashFloats(*second_function, all_features, functionB, weights,
    global_to_local);

  // Calculate the component-wise loss. We want the component-wise loss to have
  // the following properties... (TODO: Explain the loss function choice).
  for (uint32_t i = 0; i < 128; ++i) {
    if (attract) {
      R x = functionA[i];
      R y = functionB[i];
      R absolute_distance = sqrt((x-y)*(x-y));
      loss += 10*((x-y)*(x-y)) + (x-1)*(x-1) + (y-1)*(y-1);
    }
  }
  return loss;
}

class SimHashPairLossTerm {
public:
  SimHashPairLossTerm(
    const std::vector<FeatureHash>* all_features,
    const FunctionFeatures* functionA,
    const FunctionFeatures* functionB,
    bool attract,
    uint32_t number_of_pairs,
    const std::map<uint32_t, uint32_t> global_to_local_index) :
      all_features_(all_features),
      functionA_(functionA),
      functionB_(functionB),
      number_of_pairs_(number_of_pairs),
      attract_(attract),
      global_to_local_(global_to_local_index) {};

  template <typename R>
  R operator()(const std::vector<int>& dimensions,
    const R* const* const weights) const {
    R result = calculatePairLoss(
      all_features_,
      functionA_,
      functionB_,
      weights,
      &global_to_local_,
      attract_);
    R temp = number_of_pairs_;
    return result / temp;
  }

private:
  const std::vector<FeatureHash>* all_features_;
  const FunctionFeatures* functionA_;
  const FunctionFeatures* functionB_;
  std::map<uint32_t, uint32_t> global_to_local_;
  uint32_t number_of_pairs_;
  bool attract_;
};

#endif // SIMHASHWEIGHTSLOSSFUNCTOR_HPP
