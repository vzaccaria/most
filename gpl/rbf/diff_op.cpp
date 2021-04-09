#include "error.hpp"
#include "diff_op.hpp"
#include "func.hpp"


namespace bvp{
  using namespace std;
  using namespace linalg;
  using namespace error_handling;
  

  //************** Differential operator functions *********************
  diff_op::diff_op(){
  }

  double diff_op::operator()(const realfunc &f, const point &p) const{
    return at(f,p);
  }
  
  double linear_diff_op2::operator()(const realfunc &f, const point &p) const{
    return diff_op::operator()(f,p);
  }


  double bdry_diff_op::operator()(const realfunc &f, 
				  const point &p) const{
    return at(f,p);
  }
  double bdry_diff_op::operator()(const realfunc &f, 
				  const point &p, const vector &n) const{
    return at(f,p,n);
  }

  double dirichlet_op::at(const realfunc &f, 
			  const point &p) const{
    return f(p);
  }
  double dirichlet_op::at(const realfunc &f, const point &p, 
			  const vector &n) const{
    n.size();
    return f(p);
  }

  double neumann_op::at(const realfunc &f, const point &p, 
			const vector &n) const{
    size_t dim = n.size();
    vector grad(dim);
    for(size_t i = 1; i <= dim; i++)
      grad(i) =f.d(p,i);
    return grad*n/norm(n);
  }

  double Id_op::at(const realfunc &f, const point &p) const{
    return f(p);
  }

  double del1::at(const realfunc &f, const point &p) const{
    return f.d(p,1);
  }

  double del1::at(const realfunc &f, const point &p, size_t i) const{
    return f.d(p,i);
  }

  double del2::at(const realfunc &f, const point &p) const{
    return f.d2(p,1,1);
  }

  double del2::at(const realfunc &f, const point &p, 
		  size_t i, size_t j) const{
    return f.d2(p,i,j);
  }

  double Laplacian::at(const realfunc &f, const point &p) const{
    size_t n = p.size();
    double result = 0;
    del2 D2;
    for (size_t i = 1; i <=n; i++){
      result += D2.at(f, p, i, i);
    }
    return result;
  }
}
