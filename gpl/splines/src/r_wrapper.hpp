#ifndef R_WRAPPER_HPP
#define R_WRAPPER_HPP

#include "parameters.h"
#include <string>
#include <vector>

using namespace std;
class r_wrapper
{
 public:
  r_wrapper(Parameters& params,vector<vector<double> > training_configurations, vector<vector<double> > training_observations);
  r_wrapper(Parameters& params,vector<vector<double> > training_configurations, vector<vector<double> > training_observations,vector<vector<double> > prediction_configurations);
  ~r_wrapper();
  void start();
 private:
  void delete_temp();
  void delete_temp_prob();
  vector<vector<double> > training_configurations;
  vector<vector<double> > training_observations;
  vector<vector<double> > prediction_configurations;
  Parameters param;
};
#endif
