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
#ifndef _OPT_UTILS_H
#define _OPT_UTILS_H

#include "st_env.h"
#include "st_list.h"
#include "st_object.h"
#include "st_optimizer.h"
#include "st_shell_variables.h"

void opt_load_optimizers();
bool opt_select_optimizer(st_env *, string const &);

double opt_compute_cost_function(const st_point &p, st_env *);
string opt_transform_point_in_string_vector(st_point p, st_env *, bool);
string opt_transform_point_in_csv_string_vector(st_point p, st_env *);
string opt_transform_point_in_csv_integer_vector(st_point p, st_env *);

bool opt_check_constraints(const st_point &p, st_env *env, int &rank,
                           double &penalty);
void opt_print_percentage(string msg, int act, int max);

#endif
