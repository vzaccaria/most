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

class st_full_search : public st_doe {
public:
  st_full_search() {}
  string get_information();
  st_vector *generate_doe(st_env *env);
};

string st_full_search::get_information() { return "Full search DoE"; }

st_vector *st_full_search::generate_doe(st_env *env) {
  st_assert(env->current_driver);
  prs_display_message("Generating full search DoE");
  st_vector *doe = new st_vector();

  /* the following is very heuristic */
  bool finished = false;

  st_point actual_point = env->current_design_space->begin(env);

  int n = 0;

  while (!finished) {
    doe->insert(n, actual_point);
    finished = !env->current_design_space->next(env, actual_point);
    n++;
  }
  return doe;
}

extern "C" {
st_doe *doe_generate_doe() { return new st_full_search(); }
st_command *get_help() {
  const char *ref[] = {NULL};
  const char *ref_help[] = {NULL};

  st_command *help = new st_command(
      multiple_opts,
      "This design of experiments generates all the designs of the design "
      "space (by taking into account optional boundaries for each parameter). ",
      "", ref, ref_help,
      "This design of experiments generates all the designs of the design "
      "space.",
      STDRET);
  help->alt_command_name = "Full Search";
  help->alt_command_synopsys = "doe_load_doe st_full_search";
  help->alt_command_type = "doe";

  return help;
};
}
