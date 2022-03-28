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
#include "st_object.h"
#include "st_map.h"
#include "st_vector.h"
#include "st_shell_variables.h"
#include "st_parser.h"
#include "st_optimizer.h"
#include "st_opt_utils.h"
#include "st_sim_utils.h"

extern int interrupt_exploration;

class st_pareto_parallel_doe : public st_optimizer {
  int explored_points;

public:
  st_pareto_parallel_doe() {}
  string get_information();
  int explore(st_env *env);
};

string st_pareto_parallel_doe::get_information() {
  return "Parallel Pareto DoE optimizer - (parallel_doe_temp_database, "
         "parallel_doe_tempdb_granularity)";
}

#define DISPLAY_STATUS_EVERY 100

int st_pareto_parallel_doe::explore(st_env *env) {
  st_assert(env->current_driver);
  st_assert(env->current_doe);
  st_codi_reset_time();
  st_codi_display_p("Starting Pareto DoE parallel execution process");
  bool temporary_save = false;

  string filename;
  int gran = 10;

  if (env->shell_variables.get_string("parallel_doe_temp_database", filename))
    temporary_save = true;

  if (!env->shell_variables.get_integer("parallel_doe_tempdb_granularity",
                                        gran)) {
    gran = 10;
    env->shell_variables.set_integer("parallel_doe_tempdb_granularity", gran);
  }
  int par;

  par = st_mpi_get_number_of_nodes();

  st_database *P = env->get_pre_populated_database();

  explored_points = 0;

  st_vector *doe = env->current_doe->generate_doe(env);
  int doe_point = 0;

  while (doe_point < doe->size()) {
    if (interrupt_exploration == 1) {
      interrupt_exploration = 0;
      delete doe;
      return explored_points;
    }
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
        st_assert(sp->check_consistency(env));
        if (!sp->get_error()) {
          explored_points++;
          if ((explored_points % DISPLAY_STATUS_EVERY) == 0) {
            st_codi_display_ep(((float)explored_points) / ((float)doe->size()),
                               "Executing DoE.");
          }

          if (sim_recompute_pareto_with_new_point(
                  env, P, dynamic_cast<st_point &>(*sp)))
            P->insert_point(sp);

          if (temporary_save && ((explored_points % gran) == 0)) {
            st_codi_display_ep(((float)explored_points) / ((float)doe->size()),
                               "Writing temporary database");
            env->source_database->write_to_file(filename.c_str());
          }
        }
        delete sp;
      }
    }
  }
  delete doe;
  env->insert_new_database(P, env->current_destination_database_name);
  return explored_points;
}

extern "C" {
st_optimizer *opt_generate_optimizer() { return new st_pareto_parallel_doe(); }
st_command *get_help() {
  const char *ref[] = {"parallel_doe_temp_database",
                       "parallel_doe_tempdb_granularity", NULL};
  const char *ref_help[] = {
      "name of the temporary database file to store incremental results",
      "number of evaluations after which the temporary database is written",
      NULL};

  st_command *help = new st_command(
      multiple_opts,
      "Once loaded with the **opt_load_optimizer** command, the algorithm is "
      "invoked by using the **opt_tune** command (see manual). "
      "This algorithm evaluates all the designs specified by the current DoE "
      "(as defined by **doe_load_doe**). "
      "If the problem objectives have been specified, at every evaluation "
      "dominated designs are removed from the destination database "
      "(i.e., only the Pareto frontier is maintained in the database). "
      "All evaluated designs are stored in the destination database specified "
      "as the **opt_tune** command. "
      "When MOST is run with MPI on parallel nodes, evaluations are performed "
      "concurrently. The number of concurrent evaluations is"
      " equal to the number of MPI nodes specified in the command line. "
      "A temporary database to store the incremental results of the "
      "evaluations can be specified with ^opt(0).",
      "", ref, ref_help,
      "This algorithm evaluates all the designs specified by the current DoE "
      "(as defined by **doe_load_doe**) by maintaining only non-dominated "
      "points (when objectives have been specified). ",
      STDRET);
  help->alt_command_name = "Parallel Pareto DoE.";
  help->alt_command_synopsys = "opt_load_optimizer st_parallel_pareto_doe";
  help->alt_command_type = "opt";

  return help;
};
}
