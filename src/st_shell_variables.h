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
#ifndef SHELL_VARIABLES
#define SHELL_VARIABLES

#include "st_map.h"
#include "st_object.h"
#include "st_vector.h"

class st_shell_variables : public st_map {
public:
  /** Just prints a parser message when inserting a variable */
  void insert(string n, const st_object &o);

  /** Returns a plain integer */
  bool get_integer(string name, int &);

  /** Returns a plain string*/
  bool get_string(string name, string &str);

  /** Returns a plain double*/
  bool get_double(string name, double &d);

  /** Inserts a plain integer */
  void set_integer(string name, int);

  /** Inserts a plain string*/
  void set_string(string name, const char *);

  /** Inserts a plain double*/
  void set_double(string name, double);

  /** Prints a parseable structure */
  string print();

  /** Stores directly the pointer */
  void insert_dont_copy(string name, st_object *o);

  /** Get vector from variables */
  bool get_vector(string name, st_vector *&v);
};

#endif /* #SHELL_VARIABLES */
