/*!\file linalg.hpp
 * \brief Wrapper linear algebra classes for the GSL.
 *
 * This header file puts some C++ wrappers around the GSL vector and
 * matrix structures plus providing a C++ interface to some pertinent
 * BLAS and general linear algebra routines.
 */

#ifndef __LINALG_H__
#define __LINALG_H__

#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_permutation.h>
#include "error.hpp"

/// Linear algebra namespace
namespace linalg{
  using namespace error_handling;

  class slice; 
  class vector;
  class vector_view;

  /// A wrapper class for GNU Scientific Library matrices
  class matrix{
  private:
    class LUmatrix;

  public:
    //Constructors

    /// Allocate 1x1 matrix zero matrix.
    matrix(); 
    /*! \brief Allocate m by n matrix, filled with some value.
      \param m - Number of rows.
      \param n - Number of columns.
      \param fillvalue - Value to fill matrix with, default is 0.
     */
    matrix(const size_t m, const size_t n, const double fillvalue = 0) ;
    /// Assign a matrix from a GSL matrix pointer.
    matrix(gsl_matrix *M);

    /// Copy constructor
    matrix(const matrix& M);
    /// Assignment.
    matrix operator=(const matrix& M);
    
    //Destructors
    
    /// Clears the memory allocated by the GSL
    ~matrix();

    /// Number of decimal digits to output.
    size_t precision() const;
    /// Set the number of decimal digits to output.
    static void set_precision(size_t p);

    /// Number of rows.
    size_t rows() const;
    /// Number of columns.
    size_t cols() const;

    /*! \brief Fortran-style parenthetical indexing (hence Octave-style too)
     *  Indices start from 1.
     *  @param i - Row number.
     *  @param j - Column number.
     *  @return A reference to the matrix element.
     */
    double& operator()(const size_t i, const size_t j);
    
    /// Constant version of previous function.
    const double& operator()(const size_t i, const size_t j) const;

    /*! \brief Indexing for vectorisation.
     *  The GSL provides limited vectorisation routines which can be
     *  useful for operating on blocks of matrices all at once.
     *  @param i - Row to be sliced.
     *  @param b - Slice range.
     *  @return A vector_view of the sliced row.
     */
    vector_view operator()(const size_t  i, const slice &b);
    /// Constant version of previous function.
    const vector_view operator()(const size_t  i, const slice &b) const;

    /*! \brief Indexing for vectorisation.
     *  The GSL provides limited vectorisation routines which can be
     *  useful for operating on blocks of matrices all at once.
     *  @param a - Slice range
     *  @param j - column to be sliced.
     *  @return A vector_view of the sliced column.
     */
    vector_view operator()(const slice &a, const size_t  j);
    /// Constant version of previous function.
    const vector_view operator()(const slice &a, const size_t  j) const;

    //Arithmetic operations
    ///Scale the matrix.
    matrix operator*(const double  a) const;
    /// Matrix addition
    matrix operator+(const matrix& N) const;
    /// Matrix multiplication
    matrix operator*(const matrix& N) const;
    /// Matrix subtraction
    matrix operator-(const matrix& N) const;
    /// Matrix vector product (gaxpy operation).
    vector operator*(const vector& v) const; //Mv, where v is treated
					     //as a column vector. 

    //More complex operations.

    /*! \brief Inverse.
     *
     * This function returns the inverse matrix, computed via an LU
     * factorisation. If the matrix is ill-conditioned or the inverse
     * does not exist, it will happily return a matrix full of NaN.
     * \returns The inverse matrix.
     */
    matrix inv()   const;    
    /*! \brief Tranpose.
     * Returns a copy of the original matrix, transposed. The original
     * matrix is not modified.
     * \returns Transposed copy of matrix.
     */
    matrix T()     const;    
    /*! \brief Trace.
     * The trace of a square matrix.
     * \returns The trace.
     */
    double tr()    const;    
    /*! \brief Determinant.
     * The determinant computed via an LU factorisation
     * \returns The determinant.
     */
    double det()   const;    

    /*! \brief Solves Mv = w for v with LU factorisation. 
     * Given another vector, will return the solution vector v to the
     * equation Mv = w.
     * \param w - Another vector, must have the same size as number of
     * rows in the matrix.
     * \returns The solution v to the equation Mv = w.
    */
    vector inv(const vector& w) const; 

    /*! \brief L2 condition number, using SVD.
     *
     * This function computes the L2 condition number of a matrix, the
     * largest singular value divided by the smallest. It directly
     * uses the SVD decomposition (as provided by the GSL), and it may
     * take a long time for large matrices.
     * \returns The L2 condition number.
     */
    double cond() const; 

    friend class vector_view;
  private:
    /*! \brief LU decomposition, pivots in U.
     *
     * This function will compute the LU factorisation of a matrix,
     * where the pivots are in the upper triangular matrix U and L has
     * ones along the diagonal (not stored). For efficiency, L and U
     * are both internally stored within a single matrix.
     * \returns A pointer to the private class LUmatrix.
     */ 
    LUmatrix* LU() const;    

    gsl_matrix * A;
    static size_t precsn;

    // Internal members, all can change, none of them affect the
    // actual matrix per se.
    mutable LUmatrix* LUptr;
    mutable bool LUfactored;

    mutable double condition_number;  //L2 condition number, obtained by svd.
    mutable gsl_vector* SVD;          //Matrix's singular values.
    mutable bool SVDfactored;

    void SVDfactor() const;

    /// A private matrix member class for matrices factored in LU form.
    class LUmatrix{ 
    public:
      gsl_matrix* matrix_ptr();
      gsl_permutation* perm_ptr();
      int sgn();
      int* sgn_ptr();

      LUmatrix(gsl_matrix* M);
      LUmatrix(const LUmatrix& LU);
      LUmatrix operator=(const LUmatrix& LU);
      ~LUmatrix();
    private: 
      // Private interface to the GSL.
      gsl_permutation* p;
      gsl_matrix* A;
      int signum;
      LUmatrix();
    };//End LUmatrix
  }; //End matrix

  /// A wrapper class for GSL vectors
  class vector{
  public: 
    //Constructors

    ///Allocate zero vector of size one.
    vector(); 
    /*! \brief Allocate GSL vector of size n, filled with fillvalue
     * \param n - Size of new vector
     * \param fillvalue - Value for filling vector, default is 0.
     */
    vector(const size_t n, const double fillvalue = 0);
    /// Associate a  vector to a GSL vector pointer.
    vector(gsl_vector *y);
    /// Associate a  vector to a constant GSL vector pointer.
    vector(const gsl_vector *y);

    /// Copy contstructor.
    vector(const vector& y);
    /// Assignment.
    vector& operator=(const vector &y);

    //Destructor
    /// Clear memory assigned to vector by GSL.
    ~vector();

    //Number of decimal digits to output.
    /// Number of decimal digits to output
    size_t precision() const;
    /// Set the number of decimal digits to output
    static void set_precision(size_t p);

    ///Number of elements
    size_t size() const;

    /*! \brief Fortran-style parenthetical indexing (hence
     *   Octave-style too). Indices start at 1.
     * \param i - Index number.
     * \return A reference to the vector element.
     */
    double& operator()(const size_t i);

    /// Constant version of previous function.
    const double& operator()(const size_t i) const;
  
    /*! \brief Indexing for vectorisation.
     *  The GSL provides limited vectorisation routines which can be
     *  useful for operating on blocks of vectors all at once.
     *  @param v - Slice range.
     *  @return A vector_view of the slice.
     */
    vector_view operator()(const slice &v);
    ///Constant version of previous function.
    const vector_view operator()(const slice &v) const;

    //Arithmetic operations:
    ///Scale the vector.
    vector operator*(const double a)  const; 
    ///Vector addition.
    vector operator+(const vector& w) const;
    ///Vector subtration.
    vector operator-(const vector& w) const;
    ///xDot product.
    double operator*(const vector& w) const; 
    ///Computes vM where v is treated as a row vector.
    vector operator*(const matrix& M) const; 

    //Comparison
    
    /// Compares vectors elementwise, using machine epsilon.
    bool operator==(const vector& w) const;
    /*! \brief Lexicographical order, used for putting into STL sets or
     * maps.
     *
     * Lexicographical ordering of vectors. Vectors of smaller
     * dimension are smaller.
     */
    bool operator<(const vector& w) const; 

    //More complex operations
   ///Euclidean norm.
    double norm() const; 
    
    friend class vector_view;
    friend class matrix;
  private:
    /// Pointer to associated GSL vector.
    gsl_vector * x;
    /// Output precision.
    static size_t precsn;
  };


  /*! \brief  A vector that doesn't own its data; rather, points to data owned
   * by another vector. 
   */
  class vector_view : public vector{
    friend class vector;
    friend class matrix;
  public:
    ~vector_view();
    /// Assignment to a vector; will share same data in memory as that
    /// other vector.
    vector_view& operator=(const vector& w);
    /// Assignment, useful for modifying chunks of vectors all at once.
    vector_view& operator=(const vector_view& w);
  private:
    /// vector_views are invisible to the user and can only
    /// be created using slices and matrix or vector indexing.
    vector_view();
    /// See above.
    vector_view(gsl_vector* y, const slice& s);
    /// See above.
    vector_view(gsl_matrix* A, const slice& a, const size_t j);
    /// See above.
    vector_view(gsl_matrix* A, const size_t i, const slice& b);
  };

  
  /*! \brief Vector slices corresponding to GNU Octave ranges.
   *
   * Vector slices are useful for certain vectorisation routines and
   * for referring to rows, columns, or equally spaced chunks of a
   * vector or a matrix. Set a begininning and an end for the
   * stride, and a step size, and this will be the elements that can
   * be indexed by this slice in a vector or matrix. E.g. if the
   * beginning is 2, the end 8, and the stride is 2, then this
   * refers to the slice [2, 4, 6, 8]. The default stride is 1.
   *
   * Unlike GNU Octave slices, there is no shorthand for referring
   * to the entirety of a column or row of a matrix. Must instead
   * create a slice for that purpose as s(1,n) where n is the number
   * of rows or columns respectively.
   */
  class slice{
  public:
    /*! \brief Create a vector slice.
     * 
     * \param a - Beginning of the slice.
     * \param b - End of the slice.
     * \param k - Stride (default is 1).
     */
    slice(size_t a, size_t b, size_t k=1); 

    /*! For setting the slice parameters anew.
     *
     * Indices start from 1.
     * \param a - Beginning of the slice.
     * \param b - End of the slice.
     * \param k - Stride (default is 1).
     */
    slice operator()(size_t a, size_t b, size_t k=1);
    slice set(size_t a, size_t b, size_t k=1) ;

    size_t begin()const {return beg;};
    size_t end() const{return fin;};
    size_t stride() const{return str;};

  private:
    /// Empty and private default constructor.
    slice(){};
    size_t beg,fin;
    size_t str;    
  };
  
  /// Useful alias, vectors are also points in space.
  typedef vector point; //Useful alias.
}

namespace linalg{ //Non-member functions.
  
  // I/O

  ///Stream insertion operator
  std::ostream& operator<<(std::ostream& os, const vector &v);
  ///Stream extraction operator
  vector operator>>(std::istream& is, vector& v);
  ///Stream insertion operator
  std::ostream& operator<<(std::ostream& os, const matrix &M);
  ///Stream extraction operator
  matrix operator>>(std::istream& is, matrix& v);

  //Some arithmetic functions for comfortable syntax.

  /// Scale a vector.
  vector operator*(double a, const vector& v);
  /// Euclidean norm of a vector.
  double norm(const vector& v);
  /// Scale a matrix.
  matrix operator*(double a, const matrix& M);
  /// Matrix inverse, computed with LU factorisation.
  matrix inv(const matrix& A);
  /// Return copy of transposed matrix.
  matrix T(const matrix& A);          
  /// Trace.
  double tr(const matrix& A);
  /// Determinant.
  double det(matrix& A);        
  /// L2 condition number, computed with SVD.
  double cond(matrix& A);

}

//Inlined functions
namespace linalg{
  inline double& matrix::operator()(const size_t i, const size_t j){
    try{
      if(LUfactored)
	delete LUptr;
      if(SVDfactored)
	gsl_vector_free(SVD);
  
      LUfactored = false;
      SVDfactored = false;
      return(*gsl_matrix_ptr(A,i-1,j-1));
    }
    catch(indexOutOfRange& exc){
      exc.i = i;
      exc.j = j;
      exc.m = A -> size1;
      exc.n = A -> size2;
      throw exc;
    }
  }
  inline const double& matrix::operator()(const size_t i,  
					 const size_t j) const{
    try{
      return(*gsl_matrix_const_ptr(A,i-1,j-1));
    }
    catch(indexOutOfRange& exc){
      exc.i = i;
      exc.j = j;
      exc.m = A -> size1;
      exc.n = A -> size2;
      throw exc;
    }
  }

  inline double& vector::operator()(const size_t i) {
    try{
      return *gsl_vector_ptr(x,i-1);
    }
    catch(indexOutOfRange& exc){
      exc.i = i;
      exc.n = x -> size;
      throw exc;
    }
  }

  inline const double& vector::operator()(const size_t i) const {
    try{
      return *gsl_vector_const_ptr(x,i-1);
    }
    catch(indexOutOfRange& exc){
      exc.i = i;
      exc.n = x -> size;
      throw exc;
    }
  }
}
#endif //__LINALG_H__
