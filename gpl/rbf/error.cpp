// File with a few function definitions.
#include "error.hpp"

namespace error_handling{
  void errorHandler(const char * reason, const char * file, 
		    int line, int gsl_errno){
    //This exception is so common that we will want more information
    //when it happens.
    if(reason == string("index out of range")){ //GSL gives this message.
      throw indexOutOfRange(reason,file,line);
    }

    else{ //Other exceptions are more generic
    
      switch(gsl_errno){
      case -2:
	throw noConvergence(reason,file,line); break;
      case 1:
	throw badDomain(reason,file,line); break;
      case 2:
	throw badRange(reason,file,line); break;
      case 3:
	throw badPointer(reason,file,line); break;
      case 4:
	throw badArgument(reason,file,line); break;
      case 5:
	throw failure(reason,file,line); break;
      case 6:
	throw failedFactorisation(reason,file,line); break;
      case 7:
	throw failedSanity(reason,file,line); break;
      case 8:
	throw outOfMemory(reason,file,line); break;
      case 9:
	throw badFunction(reason,file,line); break;
      case 10:
	throw runAway(reason,file,line); break;
      case 11:
	throw maxIterations(reason,file,line); break;
      case 12:
	throw divideByZero(reason,file,line); break;
      case 13:
	throw badTolerance(reason,file,line); break;
      case 14:
	throw aboveTolerance(reason,file,line); break;
      case 15:
	throw underflow(reason,file,line); break;
      case 16:
	throw overflow(reason,file,line); break;
      case 17:
	throw lossOfAccuracy(reason,file,line); break;
      case 18:
	throw roundOffError(reason,file,line); break;
      case 19:
	throw inconformantSizes(reason,file,line); break;
      case 20:
	throw matrixNotSquare(reason,file,line); break;
      case 21:
	throw singularityFound(reason,file,line); break;
      case 22:
	throw integralOrSeriesDivergent(reason,file,line); break;
      case 23:
	throw badHardware(reason,file,line); break;
      case 24:
	throw notImplemented(reason,file,line); break;
      case 25:
	throw cacheLimitExceeded(reason,file,line); break;
      case 26:
	throw tableLimitExceeded(reason,file,line); break;
      case 27:
	throw iterationNotProgressing(reason,file,line); break;
      case 28:
	throw jacobiansNotImprovingSolution(reason,file,line); break;
      case 29:  
	cannotReachToleranceInF(reason,file,line); break;
      case 30:
	throw cannotReachToleranceInX(reason,file,line); break;
      case 31:
	throw cannotReachToleranceInGradient(reason,file,line); break;
      case 32:
	throw endOfFile(reason,file,line); break;
      default: //Corresponds to GSL_ERRNO=-1.
	throw error(reason,file,line);
      }

    }

  } //ends void handler(const char*, const char*, int, int);
} //Ends the error_handling namespace.
