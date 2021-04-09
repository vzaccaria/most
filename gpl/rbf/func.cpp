#include "func.hpp"
#include "error.hpp"
#include <gsl/gsl_deriv.h>
#include <cmath>
#include <limits>

namespace bvp{

  //The static variables...
  double realfunc::eps = 0;
  double realfunc::sqrteps = 0;
  double realfunc::root3eps = 0;
  bool realfunc::initialised = false;

  //Static variables
  point gsl_function_wrapper::x;
  size_t gsl_function_wrapper::index = 1;
  realfunc gsl_function_wrapper::myfunc(0);
  gsl_function* gsl_function_wrapper::f = 0;

  //******************* Wrapper functions *****************************
  gsl_function_wrapper::gsl_function_wrapper( const realfunc &thefunc, 
					      point p, size_t idx){
    myfunc = thefunc;
    x = p;
    index = idx;
    f -> function = &takemyaddress;
    f -> params = 0;
   
  }
  
  void gsl_function_wrapper::set_params(const realfunc &thefunc,
					point p, size_t idx){
    myfunc = thefunc;
    x = p;
    index = idx;
    f -> function = &takemyaddress;
    f -> params = 0;
  }

  gsl_function* gsl_function_wrapper::get_gsl_function() const{
    return f;
  }

  double gsl_function_wrapper::takemyaddress(double xi, void* nothing){
    x(index) = xi;
    nothing = 0;
    return myfunc(x);
  }

  // **************** realfunc functions *********************************
  realfunc::realfunc() : myfunc(0){
    if(!initialised){
      eps = std::numeric_limits<double>::epsilon();
      sqrteps = sqrt(eps);
      root3eps = pow(eps,1/3.0);
      initialised = true;
    }
  }

  realfunc::realfunc( double(*f)(const point&)) : myfunc(f) {
    if(!initialised){
      eps = std::numeric_limits<double>::epsilon();
      sqrteps = sqrt(eps);
      root3eps = pow(eps,1/3.0);
      initialised = true;
    }
  }

  void realfunc::set_function_ptr(double (f_in)(const point &p)){
    myfunc = f_in;
  }

  double realfunc::operator()(const point& p) const{
    return at(p);
  }
  double realfunc::at(const point& p) const{
    if(myfunc == 0){
      no_init(__LINE__,__FILE__);
    }

    return myfunc(p);
  }

  double realfunc::d(const point& p, size_t k) const{
    gsl_function_wrapper gfw(*this,p,k);
    double result,  abserror;
    double x = p(1); 
    double typx = (1 > log(x) ? 1 : log(x));
    double h = sqrteps*( fabs(x) > typx ? fabs(x) : typx);

    gsl_deriv_central(gfw.get_gsl_function(), x, h, &result, &abserror);
    return result;    
  }

  double realfunc::d2(const point& p, size_t k1, size_t k2) const{
    //FIXME
    //Figure this out later.
    k1 = k2;
    p.size();
    return 0;
  }

  void realfunc::no_init(int line, string file) const{
    error_handling::badArgument exc;
    exc.line = line;
    exc.file = file;
    exc.reason = "Did not assign a function pointer to a realfunc object.";
    throw exc;
  }
}
