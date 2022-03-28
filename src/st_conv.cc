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

#include <iomanip>
#include <sstream>
#include <st_conv.h>

string st_ftoa(double nm) {
  ostringstream str;
  str << fixed << setprecision(2) << nm;
  return str.str();
}

string st_itoa(int i) {
  ostringstream str;
  str << i;
  return str.str();
}
