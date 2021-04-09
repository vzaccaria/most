#include <map>
#include <set>
#include <utility>
#include <fstream>
#include "bvp.hpp"
#include "error.hpp"
#include "utils.hpp"
#include "func.hpp"

namespace bvp {
using std::make_pair;
using std::pair;
using utils::contains;

//******************** domain stuff ************************************

domain::domain(size_t dimension) { dim = dimension; }
domain::domain(size_t dimension, set<point> intr, set<point> bdry,
               map<point, vector> ns) {
  dim = dimension;
  add_to_interior(intr);
  add_to_boundary(bdry);
  add_to_normals(ns);
}

domain::domain(string intr, string bdry, string ns) {
  using namespace utils;
  using namespace linalg;
  using namespace error_handling;

  bool intr_empty, bdry_empty;

  matrix intr_m;
  try {
    intr_m = read_matrix(intr);
    intr_empty = false;
  } catch (endOfFile) {
    intr_empty = true;
  }

  matrix bdry_m;
  try {
    bdry_m = read_matrix(bdry);
    bdry_empty = false;
  } catch (endOfFile) {
    bdry_empty = true;
  }

  matrix ns_m;
  try {
    ns_m = read_matrix(ns);
  } catch (endOfFile &exc) {
    if (!bdry_empty) {
      exc.reason = "Boundary is not empty, so normals cannot be either.";
      throw exc;
    }
  }

  dim = intr_m.cols();
  if (bdry_m.cols() != dim and !bdry_empty) {
    badArgument exc;
    exc.reason =
        "Wrong parameters for domain from filename. \n"
        "Dimension of boundary (columns) must equal dimension of interior.";
    exc.line = __LINE__;
    exc.file = __FILE__;
    throw exc;
  }
  if (ns_m.cols() != 2 * dim and !bdry_empty) {
    badArgument exc;
    exc.reason = "Wrong parameters for domain from filename. \n"
                 "Dimension of normals (columns) must equal twice the \n"
                 "dimension of the interior and the boundary.";
    exc.line = __LINE__;
    exc.file = __FILE__;
    throw exc;
  }
  slice s(1, dim), s1(1, dim), s2(dim + 1, 2 * dim);

  if (!intr_empty)
    for (size_t i = 1; i <= intr_m.rows(); i++)
      add_to_interior(intr_m(i, s));

  if (!bdry_empty) {
    for (size_t i = 1; i <= bdry_m.rows(); i++)
      add_to_boundary(bdry_m(i, s));
    for (size_t i = 1; i <= ns_m.rows(); i++)
      add_to_normals(ns_m(i, s1), ns_m(i, s2));
  }
}

domain::~domain() {
  // Nothing!
}

// This clears any data already in the domain and sets the dimension.
void domain::set_dimension(size_t dimension) {
  dim = dimension;
  interior.clear();
  boundary.clear();
  normals.clear();
}

// Add information to the domain
void domain::add_to_interior(const set<point> &intr) {
  for (set<point>::const_iterator I = intr.begin(); I != intr.end(); I++) {
    if (I->size() != dim) {
      badArgument exc;
      exc.reason =
          "Cannot assign to domain's interior: inconformant dimensions.";
      exc.line = __LINE__;
      exc.file = __FILE__;
      throw exc;
    }
    interior.insert(*I);
  }
}

void domain::add_to_interior(const point &intr) {
  if (intr.size() != dim) {
    badArgument exc;
    exc.reason = "Cannot assign to domain's interior: inconformant dimensions.";
    exc.line = __LINE__;
    exc.file = __FILE__;
    throw exc;
  }
  interior.insert(intr);
}

void domain::add_to_boundary(const set<point> &bdry) {
  for (set<point>::const_iterator I = bdry.begin(); I != bdry.end(); I++) {
    if (I->size() != dim) {
      badArgument exc;
      exc.reason =
          "Cannot assign to domain's boundary: inconformant dimensions.";
      exc.line = __LINE__;
      exc.file = __FILE__;
      throw exc;
    }
    boundary.insert(*I);
  }
}

void domain::add_to_boundary(const point &bdry) {
  if (bdry.size() != dim) {
    badArgument exc;
    exc.reason = "Cannot assign to domain's boundary: inconformant dimensions.";
    exc.line = __LINE__;
    exc.file = __FILE__;
    throw exc;
  }
  boundary.insert(bdry);
}

void domain::add_to_normals(const map<point, vector> &ns) {
  for (map<point, vector>::const_iterator I = ns.begin(); I != ns.end(); I++) {
    if (!utils::contains(boundary, I->first)) {
      badArgument exc;
      exc.reason = "Bad normal given: must match a point on the boundary.";
      exc.line = __LINE__;
      exc.file = __FILE__;
      throw exc;
    }
    if (I->first.size() != dim or I->second.size() != dim) {
      badArgument exc;
      exc.reason = "Bad normal given: inconformant dimensions.";
      exc.line = __LINE__;
      exc.file = __FILE__;
      throw exc;
    }
    normals.insert(*I);
  }
}

void domain::add_to_normals(const point &bdry, const vector &n) {
  if (!utils::contains(boundary, bdry)) {
    badArgument exc;
    exc.reason = "Bad normal given: must match a point on the boundary.";
    exc.line = __LINE__;
    exc.file = __FILE__;
    throw exc;
  }
  if (bdry.size() != dim or n.size() != dim) {
    badArgument exc;
    exc.reason = "Bad normal given: inconformant dimensions.";
    exc.line = __LINE__;
    exc.file = __FILE__;
    throw exc;
  }
  pair<point, vector> pn = make_pair(bdry, n);
  normals.insert(pn);
}

// Read that information
size_t domain::get_dimension() const { return dim; }
const set<point> &domain::get_interior() const { return interior; }
const set<point> &domain::get_boundary() const { return boundary; }
const map<point, vector> &domain::get_normals() const { return normals; }

// Is point in this domain, whether interior or boundary?
bool domain::contains(const point &p) const {
  if (utils::contains(interior, p))
    return true;
  if (utils::contains(boundary, p))
    return true;
  return false;
}

//**************** BVP stuff *********************************

BVP::BVP(shared_ptr<const domain> O, shared_ptr<const diff_op> L_in,
         shared_ptr<const bdry_diff_op> B_in, const map<point, double> &f_in,
         const map<point, double> &g_in) {
  Omega = O;
  L = L_in;
  B = B_in;
  set_f(f_in);
  set_g(g_in);
}

BVP::BVP(shared_ptr<const domain> O, shared_ptr<const diff_op> L_in,
         shared_ptr<const bdry_diff_op> B_in, const realfunc &f_in,
         const realfunc &g_in) {
  Omega = O;
  L = L_in;
  B = B_in;
  set_f(f_in);
  set_g(g_in);
}

shared_ptr<const domain> BVP::get_domain() const { return Omega; }
shared_ptr<const diff_op> BVP::get_diff_op() const { return L; }
shared_ptr<const bdry_diff_op> BVP::get_bdry_diff_op() const { return B; }
const map<point, double> &BVP::get_f() const { return f; }
const map<point, double> &BVP::get_g() const { return g; }

void BVP::set_f(const map<point, double> &f_in) {
  for (map<point, double>::const_iterator I = f_in.begin(); I != f_in.end();
       I++) {
    if (!contains(Omega->get_interior(), I->first)) {
      badArgument exc;
      exc.reason = "Bad specification of f in BVP: "
                   "gave a point not in the interior.";
      exc.line = __LINE__;
      exc.file = __FILE__;
      throw exc;
    }
    f[I->first] = I->second;
  }
}
void BVP::set_g(const map<point, double> &g_in) {
  for (map<point, double>::const_iterator I = g_in.begin(); I != g_in.end();
       I++) {
    if (!contains(Omega->get_boundary(), I->first)) {
      badArgument exc;
      exc.reason = "Bad specification of g in BVP: "
                   "gave a point not on the boundary.";
      exc.line = __LINE__;
      exc.file = __FILE__;
      throw exc;
    }
    g[I->first] = I->second;
  }
}
void BVP::set_f(const realfunc &f_in) {
  for (set<point>::iterator I = Omega->get_interior().begin();
       I != Omega->get_interior().end(); I++)
    f[*I] = f_in(*I);
}

void BVP::set_g(const realfunc &g_in) {
  for (set<point>::iterator I = Omega->get_boundary().begin();
       I != Omega->get_boundary().end(); I++)
    g[*I] = g_in(*I);
}

shared_ptr<const linear_diff_op2> linear_BVP2::get_linear_diff_op2() const {
  return std::dynamic_pointer_cast<const linear_diff_op2>(BVP::get_diff_op());
}

} // namespace bvp
