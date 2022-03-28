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
#include <st_opt_utils.h>
#include <math.h>
#include <st_doe.h>
#include <st_vector.h>
#include <set>

class st_neighbor : public st_doe {
public:
  st_neighbor() {}
  ~st_neighbor() {}
  string get_information();
  st_vector *generate_doe(st_env *env);
};

string st_neighbor::get_information() {
  return "Neighborhood DoE - (neighbor_starting_point, neighbor_range)";
}

st_vector *st_neighbor::generate_doe(st_env *env) {
  st_assert(env->current_driver);
  prs_display_message("Generating neighbor DoE");

  st_vector *doe = new st_vector();

  vector<st_point> near_points;

  st_object const *start_var;

  st_point start;

  if (!env->shell_variables.get("neighbor_starting_point", start_var)) {
    start = env->current_design_space->random_point(env);
  } else {
    if (is_a<st_point const *>(start_var))
      start = *(to<st_point const *>(start_var));
    else
      start = env->current_design_space->random_point(env);
  }
  int range;
  int n_solutions;

  if (!env->shell_variables.get_integer("neighbor_range", range))
    range = 1;

  env->current_design_space->get_points_at_distance_N(env, &start, range,
                                                      near_points);

  for (int i = 0; i < near_points.size(); i++) {
    doe->insert(i, near_points[i]);
  }

  return doe;
}

extern "C" {
st_doe *doe_generate_doe() { return new st_neighbor(); }
st_command *get_help() {
  const char *ref[] = {"neighbor_starting_point", "neighbor_range", NULL};
  const char *ref_help[] = {
      "Design point to be considered (see the overall documentation)",
      "Neighborhood of the design point", NULL};

  st_command *help = new st_command(
      multiple_opts,
      "This design of experiments generates all the designs in the "
      "neighborhood range ^opt(1) of a design point ^opt(0). ",
      "", ref, ref_help,
      "This design of experiments generates all the designs in the "
      "neighborhood of a design point.",
      STDRET);
  help->alt_command_name = "Neighborhood Design of Experiments";
  help->alt_command_synopsys = "doe_load_doe st_neighbor";
  help->alt_command_type = "doe";

  return help;
};
}
