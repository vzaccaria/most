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

class st_full_factorial_extended : public st_doe {
public:
  st_full_factorial_extended() {}
  string get_information();
  st_vector *generate_doe(st_env *env);
};

string st_full_factorial_extended::get_information() {
  return "Full factorial DoE";
}

st_vector *st_full_factorial_extended::generate_doe(st_env *env) {
  st_assert(env->current_driver);
  prs_display_message("Generating full factorial DoE");

  st_vector *doe = new st_vector();

  env->current_design_space->add_full_factorial_extended_designs(env, doe);

  return doe;
}

extern "C" {
st_doe *doe_generate_doe() { return new st_full_factorial_extended(); }
st_command *get_help() {
  const char *ref[] = {NULL};
  const char *ref_help[] = {NULL};

  st_command *help = new st_command(
      multiple_opts,
      "In statistics, a two-level full factorial experiment is an experiment "
      "whose design consists of two or more parameters, each with discrete "
      "possible values or **levels** and whose experimental units take on all "
      "possible combinations of the minimum and maximum levels for such "
      "parameters. Such an experiment enables the evaluation of the effects of "
      "each parameter on the response variable, as well as the effects of "
      "interactions between parameters on the response variable. ",
      "", ref, ref_help,
      "This design of experiments generates Full Factorial Extended design of "
      "experiments. It is similar to the plain Full factorial design but with "
      "a different twist on the generation of opposite values for permutations "
      "and masks. ",
      STDRET);
  help->alt_command_name = "Full Factorial Design of Experiments Extended";
  help->alt_command_synopsys = "doe_load_doe st_full_factorial_extended";
  help->alt_command_type = "doe";

  return help;
};
}
