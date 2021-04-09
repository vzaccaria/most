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
/* Abstract syntax tree definition for the grammar */
#ifndef ST_AST
#define ST_AST
#include <iostream>
#include <list>
#include <map>
#include "st_object.h"

struct st_ast;

struct st_ast {
  int line;
  const char *decl_file_name;
  st_ast() {
    line = 0;
    decl_file_name = NULL;
  };
  virtual void print() = 0;
  virtual ~st_ast(){};
  virtual st_ast *copy() = 0;
};

template <class T> inline T to(st_ast *p) { return dynamic_cast<T>(p); }

void st_print_ast_list(list<st_ast *> &list, bool commas);
void st_copy_ast_list(list<st_ast *> &alist, list<st_ast *> &dest);
void st_copy_ast_map(map<string, st_ast *> &m1, map<string, st_ast *> &m2);
void st_delete_ast_list(list<st_ast *> &list);

typedef map<string, st_object *> st_ast_stack_frame;
typedef vector<st_ast_stack_frame> st_ast_expression_stack;

struct st_env;
st_object *st_ast_eval(st_ast *, st_ast_expression_stack *, st_env *);

//#define DEBUG
#if defined(DEBUG)
#define DP(x)                                                                  \
  printf("[%s @ %x, file %s, line %d]", #x, this,                              \
         (this->decl_file_name != NULL) ? this->decl_file_name : "INTER",      \
         this->line);
#else
#define DP(x)
#endif

struct st_ast_leaf : public st_ast {
  st_object *leaf_value;
  void print() {
    DP(leaf);
    cout << leaf_value->print();
  }
  ~st_ast_leaf() { delete leaf_value; };
  st_ast *copy() {
    st_ast_leaf *s = new st_ast_leaf();
    s->leaf_value = leaf_value->gen_copy();
    s->decl_file_name = decl_file_name;
    s->line = line;
    return s;
  }
};

struct st_ast_leaf_R : public st_ast {
  string leaf_value;
  void print() {
    DP(leaf);
    cout << leaf_value;
  }
  ~st_ast_leaf_R(){};
  st_ast *copy() {
    st_ast_leaf_R *s = new st_ast_leaf_R();
    s->decl_file_name = decl_file_name;
    s->line = line;
    return s;
  }
};

struct st_ast_variable : public st_ast {
  string variable_name;
  void print() {
    DP(variable);
    cout << "$" << variable_name;
  }
  st_ast *copy() {
    st_ast_variable *s = new st_ast_variable();
    s->variable_name = variable_name;
    s->decl_file_name = decl_file_name;
    s->line = line;
    return s;
  }
};

struct st_ast_simple_command : public st_ast {
  string command_name;
  map<string, st_ast *> command_parameters;
  void print() {
    DP(command);
    cout << command_name << " ";
    map<string, st_ast *>::iterator i = command_parameters.begin();
    while (i != command_parameters.end()) {
      cout << "--" << i->first << "=";
      i->second->print();
      cout << " ";
      i++;
    }
  }
  st_ast *copy() {
    st_ast_simple_command *s = new st_ast_simple_command();
    s->decl_file_name = decl_file_name;
    s->line = line;
    s->command_name = command_name;
    map<string, st_ast *>::iterator i = command_parameters.begin();
    while (i != command_parameters.end()) {
      pair<string, st_ast *> p(i->first, i->second->copy());
      s->command_parameters.insert(p);
      i++;
    }
    return s;
  }
  ~st_ast_simple_command() {
    map<string, st_ast *>::iterator i = command_parameters.begin();
    while (i != command_parameters.end()) {
      delete i->second;
      i++;
    }
  }
};

struct st_ast_identifier : public st_ast {
  string id_name;
  void print() {
    DP(indentifier);
    cout << id_name;
  }
  st_ast *copy() {
    st_ast_identifier *s = new st_ast_identifier();
    s->id_name = id_name;
    s->decl_file_name = decl_file_name;
    s->line = line;
    return s;
  }
};

struct st_ast_set : public st_ast {
  string variable_name;
  st_ast *expression;
  bool local;
  bool at;
  st_ast *index;
  st_ast_set() {
    local = false;
    at = false;
    index = NULL;
  }
  void print() {
    DP(set);
    cout << "set " << variable_name << " = ";
    expression->print();
    cout << endl;
  }
  st_ast *copy() {
    st_ast_set *s = new st_ast_set();
    s->variable_name = variable_name;
    s->expression = expression->copy();
    s->local = local;
    s->decl_file_name = decl_file_name;
    s->line = line;
    s->at = at;
    s->index = index;
    return s;
  }
  ~st_ast_set() { delete expression; }
};

struct st_ast_echo : public st_ast {
  st_ast *expression;
  string mode;
  string file_name;
  void print() {
    DP(echo);
    cout << "echo ";
    expression->print();
    cout << endl;
  }
  st_ast *copy() {
    st_ast_echo *s = new st_ast_echo();
    s->mode = mode;
    s->expression = expression->copy();
    s->file_name = file_name;
    s->decl_file_name = decl_file_name;
    s->line = line;
    return s;
  }
  ~st_ast_echo() { delete expression; }
};

struct st_ast_compound_commands : public st_ast {
  list<st_ast *> compound_commands;
  void print() {
    DP(compound_commands);
    st_print_ast_list(compound_commands, false);
  }
  st_ast *copy() {
    st_ast_compound_commands *s = new st_ast_compound_commands();
    s->decl_file_name = decl_file_name;
    s->line = line;
    st_copy_ast_list(compound_commands, s->compound_commands);
    return s;
  }
  ~st_ast_compound_commands() { st_delete_ast_list(compound_commands); }
};

struct st_ast_command_while : public st_ast {
  st_ast *evaluation_expression;
  list<st_ast *> compound_commands;
  void print() {
    DP(while);
    cout << "while ";
    evaluation_expression->print();
    cout << " { " << endl;
    st_print_ast_list(compound_commands, false);
    cout << "} " << endl;
  }
  st_ast *copy() {
    st_ast_command_while *s = new st_ast_command_while();
    s->decl_file_name = decl_file_name;
    s->line = line;
    s->evaluation_expression = evaluation_expression->copy();
    st_copy_ast_list(compound_commands, s->compound_commands);
    return s;
  }
  ~st_ast_command_while() {
    delete evaluation_expression;
    st_delete_ast_list(compound_commands);
  }
};

struct st_ast_command_for : public st_ast {
  string iterator_variable;
  st_ast *list_of_values;
  list<st_ast *> compound_commands;
  void print() {
    DP(foreach);
    cout << "foreach " << iterator_variable << " in ";
    list_of_values->print();
    cout << endl;
    st_print_ast_list(compound_commands, false);
    cout << "done " << endl;
  }
  st_ast *copy() {
    st_ast_command_for *s = new st_ast_command_for();
    s->decl_file_name = decl_file_name;
    s->line = line;
    s->iterator_variable = iterator_variable;
    s->list_of_values = list_of_values->copy();
    st_copy_ast_list(compound_commands, s->compound_commands);
    return s;
  }
  ~st_ast_command_for() {
    delete list_of_values;
    st_delete_ast_list(compound_commands);
  }
};

struct st_ast_command_if : public st_ast {
  st_ast *evaluation_expression;
  list<st_ast *> then_commands;
  list<st_ast *> else_commands;
  void print() {
    DP(if);
    cout << "if ";
    evaluation_expression->print();
    cout << " { " << endl;
    st_print_ast_list(then_commands, false);
    cout << "} " << endl;
    cout << "else {";
    st_print_ast_list(else_commands, false);
    cout << "} " << endl;
  }
  st_ast *copy() {
    st_ast_command_if *s = new st_ast_command_if();
    s->decl_file_name = decl_file_name;
    s->line = line;
    s->evaluation_expression = evaluation_expression->copy();
    st_copy_ast_list(then_commands, s->then_commands);
    st_copy_ast_list(else_commands, s->else_commands);
    return s;
  }
  ~st_ast_command_if() {
    delete evaluation_expression;
    st_delete_ast_list(then_commands);
    st_delete_ast_list(else_commands);
  }
};

struct st_ast_construct_point : public st_ast {
  list<st_ast *> point_coordinates;
  void print() {
    DP(point);
    cout << " % ";
    st_print_ast_list(point_coordinates, false);
    cout << " % ";
  }
  st_ast *copy() {
    st_ast_construct_point *s = new st_ast_construct_point();
    s->decl_file_name = decl_file_name;
    s->line = line;
    st_copy_ast_list(point_coordinates, s->point_coordinates);
    return s;
  }
  ~st_ast_construct_point() { st_delete_ast_list(point_coordinates); }
};

struct st_ast_construct_vector : public st_ast {
  list<st_ast *> vector_coordinates;
  void print() {
    DP(vector);
    cout << " [ ";
    st_print_ast_list(vector_coordinates, true);
    cout << " ] ";
  }
  st_ast *copy() {
    st_ast_construct_vector *s = new st_ast_construct_vector();
    s->decl_file_name = decl_file_name;
    s->line = line;
    st_copy_ast_list(vector_coordinates, s->vector_coordinates);
    return s;
  }
  ~st_ast_construct_vector() { st_delete_ast_list(vector_coordinates); }
};

struct st_ast_construct_map : public st_ast {
  map<string, st_ast *> map_elements;
  void print() {
    DP(map);
    cout << " ( ";
    map<string, st_ast *>::iterator i = map_elements.begin();
    while (i != map_elements.end()) {
      cout << i->first;
      cout << "=";
      i->second->print();
      cout << " ";
      i++;
    }
    cout << " ) ";
  }
  st_ast *copy() {
    st_ast_construct_map *s = new st_ast_construct_map();
    s->decl_file_name = decl_file_name;
    s->line = line;
    map<string, st_ast *>::iterator i = map_elements.begin();
    while (i != map_elements.end()) {
      pair<string, st_ast *> p(i->first, i->second->copy());
      s->map_elements.insert(p);
      i++;
    }
    return s;
  }
  ~st_ast_construct_map() {
    map<string, st_ast *>::iterator i = map_elements.begin();
    while (i != map_elements.end()) {
      delete i->second;
      i++;
    }
  }
};

struct st_ast_construct_list : public st_ast {
  list<st_ast *> list_elements;
  void print() {
    DP(list);
    cout << " { ";
    st_print_ast_list(list_elements, true);
    cout << " } ";
  }
  st_ast *copy() {
    st_ast_construct_list *s = new st_ast_construct_list();
    st_copy_ast_list(list_elements, s->list_elements);
    s->decl_file_name = decl_file_name;
    s->line = line;
    return s;
  }
  void free_container() { list_elements.clear(); }
  ~st_ast_construct_list() { st_delete_ast_list(list_elements); }
};

struct st_ast_functional : public st_ast {
  string functional_name;
  string iterator;
  st_ast *min;
  st_ast *max;
  st_ast *expression;
  void print() {
    DP(functional);
    cout << functional_name << "(" << iterator << ",";
    min->print();
    cout << ",";
    max->print();
    cout << ",";
    expression->print();
    cout << ")";
  }
  st_ast *copy() {
    st_ast_functional *s = new st_ast_functional();
    s->decl_file_name = decl_file_name;
    s->line = line;
    s->functional_name = functional_name;
    s->iterator = iterator;
    s->min = min->copy();
    s->max = max->copy();
    s->expression = expression->copy();
    return s;
  }
  ~st_ast_functional() {
    delete min;
    delete max;
    delete expression;
  }
};

struct st_ast_expression : public st_ast {
  string operator_name;
  st_ast *left;
  st_ast *right;
  void print() {
    DP(expression);
#if defined(DEBUG)
    cout << "[" << operator_name << "]" << endl;
#endif
    if (left) {
      if (to<st_ast_expression *>(left)) {
        bool printed = false;
        string down_op = to<st_ast_expression *>(left)->operator_name;
        if ((operator_name == "+" || operator_name == "-" ||
             operator_name == "|") &&
            (down_op == "<" || down_op == ">" || down_op == "==" ||
             down_op == "!=" || down_op == "<=" || down_op == ">=")) {
          printed = true;
          cout << "(";
          left->print();
          cout << ")";
        }

        if ((operator_name == "*" || operator_name == "/" ||
             operator_name == "&") &&
            (down_op == "+" || down_op == "-" || down_op == "|" ||
             down_op == "<" || down_op == ">" || down_op == "==" ||
             down_op == "!=" || down_op == "<=" || down_op == ">=")) {
          printed = true;
          cout << "(";
          left->print();
          cout << ")";
        }
        if (!printed)
          left->print();
      } else {
        left->print();
      }
    }
    if (operator_name != "unary-")
      cout << operator_name;
    else
      cout << "-";

    if (right) {
      if (to<st_ast_expression *>(right)) {
        bool printed = false;
        string down_op = to<st_ast_expression *>(right)->operator_name;

        if (operator_name == "@") {
          printed = true;
          cout << "(";
          right->print();
          cout << ")";
        }

        if (operator_name == "!") {
          printed = true;
          cout << "(";
          right->print();
          cout << ")";
        }

        if (operator_name == "/" && (down_op == "/" || down_op == "*")) {
          printed = true;
          cout << "(";
          right->print();
          cout << ")";
        }

        if ((operator_name == "-" || operator_name == "unary-") &&
            (down_op == "-" || down_op == "+")) {
          printed = true;
          cout << "(";
          right->print();
          cout << ")";
        }

        if ((operator_name == "+" || operator_name == "-" ||
             operator_name == "|") &&
            (down_op == "<" || down_op == ">" || down_op == "==" ||
             down_op == "!=" || down_op == "<=" || down_op == ">=")) {
          printed = true;
          cout << "(";
          right->print();
          cout << ")";
        }

        if ((operator_name == "*" || operator_name == "/" ||
             operator_name == "&") &&
            (down_op == "+" || down_op == "-" || down_op == "|" ||
             down_op == "<" || down_op == ">" || down_op == "==" ||
             down_op == "!=" || down_op == "<=" || down_op == ">=")) {
          printed = true;
          cout << "(";
          right->print();
          cout << ")";
        }
        if (!printed)
          right->print();
      } else {
        right->print();
      }
    }
  }
  st_ast *copy() {
    st_ast_expression *s = new st_ast_expression();
    s->decl_file_name = decl_file_name;
    s->line = line;

    if (left)
      s->left = left->copy();
    else
      s->left = NULL;

    if (right)
      s->right = right->copy();
    else
      s->right = NULL;

    s->operator_name = operator_name;
    return s;
  }
  ~st_ast_expression() {
    if (left)
      delete left;
    if (right)
      delete right;
  }
};

struct st_ast_anon_function_call : public st_ast {
  st_ast *function_name;
  st_ast *operand;
  void print() {
    DP(function_anon_call);
    cout << function_name;
    st_assert(to<st_ast_construct_vector *>(operand));
    cout << "(";
    st_print_ast_list(
        to<st_ast_construct_vector *>(operand)->vector_coordinates, true);
    cout << ")";
  }
  st_ast *copy() {
    st_ast_anon_function_call *s = new st_ast_anon_function_call();
    s->decl_file_name = decl_file_name;
    s->line = line;
    s->function_name = function_name->copy();
    s->operand = operand->copy();
    return s;
  }
  ~st_ast_anon_function_call() {
    // cout << "ANON SD" << endl;
    // cout << "DELETING " << function_name << endl;
    // cout << "DELETING " << operand << endl;
    delete function_name;
    delete operand;
  }
};

struct st_ast_function_call : public st_ast {
  string function_name;
  st_ast *operand;
  /* WARNING: these are not copied (explicitly) */
  int stack_level;
  void print() {
    DP(function_call);
    cout << function_name;
    st_assert(to<st_ast_construct_vector *>(operand));
    cout << "(";
    st_print_ast_list(
        to<st_ast_construct_vector *>(operand)->vector_coordinates, true);
    cout << ")";
  }
  st_ast *copy() {
    st_ast_function_call *s = new st_ast_function_call();
    s->decl_file_name = decl_file_name;
    s->line = line;
    s->function_name = function_name;
    s->operand = operand->copy();
    s->stack_level = -1;
    return s;
  }
  ~st_ast_function_call() { delete operand; }
};

struct st_ast_define_objective : public st_ast {
  st_ast *objective_expression;
  string point_name;
  string name;
  void print() {
    DP(set_objective);
    cout << "set_objective " << name << " (" << point_name << ") = ";
    objective_expression->print();
  }
  st_ast *copy() {
    st_ast_define_objective *s = new st_ast_define_objective();
    s->decl_file_name = decl_file_name;
    s->line = line;
    s->name = name;
    s->objective_expression = objective_expression->copy();
    s->point_name = point_name;
    return s;
  }
  ~st_ast_define_objective() { delete objective_expression; }
};

struct st_ast_define_metric : public st_ast {
  st_ast *metric_expression;
  string point_name;
  string name;
  void print() {
    DP(set_metric);
    cout << "set_metric " << name << " (" << point_name << ") = ";
    metric_expression->print();
  }
  st_ast *copy() {
    st_ast_define_metric *s = new st_ast_define_metric();
    s->decl_file_name = decl_file_name;
    s->line = line;
    s->name = name;
    s->metric_expression = metric_expression->copy();
    s->point_name = point_name;
    return s;
  }
  ~st_ast_define_metric() { delete metric_expression; }
};

struct st_ast_define_constraint : public st_ast {
  st_ast *constraint_expression;
  string name;
  string point_name;
  void print() {
    DP(set_constraint);
    cout << "set_constraint " << name << " (" << point_name << ") = ";
    constraint_expression->print();
  }
  st_ast *copy() {
    st_ast_define_constraint *s = new st_ast_define_constraint();
    s->decl_file_name = decl_file_name;
    s->line = line;
    s->name = name;
    s->constraint_expression = constraint_expression->copy();
    s->point_name = point_name;
    return s;
  }
  ~st_ast_define_constraint() { delete constraint_expression; }
};

struct st_ast_define_anon_function : public st_ast {
  st_ast *function_expression;
  st_ast *local_parameters;
  bool existing;
  string existing_name;
  st_ast_define_anon_function() {
    function_expression = NULL;
    local_parameters = NULL;
    existing = true;
    existing_name = "nil";
  }
  void print() {
    DP(set_anon_function);
    cout << "^ (";
    // st_print_ast_list(to<st_ast_construct_vector
    // *>(local_parameters)->vector_coordinates, true);
    cout << ") = ";
    cout << existing_name;
  }
  st_ast *copy() {
    st_ast_define_anon_function *s = new st_ast_define_anon_function();
    s->decl_file_name = decl_file_name;
    s->line = line;
    s->function_expression =
        (function_expression != NULL) ? function_expression->copy() : NULL;
    s->local_parameters =
        (local_parameters != NULL) ? (local_parameters->copy()) : NULL;
    s->existing = existing;
    s->existing_name = existing_name;
    return s;
  }
  ~st_ast_define_anon_function() {
    if (function_expression)
      delete function_expression;

    if (local_parameters)
      delete local_parameters;
  }
};

extern string st_ast_get_afun_name();

struct st_ast_define_function : public st_ast {
  st_ast *function_expression;
  st_ast *local_parameters;
  string name;
  void print() {
    DP(set_function);
    cout << "set_function (" << name;
    st_print_ast_list(
        to<st_ast_construct_vector *>(local_parameters)->vector_coordinates,
        true);
    cout << ") = ";
    function_expression->print();
  }
  st_ast *copy() {
    st_ast_define_function *s = new st_ast_define_function();
    s->decl_file_name = decl_file_name;
    s->line = line;
    s->name = name;
    s->function_expression = function_expression->copy();
    s->local_parameters = (local_parameters->copy());
    return s;
  }
  ~st_ast_define_function() {
    delete function_expression;
    delete local_parameters;
  }
};

struct st_ast_define_procedure : public st_ast {
  st_ast *local_parameters;
  string name;
  st_ast *compound_commands;
  void print() {
    DP(set_procedure);
    cout << "set_procedure (" << name;
    st_print_ast_list(
        to<st_ast_construct_vector *>(local_parameters)->vector_coordinates,
        true);
    cout << ") = ";
    compound_commands->print();
  }
  st_ast *copy() {
    st_ast_define_procedure *s = new st_ast_define_procedure();
    s->decl_file_name = decl_file_name;
    s->line = line;
    s->name = name;
    s->local_parameters = (local_parameters->copy());
    s->compound_commands = compound_commands->copy();
    return s;
  }
  ~st_ast_define_procedure() {
    delete compound_commands;
    delete local_parameters;
  }
};

struct st_ast_full_object : public st_ast {
  st_ast *full_object_description;
  st_ast *properties;
  void print() {
    DP(full_object);
    cout << "_obj_repr_#";
    full_object_description->print();
    cout << "#";
    properties->print();
  }
  st_ast *copy() {
    st_ast_full_object *s = new st_ast_full_object();
    s->decl_file_name = decl_file_name;
    s->line = line;
    s->full_object_description = full_object_description->copy();
    s->properties = properties->copy();
    return s;
  }
  ~st_ast_full_object() {
    delete full_object_description;
    delete properties;
  }
};

struct st_user_def_function {
  vector<string> local_parameters;
  st_ast *function_expression;
  st_user_def_function() { function_expression = NULL; }
};

struct st_ast_return_exception {
  st_object *value;
  st_ast_return_exception(const st_ast_return_exception &x) { value = x.value; }
  st_ast_return_exception() { value = NULL; }
};

extern map<string, st_user_def_function> st_user_defined_functions;

extern st_object *st_invoke_recursive_evaluation_1(
    st_ast *expression, st_ast_expression_stack *stack, string parameter_name,
    st_object *value, st_env *);
void st_ast_initialize_ast();
void st_ast_free_ast();
#endif
