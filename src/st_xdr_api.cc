/* @STSHELL_LICENSE_START@
 *
 *      __  _______  ___________
 *     /  |/  / __ \/ ___/_  __/
 *    / /|_/ / / / /\__ \ / /
 *   / /  / / /_/ /___/ // /
 *  /_/  /_/\____//____//_/
 *
 * Multi-Objective System Tuner
 * Copyright (c) 2007-2011 Politecnico di Milano
 *
 * Development leader: Vittorio Zaccaria
 * Main developers: Vittorio Zaccaria, Gianluca Palermo, Fabrizio Castro
 *
 * This file is confidential property of Politecnico di Milano.
 *
 * @STSHELL_LICENSE_END@ */

#include <fstream>
#include <iostream>
#include <map>
#include <st_xdr_api.h>

using namespace std;

void st_xdr_write_design(ofstream &fout, vector<int> &data,
                         vector<double> &values) {
  fout << data.size() << " ";
  for (int i = 0; i < data.size(); i++) {
    fout << data[i] << " ";
    // cout << data[i] << " ";
  }
  fout << values.size() << " ";
  for (int i = 0; i < values.size(); i++) {
    fout << values[i] << " ";
    // cout << tmp_val << " ";
  }
  fout << endl;
  // cout << endl;
}

void st_xdr_write_raw_data(ofstream &fout, vector<double> &data,
                           vector<double> &values) {
  fout << data.size() << " ";
  for (int i = 0; i < data.size(); i++) {
    fout << data[i] << " ";
    // cout << data[i] << " ";
  }
  fout << values.size() << " ";
  for (int i = 0; i < values.size(); i++) {
    fout << values[i] << " ";
    // cout << tmp_val << " ";
  }
  fout << endl;
  // cout << endl;
}

// file("kool.cpp",ios::in|ios::out);
//

bool st_xdr_read_vector(ifstream &input, vector<double> &x) {
  x.clear();
  int num_of_elements;
  if (input.eof() || !input.good())
    return false;
  input >> num_of_elements;
  if (input.eof() || !input.good())
    return false;
  int n = 0;
  while (!input.eof() && input.good() && n < num_of_elements) {
    double delta;
    input >> delta;
    x.push_back(delta);
    n++;
  }
  if (n < num_of_elements)
    return false;
  return true;
}

#define GO_AHEAD(fin) (fin.good() && !fin.eof())

bool st_xdr_read_design(ifstream &fin, vector<double> &data,
                        vector<double> &values) {
  if (!GO_AHEAD(fin)) {
    return false;
  }

  data.clear();

  int dsize = 0;
  fin >> dsize;
#if defined(DEBUG)
  cout << " [" << dsize << "] ";
#endif
  int i;
  for (i = 0; i < dsize && GO_AHEAD(fin); i++) {
    double k;
    fin >> k;
    data.push_back(k);
#if defined(DEBUG)
    cout << " " << k;
#endif
  }
  if (i != dsize) {
    return false;
  }

  if (!GO_AHEAD(fin)) {

    return false;
  }
  values.clear();

  fin >> dsize;
#if defined(DEBUG)
  cout << " [" << dsize << "] ";
#endif
  for (i = 0; i < dsize && GO_AHEAD(fin); i++) {
    double x;
    fin >> x;
    values.push_back(x);
#if defined(DEBUG)
    cout << " " << x;
#endif
  }
#if defined(DEBUG)
  cout << endl;
#endif
  if (i != dsize) {
    return false;
  }

  return true;
}
