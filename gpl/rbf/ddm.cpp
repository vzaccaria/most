#include <map>
#include <set>
#include <utility>
#include "ddm.hpp"
#include "bvp.hpp"
#include "rbf.hpp"
#include "interpolator.hpp"
#include "utils.hpp"
#include "error.hpp"

// debug
#include <iostream>

namespace bvp {
using namespace std;
using namespace linalg;
using linalg::vector;
using std::dynamic_pointer_cast;
using std::shared_ptr;

//************* ddm_bdry_diff_op stuff *****************

ddm_bdry_diff_op::ddm_bdry_diff_op(shared_ptr<const bdry_diff_op> B_in,
                                   shared_ptr<const bdry_diff_op> Bprime_in,
                                   const set<point> &ibps) {
  B = B_in;
  intr_bdry_pts = ibps;
  Bprime = Bprime_in;
}

double ddm_bdry_diff_op::at(const realfunc &f, const point &p) const {
  if (utils::contains(intr_bdry_pts, p))
    return Bprime->at(f, p);

  return B->at(f, p);
}

double ddm_bdry_diff_op::at(const realfunc &f, const point &p,
                            const vector &n) const {
  if (utils::contains(intr_bdry_pts, p))
    return Bprime->at(f, p, n);

  return B->at(f, p);
}

// ************ ddm stuff ******************************

template <typename RBF>
ddm<RBF>::ddm(const set<shared_ptr<const domain> > &ds,
              shared_ptr<const BVP> thebvp) {
  // Gotta check this is actually a domain decomposition...
  set<point> union_interior;
  set<point> union_boundary;

  for (set<shared_ptr<const domain> >::iterator i = ds.begin(); i != ds.end();
       i++) {
    set<point> intr = (*i)->get_interior();
    set<point> bdry = (*i)->get_boundary();
    union_interior.insert(intr.begin(), intr.end());
    union_boundary.insert(bdry.begin(), bdry.end());
  }

  bvp = thebvp;
  shared_ptr<const domain> Omega = bvp->get_domain();
  set<point> interior = Omega->get_interior();
  set<point> boundary = Omega->get_boundary();

  if (interior != union_interior) {
    badArgument exc;
    exc.reason = "Bad argument in domain decomposition method constructor: \n"
                 "The union of the interior of the proposed domains does not \n"
                 "equal the interior of the domain.";
    exc.line = __LINE__;
    exc.file = __FILE__;
    throw exc;
  }

  if (!utils::includes(boundary, union_boundary)) {
    badArgument exc;
    exc.reason = "Bad argument in domain decomposition method constructor: \n"
                 "The union of the boundary of the proposed domains does not \n"
                 "contain the boundary of the  domain.";
    exc.line = __LINE__;
    exc.file = __FILE__;
    throw exc;
  }

  domains = ds;
  tolerance = 1e-7;
}

template <typename RBF> ddm<RBF>::~ddm() {
  // Nothing!
}

template <typename RBF> void ddm<RBF>::set_tolerance(double tol) {
  tolerance = tol;
}

template <typename RBF> double ddm<RBF>::operator()(const point &p) const {
  return at(p);
}

// ************** additive_schwarz_ddm stuff *************************

template <typename RBF>
additive_schwarz_ddm<RBF>::additive_schwarz_ddm(
    const set<shared_ptr<const domain> > &ds,
    const shared_ptr<const linear_BVP2> thebvp)
    : ddm<RBF>(ds, thebvp) {

  shared_ptr<const linear_diff_op2> L =
      dynamic_pointer_cast<const linear_diff_op2>(this->bvp->get_diff_op());

  shared_ptr<const bdry_diff_op> B = this->bvp->get_bdry_diff_op();

  map<point, double> global_f = this->bvp->get_f();
  map<point, double> global_g = this->bvp->get_g();

  // Define a bvp for each domain and assign it an interpolator.
  for (set<shared_ptr<const domain> >::iterator i = this->domains.begin();
       i != this->domains.end(); i++) {
    shared_ptr<const overlapping_domain> this_domain =
        dynamic_pointer_cast<const overlapping_domain>(*i);

    set<point> this_intr = this_domain->get_interior();
    map<point, double> this_f;
    for (set<point>::iterator spi = this_intr.begin(); spi != this_intr.end();
         spi++)
      this_f[*spi] = global_f[*spi];

    set<point> this_bdry = this_domain->get_boundary();
    map<point, double> this_g;
    set<point> interior_boundary_pts;
    for (set<point>::iterator spi = this_bdry.begin(); spi != this_bdry.end();
         spi++) {
      if (this_domain->which_domain(*spi).get() == 0) {
        this_g[*spi] = global_g[*spi];
      } else {
        interior_boundary_pts.insert(*spi);
        this_g[*spi] = 0; // Init to zero artificial boundary conditions.
      }
    }
    shared_ptr<dirichlet_op> D(new dirichlet_op);
    shared_ptr<ddm_bdry_diff_op> this_B(
        new ddm_bdry_diff_op(B, D, interior_boundary_pts));

    shared_ptr<linear_BVP2> this_bvp(
        new linear_BVP2(this_domain, L, this_B, this_f, this_g));
    shared_ptr<interpolator<RBF> > rbf_ptr(new interpolator<RBF>(this_bvp));
    phis[this_domain] = rbf_ptr;
  }
  solve();
}

template <typename RBF>
double additive_schwarz_ddm<RBF>::at(const point &p) const {
  // If p is one of the domain points, then it's used. If more than
  // one domain contains p, then the average is taken. If no domain
  // contains p, we exhaustively search for the closest point in the
  // domains and use the interpolator from that domain to evaluate.

  set<shared_ptr<const interpolator<RBF> > > relevant_interpolators =
      which_interps(p);

  if (relevant_interpolators.size() != 0) // p is in one of the
                                          // domains.
    return avg_interp(relevant_interpolators, p);

  // Else, p is not one of the grid points. Find closest grid point
  // and evaluate on the domain(s) that the point belongs to.

  // Uh, just begin with closest being some point in the whatever
  // domain.
  point c = *(((*(this->domains.begin()))->get_interior()).begin());

  // Search each domain's interior and boundary. Can't do better than
  // exhaustive search.
  for (set<shared_ptr<const domain> >::iterator i = this->domains.begin();
       i != this->domains.end(); i++) {
    for (set<point>::iterator j = (*i)->get_interior().begin();
         j != (*i)->get_interior().end(); j++)
      if (norm(*j - p) < norm(c - p))
        c = *j;

    for (set<point>::iterator j = (*i)->get_boundary().begin();
         j != (*i)->get_boundary().end(); j++)
      if (norm(*j - p) < norm(c - p))
        c = *j;
  }

  return avg_interp(which_interps(c), p);
}

template <typename RBF>
set<shared_ptr<const interpolator<RBF> > >
additive_schwarz_ddm<RBF>::which_interps(const point &p) const {
  set<shared_ptr<const interpolator<RBF> > > relevant_interpolators;
  for (set<shared_ptr<const domain> >::iterator i = this->domains.begin();
       i != this->domains.end(); i++) {
    if ((*i)->contains(p)) {
      shared_ptr<const overlapping_domain> j =
          dynamic_pointer_cast<const overlapping_domain>(*i);
      relevant_interpolators.insert(phis.at(j));
      // at is not in current STL standard; but it is necessary here
      // because operator[] can't be used here since this is a const
      // function. at is currently a GNU extension, and we're using
      // it.
    }
  }
  return relevant_interpolators;
}

template <typename RBF>
double additive_schwarz_ddm<RBF>::avg_interp(
    set<shared_ptr<const interpolator<RBF> > > relevant_interpolators,
    const point &p) const {
  double result = 0;
  int n = 0;
  for (typename set<shared_ptr<const interpolator<RBF> > >::iterator i =
           relevant_interpolators.begin();
       i != relevant_interpolators.end(); i++) {
    result += (*i)->at(p);
    n++;
  }
  return result / n;
}

template <typename RBF> void additive_schwarz_ddm<RBF>::solve() {
  // Recap: each domain already has an interpolator associated to it
  // and all overlapping domain information has been defined. Just
  // need to iterate on the boundary conditions. Method converges
  // when 2-norm (Frobenius, if it were a matrix) of the artificial
  // boundary does not change by more than tolerance.
  using std::make_pair;

  vector newv = at_all_points();
  vector oldv(newv.size());
  double change;
  do {
    oldv = newv;

    // Each domain will have new g's.
    map<shared_ptr<const overlapping_domain>, map<point, double> >
        new_bdry_assts;
    for (set<shared_ptr<const domain> >::iterator i = this->domains.begin();
         i != this->domains.end(); i++) {
      shared_ptr<const overlapping_domain> d =
          dynamic_pointer_cast<const overlapping_domain>(*i);
      for (set<point>::iterator j = (*i)->get_boundary().begin();
           j != (*i)->get_boundary().end(); j++)
        if (d->which_domain(*j).get() != 0) {

          new_bdry_assts[d].insert(
              make_pair(*j, phis[d->which_domain(*j)]->at(*j)));
        }
    }

    // Now assign to each interpolator the modified boundary.
    for (typename map<shared_ptr<const overlapping_domain>,
                      shared_ptr<interpolator<RBF> > >::iterator i =
             phis.begin();
         i != phis.end(); i++)
      i->second->set_g(new_bdry_assts[i->first]);

    newv = at_all_points();
    change = norm(oldv - newv);

    // debug
    cout << "Change: " << change << endl;
  } while (change > this->tolerance);
}

// Evaluate problem at all artificial boundary points.
template <typename RBF>
vector additive_schwarz_ddm<RBF>::at_all_points() const {
  set<point> art_bdry;
  for (set<shared_ptr<const domain> >::const_iterator i = this->domains.begin();
       i != this->domains.end(); i++) {
    shared_ptr<const overlapping_domain>
        j = dynamic_pointer_cast<const overlapping_domain>(*i),
        zero; // The zero pointer.

    for (set<point>::iterator p = j->get_boundary().begin();
         p != j->get_boundary().end(); p++) {
      if (j->which_domain(*p) != zero)
        art_bdry.insert(*p);
    }
  }

  set<point>::iterator I = art_bdry.begin();
  vector result(art_bdry.size());
  for (size_t i = 1; i <= result.size(); i++) {
    result(i) = at(*I);
    I++;
  }
  return result;
}

//************** overlapping_domain stuff *************

overlapping_domain::overlapping_domain(string intr, string bdry, string ns)
    : domain(intr, bdry, ns) {}

overlapping_domain::overlapping_domain(
    string intr, string bdry, string ns,
    const set<shared_ptr<const overlapping_domain> > &ols,
    const map<point, shared_ptr<const overlapping_domain> > &bdry_asst)
    : domain(intr, bdry, ns) {
  set<point> bdry_copy = get_boundary();
  for (map<point, shared_ptr<const overlapping_domain> >::const_iterator i =
           bdry_asst.begin();
       i != bdry_asst.end(); i++) {
    if (!utils::contains(ols, i->second) or
        !utils::contains(bdry_copy, i->first)) {
      badArgument exc;
      exc.reason = "Bad argument in overlapping_domain constructor: \n"
                   "Every boundary assignment must be a boundary point \n"
                   "assigned to some overlapping domain.";
      exc.line = __LINE__;
      exc.file = __FILE__;
      throw exc;
    }
  }
  overlappers = ols;
  boundary_assignments = bdry_asst;
}

overlapping_domain::overlapping_domain(size_t dimension) : domain(dimension) {}
overlapping_domain::overlapping_domain(size_t dimension, set<point> intr,
                                       set<point> bdry, map<point, vector> ns)
    : domain(dimension, intr, bdry, ns) {}

set<shared_ptr<const overlapping_domain> >
overlapping_domain::get_domains() const {
  return overlappers;
}

shared_ptr<const overlapping_domain>
overlapping_domain::which_domain(const point &p) const {
  if (!utils::contains(boundary_assignments, p)) {
    shared_ptr<const overlapping_domain> zero;
    return zero;
  }
  return boundary_assignments.at(p);
}

void overlapping_domain::set_overlapper_info(
    const point &p, const shared_ptr<overlapping_domain> o) {
  if (utils::contains(this->get_boundary(), p))
    boundary_assignments[p] = o;
}

//************** friends of overlapping_domain *******************

void set_overlapper_info(set<shared_ptr<overlapping_domain> > domains) {
  for (set<shared_ptr<overlapping_domain> >::iterator d = domains.begin();
       d != domains.end(); d++) {
    for (set<point>::iterator p = (*d)->get_boundary().begin();
         p != (*d)->get_boundary().end(); p++) {
      for (set<shared_ptr<overlapping_domain> >::iterator d_other =
               domains.begin();
           d_other != domains.end(); d_other++)
        if (utils::contains((*d_other)->get_interior(), *p)) {
          (*d)->boundary_assignments[*p] = *d_other;
          (*d)->overlappers.insert(*d_other);
          break; // We're assuming no three domains overlap at one
                 // point.
        }
    }
  }
}

// Instantiations
using namespace rbf;

template class ddm<piecewise_polynomial>;
template class ddm<thin_plate_spline>;
template class ddm<multiquadric>;
template class ddm<inverse_multiquadric>;
template class ddm<inverse_quadratic>;
template class ddm<gaussian>;

template class additive_schwarz_ddm<piecewise_polynomial>;
template class additive_schwarz_ddm<thin_plate_spline>;
template class additive_schwarz_ddm<multiquadric>;
template class additive_schwarz_ddm<inverse_multiquadric>;
template class additive_schwarz_ddm<inverse_quadratic>;
template class additive_schwarz_ddm<gaussian>;
} // namespace bvp
