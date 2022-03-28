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

#include <iomanip>
#include "st_ast.h"
#include "st_env.h"
#include "st_objectives_constraints.h"

double st_objective::eval(st_point *point, int num_objective) {
  st_object *evaluated = NULL;
  bool eval = true;
  try {
    if (point->o_cache.size() > num_objective) {
      if (point->o_cache[num_objective].optimization_timestamp ==
          current_environment.optimization_timestamp) {
        eval = false;
        evaluated = new st_double(point->o_cache[num_objective].value);
      }
    }
    st_ast_expression_stack s;
    if (eval)
      evaluated = st_invoke_recursive_evaluation_1(
          obj_expression, &s, point_name, point, &current_environment);

  } catch (exception &e) {
    st_object_discard(evaluated);
    throw std::logic_error(e.what());
  }
  if (!evaluated ||
      !(to<st_double *>(evaluated) || to<st_integer *>(evaluated))) {
    st_object_discard(evaluated);
    throw std::logic_error("The recursive evaluation failed or values returned "
                           "were'nt integers nor doubles");
  }
  if (to<st_double *>(evaluated)) {
    if (isnan(to<st_double *>(evaluated)->get_double()) ||
        isinf(to<st_double *>(evaluated)->get_double())) {
      st_object_discard(evaluated);
      throw std::logic_error(
          "Nans or infs caught during the computation of the objective");
    }
  }

  if (to<st_double *>(evaluated)) {
    double res = to<st_double *>(evaluated)->get_double();
    st_object_discard(evaluated);
    if (eval) {
      if (point->o_cache.size() <= num_objective)
        point->o_cache.resize(num_objective + 1);
      point->o_cache[num_objective].optimization_timestamp =
          current_environment.optimization_timestamp;
      point->o_cache[num_objective].value = res;
    }
    return res;
  }

  if (to<st_integer *>(evaluated)) {
    int res = to<st_integer *>(evaluated)->get_integer();
    st_object_discard(evaluated);
    if (eval) {
      if (point->o_cache.size() <= num_objective)
        point->o_cache.resize(num_objective + 1);
      point->o_cache[num_objective].optimization_timestamp =
          current_environment.optimization_timestamp;
      point->o_cache[num_objective].value = res;
    }
    return res;
  }

  cout << evaluated->print() << endl;
  st_object_discard(evaluated);
  bool should_never_happen = false;
  st_assert(should_never_happen);
  return 0.0;
};

bool st_constraint::violated(st_point *point, double &penalty) {
  penalty = 0;
  st_object *evaluated_l = NULL;
  st_object *evaluated_r = NULL;
  try {
    st_ast_expression_stack sl;
    st_ast_expression_stack sr;
    evaluated_l = st_invoke_recursive_evaluation_1(left, &sl, point_name, point,
                                                   &current_environment);
    evaluated_r = st_invoke_recursive_evaluation_1(right, &sr, point_name,
                                                   point, &current_environment);
    st_assert(sl.size() == 0);
    st_assert(sr.size() == 0);
  } catch (exception &e) {
    st_object_discard(evaluated_l);
    st_object_discard(evaluated_r);
    throw std::logic_error(e.what());
  }

  if (!evaluated_r ||
      !(to<st_double *>(evaluated_r) || to<st_integer *>(evaluated_r))) {
    st_object_discard(evaluated_l);
    st_object_discard(evaluated_r);
    throw std::logic_error("Was not able to convert to integer nor double the "
                           "right part of the constraint");
  }

  if (!evaluated_l ||
      !(to<st_double *>(evaluated_l) || to<st_integer *>(evaluated_l))) {
    st_object_discard(evaluated_l);
    st_object_discard(evaluated_r);
    throw std::logic_error("Was not able to convert to integer nor double the "
                           "left part of the constraint");
  }

  double f_l = to<st_double *>(evaluated_l)
                   ? to<st_double *>(evaluated_l)->get_double()
                   : to<st_integer *>(evaluated_l)->get_integer();

  double f_r = to<st_double *>(evaluated_r)
                   ? to<st_double *>(evaluated_r)->get_double()
                   : to<st_integer *>(evaluated_r)->get_integer();

  if (isnan(f_l) || isnan(f_r) || isinf(f_l) || isinf(f_r)) {
    st_object_discard(evaluated_l);
    st_object_discard(evaluated_r);
    throw std::logic_error(
        "Nans or infs caught during the computation of the constraint");
  }

  st_object_discard(evaluated_l);
  st_object_discard(evaluated_r);

  switch (type) {

  case ST_CONSTR_LT:
    if (f_l < f_r)
      return false;
    else {
      penalty = 1 + fmax(0.0, f_l - f_r);
      return true;
    }
    break;
  case ST_CONSTR_GT:
    if (f_l > f_r)
      return false;
    else {
      penalty = 1 + fmax(0.0, f_r - f_l);
      return true;
    }

  case ST_CONSTR_LTE:
    if (f_l <= f_r)
      return false;
    else {
      penalty = 1 + fmax(0.0, f_l - f_r);
      return true;
    }
    break;
  case ST_CONSTR_GTE:
    if (f_l >= f_r)
      return false;
    else {
      penalty = 1 + fmax(0.0, f_r - f_l);
      return true;
    }
  case ST_CONSTR_EQ:
    double precision;
    if (!current_environment.shell_variables.get_double("constraint_precision",
                                                        precision))
      precision = 0.000000001;
    if (fabs(f_l - f_r) < precision) {
      return false;
    } else {
      penalty = 1 + fabs(f_r - f_l);
      return true;
    }
  }
  return true;
}
