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
/*
 * Abstract syntax tree definition for the grammar.
 *
 * Author: V. Zaccaria 2007
 * */
#include <errno.h>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <math.h>
#include <pthread.h>
#include <sstream>
#include "st_ast.h"
#include "st_commands.h"
#include "st_common_utils.h"
#include "st_env.h"
#include "st_map.h"
#include "st_object.h"
#include "st_opt_utils.h"
#include "st_parser.h"
#include "st_point.h"
#include "st_shell_command.h"
#include "st_design_space.h"
#include "st_sim_utils.h"
#include "st_vector.h"
#include "st_xdr_api.h"
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/unistd.h>

typedef st_object *(*st_function_name)(st_object *, st_env *);
map<string, st_function_name> st_ast_functions;

extern vector<string> possible_commands;

/** The following is for displaying line numbers when giving errors */
st_ast *last_known_ast;

extern st_object *dont_destroy;
void st_set_current_scoped_value(st_ast_expression_stack *stack, string &name,
                                 st_object *expr);

void st_display_message_assigned(st_env *env, string name,
                                 const st_object *val);

double get_int_or_double(st_object *vec, int index);
string get_string(st_object *vec, int index);

static int fun_num = 0;

extern string st_itos(int i);

string st_ast_get_afun_name() {
  return string("_st_tmp_fun_") + st_itos(fun_num++);
}

void st_print_stack(st_ast_expression_stack *stack) {
  printf("Stack: \n");
  if (stack->size() == 0)
    return;

  int s = stack->size() - 1;
  for (; s >= 0; s--) {
    map<string, st_object *>::iterator i;
    for (i = (*stack)[s].begin(); i != (*stack)[s].end(); i++) {
      printf("%-8s  %8p %c %c %-20s\n", i->first.c_str(), i->second,
             i->second->is_global() ? 'G' : 'L',
             i->second->is_stack_permanent() ? 'P' : 'D',
             i->second->print().c_str());
    }
    printf("---------\n");
  }
}

st_object *st_get_scoped_value(string variable, st_ast_expression_stack *stack,
                               st_env *env, int stack_level) {
  if (stack_level > 0) {
    if (stack->size() == 0)
      return NULL;

    if ((*stack)[stack_level - 1].count(variable)) {
      return ((*stack)[stack_level - 1][variable]);
    }
  }
  return NULL;
}

st_object *st_get_current_scoped_value(string variable,
                                       st_ast_expression_stack *stack,
                                       st_env *env) {
  if (stack->size() == 0)
    return NULL;

  int s = stack->size() - 1;
  if (s >= 0) {
    if ((*stack)[s].count(variable)) {
      return ((*stack)[s][variable]);
    }
  }

  return NULL;
}

bool st_in_anonymous_function(st_ast_expression_stack *stack, st_env *env,
                              int &stack_level) {
  st_object *sl = st_get_current_scoped_value("stack_level", stack, env);

  if (!sl)
    return false;

  stack_level = to<st_integer *>(sl)->get_integer();

  if (stack_level >= 0)
    return true;

  return false;
}

map<string, st_user_def_function> st_user_defined_functions;

void st_discard_stack_frame(st_ast_stack_frame &f);

void st_discard_stack_top(st_ast_expression_stack *stack) {
  if (stack->size() >= 1) {
    st_discard_stack_frame((*stack)[stack->size() - 1]);
    stack->resize(stack->size() - 1);
  }
}

/** Attention, this does not destroy 'value'! */
st_object *st_invoke_recursive_evaluation_1(st_ast *expression,
                                            st_ast_expression_stack *stack,
                                            string parameter_name,
                                            st_object *value, st_env *env) {
  if (!expression)
    return NULL;

  if (!value)
    return NULL;

  st_ast_stack_frame f;
  stack->push_back(f);
  st_set_current_scoped_value(stack, parameter_name, value->gen_copy());
  st_object *res;
  try {
    res = st_ast_eval(expression, stack, env);
  } catch (exception &e) {
    last_known_ast = expression;
    st_discard_stack_top(stack);
    throw std::logic_error(e.what());
  } catch (st_ast_return_exception &x) {
    last_known_ast = expression;
    stack->resize(stack->size() - 1);
    throw std::logic_error(
        "Can't use return value in the description of objectives/constraints");
  }
  st_discard_stack_top(stack);
  return res;
}

st_object *st_call_user_defined_function(string function,
                                         st_ast_expression_stack *stack,
                                         st_object *parameters, st_env *env,
                                         int level) {
  if (!to<st_vector *>(parameters))
    return NULL;

  if (!st_user_defined_functions.count(function))
    return NULL;

  st_vector *vpar = to<st_vector *>(parameters);

  if (vpar->size() !=
      st_user_defined_functions[function].local_parameters.size())
    return NULL;

  st_ast_stack_frame f;
  stack->push_back(f);

  for (int i = 0; i < vpar->size(); i++) {
    st_set_current_scoped_value(
        stack, st_user_defined_functions[function].local_parameters[i],
        const_cast<st_object *>(&vpar->get(i))->gen_copy());
  }

  string stack_level_name = "stack_level";

  if (level >= 0) {
    st_set_current_scoped_value(stack, stack_level_name,
                                st_integer(level).gen_copy());
  }

  st_object *res = NULL;
  try {
    res = st_ast_eval(st_user_defined_functions[function].function_expression,
                      stack, env);
    // st_print_stack(stack);
    // cout << "^^^^^^^^^^^^^^^ during stack unwinding of " << function << endl;

  } catch (exception &e) {
    st_discard_stack_top(stack);
    throw;
  } catch (st_ast_return_exception &ret) {
    // st_print_stack(stack);
    // cout << "^^^^^^^^^^^^^^^ during stack unwinding of " << function << endl;

    st_object_discard(res);
    res = ret.value;
  }
  st_discard_stack_top(stack);

  return res;
}

st_object *st_ast_prod(st_object *obj, st_env *);
st_object *st_ast_sum(st_object *obj, st_env *);
st_object *st_ast_min(st_object *obj, st_env *);
st_object *st_ast_max(st_object *obj, st_env *);
st_object *st_ast_div(st_object *obj, st_env *env);
st_object *st_ast_pow(st_object *obj, st_env *env);

void st_discard_stack_frame(st_ast_stack_frame &f) {
  map<string, st_object *>::iterator k;
  for (k = f.begin(); k != f.end(); k++) {
    st_object_discard_local(k->second);
  }
  f.clear();
}

void st_copy_stack_frame(st_ast_stack_frame &cur, st_ast_stack_frame &f,
                         string exclude) {
  map<string, st_object *>::iterator k;
  for (k = cur.begin(); k != cur.end(); k++) {
    if (k->first != exclude) {
      f[k->first] = k->second->gen_copy();

      if (k->second->is_global())
        f[k->first]->set_global();

      if (k->second->is_stack_permanent())
        f[k->first]->set_stack_permanent();
    }
  }
}

st_object *st_compute_functional_value(st_ast_functional *ast,
                                       st_ast_expression_stack *stack,
                                       st_env *env) {
  st_object *min = st_ast_eval(ast->min, stack, env);
  st_object *max;
  try {
    max = st_ast_eval(ast->max, stack, env);
  } catch (exception &e) {
    st_object_discard(min);
    throw;
  }

  if (!to<st_integer *>(min) || !to<st_integer *>(max)) {
    st_object_discard(min);
    st_object_discard(max);
    return NULL;
  }

  int iterator = to<st_integer *>(min)->get_integer();
  int it_max = to<st_integer *>(max)->get_integer();

  st_object_discard(min);
  st_object_discard(max);

  if (iterator > it_max) {
    return new st_integer(0);
  }

  st_ast_stack_frame f;
  if (stack->size() > 0) {
    st_ast_stack_frame cur = (*stack)[stack->size() - 1];
    st_copy_stack_frame(cur, f, ast->iterator);
  }
  stack->push_back(f);

  st_set_current_scoped_value(stack, ast->iterator, new st_integer(iterator));

  st_object *current;
  try {
    current = st_ast_eval(ast->expression, stack, env);
  } catch (exception &e) {
    st_object_discard(current);
    st_discard_stack_top(stack);
    throw;
  }
  if (!current) {
    st_discard_stack_top(stack);
    return NULL;
  }
  iterator++;

  int n = 1;
  while (iterator <= it_max && current) {
    st_object *curcp = current->gen_copy();
    st_object_discard(current);
    current = curcp;
    st_set_current_scoped_value(stack, ast->iterator, new st_integer(iterator));

    st_object *res;
    try {
      res = st_ast_eval(ast->expression, stack, env);
    } catch (exception &e) {
      st_object_discard(current);
      st_discard_stack_top(stack);
      throw;
    }

    if (!res) {
      st_object_discard(current);
      st_discard_stack_top(stack);
      return NULL;
    }

    st_vector params;

    params.insert(0, *current);
    params.insert(1, *res);

    st_object_discard(current);
    st_object_discard(res);

    if (ast->functional_name == "sum" || ast->functional_name == "avg")
      current = st_ast_sum(&params, env);
    if (ast->functional_name == "prod" || ast->functional_name == "geomavg")
      current = st_ast_prod(&params, env);
    if (ast->functional_name == "max")
      current = st_ast_max(&params, env);
    if (ast->functional_name == "min")
      current = st_ast_min(&params, env);

    iterator++;
    n++;
  }
  if (ast->functional_name == "avg") {
    st_vector params;
    st_object *rg = new st_integer(n);

    params.insert(0, *current);
    params.insert(1, *rg);

    st_object_discard(current);
    st_object_discard(rg);

    current = st_ast_div(&params, env);
  }
  if (ast->functional_name == "geomavg") {
    st_vector params;
    st_object *rg = new st_double(1.0 / ((double)n));

    params.insert(0, *current);
    params.insert(1, *rg);

    st_object_discard(current);
    st_object_discard(rg);

    current = st_ast_pow(&params, env);
  }
  st_object *retvalue = current->gen_copy();
  st_object_discard(current);
  st_discard_stack_top(stack);
  return retvalue;
}

st_object *st_ast_get_par(st_object *vec, int arg) {
  if (!vec)
    return NULL;

  if (!to<st_vector *>(vec))
    return NULL;

  if (to<st_vector *>(vec)->size() <= arg)
    return NULL;

  st_object *k = const_cast<st_object *>((&to<st_vector *>(vec)->get(arg)));

  return k;
}

st_point *st_ast_get_point_n_discard(st_object *vec) {
  st_object *point = st_ast_get_par(vec, 0);
  if (!to<st_point *>(point)) {
    st_object_discard(vec);
    return NULL;
  }

  st_point *nx = to<st_point *>(point->gen_copy());
  st_object_discard(vec);
  return nx;
}

/*
#define maybe_one(x) x "\\{0,1\\}"
#define one(x) x "\\{1,1\\}"
#define maybe_more(x) x "*"
#define more(x) x x "*"
#define nl "\\\\n"
#define ta "\\\\t"
#define cc "\\\\r"
*/

#define maybe_one(x) x "{0,1}"
#define one(x) x "{1,1}"
#define maybe_more(x) x "*"
#define more(x) x x "*"
#define either(x, y) x + "|" + y
#define nl "\\\\n"
#define ta "\\\\t"
#define cc "\\\\r"

st_object *st_ast_printf(st_object *obj, st_env *env) {
  st_object *fo = st_ast_get_par(obj, 0);
  if (!to<st_string *>(fo))
    return NULL;

  string fmt = to<st_string *>(fo)->get_string();
  /* format is %[+-]?n.n{f,d,s,l,v} */
  string integer_pattern =
      one("%") maybe_one("[\\+\\-]") maybe_more("[0-9]") "d";
  string double_pattern = one("%") maybe_one("[\\+\\-]") maybe_more("[0-9]")
      maybe_one(".") maybe_more("[0-9]") "f";
  string string_pattern =
      one("%") maybe_one("[\\+\\-]") maybe_more("[0-9]") "s";
  string printable_pattern = either(
      either(either(integer_pattern, double_pattern), string_pattern), nl);
  printable_pattern = either(printable_pattern, ta);
  printable_pattern = either(printable_pattern, cc);

  regex_t fetch_integer;
  regex_t fetch_double;
  regex_t fetch_string;
  regex_t fetch_printable;
  if (regcomp(&fetch_integer, integer_pattern.c_str(), REG_EXTENDED) ||
      regcomp(&fetch_double, double_pattern.c_str(), REG_EXTENDED) ||
      regcomp(&fetch_string, string_pattern.c_str(), REG_EXTENDED) ||
      regcomp(&fetch_printable, printable_pattern.c_str(), REG_EXTENDED)) {
    throw std::logic_error("Cannot compile regular expressions");
  }

  regmatch_t positions[1];
  string previous;
  string actual;
  string next = fmt;
  int narg = 1;
  while (next != "") {
    string curr = next;
    if (!regexec(&fetch_printable, curr.c_str(), 1, positions, 0)) {
      int length = positions[0].rm_eo - positions[0].rm_so;
      if (positions[0].rm_so > 0)
        previous = curr.substr(0, positions[0].rm_so);
      actual = curr.substr(positions[0].rm_so, length);
      next = curr.substr(positions[0].rm_eo, curr.size() - length);
      cout << previous;
      if (actual == "\\n") {
        printf("\n");
        continue;
      }
      if (actual == "\\r") {
        printf("\r");
        continue;
      }
      if (actual == "\\t") {
        printf("\t");
        continue;
      }

      if (actual[actual.size() - 1] == 'd') {
        st_object *o = st_ast_get_par(obj, narg++);
        if (!o) {
          throw std::logic_error("Bad format specifier in printf");
        }
        if (!to<st_integer *>(o)) {
          throw std::logic_error("Bad format specifier in printf");
        }
        printf(actual.c_str(), to<st_integer *>(o)->get_integer());
        continue;
      }
      if (actual[actual.size() - 1] == 'f') {
        st_object *o = st_ast_get_par(obj, narg++);
        if (!o) {
          throw std::logic_error("Bad format specifier in printf");
        }
        if (!to<st_double *>(o)) {
          throw std::logic_error("Bad format specifier in printf");
        }
        printf(actual.c_str(), to<st_double *>(o)->get_double());
        continue;
      }
      if (actual[actual.size() - 1] == 's') {
        st_object *o = st_ast_get_par(obj, narg++);
        if (!o) {
          throw std::logic_error("Bad format specifier in printf");
        }
        if (!to<st_string *>(o)) {
          throw std::logic_error("Bad format specifier in printf");
        }
        printf(actual.c_str(), to<st_string *>(o)->get_string().c_str());
        continue;
      }
    } else {
      previous = curr;
      next = "";
      cout << previous;
    }
  }
  return NULL;
}

st_object *st_ast_get_properties(st_object *obj, st_env *env) {
  obj = st_ast_get_par(obj, 0);

  if (!obj)
    return NULL;

  st_map *m = (obj->get_properties_map());
  return m;
}

#define META_DEREF_ANON "MOST_MAGIC_ANON_D"

st_object *st_create_anon_from_existing(string existing) {
  st_map *p = to<st_map *>((new st_map()));

  st_string key1 = st_string("meta");
  st_string val1 = st_string(META_DEREF_ANON);

  prs_add_element_to_map(p, &key1, &val1);

  st_string key2 = st_string("name");
  st_string val2 = st_string(existing);

  prs_add_element_to_map(p, &key2, &val2);

  st_string key3 = st_string("stack_level");
  st_integer val3 = st_integer(0);

  prs_add_element_to_map(p, &key3, &val3);

  return (p);
}

st_object *st_ast_get_objectives(st_object *obj, st_env *env) {
  obj = st_ast_get_par(obj, 0);

  if (!to<st_point *>(obj))
    return NULL;

  st_vector *vob = new st_vector();
  st_point *p = to<st_point *>(obj);
  try {
    for (int i = 0; i < env->optimization_objectives.size(); i++) {
      st_double e_m_value = env->optimization_objectives[i]->eval(p, i);
      vob->insert(i, e_m_value);
    }
  } catch (exception &e) {
    delete vob;
    return NULL;
  }
  return vob;
}

st_object *st_ast_get_scalar_parameter_index_list(st_object *obj, st_env *env) {
  st_vector *vob = new st_vector();

  if (!env->current_design_space)
    return NULL;

  try {
    int n = 0;
    map<string, st_scalar>::iterator i;
    for (i = env->current_design_space->scalar_parameters.begin();
         i != env->current_design_space->scalar_parameters.end(); i++) {
      string par_name(i->first);
      st_integer idx(env->current_design_space->ds_parameters_index[par_name]);
      vob->insert(n++, idx);
    }
  } catch (exception &e) {
    delete vob;
    return NULL;
  }
  return vob;
}

st_object *st_ast_get_scalar_parameter_list(st_object *obj, st_env *env) {
  st_vector *vob = new st_vector();

  if (!env->current_design_space)
    return NULL;

  try {
    int n = 0;
    map<string, st_scalar>::iterator i;
    for (i = env->current_design_space->scalar_parameters.begin();
         i != env->current_design_space->scalar_parameters.end(); i++) {
      st_string par_name(i->first);
      vob->insert(n++, par_name);
    }
  } catch (exception &e) {
    delete vob;
    return NULL;
  }
  return vob;
}

st_object *st_ast_get_objectives_fun(st_object *obj, st_env *env) {
  st_vector *vob = new st_vector();
  try {
    for (int i = 0; i < env->optimization_objectives.size(); i++) {
      st_object *an =
          st_create_anon_from_existing((env->optimization_objectives[i]->name));
      vob->insert(i, *an);
      delete an;
    }
  } catch (exception &e) {
    delete vob;
    return NULL;
  }
  return vob;
}

st_object *st_ast_get_objectives_list(st_object *obj, st_env *env) {
  st_vector *vob = new st_vector();
  try {
    for (int i = 0; i < env->optimization_objectives.size(); i++) {
      st_string objective_name(env->optimization_objectives[i]->name);
      vob->insert(i, objective_name);
    }
  } catch (exception &e) {
    delete vob;
    return NULL;
  }
  return vob;
}

st_object *st_ast_get_metrics(st_object *obj, st_env *env) {
  obj = st_ast_get_par(obj, 0);

  if (!to<st_point *>(obj)) {
    throw std::logic_error(
        "Deriving metrics from something which is not a point");
  }

  st_object const *point_metrics;
  if (!obj->get_properties("metrics", point_metrics)) {
    throw std::logic_error("The object does not have a metric property");
  }
  return point_metrics->gen_copy();
}

st_object *st_ast_set_metrics(st_object *obj, st_env *env) {
  st_object *par_0 = st_ast_get_par(obj, 0);
  st_object *par_1 = st_ast_get_par(obj, 1);
  st_object *par_2 = st_ast_get_par(obj, 2);
  if (!is_a<st_point *>(par_0)) {
    throw std::logic_error("The object given as first argument is not a point");
  }
  if (!is_a<st_integer *>(par_1)) {
    throw std::logic_error(
        "The object given as second argument is not an integer");
  }
  if (!is_a<st_double *>(par_2)) {
    throw std::logic_error(
        "The object given as third argument is not a double");
  }
  st_point *the_point = to<st_point *>(par_0->gen_copy());
  int metric_number = to<st_integer *>(par_1)->get_integer();
  double metric_value = to<st_double *>(par_2)->get_double();

  st_object const *point_metrics;
  if (!the_point->get_properties("metrics", point_metrics)) {
    throw std::logic_error("The object does not have a metric property");
  }
  st_object *metrics_obj = point_metrics->gen_copy();
  if (!is_a<st_vector *>(metrics_obj)) {
    throw std::logic_error("The metrics are in an unknown format");
  }
  st_vector *metrics_vector = to<st_vector *>(metrics_obj);
  if (metric_number < 0 || metric_number >= metrics_vector->size()) {
    throw std::logic_error("The selected metric doen't exist");
  }
  st_double new_value(metric_value);
  metrics_vector->insert(metric_number, new_value);
  the_point->set_properties("metrics", *metrics_vector);
  return the_point;
}

st_object *st_ast_get_time(st_object *obj, st_env *env) {
  return (new st_double(st_get_time()));
}

st_object *st_ast_abs(st_object *obj, st_env *env) {
  obj = st_ast_get_par(obj, 0);

  if (!obj)
    return NULL;

  if (is_a<st_double *>(obj)) {
    double d = to<st_double *>(obj)->get_double();
    if (d < 0.0)
      return (new st_double(-1 * d));
    else
      return obj->gen_copy();
  }

  if (is_a<st_integer *>(obj)) {
    int d = to<st_integer *>(obj)->get_integer();
    if (d < 0)
      return (new st_integer(-1 * d));
    else
      return obj->gen_copy();
  }

  return NULL;
}

#define check_one_or_both_double(ad, bd, ai, bi)                               \
  (((ad) && (bd)) || ((ad) && (bi)) || ((ai) && (bd)))

st_object *st_ast_prod(st_object *obj, st_env *env) {
  st_object *a = st_ast_get_par(obj, 0);
  st_object *b = st_ast_get_par(obj, 1);

  if (!a && !b)
    return NULL;

  bool b_is_double = is_a<st_double *>(b);
  bool b_is_int = is_a<st_integer *>(b);

  bool a_is_double = is_a<st_double *>(a);
  bool a_is_int = is_a<st_integer *>(a);

  if (check_one_or_both_double(a_is_double, b_is_double, a_is_int, b_is_int)) {
    double b_d = b_is_double ? to<st_double *>(b)->get_double()
                             : to<st_integer *>(b)->get_integer();
    double a_d = a_is_double ? to<st_double *>(a)->get_double()
                             : to<st_integer *>(a)->get_integer();
    return (new st_double(a_d * b_d));
  }
  if (a_is_int && b_is_int) {
    int b_d = to<st_integer *>(b)->get_integer();
    int a_d = to<st_integer *>(a)->get_integer();
    return (new st_integer(a_d * b_d));
  }
  return NULL;
}

st_object *st_ast_div(st_object *obj, st_env *env) {
  double a = get_int_or_double(obj, 0);
  double b = get_int_or_double(obj, 1);

  return (new st_double(a / b));
}

st_object *st_ast_sum(st_object *obj, st_env *env) {
  st_object *a = st_ast_get_par(obj, 0);
  st_object *b = st_ast_get_par(obj, 1);

  if (!a && !b) {
    return NULL;
  }

  bool b_is_double = is_a<st_double *>(b);
  bool b_is_int = is_a<st_integer *>(b);

  bool a_is_double = is_a<st_double *>(a);
  bool a_is_int = is_a<st_integer *>(a);

  if (check_one_or_both_double(a_is_double, b_is_double, a_is_int, b_is_int)) {
    double b_d = b_is_double ? to<st_double *>(b)->get_double()
                             : to<st_integer *>(b)->get_integer();
    double a_d = a_is_double ? to<st_double *>(a)->get_double()
                             : to<st_integer *>(a)->get_integer();
    return (new st_double(a_d + b_d));
  }
  if (a_is_int && b_is_int) {
    int b_d = to<st_integer *>(b)->get_integer();
    int a_d = to<st_integer *>(a)->get_integer();
    return (new st_integer(a_d + b_d));
  }
  return NULL;
}

st_object *st_ast_max(st_object *obj, st_env *env) {
  st_object *a = st_ast_get_par(obj, 0);
  st_object *b = st_ast_get_par(obj, 1);

  if (!a && !b)
    return NULL;

  bool b_is_double = is_a<st_double *>(b);
  bool b_is_int = is_a<st_integer *>(b);

  bool a_is_double = is_a<st_double *>(a);
  bool a_is_int = is_a<st_integer *>(a);

  if (check_one_or_both_double(a_is_double, b_is_double, a_is_int, b_is_int)) {
    double b_d = b_is_double ? to<st_double *>(b)->get_double()
                             : to<st_integer *>(b)->get_integer();
    double a_d = a_is_double ? to<st_double *>(a)->get_double()
                             : to<st_integer *>(a)->get_integer();
    return (new st_double(fmax(a_d, b_d)));
  }
  if (a_is_int && b_is_int) {
    int b_d = to<st_integer *>(b)->get_integer();
    int a_d = to<st_integer *>(a)->get_integer();
    return (new st_integer((a_d > b_d) ? a_d : b_d));
  }
  return NULL;
}

st_object *st_ast_min(st_object *obj, st_env *env) {
  st_object *a = st_ast_get_par(obj, 0);
  st_object *b = st_ast_get_par(obj, 1);

  if (!a && !b)
    return NULL;

  bool b_is_double = is_a<st_double *>(b);
  bool b_is_int = is_a<st_integer *>(b);

  bool a_is_double = is_a<st_double *>(a);
  bool a_is_int = is_a<st_integer *>(a);

  if (check_one_or_both_double(a_is_double, b_is_double, a_is_int, b_is_int)) {
    double b_d = b_is_double ? to<st_double *>(b)->get_double()
                             : to<st_integer *>(b)->get_integer();
    double a_d = a_is_double ? to<st_double *>(a)->get_double()
                             : to<st_integer *>(a)->get_integer();
    return (new st_double(fmin(a_d, b_d)));
  }
  if (a_is_int && b_is_int) {
    int b_d = to<st_integer *>(b)->get_integer();
    int a_d = to<st_integer *>(a)->get_integer();
    return (new st_integer((a_d < b_d) ? a_d : b_d));
  }
  return NULL;
}

st_object *st_ast_pow(st_object *obj, st_env *env) {
  double base = get_int_or_double(obj, 0);
  double power = get_int_or_double(obj, 1);
  double res = pow(base, power);
  return (new st_double(res));
}

st_object *st_ast_exp(st_object *obj, st_env *env) {
  double d = get_int_or_double(obj, 0);
  return new st_double(exp(d));
}

st_object *st_ast_system(st_object *obj, st_env *env) {
  string d = get_string(obj, 0);
  bool ret = shell_command(d, true);
  return new st_integer(ret);
}

st_object *st_ast_log(st_object *obj, st_env *env) {
  double d = get_int_or_double(obj, 0);
  return new st_double(log(d));
}

st_object *st_ast_sqrt(st_object *obj, st_env *env) {
  double d = get_int_or_double(obj, 0);
  return new st_double(sqrt(d));
}

st_object *st_ast_ceil(st_object *obj, st_env *env) {
  double d = get_int_or_double(obj, 0);
  return new st_integer((int)ceil(d));
}

st_object *st_ast_floor(st_object *obj, st_env *env) {
  double d = get_int_or_double(obj, 0);
  return new st_integer((int)floor(d));
}

st_object *st_ast_round(st_object *obj, st_env *env) {
  double d = get_int_or_double(obj, 0);
  double rounded;
  if (d < 0.0)
    rounded = ceil(d - 0.5);
  else
    rounded = floor(d + 0.5);
  return new st_integer((int)rounded);
}

st_object *st_ast_fmod(st_object *obj, st_env *env) {
  double x = get_int_or_double(obj, 0);
  double y = get_int_or_double(obj, 1);
  double reminder = fmod(x, y);
  return (new st_integer((int)reminder));
}

st_object *st_ast_decimal_part(st_object *obj, st_env *env) {
  double number = get_int_or_double(obj, 0);
  double integer_part;
  double decimal_part = modf(number, &integer_part);
  return (new st_double(decimal_part));
}

st_object *st_ast_integer_part(st_object *obj, st_env *env) {
  double number = get_int_or_double(obj, 0);
  double integer_part;
  double decimal_part = modf(number, &integer_part);
  return (new st_integer((int)integer_part));
}

st_object *st_ast_sort_vector_asc(st_object *obj, st_env *env) {
  st_object *o = st_ast_get_par(obj, 0);
  st_vector *input_vector;
  vector<double> intermediate_vector;
  if (to<st_vector *>(o))
    input_vector = to<st_vector *>(o);
  else
    throw std::logic_error("Expecting a vector");
  for (int i = 0; i < input_vector->size(); i++) {
    double number;
    st_object *element = input_vector->get(i).gen_copy();
    if (to<st_double *>(element))
      number = to<st_double *>(element)->get_double();
    else if (to<st_integer *>(element))
      number = (double)(to<st_integer *>(element)->get_integer());
    else
      throw std::logic_error("Expecting a vector of numbers");
    intermediate_vector.push_back(number);
  }
  sort(intermediate_vector.begin(), intermediate_vector.end());
  return (new st_vector(intermediate_vector));
}

st_object *st_ast_get_max_level(st_object *obj, st_env *env) {
  st_object *index = st_ast_get_par(obj, 0);

  if (!index || !is_a<st_string *>(index))
    return NULL;

  string idx = to<st_string *>(index)->get_string();

  if (!env->current_design_space->scalar_parameters.count(idx))
    throw std::logic_error("The specified parameter does not exist");

  return (new st_integer(env->current_design_space->get_scalar_max(env, idx)));
}

st_object *st_ast_get_min_level(st_object *obj, st_env *env) {
  st_object *index = st_ast_get_par(obj, 0);

  if (!index || !is_a<st_string *>(index))
    return NULL;

  string idx = to<st_string *>(index)->get_string();

  if (!env->current_design_space->scalar_parameters.count(idx))
    throw std::logic_error("The specified parameter does not exist");

  return (new st_integer(env->current_design_space->get_scalar_min(env, idx)));
}

st_object *st_ast_get_level(st_object *obj, st_env *env) {
  st_object *point = st_ast_get_par(obj, 0);
  st_object *index = st_ast_get_par(obj, 1);

  if (!point || !index || !is_a<st_point *>(point) ||
      !is_a<st_integer *>(index)) {
    cout << "HHHHHHHHHHH" << point->print() << index->print() << endl;

    return NULL;
  }

  st_point *pp = to<st_point *>(point);
  int idx = to<st_integer *>(index)->get_integer();

  int sz = env->current_design_space->ds_parameters.size();

  if (idx >= sz)
    throw std::logic_error("Index out of bounds");

  if (pp->size() != sz)
    throw std::logic_error(
        "The point is inconsistent with the current design space");

  return (new st_integer((*pp)[idx]));
}

st_object *st_ast_get_integer_representation(st_object *obj, st_env *env) {
  st_object *point = st_ast_get_par(obj, 0);
  st_object *index = st_ast_get_par(obj, 1);

  if (!point || !index || !is_a<st_point *>(point) ||
      !is_a<st_integer *>(index))
    return NULL;
  st_point pp = *(to<st_point *>(point));
  int idx = to<st_integer *>(index)->get_integer();

  int sz = env->current_design_space->ds_parameters.size();

  if (idx >= sz || idx < 0)
    throw std::logic_error("Index out of bounds");

  if (pp.size() != sz)
    throw std::logic_error(
        "The point is inconsistent with the current design space");
  string representation =
      env->current_design_space->get_parameter_representation(
          env, pp, env->current_design_space->ds_parameters_names[idx]);
  return (new st_integer(atoi(representation.c_str())));
}

st_object *st_ast_level(st_object *obj, st_env *env) {
  st_object *parname = st_ast_get_par(obj, 0);
  st_object *symparvalue = st_ast_get_par(obj, 1);

  if (!parname || !symparvalue || !is_a<st_string *>(parname) ||
      !is_a<st_string *>(symparvalue))
    return NULL;

  string pn = to<st_string *>(parname)->get_string();
  string sv = to<st_string *>(symparvalue)->get_string();

  if (!env->current_design_space->ds_parameters_index.count(pn)) {
    return NULL;
  }

  bool res;

  int index = env->current_design_space->get_scalar_level(env, pn, sv);

  return (new st_integer(index));
}

/*
 * FIXME: this is probably not needed and would require rewriting the design
space alot.

st_object *st_ast_level_sym_value(st_object *obj, st_env *env)
{
    st_object *parname=  st_ast_get_par(obj,0);
    st_object *parvalue =  st_ast_get_par(obj,1);

    if(!parname|| !parvalue || !is_a<st_string *>(parname) || !is_a<st_integer
*>(parvalue)) return NULL;

    string pn = to<st_string *>(parname)->get_string();
    int sv = to<st_integer *>(parvalue)->get_integer();

    if(!env->current_design_space->ds_parameters_index.count(pn))
    {
        return NULL;
    }

    bool res;

    string sym = get_parameter_representation(env, st_point &point, string
parname);

        sim_get_sym_value_by_parname_and_level(env, st_string(pn), sv, res);

    if(!res)
    {
        return NULL;
    }

    return (new st_string(sym));
}
*/

st_object *st_ast_is_infinite(st_object *obj, st_env *env) {
  obj = st_ast_get_par(obj, 0);

  if (to<st_double *>(obj))
    return (new st_integer(!isfinite(to<st_double *>(obj)->get_double())));

  return (new st_integer(0));
}

st_object *st_ast_is_nan(st_object *obj, st_env *env) {
  obj = st_ast_get_par(obj, 0);

  if (to<st_double *>(obj))
    return (new st_integer(isnan(to<st_double *>(obj)->get_double())));

  if (to<st_integer *>(obj))
    return (new st_integer(0));

  return (new st_integer(1));
}

st_object *st_ast_is_valid(st_object *obj, st_env *env) {
  obj = st_ast_get_par(obj, 0);

  if (!to<st_point *>(obj)) {
    return NULL;
  }
  st_point *p = to<st_point *>(obj);

  if (!p->check_consistency(env))
    return (new st_integer(0));

  if (p->get_error())
    return (new st_integer(0));
  return (new st_integer(1));
}

st_object *st_ast_config_to_point(st_object *obj, st_env *env) {
  st_point p;
  st_object *conf_vector = st_ast_get_par(obj, 0);

  for (int i = 0; i < env->current_design_space->ds_parameters.size(); i++) {
    st_object *config = st_ast_get_par(conf_vector, i);

    if (!config)
      throw std::logic_error("Non feasible configuration");

    st_string *pval = to<st_string *>(config);

    if (!pval)
      throw std::logic_error("Non feasible configuration");

    string sv = pval->get_string();

    int level = env->current_design_space->get_scalar_level(
        env, env->current_design_space->ds_parameters[i].name, sv);

    p.push_back(level);
  }
  return p.gen_copy();
}

st_object *st_ast_get_point(st_object *obj, st_env *env) {
  st_object *dbname = st_ast_get_par(obj, 0);
  st_object *point = st_ast_get_par(obj, 1);
  if (!dbname && !point)
    return NULL;
  if (!to<st_string *>(dbname) || !to<st_point *>(point))
    return NULL;
  st_database *db = env->get_database(to<st_string *>(dbname)->get_string());
  if (!db)
    return NULL;
  st_point *p = db->look_for_point(to<st_point *>(point));
  if (p)
    return p->gen_copy();
  else {
    return sim_generate_dummy_point(env);
  }
}

st_object *st_ast_get_database_list(st_object *obj, st_env *env) {
  obj = st_ast_get_par(obj, 0);

  if (!obj)
    return NULL;
  else {
    if (!to<st_string *>(obj))
      return NULL;
    string name = to<st_string *>(obj)->get_string();
    if (!env->available_dbs.count(name))
      return NULL;
    st_database *db = env->available_dbs[name];
    st_point_set *set = db->get_set_of_points();
    st_list *list = new st_list();
    st_point_set::iterator i;
    for (i = set->begin(); i != set->end(); i++) {
      list->insert(*(i->second));
    }
    return list;
  }
}

st_object *st_ast_get_database_vector(st_object *obj, st_env *env) {
  obj = st_ast_get_par(obj, 0);

  if (!obj)
    return NULL;
  else {
    if (!to<st_string *>(obj))
      return NULL;
    string name = to<st_string *>(obj)->get_string();
    if (!env->available_dbs.count(name))
      return NULL;
    st_database *db = env->available_dbs[name];
    st_point_set *set = db->get_set_of_points();
    st_vector *vec = new st_vector();
    st_point_set::iterator i;
    int n = 0;
    for (i = set->begin(); i != set->end(); i++) {
      vec->insert(n, *(i->second));
      n++;
    }
    return vec;
  }
}

st_object *st_ast_get_size(st_object *obj, st_env *env) {
  obj = st_ast_get_par(obj, 0);

  if (!obj)
    return (new st_integer(0));

  if (!to<st_vector *>(obj) && !to<st_list *>(obj) && !to<st_map *>(obj))
    return (new st_integer(0));

  if (to<st_vector *>(obj))
    return (new st_integer(to<st_vector *>(obj)->size()));

  if (to<st_list *>(obj))
    return (new st_integer(to<st_list *>(obj)->size()));

  if (to<st_map *>(obj))
    return (new st_integer(to<st_map *>(obj)->size()));

  return (new st_integer(0));
}

st_object *st_ast_change_element(st_object *obj, st_env *env) {
  if (!obj)
    return (new st_integer(0));

  if (!to<st_vector *>(obj))
    return (new st_integer(0));

  st_vector *param = to<st_vector *>(obj);

  if (param->size() < 3)
    return NULL;

  if (is_a<st_vector const *>(&param->get(0))) {
    if (!is_a<st_integer const *>(&param->get(1)))
      return NULL;

    int index = to<st_integer const *>(&param->get(1))->get_integer();
    st_vector *x = to<st_vector *>(param->get(0).gen_copy());
    x->insert(index, param->get(2));
    return (x);
  }

  if (is_a<st_map const *>(&param->get(0))) {
    if (!is_a<st_string const *>(&param->get(1)))
      return NULL;

    string key = to<st_string const *>(&param->get(1))->get_string();
    st_map *x = to<st_map *>(param->get(0).gen_copy());
    x->insert(key, param->get(2));
    return (x);
  }

  return NULL;
}

/* FIXME: Is it needed?
st_object *st_ast_get_bound(st_object *obj, st_env *env)
{
    obj = st_ast_get_par(obj,0);

    if(!obj)
        return NULL;

    if(!to<st_string *>(obj))
        return NULL;

    string bound = to<st_string *>(obj)->get_string();

    if(bound == "lower")
        return (new st_point(opt_get_lower_bound(env)));
    else
    {
        if(bound == "upper")
            return (new st_point(opt_get_upper_bound(env)));
    }
    return NULL;
}
*/

bool get_definition_stack_level(st_object *o, int &level) {
  if (to<st_map *>(o)) {
    level = to<st_map *>(o)->get_integer("stack_level");
    return true;
  }
  return false;
}

bool is_anon_deref(st_object *o, string &name) {
  if (to<st_map *>(o)) {
    try {
      if ((to<st_map *>(o)->get_string("meta") == META_DEREF_ANON)) {
        name = (to<st_map *>(o)->get_string("name"));
        return true;
      }

    } catch (exception e) {
    }
  }
  return false;
}

string get_string(st_object *vec, int index) {
  st_object *o = st_ast_get_par(vec, index);
  if (to<st_string *>(o))
    return to<st_string *>(o)->get_string();

  throw std::logic_error("Expecting a string");
}

double get_int_or_double(st_object *vec, int index) {
  st_object *o = st_ast_get_par(vec, index);
  if (to<st_double *>(o))
    return to<st_double *>(o)->get_double();

  if (to<st_integer *>(o))
    return to<st_integer *>(o)->get_integer();

  throw std::logic_error("Expecting a double or an integer");
}

ofstream normal_out;
ifstream normal_in;

pthread_mutex_t normal_lock;

bool st_initialize_gauss(st_env *env) {
  int res = mkfifo("normal_input", S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
  if ((res == -1) && (errno != EEXIST)) {
    return false;
  }
  res = mkfifo("normal_output", S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
  if ((res == -1) && (errno != EEXIST)) {
    return false;
  }
  string command = "normal";
  string cbp;
  if (env->shell_variables.get_string("current_build_path", cbp)) {
    command = cbp + "/bin/" + command;
  }
  if (!fork()) {
    /*prs_display_message(("Opening "+command).c_str());*/
#if !defined(__MAC_OSX__)
    if (!shell_command(command + " normal_output normal_input")) {
      cout << "Problems with the normal program\n";
      exit(0);
    }
    exit(0);
#else
    int res = execl(command.c_str(), command.c_str(), "normal_output",
                    "normal_input");
    if (res == -1) {
      cout << "Problems with the normal program\n";
      exit(0);
    }
    exit(0);
#endif
  }
  normal_out.open("normal_output");
  normal_in.open("normal_input");
  pthread_mutex_init(&normal_lock, NULL);
  return true;
}

st_object *st_ast_gauss(st_object *obj, st_env *env) {
  double mean_x = get_int_or_double(obj, 0);
  double mean_y = get_int_or_double(obj, 1);
  double std_x = get_int_or_double(obj, 2);
  double std_y = get_int_or_double(obj, 3);
  double rho = get_int_or_double(obj, 4);

  vector<double> xstat;
  vector<double> ystat;

  xstat.push_back(mean_x);
  xstat.push_back(std_x);
  xstat.push_back(rho);

  ystat.push_back(mean_y);
  ystat.push_back(std_y);

  pthread_mutex_lock(&normal_lock);

  if (!normal_out.good()) {
    pthread_mutex_unlock(&normal_lock);
    throw std::logic_error("Problems at the interface with the normalizer");
  }
  st_xdr_write_raw_data(normal_out, xstat, ystat);

  vector<double> raw_data;
  if (!normal_in.good()) {
    pthread_mutex_unlock(&normal_lock);
    throw std::logic_error("Problems at the interface with the normalizer");
  }

  if (!st_xdr_read_vector(normal_in, raw_data)) {
    pthread_mutex_unlock(&normal_lock);
    throw std::logic_error("Problems at the interface with the normalizer");
  }

  if (raw_data.size() < 2) {
    pthread_mutex_unlock(&normal_lock);
    throw std::logic_error("Problems at the interface with the normalizer");
  }

  st_vector *ret = new st_vector();
  ret->insert(0, st_double(raw_data[0]));
  ret->insert(1, st_double(raw_data[1]));
  pthread_mutex_unlock(&normal_lock);
  return ret;
}
extern bool st_rsm_read_from_raw_data(vector<double> &row_data,
                                      int num_predictors, vector<double> &coeff,
                                      double &Radj, double &F, double &PF,
                                      vector<double> &T, vector<double> &PT);

st_object *st_ast_linear_regression(st_object *obj, st_env *env) {
  st_vector *designs = to<st_vector *>(st_ast_get_par(obj, 0));
  st_vector *observations = to<st_vector *>(st_ast_get_par(obj, 1));

  if (!designs || !observations)
    throw std::logic_error("Please specify correctly observations and designs");

  int sz = designs->size();
  if (sz != observations->size())
    throw std::logic_error(
        "Observations and designs should have the same size");

  string file_out = "st_lr_out.dat";
  string file_in = "st_lr_in.dat";
  string command = "rm -rf " + file_out;
  if (!shell_command(command)) {
    throw std::logic_error("Problems at the interface with the regressor");
  }
  ofstream fx(file_out.c_str());
  if (!fx.good()) {
    throw std::logic_error("Problems at the interface with the regressor");
  }
  int predictors = 0;
  for (int i = 0; i < sz; i++) {
    vector<double> obs;
    double observ = get_int_or_double(observations, i);
    obs.push_back(observ);
    st_vector *the_design = to<st_vector *>(st_ast_get_par(designs, i));
    if (!the_design)
      throw std::logic_error(
          "Please specify correctly observations and designs");
    vector<double> des;
    if (!i)
      predictors = the_design->size();
    else {
      if (predictors != the_design->size())
        throw std::logic_error(
            "Please specify correctly observations and designs");
    }
    for (int k = 0; k < the_design->size(); k++) {
      des.push_back(get_int_or_double(the_design, k));
    }
    st_xdr_write_raw_data(fx, des, obs);
  }
  fx.close();
  command = "regressor";
  string cbp;
  if (env->shell_variables.get_string("current_build_path", cbp)) {
    command = cbp + "/gpl/" + command;
  }
  command = command + " " + file_out + " " + file_in;
  if (!shell_command("rm -rf " + file_in)) {
    throw std::logic_error("Problems at the interface with the regressor");
  }
  vector<double> raw_data;
  vector<double> residuals;
  vector<double> zj;
  ifstream ifx(file_in.c_str());
  if (!st_xdr_read_vector(ifx, raw_data)) {
    throw std::logic_error("Problems at the interface with the regressor");
  }
  st_xdr_read_vector(ifx, residuals);
  st_xdr_read_vector(ifx, zj);
  ifx.close();
  vector<double> coeff;
  vector<double> T;
  vector<double> PT;
  double F;
  double Radj;
  double PF;
  if (!st_rsm_read_from_raw_data(raw_data, predictors, coeff, Radj, F, PF, T,
                                 PT)) {
    throw std::logic_error("Problems at the interface with the regressor");
  }
  st_map *resul = new st_map();

  resul->insert("coefficients", st_vector(coeff));
  resul->insert("radj", st_double(Radj));
  resul->insert("f", st_double(F));
  resul->insert("pf", st_double(PF));
  resul->insert("t", st_vector(T));
  resul->insert("pt", st_vector(PT));
  resul->insert("residuals", st_vector(residuals));
  resul->insert("zj", st_vector(zj));
  return resul;
}

void st_ast_initialize_ast() {
  st_ast_functions["properties"] = st_ast_get_properties;
  st_ast_functions["metrics"] = st_ast_get_metrics;
  st_ast_functions["set_metric_value"] = st_ast_set_metrics;
  st_ast_functions["objectives"] = st_ast_get_objectives;
  st_ast_functions["get_objectives_list"] = st_ast_get_objectives_list;
  st_ast_functions["get_objectives_fun"] = st_ast_get_objectives_fun;
  st_ast_functions["get_scalar_parameter_list"] =
      st_ast_get_scalar_parameter_list;
  st_ast_functions["get_scalar_parameter_index_list"] =
      st_ast_get_scalar_parameter_index_list;
  st_ast_functions["is_valid"] = st_ast_is_valid;
  st_ast_functions["is_infinite"] = st_ast_is_infinite;
  st_ast_functions["is_nan"] = st_ast_is_nan;
  st_ast_functions["get_points_list"] = st_ast_get_database_list;
  st_ast_functions["get_points_vector"] = st_ast_get_database_vector;
  st_ast_functions["get_similar_point"] = st_ast_get_point;
  st_ast_functions["config_to_point"] = st_ast_config_to_point;
  st_ast_functions["size"] = st_ast_get_size;
  /* st_ast_functions["get_bound"] = st_ast_get_bound; */
  st_ast_functions["change_element"] = st_ast_change_element;
  st_ast_functions["get_time"] = st_ast_get_time;
  st_ast_functions["abs"] = st_ast_abs;
  st_ast_functions["pow"] = st_ast_pow;
  st_ast_functions["exp"] = st_ast_exp;
  st_ast_functions["log"] = st_ast_log;
  st_ast_functions["system"] = st_ast_system;
  st_ast_functions["sqrt"] = st_ast_sqrt;
  st_ast_functions["ceil"] = st_ast_ceil;
  st_ast_functions["floor"] = st_ast_floor;
  st_ast_functions["round"] = st_ast_round;
  st_ast_functions["fmod"] = st_ast_fmod;
  st_ast_functions["decimal_part"] = st_ast_decimal_part;
  st_ast_functions["integer_part"] = st_ast_integer_part;
  st_ast_functions["sort_vector_asc"] = st_ast_sort_vector_asc;
  st_ast_functions["maxf"] = st_ast_max;
  st_ast_functions["minf"] = st_ast_min;
  st_ast_functions["gauss"] = st_ast_gauss;
  /* st_ast_functions["level_sym_value"] = st_ast_level_sym_value; */
  st_ast_functions["level"] = st_ast_level;
  st_ast_functions["get_level"] = st_ast_get_level;
  st_ast_functions["get_min_level"] = st_ast_get_min_level;
  st_ast_functions["get_max_level"] = st_ast_get_max_level;
  st_ast_functions["get_integer_representation"] =
      st_ast_get_integer_representation;
  st_ast_functions["printf"] = st_ast_printf;
  st_ast_functions["linear_regression"] = st_ast_linear_regression;

  st_initialize_gauss(&current_environment);

  map<string, st_function_name>::iterator i;
  for (i = st_ast_functions.begin(); i != st_ast_functions.end(); i++) {
    st_add_possible_command(i->first);
  }
  st_add_possible_command("get_points");
  st_add_possible_command("sum");
  st_add_possible_command("prod");
  st_add_possible_command("avg");
  st_add_possible_command("geomavg");
  sort(possible_commands.begin(), possible_commands.end());
}

void st_ast_free_ast() {
  map<string, st_user_def_function>::iterator i;
  for (i = st_user_defined_functions.begin();
       i != st_user_defined_functions.end(); i++) {
    delete i->second.function_expression;
  }
  pthread_mutex_destroy(&normal_lock);
}

void st_print_ast_list(list<st_ast *> &l, bool commas) {
  list<st_ast *>::iterator i = l.begin();
  while (i != l.end()) {
    (*i)->print();
    i++;
    if ((i) != l.end()) {
      if (commas)
        cout << ", ";
      else
        cout << " ";
    }
  }
}

void st_copy_ast_list(list<st_ast *> &l, list<st_ast *> &dest) {
  list<st_ast *>::iterator i = l.begin();
  while (i != l.end()) {
    dest.push_back((*i)->copy());
    i++;
  }
}

void st_copy_ast_map(map<string, st_ast *> &m1, map<string, st_ast *> &m2) {
  map<string, st_ast *>::iterator i = m1.begin();
  while (i != m1.end()) {
    pair<string, st_ast *> p(i->first, i->second->copy());
    m2.insert(p);
    i++;
  }
}

void st_delete_ast_list(list<st_ast *> &l) {
  list<st_ast *>::iterator i = l.begin();
  while (i != l.end()) {
    if (*i)
      (delete *i);
    i++;
  }
}

void st_set_current_scoped_value(st_ast_expression_stack *stack, string &name,
                                 st_object *expr) {
  if ((*(stack))[(stack)->size() - 1].count((name))) {
    st_object_discard_local((*(stack))[(stack)->size() - 1][(name)]);
  }
  if (expr->is_global() || expr->is_stack_permanent()) {
    (*stack)[(stack)->size() - 1][(name)] = expr->gen_copy();
    (*stack)[(stack)->size() - 1][(name)]->set_stack_permanent();
  } else {
    (*stack)[(stack)->size() - 1][(name)] = expr;
    (*stack)[(stack)->size() - 1][(name)]->set_stack_permanent();
  }
}

#define st_is_a_local_procedure(stack) ((stack)->size() >= 1)

//#define DEBUG

// TOP
st_object *st_ast_eval(st_ast *ast, st_ast_expression_stack *stack,
                       st_env *env) {
  if (ast) {

#if defined(DEBUG)
    cout << "******************************" << endl;
    cout << "Evaluating AST: " << endl;
    ast->print();
    cout << endl;
    cout << "Current AST Stack is: " << endl;
    st_print_stack(stack);
#endif
    if (to<st_ast_set *>(ast)) {
      st_ast_set *cast = to<st_ast_set *>(ast);
      st_string vname = cast->variable_name;
      st_object *expr_value = st_ast_eval(cast->expression, stack, env);
      if (!expr_value) {
        last_known_ast = ast;
        throw std::logic_error("Problems in the evaluation of the expression");
      }
      if (!cast->at) {
        if (st_is_a_local_procedure(stack) && cast->local) {
          int verbose;
          bool v = env->shell_variables.get_integer("verbose_local", verbose);
          st_set_current_scoped_value(stack, cast->variable_name, expr_value);
          if (v && verbose)
            st_display_message_assigned(env, cast->variable_name, expr_value);
          st_object_discard(expr_value);
        } else {
          // cout << "!!!!!!!!!!!!!!!!!! Assigning to " << vname.get_string() <<
          // " " <<  expr_value->print() << endl; st_print_stack(stack);
          prs_command_set(&vname, expr_value);
          // cout << ">>>>>>>>>>>>>>>>>" << endl;
          // st_print_stack(stack);
        }
        return NULL;
      } else {
        st_object *var =
            st_get_current_scoped_value(cast->variable_name, stack, env);
        if (!var) {
          st_object const *the_obj;
          if (!env->shell_variables.get(cast->variable_name, the_obj)) {
            last_known_ast = ast;
            throw std::logic_error("Variable " + cast->variable_name +
                                   " does not exist");
          }
          var = const_cast<st_object *>(the_obj);
        }
        if (!to<st_vector *>(var)) {
          last_known_ast = ast;
          throw std::logic_error("Variable " + cast->variable_name +
                                 " should be a vector");
        }
        st_object *index_value = st_ast_eval(cast->index, stack, env);
        if (!index_value || !to<st_integer *>(index_value)) {
          last_known_ast = ast;
          throw std::logic_error("Problems in the evaluation of the index");
        }
        int index = to<st_integer *>(index_value)->get_integer();
        to<st_vector *>(var)->insert(index, *expr_value);
        ostringstream linestream;
        linestream << index;
        if (st_is_a_local_procedure(stack) && cast->local) {
          int verbose;
          bool v = env->shell_variables.get_integer("verbose_local", verbose);
          if (v && verbose)
            st_display_message_assigned(
                env, cast->variable_name + "@" + linestream.str(), expr_value);
        } else {
          st_display_message_assigned(
              env, cast->variable_name + "@" + linestream.str(), expr_value);
        }
        st_object_discard(expr_value);
        return NULL;
      }
    }
    if (to<st_ast_echo *>(ast)) {
      st_ast_echo *cast = to<st_ast_echo *>(ast);
      st_object *expr_value = st_ast_eval(cast->expression, stack, env);
      if (!expr_value) {
        last_known_ast = ast;
        throw std::logic_error("Problems in the evaluation of the expression");
      }
      prs_command_echo(expr_value, cast->mode, cast->file_name);
      st_object_discard(expr_value);
      return NULL;
    }
    if (to<st_ast_simple_command *>(ast)) {
      st_ast_simple_command *cast = to<st_ast_simple_command *>(ast);
      st_string vname = cast->command_name;
      st_map cp;

      if (cast->command_name == "quit" || cast->command_name == "exit") {
        last_known_ast = ast;
        st_ast_return_exception e;
        throw e;
      }

      if (vname.get_string() == "return") {
        if (cast->command_parameters.size() != 0) {
          map<string, st_ast *>::iterator i;

          for (i = cast->command_parameters.begin();
               i != cast->command_parameters.end(); i++) {
            if (i->first == "") {
              st_object *arg;
              try {
                arg = st_ast_eval(i->second, stack, env);
              } catch (exception &x) {
                throw;
              }
              st_ast_return_exception e;
              if (arg->is_global()) {
                e.value = arg;
                throw e;
              } else {
                if (arg->is_stack_permanent()) {
                  map<string, st_object *>::iterator i;
                  st_ast_stack_frame &the_frame = (*stack)[(stack)->size() - 1];
                  for (i = the_frame.begin(); i != the_frame.end(); i++) {
                    if (i->second == arg) {
                      the_frame.erase(i);
                      break;
                    }
                  }
                  arg->unset_stack_permanent();
                  e.value = arg;
                  throw e;
                } else {
                  e.value = arg->gen_copy();
                  delete arg;
                  throw e;
                }
              }
            }
          }
          throw std::logic_error("Bad return specified");
        }
        st_ast_return_exception e;
        throw e;
      }
      map<string, st_ast *>::iterator i;

      for (i = cast->command_parameters.begin();
           i != cast->command_parameters.end(); i++) {
        string key = (i->first);
        st_object *arg;
        try {
          arg = st_ast_eval(i->second, stack, env);
        } catch (exception &e) {
          cp.discard_temporary();
          throw;
        }
        if (arg) {
          cp.insert_dont_copy(key, arg);
        } else {
          cp.discard_temporary();
          last_known_ast = ast;
          throw std::logic_error("Argument cannot be resolved");
        }
      }
      last_known_ast = ast;
      st_parse_command(&vname, &cp);
      cp.discard_temporary();
      return NULL;
    }
    if (to<st_ast_variable *>(ast)) {
      st_ast_variable *cast = to<st_ast_variable *>(ast);

      int stack_level = -1;
      st_string vname = cast->variable_name;
      st_object *localvar;

      if (st_in_anonymous_function(stack, env, stack_level)) {
        // st_print_stack(stack);

        /* Anonymous functions have a precedence in their definition scope */
        localvar =
            st_get_scoped_value(vname.get_string(), stack, env, stack_level);

        if (localvar)
          return localvar;
      }

      localvar = st_get_current_scoped_value(vname.get_string(), stack, env);

      if (localvar)
        return localvar;

      st_object *p = prs_command_read_variable(&vname, false);
      if (!p) {
        /**
         * Hoohoo, beware. This may break something.
         * Accessing variables in the dynamic scope of function calls.
         * This is to make anonymous functions work. (VZ)
         */
        p = st_get_current_scoped_value(vname.get_string(), stack, env);

        if (!p) {
          last_known_ast = ast;
          throw std::logic_error("Variable " + cast->variable_name +
                                 " does not exist");
        }
      }
      return p;
    }
    if (to<st_ast_leaf_R *>(ast)) {
      st_ast_leaf_R *cast = to<st_ast_leaf_R *>(ast);
      ofstream R_cmd("R_tmp.scr");
      R_cmd << cast->leaf_value << endl;
      R_cmd.close();
      bool re = shell_command("R --vanilla --file=R_tmp.scr --quiet", true);
      st_object *ret = new st_integer(re);
      return ret;
    }
    if (to<st_ast_leaf *>(ast)) {
      st_ast_leaf *cast = to<st_ast_leaf *>(ast);
      st_object *ret = cast->leaf_value->gen_copy();
      return ret;
    }
    if (to<st_ast_define_objective *>(ast)) {
      st_ast_define_objective *cast = to<st_ast_define_objective *>(ast);

      env->remove_objective(cast->name);

      st_objective *s = new st_objective(cast->name, cast->objective_expression,
                                         cast->point_name);
      prs_display_message_n_action_m("Setting objective " + cast->name + " to ",
                                     cast->objective_expression->print(), "");
      env->shell_variables.set_integer("objectives_number",
                                       env->optimization_objectives.size() + 1);
      env->optimization_objectives.push_back(s);
      env->optimization_timestamp++;
      st_add_possible_command(cast->name);
      sort(possible_commands.begin(), possible_commands.end());
      return NULL;
    }
    if (to<st_ast_define_metric *>(ast)) {
      st_ast_define_metric *cast = to<st_ast_define_metric *>(ast);

      env->remove_companion_metric(cast->name);

      st_companion_metric *s = new st_companion_metric(
          cast->name, cast->metric_expression, cast->point_name);
      prs_display_message_n_action_m("Setting metric " + cast->name + " to ",
                                     cast->metric_expression->print(), "");
      env->companion_metrics.push_back(s);
      env->optimization_timestamp++;
      st_add_possible_command(cast->name);
      sort(possible_commands.begin(), possible_commands.end());
      return NULL;
    }
    if (to<st_ast_define_constraint *>(ast)) {
      st_ast_define_constraint *cast = to<st_ast_define_constraint *>(ast);

      st_ast *obj_expr = cast->constraint_expression;
      if (!to<st_ast_expression *>(obj_expr)) {
        last_known_ast = ast;
        throw std::logic_error("Cannot define constraint");
      }

      string op = to<st_ast_expression *>(obj_expr)->operator_name;

      st_ast *left = to<st_ast_expression *>(obj_expr)->left;

      st_ast *right = to<st_ast_expression *>(obj_expr)->right;

      if (op != ">" && op != "<" && op != "==" && op != "<=" && op != ">=" &&
          op != "==") {
        last_known_ast = ast;
        throw std::logic_error("Cannot define constraint");
      }

      env->remove_constraint(cast->name);

      st_constraint *s =
          new st_constraint(cast->name, op, left, right, cast->point_name);
      prs_display_message_n_action_m("Setting constraint " + cast->name +
                                         " to ",
                                     cast->constraint_expression->print(), "");
      env->optimization_constraints.push_back(s);
      env->optimization_timestamp++;
      return NULL;
    }

    if (to<st_ast_define_anon_function *>(ast)) {
      st_ast_define_anon_function *cast =
          to<st_ast_define_anon_function *>(ast);

      if (cast->existing == false) {
        cast->existing_name = st_ast_get_afun_name();
        st_user_def_function newfunction;
        newfunction.function_expression = cast->function_expression->copy();
        if (!to<st_ast_construct_vector *>(cast->local_parameters)) {
          last_known_ast = ast;
          throw std::logic_error("Cannot define function.");
        }
        list<st_ast *>::iterator i;
        for (i = to<st_ast_construct_vector *>(cast->local_parameters)
                     ->vector_coordinates.begin();
             i != to<st_ast_construct_vector *>(cast->local_parameters)
                      ->vector_coordinates.end();
             i++) {
          if (!to<st_ast_identifier *>(*i)) {
            last_known_ast = ast;
            throw std::logic_error("Bad function variables specified");
          }
          st_object *eval = st_ast_eval(*i, stack, env);
          st_assert(to<st_string *>(eval));
          newfunction.local_parameters.push_back(
              to<st_string *>(eval)->get_string());
          st_object_discard(eval);
        }
        // cout << "Adding " << cast->existing_name << endl;
        st_user_defined_functions[cast->existing_name] = newfunction;
      }

      st_map *p = to<st_map *>((new st_map()));

      st_string key1 = st_string("meta");
      st_string val1 = st_string(META_DEREF_ANON);

      prs_add_element_to_map(p, &key1, &val1);

      st_string key2 = st_string("name");
      st_string val2 = st_string(cast->existing_name);

      prs_add_element_to_map(p, &key2, &val2);

      /* If the function is defined at top, it should access only global
       * variables, otherwise it should access variables at the level of the
       * stack in which it has been defined */

      st_string key3 = st_string("stack_level");
      st_integer val3 = st_integer(stack->size());

      prs_add_element_to_map(p, &key3, &val3);

      return (p);
    }
    if (to<st_ast_define_function *>(ast)) {

      st_ast_define_function *cast = to<st_ast_define_function *>(ast);

      if (st_user_defined_functions.count(cast->name)) {
        if (st_user_defined_functions[cast->name].function_expression)
          delete st_user_defined_functions[cast->name].function_expression;

        st_user_defined_functions.erase(
            st_user_defined_functions.find(cast->name));
      }
      st_user_def_function newfunction;
      newfunction.function_expression = cast->function_expression->copy();
      if (!to<st_ast_construct_vector *>(cast->local_parameters)) {
        last_known_ast = ast;
        throw std::logic_error("Cannot define function.");
      }
      list<st_ast *>::iterator i;
      for (i = to<st_ast_construct_vector *>(cast->local_parameters)
                   ->vector_coordinates.begin();
           i != to<st_ast_construct_vector *>(cast->local_parameters)
                    ->vector_coordinates.end();
           i++) {
        if (!to<st_ast_identifier *>(*i)) {
          last_known_ast = ast;
          throw std::logic_error("Bad function variables specified");
        }
        st_object *eval = st_ast_eval(*i, stack, env);
        st_assert(to<st_string *>(eval));
        newfunction.local_parameters.push_back(
            to<st_string *>(eval)->get_string());
        st_object_discard(eval);
      }
      st_user_defined_functions[cast->name] = newfunction;
      st_add_possible_command(cast->name);
      sort(possible_commands.begin(), possible_commands.end());
      return NULL;
    }
    if (to<st_ast_define_procedure *>(ast)) {

      st_ast_define_procedure *cast = to<st_ast_define_procedure *>(ast);

      if (st_user_defined_functions.count(cast->name)) {
        if (st_user_defined_functions[cast->name].function_expression)
          delete st_user_defined_functions[cast->name].function_expression;

        st_user_defined_functions.erase(
            st_user_defined_functions.find(cast->name));
      }
      st_user_def_function newfunction;
      newfunction.function_expression = cast->compound_commands->copy();
      if (!to<st_ast_construct_vector *>(cast->local_parameters)) {
        last_known_ast = ast;
        throw std::logic_error("Cannot define procedure.");
      }
      list<st_ast *>::iterator i;
      for (i = to<st_ast_construct_vector *>(cast->local_parameters)
                   ->vector_coordinates.begin();
           i != to<st_ast_construct_vector *>(cast->local_parameters)
                    ->vector_coordinates.end();
           i++) {
        if (!to<st_ast_identifier *>(*i)) {
          last_known_ast = ast;
          throw std::logic_error("Bad function variables specified");
        }
        st_object *eval = st_ast_eval(*i, stack, env);
        st_assert(to<st_string *>(eval));
        newfunction.local_parameters.push_back(
            to<st_string *>(eval)->get_string());
        st_object_discard(eval);
      }
      st_user_defined_functions[cast->name] = newfunction;
      st_add_possible_command(cast->name);
      sort(possible_commands.begin(), possible_commands.end());
      return NULL;
    }
    if (to<st_ast_command_for *>(ast)) {
      st_ast_command_for *cast = to<st_ast_command_for *>(ast);

      bool db_loop = false;
      st_point_set::iterator s;
      st_point_set::iterator end;
      if (to<st_ast_function_call *>(cast->list_of_values)) {
        st_ast_function_call *f =
            to<st_ast_function_call *>(cast->list_of_values);
        if (f->function_name == "get_points") {
          st_object *opeval = st_ast_eval(f->operand, stack, env);
          st_object *dbname = st_ast_get_par(opeval, 0);
          if (!to<st_string *>(dbname)) {
            last_known_ast = ast;
            st_object_discard(opeval);
            throw std::logic_error("Please specify a valid db name");
          }
          string nam = to<st_string *>(dbname)->get_string();
          st_database *db = env->get_database(nam);
          if (!db) {
            last_known_ast = ast;
            st_object_discard(opeval);
            throw std::logic_error("Please specify a valid db name");
          }
          st_object_discard(opeval);
          s = db->get_set_of_points()->begin();
          end = db->get_set_of_points()->end();
          db_loop = true;
        }
      }
      if (!db_loop) {
        st_object *list_eval = st_ast_eval(cast->list_of_values, stack, env);
        if (!is_a<st_list *>(list_eval)) {
          last_known_ast = ast;
          st_object_discard(list_eval);
          throw std::logic_error(
              "Please specify a valid list of values for the value");
        }
        st_list *the_list = to<st_list *>(list_eval);
        list<st_object *>::iterator i;
        for (i = the_list->begin(); i != the_list->end(); i++) {
          if (!st_is_a_local_procedure(stack))
            env->shell_variables.insert(cast->iterator_variable, **i);
          else {
            st_set_current_scoped_value(stack, cast->iterator_variable,
                                        (*i)->gen_copy());
          }

          list<st_ast *>::iterator k;
          for (k = cast->compound_commands.begin();
               k != cast->compound_commands.end(); k++) {
            /** Discard output value, since we are in a while statement */
            try {
              st_object *com = st_ast_eval(*k, stack, env);
              st_object_discard(com);
            } catch (exception &e) {
              st_object_discard(list_eval);
              throw;
            }
          }
        }
        st_object_discard(list_eval);
        return NULL;
      } else {
        for (; s != end; s++) {
          st_point *p = s->second;

          // cout << "++++++++++++++++++++++++++++++++++++++ " << endl;
          // st_print_stack(stack);

          if (!st_is_a_local_procedure(stack))
            env->shell_variables.insert(cast->iterator_variable, *p);
          else {
            st_set_current_scoped_value(stack, cast->iterator_variable,
                                        p->gen_copy());
          }

          // cout << "-------------------------------------^ " << endl;
          // st_print_stack(stack);

          list<st_ast *>::iterator k;
          for (k = cast->compound_commands.begin();
               k != cast->compound_commands.end(); k++) {
            /** Discard output value, since we are in a while statement */
            try {
              st_object *com = st_ast_eval(*k, stack, env);
              st_object_discard(com);
            } catch (exception &e) {
              throw;
            }
          }
        }
        return NULL;
      }
    }
    if (to<st_ast_command_while *>(ast)) {
      st_ast_command_while *cast = to<st_ast_command_while *>(ast);

      st_object *e = st_ast_eval(cast->evaluation_expression, stack, env);
      bool is_integer = to<st_integer *>(e) != NULL;
      if (!is_integer) {
        last_known_ast = ast;
        st_object_discard(e);
        throw std::logic_error("While expression should be an integer value");
      }
      int val = to<st_integer *>(e)->get_integer();
      st_object_discard(e);

      while (is_integer && val) {
        list<st_ast *>::iterator i;
        for (i = cast->compound_commands.begin();
             i != cast->compound_commands.end(); i++) {
          /** Discard output value, since we are in a while statement */
          st_object *com = st_ast_eval(*i, stack, env);
          st_object_discard(com);
        }
        e = st_ast_eval(cast->evaluation_expression, stack, env);
        is_integer = to<st_integer *>(e) != NULL;
        if (!is_integer) {
          last_known_ast = ast;
          st_object_discard(e);
          throw std::logic_error("While expression should be an integer value");
        }
        val = to<st_integer *>(e)->get_integer();
        st_object_discard(e);
      }
      return NULL;
    }
    if (to<st_ast_command_if *>(ast)) {
      st_ast_command_if *cast = to<st_ast_command_if *>(ast);

      st_object *e = st_ast_eval(cast->evaluation_expression, stack, env);
      bool is_integer = to<st_integer *>(e) != NULL;
      if (!is_integer) {
        last_known_ast = ast;
        throw std::logic_error("If expression should be an integer value");
      }
      int val = to<st_integer *>(e)->get_integer();
      st_object_discard(e);
      if (is_integer && val) {
        list<st_ast *>::iterator i;
        for (i = cast->then_commands.begin(); i != cast->then_commands.end();
             i++) {
          /** Discard output value, since we are in a if statement */
          st_object *com = st_ast_eval(*i, stack, env);
          st_object_discard(com);
        }
      } else {
        list<st_ast *>::iterator i;
        for (i = cast->else_commands.begin(); i != cast->else_commands.end();
             i++) {
          st_object *com = st_ast_eval(*i, stack, env);
          st_object_discard(com);
        }
      }
      return NULL;
    }

    if (to<st_ast_construct_point *>(ast)) {
      st_ast_construct_point *cast = to<st_ast_construct_point *>(ast);
      st_point *p = to<st_point *>((new st_point()));
      list<st_ast *>::iterator i;
      for (i = cast->point_coordinates.begin();
           i != cast->point_coordinates.end(); i++) {
        st_object *res;
        try {
          res = st_ast_eval(*i, stack, env);
        } catch (exception &e) {
          st_object_discard(p);
          throw;
        }
        if (!res) {
          last_known_ast = ast;
          st_object_discard(p);
          throw std::logic_error("Error while parsing point coordinates");
        }
        /** Check if it's an integer */
        if (!to<st_integer *>(res)) {
          last_known_ast = ast;
          st_object_discard(p);
          st_object_discard(res);
          throw std::logic_error("Point coordinates need to be integers");
        }
        prs_add_coord_to_point(p, res);
        st_object_discard(res);
      }
      return (p);
    }
    if (to<st_ast_construct_vector *>(ast)) {
      st_ast_construct_vector *cast = to<st_ast_construct_vector *>(ast);
      st_vector *p = to<st_vector *>((new st_vector()));
      list<st_ast *>::iterator i;
      for (i = cast->vector_coordinates.begin();
           i != cast->vector_coordinates.end(); i++) {
        st_object *res;
        try {
          res = st_ast_eval(*i, stack, env);
        } catch (exception &e) {
          st_object_discard(p);
          throw;
        }
        if (!res) {
          last_known_ast = ast;
          st_object_discard(p);
          throw std::logic_error("Error while parsing vector coordinates");
        }
        prs_insert_in_vector(p, res);
        st_object_discard(res);
      }
      return (p);
    }
    if (to<st_ast_construct_map *>(ast)) {
      st_ast_construct_map *cast = to<st_ast_construct_map *>(ast);
      st_map *p = to<st_map *>((new st_map()));
      map<string, st_ast *>::iterator i;
      for (i = cast->map_elements.begin(); i != cast->map_elements.end(); i++) {
        st_string key = st_string(i->first);
        st_object *value;
        try {
          value = st_ast_eval(i->second, stack, env);
        } catch (exception &e) {
          st_object_discard(p);
          throw;
        }
        prs_add_element_to_map(p, &key, value);
        st_object_discard(value);
      }
      return (p);
    }
    if (to<st_ast_construct_list *>(ast)) {
      st_ast_construct_list *cast = to<st_ast_construct_list *>(ast);
      st_list *p = to<st_list *>((new st_list()));
      list<st_ast *>::iterator i;
      for (i = cast->list_elements.begin(); i != cast->list_elements.end();
           i++) {
        st_object *key;
        try {
          key = st_ast_eval(*i, stack, env);
        } catch (exception &e) {
          st_object_discard(p);
          throw;
        }
        prs_insert_in_list(p, key);
        st_object_discard(key);
      }
      return (p);
    }
    if (to<st_ast_expression *>(ast)) {
      st_ast_expression *cast = to<st_ast_expression *>(ast);
      st_object *lo;
      try {
        lo = st_ast_eval(cast->left, stack, env);
      } catch (exception &e) {
        throw;
      }
      st_object *ro;
      try {
        ro = st_ast_eval(cast->right, stack, env);
      } catch (exception &e) {
        st_object_discard(lo);
        throw;
      }

      bool l_is_integer = is_a<st_integer *>(lo);
      bool r_is_integer = is_a<st_integer *>(ro);

      bool l_is_double = is_a<st_double *>(lo);
      bool r_is_double = is_a<st_double *>(ro);

      bool l_is_string = is_a<st_string *>(lo);
      bool r_is_string = is_a<st_string *>(ro);

      bool l_is_vector = is_a<st_vector *>(lo);
      bool l_is_point = is_a<st_point *>(lo);

      bool l_is_list = is_a<st_list *>(lo);
      bool r_is_list = is_a<st_list *>(ro);

      bool l_is_map = is_a<st_map *>(lo);

      if (l_is_list && r_is_list && cast->operator_name == "+") {
        list<st_object *>::iterator i;

        st_list *ll = to<st_list *>(lo);
        st_list *rl = to<st_list *>(ro);

        st_list *res = new st_list;
        for (i = ll->begin(); i != ll->end(); i++) {
          res->insert(**i);
        }
        for (i = rl->begin(); i != rl->end(); i++) {
          res->insert(**i);
        }
        st_object_discard(lo);
        st_object_discard(ro);
        return (res);
      }

      if (cast->operator_name == "@") {
        if ((l_is_vector && r_is_integer) || (l_is_point && r_is_integer) ||
            (l_is_map && r_is_string)) {
          if (l_is_vector) {
            int index = to<st_integer *>(ro)->get_integer();
            if ((index >= to<st_vector *>(lo)->size()) || (index < 0)) {
              last_known_ast = ast;
              st_object_discard(lo);
              st_object_discard(ro);
              throw std::logic_error("Index is outside bounds");
            } else {
              st_object const *x;
              try {
                x = &to<st_vector *>(lo)->get(index);
              } catch (exception &e) {
                st_object_discard(lo);
                st_object_discard(ro);
                throw std::logic_error(
                    "Index is within bounds but generated an exception");
              }
              st_object *ret = x->gen_copy();
              st_object_discard(lo);
              st_object_discard(ro);
              return (ret);
            }
          } else {
            if (l_is_map) {
              string key = to<st_string *>(ro)->get_string();
              st_map *tmap = to<st_map *>(lo);
              st_object const *obj;
              if (tmap->get(key, obj)) {
                st_object *ret = obj->gen_copy();
                st_object_discard(lo);
                st_object_discard(ro);
                return (ret);
              } else {
                last_known_ast = ast;
                st_object_discard(lo);
                st_object_discard(ro);
                throw std::logic_error("Key " + key + "does not exist");
              }
            }
            if (l_is_point) {
              int index = to<st_integer *>(ro)->get_integer();
              st_point *p = to<st_point *>(lo);
              if (index >= p->size()) {
                last_known_ast = ast;
                st_object_discard(lo);
                st_object_discard(ro);
                throw std::logic_error("Index is outside bounds");
              } else {
                int ic = (*p)[index];
                st_object_discard(lo);
                st_object_discard(ro);
                return (new st_integer(ic));
              }
            }
          }
        }
        throw std::logic_error("@ operator used within the wrong context !" +
                               lo->print() + ", " + ro->print() + "!");
      }
      if ((r_is_integer || r_is_double) && cast->operator_name == "unary-") {
        if (r_is_double) {
          double result;
          result = (-(to<st_double *>(ro)->get_double()));
          st_object_discard(lo);
          st_object_discard(ro);
          return (new st_double(result));
        }
        if (r_is_integer) {
          int result;
          result = (-(to<st_integer *>(ro)->get_integer()));
          st_object_discard(lo);
          st_object_discard(ro);
          return (new st_integer(result));
        }
      }

      if ((!l_is_integer && !l_is_double && !l_is_string) ||
          (!r_is_integer && !r_is_double && !r_is_string)) {

        last_known_ast = ast;
#define DEBUG
#if defined(DEBUG)
        cout << "******************" << endl;
        cast->print();
        cout << endl;
        cout << "Evaluated with: " << endl;
        if (lo) {
          cast->left->print();
          cout << " ~= " << lo->print() << endl;
        } else {
          cast->left->print();
          cout << " ~="
               << "NULL" << endl;
        }

        if (ro) {
          cast->right->print();
          cout << " ~= " << ro->print() << endl;
        } else {
          cast->right->print();
          cout << " ~="
               << "NULL" << endl;
        }
        st_print_stack(stack);
#endif

        st_object_discard(lo);
        st_object_discard(ro);
        throw std::logic_error(
            "Arithmetic is valid only for integer, double or string values");
      }
      if (((l_is_double) && r_is_string) || ((r_is_double) && l_is_string)) {
        if (cast->operator_name == "==") {
          return (new st_integer(0));
        }
        if (cast->operator_name == "+") {
          ostringstream ssuffix;
          if (l_is_double)
            ssuffix << to<st_double *>(lo)->get_double()
                    << to<st_string *>(ro)->get_string();
          else
            ssuffix << to<st_string *>(lo)->get_string()
                    << to<st_double *>(ro)->get_double();
          string result = ssuffix.str();
          st_object_discard(lo);
          st_object_discard(ro);
          return (new st_string(result));
        }
        st_object_discard(lo);
        st_object_discard(ro);
        last_known_ast = ast;
        throw std::logic_error(
            "String arithmetic allows only a + between strings and doubles");
        return NULL;
      }
      if (((l_is_integer) && r_is_string) || ((r_is_integer) && l_is_string)) {
        if (cast->operator_name == "+") {
          ostringstream ssuffix;
          if (l_is_integer)
            ssuffix << to<st_integer *>(lo)->get_integer()
                    << to<st_string *>(ro)->get_string();
          else
            ssuffix << to<st_string *>(lo)->get_string()
                    << to<st_integer *>(ro)->get_integer();
          string result = ssuffix.str();
          st_object_discard(lo);
          st_object_discard(ro);
          return (new st_string(result));
        }
        st_object_discard(lo);
        st_object_discard(ro);
        last_known_ast = ast;
        throw std::logic_error("String operator not recognized");
      }
      if (r_is_string && l_is_string) {
        string lv = to<st_string *>(lo)->get_string();
        string rv = to<st_string *>(ro)->get_string();

        st_object_discard(lo);
        st_object_discard(ro);

        if (cast->operator_name == "+") {
          string result;
          result = lv + rv;
          return (new st_string(result));
        }
        if (cast->operator_name == "==") {
          int result;
          result = (lv.compare(rv) == 0);
          return (new st_integer(result));
        }
        last_known_ast = ast;
        throw std::logic_error("String operator not recognized");
      }
      if (l_is_double || r_is_double) {
        double lv = l_is_integer ? to<st_integer *>(lo)->get_integer()
                                 : to<st_double *>(lo)->get_double();

        double rv = r_is_integer ? to<st_integer *>(ro)->get_integer()
                                 : to<st_double *>(ro)->get_double();

        st_object_discard(lo);
        st_object_discard(ro);

        if (cast->operator_name == "+") {
          double result;
          result = lv + rv;
          return (new st_double(result));
        }
        if (cast->operator_name == "-") {
          double result;
          result = lv - rv;
          return (new st_double(result));
        }
        if (cast->operator_name == "*") {
          double result;
          result = lv * rv;
          return (new st_double(result));
        }
        if (cast->operator_name == "/") {
          double result;
          if (rv == 0.0) {
            return (new st_double(INFINITY));
          }
          result = lv / rv;
          return (new st_double(result));
        }
        if (cast->operator_name == "<") {
          int result;
          result = lv < rv;
          return (new st_integer(result));
        }
        if (cast->operator_name == ">") {
          int result;
          result = lv > rv;
          return (new st_integer(result));
        }
        if (cast->operator_name == "<=") {
          int result;
          result = lv <= rv;
          return (new st_integer(result));
        }
        if (cast->operator_name == ">=") {
          int result;
          result = lv >= rv;
          return (new st_integer(result));
        }
        if (cast->operator_name == "==") {
          int result;
          result = (lv == rv);
          return (new st_integer(result));
        }
        if (cast->operator_name == "!=") {
          int result;
          result = (lv != rv);
          return (new st_integer(result));
        }
        if (cast->operator_name == "&") {
          int result;
          result = ((int)lv) && ((int)rv);
          return (new st_integer(result));
        }
        if (cast->operator_name == "|") {
          int result;
          result = ((int)lv) || ((int)rv);
          return (new st_integer(result));
        }
        if (cast->operator_name == "!") {
          int result;
          result = (!((int)rv));
          return (new st_integer(result));
        }
      } else {
        int lv = to<st_integer *>(lo)->get_integer();
        int rv = to<st_integer *>(ro)->get_integer();
        st_object_discard(lo);
        st_object_discard(ro);
        if (cast->operator_name == "+") {
          int result;
          result = lv + rv;
          return (new st_integer(result));
        }
        if (cast->operator_name == "-") {
          int result;
          result = lv - rv;
          return (new st_integer(result));
        }
        if (cast->operator_name == "*") {
          int result;
          result = lv * rv;
          return (new st_integer(result));
        }
        if (cast->operator_name == "/") {
          double result;
          if (rv == 0.0) {
            return (new st_double(INFINITY));
          }
          result = ((double)lv) / ((double)rv);
          return (new st_double(result));
        }
        if (cast->operator_name == "<") {
          int result;
          result = lv < rv;
          return (new st_integer(result));
        }
        if (cast->operator_name == ">") {
          int result;
          result = lv > rv;
          return (new st_integer(result));
        }
        if (cast->operator_name == "<=") {
          int result;
          result = lv <= rv;
          return (new st_integer(result));
        }
        if (cast->operator_name == ">=") {
          int result;
          result = lv >= rv;
          return (new st_integer(result));
        }
        if (cast->operator_name == "==") {
          int result;
          result = (lv == rv);
          return (new st_integer(result));
        }
        if (cast->operator_name == "!=") {
          int result;
          result = (lv != rv);
          return (new st_integer(result));
        }
        if (cast->operator_name == "&") {
          int result;
          result = ((int)lv) && ((int)rv);
          return (new st_integer(result));
        }
        if (cast->operator_name == "|") {
          int result;
          result = ((int)lv) || ((int)rv);
          return (new st_integer(result));
        }
        if (cast->operator_name == "!") {
          int result;
          result = (!((int)rv));
          return (new st_integer(result));
        }
      }
      last_known_ast = ast;
      throw std::logic_error("Illegal arithmetic expression");
    }
    if (to<st_ast_functional *>(ast)) {
      return st_compute_functional_value(to<st_ast_functional *>(ast), stack,
                                         env);
    }
    if (to<st_ast_anon_function_call *>(ast)) {

      // st_print_stack(stack);
      st_ast_anon_function_call *cast = to<st_ast_anon_function_call *>(ast);
      st_object *o;

      /* Here we had a problem.
       *
       * If the function name is contained in a variable in the same scope as
       * the function call or a global variable, then OK. Otherwise, if the
       * variable is not in the scope of the function call (e.g., when you use
       * it within another anonymous function) then we should traverse the stack
       * looking for the actual value of that variable since we dont implement
       * closures.
       *
       * This is currently done when evaluating a variable in a function call.
       * It traverses backward the stack. The only catch is that if you refer to
       * a variable in an anonymous function that is synonym with one next on
       * the stack, then the last one is used. To solve this problem, name
       * non-ambigously the variables used in anonymous functions.
       */

      try {
        o = st_ast_eval(cast->function_name, stack, env);
      }

      catch (exception e) {
        last_known_ast = ast;
        //               cout << "problems evaluating" << endl;
        //              cast->function_name->print();
        throw std::logic_error("Anonymous function cant be determined.");
      }

      string name;
      if (is_anon_deref(o, name)) {
        /* def_level should be propagated to the activation stack of this
         * function call */
        int stack_level;
        st_ast_function_call *fast = new st_ast_function_call();

        fast->operand = cast->operand->copy();
        fast->function_name = name;

        fast->line = cast->line;
        fast->decl_file_name = cast->decl_file_name;

        if (get_definition_stack_level(o, stack_level)) {
          fast->stack_level = stack_level;
        } else {
          fast->stack_level = -1;
        }

        st_object *obj;
        try {
          obj = st_ast_eval(fast, stack, env);
          //    st_print_stack(stack);
          //    cout << "^^^^^^^^^^^^^^^^^ after invoking anonymous function" <<
          //    endl;
        } catch (exception &e) {
          st_object_discard(o);
          delete fast;
          throw;
        }
        st_object_discard(o);
        delete fast;
        // st_print_stack(stack);
        // cout << "^^^^^^^^^^^^^^^^^ after deleting temporary expression" <<
        // endl;
        return obj;
      }
      st_object_discard(o);
      throw std::logic_error("Anonymous function cant be evaluated.");
    }
    if (to<st_ast_function_call *>(ast)) {
      // cout << "Entered function call AST: " << ast << endl;
      st_ast_function_call *cast = to<st_ast_function_call *>(ast);
      // cout << "Entered function call CAST: " << ast << endl;
      // cast->print();
      if (env->current_design_space) {
        if (env->current_design_space->ds_parameters_index.count(
                (cast->function_name))) {
          int parindex;
          st_object *obj = st_ast_eval(cast->operand, stack, env);
          parindex = env->current_design_space
                         ->ds_parameters_index[cast->function_name];
          st_point *p = st_ast_get_point_n_discard(obj);
          if (!p) {
            last_known_ast = ast;
            throw std::logic_error("Expecting a point as operand");
          }
          int value = (*p)[parindex];
          st_object_discard(p);
          return (new st_integer(value));
        }

        if (cast->function_name == "path") {
          st_object *obj = st_ast_eval(cast->operand, stack, env);
          st_point *p = st_ast_get_point_n_discard(obj);
          if (!p) {
            last_known_ast = ast;
            throw std::logic_error("Expecting a point as operand");
          }
          string value = p->get_rpath();
          st_object_discard(p);
          return (new st_string(value));
        }

        if (cast->function_name == "get_cluster") {
          st_object *obj = st_ast_eval(cast->operand, stack, env);
          st_point *p = st_ast_get_point_n_discard(obj);
          if (!p) {
            last_known_ast = ast;
            throw std::logic_error("Expecting a point as operand");
          }
          int value = p->get_cluster();
          st_object_discard(p);
          return (new st_integer(value));
        }

        if (env->current_design_space->metric_index.count(
                cast->function_name)) {

          int metric_index =
              env->current_design_space->metric_index[cast->function_name];
          st_object *obj = st_ast_eval(cast->operand, stack, env);
          st_point *p = st_ast_get_point_n_discard(obj);
          if (!p) {
            last_known_ast = ast;
            throw std::logic_error("Expecting a point as operand");
          }
          double value = p->get_metrics(metric_index);
          st_object_discard(p);
          return new st_double(value);
        }

        for (int i = 0; i < env->optimization_objectives.size(); i++) {
          if (env->optimization_objectives[i]->name == cast->function_name) {
            st_object *obj = st_ast_eval(cast->operand, stack, env);
            st_point *p = st_ast_get_point_n_discard(obj);
            if (!p) {
              last_known_ast = ast;
              throw std::logic_error("Expecting a point as operand");
            }
            double e_m_value;
            try {
              e_m_value = env->optimization_objectives[i]->eval(p, i);
            } catch (exception e) {
              st_object_discard(p);
              last_known_ast = ast;
              throw std::logic_error("Cant compute objective.");
            }
            st_object_discard(p);
            return new st_double(e_m_value);
          }
        }
        for (int i = 0; i < env->companion_metrics.size(); i++) {
          if (env->companion_metrics[i]->name == cast->function_name) {
            st_object *obj = st_ast_eval(cast->operand, stack, env);
            st_point *p = st_ast_get_point_n_discard(obj);
            if (!p) {
              last_known_ast = ast;
              throw std::logic_error("Expecting a point as operand");
            }
            st_object *e_m_value;
            try {
              e_m_value = env->companion_metrics[i]->eval(p, i);
            } catch (exception e) {
              st_object_discard(p);
              last_known_ast = ast;
              throw std::logic_error("Cant compute metric.");
            }
            st_object_discard(p);
            return (e_m_value);
          }
        }
      }

      if (st_user_defined_functions.count(cast->function_name)) {
        // cout << "Invoking " << cast->function_name << endl;
        // cout << "Operand address " << cast->operand << endl;
        // cout << "Operand expression "; cast->operand->print(); cout << endl;
        st_object *opval = st_ast_eval(cast->operand, stack, env);
        st_object *obj;
        try {
          obj = st_call_user_defined_function(cast->function_name, stack, opval,
                                              env, cast->stack_level);
        } catch (exception &e) {
          st_object_discard(opval);
          throw;
        }
        st_object_discard(opval);
        return obj;
      }
      if (!st_ast_functions.count(cast->function_name)) {
        last_known_ast = ast;
        throw std::logic_error("Function " + cast->function_name +
                               " not existing");
      }
      st_object *obj = st_ast_eval(cast->operand, stack, env);
      st_object *res;
      try {
        res = st_ast_functions[cast->function_name](obj, env);
      } catch (exception &e) {
        last_known_ast = ast;
        st_object_discard(obj);
        throw;
      }
      // cout << "We are out of " << cast->function_name << endl;
      st_object_discard(obj);
      return res;
    }
    if (to<st_ast_full_object *>(ast)) {
      st_ast_full_object *cast = to<st_ast_full_object *>(ast);
      st_object *fo = st_ast_eval(cast->full_object_description, stack, env);
      st_object *mo;
      try {
        mo = st_ast_eval(cast->properties, stack, env);
      } catch (exception &e) {
        st_object_discard(fo);
        throw;
      }
      prs_insert_map_as_property(fo, mo);
      st_object_discard(mo);
      return fo;
    }
    if (to<st_ast_compound_commands *>(ast)) {
      /* Simple command list evaluated on the command line */
      st_ast_compound_commands *cast = to<st_ast_compound_commands *>(ast);
      list<st_ast *>::iterator i;
      for (i = cast->compound_commands.begin();
           i != cast->compound_commands.end(); i++) {
        st_object *com = st_ast_eval(*i, stack, env);
        st_object_discard(com);
      }
      return NULL;
    }
    /* identifiers can be used within a set command to identify the variable or
     * in a list where are converted to strings.. */
    if (to<st_ast_identifier *>(ast)) {
      st_ast_identifier *cast = to<st_ast_identifier *>(ast);
      return (new st_string(cast->id_name));
    }
  }
  return NULL;
}
