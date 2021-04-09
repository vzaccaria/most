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

class st_box_behnken : public st_doe {
public:
  st_box_behnken() {}
  string get_information();
  st_vector *generate_doe(st_env *env);
};

string st_box_behnken::get_information() { return "Box behnken DoE"; }

vector<string> center_representation(int n) {
  vector<string> repr(n);
  for (int i = 0; i < n; i++)
    repr[i] = "0";
  return repr;
}

st_vector *st_box_behnken::generate_doe(st_env *env) {
  st_assert(env->current_driver);
  prs_display_message("Generating Box Behnken DoE");
  st_vector *doe = new st_vector();

  int num_of_prs = env->current_design_space->ds_parameters.size();

  if (num_of_prs < 2) {
    prs_display_error(
        "Too few parameters, at least two parameters are needed for this DoE.");
    return NULL;
  }
  int n = 0;

  for (int first = 0; first < (num_of_prs - 1); first++) {
    for (int second = first + 1; second < (num_of_prs - 1); second++) {
      vector<string> apr = center_representation(num_of_prs);
      if (first != second) {
        st_point actual_point;
        apr[first] = "-";
        apr[second] = "-";
        env->current_design_space->convert_factorial_representation(
            env, actual_point, apr, TWO_LEVEL_FF_MODE_CLASSIC);
        env->current_design_space->print_factorial_design(env, apr, n);
        doe->insert(n++, actual_point);
        apr[first] = "-";
        apr[second] = "+";
        env->current_design_space->convert_factorial_representation(
            env, actual_point, apr, TWO_LEVEL_FF_MODE_CLASSIC);
        env->current_design_space->print_factorial_design(env, apr, n);
        doe->insert(n++, actual_point);
        apr[first] = "+";
        apr[second] = "-";
        env->current_design_space->convert_factorial_representation(
            env, actual_point, apr, TWO_LEVEL_FF_MODE_CLASSIC);
        env->current_design_space->print_factorial_design(env, apr, n);
        doe->insert(n++, actual_point);
        apr[first] = "+";
        apr[second] = "+";
        env->current_design_space->convert_factorial_representation(
            env, actual_point, apr, TWO_LEVEL_FF_MODE_CLASSIC);
        env->current_design_space->print_factorial_design(env, apr, n);
        doe->insert(n++, actual_point);
      }
    }
  }
  return doe;
}

extern "C" {
st_doe *doe_generate_doe() { return new st_box_behnken(); }
st_command *get_help() {
  const char *ref[] = {NULL};
  const char *ref_help[] = {NULL};

  st_command *help = new st_command(
      multiple_opts,
      "This design of experiments generates design by using the Box Behnken "
      "technique. The Box–Behnken DoE is suitable for constructing RSM "
      "quadratic models where parameter combinations that are at the center of "
      "the edges of the design space in addition to a design with all the "
      "parameters at the center. The main advantage is that the parameter "
      "combinations avoid taking extreme values taken at the same time (in "
      "contrast with the central composite design). This may be suitable to "
      "avoid singular points in the generation of the response surface, which "
      "would deteriorate it. ",
      "", ref, ref_help,
      "This design of experiments generates design by using the Box Behnken "
      "technique. ",
      STDRET);
  help->alt_command_name = "Box Behnken Design of Experiments";
  help->alt_command_synopsys = "doe_load_doe st_box_behnken";
  help->alt_command_type = "doe";

  return help;
};
}
