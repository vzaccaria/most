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

#include <dlfcn.h>
#include <iostream>
#include <map>
#include <math.h>
#include "st_common_utils.h"
#include "st_objectives_constraints.h"
#include "st_opt_utils.h"
#include "st_parser.h"
#include "st_rand.h"
#include "st_sim_utils.h"

int interrupt_exploration = 0;

typedef st_optimizer *(*opt_fun_type)();

bool opt_select_optimizer(st_env *env, string const &opt_name) {
  string c_opt_name; // complete driver name
  if (st_look_for_filename_in_search_path(env, opt_name, c_opt_name)) {
    void *h = dlopen(c_opt_name.c_str(), RTLD_NOW);
    if (h) {
      opt_fun_type genopt = (opt_fun_type)dlsym(h, "opt_generate_optimizer");
      if (!genopt) {
        prs_display_error(string(dlerror()));
        return false;
      }
      env->current_optimizer = genopt();
      env->current_optimizer->handle = h;
      return true;
    } else {
      prs_display_error(string(dlerror()));
    }
  }
  return false;
}

bool opt_check_constraints(const st_point &p, st_env *env, int &rank,
                           double &penalty) {
  try {
    penalty = 1;
    rank = 0;
    bool eval = true;
    st_point *po = const_cast<st_point *>(&p);
    if (po->c_cache.optimization_timestamp == env->optimization_timestamp) {
      rank = po->c_cache.constraints_violated;
      penalty = po->c_cache.penalty;
      eval = false;
    }
    if (eval) {

      for (int i = 0; i < env->optimization_constraints.size(); i++) {
        double local_penalty;
        if (env->optimization_constraints[i]->violated(po, local_penalty)) {
          rank++;
          penalty = penalty * local_penalty;
        }
      }
      po->c_cache.optimization_timestamp = env->optimization_timestamp;
      po->c_cache.constraints_violated = rank;
      if (!rank)
        po->c_cache.penalty = 0;
      else
        po->c_cache.penalty = penalty;
    }
    if (rank)
      return false;
    else {
      penalty = 0;
      return true;
    }
  } catch (exception &e) {
    prs_display_error(
        string("Something went wrong with the constraint evaluation: '") +
        e.what() + "'");
  }
  return false;
}

extern int show_progress;

int last_sz = -1;

void opt_print_percentage(string msg, int act, int max) {
  if (show_progress) {
#if defined(DEBUG)
    cout << msg << " : " << act << " " << max << endl;
#else
    bool last = false;
    if (act == (max - 1))
      last = true;
    double perc;
    if (max > 1)
      perc = ((double)act) / ((double)(max - 1)) * 100.0;
    else
      perc = 100.0;
    string s;
    int sz = (int)ceil(perc / 10);
    if (sz < last_sz)
      last_sz = -1;
    if (last_sz < sz) {
      int i;
      for (i = 0; i < sz; i++) {
        s = s + "#";
      }
      for (; i < 10; i++) {
        s = s + " ";
      }
      printf("\rInformation: %s - progress: [%s]                           ",
             msg.c_str(), s.c_str());
      // printf("\rInformation: %s - progress: [%s]  (%7d, %7d)", msg.c_str(),
      // s.c_str(), act, max);
      fflush(stdout);
      last_sz = sz;
    }
    if (last)
      printf("\n");
#endif
  }
}
