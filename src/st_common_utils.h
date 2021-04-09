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
#ifndef _COMMON_UTILS_H
#define _COMMON_UTILS_H

#include <math.h>
#include <set>
#include <sstream>
#include "st_driver.h"
#include "st_env.h"

using namespace std;

bool st_look_for_filename_in_search_path(st_env *, string const &file_name,
                                         string &complete_filename);
set<string> st_get_libraries_and_db_in_search_path(st_env *env);
string st_get_current_build_path(st_env *env);

void st_find_and_replace(string *text, string to_find, string to_replace,
                         int *substitutions_count);
bool st_find_first_and_replace(string *text, string to_find, string to_replace);
bool st_find_last_and_replace(string *text, string to_find, string to_replace);
string st_toupper(string the_string);
string st_tolower(string the_string);

template <class T> string st_to_string(T data) {
  ostringstream temp;
  temp << data;
  return temp.str();
}

string tmp_directory();
string tmp_session(string name);
bool set_session(string name);

bool st_create_directory(string directory_name);
bool st_overwrite_directory(string directory_name);
bool st_directory_already_exists(string directory_name);
bool st_delete_directory(string directory_name);

bool st_isinf(double number);
bool st_isinf(float number);

bool st_isnan(double number);
bool st_isnan(float number);

#define RE_ERROR_STRING "[Ee][Rr][Rr][Oo][Rr]"

#define RE_IDENTIFIER "[A-Za-z_][A-Za-z0-9_]*"

#define RE_SPACE "[ \t]*"
#define RE_SIGN "[+-]?"

#define RE_TRUE "[Tt][Rr][Uu][Ee]"
#define RE_FALSE "[Ff][Aa][Ll][Ss][Ee]"
#define RE_BOOL "(" RE_TRUE "|" RE_FALSE ")"

#define RE_INTEGER_NUMBER RE_SIGN "[0-9]+"
#define RE_INTEGER RE_INTEGER_NUMBER

#define RE_NAN "[Nn][Aa][Nn]"
#define RE_INF RE_SIGN "[Ii][Nn][Ff]"
#define RE_DOUBLE_NUMBER                                                       \
  RE_INTEGER_NUMBER "([\\.][0-9]*)?"                                           \
                    "("                                                        \
                    "[eE]" RE_INTEGER_NUMBER ")?"
//#define RE_DOUBLE		RE_SPACE "(" RE_DOUBLE_NUMBER "|" RE_NAN "|"
//RE_INF ")" RE_SPACE
#define RE_DOUBLE "(" RE_DOUBLE_NUMBER "|" RE_NAN "|" RE_INF ")"

#define RE_QUOTED_STRING "\"([^\"]*([\\][\"])?)*\""
#define RE_PATH "([^\"]*([\\][\"])?)*"

bool st_initialize_regular_expressions();
bool st_recognize_re(string *text, string *before, string *after,
                     regex_t &re_buf);
bool st_recognize_re(string *text, string *matched, string *before,
                     string *after, regex_t &re_buf);
bool st_recognize_identifier(string *text, string *before, string *after);
bool st_recognize_boolean(string *text, bool *boolean, string *before,
                          string *after);
bool st_recognize_integer(string *text, int *integer_value, string *before,
                          string *after);
bool st_recognize_inf(string *text, double *double_value, string *before,
                      string *after);
bool st_recognize_nan(string *text, double *double_value, string *before,
                      string *after);
bool st_recognize_error(string *text, string *before, string *after);
bool st_recognize_double_number(string *text, double *double_value,
                                string *before, string *after);
bool st_recognize_double(string *text, double *double_value, string *before,
                         string *after);
bool st_recognize_quoted_string(string *text, string *value, string *before,
                                string *after);
bool st_recognize_space(string *text, string *before, string *after);

double st_get_time();

st_command *st_get_help_from_module(string module);
set<string> st_get_modules_in_search_path();

#endif
