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
#ifndef ST_COMPANION_METRICS_H
#define ST_COMPANION_METRICS_H

#include "st_ast.h"
#include "st_object.h"
#include "st_point.h"  

struct st_companion_metric {
  string name;
  st_ast *obj_expression;
  string point_name;

public:
  st_companion_metric(string n, st_ast *expression, string pn) {
    name = n;
    obj_expression = expression->copy();
    point_name = pn;
  }
  st_object *eval(st_point *p, int num_objective);
  ~st_companion_metric() { delete obj_expression; }
};

class st_env;

#endif
