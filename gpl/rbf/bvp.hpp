/*! \file bvp.hpp
 * \brief Classes domain, BVP, and linear_BVP2 are declared here.
 */

#ifndef __BVP_H__
#define __BVP_H__

#include <map>
#include <memory>
#include <set>
#include <string>
#include "linalg.hpp"
#include "diff_op.hpp"
#include "func.hpp"

/// Boundary value problems namespace
namespace bvp {
using linalg::matrix;
using linalg::point;
using linalg::vector;
using std::map;
using std::set;
using std::shared_ptr;
using std::string;

/// A domain is a bunch of interior points, a bunch of boundary points,
/// and a normal vector at each boundary point.
class domain {
public:
  /// Allocate empty domain of given dimension.
  domain(size_t dimension);
  /// Allocate domain given a boundary, an interior, and a set of normals.
  domain(size_t dimension, set<point> intr, set<point> bdry,
         map<point, vector> ns);

  /*!/brief Construct the domain from filenames.
   *
   * Each file containing the desired points as a matrix structure
   * where the number of columns must be the dimensionality of the
   * points and each row is one point.
   *
   * The normals are given by a matrix with number of columns twice
   * that of the interior and boundary, with the first part of the
   * columns giving the basepoint and the second part of the columns
   * giving the normal itself.
   *
   * A boundary or interior matrix can be empty, but the normals
   * matrix can't be empty if the boundary isn't.
   * \param intr - Filename holding the interior of the domain.
   * \param bdry - Filename holding the boundary of the domain.
   * \param ns   - Filename holding the normals of the domain.
   */
  domain(string intr, string bdry, string ns);

  /// This clears any data already in the domain and sets the dimension.
  void set_dimension(size_t dimension);

  // Add information to the domain
  /// Add a set of points to the interior of the domain.
  void add_to_interior(const set<point> &intr);
  /// Add a point to the interior of the domain.
  void add_to_interior(const point &intr);
  /// Add a set of points to the boundary of the domain.
  void add_to_boundary(const set<point> &bdry);
  /// Add a point to the boundary of the domain.
  void add_to_boundary(const point &bdry);
  /*! \brief  Add a set of normals to the domain.
   *
   * Every normal added to the domain must be attached to a boundary
   * point already in the domain.
   * \param ns - A set of normals to add.
   */
  void add_to_normals(const map<point, vector> &ns);
  /*! \brief Add a normal to the domain
   * Every normal added to the domain must be attached to a boundary
   * point already in the domain.
   * \param bdry - The boundary point where to attach this normal.
   * \param n - A  normal to add.
   */
  void add_to_normals(const point &bdry, const vector &n);

  // Read that information
  /// Get the domain's dimension.
  size_t get_dimension() const;
  /// Get the interior.
  const set<point> &get_interior() const;
  /// Get the boundary.
  const set<point> &get_boundary() const;
  /// Get the normals.
  const map<point, vector> &get_normals() const;

  /// Is point in this domain, whether interior or boundary?
  bool contains(const point &p) const;

  virtual ~domain();

private:
  /// Can't create a domain without at least specifying its
  /// dimension.
  domain();
  size_t dim;
  set<point> interior;
  set<point> boundary;
  map<point, vector> normals;
};

/*! \brief A boundary value problem
 *
 * A boundary value problem is a domain \f$\Omega\f$, a differential
 * operator \f$\mathcal{L}\f$ on \f$\Omega\f$, a boundary
 * differential operator \f$\mathcal{B}\f$ on \f$\partial\Omega\f$,
 * and the right hand side values of the equations. Think
 *
 \f[
 \begin{cases}
 \mathcal{L}u = f &\text{on } \Omega \\
 \mathcal{B}u = g &\text{on } \partial\Omega.
 \end{cases}
 \f]
 */
class BVP {
public:
  /*! \brief Create a boundary value problem.
   *
   * Given a domain, boundary and interior operators, and values
   * that those operators must take on domain and interior, create a
   * BVP.
   * \param O - A shared_ptr to the domain.
   * \param L_in - A shared_ptr to the interior operator.
   * \param B_in - A shared_ptr to the boundary operator.
   * \param f_in - An std::map giving the values that the interior
   * operator must take.
   * \param g_in - An std::map giving the values that the boundary
   * operator must take.
   */
  BVP(shared_ptr<const domain> O, shared_ptr<const diff_op> L_in,
      shared_ptr<const bdry_diff_op> B_in, const map<point, double> &f_in,
      const map<point, double> &g_in);
  /*! \brief Create a boundary value problem.
   *
   * Given a domain, boundary and interior operators, and values
   * that those operators must take on domain and interior, create a
   * BVP.
   * \param O - A shared_ptr to the domain.
   * \param L_in - A shared_ptr to the interior operator.
   * \param B_in - A shared_ptr to the boundary operator.
   * \param f_in - A realfunc giving the values tha the interior
   * operator must take.
   * \param g_in - A realfunc giving the values that the boundary
   * operator must take.
   */
  BVP(shared_ptr<const domain> O, shared_ptr<const diff_op> L_in,
      shared_ptr<const bdry_diff_op> B_in, const realfunc &f_in,
      const realfunc &g_in);
  virtual ~BVP(){};

  /// Get the domain of this BVP.
  shared_ptr<const domain> get_domain() const;
  /// Get the interior operator of this BVP.
  shared_ptr<const diff_op> get_diff_op() const;
  /// Get the boundary operator of this BVP.
  shared_ptr<const bdry_diff_op> get_bdry_diff_op() const;
  /// Get the interior values of this BVP.
  const map<point, double> &get_f() const;
  /// Get the boundary values of this BVP.
  const map<point, double> &get_g() const;

  /// Change the interior values of this BVP.
  void set_f(const realfunc &f_in);
  /// Change the boundary values of this BVP.
  void set_g(const realfunc &g_in);

  /*! \brief Change or set interior values of this BVP.
   *
   * Given an std::map of interior values, this function sets or
   * changes the corresponding interior values in the BVP.
   * \param f_in - An std::map assigning scalar values to interior
   * points.
   */
  void set_f(const map<point, double> &f_in);
  /*! \brief Change or set interior values of this BVP.
   *
   * Given an std::map of boundary values, this function sets or
   * changes the corresponding boundary values in the BVP.
   * \param g_in - An std::map assigning scalar values to boundary
   * points.
   */
  void set_g(const map<point, double> &g_in);

private:
  /// No copying allowed!
  BVP(const BVP &){};
  shared_ptr<const domain> Omega;
  shared_ptr<const diff_op> L;
  shared_ptr<const bdry_diff_op> B;
  map<point, double> f;
  map<point, double> g;
};

/// A linear BVP of order at most 2.
class linear_BVP2 : public BVP {
public:
  /// Identical to base class constructor.
  linear_BVP2(shared_ptr<const domain> O,
              shared_ptr<const linear_diff_op2> L_in,
              shared_ptr<const bdry_diff_op> B_in,
              const map<point, double> &f_in, const map<point, double> &g_in)
      : BVP(O, L_in, B_in, f_in, g_in){};
  /// Identical to base class constructor.
  linear_BVP2(shared_ptr<const domain> O,
              shared_ptr<const linear_diff_op2> L_in,
              shared_ptr<const bdry_diff_op> B_in, const realfunc &f_in,
              const realfunc &g_in)
      : BVP(O, L_in, B_in, f_in, g_in){};
  /// Give the interior diff_op.
  shared_ptr<const linear_diff_op2> get_linear_diff_op2() const;
};
} // namespace bvp

#endif //__BVP_H__
