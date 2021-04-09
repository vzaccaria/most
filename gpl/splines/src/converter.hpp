#ifndef CONVERTER_H
#define CONVERTER_H
#include <vector>
#include <string>

using namespace std;

class converter
{
 public:
  //configuration and observation refers to training data set in which configuration contains the values of the domains 
  //for a corresponding observation
  //pred configuration contains the points in which evaluate the predicted function
  converter();
  converter(string obs_name,string pred_conf_name);
  ~converter();
  void convert_in(vector<vector<double> > configuration,vector<vector<double> > observation,vector<vector<double> > pred_configuration);
  void convert_out(string output);
};

#ifndef GOON_FUN
#define GOON_FUN
#define GOON(x) (x.good() && !x.eof())
#endif

#endif
