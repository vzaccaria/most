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
#include <st_opt_utils.h>
#include <math.h>
#include <time.h>
#include <st_doe.h>
#include <st_vector.h>
#include <st_rand.h>

class st_random : public st_doe {
public:
  st_random() {}
  ~st_random() {}
  string get_information();
  st_vector *generate_doe(st_env *env);
};

string st_random::get_information() {
  return "Random DoE with effect - (random_doe_solutions_number)";
}

inline bool is_in_vector(st_vector *doe, st_point *actual_point) {
  for (int index = 0; index < doe->size(); index++) {
    st_point *current_point = to<st_point *>(doe->get(index).gen_copy());
    if ((*current_point) == (*actual_point))
      return true;
  }
  return false;
}

st_vector *st_random::generate_doe(st_env *env) {
  st_assert(env->current_driver);
  bool effect = false;
  string effect_par;
  st_string fp("");

  prs_display_message("Generating random DoE with effect");

  st_vector *doe = new st_vector();

  st_point actual_point;

  int n_solutions;

  if (env->shell_variables.get_string("random_doe_effect", effect_par)) {
    if (env->current_design_space->scalar_parameters.count(effect_par))
      effect = true;
  }

  if (!env->shell_variables.get_integer("random_doe_solutions_number",
                                        n_solutions)) {
    n_solutions = 10;
    env->shell_variables.set_integer("random_doe_solutions_number",
                                     n_solutions);
  }

  /*FIXME - verify that the solutions number is < than design space size*/

  bool no_replicate = false;
  int number;
  if (env->shell_variables.get_integer("random_doe_no_replicate", number)) {
    if (number == 1)
      no_replicate = true;
  }

  for (int points = 0; points < n_solutions; points++) {
    if (no_replicate) {
      do {
        actual_point = env->current_design_space->random_point(env);
        doe->insert(points, actual_point);
      } while (is_in_vector(doe, &actual_point));
    } else {
      if (effect) {
        actual_point = env->current_design_space->random_point(env);

        actual_point[env->current_design_space
                         ->ds_parameters_index[effect_par]] =
            env->current_design_space->get_scalar_min(env, effect_par);
        doe->insert(points++, actual_point);

        st_point n = actual_point;
        actual_point[env->current_design_space
                         ->ds_parameters_index[effect_par]] =
            env->current_design_space->get_scalar_max(env, effect_par);
        doe->insert(points, actual_point);

      } else {
        actual_point = env->current_design_space->random_point(env);
        doe->insert(points, actual_point);
      }
    }
  }
  return doe;
}

extern "C" {
st_doe *doe_generate_doe() { return new st_random(); }
st_command *get_help() {
  const char *ref[] = {"random_doe_solutions_number", "random_doe_no_replicate",
                       "random_doe_effect", NULL};
  const char *ref_help[] = {
      "Number of random design points to be generated",
      "If true, no replication is allowed",
      "Specified a scalar parameter for which both high and low levels should "
      "be considered. Only if random_doe_no_replicate is not specified.",
      NULL};

  st_command *help = new st_command(
      multiple_opts,
      "This design of experiments generates ^opt(0) design points out of the "
      "design space. You can specify also an effect parameter for which both "
      "high and low levels should be generated. Useful for db_plot "
      "--effects=parameter",
      "", ref, ref_help,
      "This design of experiments generates a set of random design points.",
      STDRET);
  help->alt_command_name = "Random Design of Experiments";
  help->alt_command_synopsys = "doe_load_doe st_random";
  help->alt_command_type = "doe";

  return help;
};
}
