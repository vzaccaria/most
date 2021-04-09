#include "linalg.hpp"
#include "utils.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <list>
#include <limits> //For machine epsilon
#include <cmath>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_math.h> //For floating-point comparison.

//Matrix stuff
namespace linalg{
  using namespace error_handling;

  // *************** Matrix allocation stuff ***************************
  matrix::matrix(){
    A = gsl_matrix_calloc(1,1); //Allocate 1x1 matrix zero matrix.    
    LUfactored = false;
    SVDfactored = false;
  }

  matrix::matrix(size_t m, size_t n, double fillvalue){
    A = gsl_matrix_alloc(m,n);
    gsl_matrix_set_all(A,fillvalue);
    LUfactored = false;
    SVDfactored = false;
  }

  matrix::matrix(gsl_matrix *M){
    A = M;
    LUfactored = false;
    SVDfactored = false;
  }

  matrix::matrix(const matrix& M){
    A = gsl_matrix_alloc(M.A->size1, M.A->size2);
    gsl_matrix_memcpy(A,M.A);

    LUfactored = M.LUfactored;
    if(LUfactored)
      LUptr = new LUmatrix(*M.LUptr);
    
    condition_number = M.condition_number;

    SVDfactored = M.SVDfactored;
    if(SVDfactored){
      SVD = gsl_vector_alloc(M.SVD->size);
      gsl_vector_memcpy(SVD,M.SVD);
    }       
  }

  matrix matrix::operator=(const matrix& M){
    if(this != &M){
      gsl_matrix_free(A);
      A = gsl_matrix_alloc(M.A->size1, M.A->size2);
      gsl_matrix_memcpy(A,M.A);

      if(LUfactored)
	delete LUptr;

      LUfactored = M.LUfactored;
      if(LUfactored){
	LUptr = new LUmatrix(*M.LUptr);
      }
      
      if(SVDfactored)
	gsl_vector_free(SVD);

      SVDfactored = M.SVDfactored;
      if(SVDfactored){
	condition_number = M.condition_number;
	SVD = gsl_vector_alloc(M.SVD->size);
	gsl_vector_memcpy(SVD,M.SVD);
      }       
    }
    return *this;
  }

  matrix::~matrix(){
    if(A != 0) //Has subclass already deleted this matrix?
      gsl_matrix_free(A);
    if(LUfactored)
      delete LUptr;
    if(SVDfactored)
      gsl_vector_free(SVD);
  }


  // *********** Matrix accessor stuff *****************************
  size_t matrix::precsn = 4;
  
  size_t matrix::precision() const{
    return precsn;
  }

  void matrix::set_precision(size_t p){
    precsn = p;
  }

  size_t matrix::rows() const{
    return A -> size1;
  }

  size_t matrix::cols() const{
    return A -> size2;
  }
    
  
  

  
  vector_view matrix::operator()(const size_t  i, const slice &b){
    vector_view x_sub(A,i,b);

    if(LUfactored)
      delete LUptr;
    if(SVDfactored)
      gsl_vector_free(SVD);

    LUfactored = false;
    SVDfactored = false;
    return x_sub;
  }
  const vector_view matrix::operator()(const size_t  i, const slice &b) const{
    vector_view x_sub(A,i,b);
    return x_sub;
  }
  
  vector_view matrix::operator()(const slice &a, const size_t j){
    vector_view x_sub(A,a,j);

    if(LUfactored)
      delete LUptr;
    if(SVDfactored)
      gsl_vector_free(SVD);
    
    LUfactored = false;
    SVDfactored = false;
    return x_sub;
  }
  const vector_view matrix::operator()(const slice &a, const size_t j) const{
    vector_view x_sub(A,a,j);
    return x_sub;
  }

  // ******************* Matrix arithmetic stuff ******************
  matrix matrix::operator*(const double  a) const{
    matrix Z = *this;
    gsl_matrix_scale(Z.A,a);
    return Z;
  }

  matrix matrix::operator+(const matrix& N) const{
    matrix Z = *this;
    try{
      gsl_matrix_add(Z.A,N.A);
    }
    catch(inconformantSizes& exc){
      exc.n_A = A->size1;
      exc.m_A = A->size2;
      exc.n_B = N.A -> size1;
      exc.m_B = N.A -> size2;
      throw exc;
    }
    return Z;
  }

  matrix matrix::operator*(const matrix& N)const{
    matrix Z(A->size1,N.A->size2);
    try{
      gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1,  A, N.A, 0, Z.A);
    }
    catch(inconformantSizes& exc){
      exc.n_A = A->size1;
      exc.m_A = A->size2;
      exc.n_B = N.A -> size1;
      exc.m_B = N.A -> size2;
      throw exc;
    }
    return Z;
  }

  matrix matrix::operator-(const matrix& N) const{
    return *this + (-1)*N;
  }

  vector matrix::operator*(const vector& v) const{
    vector w(A -> size1);
    try{
      gsl_blas_dgemv(CblasNoTrans, 1, A, v.x, 0, w.x);
    }
    catch(inconformantSizes& exc){
      exc.n_A = A->size1;
      exc.m_A = A->size2;
      exc.n_B = v.x -> size;
      exc.m_B = 1;
      throw exc;
    }
    return w;
  }

  // ***************** Other arithmetic functions ******************


  matrix::LUmatrix* matrix::LU() const{
    if(A -> size1 != A -> size2){
      matrixNotSquare exc;
      exc.reason = "Cannot LU-factorise non-square matrices.";
      exc.file = __FILE__;
      exc.line = __LINE__;
      throw exc;
    }

    if(!LUfactored){
      LUptr = new LUmatrix(A);
      gsl_linalg_LU_decomp(LUptr->matrix_ptr(), LUptr->perm_ptr(), 
			   LUptr->sgn_ptr());
      LUfactored = true;
    }
    return LUptr;    
  }

  matrix matrix::inv() const{
    if(A -> size1 != A -> size2){
      matrixNotSquare exc;
      exc.reason = "Cannot invert non-square matrices.";
      exc.file = __FILE__;
      exc.line = __LINE__;
      throw exc;
    }
    
    if(!LUfactored){
      LUptr =  LU();    
      LUfactored = true;
    }
    
    matrix Z(LUptr->matrix_ptr()->size1, LUptr->matrix_ptr() -> size2);

    gsl_linalg_LU_invert(LUptr->matrix_ptr(), LUptr->perm_ptr(), Z.A);
    return Z;
  }

  matrix matrix::T()  const{
    matrix Z(A->size2, A->size1);
    for(size_t i = 1; i <= A->size1; i++)
      for(size_t j = 1; j <= A->size2; j++)
	Z(j,i) = gsl_matrix_get(A,i-1,j-1);
    return Z;
  }
  
  double matrix::tr() const{
    if(A -> size1 != A -> size2){
      matrixNotSquare exc;
      exc.reason = "Cannot find trace of non-square matrix.";
      exc.file = __FILE__;
      exc.line = __LINE__;
      throw exc;
    }

    double result=0;
    for(size_t i = 1; i <= A->size1; i++)
      result += gsl_matrix_get(A,i,i);
    return result;
  }

  double matrix::det() const{
    if(A -> size1 != A -> size2){
      matrixNotSquare exc;
      exc.reason = "Cannot find determinant of non-square matrices.";
      exc.file = __FILE__;
      exc.line = __LINE__;
      throw exc;
    }
    
    if( !LUfactored ){
      LUptr = LU();
      LUfactored = true;
    }

    double out = gsl_linalg_LU_det(LUptr->matrix_ptr(),LUptr->sgn());
    return out;
  }

  vector matrix::inv(const vector& w) const{
    if(A -> size1 != A -> size2){
      matrixNotSquare exc;
      exc.reason = "Cannot invert non-square matrices.";
      exc.file = __FILE__;
      exc.line = __LINE__;
      throw exc;
    }

    vector v(w.size());
    if( !LUfactored){
      LUptr = LU();
      LUfactored = true;
    }

    gsl_linalg_LU_solve(LUptr->matrix_ptr(), LUptr->perm_ptr(), w.x, v.x);
    return v;
  }
 
  double matrix::cond() const{
    if(SVDfactored)
      return condition_number;

    SVDfactor();
    condition_number =  gsl_vector_get(SVD,0) 
                        /gsl_vector_get(SVD,std::max(A->size1, A->size2)-1);
    return condition_number;
  }

  void matrix::SVDfactor() const{
    if(SVDfactored)
      return;

    gsl_matrix * M;
    if(A -> size1 < A -> size2){ //Transpose so that m>=n
      M = gsl_matrix_alloc(A -> size2, A -> size1);
      gsl_matrix_transpose_memcpy(M,A);
    }
    else{
      M = gsl_matrix_alloc(A -> size1, A -> size2);
      gsl_matrix_memcpy(M,A);
    }
    
    SVD = gsl_vector_alloc(M -> size2);
    gsl_matrix* V = gsl_matrix_alloc(M -> size2, M -> size2);
    gsl_vector* work = gsl_vector_alloc(M -> size2);
    gsl_linalg_SV_decomp(M,V,SVD,work);
    SVDfactored = true;
    gsl_vector_free(work);
    gsl_matrix_free(V);
    gsl_matrix_free(M);
  }
}



//***************** LUmatrix stuff **********************************
namespace linalg{
  matrix::LUmatrix::LUmatrix(gsl_matrix* M){
    A = gsl_matrix_alloc(M -> size1, M -> size2);
    gsl_matrix_memcpy(A,M);
    p = gsl_permutation_alloc(A->size1);
    signum = 0;
  }
  
  matrix::LUmatrix::LUmatrix(const LUmatrix& M){
    p = gsl_permutation_alloc(M.p->size);
    gsl_permutation_memcpy(p,M.p);
    A = gsl_matrix_alloc(M.A -> size1, M.A -> size2);
    gsl_matrix_memcpy(A,M.A);
    signum = M.signum;
  }

  matrix::LUmatrix matrix::LUmatrix::operator=(const LUmatrix& M){
    if(this != &M){
      gsl_permutation_free(p);
      p = gsl_permutation_alloc(M.p->size);
      gsl_permutation_memcpy(p,M.p);

      gsl_matrix_free(A);
      A = gsl_matrix_alloc(M.A -> size1, M.A -> size2);
      gsl_matrix_memcpy(A,M.A);

      signum = M.signum;      
    }
    return *this;
  }

  gsl_matrix* matrix::LUmatrix::matrix_ptr(){
    return A;
  }
  gsl_permutation* matrix::LUmatrix::perm_ptr(){
    return p;
  }

  int* matrix::LUmatrix::sgn_ptr(){
    return &signum;
  }
  
  int matrix::LUmatrix::sgn(){
    return signum;
  }

  matrix::LUmatrix::LUmatrix(){
    A = 0;
    p = 0;
    signum = 0;
  }

  matrix::LUmatrix::~LUmatrix(){
    gsl_permutation_free(p);
    p = 0;
    gsl_matrix_free(A);
    A = 0;
  }
}


//Vector stuff

namespace linalg{
  using namespace error_handling;

  // **************** Vector allocation stuff ********************* 
  
  vector::vector(){
    x = gsl_vector_calloc(1); //Allocate zero vector of size one.
  }

  vector::vector(const size_t n, const double fillvalue){
    x = gsl_vector_alloc(n);
    gsl_vector_set_all(x,fillvalue);
  }

  vector::vector(gsl_vector *y){
    x = y;
  }

  vector::vector(const gsl_vector *y){
    x = gsl_vector_alloc(y -> size);
    gsl_vector_memcpy(x,y);
  }

  vector::vector(const vector &y){
    x = gsl_vector_alloc(y.x->size);
    gsl_vector_memcpy(x,y.x);
  }
 
  vector& vector::operator=(const vector &y){
    if(this != &y){
      gsl_vector_free(x);
      x = gsl_vector_alloc(y.x -> size);
      gsl_vector_memcpy(x,y.x);
    }
    return *this;
  }

  vector::~vector(){
    if(x != 0) //Has subclass vector_view already deleted this vector?
      gsl_vector_free(x);
  }
  
  // **************** Vector accessor stuff **********************
  size_t vector::precsn = 4;

  size_t vector::precision() const{
    return precsn;
  }

  void vector::set_precision(size_t p){
    precsn = p;
  }

  size_t vector::size() const{
    return x->size;
  }
  
  vector_view vector::operator()(const slice &s) {
    vector_view x_sub(x,s);
    return x_sub;
  }

  const vector_view vector::operator()(const slice &s) const {
    vector_view x_sub(x,s);
    return x_sub;
  }

  //*********************** Vector arithmetic stuff ***************

  vector vector::operator*(const double a) const{
    vector v = *this;
    gsl_vector_scale(v.x, a);
    return v;
  }

  vector vector::operator+(const vector& w) const{
    if(x -> size != w.x->size){
      inconformantSizes exc;
      exc.reason = "Cannot add vectors of different sizes.";
      exc.file = __FILE__;
      exc.line = __LINE__;
      exc.n_A = x->size;
      exc.n_B = w.x->size;
      throw exc;
    }
    vector u = *this;
    gsl_vector_add(u.x,w.x);
    return u;    
  }

  vector vector::operator-(const vector& w) const{
    return *this + (-1)*w;
  }

  double vector::operator*(const vector& w) const{
    double a;
    try{
      gsl_blas_ddot(x, w.x, &a);
    }
    catch(inconformantSizes& exc){
      exc.reason = "Can't dot product vectors of different sizes.";
      exc.file = __FILE__;
      exc.line = __LINE__;
      exc.n_A = x->size;
      exc.n_B = w.x -> size;
      throw exc;
    }
    return a;
  }

  vector vector::operator*(const matrix& M) const{
    return M* (*this);
  }
  
  double vector::norm() const{
    return gsl_blas_dnrm2(x);
  }

  //Comparison
  bool vector::operator==(const vector& w) const{
    if(x -> size != w.x -> size){
      badArgument exc;
      exc.reason = "Cannot compare vectors of different sizes.";
      exc.file = __FILE__;
      exc.line = __LINE__;
      throw exc;
    }
    
    double eps = std::numeric_limits<double>::epsilon();
    for(size_t i = 0; i < x -> size; i++)
      if(gsl_fcmp(gsl_vector_get(x,i), gsl_vector_get(w.x,i), eps) != 0)
	return false;
    
    return true;
  }

  bool vector::operator<(const vector& w) const{
    if(x -> size < w.x -> size) //Smaller vectors go first in this order.
       return true;

    for(size_t i = 0; i < x -> size; i++){
      double L = gsl_vector_get(x,i);
      double R = gsl_vector_get(w.x,i);
      if(L < R )
	return true;
      if(L > R )
	return false;
    }
   
    return false; //Then vectors are equal.
  }
}

//************************* Vector view stuff *******************************
namespace linalg{
  vector_view::vector_view(){
    x = 0;
  }
  vector_view::vector_view(gsl_vector* y, const slice &s):vector(new gsl_vector){
    if(s.end() > y->size - 1){
      indexOutOfRange exc;
      exc.file = __FILE__;
      exc.line = __LINE__;
      exc.reason = "invalid vector slice upper range.";
      exc.i = s.end();
      exc.n = y->size;
      throw exc;
    }
    x -> size = (s.end() - s.begin())/s.stride()+1;
    x -> data = gsl_vector_ptr(y,s.begin());
    x -> stride = s.stride();
    x -> block = 0;
    x -> owner = 0;
  }

  vector_view::vector_view(gsl_matrix* A, const slice& a, const size_t j ):vector(new gsl_vector){
    if(a.end() > A -> size1 - 1 or j-1 > A -> size2-1){
      indexOutOfRange exc;
      exc.file = __FILE__;
      exc.line = __LINE__;
      exc.reason = "invalid matrix view range.";
      exc.i = a.end();
      exc.j = j;
      exc.n = A -> size1;
      exc.m = A -> size2;
      throw exc;
    }
    x -> size = (a.end() - a.begin())/a.stride()+1;
    x -> data = gsl_matrix_ptr(A, a.begin(), j-1);
    x -> stride = a.stride()*(A -> tda); //Note that a longer step is
					 //necessary here.
    x -> block = 0;
    x -> owner = 0;
  }

  vector_view::vector_view(gsl_matrix* A, const size_t i, const slice& b ):vector(new gsl_vector){
    if(b.end() > A -> size2 - 1 or i-1 > A-> size1-1){
      indexOutOfRange exc;
      exc.file = __FILE__;
      exc.line = __LINE__;
      exc.reason = "invalid matrix view range.";
      exc.i = i;
      exc.j = b.end();
      exc.n = A -> size1;
      exc.m = A -> size2;
      throw exc;
    }
    x -> size = (b.end() - b.begin())/b.stride()+1;
    x -> data = gsl_matrix_ptr(A, i-1, b.begin());
    x -> stride = b.stride();
    x -> block = 0;
    x -> owner = 0;
  }

  vector_view& vector_view::operator=(const vector& w){
    if(x->size != w.size()){
      badArgument exc;
      exc.reason =  "Cannot assign to vector view: incomformant sizes.";
      exc.file = __FILE__;
      exc.line = __LINE__;
      throw exc;
    }
    gsl_vector_memcpy(x,w.x);
    return *this;
  }

  vector_view& vector_view::operator=(const vector_view& w){
    if(x->size != w.size()){
      badArgument exc;
      exc.reason =  "Cannot assign to vector view: incomformant sizes.";
      exc.file = __FILE__;
      exc.line = __LINE__;
      throw exc;
    }
    gsl_vector_memcpy(x,w.x);
    return *this;
    
  }

  vector_view::~vector_view(){
    delete x;
    x = 0;
  }
}


//Slice stuff
namespace linalg{
  using namespace error_handling;

  slice::slice(size_t a, size_t b, size_t k) {
    set(a,b,k);
  } 

  slice slice::operator()(size_t a, size_t b, size_t k) {
    return set(a,b,k);
  }

  slice slice::set(size_t a, size_t b, size_t k) {
    if(b < a){
      badArgument exc;
      exc.reason = 
	"Invalid arguments to slice::set. "
	"Right endpoint must be greater than left endpoint.";
      throw exc;
    }
    else if(k==0){
      badArgument exc;
      exc.reason = 
	"Invalid arguments to slice::set. "
	"Cannot take zero stride.";
      throw exc;
    }
    else if( k > b-a){
      badArgument exc;
      exc.reason = "Invalid arguments to slice::set. " 
	"Step size cannot be greater than range.";
      throw exc;
    }
    beg = a-1;
    fin = b-1;
    str = k;
    return *this;
  }

}

//Non-member functions
namespace linalg{
  
  // *************      I/O       ************************
  
/*
// edited by stefano 22 may 2008
  std::ostream& operator<<(std::ostream& os, const vector &v){
    //os.setf(std::ios::scientific);
    //os << std::setprecision(v.precision());
    for(size_t i = 1; i <= v.size(); i++){
      //os << " " << std::setw(v.precision()+6) << v(i) << " ";
     // os << v(i) << " ";
    //}
   // os << std::endl;
    
     //return os;
   //}
   */
  /* original */

  std::ostream& operator<<(std::ostream& os, const vector &v){
    os.setf(std::ios::scientific);
    os << std::setprecision(v.precision());
    for(size_t i = 1; i <= v.size(); i++){
      os << " " << std::setw(v.precision()+6) << v(i) << " ";
 
    }
    //os << std::endl;
    
     return os;
   }

  // STE: 2
  
  /*std::ostream& operator<<(std::ostream& os, const std::vector < RBF > &v){
    //os.setf(std::ios::scientific);
    os << std::setprecision(v.precision());
    for(size_t i = 1; i <= v.size(); i++){
      os << " " << std::setw(v.precision()+6) << v(i) << " ";
 
    }
    os << std::endl;
    
     return os;
   }
*/
   vector operator>>(std::istream& is, vector& v){
     using namespace std;
     string s;
     list<double> data;
     bool colvector = true;
     bool shouldbedone = false;
     while(getline(is, s)){
       s = utils::trim(s);
       if(s[0] == '#' or s.size() == 0) //Blank line or comment character
	 continue;

       stringstream ss;
       ss << s;

       double x;
       size_t i = 0;
       while(ss >> x){
	 if( (i > 0 and colvector == false) or (shouldbedone == true)){
	   badArgument exc;
	   exc.reason = "Cannot read vector: bad format in input";
	   exc.file = __FILE__;
	   exc.line = __LINE__;
	   throw exc;
	 }
	 data.push_back(x);
	 i++;
       }
       if(i > 1){
	 colvector = false; //So it must be a row vector instead
	 shouldbedone = true;
       }
     }

     if(data.size() == 0){
       endOfFile exc;
       exc.reason = "Cannot read empty vector from input.";
       exc.file = __FILE__;
       exc.line = __LINE__;
       throw exc;
     }

     vector w(data.size());
     typedef list<double>::iterator LI;
     size_t k = 1;
     for(LI i = data.begin(); i != data.end(); i++){
       w(k) = *i;
       k++;
     }
     v = w;
     return v;
   }
  /* 
   // edited by stefano 22-05-2008
   std::ostream& operator<<(std::ostream& os, const matrix& A){
      // os.setf(std::ios::scientific);
       //os << std::setprecision(A.precision());
       for(size_t i = 1; i <= A.rows(); i++){
         for(size_t j = 1; j <= A.cols(); j++)
  	 os << " pippo" << A(i,j) << " ";
         os << std::endl;
       }
       return os;
     }
   */
   // original
  std::ostream& operator<<(std::ostream& os, const matrix& A){
     os.setf(std::ios::scientific);
     os << std::setprecision(A.precision());
     for(size_t i = 1; i <= A.rows(); i++){
       for(size_t j = 1; j <= A.cols(); j++)
	 os << " " << std::setw(A.precision()+6) << A(i,j) << " ";
       os << std::endl;
     }
     return os;
   }

  matrix operator>>(std::istream& is, matrix& A){
    using namespace std;
    string line, token;
    bool rowset = false;
    list<double> data;
     
    size_t rowsize = 0;
    size_t rows = 0;
    size_t cols = 0;
    while(getline(is, line)){
      line = utils::trim(line);
      //Blank row or comment character.
      if(line[0] == '#' or line.length() == 0) 
	continue;
         
      stringstream ss_line;      
      cols = 0;
      ss_line << line;
      while(ss_line >> token){
	if(token[0] == '#'){
	  break; //Rest of line is comment.
	}

	//The following may fail on a C++ implementation that doesn't
	//obey IEEE arithmetic (IEC 559). We could check for those,
	//but do we really want to compile Octave on C++
	//implementations that don't follow IEEE arithmetic?
	else if(token == "NaN"){ 
	  double x = std::numeric_limits<double>::quiet_NaN();
	  data.push_back(x);
	  cols++;
	}
	else if(token == "Inf"){
	  double x = std::numeric_limits<double>::infinity();
	  data.push_back(x);
	  cols++;
	}
	else if(token == "-Inf"){
	  double x = -std::numeric_limits<double>::infinity();
	  data.push_back(x);
	  cols++;
	}
	else if(token == ","){
	  ss_line >> token;
	}

	//This also ignores commas and any other token. I think. If
	//there's garbage in the token, I have to see what happens
	//here. Do we also need to check for garbage?
	else{ 
	  double x;
	  stringstream ss_token;
	  ss_token << token;
	  ss_token >> x;
	  data.push_back(x);
	  cols++;
	}

      }
       
      //First row gives the number of columns, and all successive rows
      //must have the same number of elements. 
      if(!rowset){
	rowset = true;
	rowsize = cols;
      }
       
      if (cols != rowsize){
	badArgument exc;
	exc.reason = "Cannot read matrix: bad format in input";
	exc.file = __FILE__;
	exc.line = __LINE__;
	throw exc;
      }
      rows++;
    }
    if(rows == 0){
      endOfFile exc;
      exc.reason = "Cannot read empty matrix from input.";
      exc.file = __FILE__;
      exc.line = __LINE__;
      throw exc;
    }
    
    matrix M(rows,cols);
    typedef list<double>::iterator LI;
    
    LI k = data.begin();
    for(size_t i = 1; i <= rows; i++){
      for(size_t j = 1; j <= cols; j++){
	M(i,j) = *k;
	k++;
      }
    }

    A = M;    
    return A;
  }

  // *********** non-member arithmetic functions *******************

  vector operator*(double a, const vector& v){
    return v*a;
  }
  double norm(const vector& v){
    return v.norm();
  }
  matrix operator*(double a, const matrix& M){
    return M*a;
  }
  matrix inv(const matrix& A){
    return A.inv();
  }
  matrix T(const matrix& A){
    return A.T();
  }
  double tr(const matrix& A){
    return A.tr();
  }
  double det(matrix& A){
    return A.det();
  }

  double cond(matrix& A){
    return A.cond();
  }

}
