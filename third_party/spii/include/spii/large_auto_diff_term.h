#ifndef LARGE_AUTO_DIFF_TERM_H
#define LARGE_AUTO_DIFF_TERM_H
//
// This header defines LargeAutoDiffTerm, in which both the number
// of variables and their sizes are known only at runtime.
//
//    class Functor {
//     public:
//      template <typename R>
//      R operator()(const std::vector<int>& dimensions, const R* const* const x) const {
//        // ...
//      }
//    };
//    vector<int> dimensions = {2, 3, 5};
//    LargeAutoDiffTerm<Functor> my_term(dimensions);
//
// is equivalent to
//
//    class Functor {
//     public:
//      template <typename R>
//      R operator()(const R* const x, const R* const y, const R* const z) const {
//        // ...
//      }
//    };
//    AutoDiffTerm<Functor, 2, 3, 5> my_term();
//
// Note that LargeAutoDiffTerm will be slower than AutoDiffTerm
// for small number of variables.
//
#include <vector>

#include <spii-thirdparty/badiff.h>
#include <spii-thirdparty/fadiff.h>

#include <spii/term.h>

namespace spii {

template <typename Functor>
class LargeAutoDiffTerm final : public Term {
 public:
	template <typename... Args>
	LargeAutoDiffTerm(std::vector<int> dimensions_, Args&&... args)
	    : functor(std::forward<Args>(args)...),
	      dimensions(std::move(dimensions_)),
	      total_size(create_total_size(dimensions)) {}

	static int create_total_size(const std::vector<int>& dimensions) {
		spii_assert(dimensions.size() >= 1, "Number of variables can not be 0.");
		int total_size = 0;
		for (auto d : dimensions) {
			spii_assert(d >= 1, "A variable dimension must be 1 or greater.");
			total_size += d;
		}
		return total_size;
	}

	int number_of_variables() const override { return dimensions.size(); }

	int variable_dimension(int var) const { return dimensions[var]; }

	double evaluate(double* const* const variables) const override {
		return functor(dimensions, variables);
	}

	double evaluate(double* const* const variables,
	                std::vector<Eigen::VectorXd>* gradient) const override {
		using R = fadbad::B<double>;

		std::vector<R> x_data(total_size);
		std::vector<R*> x(dimensions.size());
		int pos = 0;
		for (int i = 0; i < dimensions.size(); ++i) {
			auto d = dimensions[i];
			x[i] = &x_data[pos];
			for (int j = 0; j < d; ++j) {
				x[i][j] = variables[i][j];
			}
			pos += d;
		}

		R f = functor(dimensions, x.data());
		f.diff(0, 1);

		for (int i = 0; i < dimensions.size(); ++i) {
			auto d = dimensions[i];
			for (int j = 0; j < d; ++j) {
        auto gradient_entry = x[i][j].d(0);

        if (isnan(gradient_entry)) {
          printf("Encountered NaN gradient entry!\n");
          exit(-1);
        }
				(*gradient)[i][j] = x[i][j].d(0);
			}
			pos += d;
		}

		return f.val();
	}

	double evaluate(double* const* const variables,
	                std::vector<Eigen::VectorXd>* gradient,
	                std::vector<std::vector<Eigen::MatrixXd>>* hessian) const override {
		throw std::runtime_error("Hessian not implemented for LargeAutoDiffTerm.");
	}

 private:
	const Functor functor;
	const std::vector<int> dimensions;
	const int total_size;
};
}

#endif
