#include <spii/auto_diff_term.h>
#include <array>

#include "gtest/gtest.h"
#include "sgdsolver.hpp"
#include "util.hpp"

class BasicTestLossTerm {
public:
  BasicTestLossTerm() {};

  template <typename R>
  R operator()(const R* const x, const R* const y) const {

    return (x[0]-1)*(x[0]-1) + (y[0]-1)*(y[0]-1);
  }
};

// Find the minimum of (x+1)**2 + (y+1)**2
TEST(sgdsolver, basic_test) {
  spii::Function function;

  double x = 6.0;
  double y = -5.0;

  function.add_variable(&x, 1);
  function.add_variable(&y, 1);

  std::vector<double*> vars = {&x, &y};

  auto term = std::make_shared<spii::AutoDiffTerm<BasicTestLossTerm, 1, 1>>();

  function.add_term(term, vars);

  spii::SGDSolver solver;
  spii::SolverResults results;
  solver.solve(function, &results);
}



