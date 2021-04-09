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
 */

#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <string>

#ifndef ARG_HH
#define ARG_HH

using namespace std;

typedef map<string, string> options;
typedef set<string> parameters;

typedef enum {
  OPTION_SIMPLE,
  OPTION_MULTIPLE_ARG,
} arg_parser_item_type;

typedef struct arg_parser_item {
  arg_parser_item_type type;
  string option_name;
  string short_option_name;
  string argument_name;
  set<string> values;
  string comment;
} arg_parser_item;

typedef map<string, arg_parser_item> option_list;
typedef list<string> parameter_list;

typedef struct arg_parser {
  string prog_name;
  string parameter_name;
  option_list options;
  parameter_list parameters;
  string bug_report;

  string look_for_option_name(string short_option);

  void insert(const char *option_name, const char short_option,
              const char *argument, const char *comment,
              arg_parser_item_type type);

  bool option_is_set(string option_name);

  string get_option_value(string option_name);

  bool process_options(int argc, char **argv);

  void print_help();

  void print_short_help();

} arg_parser;

#endif
