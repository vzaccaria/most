/* 
 * Authors: Vittorio Zaccaria
 * Copyright (c) 2007 Politecnico di Milano
 *
 * This file will eventually become GPL.
 * 
 */
#ifndef GPL_XDR_API
#define GPL_XDR_API

#include <vector>
#include <map>
#include <fstream>

using namespace std;
bool gpl_xdr_read_design(std::ifstream &, vector<double> &, vector<double> &);
void gpl_xdr_write_vector(std::ofstream &, vector<double> & x);
void gpl_xdr_write_raw_data(ofstream &fout, vector<double> & data, vector<double> & values);

#define GO_AHEAD(fin) (fin.good() && !fin.eof())

#endif
