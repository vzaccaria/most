#include "converter.hpp"
#include "../../gpl_xdr_api.h"
#include <iostream>
#include <fstream>
using namespace std;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
converter::converter() {}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
converter::~converter() {}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void converter::convert_in(vector<vector<double> > configuration,
                           vector<vector<double> > observation,
                           vector<vector<double> > pred_configuration) {
  const char sep = ','; // cvs separator
  ofstream conf("observation.csv");
  ofstream pred("pred_conf.csv");
  const unsigned int conf_size = configuration.size();
  const unsigned int nvar = configuration.at(0).size();
  const unsigned int nfun = observation.at(0).size();
  // write the header in order to assign a name to each variable
  for (unsigned int i = 0; i < nvar; i++) {
    conf << "x" << i << sep;
    pred << "x" << i;
    if (i + 1 == nvar)
      pred << std::endl;
    else
      pred << sep;
  }
  for (unsigned int i = 0; i < nfun; i++) {
    conf << "f" << i;
    if (i + 1 == nfun) {
      conf << std::endl;
    } else {
      conf << sep;
    }
  }

  // writing the trining set file
  vector<double> single;
  for (unsigned int i = 0; i < conf_size; i++) {
    single = configuration.at(i);
    for (unsigned int j = 0; j < nvar; j++) {
      conf << single.at(j);
      conf << sep;
    }
    single = observation.at(i);
    for (unsigned int j = 0; j < nfun; j++) {
      conf << single.at(j);
      if (j + 1 == nfun)
        conf << std::endl;
      else
        conf << sep;
    }
  }

  // writing the prediction configuration file
  const unsigned int pred_size = pred_configuration.size();
  for (unsigned int i = 0; i < pred_size; i++) {
    single = pred_configuration.at(i);
    for (unsigned int j = 0; j < nvar; j++) {
      pred << single.at(j);
      if (j + 1 == nvar)
        pred << std::endl;
      else
        pred << sep;
    }
  }

  conf.close();
  pred.close();
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void converter::convert_out(string output) {
  cout << "\nStarting finals operation\n";
  // read the temporary result file "xdr_temp.db" saved by spline_script.cc
  ifstream temp("xdr_temp.db");
  double nvar = 0, nfun = 0, swap;
  vector<double> aux;
  vector<vector<double> > configuration, prediction;
  // load the prediction file produced by R
  if (!GOON(temp))
    return;
  temp >> nvar;
  while (GOON(temp)) {
    aux.clear();
    for (unsigned int i = 0; i < nvar; i++) // read configurations
    {
      temp >> swap;
      aux.push_back(swap);
    }

    if (!GOON(temp))
      break;
    configuration.push_back(aux);
    aux.clear();

    temp >> nfun;
    for (unsigned int i = 0; i < nfun; i++) // read prediction
    {
      temp >> swap;
      aux.push_back(swap);
    }
    prediction.push_back(aux);
    temp >> nvar;
  }
  temp.close();
  // write the output file
  cout << "Writing the result.db\n";
  ofstream out(output.c_str());
  unsigned int pred_size = configuration.size();
  for (unsigned int i = 0; i < pred_size; i++) {
    gpl_xdr_write_raw_data(out, configuration.at(i), prediction.at(i));
  }
  out.close();
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
