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
#include <typeinfo>
#include <vector>

#include "util/util.hpp"

// Two useful functions for debugging the learning process (allow conversion
// to double) during loss function calculation.
double toDouble(double d) {
  return d;
}

double toDouble(const fadbad::B<double>& d) {
  return d.val();
}

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
R calculatePairLoss(
  const std::vector<FeatureHash>* all_features,
  const FunctionFeatures* first_function,
  const FunctionFeatures* second_function,
  const R* const* const weights,
  const std::map<uint32_t, uint32_t>* global_to_local,
  bool attract=true) {
  R loss = 0.0;

  // Initialize the two vectors of double/float/sums of weights. Normally, this
  // is done automatically, but after losing 2 weeks debugging FADBAD++ (which
  // at the time did not auto-initialize the entries) this code will be extra-
  // prudent.
  std::vector<R> functionA(128);
  std::vector<R> functionB(128);

  for (int i = 0; i < 128; ++i) {
    functionA[i] = 0;
    functionB[i] = 0;
  }
  // Sum the features with their weights into the relevant vector.
  calculateSimHashFloats(*first_function, all_features, functionA, weights,
    global_to_local);
  calculateSimHashFloats(*second_function, all_features, functionB, weights,
    global_to_local);

  // Calculate the component-wise loss. Understanding the shape of the loss
  // function is a bit involved. It will be explained below, and commands
  // will be provided that help plotting individual components in gnuplot
  // (and hence understand better the function was chosen).
  //
  // At this point in time, we have two vectors of floats, where every entry
  // is a linear combination of the per-feature weights, either with positive
  // or negative sign (depending on the hash of the feature). For "attraction
  // pairs", we wish to minimize the hamming distance of the resulting SimHash -
  // but Hamming Distances optimize poorly because our local derivatives are
  // going to be zero (Hamming Distances are discrete and "jump").
  //
  // In order to minimize that hamming distance, we wish to "punish" weights
  // where bot functionA[i] and functionB[i] have different signs -- because
  // that will lead to different bits in the SimHash, and hence to a bigger
  // hamming distance.
  //
  // Issue the following gnuplot commands:
  //
  // set pm3d
  // g(x,y) = (-(x*y)/sqrt(x**2 * y**2 + 1.0))+1
  // splot [-10:10][-10:10] g(x,y)
  //
  // You will see a "smoothed step function", e.g. a function that approaches
  // 1 when the signs of x and y are not equal, and that goes to zero if the
  // signs of the two functions are equal. The "1.0" constant is a sort of
  // smoothing parameter -- as you decrease it toward 0, the edges of the step
  // function get harder, if you increase it, the entire function gets softer.
  // It also helps ensure that we never issue a sqrt(0), which tends to wreak
  // havoc on the reverse mode differentiation code we are using.
  //
  // We will use this smoothed step function as a building block. Right now,
  // we would like to multiply something into it that makes sure that the flat
  // areas of +1 exhibit some slope to allow weights to be moved so they "slide
  // off the cliff" into an area that has lower loss.
  //
  // First, we get distance between the two floats functionA[i] and functionB[i],
  // and run it through a smoothed version of abs() as follows:
  //    sqrt((functionA[i] - functionB[i])**2 + 0.1)
  // The constant 0.1 helps smooth the kink in the distance function and prevents
  // problems with sqrt(0).
  //
  // Issue the following gnuplot commands:
  // d(x,y) = sqrt((x-y)**2+0.1)
  // splot [-10:10][-10:10] d(x,y)
  //
  // Ok - now if we multiply both functions together, we get something that has
  // most of the properties we want:
  //
  // splot [-10:10][-10:10] g(x,y)*d(x,y)
  //
  // This looks pretty good, but after staring at it for a moment I decided
  // that the near-linear decrease of loss near 0 is not something I wanted, and
  // added a sqrt() around the distance for good measure.
  //
  // Finally, there is a pleasing shape for the loos function:
  //
  // splot [-10:10][-10:10] g(x,y)*sqrt(d(x,y)+0.01)
  //
  // For the "repulsion-pairs" we pretty much just want to flip the entire
  // diagram, so we invert the sign of x.
  for (uint32_t i = 0; i < 128; ++i) {
    R x = functionA[i];
    R y = functionB[i];

    // Flip sign of x for repulsion pairs.
    if (!attract) {
      x = -x;
    }
    R x_times_y = x*y;
    R x_times_y_square = x_times_y * x_times_y;
    // The "1.0" in the following line is the smoothing parameter for the step
    // function.
    R sqrt_of_xsqr_ysqr_plus_one = sqrt(x_times_y_square + 1.0);
    // Function "g" in the comment above.
    R step_function = (-x_times_y / sqrt_of_xsqr_ysqr_plus_one) + 1;

    R distance = x-y;
    R distance_squared = distance * distance;
    // Function "d" in the comment above.
    R absolute_distance = sqrt(distance_squared + 0.1);

    // The final loss to be added for the two floats.
    R added_loss = step_function * sqrt(absolute_distance + 0.01);
    loss += added_loss;
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
    return result / (number_of_pairs_ / 64.0);
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
