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
#include <st_object.h>
#include <st_map.h>
#include <st_vector.h>
#include <st_shell_variables.h>
#include <st_parser.h>
#include <st_optimizer.h>
#include <st_doe.h>
#include <st_opt_utils.h>

class st_placket_burman : public st_doe {
public:
  st_placket_burman() {}
  string get_information();
  st_vector *generate_doe(st_env *env);
};

string st_placket_burman::get_information() { return "Box behnken DoE"; }

vector<string> center_representation(int n) {
  vector<string> repr(n);
  for (int i = 0; i < n; i++)
    repr[i] = "0";
  return repr;
}

st_vector *st_placket_burman::generate_doe(st_env *env) {
  st_assert(env->current_driver);
  prs_display_message("Generating Placket-Burman DoE");
  st_vector *doe = new st_vector();

  int num_of_prs = env->current_design_space->ds_parameters.size();

  if (num_of_prs < 2) {
    prs_display_error(
        "Too few parameters, at least two parameters are needed for this DoE.");
    return NULL;
  }
  int n = 0;

  // vector<string> apr;
  vector<string> apr = center_representation(num_of_prs);
  string tmp_apr;
  st_point actual_point;
  int additional_experiments;

  if (num_of_prs < 12) {
    additional_experiments = 11;
    tmp_apr = "-+-+++---+-";
  } else if (num_of_prs < 20) {
    additional_experiments = 19;
    tmp_apr = "-+--++++-+-+----++-";
  } else if (num_of_prs < 24) {
    additional_experiments = 23;
    tmp_apr = "-++++-+-++--++--+-+----";
  } else {
    prs_display_error("Too many parameters! Value not supported by this DoE.");
    return NULL;
  }

  for (int i_params = 0; i_params < num_of_prs; i_params++) {
    apr[i_params] = "+";
  }
  env->current_design_space->convert_factorial_representation(
      env, actual_point, apr, TWO_LEVEL_FF_MODE_CLASSIC);
  env->current_design_space->print_factorial_design(env, apr, n);
  doe->insert(n++, actual_point);

  for (int i_params = 0; i_params < num_of_prs; i_params++) {
    apr[i_params] = "-";
  }
  env->current_design_space->convert_factorial_representation(
      env, actual_point, apr, TWO_LEVEL_FF_MODE_CLASSIC);
  env->current_design_space->print_factorial_design(env, apr, n);
  doe->insert(n++, actual_point);

  for (int i_experiments = 0; i_experiments < additional_experiments;
       i_experiments++) {
    for (int i_params = 0; i_params < num_of_prs; i_params++) {
      apr[i_params] =
          tmp_apr[(i_params + i_experiments) % additional_experiments];
    }
    env->current_design_space->convert_factorial_representation(
        env, actual_point, apr, TWO_LEVEL_FF_MODE_CLASSIC);
    env->current_design_space->print_factorial_design(env, apr, n);
    doe->insert(n++, actual_point);
  }

  return doe;
}

extern "C" {
st_doe *doe_generate_doe() { return new st_placket_burman(); }
st_command *get_help() {
  const char *ref[] = {NULL};
  const char *ref_help[] = {NULL};

  st_command *help = new st_command(
      multiple_opts,
      " Plackett–Burman DoEs are experimental designs presented in 1946 by "
      "Robin L. Plackett and J. P. Burman. Their goal was to find experimental "
      "designs for investigating the dependence of some measured quantity on a "
      "number of independent variables (factors), each taking L levels, in "
      "such a way as to minimize the variance of the estimates of these "
      "dependencies using a limited number of experiments. Interactions "
      "between the factors were considered negligible. The solution to this "
      "problem is to find an experimental design where each combination of "
      "levels for any pair of factors appears the same number of times, "
      "throughout all the experimental runs (refer table). A complete "
      "factorial design would satisfy this criterion, but the idea was to find "
      "smaller design. ",
      "", ref, ref_help,
      "This design of experiments generates design by using the PLACKETT "
      "BURMAN technique. ",
      STDRET);
  help->alt_command_name = "Placket Burman Design of Experiments";
  help->alt_command_synopsys = "doe_load_doe st_placket_burman";
  help->alt_command_type = "doe";

  return help;
};
}
