/*! \file  rbf.hpp
 * \brief Radial basis functions declared here.
 */
#ifndef __RBF_H__
#define __RBF_H__

#include "linalg.hpp"
#include "error.hpp"
#include "func.hpp"

namespace error_handling {
///An exception struct for RBFs when attempting to use the wrong dimension.
struct badDimension : public badArgument {
	badDimension() {
	}
	;
	badDimension(string r, string f, int l) :
		badArgument(r, f, l) {
	}
	;
	size_t actual_dimension;
	size_t given_dimension;
};
}

/// Radial basis function namespace
namespace rbf {
using namespace linalg;

///Base abstract class.
class radial_basis_function : public bvp::realfunc {


public:
	/// Default constructor, centred at zero by default. 
	radial_basis_function();
	virtual ~radial_basis_function();
	//~radial_basis_function();

	///Specify the dimension of all rbf's.
	static void set_dimension(size_t dim);

	///Specify the point where this rbf is centred.
	radial_basis_function(const point& c);
	///Specify the point where this rbf is centred.
	void set_centre(const point& c);

	///Evaluate rbf at x.  
	double at(const point& x) const;
	///Evaluate rbf at x.  
	double operator()(const point& x) const;

	///First partial derivative at x in direction k;
	double d(const point& x, size_t k) const;

	///Second derivative at x in directions k1 and k2.
	double d2(const point& x, size_t k1, size_t k2) const;
	
	//STE
	point getCentre();
	size_t getDimension();
	

private:

	///In case we attempt to give this rbf an argument of the wrong
	///dimension.
	void bad_dimension(string file, int line, size_t dim) const;

	///The scalar functions defining each RBF.
	//double operator()(double r) const;
	virtual double operator()(double r) const = 0;
	///The scalar first derivative defining each RBF.
	//double d(double r) const;
	virtual double d(double r) const = 0;
	///The scalar second derivative defining each RBF.
	//double d2(double r) const;
	virtual double d2(double r) const = 0;

	///Point on which this rbf is centred.
	point centre;
	///Dimensionality of all rbfs.
	static size_t dimension;
};

}

//Two important subclasses
namespace rbf {
/// Piecewise smooth RBFs
class piecewise_smooth_rbf : public radial_basis_function {
public:
	piecewise_smooth_rbf();
	piecewise_smooth_rbf(const point &c) :
		radial_basis_function(c) {
		;
	}
	;
	virtual ~piecewise_smooth_rbf();
};

/// C-infty RBFs
class c_infty_rbf : public radial_basis_function {
public:
	c_infty_rbf();
	c_infty_rbf(const point &c) :
		radial_basis_function(c) {
		;
	}
	;

	virtual ~c_infty_rbf();

	static void set_epsilon(double e);
protected:
	///Shape parameter
	static double eps;
};
}

//Specific rbf's follow...
namespace rbf {

/// r^n with n odd
class piecewise_polynomial : public piecewise_smooth_rbf {
public:
	static void set_n(size_t new_n);

public:
	piecewise_polynomial() {
	}
	;
	piecewise_polynomial(const point& c) :
		piecewise_smooth_rbf(c) {
		;
	}
	;

	double at(const point& x) const {
		return radial_basis_function::at(x);
	}
	;
	double operator()(const point& x) const {
		return radial_basis_function::operator()(x);};
	double d(const point& x, size_t k) const
	{	return radial_basis_function::d(x,k);};
	double d2(const point& x, size_t k1, size_t k2) const
	{	return radial_basis_function::d2(x,k1,k2);};
private:
	///Shape parameter
	static size_t n;
	double operator()(double r) const;
	double d(double r) const;
	double d2(double r) const;
};

///a common synonym.
typedef piecewise_polynomial conical;
}

namespace rbf {
/// r^n log(r) with n even
class thin_plate_spline : public piecewise_smooth_rbf {
public:
	static void set_n(size_t new_n);

public:
	thin_plate_spline() {
	}
	;
	thin_plate_spline(const point& c) :
		piecewise_smooth_rbf(c) {
		;
	}
	;

	double at(const point& x) const {
		return radial_basis_function::at(x);
	}
	;
	double operator()(const point& x) const {
		return radial_basis_function::operator()(x);};
	double d(const point& x, size_t k) const
	{	return radial_basis_function::d(x,k);};
	double d2(const point& x, size_t k1, size_t k2) const
	{	return radial_basis_function::d2(x,k1,k2);};
private:
	///Shape parameter
	static size_t n;
	double operator()(double r) const;
	double d(double r) const;
	double d2(double r) const;
};
}

namespace rbf {
/// sqrt(1+(eps*r)^2) with eps > 0
class multiquadric : public c_infty_rbf {
public:
	multiquadric() {
	}
	;
	multiquadric(const point& c) :
		c_infty_rbf(c) {
		;
	}
	;

	double at(const point& x) const {
		return radial_basis_function::at(x);
	}
	;
	double operator()(const point& x) const {
		return radial_basis_function::operator()(x);};
	double d(const point& x, size_t k) const
	{	return radial_basis_function::d(x,k);};
	double d2(const point& x, size_t k1, size_t k2) const
	{	return radial_basis_function::d2(x,k1,k2);};
private:
	double operator()(double r) const;
	double d(double r) const;
	double d2(double r) const;
};
}

namespace rbf {
/// 1/sqrt(1 + (eps*r)^2) with eps > 0
class inverse_multiquadric : public c_infty_rbf {
public:
	inverse_multiquadric() {
	}
	;
	inverse_multiquadric(const point& c) :
		c_infty_rbf(c) {
		;
	}
	;

	double at(const point& x) const {
		return radial_basis_function::at(x);
	}
	;
	double operator()(const point& x) const {
		return radial_basis_function::operator()(x);};
	double d(const point& x, size_t k) const
	{	return radial_basis_function::d(x,k);};
	double d2(const point& x, size_t k1, size_t k2) const
	{	return radial_basis_function::d2(x,k1,k2);};
private:
	double operator()(double r) const;
	double d(double r) const;
	double d2(double r) const;
};
}

namespace rbf {
/// 1/(1+(eps*r)^2) with eps > 0
class inverse_quadratic : public c_infty_rbf {
public:
	inverse_quadratic() {
	}
	;
	inverse_quadratic(const point& c) :
		c_infty_rbf(c) {
		;
	}
	;

	double at(const point& x) const {
		return radial_basis_function::at(x);
	}
	;
	double operator()(const point& x) const {
		return radial_basis_function::operator()(x);};
	double d(const point& x, size_t k) const
	{	return radial_basis_function::d(x,k);};
	double d2(const point& x, size_t k1, size_t k2) const
	{	return radial_basis_function::d2(x,k1,k2);};
private:
	double operator()(double r) const;
	double d(double r) const;
	double d2(double r) const;
};
}

namespace rbf {
/// exp(- (eps*r)^2) with eps > 0.
class gaussian : public c_infty_rbf {
public:
	gaussian() {
	}
	;
	gaussian(const point& c) :
		c_infty_rbf(c) {
		;
	}
	;

	double at(const point& x) const {
		return radial_basis_function::at(x);
	}
	;
	double operator()(const point& x) const {
		return radial_basis_function::operator()(x);};
	double d(const point& x, size_t k) const
	{	return radial_basis_function::d(x,k);};
	double d2(const point& x, size_t k1, size_t k2) const
	{	return radial_basis_function::d2(x,k1,k2);};
private:
	double operator()(double r) const;
	double d(double r) const;
	double d2(double r) const;
};
}

#endif // __RBF_H__ 
