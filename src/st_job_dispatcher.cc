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
#include <dlfcn.h>
#include <fstream>
#include <iostream>
#include <pthread.h>
#include <regex.h>
#include "st_job_dispatcher.h"
#include "st_list.h"
#include "st_map.h"
#include "st_parser.h"
#include "st_point.h"
#include "st_sim_utils.h"
#include "st_vector.h"

pthread_key_t st_key_to_thread_state;

#define MAXSTRING 200

/** Hooks to external functions */
extern int prs_command_drv_load_driver(string n);

void st_update_add_to_kernel_realistic_simulations(st_env *env, int n);
void st_update_add_to_kernel_evaluations(st_env *env, int n);
void st_update_add_to_kernel_root_hit(st_env *env, int n);

bool realistic_simulation_mode = false;

void st_set_realistic_simulation_mode(bool mode) {
  if (mode) {
    prs_display_message("Setting realistic simulation profiling. Updating "
                        "'kernel_realistic_simulations' variable.");
    realistic_simulation_mode = true;
    st_database *root = current_environment.source_database;
    if (root != NULL) {
      st_point_set *root_list = root->get_set_of_points();
      st_point_set::iterator pi;

      for (pi = root_list->begin(); pi != root_list->end(); pi++) {
        st_point &point = *(pi->second);
        point.set_sims(0);
      }
    }
  } else {
    realistic_simulation_mode = false;
  }
}

class st_thread_state {
public:
  st_point *current_point;
  list<st_object *>::iterator iterator_to_current_point;
  int thread_index;
  int node;
  pthread_t *pthread_key;
  st_env *env;
};

st_thread_state *st_thread_get_thread_state();

void *st_thread_body(void *the_thread_state) {
  st_thread_state *current_state = (st_thread_state *)the_thread_state;

  pthread_setspecific(st_key_to_thread_state, the_thread_state);

  current_state->current_point = current_state->env->current_driver->simulate(
      *current_state->current_point, current_state->env);

  current_state->current_point->clear_cache();

  /** Thread ended */
  return NULL;
}

st_thread_state *st_thread_get_thread_state() {
  st_thread_state *current_state =
      ((st_thread_state *)pthread_getspecific(st_key_to_thread_state));

  return current_state;
}

#ifndef ST_NO_THREADS
string st_get_unique_string_identifier() {
  string s;
  char tmp[MAXSTRING];
  /*
   * As of March 2010, each node executes simulations serially so thread_index
   * will always be 0. Simultaneously, the node will be directly derived with
   * st_mpi_get_node()
   */
  sprintf(tmp, "%d.%d", st_mpi_get_node(), 0);
  s = tmp;
  return s;
}
#else
string st_get_unique_string_identifier() {
  string s;
  char tmp[MAXSTRING];
  sprintf(tmp, "%d", st_mpi_get_node());
  s = tmp;
  return s;
}
#endif

#define ST_RUN_SERIALIZED_THREADS 0
#define ST_RUN_PARALLEL_THREADS 1

struct st_thread_pool {
  vector<st_thread_state> tsv;
  st_batch_job *the_current_batch_job;
  st_thread_pool(st_batch_job *s) { the_current_batch_job = s; }
  bool run_parallel(st_env *env, int mode) {
    int return_value;
    pthread_attr_t attr;
    pthread_t *pt;

    if (pthread_key_create(&(st_key_to_thread_state), NULL)) {
      prs_display_message("Internal error during parallelization. Unable to "
                          "create thread specific data.");
      return false;
    }

    tsv.resize(the_current_batch_job->list_of_points.size());
    int i = 0;
    list<st_object *>::iterator k;

    for (k = the_current_batch_job->list_of_points.begin();
         k != the_current_batch_job->list_of_points.end(); k++) {
      pthread_attr_init(&attr);
      pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
      pthread_attr_setschedpolicy(&attr, SCHED_FIFO);

      pt = (pthread_t *)malloc(sizeof(pthread_t));

      tsv[i].current_point = to<st_point *>((*k));
      tsv[i].iterator_to_current_point = k;
      tsv[i].thread_index = i;
      tsv[i].node = st_mpi_get_node();

      tsv[i].pthread_key = pt;
      tsv[i].env = env;

      /* Dont invoke threads. It seems libXML2 is broken at this time. */

      st_thread_body(&tsv[i]);

      /*
      return_value = pthread_create(pt, &attr, st_thread_body, &tsv[i]);
      if(mode==ST_RUN_SERIALIZED_THREADS)
          pthread_join(*tsv[i].pthread_key, NULL);
      */

      i++;
    }
    for (i = 0; i < the_current_batch_job->list_of_points.size(); i++) {
      /*
      if(mode==ST_RUN_PARALLEL_THREADS)
         pthread_join(*tsv[i].pthread_key, NULL);
      */

      delete *(tsv[i].iterator_to_current_point);
      *(tsv[i].iterator_to_current_point) = tsv[i].current_point;
      // cout << "Job received " << tsv[i].current_point << endl;
      free(tsv[i].pthread_key);
    }
    pthread_key_delete(st_key_to_thread_state);
    return true;
  }
#ifndef ST_NO_THREADS
  bool run(st_env *env) {
    /*
     * March 2010, VZ: each node should execute simulations serially.
    if(env->current_driver->is_thread_safe())
        return run_parallel(env, ST_RUN_PARALLEL_THREADS);
    else
    */
    return run_parallel(env, ST_RUN_SERIALIZED_THREADS);
  }
#else
  bool run(st_env *env) { return run_no_threads(env); }
#endif
  bool run_no_threads(st_env *env) {
    list<st_object *>::iterator k;
    st_point *current_point;
    for (k = the_current_batch_job->list_of_points.begin();
         k != the_current_batch_job->list_of_points.end(); k++) {
      *k = env->current_driver->simulate(*to<st_point *>((*k)), env);
    }
    return true;
  }
};

bool st_job_dispatcher::submit(st_env *env, st_batch_job *the_job) {
  list<st_object *>::iterator k;
  st_batch_job found_in_root_db;
  vector<st_batch_job> tbjv;
  tbjv.resize(st_mpi_get_number_of_nodes());
  unsigned int i = 0;

  for (k = the_job->list_of_points.begin(); k != the_job->list_of_points.end();
       k++) {
    int dont_look = 0;
    st_update_add_to_kernel_evaluations(env, 1);

    env->shell_variables.get_integer("dont_look_into_source_database",
                                     dont_look);
    if (!dont_look) {
      st_point *fp;
      st_point *cp = to<st_point *>(*k);
      // cout << "Looking into: " << env->source_database << " for " <<
      // cp->print();

      if ((fp = env->source_database->look_for_point(cp))) {
        // cout << ": FOUND" << endl;
        found_in_root_db.list_of_points.insert(*fp);
        if (realistic_simulation_mode) {
          int sims = fp->get_sims();
          // cout << "SRC: " << env->source_database << " P: " << fp << " " <<
          // fp->print() << " " << sims << endl;
          if (sims == 0) {
            st_update_add_to_kernel_root_hit(env, 0);
            sims = 1;
            fp->set_sims(sims);
            st_update_add_to_kernel_realistic_simulations(env, 1);
          } else {
            st_update_add_to_kernel_root_hit(env, 1);
            sims++;
            fp->set_sims(sims);
          }
        } else {
          st_update_add_to_kernel_root_hit(env, 1);
          st_update_add_to_kernel_realistic_simulations(env, 0);
        }
        continue;
      } else {
        // cout << ": NOT FOUND" << endl;
        /** The point should be simulated */
        tbjv[i].list_of_points.insert(**k);
        i = (i + 1) % st_mpi_get_number_of_nodes();
        st_update_add_to_kernel_root_hit(env, 0);
        st_update_add_to_kernel_realistic_simulations(env, 1);
      }
    } else {
      /** Dont look into root, create batch jobs in round robin for all the
       * points.*/
      tbjv[i].list_of_points.insert(**k);
      i = (i + 1) % st_mpi_get_number_of_nodes();
      st_update_add_to_kernel_root_hit(env, 0);
      st_update_add_to_kernel_realistic_simulations(env, 1);
    }
  }
  /** Dispatch the jobs */
  for (i = 1; i < st_mpi_get_number_of_nodes(); i++) {
    st_mpi_send_command("EXECUTE_BATCH_JOB", i);
    tbjv[i].send_to_node(i);
  }
  st_thread_pool pool(&tbjv[0]);
  pool.run(env);
  the_job->clear();
  the_job->list_of_points.concat(&tbjv[0].list_of_points);
  for (i = 1; i < st_mpi_get_number_of_nodes(); i++) {
    tbjv[i].receive_from_node(i);
    the_job->list_of_points.concat(&tbjv[i].list_of_points);
  }
  for (k = the_job->list_of_points.begin(); k != the_job->list_of_points.end();
       k++) {
    if (*k) {
      // cout << "Inserting point " << (*k)->print_canonical() << endl;
      env->source_database->insert_point(to<st_point *>(*k));
    }
  }
  the_job->list_of_points.concat(&found_in_root_db.list_of_points);
  return true;
}

bool st_job_receiver::run(st_env *env) {
  bool finished = false;
  string log_file =
      "most.node." + st_mpi_get_string_representation_of_node() + ".log";

  ofstream lout(log_file.c_str());

  while (!finished) {
    lout << prs_get_time() << ": Waiting for command " << endl;
    string command = st_mpi_receive_command(0);
    lout << prs_get_time() << ": Command received - " << command << endl;
    if (command == "QUIT") {
      if (env->current_driver != NULL) {
        void *handle = env->current_driver->handle;
        delete env->current_driver;
        dlclose(handle);
      }
      finished = true;
      st_mpi_end();
      lout << prs_get_time() << ": QUIT" << endl;
      lout.close();
      exit(0);
    } else {
      if (command == "SYNCHRONIZE_SHELL_VARIABLES") {
        lout << prs_get_time() << ": SYNC " << endl;
        string to_be_parsed = st_mpi_receive_data(0);
        string file_name =
            "tmp_shell_var_" + st_mpi_get_string_representation_of_node();
        ofstream fout(file_name.c_str());
        fout << to_be_parsed;
        fout.close();
        prs_parse_and_execute_file(file_name.c_str());
        string cleanup = "rm -f " + file_name;
        shell_command(cleanup);
      }
      if (command == "LOAD_DRIVER") {
        lout << prs_get_time() << ": LOAD DRIVER " << endl;
        string driver_name = st_mpi_receive_data(0);
        bool su = prs_command_drv_load_driver(driver_name);
        lout << prs_get_time() << ": LOAD DRIVER " << driver_name << " = " << su
             << endl;
      }
      if (command == "EXECUTE_BATCH_JOB") {
        lout << prs_get_time() << ": BATCH JOB RECEIVED" << endl;
        st_batch_job current_batch;
        current_batch.receive_from_node(0);
        st_thread_pool pool(&current_batch);
        lout << prs_get_time() << ": BATCH JOB SIZE " << current_batch.size()
             << endl;
        pool.run(env);
        current_batch.send_to_node(0);
        lout << prs_get_time() << ": BATCH JOB END " << current_batch.size()
             << endl;
      }
    }
  }
  return true;
}

void st_update_add_to_kernel_evaluations(st_env *env, int n) {
  int number;
  if (!env->shell_variables.get_integer("kernel_evaluations_number", number)) {
    number = n;
    env->shell_variables.set_integer("kernel_evaluations_number", number);
  } else {
    number += n;
    env->shell_variables.set_integer("kernel_evaluations_number", number);
  }
}

void st_update_add_to_kernel_realistic_simulations(st_env *env, int n) {
  int number;
  if (!env->shell_variables.get_integer("kernel_realistic_simulations",
                                        number)) {
    number = n;
    env->shell_variables.set_integer("kernel_realistic_simulations", number);
  } else {
    number += n;
    env->shell_variables.set_integer("kernel_realistic_simulations", number);
  }
}

void st_update_add_to_kernel_root_hit(st_env *env, int n) {
  int number;
  if (!env->shell_variables.get_integer("kernel_root_hits", number)) {
    number = n;
    env->shell_variables.set_integer("kernel_root_hits", number);
  } else {
    number += n;
    env->shell_variables.set_integer("kernel_root_hits", number);
  }
}
