/*! \file error.hpp
 * \brief Throw exceptions instead of using GSL error handler function which
 * prefers to call abort().
 *

 * Remember to put `gsl_set_error_handler(&errorHandler);' in the
 * main() loops when including this header file; otherwise it's
 * useless!
 */

#ifndef __ERROR_H__
#define __ERROR_H__

#include <gsl/gsl_errno.h>
#include <string>

/// Exception classes in this namespace
namespace error_handling{
  //Structs to be thrown as exceptions
  using std::string;

  /// generic error and base struct.   /*GSL_FAILURE  = -1,*/
  class error{   
  public:
    std::string reason, file;
    int line;
    error(){};
    error(string r, string f, int l) : 
      reason(r), file(f), line(l) {};

  };

  struct noConvergence ;
  struct badDomain ;
  struct badRange ;
  struct badPointer ;
  struct badArgument ;
  struct failure ;
  struct failedFactorisation ;
  struct failedSanity ;
  struct outOfMemory ;
  struct badFunction ;
  struct runAway ;
  struct maxIterations ;
  struct divideByZero ;
  struct badTolerance ;
  struct aboveTolerance ;
  struct underflow ;
  struct overflow ;
  struct lossOfAccuracy ;
  struct roundOffError ;
  struct inconformantSizes ;
  struct matrixNotSquare ;
  struct singularityFound ;
  struct integralOrSeriesDivergent ;
  struct badHardware ;
  struct notImplemented ;
  struct cacheLimitExceeded ;
  struct tableLimitExceeded ;
  struct iterationNotProgressing ;
  struct jacobiansNotImprovingSolution ;  

  struct cannotReachToleranceInF ;
  struct cannotReachToleranceInX ;
  struct cannotReachToleranceInGradient ;
  struct endOfFile ;

  /*! \brief Custom error handler to be used for GSL. 
   * 
   * Throw exceptions instead of calling abort().
   *
   * Remember to put `gsl_set_error_handler(&errorHandler);' in the
   * main() loops when including this header file; otherwise it's
   * useless!
   */
  void errorHandler(const char * reason, const char * file, 
		    int line, int gsl_errno); 
}


//A few more details about the structs we're throwing as exceptions.
namespace error_handling{
  using std::string;

  ///GSL_CONTINUE = -2,  /* iteration has not converged */
  struct noConvergence : public error {
    noConvergence() {};
    noConvergence(string r, string f, int l)  : 
      error(r,f,l) {};
  };

  ///GSL_EDOM     = 1,   /* input domain error, e.g sqrt(-1) */
  struct badDomain : public error {
    badDomain() {};
    badDomain(string r, string f, int l)  : 
      error(r,f,l) {};
  };

  ///GSL_ERANGE   = 2,   /* output range error, e.g. exp(1e100) */
  struct badRange : public error {
    badRange() {};
    badRange(string r, string f, int l)  : 
      error(r,f,l) {};
  };

  ///GSL_EFAULT   = 3,   /* invalid pointer */
  struct badPointer : public error {
    badPointer() {};
    badPointer(string r, string f, int l)  : 
      error(r,f,l) {};
  };

  ///GSL_EINVAL   = 4,   /* invalid argument supplied by user */
  struct badArgument : public error {
    badArgument() {};
    badArgument(string r, string f, int l)  : 
      error(r,f,l) {};
  };

  ///GSL_EFAILED  = 5,   /* generic failure */
  struct failure : public error {
    failure() {};
    failure(string r, string f, int l)  : 
      error(r,f,l) {};
  };

  ///GSL_EFACTOR  = 6,   /* factorization failed */
  struct failedFactorisation : public error {
    failedFactorisation() {};
    failedFactorisation(string r, string f, int l)  : 
      error(r,f,l) {};
  };

  ///GSL_ESANITY  = 7,   /* sanity check failed - shouldn't happen */
  struct failedSanity : public error {
    failedSanity() {};
    failedSanity(string r, string f, int l)  : 
      error(r,f,l) {};
  };

  ///GSL_ENOMEM   = 8,   /* malloc failed */
  struct outOfMemory : public error {
    outOfMemory() {};
    outOfMemory(string r, string f, int l)  : 
      error(r,f,l) {};
  };

  ///GSL_EBADFUNC = 9,   /* problem with user-supplied function */
  struct badFunction : public error {
    badFunction() {};
    badFunction(string r, string f, int l)  : 
      error(r,f,l) {};
  };

  ///GSL_ERUNAWAY = 10,  /* iterative process is out of control */
  struct runAway : public error {
    runAway() {};
    runAway(string r, string f, int l)  : 
      error(r,f,l) {};
  };

  ///GSL_EMAXITER = 11,  /* exceeded max number of iterations */
  struct maxIterations : public error {
    maxIterations() {};
    maxIterations(string r, string f, int l)  : 
      error(r,f,l) {};
  };

  ///GSL_EZERODIV = 12,  /* tried to divide by zero */
  struct divideByZero : public error {
    divideByZero() {};
    divideByZero(string r, string f, int l)  : 
      error(r,f,l) {};
  };

  ///GSL_EBADTOL  = 13,  /* user specified an invalid tolerance */
  struct badTolerance : public error {
    badTolerance() {};
    badTolerance(string r, string f, int l)  : 
      error(r,f,l) {};
  };

  ///GSL_ETOL     = 14,  /* failed to reach the specified tolerance */
  struct aboveTolerance : public error {
    aboveTolerance() {};
    aboveTolerance(string r, string f, int l)  : 
      error(r,f,l) {};
  };

  ///GSL_EUNDRFLW = 15,  /* underflow */
  struct underflow : public error {
    underflow() {};
    underflow(string r, string f, int l)  : 
      error(r,f,l) {};
  };

  ///GSL_EOVRFLW  = 16,  /* overflow  */
  struct overflow : public error {
    overflow() {};
    overflow(string r, string f, int l)  : 
      error(r,f,l) {};
  };

  ///GSL_ELOSS    = 17,  /* loss of accuracy */
  struct lossOfAccuracy : public error {
    lossOfAccuracy() {};
    lossOfAccuracy(string r, string f, int l)  : 
      error(r,f,l) {};
  };

  ///GSL_EROUND   = 18,  /* failed because of roundoff error */
  struct roundOffError : public error {
    roundOffError() {};
    roundOffError(string r, string f, int l)  : 
      error(r,f,l) {};
  };

  ///GSL_EBADLEN  = 19,  /* matrix, vector lengths are not conformant */
  struct inconformantSizes : public error {
    inconformantSizes() {};
    inconformantSizes(string r, string f, int l)  : 
      error(r,f,l) {};
    int n_A, m_A, n_B, m_B;
  };

  ///GSL_ENOTSQR  = 20,  /* matrix not square */
  struct matrixNotSquare : public error {
    matrixNotSquare() {};
    matrixNotSquare(string r, string f, int l)  : 
      error(r,f,l) {};
  };

  ///GSL_ESING    = 21,  /* apparent singularity detected */
  struct singularityFound : public error {
    singularityFound() {};
    singularityFound(string r, string f, int l)  : 
      error(r,f,l) {};
  };

  ///GSL_EDIVERGE = 22,  /* integral or series is divergent */
  struct integralOrSeriesDivergent : public error {
    integralOrSeriesDivergent() {};
    integralOrSeriesDivergent(string r, string f, int l)  : 
      error(r,f,l) {};
  };

  ///GSL_EUNSUP   = 23,  /* requested feature is not supported by the hardware */
  struct badHardware : public error {
    badHardware() {};
    badHardware(string r, string f, int l)  : 
      error(r,f,l) {};
  };

  ///GSL_EUNIMPL  = 24,  /* requested feature not (yet) implemented */
  struct notImplemented : public error {
    notImplemented() {};
    notImplemented(string r, string f, int l)  : 
      error(r,f,l) {};
  };

  ///GSL_ECACHE   = 25,  /* cache limit exceeded */
  struct cacheLimitExceeded : public error {
    cacheLimitExceeded() {};
    cacheLimitExceeded(string r, string f, int l)  : 
      error(r,f,l) {};
  };

  ///GSL_ETABLE   = 26,  /* table limit exceeded */
  struct tableLimitExceeded : public error {
    tableLimitExceeded() {};
    tableLimitExceeded(string r, string f, int l)  : 
      error(r,f,l) {};
  };

  ///GSL_ENOPROG  = 27,  /* iteration is not making progress towards solution */
  struct iterationNotProgressing : public error {
    iterationNotProgressing() {};
    iterationNotProgressing(string r, string f, int l)  : 
      error(r,f,l) {};
  };

  ///GSL_ENOPROGJ = 28, /* jacobian evaluations are not improving solution */
  struct jacobiansNotImprovingSolution : public error {
    jacobiansNotImprovingSolution() {};
    jacobiansNotImprovingSolution(string r, string f, int l)  : 
      error(r,f,l) {};
  };  

  ///GSL_ETOLF    = 29,  /* cannot reach the specified tolerance in F */
  struct cannotReachToleranceInF : public error {
    cannotReachToleranceInF() {};
    cannotReachToleranceInF(string r, string f, int l)  : 
      error(r,f,l) {};
  };

  ///GSL_ETOLX    = 30,  /* cannot reach the specified tolerance in X */
  struct cannotReachToleranceInX : public error {
    cannotReachToleranceInX() {};
    cannotReachToleranceInX(string r, string f, int l)  : 
      error(r,f,l) {};
  };

  ///GSL_ETOLG    = 31,  /* cannot reach the specified tolerance in gradient */
  struct cannotReachToleranceInGradient : public error {
    cannotReachToleranceInGradient() {};
    cannotReachToleranceInGradient(string r, string f, int l)  : 
      error(r,f,l) {};
  };

  ///GSL_EOF      = 32   /* end of file */
  struct endOfFile : public error {
    endOfFile() {};
    endOfFile(string r, string f, int l)  : 
      error(r,f,l) {};
  };

  ///Exception for indices out of range.
  struct indexOutOfRange : public badArgument{
    int i,j,m,n;
    indexOutOfRange() {};
    indexOutOfRange(string r, string f, int l)  : 
      badArgument(r,f,l) {};
  };
}

#endif //__ERROR_H__
