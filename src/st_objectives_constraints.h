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
#ifndef ST_OPTIMIZATION_OBJECTIVES_H
#define ST_OPTIMIZATION_OBJECTIVES_H

#include "st_ast.h"
#include "st_object.h"
#include "st_point.h"

struct st_objective {
  string name;
  st_ast *obj_expression;
  string point_name;

public:
  st_objective(string n, st_ast *expression, string pn) {
    name = n;
    obj_expression = expression->copy();
    point_name = pn;
  }
  double eval(st_point *p, int num_objective);
  ~st_objective() { delete obj_expression; }
};

#define ST_CONSTR_LT 0
#define ST_CONSTR_LTE 1
#define ST_CONSTR_GT 2
#define ST_CONSTR_GTE 3
#define ST_CONSTR_EQ 4

struct st_constraint {
  int type;
  string name;
  string point_name;
  st_ast *left;
  st_ast *right;

public:
  st_constraint(string n, string oper, st_ast *left_e, st_ast *right_e,
                string pn) {
    name = n;
    if (oper == "<")
      type = ST_CONSTR_LT;
    if (oper == ">")
      type = ST_CONSTR_GT;
    if (oper == "<=")
      type = ST_CONSTR_LTE;
    if (oper == ">=")
      type = ST_CONSTR_GTE;
    if (oper == "==")
      type = ST_CONSTR_EQ;

    left = left_e->copy();
    right = right_e->copy();
    point_name = pn;
  }
  void print_constraint() {
    left->print();
    switch (type) {
    case ST_CONSTR_LT:
      cout << " < ";
      break;
    case ST_CONSTR_GT:
      cout << " > ";
      break;
    case ST_CONSTR_LTE:
      cout << " <= ";
      break;
    case ST_CONSTR_GTE:
      cout << " >= ";
      break;
    case ST_CONSTR_EQ:
      cout << " == ";
      break;
    }
    right->print();
  }
  bool violated(st_point *p, double &penalty);
  ~st_constraint() {
    delete left;
    delete right;
  }
};

class st_env;

#endif
