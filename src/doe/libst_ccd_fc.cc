/* @STSHELL_LICENSE_START@
 *
 *      __  _______  ___________ 
 *     /  |/  / __ \/ ___/_  __/ 
 *    / /|_/ / / / /\__ \ / /    
 *   / /  / / /_/ /___/ // /    
 *  /_/  /_/\____//____//_/     
 * 
 * Multi-Objective System Tuner 
 * Copyright (c) 2007-2022 Politecnico di Milano
 * 
 * Development leader: Vittorio Zaccaria
 * Main developers: Vittorio Zaccaria, Gianluca Palermo, Fabrizio Castro
 *  
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

class st_ccd_fc : public st_doe {
public:
  st_ccd_fc() {}
  string get_information();
  st_vector *generate_doe(st_env *env);
};

string st_ccd_fc::get_information() {
  return "Face centered central composite DoE";
}

vector<string> ccd_center_representation(int n) {
  vector<string> repr(n);
  for (int i = 0; i < n; i++)
    repr[i] = "0";
  return repr;
}

st_vector *st_ccd_fc::generate_doe(st_env *env) {
  st_assert(env->current_driver);
  prs_display_message("Generating Central Composite Design Face-Centered DoE");
  st_vector *doe = new st_vector();

  env->current_design_space->add_full_factorial_designs(env, doe);

  vector<string> apr;
  st_point actual_point;

  int num_of_prs = env->current_design_space->ds_parameters.size();

  apr = ccd_center_representation(num_of_prs);
  env->current_design_space->convert_factorial_representation(
      env, actual_point, apr, TWO_LEVEL_FF_MODE_CLASSIC);
  env->current_design_space->print_factorial_design(env, apr, doe->size());

  doe->insert(doe->size(), actual_point);

  for (int i = 0; i < num_of_prs; i++) {
    st_point actual_point;
    apr = ccd_center_representation(num_of_prs);

    apr[i] = "-";
    env->current_design_space->convert_factorial_representation(
        env, actual_point, apr, TWO_LEVEL_FF_MODE_CLASSIC);
    env->current_design_space->print_factorial_design(env, apr, doe->size());
    doe->insert(doe->size(), actual_point);
    apr[i] = "+";
    env->current_design_space->convert_factorial_representation(
        env, actual_point, apr, TWO_LEVEL_FF_MODE_CLASSIC);
    env->current_design_space->print_factorial_design(env, apr, doe->size());
    doe->insert(doe->size(), actual_point);
  }
  return doe;
}

extern "C" {
st_doe *doe_generate_doe() { return new st_ccd_fc(); }
st_command *get_help() {
  const char *ref[] = {NULL};
  const char *ref_help[] = {NULL};

  st_command *help = new st_command(
      multiple_opts,
      "A central composite design is an experimental design specifically "
      "targeted to the construction of response surfaces of the second order "
      "(quadratic) without requiring a three-level full or fractional "
      "factorial DoE. The design consists of the following three distinct sets "
      "of experimental runs: 1) a two-level full or fractional factorial "
      "design; 2) a set of center points, i.e., experimental runs whose values "
      "of each parameter are the medians of the values used in the factorial "
      "portion; and 3) a set of axial points, i.e., experimental runs that are "
      "identical to the center points except for one parameter. In the general "
      "central composite design, this parameter will take on values both below "
      "and above the median of the two levels. ",
      "", ref, ref_help,
      "This design of experiments generates design by using the Central "
      "Composite Design technique. ",
      STDRET);
  help->alt_command_name = "Central Composite Design Face-Centered DoE";
  help->alt_command_synopsys = "doe_load_doe st_ccd_fc";
  help->alt_command_type = "doe";

  return help;
};
}
