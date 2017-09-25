#ifndef SGDSOLVER_H
#define SGDSOLVER_H

// Simple SGD solver to experiment with convergence / divergence.
class SPII_API SGDSolver
  public Solver {
public:
  virtual void solve(const Function& function, SolverResults* results) const override;
};

#endif
