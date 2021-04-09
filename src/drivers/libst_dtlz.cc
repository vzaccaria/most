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
/**
 * DTLZ test suite. Written by V. Zaccaria
 *
 * Apologies, I had to access directly current_environment from here.. Don't do
 * the same :-)
 */
#include <iostream>
#include <sstream>
#include <st_shell_variables.h>
#include <string>
#include <st_parser.h>
#include <st_sim_utils.h>
#include <st_driver.h>
#include <st_rand.h>
#include <math.h>
#include <st_design_space.h>

using namespace std;

class st_dtlz : public st_driver {
  vector<int> sample_num;

public:
  string get_information();
  st_vector *get_design_space_parameters();
  st_vector *get_metrics();
  st_vector *get_statistics();
  st_point *simulate(st_point &, st_env *);
  string get_name();
  int get_number_of_parameters();
  double f1(st_point &);
  double f2(st_point &);
  double f3(st_point &);
  double g(st_point &);
  double t1(st_point &);
  double t2(st_point &);
  /** Only for problem 6 */
  double h(double f1, double f2, double g);
  /** Only for problem 7 */
  double g1(st_point &);
  double g2(st_point &);
  double g3(st_point &);
  double log_sum(int);
  st_dtlz();
  ~st_dtlz();
};

#define X_(k) (((double)x[(k)-1]) / ((double)sample_num[(k)-1] - 1.0))

double st_dtlz::f1(st_point &x) {
  string problem;
  current_environment.shell_variables.get_string("problem", problem);
  if (problem == "dtlz1")
    return (1.0 / 2.0 * X_(1) * X_(2) * (1.0 + g(x)));
  if (problem == "dtlz2")
    return (cos(X_(1) * M_PI / 2.0) * cos(X_(2) * M_PI / 2.0) * (1.0 + g(x)));
  if (problem == "dtlz3")
    return (cos(X_(1) * M_PI / 2.0) * cos(X_(2) * M_PI / 2.0) * (1.0 + g(x)));
  if (problem == "dtlz4") {
    auto xx = ((double)pow_double_to_int(X_(1), 100)) * M_PI / 2;
    auto yy = ((double)pow_double_to_int(X_(2), 100)) * M_PI / 2.0;
    return (cos(xx) * cos(yy) * (1.0 + g(x)));
  }
  if (problem == "dtlz5")
    return (cos(t1(x) * M_PI / 2.0) * cos(t2(x)) * (1.0 + g(x)));
  if (problem == "dtlz6")
    return (X_(1));
  if (problem == "dtlz7")
    return (X_(1) + X_(2) + X_(3) + X_(4) + X_(5) + X_(6) + X_(7) + X_(8) +
            X_(9) + X_(10)) /
           10.0;
  st_assert(0);
}

double st_dtlz::f2(st_point &x) {
  string problem;
  current_environment.shell_variables.get_string("problem", problem);
  if (problem == "dtlz1")
    return (1.0 / 2.0 * X_(1) * (1.0 - X_(2)) * (1.0 + g(x)));
  if (problem == "dtlz2")
    return (cos(X_(1) * M_PI / 2.0) * sin(X_(2) * M_PI / 2.0) * (1.0 + g(x)));
  if (problem == "dtlz3")
    return (cos(X_(1) * M_PI / 2.0) * sin(X_(2) * M_PI / 2.0) * (1.0 + g(x)));
  if (problem == "dtlz4")
    return (cos(pow_double_to_int(X_(1), 100) * M_PI / 2.0) *
            sin(pow_double_to_int(X_(2), 100) * M_PI / 2.0) * (1.0 + g(x)));
  if (problem == "dtlz5")
    return (cos(t1(x) * M_PI / 2.0) * sin(t2(x)) * (1.0 + g(x)));
  if (problem == "dtlz6")
    return (X_(2));
  if (problem == "dtlz7")
    return (X_(11) + X_(12) + X_(13) + X_(14) + X_(15) + X_(16) + X_(17) +
            X_(18) + X_(19) + X_(20)) /
           10.0;
  st_assert(0);
}

double st_dtlz::f3(st_point &x) {
  string problem;
  current_environment.shell_variables.get_string("problem", problem);
  if (problem == "dtlz1")
    return (1.0 / 2.0 * (1 - X_(1)) * (1.0 + g(x)));
  if (problem == "dtlz2")
    return (sin(X_(1) * M_PI / 2.0) * (1.0 + g(x)));
  if (problem == "dtlz3")
    return (sin(X_(1) * M_PI / 2.0) * (1.0 + g(x)));
  if (problem == "dtlz4")
    return (sin(pow_double_to_int(X_(1), 100) * M_PI / 2.0) * (1.0 + g(x)));
  if (problem == "dtlz5")
    return (sin(t1(x) * M_PI / 2.0) * (1.0 + g(x)));
  if (problem == "dtlz6")
    return (1.0 + g(x)) * h(f1(x), f2(x), g(x));
  if (problem == "dtlz7")
    return (X_(21) + X_(22) + X_(23) + X_(24) + X_(25) + X_(26) + X_(27) +
            X_(28) + X_(29) + X_(30)) /
           10.0;
  st_assert(0);
}

#define NORM(x) ((double)x.size())

double st_dtlz::g(st_point &x) {
  string problem;
  current_environment.shell_variables.get_string("problem", problem);
  if (problem == "dtlz1" || problem == "dtlz3") {
    double result = NORM(x) - 2.0;
    double sum_res = 0.0;
    for (int i = 3; i <= x.size(); i++) {
      sum_res +=
          pow_double_to_int(X_(i) - 0.5, 2) - cos(20.0 * M_PI * (X_(i) - 0.5));
    }
    result = 100.0 * (result + sum_res);
    return result;
  }
  if (problem == "dtlz2") {
    double sum_res = 0.0;
    for (int i = 3; i <= x.size(); i++) {
      sum_res += pow_double_to_int(X_(i) - 0.5, 2);
    }
    return sum_res;
  }
  if (problem == "dtlz4") {
    double sum_res = 0.0;
    for (int i = 3; i <= x.size(); i++) {
      sum_res += pow_double_to_int(X_(i) - 0.5, 2);
    }
    return sum_res;
  }
  if (problem == "dtlz5") {
    double sum_res = 0.0;
    for (int i = 3; i <= x.size(); i++) {
      sum_res += pow_double_to_int(X_(i) - 0.5, 2);
    }
    return sum_res;
  }
  if (problem == "dtlz6") {
    double sum_res = 0.0;
    for (int i = 3; i <= x.size(); i++) {
      sum_res += (X_(i));
    }
    sum_res = sum_res * 9.0 / (22.0 - 2.0);
    return sum_res;
  }
  throw "DTLZ: Unexpected path executed";
}

double st_dtlz::t1(st_point &x) { return X_(1); }

double st_dtlz::t2(st_point &x) {
  return M_PI / (4.0 * (1.0 + g(x))) * (1.0 + 2.0 * g(x) * X_(2));
}

double st_dtlz::h(double f1, double f2, double g) {
  return 3.0 - f1 / (1.0 + g) * (1.0 + sin(3.0 * M_PI * f1)) +
         f2 / (1.0 + g) * (1.0 + sin(3.0 * M_PI * f2));
}

double st_dtlz::g1(st_point &x) { return f3(x) + 4.0 * f1(x) - 1.0; }

double st_dtlz::g2(st_point &x) { return f3(x) + 4.0 * f2(x) - 1.0; }

double st_dtlz::g3(st_point &x) { return 2.0 * f3(x) + f1(x) + f2(x) - 1.0; }

string st_dtlz::get_information() {
  string info = "";
  info.append("DTLZ reference functions for testing optimization algorithms");
  return info;
}

double st_dtlz::log_sum(int par) {
  double sum = 0;
  for (int j = 0; j < par; j++)
    sum += log((double)sample_num[j]);
  return sum;
}

st_vector *st_dtlz::get_design_space_parameters() {
  int discretized_space_size;
  if (!current_environment.shell_variables.get_integer(
          "discretized_space_size", discretized_space_size)) {
    prs_display_message("Please size of the discretized space with "
                        "'discretized_space_size', defaulting to 200,000");
    discretized_space_size = 200000;
    current_environment.shell_variables.set_integer("discretized_space_size",
                                                    discretized_space_size);
  }

  int num_of_parameters = get_number_of_parameters();
  sample_num.resize(num_of_parameters, 1);

  int l = 0;
  while (log_sum(num_of_parameters) < log((double)discretized_space_size)) {
    sample_num[l]++;
    l = (l + 1) % num_of_parameters;
  }

  st_vector *dse = new st_vector();
  for (int p = 0; p < num_of_parameters; p++) {
    st_vector value_vector;
    for (int i = 0; i < sample_num[p]; i++) {
      ostringstream value_string;
      value_string << ((double)i) / (sample_num[p] - 1);
      value_vector.insert(i, st_string(value_string.str()));
    }
    ostringstream par_string;
    par_string << "x_" << p;
    value_vector.set_properties("name", st_string(par_string.str()));
    dse->insert(p, value_vector);
  }
  return dse;
}

st_vector *st_dtlz::get_metrics() {
  st_vector *metrics = new st_vector();
  st_string M1("f1");
  metrics->insert(0, M1);
  st_string M2("f2");
  metrics->insert(1, M2);
  st_string M3("f3");
  metrics->insert(2, M3);
  return metrics;
}

st_vector *st_dtlz::get_statistics() { return (new st_vector()); }

st_point *st_dtlz::simulate(st_point &point, st_env *env) {
  st_vector metrics;
  st_vector stats;
  st_point *fp;

  st_point *simulated_point = new st_point(point);
  const st_vector *ds = opt_get_design_space_vector(env);

  if (point.size() != ds->size()) {
    simulated_point->set_error(ST_ERR_CONSTR_ERR);
    return simulated_point;
  }

  double M1, M2, M3;

  M1 = f1(*simulated_point);
  M2 = f2(*simulated_point);
  M3 = f3(*simulated_point);

  string problem;
  env->shell_variables.get_string("problem", problem);

  if (problem == "dtlz7") {
    if (g1(*simulated_point) < 0 || g2(*simulated_point) < 0 ||
        g3(*simulated_point) < 0) {
      simulated_point->set_error(ST_ERR_CONSTR_ERR);
      return simulated_point;
    }
  }

  metrics.insert(0, st_double(M1));
  metrics.insert(1, st_double(M2));
  metrics.insert(2, st_double(M3));

  simulated_point->set_properties("metrics", metrics);
  simulated_point->set_properties("statistics", stats);

  return simulated_point;
}

string st_dtlz::get_name() {
  string name = "dtlz";
  return name;
}

st_dtlz::st_dtlz() {
  string problem;
  prs_display_message("Loading the dtlz driver");
  if (!current_environment.shell_variables.get_string("problem", problem)) {
    prs_display_message("Please specify the 'problem' variable as {'dtlz1' ... "
                        "'dtlz7'}, defaulting to dtlz1");
    problem = "dtlz1";
    current_environment.shell_variables.set_string("problem", problem.c_str());
  }
}

int st_dtlz::get_number_of_parameters() {
  string problem;
  current_environment.shell_variables.get_string("problem", problem);
  if (problem == "dtlz1")
    return 7;
  if (problem == "dtlz2")
    return 12;
  if (problem == "dtlz3")
    return 12;
  if (problem == "dtlz4")
    return 12;
  if (problem == "dtlz5")
    return 12;
  if (problem == "dtlz6")
    return 22;
  if (problem == "dtlz7")
    return 30;
}

st_dtlz::~st_dtlz() { prs_display_message("Removing dtlz driver"); }

extern "C" {

st_dtlz *drv_generate_driver() {
  prs_display_message("Creating the dtlz driver");
  return new st_dtlz();
}
}
