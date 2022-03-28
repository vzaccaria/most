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

#ifndef SIM_UTILS_H
#define SIM_UTILS_H

#include "st_env.h"
#include "st_object.h"
#include "st_opt_utils.h"
#include "st_point.h"

#define sim_get_point_at_iterator(i) dynamic_cast<st_point &>(**i)

/*
#define sim_get_metric_from_point(p,m,env)
sim_get_metrics_stats(&const_cast<st_point &>(p), m,env).get_double() const
st_vector *sim_get_metrics_vector(st_env *p); const st_vector
*sim_get_stats_vector(st_env *p); st_double       sim_get_metrics_stats(st_point
*p, st_string metric_stat_name, st_env *env); int
sim_get_parameter_index_by_name(st_env *,st_string ); bool
sim_is_a_parameter(st_env *, st_string); string
sim_get_sym_value_by_parname_and_level(st_env *env, st_string name,int level,
bool & res); st_vector const & sim_get_parameter_space_in_design_space(const
st_vector *ds, int index); st_string sim_get_sym_value_in_parameter_space(const
st_vector & s, int index); st_string sim_get_parameter_name_by_index(st_env *,
int );
*/
bool sim_is_strictly_dominated(st_env *env, st_point &e, st_point &p,
                               bool &feasible_e, bool &feasible_p, int &rank_e,
                               int &rank_p, double &penalty_e,
                               double &penalty_p);

void sim_compute_pareto(st_env *, st_database *, bool);
void sim_compute_pareto_nth(st_env *, st_database *, int, bool);
bool sim_recompute_pareto_with_new_point(st_env *, st_database *, st_point &);
double sim_compute_two_set_coverage_metric(st_env *env, st_database *a,
                                           st_database *b, bool verbose);
double sim_compute_ADRS(st_env *env, st_database *A, st_database *X);
double sim_compute_avg_euclidean_distance(st_env *env, st_database *A,
                                          st_database *X);
double sim_compute_median_euclidean_distance(st_env *env, st_database *A,
                                             st_database *X);
double sim_compute_kmeans_clusters(st_env *env, int klusters, int iterations,
                                   st_database *the_database);

int sim_compute_how_many_points_in_A_are_in_B(st_env *env, st_database *a,
                                              st_database *b);
st_object *sim_generate_dummy_point(st_env *env);
void sim_normalize_databases(st_env *env, st_database *b, st_database *a,
                             bool valid);

#endif
