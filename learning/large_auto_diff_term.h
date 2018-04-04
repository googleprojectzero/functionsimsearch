// Thomas Dullien 2017
#ifndef SPII_LARGE_AUTO_DIFF_TERM_H
#define SPII_LARGE_AUTO_DIFF_TERM_H
//
// This header provides a specialized version of AutoDiffTerm, allowing the
// size and the number of variables to be specified at runtime. The goal is to
// have a Term that can use an arbitrary subset of a much larger set of
// variables.
//
// Since only the Functor will know which (and how many) variables it uses,
// we require it to not be a true Functor -- e.g. we require it to provide
// the extra functions
//    const std::vector<double*> getVariables() const;
//    const int getDimension(int variable_index) const;
// The second function will fill the output vector with double*'s pointing to
// the individual variables.
//
#include <type_traits>
#include <typeinfo>
#include <spii-thirdparty/badiff.h>
#include <spii-thirdparty/fadiff.h>

#include <spii/auto_diff_term.h>
#include <spii/dynamic_auto_diff_term.h>

namespace spii {

template<typename NotReallyFunctor>
class LargeAutoDiffTerm;

// Since we make requirements beyond operator(), we do not call our functor-like
// template parameter a functor.
template<typename NotReallyFunctor>
class LargeAutoDiffTerm : public Term {
public:
  template<typename... Args>
  LargeAutoDiffTerm(Args&&... args)
    : not_functor_(args...), used_variables_(not_functor_.getVariables()),
    number_of_variables_(used_variables_.size()) {
  }

  virtual int number_of_variables() const override {
    return number_of_variables_;
  }

  virtual int variable_dimension(int var) const override {
    return not_functor_.getDimension(var);
  }

  virtual void read(std::istream& in) override {
    call_read_if_exists(in, not_functor_);
  }

  virtual void write(std::ostream& out) const override {
    call_write_if_exists(out, not_functor_);
  }

  virtual double evaluate(double * const * const variables) const override {
    return not_functor_(variables);
  }

  virtual double evaluate(double * const * const variables,
    std::vector<Eigen::VectorXd>* gradient) const override {
    typedef fadbad::F<double> Dual;

    // Calculate the total number of values under consideration by adding the
    // dimensionality of each variable.
    int number_of_vars = 0;
    for (uint32_t variable_index = 0; variable_index < number_of_variables_;
      ++variable_index) {
      number_of_vars += not_functor_.getDimension(variable_index);
    }

    // Create a vector of vectors of Duals - one Dual per value to optimize.
    std::vector<std::vector<Dual>> all_vars;
    for (int variable_index = 0; variable_index < number_of_variables_;
      ++variable_index) {
      // Make sure the n-th entry in the vector is large enough to accomodate
      // one Dual per dimension.
      int per_var_dimension = not_functor_.getDimension(variable_index);
      all_vars.emplace_back(per_var_dimension);
      std::vector<Dual>& vars = all_vars.back();

      // Copy the variables over.
      for (int i = 0; i < not_functor_.getDimension(variable_index); ++i) {
        vars[i] = variables[variable_index][i];
        vars[i].diff(i, number_of_vars);
      }
    }

    // The functor expects an R **, so construct an array of R*'s for it.
    Dual* data_pointers[all_vars.size()];
    for (int i = 0; i < all_vars.size(); ++i) {
      data_pointers[i] = all_vars[i].data();
    }
    // Call the functor to perform the calculation.
    Dual f(not_functor_(data_pointers));

    for (int variable_index = 0; variable_index < number_of_variables_;
      ++variable_index) {
      for (int dimension = 0;
        dimension < not_functor_.getDimension(variable_index); ++dimension) {
        (*gradient)[variable_index](dimension) = f.d(dimension);
      }
    }

    return f.x();
  }

  virtual double evaluate(double * const * const variables,
    std::vector<Eigen::VectorXd>* gradient,
    std::vector< std::vector<Eigen::MatrixXd> >* hessian) const override {
    check(false, to_string(typeid(*this).name(), ": hessian not implemented."));
    return 0;
  }

  const std::vector<double*> getVariables() const {
    return not_functor_.getVariables();
  }

private:
  NotReallyFunctor not_functor_;
  const std::vector<double*> used_variables_;
  const int number_of_variables_;
};

}; // namespace spii

#endif // SPII_DYNAMIC_AUTO_DIFF_TERM_H

