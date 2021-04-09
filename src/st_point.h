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
#ifndef ST_POINT
#define ST_POINT

#include <algorithm>
#include <iostream>
#include <list>
#include <map>
#include "st_object.h"
#include "st_object_utils.h"
#include <string>
#include <vector>

class st_env;

/** Error codes associated to a point */
#define ST_POINT_NO_ERROR 0
#define ST_POINT_FATAL_ERROR 1
#define ST_POINT_NON_FATAL_ERROR 2

#define ST_ERR_NO_ERR ST_POINT_NO_ERROR
#define ST_ERR_SIM_ERR ST_POINT_FATAL_ERROR
#define ST_ERR_CONSTR_ERR ST_POINT_NON_FATAL_ERROR
#define ST_ERR_OTHER_ERR ST_POINT_FATAL_ERROR

struct metric_cache {
  int optimization_timestamp;
  st_object *value;
  metric_cache() {
    optimization_timestamp = -1;
    value = NULL;
  }
  metric_cache(const metric_cache &x) {
    optimization_timestamp = x.optimization_timestamp;
    if (x.value)
      value = x.value->gen_copy();
    else
      value = NULL;
  }
  void set_value(st_object *x) {
    if (value)
      delete value;
    value = x->gen_copy();
  }
  ~metric_cache() {
    if (value)
      delete value;
  }
  void clear_cache() {
    if (value)
      delete value;
  }
};

struct objective_cache {
  int optimization_timestamp;
  double value;
  objective_cache() { optimization_timestamp = -1; }
  objective_cache(const objective_cache &x) {
    optimization_timestamp = x.optimization_timestamp;
    value = x.value;
  }
  void clear_cache() { optimization_timestamp = -1; }
};

struct constraint_cache {
  int optimization_timestamp;
  int constraints_violated;
  double penalty;
  constraint_cache() { optimization_timestamp = -1; }
  constraint_cache(const constraint_cache &x) {
    optimization_timestamp = x.optimization_timestamp;
    constraints_violated = x.constraints_violated;
    penalty = x.penalty;
  }
  void clear_cache() { optimization_timestamp = -1; }
};

class st_point : public st_object, public vector<int> {
public:
  vector<objective_cache> o_cache;
  vector<metric_cache> m_cache;
  constraint_cache c_cache;

  st_point();
  st_point(const st_point &p);
  st_point(const vector<int> &p);
  st_point(int);
  ~st_point();
  st_point &operator=(const st_point &);
  bool operator==(const st_point &o);
  void insert(int, int);
  string print() const;
  string print_canonical() const;
  string print_m3_canonical() const;
  string print_m3_canonical_objectives(st_env *env);
  string print_octave(st_env *env);
  string full_print() const;
  st_object *gen_copy() const;
  st_point *gen_copy_point() const;
  void set_sims(int num);
  int get_sims();
  void set_error(int errorcode);
  void set_error(int errorcode, string reason);
  void set_rpath(string rpath);
  string get_rpath();
  int get_error();
  string get_error_description();
  bool fatal();
  void set_cluster(int cluster);
  int get_cluster();
  bool check_consistency(st_env *) const;
  double get_metrics(int) const;
  double get_metrics(string name, st_env *) const;
  double get_statistics(int) const;
  void clear_cache() {
    for (int i = 0; i < o_cache.size(); i++)
      o_cache[i].clear_cache();
    for (int i = 0; i < m_cache.size(); i++)
      m_cache[i].clear_cache();
    c_cache.clear_cache();
  }
};

#endif
