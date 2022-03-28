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
/**
 *
 * Pareto Simulated Annealing. Derived from Smith & al. "Dominance based
 * Multi-objective Simulated annealing"
 *
 * Author: V. Zaccaria 2007
 */

#include <iostream>
#include <vector>
#include "st_design_space.h"
#include "st_object.h"
#include "st_map.h"
#include "st_vector.h"
#include "st_shell_variables.h"
#include "st_parser.h"
#include "st_optimizer.h"
#include "st_opt_utils.h"
#include "st_sim_utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <math.h>
#include <st_rand.h>

class st_mosa : public st_optimizer {
public:
  st_mosa() {}
  ~st_mosa() {}
  string get_information();
  int explore(st_env *env);
  st_point *neighbors(st_env *env, st_point actual_point, bool uniform);
  void find_initial_feasible_solution(st_env *env, int num_of_prs,
                                      st_database *F);
};

int explored_points = 0;

string st_mosa::get_information() {
  return "Dominance based Multi-objective Simulated annealing - (epochs, "
         "epoch_length, t_decrease_coefficient)";
}

bool probabilistic_accept(double probability) {
  double x = rnd_flat_float();
  if (x <= probability)
    return true;
  else
    return false;
}

double compute_scaled_energy(st_env *env, st_point x_p, st_database *F_tilde) {
  double num_of_points = 0.0;
  double dominating_points = 0.0;
  st_point_set *DB_list = F_tilde->get_set_of_points();
  st_point_set::iterator i;
  for (i = DB_list->begin(); i != DB_list->end(); i++) {
    st_point &p = *(i->second);
    bool feasible_x_p;
    bool feasible_x;
    int rank_x_p;
    int rank_x;
    double penalty_x_p;
    double penalty_x;
    if (sim_is_strictly_dominated(env, x_p, p, feasible_x_p, feasible_x,
                                  rank_x_p, rank_x, penalty_x_p, penalty_x))
      dominating_points += 1.0;
    num_of_points += 1.0;
  }
  return dominating_points / num_of_points;
}

double compute_energy_difference(st_env *env, st_point &x_p, st_point &x,
                                 st_database *F_tilde) {
  if (!env->optimization_objectives.size())
    return 0.0;

  if (env->optimization_objectives.size() == 1) {
    int rank_x;
    double penalty_x;

    int rank_x_p;
    double penalty_x_p;

    bool feasible_x = opt_check_constraints(x, env, rank_x, penalty_x);
    bool feasible_x_p = opt_check_constraints(x_p, env, rank_x_p, penalty_x_p);

    double f_x_p = env->optimization_objectives[0]->eval(&x_p, 0);
    double f_x = env->optimization_objectives[0]->eval(&x, 0);

    if (feasible_x && feasible_x_p)
      return f_x_p - f_x;

    if (feasible_x && !feasible_x_p)
      return f_x_p * rank_x_p - f_x;

    if (!feasible_x && feasible_x_p)
      return 0;

    if (!feasible_x && !feasible_x_p)
      return penalty_x_p - penalty_x;

  } else {
    double f_x_p = compute_scaled_energy(env, x_p, F_tilde);
    double f_x = compute_scaled_energy(env, x, F_tilde);
    return f_x_p - f_x;
  }
  throw "MOSA/Should not arrive here";
}

#define NUMBER_OF_TRIES 10

st_point *st_mosa::neighbors(st_env *env, st_point actual_point, bool uniform) {
  int nt = 0;
  st_point *sp = NULL;

  while (nt < NUMBER_OF_TRIES) {

    st_point neighbor_point =
        env->current_design_space->get_random_point_along_a_random_parameter(
            env, actual_point);

    st_batch_job job(neighbor_point);
    env->current_dispatcher->submit(env, &job);
    sp = job.get_point_at(0);

    if (sp) {
      if (!sp->get_error()) {
        explored_points++;
        return sp;
      } else {
        delete sp;
        nt++;
      }
    } else {
      nt++;
    }
  }
  throw st_exception("The MOSA algorithm has not been able to find a suitable "
                     "valid point. Something may be wrong with the algorithm "
                     "parameters and/or the design space.");
}

/** Tries to find an initial feasible population */
void st_mosa::find_initial_feasible_solution(st_env *env, int num_of_prs,
                                             st_database *F) {

  st_point *sp = NULL;
  int nt = 0;

  while (nt < NUMBER_OF_TRIES) {
    st_point actual_point = env->current_design_space->random_point(env);

    st_batch_job job(actual_point);
    env->current_dispatcher->submit(env, &job);
    st_point *sp = job.get_point_at(0);

    if (sp) {
      if (!sp->get_error()) {
        explored_points++;
        F->insert_point(sp);
        delete sp;
        return;
      } else {
        nt++;
        delete sp;
      }
    } else {
      nt++;
    }
  }
}

int st_mosa::explore(st_env *env) {
  if (!env->current_driver) {
    prs_display_error("no driver defined");
    return 0;
  }
  st_codi_reset_time();
  st_codi_display_p("Starting Pareto Simulated Annealing");

  double temperature = 1;
  double T_decrease;
  int epochs;
  int epoch_length;
  explored_points = 0;

  if (!env->shell_variables.get_integer("epochs", epochs)) {
    epochs = 10;
    env->shell_variables.set_integer("epochs", epochs);
  }

  if (!env->shell_variables.get_double("t_decrease_coefficient", T_decrease)) {
    T_decrease =
        0.19; /** So Tk is 10^-5, for k=7 which is 2/3 of 10, from the paper. */
    env->shell_variables.set_double("t_decrease_coefficient", T_decrease);
  }

  if (!env->shell_variables.get_integer("epoch_length", epoch_length)) {
    epoch_length = 100;
    env->shell_variables.set_integer("epoch_length", epoch_length);
  }
  int num_of_prs = env->current_design_space->ds_parameters.size();

  st_database *F = env->get_pre_populated_database();

  /** Find initial population */
  st_codi_display_p("Computing initial population");
  find_initial_feasible_solution(env, num_of_prs, F);

  /** DTLZ2 suggests us to keep it true */
  bool uniform = true;
  string unif;
  if (env->shell_variables.get_string("use_uniform_distribution", unif)) {
    if (unif == "true")
      uniform = true;
  }
  /* Run a fast pareto computation */

  /** Here, at leas one point should be in F */
  st_point_set *database_list = F->get_set_of_points();

  /** F starts with an initial feasible solution */
  st_point x = *(database_list->begin()->second);

  for (int e = 0; e < epochs; e++) {
    /** We do the perturbation for each element of the current F */
    for (int step = 0; step < epoch_length; step++) {
      st_point *x_p = neighbors(env, x, uniform);
      st_database *F_tilde = new st_database(*F);
      F_tilde->insert_point(x_p);
      F_tilde->insert_point(&x);
      double delta_E = compute_energy_difference(env, *x_p, x, F_tilde);
      // printf("Probability of next solution to be accepted is %f temp(%f)\n",
      // min(1.0, exp(-delta_E/temperature)), temperature);
      if (probabilistic_accept(min(1.0, exp(-delta_E / temperature)))) {
        x = *x_p;
        F->insert_point(&x);
        /* If x is dominated, F stays the same, while if it is not
         * we recompute the new pareto set. Keep in mind that even if x is
         * dominated since we accepted it, we are going now through its
         * neighbors to check for an optimal point. */
        sim_compute_pareto(env, F, false);
      }
      delete x_p;
      delete F_tilde;
    }
    st_codi_display_ep(((float)e) / ((float)epochs),
                       "Computing next temperature");
    temperature = T_decrease * temperature;
  }
  env->insert_new_database(F, env->current_destination_database_name);
  return explored_points;
}

extern "C" {
st_optimizer *opt_generate_optimizer() { return new st_mosa(); }
st_command *get_help() {
  const char *ref[] = {"epochs", "epoch_length", "t_decrease_coefficient",
                       NULL};
  const char *ref_help[] = {
      "number of macro-iterations with different temperature coefficients",
      "number of iterations with the same temperature",
      "multiplier used to compute the next temperature (ideally it should be "
      "<1, default is 0.19)",
      NULL};

  st_command *help = new st_command(
      multiple_opts,
      "Once loaded with the **opt_load_optimizer** command, the algorithm is "
      "invoked by using the **opt_tune** command (see manual)."
      "Each epoch is composed by iterations characterized by the same "
      "temperature. The number of iterations per epoch is defined by"
      "the ^opt(1) shell variable, while the number of epochs is defined by "
      "the ^opt(0) shell variable. The temperature is internally"
      " defined and impacts on the randomness of generated configurations. The "
      "shell variable ^opt(2) defines the temperature ratio"
      " between consecutive epochs.",
      "", ref, ref_help,
      "Simulated annealing is a Monte Carlo approach for minimizing "
      "multivariate functions. The term simulated annealing derives from the "
      "analogy with the physical process of heating and then slowly cooling a "
      "substance to obtain a strong crystalline structure. In the Simulated "
      "Annealing algorithm a new configuration is constructed by imposing a "
      "random displacement. If the cost function of this new state is less "
      "than the previous one, the change is accepted unconditionally and the "
      "system is updated. If the cost function is greater, the new "
      "configuration is accepted probabilistically; the acceptance possibility "
      "decreases with the temperature (optimization time). This procedure "
      "allows the system to move consistently towards lower cost function "
      "states, thus jumping out of local minima due to the probabilistic "
      "acceptance of some upward moves. This optimizer is derived by: Smith, "
      "K. I.; Everson, R. M.; Fieldsend, J. E.; Murphy, C.; Misra, R., "
      "Dominance-Based Multiobjective Simulated Annealing,IEEE Transaction on "
      "Evolutionary Computation, 12(3): 323-342 - 2008",
      STDRET);
  help->alt_command_name = "Multi-Objective Simulated Annealing";
  help->alt_command_synopsys = "opt_load_optimizer st_mosa";
  help->alt_command_type = "opt";

  return help;
};
}
