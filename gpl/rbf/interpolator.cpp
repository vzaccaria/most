#include <vector>
#include <set>
#include <fstream>
#include <iostream>
#include <utility>
#include "interpolator.hpp"
#include "rbf.hpp"
#include "func.hpp"

namespace bvp {
using std::shared_ptr;
template <typename RBF> interpolator<RBF>::interpolator() { initted = false; }

template <typename RBF>
interpolator<RBF>::interpolator(shared_ptr<linear_BVP2> bvp) {
  // Workaround because gdb can't break inside constructors. :-(
  init(bvp);
}

template <typename RBF>
interpolator<RBF>::interpolator(const map<point, double> &Xi) {
  interpolate(Xi);
}

// STE:  gli passo la matrice e la struttura dati in cui salvare i coefficienti
template <typename RBF>
interpolator<RBF>::interpolator(const map<point, double> &Xi,
                                std::vector<double> &param,
                                std::vector<RBF> &param_rbf, bool save) {
  // param è il vettore con i parametri
  // l'ultimo parametro indica se devo salvarli, oppure caricarli da vettore
  // true = salvali
  // false = usali
  interpolate(Xi, param, param_rbf, save);
}

// STE:  gli passo la matrice e la struttura dati in cui salvare i coefficienti
template <typename RBF>
interpolator<RBF>::interpolator(std::vector<double> &param,
                                std::vector<RBF> &param_rbf, bool save) {
  // param è il vettore con i parametri
  // l'ultimo parametro indica se devo salvarli, oppure caricarli da vettore
  // true = salvali
  // false = usali
  interpolate(param, param_rbf, save);
}

template <typename RBF>
void interpolator<RBF>::interpolate(const map<point, double> &Xi,
                                    std::vector<double> &param,
                                    std::vector<RBF> &param_rbf, bool save) {

  if (Xi.empty()) { // Dude, wtf?
    badArgument exc;
    exc.reason = "Cannot interpolate if no data is given.";
    exc.line = __LINE__;
    exc.file = __FILE__;
    throw exc;
  }

  // Create a trivial bvp.
  shared_ptr<Id_op> Id(new Id_op);
  shared_ptr<dirichlet_op> D(new dirichlet_op);
  set<point> intr;
  set<point> bdry;        // empty
  map<point, point> nrml; // empty
  map<point, double> g;   // empty

  bool dim_set = false;
  size_t dimension = 0;
  for (map<point, double>::const_iterator i = Xi.begin(); i != Xi.end(); i++) {
    if (!dim_set) {
      dimension = (i->first).size();
      dim_set = true;
    } else if (dimension != (i->first).size()) {
      badArgument exc;
      exc.reason = "Inconformant dimensions in interpolation data.";
      exc.line = __LINE__;
      exc.file = __FILE__;
      throw exc;
    }
    intr.insert(i->first);
  }
  shared_ptr<domain> Omega(new domain(dimension, intr, bdry, nrml));
  shared_ptr<linear_BVP2> bvp(new linear_BVP2(Omega, Id, D, Xi, g));
  init(bvp, param, param_rbf, save);
}

template <typename RBF>
void interpolator<RBF>::interpolate(std::vector<double> &param,
                                    std::vector<RBF> &param_rbf, bool save) {

  init(param, param_rbf, save);
}

template <typename RBF>
void interpolator<RBF>::interpolate(shared_ptr<linear_BVP2> bvp) {
  init(bvp);
}

template <typename RBF>
void interpolator<RBF>::init(shared_ptr<linear_BVP2> bvp,
                             std::vector<double> &param,
                             std::vector<RBF> &param_rbf, bool save) {
  thebvp = bvp;

  using namespace linalg;
  using std::set;

  // da qui
  if (save == true) {

    shared_ptr<const domain> Omega = bvp->get_domain();
    set<point> interior = Omega->get_interior();
    set<point> boundary = Omega->get_boundary();
    map<point, vector> normals = Omega->get_normals();
    n = interior.size();
    m = boundary.size();

    vector temp(n + m);
    coeffs = temp;
    rbfs.reserve(n + m);

    RBF::set_dimension(Omega->get_dimension());

    set<point>::iterator I;
    // Define all the rbfs...
    for (I = interior.begin(); I != interior.end(); I++) {
      RBF r(*I);
      rbfs.push_back(r);
      // std::cout<<""<<r;
    }
    for (I = boundary.begin(); I != boundary.end(); I++) {
      RBF r(*I);
      rbfs.push_back(r);
    }

    param_rbf = rbfs;
    /*
     for(unsigned int g = 0; g<rbfs.size();g++){

     std::cout<<" "<<rbfs[g].getCentre()<<"
     "<<rbfs[g].getDimension()<<std::endl;
     }
     */

    // qui potrei salvare la struttura rbfs

    // qui li collego
    // param_rbf = rbfs;

    // Now define the matrix to be inverted...
    matrix Mtemp(n + m, n + m);
    M = Mtemp;
    shared_ptr<const linear_diff_op2> L = thebvp->get_linear_diff_op2();

    shared_ptr<const bdry_diff_op> B = thebvp->get_bdry_diff_op();

    I = interior.begin();
    for (size_t i = 1; i <= n; i++) {
      for (size_t j = 1; j <= n + m; j++)
        M(i, j) = L->at(rbfs[j - 1], *I);
      I++;
    }

    map<point, vector>::iterator J;
    J = normals.begin();
    for (size_t i = n + 1; i <= n + m; i++) {
      for (size_t j = 1; j <= n + m; j++)
        M(i, j) = B->at(rbfs[j - 1], J->first, J->second);
      J++;
    }

    // a qui non dovrebbe servire se carico da file i coefficineti

    // idem la chiamata qui sotto deve essere divera e caricarli solo da file,
    // senza fare l'inversa della matrice.
    computecoeffs();

    for (size_t i = 1; i <= coeffs.size(); i++) {
      // ok, output funziona
      // std::cout<<coeffs(i)<<";";
      param.push_back((double)coeffs(i));
    }
    //	std::cout<<std::endl;

  } else {

    // carico il vector da file
    for (unsigned int g = 0; g < param_rbf.size(); g++)
      rbfs.push_back(param_rbf[g]);

    //... non devo calcolarli, ma caricarli solo da file
    // std::cout<<"coefficineti caricari da file: ";
    computecoeffs(param);
    /*
     * ok funziona
     * for(size_t i = 1; i <= coeffs.size(); i++) {
     std::cout<<coeffs(i)<<";";
     }
     */

  } // quella dell'IF ... ELSE save ...

  // qui prima di confermare di aver inizializzato, posso salvare
  // coeffs
  // rbfs

  initted = true;
  // qui ho la matrice invertita M (è l'ivertita di Mtemp) e potrei salvarla
  // qui...
}

template <typename RBF>
void interpolator<RBF>::init(std::vector<double> &param,
                             std::vector<RBF> &param_rbf, bool save) {

  using namespace linalg;
  using std::set;
  if (!save) {
    // carico il vector da file
    for (unsigned int g = 0; g < param_rbf.size(); g++)
      rbfs.push_back(param_rbf[g]);

    //... non devo calcolarli, ma caricarli solo da file
    // std::cout<<"coefficineti caricari da file: ";
    computecoeffs(param);
    /*
     * ok funziona
     * for(size_t i = 1; i <= coeffs.size(); i++) {
     std::cout<<coeffs(i)<<";";
     }
     */
  }
  initted = true;
  // qui ho la matrice invertita M (è l'ivertita di Mtemp) e potrei salvarla
  // qui...
}

// qui quelle originali
template <typename RBF>
void interpolator<RBF>::interpolate(const map<point, double> &Xi) {

  if (Xi.empty()) { // Dude, wtf?
    badArgument exc;
    exc.reason = "Cannot interpolate if no data is given.";
    exc.line = __LINE__;
    exc.file = __FILE__;
    throw exc;
  }

  // Create a trivial bvp.
  shared_ptr<Id_op> Id(new Id_op);
  shared_ptr<dirichlet_op> D(new dirichlet_op);
  set<point> intr;
  set<point> bdry;        // empty
  map<point, point> nrml; // empty
  map<point, double> g;   // empty

  bool dim_set = false;
  size_t dimension = 0;
  for (map<point, double>::const_iterator i = Xi.begin(); i != Xi.end(); i++) {
    if (!dim_set) {
      dimension = (i->first).size();
      dim_set = true;
    } else if (dimension != (i->first).size()) {
      badArgument exc;
      exc.reason = "Inconformant dimensions in interpolation data.";
      exc.line = __LINE__;
      exc.file = __FILE__;
      throw exc;
    }
    intr.insert(i->first);
  }
  shared_ptr<domain> Omega(new domain(dimension, intr, bdry, nrml));
  shared_ptr<linear_BVP2> bvp(new linear_BVP2(Omega, Id, D, Xi, g));

  init(bvp);
}

template <typename RBF>
void interpolator<RBF>::init(shared_ptr<linear_BVP2> bvp) {
  thebvp = bvp;

  using namespace linalg;
  using std::set;

  shared_ptr<const domain> Omega = bvp->get_domain();
  set<point> interior = Omega->get_interior();
  set<point> boundary = Omega->get_boundary();
  map<point, vector> normals = Omega->get_normals();
  n = interior.size();
  m = boundary.size();

  vector temp(n + m);
  coeffs = temp;
  rbfs.reserve(n + m);

  RBF::set_dimension(Omega->get_dimension());

  set<point>::iterator I;
  // Define all the rbfs...
  for (I = interior.begin(); I != interior.end(); I++) {
    RBF r(*I);
    rbfs.push_back(r);
  }
  for (I = boundary.begin(); I != boundary.end(); I++) {
    RBF r(*I);
    rbfs.push_back(r);
  }

  // da qui

  // Now define the matrix to be inverted...
  matrix Mtemp(n + m, n + m);
  M = Mtemp;
  shared_ptr<const linear_diff_op2> L = thebvp->get_linear_diff_op2();

  shared_ptr<const bdry_diff_op> B = thebvp->get_bdry_diff_op();

  I = interior.begin();
  for (size_t i = 1; i <= n; i++) {
    for (size_t j = 1; j <= n + m; j++)
      M(i, j) = L->at(rbfs[j - 1], *I);
    I++;
  }

  map<point, vector>::iterator J;
  J = normals.begin();
  for (size_t i = n + 1; i <= n + m; i++) {
    for (size_t j = 1; j <= n + m; j++)
      M(i, j) = B->at(rbfs[j - 1], J->first, J->second);
    J++;
  }

  // a qui non dovrebbe servire se carico da file i coefficineti

  // idem la chiamata qui sotto deve essere divera e caricarli solo da file,
  // senza fare l'inversa della matrice.
  computecoeffs();

  // qui prima di confermare di aver inizializzato, posso salvare
  // coeffs
  // rbfs

  for (size_t i = 1; i <= coeffs.size(); i++) {
    //	cout<<coeffs(i)<<endl;
    // std::cout<<coeffs(i)<<"-";
    // std::cout<<rbfs[i-1]<<" ";
  }

  initted = true;
  // qui ho la matrice invertita M (è l'ivertita di Mtemp) e potrei salvarla
  // qui...
}

/*
 template<typename RBF> void interpolator<RBF>::interpolate(
 const map<point, double>& Xi) {

 if (Xi.empty()) {//Dude, wtf?
 badArgument exc;
 exc.reason = "Cannot interpolate if no data is given.";
 exc.line = __LINE__;
 exc.file =__FILE__;
 throw exc;
 }

 //Create a trivial bvp.
 shared_ptr<Id_op> Id(new Id_op);
 shared_ptr<dirichlet_op> D(new dirichlet_op);
 set<point> intr;
 set<point> bdry; //empty
 map<point, point> nrml; //empty
 map<point, double> g; //empty

 bool dim_set = false;
 size_t dimension = 0;
 for(map<point,double>::const_iterator i = Xi.begin(); i != Xi.end(); i++) {
 if(!dim_set) {
 dimension = (i -> first).size();
 dim_set = true;
 }
 else if(dimension != (i -> first).size()) {
 badArgument exc;
 exc.reason = "Inconformant dimensions in interpolation data.";
 exc.line = __LINE__;
 exc.file = __FILE__;
 throw exc;
 }
 intr.insert( i->first);
 }
 shared_ptr<domain> Omega(new domain(dimension, intr,bdry,nrml));
 shared_ptr<linear_BVP2> bvp(new linear_BVP2(Omega, Id, D, Xi, g));

 init(bvp);
 }

 template<typename RBF>
 void interpolator<RBF>::interpolate(shared_ptr<linear_BVP2> bvp) {
 init(bvp);
 }

 template<typename RBF>
 void interpolator<RBF>::init(shared_ptr<linear_BVP2> bvp)
 {
 thebvp = bvp;

 using namespace linalg;
 using std::set;

 shared_ptr<const domain> Omega = bvp -> get_domain();
 set<point> interior = Omega -> get_interior();
 set<point> boundary = Omega -> get_boundary();
 map<point, vector> normals = Omega -> get_normals();
 n = interior.size();
 m = boundary.size();

 vector temp(n+m);
 coeffs = temp;
 rbfs.reserve(n+m);

 RBF::set_dimension(Omega -> get_dimension());

 set<point>::iterator I;
 //Define all the rbfs...
 for(I = interior.begin(); I != interior.end(); I++) {
 RBF r(*I);
 rbfs.push_back(r);
 }
 for(I = boundary.begin(); I != boundary.end(); I++) {
 RBF r(*I);
 rbfs.push_back(r);

 }

 // da qui

 //Now define the matrix to be inverted...
 matrix Mtemp(n+m,n+m);
 M = Mtemp;
 shared_ptr<const linear_diff_op2> L = thebvp -> get_linear_diff_op2();

 shared_ptr<const bdry_diff_op> B = thebvp -> get_bdry_diff_op();

 I = interior.begin();
 for(size_t i = 1; i <= n; i++) {
 for(size_t j = 1; j <= n+m; j++)
 M(i,j) = L -> at(rbfs[j-1], *I);
 I++;
 }

 map<point, vector>::iterator J;
 J = normals.begin();
 for(size_t i = n+1; i <= n+m; i++) {
 for(size_t j = 1; j <= n+m; j++)
 M(i,j) = B -> at(rbfs[j-1], J->first, J->second);
 J++;
 }

 // a qui non dovrebbe servire se carico da file i coefficineti

 // idem la chiamata qui sotto deve essere divera e caricarli solo da file,
 senza fare
 // l'inversa della matrice.
 computecoeffs();

 // qui prima di confermare di aver inizializzato, posso salvare
 // coeffs
 // rbfs

 for(size_t i = 1; i <= coeffs.size(); i++) {
 //	cout<<coeffs(i)<<endl;
 std::cout<<coeffs(i)<<"-";
 //std::cout<<rbfs[i-1]<<" ";
 }

 std::cout<<std::endl;

 initted = true;
 // qui ho la matrice invertita M (è l'ivertita di Mtemp) e potrei salvarla
 qui...
 }
 */

//***** Evaluations, derivatives *************

template <typename RBF>
double interpolator<RBF>::operator()(const point &p) const {
  return at(p);
}

template <typename RBF> double interpolator<RBF>::at(const point &p) const {
  if (!initted) {
    not_initted(__LINE__, __FILE__);
  }
  std::vector<size_t> alpha; // empty vector
  diff_data loc = std::make_pair(p, alpha);

  // se lo ho già calcolato, allora lo trovo nella remtable ed è inutile
  // ricalcolarlo
  if (remtable.find(loc) != remtable.end())
    return remtable[loc];

  // altriemnti applico il calcolo della RBF
  double result = 0;
  for (size_t i = 1; i <= coeffs.size(); i++) {
    result += coeffs(i) * rbfs[i - 1].at(p);
    // std::cout<<" rbfs:"<<rbfs[i-1].at(p)<<std::endl;
  }

  // e lo savlo nella remtable
  remtable[loc] = result;
  return result;
}

template <typename RBF>
double interpolator<RBF>::d(const point &p, size_t k) const {
  if (!initted) {
    not_initted(__LINE__, __FILE__);
  }
  std::vector<size_t> alpha(k);
  alpha[k - 1]++;
  diff_data loc = std::make_pair(p, alpha);
  if (remtable.find(loc) != remtable.end())
    return remtable[loc];

  double result = 0;
  for (size_t i = 1; i <= coeffs.size(); i++)
    result += coeffs(i) * rbfs[i - 1].d(p, k);

  remtable[loc] = result;
  return result;
}

template <typename RBF>
double interpolator<RBF>::d2(const point &p, size_t k1, size_t k2) const {
  if (!initted) {
    not_initted(__LINE__, __FILE__);
  }
  std::vector<size_t> alpha(k1 > k2 ? k1 : k2);
  alpha[k1 - 1]++;
  alpha[k2 - 1]++;
  diff_data loc = std::make_pair(p, alpha);
  if (remtable.find(loc) != remtable.end())
    return remtable[loc];

  double result = 0;
  for (size_t i = 1; i <= coeffs.size(); i++)
    result += coeffs(i) * rbfs[i - 1].d2(p, k1, k2);
  return result;
}

//********** data manipulation ***************

template <typename RBF> void interpolator<RBF>::set_f(const realfunc &f) {
  if (!initted) {
    not_initted(__LINE__, __FILE__);
  }
  thebvp->set_f(f);
  computecoeffs();
}

template <typename RBF> void interpolator<RBF>::set_g(const realfunc &g) {
  if (!initted) {
    not_initted(__LINE__, __FILE__);
  }
  thebvp->set_g(g);
  computecoeffs();
}

template <typename RBF>
void interpolator<RBF>::set_f(const map<point, double> &f) {
  if (!initted) {
    not_initted(__LINE__, __FILE__);
  }
  thebvp->set_f(f);
  computecoeffs();
}

template <typename RBF>
void interpolator<RBF>::set_g(const map<point, double> &g) {
  if (!initted) {
    not_initted(__LINE__, __FILE__);
  }
  thebvp->set_g(g);
  computecoeffs();
}

template <typename RBF>
void interpolator<RBF>::not_initted(int line, string file) const {
  badArgument exc;
  exc.reason = "Interpolator can't interpolate without initialisation data.";
  exc.line = line;
  exc.file = file;
  throw exc;
}

template <typename RBF>
void interpolator<RBF>::computecoeffs(std::vector<double> &param) {
  using namespace std;

  // Recupero i coefficineti dal parametro questa volta

  linalg::vector tempCoeff(param.size());

  for (size_t i = 1; i <= param.size(); i++) {
    tempCoeff(i) = param[i - 1];
    //	cout<<"tempCoeff: "<<tempCoeff(i)<<endl;
  }

  coeffs = tempCoeff;

  remtable.clear();
}

template <typename RBF> void interpolator<RBF>::computecoeffs() {
  using namespace std;

  linalg::vector rhs(n + m);

  map<point, double>::const_iterator I;

  I = (thebvp->get_f()).begin();
  for (size_t i = 1; i <= n; i++) {
    rhs(i) = I->second;
    I++;
  }
  I = (thebvp->get_g()).begin();
  for (size_t i = n + 1; i <= n + m; i++) {
    rhs(i) = I->second;
    I++;
  }

  coeffs = M.inv(rhs);

  remtable.clear();
}

// Instantiations
using namespace rbf;
template class interpolator<piecewise_polynomial>;
template class interpolator<thin_plate_spline>;
template class interpolator<multiquadric>;
template class interpolator<inverse_multiquadric>;
template class interpolator<inverse_quadratic>;
template class interpolator<gaussian>;
} // namespace bvp
