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
#ifndef ST_RSM
#define ST_RSM

class st_rsm;

#include <iostream>
#include "st_database.h"
#include "st_env.h"
#include "st_map.h"
#include "st_object.h"
#include "st_vector.h"

/** Abstract RSM definition */
class st_rsm {
public:
  /** Train RSM, command line parameters are passed to the chosen RSM by means
   * of the parameters value. This command should generate a database with the
   * current estimated values */
  virtual st_object *train_rsm(st_map *parameters, st_database *, st_env *) = 0;

  /** Prints a description on a string */
  virtual string print_information() = 0;
};

bool prs_command_train_rsm(st_map *parameters);
void rsm_init_rsms(st_env *);

#endif
