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
#ifndef ST_DOE
#define ST_DOE

class st_doe;

#include <iostream>
#include "st_database.h"
#include "st_env.h" 
#include "st_map.h"
#include "st_object.h"
#include "st_vector.h"

/**
 *  Abstract doe definition
 */

class st_doe {
public:
  /** Generates the DOE */
  virtual st_vector *generate_doe(st_env *env) = 0;

  /** Returns a information string about the current doe */
  virtual string get_information() = 0;

  virtual ~st_doe(){};

  /** Handle to unload the module */
  void *handle;
};

#endif
