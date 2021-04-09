#include <gsl/gsl_math.h>
#include <cmath>

#include "rbf.hpp"
#include "linalg.hpp"
#include "error.hpp"

//Radial basis function stuff
namespace rbf {
using namespace linalg;
using namespace error_handling;

size_t radial_basis_function::dimension = 0;
double c_infty_rbf::eps = 1;
size_t piecewise_polynomial::n = 0;
size_t thin_plate_spline::n = 0;

radial_basis_function::radial_basis_function() {
	if (dimension != 0) {
		point zero(dimension); //Centred by default at the origin.
		centre = zero;
	}
}

radial_basis_function::radial_basis_function(const point& c) {
	if (c.size() != dimension)
		bad_dimension(__FILE__, __LINE__, c.size());
		else
		centre = c;
	}

	radial_basis_function::~radial_basis_function() {
		//Nothing to destroy!
	}

	void radial_basis_function::set_centre(const point& c) {
		if(c.size () != dimension)
		bad_dimension(__FILE__, __LINE__, c.size());
		else
		centre = c;
	}

	void radial_basis_function::set_dimension(size_t dim) {
		dimension = dim;
	}

	void radial_basis_function::bad_dimension(string file,
	int line, size_t dim) const {
		error_handling::badDimension exc;
		if(dimension == 0)
		exc.reason =
		"Vector of wrong dimensionality passed to "
		"radial basis function. \n"
		"(Did you forget to set the dimension?)";
		else
		exc.reason =
		"Vector of wrong dimensionality passed to "
		"radial basis function.";

		exc.line = line;
		exc.file = file;
		exc.given_dimension = dim;
		exc.actual_dimension = dimension;
		throw exc;
	}

	double radial_basis_function::at(const point& x) const {
		return operator()(x);
	}
	
	//STE
	point radial_basis_function::getCentre(){
		
		return centre;
	}
	
	//STE
		size_t radial_basis_function::getDimension(){
			
			return dimension;
		}

	double radial_basis_function::operator()(const point& x) const {
		if(x.size() != dimension)
		bad_dimension(__FILE__, __LINE__, x.size());
		return operator()( norm(x - centre) );
	}

	
	double radial_basis_function::d(const point& x, size_t k) const {
		if(x.size() != dimension)
		bad_dimension(__FILE__, __LINE__, x.size());
		else if(k < 1 or k > dimension) {
			error_handling::badArgument exc;
			exc.reason = "Cannot differentiate wrt given index: out of bounds.";
			exc.line = __LINE__;
			exc.file = __FILE__;
			throw exc;
		}
		else if(x == centre)
		return d(0);

		double r = norm(x - centre);
		//Call virtualised derived class function.
		double result = d(r)*(x(k) - centre(k))/r;
		return result;
	}

	double radial_basis_function::d2(const point& x,
	size_t k1, size_t k2) const {
		if(x.size() != dimension)
		bad_dimension(__FILE__, __LINE__, x.size());
		else if(k1 < 1 or k1 > dimension or k2 < 1 or k2 > dimension) {
			error_handling::badArgument exc;
			exc.reason = "Cannot differentiate wrt given indices: out of bounds.";
			exc.line = __LINE__;
			exc.file = __FILE__;
			throw exc;
		}
		else if(x == centre and k1 == k2)
		return d2(0);
		else if(x == centre and k1 != k2)
		return 0;
		else if(k1 != k2) {
			double r = norm(x-centre);
			double r2 = r*r;
			double top = (x(k1) - centre(k1)) * (x(k2)-centre(k2));
			return
			top * d2(r) / r2 -
			top * d(r) / (r2*r);
		}
		double r = norm(x-centre);
		double r2 = r*r;
		double top = x(k1) - centre(k1); top = top*top;
		double result = top*d2(r)/r2 + d(r)/r - top*d(r)/(r2*r);
		return result;

	}

}

//  *****************  Two important subclasses ********************

namespace rbf {

piecewise_smooth_rbf::piecewise_smooth_rbf() {
	//Nothing to create!
}

piecewise_smooth_rbf::~piecewise_smooth_rbf() {
	//Nothing to destroy!
}

c_infty_rbf::c_infty_rbf() {
	//Nothing to create!
}

c_infty_rbf::~c_infty_rbf() {
	//Nothing to destroy!
}

void piecewise_polynomial::set_n(size_t n_new) {
	if (n_new % 2 != 1) {
		badArgument exc;
		exc.reason = "Cannot assign an even n to a piecewise polynomial RBF.";
		exc.line = __LINE__;
		exc.file =__FILE__;
		throw exc;
	}
	piecewise_polynomial::n = n_new;
}

void thin_plate_spline::set_n(size_t n_new) {
	if(n_new % 2 != 0) {
		badArgument exc;
		exc.reason = "Cannot assign an odd n to a thin-plate spline RBF.";
		exc.line = __LINE__;
		exc.file = __FILE__;
		throw exc;
	}
	thin_plate_spline::n = n_new;
}

void c_infty_rbf::set_epsilon(double e) {
	if(e <= 0) {
		badArgument exc;
		exc.reason = "C-infinity RBFs must have a positive epsilon.";
		exc.line = __LINE__;
		exc.file = __FILE__;
		throw exc;
	}
	eps = e;
}
}

//  *****************  Specific RBFs ******************************* 

//Piecewise polynomial
namespace rbf {
double piecewise_polynomial::operator()(double r) const {
	if(n == 0) {
		badArgument exc;
		exc.reason =
		"Parameter n not set for piecewise_polynomial. \n"
		"Use piecewise_polynomial::set_n before evaluating.";
		exc.line = __LINE__;
		exc.file = __FILE__;
		throw exc;
	}
	r = fabs(r);

	if(n == 1)
	return r;
	if(n == 3)
	return gsl_pow_3(r);
	if(n == 5)
	return gsl_pow_5(r);
	if(n == 7)
	return gsl_pow_7(r);
	if(n == 9)
	return gsl_pow_9(r);
	return pow(r,n);
}

double piecewise_polynomial::d(double r) const {
	if(n == 0) {
		badArgument exc;
		exc.reason =
		"Parameter n not set for piecewise_polynomial. \n"
		"Use piecewise_polynomial::set_n before evaluating.";
		exc.line = __LINE__;
		exc.file = __FILE__;
		throw exc;
	}
	r = fabs(r);
	if(n == 1)
	return 1;
	if(n == 3)
	return 3*gsl_pow_2(r);
	if(n == 5)
	return 5*gsl_pow_4(r);
	if(n == 7)
	return 7*gsl_pow_6(r);
	if(n == 9)
	return 9*gsl_pow_8(r);
	return n*pow(r,n-1);
}

double piecewise_polynomial::d2(double r) const {
	if(n == 0) {
		badArgument exc;
		exc.reason =
		"Parameter n not set for piecewise_polynomial. \n"
		"Use piecewise_polynomial::set_n before evaluating.";
		exc.line = __LINE__;
		exc.file = __FILE__;
		throw exc;
	}
	r = fabs(r);
	if(n == 1)
	return 0;
	if(n == 3)
	return 6*r;
	if(n == 5)
	return 20*gsl_pow_3(r);
	if(n == 7)
	return 42*pow(r,5);
	if(n == 9)
	return 72*gsl_pow_7(r);
	return n*(n-1)*pow(r,n-2);
}
}

//Thin-plate spline
namespace rbf {
double thin_plate_spline::operator()(double r) const {
	if(n == 0) {
		badArgument exc;
		exc.reason =
		"Parameter n not set for thin_plate_spline. \n"
		"Use thin_plate_spline::set_n before evaluating.";
		exc.line = __LINE__;
		exc.file = __FILE__;
		throw exc;
	}
	r = fabs(r);
	if(r == 0)
	return 0;
	if(n == 2)
	return gsl_pow_2(r)*log(r);
	if(n == 4)
	return gsl_pow_4(r)*log(r);
	if(n == 6)
	return gsl_pow_6(r)*log(r);
	if(n == 8)
	return gsl_pow_8(r)*log(r);

	return pow(r,n)*log(r);
}

double thin_plate_spline::d(double r) const {
	if(n == 0) {
		badArgument exc;
		exc.reason =
		"Parameter n not set for thin_plate_spline. \n"
		"Use thin_plate_spline::set_n before evaluating.";
		exc.line = __LINE__;
		exc.file = __FILE__;
		throw exc;
	}
	r = fabs(r);
	if(r == 0)
	return 0;
	if(n == 2)
	return r*(2*log(r) + 1);
	if(n == 4)
	return gsl_pow_3(r)*(4*log(r) + 1);
	if(n == 6)
	return gsl_pow_5(r)*(6*log(r) + 1);
	if(n == 8)
	return gsl_pow_7(r)*(8*log(r) + 1);

	return pow(r,n-1)*(n*log(r) + 1);
}

double thin_plate_spline::d2(double r) const {
	if(n == 0) {
		badArgument exc;
		exc.reason =
		"Parameter n not set for thin_plate_spline. \n"
		"Use thin_plate_spline::set_n before evaluating.";
		exc.line = __LINE__;
		exc.file = __FILE__;
		throw exc;
	}
	if (n == 2 and r == 0) {
		badDomain exc;
		exc.reason =
		"For n = 2, thin-plate splines do no have a derivative at zero.";
		exc.line = __LINE__;
		exc.file = __FILE__;
		throw exc;
	}
	if(r == 0)
	return 0;

	if(n == 2)
	return 2*log(r) + 3;
	if(n == 4)
	return gsl_pow_2(r)*(12*log(r) + 7);
	if(n == 6)
	return gsl_pow_4(r)*(30*log(r) + 11);
	if(n == 8)
	return gsl_pow_6(r)*(56*log(r) + 15);

	return pow(r,n-2)*((n*n - n)*log(r) + 2*n - 1);
}
}

//Multiquadric
namespace rbf {
double multiquadric::operator()(double r) const {
	return sqrt(1 + gsl_pow_2(eps*r));
}

double multiquadric::d(double r)const {
	return eps*eps*fabs(r)/sqrt(1 + gsl_pow_2(eps*r));
}

double multiquadric::d2(double r)const {
	double temp = sqrt(1 + gsl_pow_2(eps*r));
	return
	eps*eps/temp - gsl_pow_4(eps)*r*r/gsl_pow_3(temp);
}
}

//Inverse multiquadric
namespace rbf {
double inverse_multiquadric::operator()(double r) const {
	return 1/sqrt(1 + gsl_pow_2(eps*r));
}

double inverse_multiquadric::d(double r) const {
	return -eps*eps*fabs(r)/gsl_pow_3(sqrt(1 + gsl_pow_2(eps*r)));
}

double inverse_multiquadric::d2(double r) const {
	double temp = sqrt(1 + gsl_pow_2(eps*r));
	return
	- eps*eps/gsl_pow_3(temp) + 3*gsl_pow_4(eps)*r*r/gsl_pow_5(temp);
}
}

//Inverse quadratic
namespace rbf {
double inverse_quadratic::operator()(double r) const {
	return 1/(1 + gsl_pow_2(eps*r));
}

double inverse_quadratic::d(double r)const {
	return -2*eps*eps*fabs(r)/gsl_pow_2(1 + gsl_pow_2(eps*r));
}

double inverse_quadratic::d2(double r)const {
	double temp = 1 + gsl_pow_2(eps*r);
	double temp2 = temp*temp, temp3 = temp2*temp;
	return
	8*gsl_pow_2(eps*eps*r)/temp3 - 2*eps*eps/temp2;

}
}

//Gaussian
namespace rbf {
double gaussian::operator()(double r) const {
	return exp(-gsl_pow_2(eps*r));
}

double gaussian::d(double r) const {
	return -2*eps*eps*fabs(r) * exp(-gsl_pow_2(eps*r));
}

double gaussian::d2(double r) const {
	double temp = exp(-gsl_pow_2(eps*r));
	return eps*eps*(4*gsl_pow_2(eps*r) - 2) * temp;

}
}
