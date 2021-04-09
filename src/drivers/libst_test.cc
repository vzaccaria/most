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

#include "st_design_space.h"
#include <iostream>
#include <sstream>
#include <st_shell_variables.h>
#include <string>
#include <st_parser.h>
#include <st_sim_utils.h>
#include <st_driver.h>

using namespace std;

class st_test_driver : public st_driver {
public:
  st_design_space *get_design_space(st_env *);

  st_point *simulate(st_point &, st_env *);
  bool is_valid(st_point &, st_env *) { return true; };
  bool is_thread_safe() { return true; };

  string get_information();
  string get_name();
  st_test_driver();
  ~st_test_driver();
};

string st_test_driver::get_information() {
  string info = "";
  info.append("Test driver written by G. Palermo");
  return info;
}

st_design_space *st_test_driver::get_design_space(st_env *env) {
  st_design_space *design_space = new st_design_space();
  design_space->insert_integer(env, "S1", 1000, 1009);
  design_space->insert_integer(env, "S2", 0, 9);
  design_space->insert_integer(env, "S3", 0, 9);
  design_space->insert_metric(env, "M1", "N/A");
  design_space->insert_metric(env, "M2", "N/A");
  design_space->insert_metric(env, "M3", "N/A");

  return design_space;
}

st_point *st_test_driver::simulate(st_point &point, st_env *env) {
  st_vector metrics;
  st_vector stats;
  st_point *fp;

  st_point *simulated_point = new st_point(point);

  double M1, M2, M3;

  int value[3];
  value[0] = (*simulated_point)[0];
  value[1] = (*simulated_point)[1];
  value[2] = (*simulated_point)[2];

  double av[3], xv[3], yv[3];
  av[0] = 0.0001;
  xv[0] = 139;
  yv[0] = 6;
  av[1] = 0.001;
  xv[1] = 58;
  yv[1] = 12;
  av[2] = 0.03;
  xv[2] = 83;
  yv[2] = 33;

  M1 = (value[0] + value[1]) * (value[0] + value[1]) * av[0] -
       2 * av[0] * xv[0] * (value[0] + value[1]) + av[0] * xv[0] * xv[0] +
       yv[0];
  M2 = ((value[2]) * (value[2]) * av[1] - 2 * av[1] * xv[1] * (value[2]) +
        av[1] * xv[1] * xv[1] + yv[1]) /
       300;
  M3 = (value[1] - value[2]) * (value[1] - value[2]) * av[2] -
       2 * av[2] * xv[2] * (value[1] - value[2]) + av[2] * xv[2] * xv[2] +
       yv[2];

  metrics.insert(0, st_double(M1));
  metrics.insert(1, st_double(M2));
  metrics.insert(2, st_double(M3));

  simulated_point->set_properties("metrics", metrics);
  simulated_point->set_properties("statistics", stats);

  return simulated_point;
}

string st_test_driver::get_name() {
  string name = "test_driver";
  return name;
}

st_test_driver::st_test_driver() { prs_display_message("Loading test"); }

st_test_driver::~st_test_driver() { prs_display_message("Removing test"); }

extern "C" {

st_test_driver *drv_generate_driver() {
  prs_display_message("Creating test");
  return new st_test_driver();
}
}
