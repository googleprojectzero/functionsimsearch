
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>

#include <Eigen/Dense>

#include <spii/spii.h>
#include <spii/solver.h>

#include "sgdsolver.hpp"

namespace spii {

void SGDSolver::solve(const Function& function, SolverResults* results) const {
  // Get the dimensionality of the problem.
  size_t dimension = function.get_number_of_scalars();
  std::cout << "Dimension is " << dimension << std::endl;

  if (dimension == 0) {
    results->exit_condition = SolverResults::FUNCTION_TOLERANCE;
    return;
  }

  Eigen::VectorXd current_point, gradient;
  Eigen::VectorXd next_point, next_point_gradient;

  function.copy_user_to_global(&current_point);
  Eigen::VectorXd stepdirection(dimension);

  results->exit_condition = SolverResults::INTERNAL_ERROR;
  int iteration = 0;

  double function_value = std::numeric_limits<double>::quiet_NaN();
  double gradient_norm = std::numeric_limits<double>::quiet_NaN();

  double previous_function_value = 0;
  double gain = 1.0;

  double exponentially_weighed_average_gain = gain;

  while (iteration < 500) {
    function_value = function.evaluate(current_point, &gradient);
    gradient_norm = std::max(gradient.maxCoeff(), -gradient.minCoeff());
    stepdirection = -gradient;

    gain = previous_function_value - function_value;

    printf("[%d:] Loss is %+.17e, norm is %+.17e, gain is %+.17e\n",
      iteration, function_value, gradient_norm, gain);

    double stepsize = (1.0 / gradient_norm) * (1.0/(iteration+1));

    // Evaluate the function at the next point and see if we are making
    // progress.
    double next_function_value;
    while (stepsize > 1.0e-10) {
      next_point = current_point + (10*stepsize) * stepdirection;
      next_function_value = function.evaluate(next_point,
        &next_point_gradient);
      // In order to make progress, the next_function_value needs to be smaller
      // than the current_function_value, so the gain needs to be positive.
      gain = function_value - next_function_value;
      if ((gain <= 0) || std::isnan(gain)) {
        printf("[%d:] gain is %+.17e, reducing stepsize * norm from %+.17e\n", 
          iteration, gain, stepsize * gradient_norm);
        stepsize = stepsize / 100.0;
      } else {
        printf("[%d:] gain is %+.17e reducing loss to %+.17e\n", iteration,
          gain, next_function_value);
        current_point = next_point;
        gradient = next_point_gradient;
        break;
      }
    }

    if (stepsize <= 1.0e-100) {
      exponentially_weighed_average_gain = gain + (0.001 *
        exponentially_weighed_average_gain);
    } else {
      exponentially_weighed_average_gain = gain +
        (0.5 * exponentially_weighed_average_gain);
    }

    printf("[%d:] Exponentially weighed avg gain: %+.17e\n", iteration,
      exponentially_weighed_average_gain);

    if (exponentially_weighed_average_gain < 1.0e-20) {
      printf("[%d:] Exponentially weighed avg gain too small, aborting\n", iteration);
      if (exponentially_weighed_average_gain < 0.0) {
        printf("[%d:] Can't be negative, exiting.\n", iteration);
        exit(-1);
      }
      break;
    }

    if (function_value < 1.0) {
      printf("[%d:] Loss less than 1, aborting\n", iteration);
      break;
    }
    ++iteration;
    previous_function_value = function_value;
  }

  function.copy_global_to_user(current_point);
}


} // namespace spii

