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

/* Abstract driver definition */
#ifndef ST_DRIVER
#define ST_DRIVER

#include <iostream>
#include "st_database.h"
#include "st_map.h"
#include "st_object.h"
#include "st_point.h"
#include "st_vector.h"

class st_env;
class st_design_space;

class st_driver {
public:
  virtual st_design_space *get_design_space(st_env *) = 0;

  virtual st_point *simulate(st_point &, st_env *) = 0;
  virtual bool is_valid(st_point &, st_env *) = 0;
  virtual bool is_thread_safe() = 0;

  virtual string get_information() = 0;
  virtual string get_name() = 0;

  virtual ~st_driver(){};

  void *handle;
};

#endif
