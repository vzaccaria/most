/* @STSHELL_LICENSE_START@
 *
 * Authors: Vittorio Zaccaria, Gianluca Palermo
 * Copyright (c) 2007 Politecnico di Milano
 *  
 * This file is confidential property of Politecnico di Milano. 
 * Distribution outside Politecnico di Milano is currently not allowed.
 *
 * @STSHELL_LICENSE_END@ */
/*
 */

#include <string>
#include <map>
#include <set>
#include <list>
#include <iostream>
#include <fstream>

#ifndef ARG_HH
#define ARG_HH

using namespace std;

typedef map<string, string> options;
typedef set<string> parameters;

typedef enum 
{
  OPTION_SIMPLE,
  OPTION_MULTIPLE_ARG,
} arg_parser_item_type;

typedef struct arg_parser_item
{
  arg_parser_item_type  type;
  string                    option_name;
  string                    short_option_name;
  string                    argument_name;
  set<string>               values;   
  string                    comment;
} arg_parser_item;

typedef map<string, arg_parser_item> option_list;
typedef list<string> parameter_list;

typedef struct arg_parser
{
  string          	prog_name;
  string          	parameter_name;
  option_list 		options;
  parameter_list 	parameters;
  string          	bug_report;

  string look_for_option_name(string short_option);

  void insert(const char *option_name, const char short_option, const char *argument, const char *comment, arg_parser_item_type type);

  bool option_is_set(string option_name);

  string get_option_value(string option_name);

  bool process_options(int argc, char **argv);

  void print_help();

  void print_short_help();

} arg_parser; 

/** 
 * The following is the main interface to this sdk. Previous declarations are meant
 * to be made opaque in the future. They also need C++ linkage which is not predictably portable
 * across different versions of C++. */

extern "C"
{
void xml_m3_declare_parameter(arg_parser *, string name);
bool xml_m3_process_options(arg_parser *, int argc, char **argv);
int  xml_m3_get_integer(arg_parser *, string name);
string  xml_m3_get_string(arg_parser *, string name);

/** Missing masks and permutations. TBD */

bool xml_m3_is_parameter_valid(arg_parser *, string name);

void xml_m3_emit_metric(string name, double value);
void xml_m3_emit_error(string error);
}

#endif
