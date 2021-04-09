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

#include <iomanip>
#include <st_ast.h>
#include <st_companion_metrics.h>
#include <st_env.h>

st_object *st_companion_metric::eval(st_point *point,
                                     int num_companion_metric) {
  st_object *evaluated = NULL;
  bool eval = true;
  try {
    if (point->m_cache.size() > num_companion_metric) {
      if (point->m_cache[num_companion_metric].optimization_timestamp ==
          current_environment.optimization_timestamp) {
        eval = false;
        evaluated = point->m_cache[num_companion_metric].value;
      }
    }
    st_ast_expression_stack s;
    if (eval)
      evaluated = st_invoke_recursive_evaluation_1(
          obj_expression, &s, point_name, point, &current_environment);

  } catch (exception &e) {
    st_object_discard(evaluated);
    throw std::logic_error("Unable to evaluate companion metric");
  }
  if (!evaluated) {
    throw std::logic_error("Unable to evaluate companion metric");
  }
  st_object *res = evaluated->gen_copy();
  if (eval) {
    if (point->m_cache.size() <= num_companion_metric)
      point->m_cache.resize(num_companion_metric + 1);

    point->m_cache[num_companion_metric].optimization_timestamp =
        current_environment.optimization_timestamp;
    point->m_cache[num_companion_metric].set_value(evaluated);
    st_object_discard(evaluated);
  }
  return res;
};
