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

#include <iostream>
#include "st_design_space.h"
#include "st_object.h"
#include "st_map.h"
#include "st_vector.h"
#include "st_shell_variables.h"
#include "st_parser.h"
#include "st_optimizer.h"
#include "st_opt_utils.h"
#include "st_sim_utils.h"
#include <math.h>

extern int interrupt_exploration;

class st_parallel_steepest : public st_optimizer {
  int explored_points;

public:
  st_parallel_steepest() {}
  string get_information();
  int explore(st_env *env);
};

string st_parallel_steepest::get_information() {
  return "Parallel Steepest Descent Optimizer - (parallel_instances, "
         "target_objective)";
}

int st_parallel_steepest::explore(st_env *env) {
  st_assert(env->current_driver);
  st_assert(env->current_doe);
  prs_display_message(
      "Starting with the parallel steepest optimization process");

  bool temporary_save = false;

  string filename;
  int gran = 10;

  int par;

  if (!env->shell_variables.get_integer("parallel_instances", par)) {
    par = st_mpi_get_number_of_nodes();
    env->shell_variables.set_integer("parallel_instances", par);
  }

  string target_obj;
  int index;
  if (!env->shell_variables.get_string("target_objective", target_obj)) {
    prs_display_message("Please define a single feasible target objective");
    if (env->optimization_objectives.size() >= 1) {
      target_obj = env->optimization_objectives[0]->name;
      prs_display_message(
          (string("Assuming ") + target_obj + string(" as a target objective"))
              .c_str());
    } else {
      return 0;
    }
  }
  try {
    index = env->get_objective_index(target_obj);
  } catch (exception e) {
    prs_display_error("Target objective not found");
    return 0;
  }

  st_vector *doe = env->current_doe->generate_doe(env);
  st_database *steepest_db = new st_database();
  explored_points = 0;
  int doe_point = 0;

  bool optimal_point_found = false;
  double min_optimal_objf = INFINITY;
  unsigned int min_rank = 65535;
  double min_penalty = INFINITY;

  st_point current_point;

  while (doe_point < doe->size()) {
    int current_job_size = 0;
    st_batch_job job;
    while (current_job_size < par && doe_point < doe->size()) {
      job.list_of_points.insert(doe->get(doe_point));
      current_job_size++;
      doe_point++;
    }
    env->current_dispatcher->submit(env, &job);
    for (int i = 0; i < current_job_size; i++) {
      st_point *sp = job.get_point_at(i);
      if (sp) {
        int rank;
        double penalty;
        if (!sp->get_error()) {
          if (opt_check_constraints(*sp, env, rank, penalty)) {
            min_rank = 0;
            min_penalty = 0.0;
            double objf = env->optimization_objectives[index]->eval(sp, index);
            if (objf < min_optimal_objf) {
              printf(
                  "Cost function %10f, constraints violated %d, penalty %10f\n",
                  objf, min_rank, min_penalty);
              min_optimal_objf = objf;
              optimal_point_found = true;
              current_point = *sp;
            }
          } else {
            if (rank <= min_rank && penalty < min_penalty) {
              double objf =
                  env->optimization_objectives[index]->eval(sp, index);
              min_rank = rank;
              min_penalty = penalty;
              optimal_point_found = true;
              current_point = *sp;
              printf(
                  "Cost function %10f, constraints violated %d, penalty %10f\n",
                  objf, min_rank, min_penalty);
            }
          }
        }
        steepest_db->insert_point(sp);
        explored_points++;
        delete sp;
      }
    }
  }
  delete doe;

  if (!optimal_point_found) {
    prs_display_error("Sorry, cannot find a suitable starting point for the "
                      "steepest descent");
    return explored_points;
  }

  bool minimum_not_found = true;

  while (minimum_not_found) {
    minimum_not_found = false;
    st_batch_job job;
    st_point best_point;

    vector<st_point> adjacent_points;

    env->current_design_space->get_closest_points(env, &current_point,
                                                  adjacent_points);

    for (int i = 0; i < adjacent_points.size(); i++) {
      job.list_of_points.insert(adjacent_points[i]);
    }
    env->current_dispatcher->submit(env, &job);
    for (int i = 0; i < job.size(); i++) {
      st_point *sp = job.get_point_at(i);
      // cout << "Evaluating point at " << sp->print_canonical() << endl;
      if (sp) {
        int rank;
        double penalty;
        if (!sp->get_error()) {
          if (opt_check_constraints(*sp, env, rank, penalty)) {
            min_rank = 0;
            min_penalty = 0.0;
            double objf = env->optimization_objectives[index]->eval(sp, index);
            // cout << "A: " << objf << " M: " << min_optimal_objf << endl;
            if (objf < min_optimal_objf) {
              printf(
                  "Cost function %10f, constraints violated %d, penalty %10f\n",
                  objf, min_rank, min_penalty);
              min_optimal_objf = objf;
              minimum_not_found = true;
              current_point = *sp;
            }
          } else {
            if (rank <= min_rank && penalty < min_penalty) {
              double objf =
                  env->optimization_objectives[index]->eval(sp, index);
              min_rank = rank;
              min_penalty = penalty;
              minimum_not_found = true;
              current_point = *sp;
              printf(
                  "Cost function %10f, constraints violated %d, penalty %10f\n",
                  objf, min_rank, min_penalty);
            }
          }
        }
        steepest_db->insert_point(sp);
        explored_points++;
        delete sp;
      }
    }
  }

  env->insert_new_database(steepest_db, env->current_destination_database_name);
  return explored_points;
}

extern "C" {
st_optimizer *opt_generate_optimizer() { return new st_parallel_steepest(); }
st_command *get_help() {
  const char *ref[] = {"parallel_instances", "target_objective", NULL};
  const char *ref_help[] = {
      "If specified, overrides the number of simultaneous evaluations set at "
      "the command line of MOST",
      "If multiple objectives have been set, this options specifies the one "
      "that should be minimized by the algorithm. ",
      NULL};

  st_command *help = new st_command(
      multiple_opts,
      "Once loaded with the **opt_load_optimizer** command, the algorithm is "
      "invoked by using the **opt_tune** command (see manual). "
      "This algorithm evaluates all the designs specified by the current DoE "
      "(as defined by **doe_load_doe**) and then starts to greedly "
      "move within the design space through neighborhood points trying to "
      "minimize a single objective. "
      "Either a single objective should be specified with **set_objective** or "
      "^opt(1) should be used to define the unique objective to be"
      " minimized.",
      "", ref, ref_help,
      "Greedy, single objective optimization algorithm. It first evaluates all "
      "the designs specified by the current DoE (as defined by "
      "**doe_load_doe**) and then starts to greedly move within the design "
      "space towards a local minimum. ",
      STDRET);
  help->alt_command_name = "Steepest descent";
  help->alt_command_synopsys =
      "opt_load_optimizer st_parallel_steepest_descent";
  help->alt_command_type = "opt";

  return help;
};
}
