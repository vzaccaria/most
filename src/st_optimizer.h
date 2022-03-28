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
#ifndef ST_OPTIMIZER
#define ST_OPTIMIZER

class st_optimizer;

#include <iostream>
#include "st_command_list.h"
#include "st_database.h"
#include "st_env.h"
#include "st_map.h"
#include "st_object.h"
#include "st_vector.h"

/**
 *  Abstract optimizer definition
 */

class st_optimizer {
public:
  /** Performs the exploration process; returns the number of points explored */
  virtual int explore(st_env *p) = 0;

  /** Returns a information string about the current optimizer */
  virtual string get_information() = 0;

  virtual ~st_optimizer(){};

  /** Handle to the module */
  void *handle;
};

#endif
