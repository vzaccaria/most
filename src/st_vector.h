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
#ifndef ST_VECTOR
#define ST_VECTOR

#include <algorithm>
#include <iostream>
#include <list>
#include <map>
#include "st_object.h"
#include "st_object_utils.h"
#include <string>
#include <vector>

class st_vector : public st_object {
public:
  vector<st_object *> the_vector;
  void copy(st_vector const *from, st_vector *to) const;
  st_vector();
  st_vector(const st_vector &l);
  st_vector(const st_object **v, int n);
  st_vector(const vector<double> &x);
  ~st_vector();

  void insert(int n, const st_object &o);
  st_object const &get(int) const;
  size_t size() const;
  string print() const;
  string full_print() const;
  st_object *gen_copy() const;

  string get_string_at(int index);
};

vector<string> get_string_vector(st_object *v);

#endif
