#ifndef SPLINE_SCRIPT_HPP
#define SPLINE_SCRIPT_HPP

#include "parameters.h"
#include <vector>
#include <set>
#include <string>

using namespace std;

enum usage{spline,linear,none};

class spline_script
{
 public:
  spline_script(Parameters& params);
  ~spline_script();
  void generate_spline_script(vector<vector<double> > configuration,vector<vector<double> > observation);
  void run_spline_script();
  bool check_problems(unsigned int nvar);
 private: 
  vector<unsigned int>  locate_configurations(vector<vector<double> > configuration,vector<vector<double> > observation);
  vector<vector<unsigned int> > check_interaction(string corr_file_name,unsigned int fnum);
  void calc_cardinalities(vector<vector<double> > configuration);
  
  bool trouble;
  vector<unsigned int> poly; //in case of bad spline, try with a polynom which degree is in this vector
  set<unsigned int> var_prob; // contains variables faund problematic after R... they cannot be used as spline
  vector<bool> var_trouble; //if a variable has caused trouble till now
  vector<usage> domains; //
  vector<unsigned int> cardinalities; //for each predictor variable it is stored the number of different configurations
  vector<unsigned int> pred_index;

  Parameters param;
};

#ifndef GOON_FUN
#define GOON_FUN
#define GOON(x) (x.good() && !x.eof())
#endif

#endif
