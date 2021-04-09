#ifndef __INTERPOLATOR_H__
#define __INTERPOLATOR_H__

#include <vector>
#include <map>
#include "bvp.hpp"
#include "linalg.hpp"
#include "func.hpp"
#include "diff_op.hpp"

namespace bvp {
using std::map;
using std::shared_ptr;
template <typename RBF> class interpolator : public realfunc {
public:
  interpolator();

  // Interpolate given a BVP
  interpolator(shared_ptr<linear_BVP2> bvp);

  // Interpolate given some data points and the value at those points
  interpolator(const map<point, double> &Xi);

  // STE: permette di salvare in param i parametri.. da restituire all'utente
  interpolator(const map<point, double> &Xi, std::vector<double> &param,
               std::vector<RBF> &param_rbf, bool save);

  // STE
  interpolator(std::vector<double> &param, std::vector<RBF> &param_rbf,
               bool save);

  // Destroys all data already in the interpolator.
  void interpolate(const map<point, double> &Xi);
  void interpolate(shared_ptr<linear_BVP2> bvp);
  void interpolate(const map<point, double> &Xi, std::vector<double> &param,
                   std::vector<RBF> &param_rbf, bool save);
  void interpolate(std::vector<double> &param, std::vector<RBF> &param_rbf,
                   bool save);

  // Evaluation
  double operator()(const point &p) const;
  double at(const point &p) const;

  // Derivatives
  double d(const point &p, size_t k) const;
  double d2(const point &p, size_t k1, size_t k2) const;

  // These functions allow for partial redefinition of the BVP as
  // required for the additive Schwartz domain decomposition method,
  // and for other methods.
  void set_f(const realfunc &f);
  void set_g(const realfunc &g);
  void set_f(const map<point, double> &f);
  void set_g(const map<point, double> &g);

private:
  // Once the matrix is defined, this function inverts it.
  void computecoeffs();
  // STE: carica da file i coefficienti, senza ricalcolare la matrice inversa
  void computecoeffs(std::vector<double> &param);

  void init(shared_ptr<linear_BVP2> bvp);
  void init(shared_ptr<linear_BVP2> bvp, std::vector<double> &param,
            std::vector<RBF> &param_rbf, bool save);
  void init(std::vector<double> &param, std::vector<RBF> &param_rbf, bool save);

  shared_ptr<linear_BVP2> thebvp;

  // Number of interior points.
  size_t n;
  // Number of boundary points.
  size_t m;

  // The matrix to invert
  matrix M;

  bool initted;
  void not_initted(int line, string file) const; // Exception thrower

  linalg::vector coeffs;
  std::vector<RBF> rbfs;

  // Data of where the interpolator has been evaluated needs a point
  // and a vector representing a multi-index of partials (an empty
  // vector represents evaluation, instead of differentiation).
  typedef std::pair<linalg::point, std::vector<size_t> > diff_data;

  // A remember table
  mutable map<diff_data, double> remtable;
};
} // namespace bvp

#endif
