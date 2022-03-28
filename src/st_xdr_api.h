/* @STSHELL_LICENSE_START@
 *
 *      __  _______  ___________ 
 *     /  |/  / __ \/ ___/_  __/ 
 *    / /|_/ / / / /\__ \ / /    
 *   / /  / / /_/ /___/ // /    
 *  /_/  /_/\____//____//_/     
 * 
 * Multi-Objective System Tuner 
 * Copyright (c) 2007-2022 Politecnico di Milano
 * 
 * Development leader: Vittorio Zaccaria
 * Main developers: Vittorio Zaccaria, Gianluca Palermo, Fabrizio Castro
 *  
 * 
 * @STSHELL_LICENSE_END@ */
#ifndef ST_XDR_API
#define ST_XDR_API

#include <fstream>
#include <map>
#include <vector>

using namespace std;
void st_xdr_write_design(std::ofstream &, vector<int> &, vector<double> &);
bool st_xdr_read_vector(std::ifstream &, vector<double> &x);
void st_xdr_write_raw_data(ofstream &fout, vector<double> &data,
                           vector<double> &values);
bool st_xdr_read_design(ifstream &fin, vector<double> &data,
                        vector<double> &values);

#endif
