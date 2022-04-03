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

#include "config.h"
#include <dlfcn.h>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include "st_commands.h"
#include "st_common_utils.h"
#include "st_doe_utils.h"
#include "st_driver_utils.h"
#include "st_env.h"
#include "st_list.h"
#include "st_map.h"
#include "st_object.h"
#include "st_opt_utils.h"
#include "st_parser.h"
#include "st_point.h"
#include "st_shell_variables.h"
#include "st_sim_utils.h"
#include "st_design_space.h"
#include <string.h>
#include <vector>

#define PRS_SET_FAIL                                                           \
  current_environment.shell_variables.insert("?", st_integer(0))
#define PRS_SET_SUCCESS                                                        \
  current_environment.shell_variables.insert("?", st_integer(1))
#define PRS_SET_SUCCESS_V(v)                                                   \
  current_environment.shell_variables.insert("?", st_integer(v))
#define PRS_SET_SUCCESS_D(v)                                                   \
  current_environment.shell_variables.insert("?", st_double(v))
#define PRS_SET_SUCCESS_O(v)                                                   \
  current_environment.shell_variables.insert("?", (v))

#define __PRS_ARGS__ st_map *__prs_parameters
#define __PRS_NO_ARGS __prs_parameters->empty()
#define __PRS_ARGS_N __prs_parameters
#define __PRS_CMDN__ st_string *__prs_command_name
#define __PRS_CMDN_N __prs_command_name

void prs_command_quit();
extern bool never_fail;

#define __PRS_GET_ARG_S st_parse_command_get_arg_string(__prs_parameters)
#define __PRS_GET_ARG_O st_parse_command_get_arg_object(__prs_parameters)
#define __PRS_GET_OPT_S(name)                                                  \
  st_parse_command_get_opt_string(__prs_parameters, name)
#define __PRS_GET_OPT_I(name)                                                  \
  st_parse_command_get_opt_integer(__prs_parameters, name)

#define __PRS_COMMAND_ACTION(name)                                             \
  if (__prs_command_name->get_string() == (name))
#define __PRS_RETURN_FAIL                                                      \
  {                                                                            \
    PRS_SET_FAIL;                                                              \
    if (!prs_current_parser_session_is_interactive() && !never_fail) {         \
      prs_command_quit();                                                      \
    }                                                                          \
    return;                                                                    \
  }
#define __PRS_RETURN_SUCCESS                                                   \
  {                                                                            \
    PRS_SET_SUCCESS;                                                           \
    return;                                                                    \
  }
#define __PRS_RETURN_SUCCESS_V(v)                                              \
  {                                                                            \
    PRS_SET_SUCCESS_V(v);                                                      \
    return;                                                                    \
  }
#define __PRS_RETURN_SUCCESS_D(v)                                              \
  {                                                                            \
    PRS_SET_SUCCESS_D(v);                                                      \
    return;                                                                    \
  }
#define __PRS_RETURN_SUCCESS_O(v)                                              \
  {                                                                            \
    PRS_SET_SUCCESS_O(v);                                                      \
    return;                                                                    \
  }
#define __PRS_RETURN_SUCCESS_OD(v)                                             \
  {                                                                            \
    PRS_SET_SUCCESS_O(*v);                                                     \
    delete v;                                                                  \
    return;                                                                    \
  }

#if !defined(__BSD_READLINE__)
#include <readline/readline.h>
#else
#if defined(__MAC_OSX__)
#include <readline/readline.h>
#else
#include <editline/readline.h>
#endif /* ! MAC_OS_X */
#endif /* ! READLINE */

#include <st_mpi_utils.h>
#define __NEED_INIT_COMMANDS__
#include <algorithm>
#include <st_ast.h>
#include <st_command_list.h>
#include <st_objectives_constraints.h>
#include <st_rsm.h>
#include <sys/stat.h>
#include <unistd.h>

string st_parse_command_get_opt_string(st_map *parameter_list, string n);
int st_parse_command_get_arg_integer(st_map *parameter_list);
string st_parse_command_get_arg_string(st_map *parameter_list);
string st_get_ro_database_name(int n);

/**
 * For each argument,
 *  if it is at 0, complete among the current commands.
 *  if it begins with a '--' complete among option names of the current command.
 *  if it begins with a '$' complete among the names of the current variables.
 *    else complete among the names of the available databases and files.
 */

char **generate_possible_expansion_list(const char *text, int start, int end);
char *command_generator(const char *text, int state);
char *variable_generator(const char *text, int state);
char *options_generator(const char *text, int state);
char *lib_generator(const char *text, int state);

void prs_initialize_readline() {
  rl_readline_name = (char *)"most";
  rl_attempted_completion_function = generate_possible_expansion_list;
}

#define completion_matches rl_completion_matches

string current_command;

char **generate_possible_expansion_list(const char *text, int start, int end) {
  char **matches;

  matches = (char **)NULL;

  if (rl_line_buffer[start - 1] == '$') {
    matches = completion_matches(text, variable_generator);
    goto dont_show_files;
  }

  if (rl_line_buffer[start - 1] == '\"') {
    matches = completion_matches(text, lib_generator);
#if defined(__MAC_OSX__)
    if (matches) {
      if (matches[2] == NULL) {
        char *m = matches[0];
        matches[0] = strdup((matches[0] + string("\" ")).c_str());
        free(m);
      }
    }
#endif
    goto dont_show_files;
  }

  if (start >= 0) {
    if (rl_line_buffer[start] == '-' && rl_line_buffer[start + 1] == '-') {
      int i = 0;
      int k = 0;
      char cmd_tmp[30];
      bool cmd_not_found = true;

      while (rl_line_buffer[i] == ' ' && i < start)
        i++;

      while (rl_line_buffer[i] != ' ' && i < start && k < (30 - 1))
        cmd_tmp[k++] = rl_line_buffer[i++];

      cmd_tmp[k] = '\0';

      current_command = cmd_tmp;

      matches = completion_matches(text, options_generator);
    } else {
      matches = completion_matches(text, command_generator);
      return (matches);
    }
  }

dont_show_files:
  /** Dont show files */
  if (matches == NULL) {
    matches = (char **)malloc(2 * sizeof(char *));
    matches[0] = strdup("");
    matches[1] = strdup("");
    matches[2] = NULL;
  }

  return (matches);
}

vector<string> possible_commands;

void print_pc() {
  vector<string>::iterator k;
  // cout << "Possible commands size " << possible_commands.size() << endl;
  for (k = possible_commands.begin(); k != possible_commands.end(); k++) {
    cout << *k << endl;
  }
}

void st_add_possible_command(string c) { possible_commands.push_back(c); }

void st_remove_possible_command(string c) {
  vector<string>::iterator k;
  k = find(possible_commands.begin(), possible_commands.end(), c);
  if (k != possible_commands.end())
    possible_commands.erase(k);
}

void st_setup_command_line_completion() {
  map<string, st_command>::iterator index;
  for (index = st_commands.begin(); index != st_commands.end(); index++) {
    st_add_possible_command(index->first);
  }
  sort(possible_commands.begin(), possible_commands.end());
}

/* Generator function for command completion.  STATE lets us
 * know whether to start from scratch; without any state
 * (i.e. STATE == 0), then we start at the top of the list. */
char *command_generator(const char *text, int state) {
  static int len;
  static vector<string>::iterator index;
  char *name;

  /* If this is a new word to complete, initialize now.  This
   * includes saving the length of TEXT for efficiency, and
   * initializing the index variable to 0. */
  if (!state) {
    index = possible_commands.begin();
    len = strlen(text);
  }

  /* Return the next name which partially matches from the
   *      command list. */
  while (index != possible_commands.end()) {
    string name = *index;
    index++;

    if (strncmp(name.c_str(), text, len) == 0)
      return (strdup(name.c_str()));
  }

  /* If no names matched, then return NULL. */
  return ((char *)NULL);
}

char *variable_generator(const char *text, int state) {
  static int len;
  static st_map_const_iterator index;
  char *name;

  /* If this is a new word to complete, initialize now.  This
   * includes saving the length of TEXT for efficiency, and
   * initializing the index variable to 0. */
  if (!state) {
    index = current_environment.shell_variables.begin();
    len = strlen(text);
  }

  /* Return the next name which partially matches from the
   *      command list. */
  while (index != current_environment.shell_variables.end()) {
    string name = index->first;
    index++;

    if (strncmp(name.c_str(), text, len) == 0)
      return (strdup(name.c_str()));
  }

  /* If no names matched, then return NULL. */
  return ((char *)NULL);
}

char *lib_generator(const char *text, int state) {
  static int len;
  static set<string>::iterator index;
  static set<string> names;
  char *name;

  /* If this is a new word to complete, initialize now.  This
   * includes saving the length of TEXT for efficiency, and
   * initializing the index variable to 0. */
  if (!state) {
    names = st_get_libraries_and_db_in_search_path(&current_environment);
    index = names.begin();
    len = strlen(text);
  }

  /* Return the next name which partially matches from the
   *      command list. */
  while (index != names.end()) {
    string name = *index;
    index++;

    // cout << "at " << index << " comparing " << name << " with " << text <<
    // endl;

    if (strncmp(name.c_str(), text, len) == 0)
      return (strdup(name.c_str()));
  }

  /* If no names matched, then return NULL. */
  return ((char *)NULL);
}

char *options_generator(const char *text, int state) {
  static int len;
  static int index;

  /* If this is a new word to complete, initialize now.  This
   * includes saving the length of TEXT for efficiency, and
   * initializing the index variable to 0. */
  if (!state) {
    index = 0;
    len = strlen(text);
  }

  /* Return the next name which partially matches from the
   *      command list. */
  while (index < st_commands[current_command].command_options_name.size()) {
    string name = st_commands[current_command].command_options_name[index];
    int p = name.find('=');
    name = name.substr(0, (p) + 1);

    index++;

    if (strncmp(name.c_str(), text, len) == 0)
      return (strdup(name.c_str()));
  }

  /* If no names matched, then return NULL. */
  return ((char *)NULL);
}

void prs_unload_driver() {
  if (current_environment.current_driver != NULL) {
    void *handle = current_environment.current_driver->handle;
    delete current_environment.current_driver;
    dlclose(handle);
  }
  current_environment.current_driver = NULL;
}

void prs_unload_doe() {
  if (current_environment.current_doe != NULL) {
    void *handle = current_environment.current_doe->handle;
    delete current_environment.current_doe;
    dlclose(handle);
  }
  current_environment.current_doe = NULL;
}

void prs_unload_optimizer() {
  if (current_environment.current_optimizer != NULL) {
    void *handle = current_environment.current_optimizer->handle;
    delete current_environment.current_optimizer;
    dlclose(handle);
  }
}

extern FILE *static_log;

extern void st_shutdown();

extern void st_graceful_exit(int ec);

#include <st_error.h>

void prs_command_quit() {
  prs_unload_driver();
  prs_unload_doe();
  prs_unload_optimizer();

  prs_display_message("Exiting from MOST shell");

  st_ast_free_ast();

  if (static_log)
    fclose(static_log);

  st_graceful_exit(EXIT_ERR_NO_ERR);
}

void prs_command_set(st_object *nameo, st_object *value) {
  if (!is_a<st_string *>(nameo)) {
    prs_display_error("Syntax error");
    __PRS_RETURN_FAIL;
  }

  if (value != NULL) {
    st_string *name = to<st_string *>(nameo);
    if (value->is_global() || value->is_stack_permanent()) {
      // cout << "Make a copy:" << name << "" << value->print() << endl;
      current_environment.shell_variables.insert(name->get_string(), *value);
    } else {
      current_environment.shell_variables.insert_dont_copy(name->get_string(),
                                                           value);
    }
    current_environment.shell_variables.insert("?", *value);
  }
}

st_object *prs_command_read_variable(st_object *n, bool silent) {
  if (!is_a<st_string *>(n)) {
    return NULL;
  }

  st_string *name = to<st_string *>(n);

  const st_object *p;
  if (!current_environment.shell_variables.get(name->get_string(), p)) {
    return NULL;
  }
  return const_cast<st_object *>(p);
}

bool prs_command_db_insert_point(st_object *p, string destination) {
  if (!is_a<st_point *>(p)) {
    prs_display_error("Syntax error, not a point.");
    return false;
  }
  if (!current_environment.get_database(destination)) {
    prs_display_error("Not existing database");
    return false;
  }
  current_environment.get_database(destination)
      ->insert_point(to<st_point *>(p));
  return true;
}

bool prs_command_db_write(string name, string origin_db) {
  if (!current_environment.get_database(origin_db)) {
    prs_display_error("Specify a valid database");
    return false;
  }

  prs_display_message("Writing the database to disk");
  if (!current_environment.get_database(origin_db)->write_to_file(
          name.c_str())) {
    prs_display_error("There were problems writing to file");
    return false;
  } else {
    prs_display_message("Database correctly written");
    return true;
  }
}

int prs_command_db_import(string csv_filename, string destination_db,
                          bool use_symbols) {
  if (current_environment.available_dbs.count(destination_db)) {
    prs_display_message("Clearing the existing database \"" + destination_db +
                        "\"");
    current_environment.available_dbs[destination_db]->clear();
  } else {
    prs_display_message("Created database " + destination_db);
    current_environment.available_dbs[destination_db] = new st_database;
  }
  int result =
      current_environment.available_dbs[destination_db]->import_from_file(
          &current_environment, csv_filename.c_str(), use_symbols);
  return result;
}

bool prs_command_read(string name, bool read_object) {
  if (!read_object) {
    prs_display_message("Reading script " + name);
  }
  prs_parse_and_execute_file(name.c_str());
  return true;
}

bool prs_command_write_object(string filename, st_object *obj) {
  prs_display_message("Writing object to file " + filename);
  string out = "set ? = " + obj->full_print() + "\n";
  ofstream file_out(filename.c_str(), ios::out);
  if (file_out.fail())
    return false;
  file_out << out;
  return true;
}

bool prs_command_db_read(string name, string destination) {
  if (destination == "") {
    prs_display_error("Please define a destination db");
    return false;
  }

  if (!current_environment.get_database(destination)) {
    current_environment.insert_new_database(new st_database(), destination);
  }

  current_environment.get_database(destination)->clear();
  prs_display_message("Reading database " + name);

  int result = current_environment.get_database(destination)
                   ->read_from_file(name.c_str());
  return result > 0;
}

bool prs_command_load_optimizer(string name) {
  prs_unload_optimizer();
  if (!opt_select_optimizer(&current_environment, name)) {
    prs_display_error("The specified optimizer does not exists; check for lib" +
                      name + ".so in your search path");
    return false;
  } else {
    prs_display_message(
        "Current optimizer has been set to '" +
        current_environment.current_optimizer->get_information() + "'");
    return true;
  }
}

bool prs_command_load_doe(string name) {
  prs_unload_doe();
  if (!doe_select_doe(&current_environment, name)) {
    prs_display_error("specified DoE does not exists; check for lib" + name +
                      ".so in your search path");
    return false;
  } else {
    prs_display_message("Current DoE has been set to '" +
                        current_environment.current_doe->get_information() +
                        "'");
    return true;
  }
}

int prs_command_drv_load_driver(string n) {
  string name = n;

  prs_unload_driver();

  if (!drv_select_driver(&current_environment, name))
    return false;
  else
    return true;
}

bool current_driver_is_xml = false;
string current_xml_ds;

bool prs_command_activate_driver(string name) {
  current_driver_is_xml = false;
  if (st_mpi_environment_active()) {
    st_mpi_broadcast_send_command("SYNCHRONIZE_SHELL_VARIABLES");
    st_mpi_broadcast_send_data(current_environment.shell_variables.print());
    st_mpi_broadcast_send_command("LOAD_DRIVER");
    st_mpi_broadcast_send_data(name);
  }
  if (prs_command_drv_load_driver(name)) {
    if (prs_command_drv_instantiate_driver()) {
      if (name == "st_xml") {
        string xf;
        if (current_environment.shell_variables.get_string(
                "xml_design_space_file", xf)) {
          current_xml_ds = xf;
          current_driver_is_xml = true;
          return true;
        }
      } else {
        return true;
      }
    }
  }
  return false;
}

bool prs_command_drv_instantiate_driver() {
  st_env *env = &current_environment;

  if (!env->current_driver) {
    prs_display_error("Cannot instantiate driver");
    return false;
  }

  st_design_space *ds = env->current_driver->get_design_space(env);

  if (!ds) {
    prs_display_error("Cannot instantiate driver");
    return false;
  }

  st_vector *parameter_names = new st_vector();
  for (int i = 0; i < ds->ds_parameters.size(); i++) {
    string name = ds->ds_parameters[i].name;

    parameter_names->insert(i, st_string(name));

    if (ds->ds_parameters[i].type == ST_DS_SCALAR) {
      string lower = ds->get_scalar_min_symbol(env, name);
      string upper = ds->get_scalar_max_symbol(env, name);

      st_vector *param_range = new st_vector();
      param_range->insert(0, st_string(lower));
      param_range->insert(1, st_string(upper));

      string par_name = name + "_bounds";

      current_environment.shell_variables.insert(par_name, *param_range);
      delete param_range;
    }
  }
  current_environment.shell_variables.insert("ds_parameters", *parameter_names);
  delete parameter_names;

  st_vector *metrics = new st_vector();

  for (int i = 0; i < ds->metric_names.size(); i++) {
    metrics->insert(i, st_string(ds->metric_names[i]));
  }
  current_environment.shell_variables.insert("metrics", *metrics);
  delete metrics;

  if (env->current_design_space)
    delete env->current_design_space;

  env->current_design_space = ds;

  return true;
}

bool prs_command_db_export_xml(st_map *parameters);

bool prs_command_db_export(st_map *parameters) {
  string file_name = st_parse_command_get_opt_string(parameters, "file_name");
  string origin = st_parse_command_get_arg_string(parameters);

  if (current_environment.current_design_space == NULL) {
    prs_display_error("Design Space not defined, format unknown");
    return false;
  }

  if (!current_environment.get_database(origin)) {
    prs_display_error("Specify a valid source database");
    return false;
  }
  int mode = st_parse_command_get_opt_integer(parameters, "mode_frontier");

  prs_display_message("Saving the database in CSV format..");

  /** Opens the file */
  ofstream file_out(file_name.c_str(), ios::out);
  if (file_out.fail()) {
    prs_display_error("Cannot open the output file");
    return false;
  }

  vector<string> &ds_parameters_names =
      current_environment.current_design_space->ds_parameters_names;
  vector<string> &metric_names =
      current_environment.current_design_space->metric_names;

  for (int i = 0; i < ds_parameters_names.size(); i++) {
    file_out << ds_parameters_names[i] << ";";
  }

  /** Write out the metric names */
  for (int i = 0; i < metric_names.size(); i++) {
    file_out << metric_names[i] << ";";
  }
  file_out << "cluster;";
  file_out << endl;

  /** Write out the database list */
  st_point_set *database_list =
      current_environment.get_database(origin)->get_set_of_points();
  st_point_set::iterator c;
  c = database_list->begin();
  st_point &ref = *(c->second);
  for (c = database_list->begin(); c != database_list->end(); c++) {
    st_point &p = *(c->second);
    if (!p.get_error()) {
      string point_repr;
      st_env *env = &current_environment;
      if (!mode)
        point_repr =
            env->current_design_space->get_point_representation_csv_s(env, p);
      else
        point_repr =
            env->current_design_space->get_point_representation_csv_i(env, p);

      file_out << point_repr;
      for (int i = 0; i < metric_names.size(); i++) {
        file_out << p.get_metrics(i) << ";";
      }
      file_out << p.get_cluster() << ";";
      file_out << endl;
    }
  }
  return true;
}

int exploring = 0;
extern int interrupt_exploration;

int prs_command_tune() {
  if (!current_environment.current_driver) {
    prs_display_error("Please define the driver");
    return false;
  }
  if (!current_environment.current_optimizer) {
    prs_display_error("Please define the optimizer");
    return false;
  }
  if (!current_environment.current_doe) {
    prs_display_error("Please define the doe");
    return false;
  }

  interrupt_exploration = 0;
  exploring = 1;
  int explored_points =
      current_environment.current_optimizer->explore(&current_environment);
  exploring = 0;

  prs_display_message_n_value_m("The current explorer analyzed ",
                                explored_points, " points");
  return explored_points;
}

st_object *prs_command_opt_estimate(st_object *point) {
  if (!current_environment.current_driver) {
    prs_display_error("Please define the driver");
    return NULL;
  }
  st_point *p = to<st_point *>(point);
  if (!p) {
    prs_display_error("Please specify a valid point");
    return NULL;
  }
  st_batch_job job(*p);
  current_environment.current_dispatcher->submit(&current_environment, &job);
  return job.get_point_at(0);
}

void prs_opt_show_info() {
  if (!current_environment.current_optimizer) {
    prs_display_error("Please define the optimizer");
    return;
  }
  cout << current_environment.current_optimizer->get_information();
}

void prs_drv_show_info() {
  if (!current_environment.current_driver) {
    prs_display_error("Please define the driver");
    return;
  }
  cout << current_environment.current_driver->get_information();
}

void prs_command_show_variables() {
  cout << "\nShell variables:\n";
  printf("%-20s  %-10s \n", "Name", "Value");
  printf("%-20s  %-10s \n", "----------", "------------------------");
  st_map_const_iterator i;
  for (i = current_environment.shell_variables.begin();
       i != current_environment.shell_variables.end(); i++) {
    if (i->first != "current_build_path" && i->first != "?" &&
        i->first != "xml_design_space_file")
      printf("%-20s  %-10s  \n", i->first.c_str(), i->second->print().c_str());
  }
  cout << endl;

  cout << "Optimization objectives:\n";
  printf("%-20s  %-10s \n", "Name", "Expression");
  printf("%-20s  %-10s \n", "----------", "------------------------");
  vector<st_objective *>::iterator k;
  for (k = current_environment.optimization_objectives.begin();
       k != current_environment.optimization_objectives.end(); k++) {
    printf("%-20s  ", ((*k)->name + "(" + (*k)->point_name + ")").c_str());
    (*k)->obj_expression->print();
    printf("\n");
  }
  cout << endl;

  cout << "Optimization constraints:\n";
  printf("%-20s  %-10s \n", "Name", "Expression");
  printf("%-20s  %-10s \n", "----------", "------------------------");
  vector<st_constraint *>::iterator p;
  for (p = current_environment.optimization_constraints.begin();
       p != current_environment.optimization_constraints.end(); p++) {
    printf("%-20s  ", ((*p)->name + "(" + (*p)->point_name + ")").c_str());
    (*p)->print_constraint();
    printf("\n");
  }
  cout << endl;

  cout << "Databases in memory\n";
  printf("%-30s  %-10s \n", "Name", "size");
  printf("--------------------------------------------\n");
  map<string, st_database *>::iterator j;
  for (j = current_environment.available_dbs.begin();
       j != current_environment.available_dbs.end(); j++) {
    printf("%-30s  %-10d \n", j->first.c_str(), j->second->count_points());
  }
}

int st_parse_command_get_opt_integer(st_map *parameter_list, string n);

bool prs_command_filter_pareto(st_map *parameter_list) {
  string database = st_parse_command_get_arg_string(parameter_list);

  if (!current_environment.get_database(database)) {
    prs_display_error("Please specify a valid database");
    return false;
  }

  int level = st_parse_command_get_opt_integer(parameter_list, "level");
  int valid = st_parse_command_get_opt_integer(parameter_list, "valid");

  try {
    prs_display_message(("Filtering " + database + " for pareto points"));

    current_environment.get_database(database)->cache_update(
        &current_environment);
    if (!level)
      sim_compute_pareto(&current_environment,
                         current_environment.get_database(database), valid);
    else
      sim_compute_pareto_nth(&current_environment,
                             current_environment.get_database(database), level,
                             valid);

    return true;
  } catch (exception &e) {
    prs_display_error("Unable to evaluate the Pareto set");
    return false;
  }
}

bool prs_command_compute_kmeans_clusters(st_map *parameter_list) {
  if (current_environment.optimization_objectives.size() >= 1) {
    string database = st_parse_command_get_arg_string(parameter_list);
    if (!current_environment.get_database(database)) {
      prs_display_error("Please specify a valid database");
      return false;
    }
    prs_display_message("Computing kmeans clusters");

    int clusters = st_parse_command_get_opt_integer(parameter_list, "clusters");
    int iterations =
        st_parse_command_get_opt_integer(parameter_list, "iterations");

    try {
      double goodness = sim_compute_kmeans_clusters(
          &current_environment, clusters, iterations,
          current_environment.get_database(database));

      current_environment.shell_variables.insert("kmeans_goodness",
                                                 st_double(goodness));
      return true;
    } catch (exception &e) {
      prs_display_error(e.what());
      return false;
    }
  } else {
    prs_display_error("Please define a consistent list of objectives");
    return false;
  }
}

st_point look_for_inverse(st_point &e, st_database *db, string parname) {
  bool found = false;
  st_point t;
  int idx;

  if (!(current_environment.current_design_space->scalar_parameters.count(
          parname))) {
    t.set_error(ST_POINT_NON_FATAL_ERROR);
    return t;
  }
  if (e.get_error()) {
    t.set_error(ST_POINT_NON_FATAL_ERROR);
    return t;
  }

  idx = current_environment.current_design_space->ds_parameters_index[parname];

  int min = current_environment.current_design_space->get_scalar_min(
      &current_environment, parname);
  int max = current_environment.current_design_space->get_scalar_max(
      &current_environment, parname);

  if (e[idx] != min) {
    t.set_error(ST_POINT_NON_FATAL_ERROR);
    return t;
  }

  st_point_set *database_list = db->get_set_of_points();
  st_point_set::iterator c;
  for (c = database_list->begin(); c != database_list->end(); c++) {
    st_point &p = *(c->second);
    if (!p.get_error()) {
      if (p[idx] != e[idx]) {
        bool equal = true;
        for (int i = 0; i < e.size(); i++) {
          if (i != idx) {
            if (p[i] != e[i])
              equal = false;
          }
        }
        if (equal) {
          t = p;
          return t;
        }
      }
    }
  }
  t.set_error(ST_POINT_NON_FATAL_ERROR);
  return t;
}

bool prs_write_out_db_effect(st_map *metrics, st_database *db, string name,
                             string parname) {

  /** Opens the file */
  ofstream file_out(name.c_str(), ios::out);

  /** Write out the database list */
  st_point_set *database_list = db->get_set_of_points();
  st_point_set::iterator c;
  for (c = database_list->begin(); c != database_list->end(); c++) {
    st_point &p = *(c->second);
    st_point e = look_for_inverse(p, db, parname);
    if (!p.get_error() & !e.get_error()) {
      st_map_const_iterator i;
      for (i = metrics->begin(); i != metrics->end(); i++) {
        if (i->first == "xaxis" || i->first == "yaxis" || i->first == "zaxis") {
          try {
            st_string name = dynamic_cast<st_string &>(
                const_cast<st_object &>(*(i->second)));
            int index =
                current_environment.get_objective_index(name.get_string());
            file_out << current_environment.optimization_objectives[index]
                            ->eval(&p, index)
                     << "   ";
          } catch (exception &e) {
            prs_display_error("Bad objectives specified");
            return false;
          }
        }
      }
      for (i = metrics->begin(); i != metrics->end(); i++) {
        if (i->first == "xaxis" || i->first == "yaxis" || i->first == "zaxis") {
          try {
            st_string name = dynamic_cast<st_string &>(
                const_cast<st_object &>(*(i->second)));
            int index =
                current_environment.get_objective_index(name.get_string());
            file_out << current_environment.optimization_objectives[index]
                            ->eval(&e, index)
                     << "   ";
          } catch (exception &e) {
            prs_display_error("Bad objectives specified");
            return false;
          }
        }
      }
      file_out << p.print_octave(&current_environment);

      file_out << endl;
    } else {
    }
  }
  return true;
}

bool prs_write_out_db(st_map *metrics, st_database *db, string name) {

  /** Opens the file */
  ofstream file_out(name.c_str(), ios::out);

  /** Write out the database list */
  st_point_set *database_list = db->get_set_of_points();
  st_point_set::iterator c;
  for (c = database_list->begin(); c != database_list->end(); c++) {
    st_point &p = *(c->second);
    if (!p.get_error()) {
      st_map_const_iterator i;
      for (i = metrics->begin(); i != metrics->end(); i++) {
        if (i->first == "xaxis" || i->first == "yaxis" || i->first == "zaxis") {
          try {
            st_string name = dynamic_cast<st_string &>(
                const_cast<st_object &>(*(i->second)));
            int index =
                current_environment.get_objective_index(name.get_string());
            file_out << current_environment.optimization_objectives[index]
                            ->eval(&p, index)
                     << "   ";
          } catch (exception &e) {
            prs_display_error("Bad objectives specified");
            return false;
          }
        }
      }
    }
    file_out << endl;
  }
  return true;
}

void prs_command_echo(st_object *par, string mode, string file) {
  string stri;
  const char *str;
  string the_string;

  if (is_a<st_string *>(par))
    str = to<st_string *>(par)->get_string().c_str();
  else {
    the_string = (par)->print();
    str = the_string.c_str();
  }

  if (mode == "") {
    printf("%s", str);
    printf("\n");
    __PRS_RETURN_SUCCESS;
  } else {
    const char *mod;
    if (mode == "create")
      mod = "w";
    else
      mod = "a";

    FILE *fh = fopen(file.c_str(), mod);

    if (!fh) {
      prs_display_error("Cannot create file");
      __PRS_RETURN_FAIL;
    }

    fprintf(fh, "%s\n", str);
    fclose(fh);

    __PRS_RETURN_SUCCESS;
  }
}

#include <st_conv.h>

void prs_write_data_and_script(st_map *metrics, ofstream &file_out,
                               st_database *db, string filename, string cmd,
                               string graphic, bool &is_first, bool allinone,
                               bool vrsize = false, bool use_line_color = false,
                               int lc = 1, bool effect = false,
                               string parname = "", bool relative = false) {
  string columns = "";
  string withvs = "";
  cmd = "plot";

  if (vrsize) {
    columns = "using 1:2:3";
    withvs = " pointtype 4 pointsize variable";
    graphic = "points";
  }
  if (db->get_set_of_points()->get_size()) {
    bool v;

    if (!effect)
      v = prs_write_out_db(metrics, db, (filename).c_str());
    else {
      filename = filename + "-" + parname;
      v = prs_write_out_db_effect(metrics, db, (filename).c_str(), parname);
    }

    if (graphic == "") {
      if (db->get_set_of_points()->get_size() > 1000)
        graphic = "dots";
      else
        graphic = "points";
    }
    if (v) {
      if (!allinone) {
        if (!is_first)
          file_out << cmd << " \"" + (filename) + "\" " + columns + " with "
                   << graphic << withvs << endl;
        else
          file_out << "replot \"" + (filename) + "\" " + columns + " with "
                   << graphic << withvs << endl;
        is_first = true;
      } else {
        if (!effect) {
          if (!is_first)
            file_out << cmd << " \"" + (filename) + "\" " + columns + " with "
                     << graphic << withvs;
          else
            file_out << ", \"" + (filename) + "\" " + columns + " with "
                     << graphic << withvs;

          is_first = true;

          if (use_line_color) {
            file_out << " ls " << st_itoa(lc) << " ";
          }
        } else {
          if (relative)
            file_out << cmd
                     << " \"" + (filename) + "\" using ($3-$1):($4-$2) with "
                     << graphic << withvs;
          else
            file_out << cmd
                     << " \"" + (filename) +
                            "\" using 1:2:($3-$1):($4-$2) with vectors ";
          is_first = true;
        }
      }
    }
  }
}

void write_out_scatter(st_vector *g1, st_vector *g2, ofstream &script,
                       string &name, bool &not_first, string graphic,
                       int allinone, string range) {
  ofstream file_outdata(name.c_str(), ios::out);
  for (int i = 0; i < g1->size(); i++) {
    file_outdata << g1->get(i).print() << "   " << g2->get(i).print() << endl;
  }
  file_outdata.close();
  if (graphic == "")
    graphic = "points";
  if (not_first) {
    if (!allinone) {
      script << "replot \"" + (name) + "\" with " << graphic << endl;
    } else {
      script << ", \"" + (name) + "\" with " << graphic;
    }
  } else {
    script << "plot " << range << " \"" + (name) + "\" with " << graphic;
    if (!allinone)
      script << endl;
  }

  not_first = true;
}

typedef vector<st_vector *> vvect;

bool construct_pair(st_vector *vs, pair<vvect, string> &p) {
  if (!vs) {
    prs_display_error(
        "Please specify a valid vector of two vectors and a string name");
    return 0;
  }
  if (vs->size() < 3) {
    prs_display_error(
        "Please specify a valid vector of two vectors and a string name");
    return 0;
  }
  st_vector *v1 = to<st_vector *>(const_cast<st_object *>(&(vs->get(0))));
  st_vector *v2 = to<st_vector *>(const_cast<st_object *>(&(vs->get(1))));
  st_string *s1 = to<st_string *>(const_cast<st_object *>(&(vs->get(2))));
  if (!v1 || !v2 || !s1) {
    prs_display_error("Please specify a valid vector of two vectors");
    return 0;
  }
  vvect data_scat;
  data_scat.push_back(v1);
  data_scat.push_back(v2);
  p.first = data_scat;
  if (s1->get_string() == "") {
    prs_display_error(
        "Please specify a valid vector of two vectors and a string name");
    return false;
  }
  p.second = s1->get_string();
  return true;
}

double get_int_or_double(st_object *obj) {
  if (to<st_double *>(obj))
    return to<st_double *>(obj)->get_double();

  if (to<st_integer *>(obj))
    return to<st_integer *>(obj)->get_integer();

  throw std::logic_error("Expecting a double or an integer");
}

string st_itos(int i) {
  ostringstream str;
  str << i;
  return str.str();
}

typedef struct box_plot_data {
  double x;
  double Q1;
  double Q3;
  double Med;
  double sample_min;
  double sample_max;
  vector<double> outliers;

  string box_plot_data_filename;
  string outliers_data_filename;
} box_plot_data;

bool prs_command_plot_vector(st_map *metrics) {
  string graphic = st_parse_command_get_opt_string(metrics, "graphic");
  string title = st_parse_command_get_opt_string(metrics, "title");
  string range = st_parse_command_get_opt_string(metrics, "range");
  int wd = st_parse_command_get_opt_integer(metrics, "use_working_dir");
  int monochrome = st_parse_command_get_opt_integer(metrics, "monochrome");
  int allinone = st_parse_command_get_opt_integer(metrics, "onepage");
  int logscale = st_parse_command_get_opt_integer(metrics, "logscale");
  double factor = st_parse_command_get_opt_double(metrics, "size", 1.0);

  bool box = st_parse_command_get_opt_integer(metrics, "box") > 0;
  bool use_classes =
      st_parse_command_get_opt_integer(metrics, "use_classes") > 0;
  bool clean_temp = st_parse_command_get_opt_integer(metrics, "clean_temp") > 0;

  bool user_yrange = false;
  double yrange_min = 0;
  double yrange_max = 0;

  bool user_xrange = false;
  double xrange_min = 0;
  double xrange_max = 0;

  if (st_parse_command_get_opt_object(metrics, "yrange") != NULL) {
    st_object *yrange_opt = st_parse_command_get_opt_object(metrics, "yrange");
    bool error = false;
    if (to<st_list *>(yrange_opt)) {
      st_list *yrange_list = to<st_list *>(yrange_opt);
      if (yrange_list->size() != 2)
        error = true;
      else {
        list<st_object *>::iterator it = yrange_list->begin();
        try {
          yrange_min = get_int_or_double((*it));
          it++;
          yrange_max = get_int_or_double((*it));
          if (yrange_min >= yrange_max)
            throw std::logic_error("");
          user_yrange = true;
        } catch (exception &e) {
          yrange_min = 0;
          yrange_max = 0;
          error = true;
        }
      }
    } else
      error = true;
    if (error)
      prs_display_error("--yrange option: expected a list of two numbers, "
                        "where the first is lesser than the second");
  }

  if (st_parse_command_get_opt_object(metrics, "xrange") != NULL) {
    st_object *xrange_opt = st_parse_command_get_opt_object(metrics, "xrange");
    bool error = false;
    if (to<st_list *>(xrange_opt)) {
      st_list *xrange_list = to<st_list *>(xrange_opt);
      if (xrange_list->size() != 2)
        error = true;
      else {
        list<st_object *>::iterator it = xrange_list->begin();
        try {
          xrange_min = get_int_or_double((*it));
          it++;
          xrange_max = get_int_or_double((*it));
          if (xrange_min >= xrange_max)
            throw std::logic_error("");
          user_xrange = true;
        } catch (exception &e) {
          xrange_min = 0;
          xrange_max = 0;
          error = true;
        }
      }
    } else
      error = true;
    if (error)
      prs_display_error("--xrange option: expected a list of two numbers, "
                        "where the first is lesser than the second");
  }

  st_object *vcs = st_parse_command_get_arg_object(metrics);

  vector<pair<vvect, string> > dbs;

  vector<box_plot_data> box_plots;
  double max_x;
  double min_x;
  double max_y;
  double min_y;
  int xrange_min_class, xrange_max_class;
  if (box) {
    /*
    Expected data format:
    { (* map_1 *),..., (* map_i *),..., (* map_n *) }, with n > 0


    map_i:
    _______________________________________
                  |
    key           | type
    ______________|________________________
    x             | st_double or st_integer
    sample_min    | st_double or st_integer
    Q1            | st_double or st_integer
    Med           | st_double or st_integer
    Q3            | st_double or st_integer
    sample_max    | st_double or st_integer
    outliers      | st_vector

    outliers:
    each element of the vector must be of type st_double or st_integer. The
    empty vector is admitted.
    */
    st_list *bes = to<st_list *>(vcs);
    if (!bes) {
      prs_display_error("Please define a list of maps");
      return false;
    }
    if (bes->size() == 0) {
      prs_display_error("Please specify at least one box plot");
      return false;
    }
    box_plots.reserve(bes->size());
    list<st_object *>::iterator it = bes->begin();
    int i = 0;
    for (it = bes->begin(); it != bes->end(); it++) {
      st_map *current_map = to<st_map *>((*it)->gen_copy());
      if (!current_map) {
        prs_display_error("Please specify a vector of maps");
        return false;
      }
      if (current_map->size() != 7) {
        prs_display_error("Please specify all the map keys for the box-plot");
        return false;
      }
      box_plot_data current_bp;
      st_object const *current_obj;
      bool get_result;

      get_result = current_map->get("x", current_obj);
      if (!get_result) {
        prs_display_error("Please specify all the map keys for the box-plot");
        return false;
      }
      current_bp.x = get_int_or_double(current_obj->gen_copy());

      get_result = current_map->get("sample_min", current_obj);
      if (!get_result) {
        prs_display_error("Please specify all the map keys for the box-plot");
        return false;
      }
      current_bp.sample_min = get_int_or_double(current_obj->gen_copy());

      get_result = current_map->get("Q1", current_obj);
      if (!get_result) {
        prs_display_error("Please specify all the map keys for the box-plot");
        return false;
      }
      current_bp.Q1 = get_int_or_double(current_obj->gen_copy());

      get_result = current_map->get("Med", current_obj);
      if (!get_result) {
        prs_display_error("Please specify all the map keys for the box-plot");
        return false;
      }
      current_bp.Med = get_int_or_double(current_obj->gen_copy());

      get_result = current_map->get("Q3", current_obj);
      if (!get_result) {
        prs_display_error("Please specify all the map keys for the box-plot");
        return false;
      }
      current_bp.Q3 = get_int_or_double(current_obj->gen_copy());

      get_result = current_map->get("sample_max", current_obj);
      if (!get_result) {
        prs_display_error("Please specify all the map keys for the box-plot");
        return false;
      }
      current_bp.sample_max = get_int_or_double(current_obj->gen_copy());

      if (i == 0) {
        max_x = current_bp.x;
        min_x = current_bp.x;
        max_y = current_bp.sample_max;
        min_y = current_bp.sample_min;
      } else {
        max_x = max(max_x, current_bp.x);
        min_x = min(min_x, current_bp.x);
        max_y = max(max_y, current_bp.sample_max);
        min_y = min(min_y, current_bp.sample_min);
      }
      get_result = current_map->get("outliers", current_obj);
      if (!get_result) {
        prs_display_error("Please specify all the map keys for the box-plot");
        return false;
      }
      st_vector *stv_outliers = to<st_vector *>(current_obj->gen_copy());
      if (stv_outliers == NULL) {
        prs_display_error(
            "To \"outliers\" key must be related a vector of numbers");
        return false;
      }
      for (int out_id = 0; out_id < stv_outliers->size(); out_id++) {
        current_bp.outliers.push_back(
            get_int_or_double(stv_outliers->get(out_id).gen_copy()));
        max_y = max(max_y, current_bp.outliers[out_id]);
        min_y = min(min_y, current_bp.outliers[out_id]);
      }
      // generating box plot and outliers data files
      current_bp.box_plot_data_filename = "box_" + st_itos(i);
      ofstream curr_box_plot(current_bp.box_plot_data_filename.c_str(),
                             ios::out);
      curr_box_plot << current_bp.x << " " << current_bp.Q1 << " "
                    << current_bp.sample_min << " " << current_bp.Med;
      // curr_box_plot << " " << current_bp.sample_max << " " << current_bp.Q3
      // << " " << " \n";
      curr_box_plot << " " << current_bp.sample_max << " " << current_bp.Q3
                    << " " << i << " \n";
      curr_box_plot.close();

      current_bp.outliers_data_filename = "outliers_" + st_itos(i);
      ofstream curr_outliers;
      curr_outliers.open(current_bp.outliers_data_filename.c_str(), ios::out);
      for (int out_id = 0; out_id < current_bp.outliers.size(); out_id++)
        curr_outliers << current_bp.x << "  " << current_bp.outliers[out_id]
                      << "\n";
      curr_outliers.close();

      box_plots.push_back(current_bp);

      i++;
    }
    xrange_max_class = i;
    xrange_min_class = -1;
  } else {
    st_list *lv = to<st_list *>(vcs);
    if (lv) {
      list<st_object *>::iterator i;
      for (i = lv->begin(); i != lv->end(); i++) {
        st_vector *vs = to<st_vector *>(*i);
        pair<vvect, string> p;
        if (construct_pair(vs, p))
          dbs.push_back(p);
        else
          return false;
      }
    }
    st_vector *s = to<st_vector *>(vcs);
    if (s) {
      pair<vvect, string> p;
      if (construct_pair(s, p))
        dbs.push_back(p);
      else
        return false;
    }
  }
  if (dbs.size() == 0 && !box) {
    prs_display_error(
        "Please define at least two vectors and a name (in a vector)");
    return false;
  }
  string ix = st_parse_command_get_opt_string(metrics, "xaxis");
  string iy = st_parse_command_get_opt_string(metrics, "yaxis");
  string override = st_parse_command_get_opt_string(metrics, "override");
  string file = st_parse_command_get_opt_string(metrics, "output");
  string key = st_parse_command_get_opt_string(metrics, "key");
  if (key == "")
    key = "right";

  string root_path;
  if (wd && file != "") {
    char *root_path_c = getcwd(NULL, 0);
    root_path = root_path_c;
    free(root_path_c);

    string cmd = "mkdir -p ./" + file;
    system(cmd.c_str());
    chdir(("./" + file).c_str());
  }
  ofstream file_out("st_shell_gplot_script.scr", ios::out);
  file_out << "unset grid" << endl;
  if (box)
    file_out << "unset key" << endl;
  else
    file_out << "set key " << key << endl;
  file_out << "set xlabel \"" << ix << "\"" << endl;
  file_out << "set ylabel \"" << iy << "\"" << endl;
  if (user_yrange)
    file_out << "set yrange [ " << yrange_min << ":" << yrange_max << " ]"
             << endl;
  if (user_xrange)
    file_out << "set xrange [ " << xrange_min << ":" << xrange_max << " ]"
             << endl;
  if (logscale)
    file_out << "set logscale y 10 " << endl;

  file_out << "set title \"" << title << "\"" << endl;
  if (factor != 1.0 && !box)
    file_out << "set size " << factor << ", " << factor << endl;

  if (file != "") {
    if (monochrome)
      file_out << "set terminal pdf monochrome" << endl;
    else
      file_out << "set terminal pdf color" << endl;

    file_out << "set output \"" << file << ".pdf\"" << endl;
  }

  if (override != "") {
    file_out.close();
    if (!shell_command("cat " + override + " >> st_shell_gplot_script.scr")) {
      prs_display_error("Override file not found.");
    }
    file_out.open("st_shell_gplot_script.scr", ios::out | ios::app);
  }

  bool nf = false;
  if (box) {
    double min_distance = 0;
    bool assigned = false;
    for (int i = 0; i < box_plots.size(); i++)
      for (int j = 0; j < box_plots.size(); j++)
        if (i != j) {
          double current_distance;
          current_distance = fabs(box_plots[i].x - box_plots[j].x);
          if (assigned)
            min_distance = min(min_distance, current_distance);
          else {
            min_distance = current_distance;
            assigned = true;
          }
        }
    if (min_distance == 0)
      min_distance = 1;
    double boxwidth = min_distance * 0.2;
    double delta_y = max_y - min_y;
    if (!use_classes)
      file_out << "set boxwidth " << boxwidth << " absolute" << endl;
    if (!user_xrange) {
      if (!use_classes)
        file_out << "set xrange [ " << (min_x - (min_distance / 2)) << ":"
                 << (max_x + (min_distance / 2)) << " ]" << endl;
      else {
        file_out << "set boxwidth 0.5 absolute" << endl;
        file_out << "set xrange [ " << xrange_min_class << ":"
                 << xrange_max_class << " ]" << endl;
      }
    }
    if (!user_yrange)
      file_out << "set yrange [ " << (min_y - (delta_y * 0.1)) << ":"
               << (max_y + (delta_y * 0.1)) << " ]" << endl;
    file_out << "set xtic( 0";
    for (int i = 0; i < box_plots.size(); i++)
      file_out << ", " << box_plots[i].x;
    file_out << " )" << endl;
    if (monochrome) {
      file_out << "set style line 1 lt -1 lc rgb \"black\" lw 1" << endl;
      file_out << "set style line 2 lt -1 lc rgb \"black\" lw 3" << endl;
      file_out << "set style line 3 lt -1 lc rgb \"black\" lw 1 pt 6 ps 0.1"
               << endl;
    } else {
      file_out << "set style line 1 lt -1 lc rgb \"cyan\" lw 1" << endl;
      file_out << "set style line 2 lt -1 lc rgb \"black\" lw 3" << endl;
      file_out << "set style line 3 lt -1 lc rgb \"cyan\" lw 1 pt 6 ps 0.1"
               << endl;
    }
    file_out << "plot ";
    for (int i = 0; i < box_plots.size(); i++) {
      if (i > 0)
        file_out << ", ";
      if (!use_classes) {
        file_out << "\"" << box_plots[i].box_plot_data_filename
                 << "\" using 1:2:3:5:6 with candlesticks ls 1 notitle "
                    "whiskerbars, ";
        file_out << "\"" << box_plots[i].box_plot_data_filename
                 << "\" using 1:4:4:4:4 with candlesticks ls 2 notitle";
      } else {
        file_out << "\"" << box_plots[i].box_plot_data_filename
                 << "\" using 7:2:3:5:6:xticlabels(1) with candlesticks ls 1 "
                    "notitle whiskerbars, ";
        file_out << "\"" << box_plots[i].box_plot_data_filename
                 << "\" using 7:4:4:4:4 with candlesticks ls 2 notitle";
      }
      if (box_plots[i].outliers.size() > 0)
        file_out << ", \"" << box_plots[i].outliers_data_filename
                 << "\" with points ls 3";
    }
  } else {
    for (int i = 0; i < dbs.size(); i++) {
      write_out_scatter(dbs[i].first[0], dbs[i].first[1], file_out,
                        dbs[i].second, nf, graphic, allinone, range);
    }
  }
  file_out << endl;
  file_out.close();
  system("gnuplot -persist st_shell_gplot_script.scr");
  if (box && clean_temp) {
    for (int i = 0; i < box_plots.size(); i++) {
      string command = "rm " + box_plots[i].box_plot_data_filename;
      system(command.c_str());
      command = "rm " + box_plots[i].outliers_data_filename;
      system(command.c_str());
    }
  }
  if (wd && file != "") {
    chdir(root_path.c_str());
  }
  return true;
}

extern bool shell_command(string command);
string st_parse_command_get_arg_string(st_map *parameter_list);

bool st_com_read_vector(ifstream &input, vector<double> &x,
                        int num_of_elements) {
  x.clear();

  if (input.eof() || !input.good())
    return false;

  int n = 0;
  while (!input.eof() && input.good() && n < num_of_elements) {
    double delta;
    input >> delta;
    x.push_back(delta);
    n++;
  }
  if (n < num_of_elements)
    return false;

  return true;
}

#define cut(sa, sz) ((sa.size() > sz) ? (sa.substr(0, sz - 2) + "..") : sa)

extern void st_printf_tabbed(string a, int maxsize);
extern void st_printf_tabbed_rev(string a, int maxsize);

string st_com_print_perc(double data) {
  ostringstream temp;
  temp << fixed << setprecision(1) << data * 100 << " %";
  return temp.str();
}

#define PRINT_CONF_SIZE 30
#define PRINT_GAP 2
#define PRINT_METRIC_SIZE 10
#define PRINT_STATS_SIZE 10

static st_table tb;

bool prs_compute_corr(st_map *optarg) {
  string td = tmp_directory();

  if (!st_overwrite_directory(td))
    return false;

  string database = st_parse_command_get_arg_string(optarg);
  if (current_environment.get_database(database)) {
    current_environment.get_database(database)->write_octave(
        (td + "/octave.txt").c_str());
  } else
    return false;

  string filename = td + "/oscript";
  ofstream fout(filename.c_str(), ios::out);

  if (fout.fail())
    return false;

  fout << "m = load('" << td + "/octave.txt"
       << "')" << endl;
  fout << "r = cor(m)" << endl;
  fout << "save('-ascii', '" << td << "/res.txt', 'r')" << endl;

  fout.close();

  if (!shell_command("octave < " + td + "/oscript")) {
    prs_display_error("Octave not found.");
    return false;
  }

  if (!shell_command("sed 's/NaN/0/g' < " + td + "/res.txt > " + td +
                     "/res1.txt")) {
    prs_display_error("Unable to parse the file.");
    return false;
  }

  int par = current_environment.current_design_space->ds_parameters.size();
  int obj = current_environment.optimization_objectives.size();

  ifstream fin((td + "/res1.txt").c_str(), ios::in);

  vector<double> vett;

  int confcolsize;
  int metriccolsize;

  if (!current_environment.shell_variables.get_integer(
          "db_report_conf_col_size", confcolsize)) {
    confcolsize = PRINT_CONF_SIZE;
    prs_display_message_n_value_m(
        "Assuming db_report_conf_col_size = ", confcolsize, "");
  }

  if (!current_environment.shell_variables.get_integer(
          "db_report_metric_col_size", metriccolsize)) {
    metriccolsize = PRINT_METRIC_SIZE;
    prs_display_message_n_value_m(
        "Assuming db_report_metric_col_size = ", metriccolsize, "");
  }

  tb.clear();
  tb.set_header("Correlation Report");

  tb.add_column("Parameters/Objectives", confcolsize, TYPE_STRING, JUST_CENTER);

  for (int j = 0; j < obj; j++) {
    tb.add_column(current_environment.optimization_objectives[j]->name,
                  metriccolsize, TYPE_STRING, JUST_RIGHT);
  }
  tb.print_header();

  for (int i = 0; i < par; i++) {
    st_com_read_vector(fin, vett, par + obj);

    tb.begin_row();

    tb.add_column_string(
        current_environment.current_design_space->ds_parameters_names[i]);

    for (int j = 0; j < obj; j++) {
      tb.add_column_string(st_com_print_perc(vett[par + j]) + " ");
    }
    tb.end_row();
  }
  tb.finish_header();

  fin.close();
  st_delete_directory(td);

  return true;
}

bool prs_gen_html_report(st_map *optarg) {
  st_object *db = st_parse_command_get_arg_object(optarg);
  string db_name;
  string m3exp_path;

  if (to<st_string *>(db)) {
    db_name = to<st_string *>(db)->get_string();
  } else
    return false;

  string rn = st_parse_command_get_opt_string(optarg, "name");
  bool objectives = st_parse_command_get_opt_integer(optarg, "objectives") > 0;

  if (rn == "") {
    prs_display_error("Please specify a report name with --name");
    return false;
  }

  if (!current_environment.current_design_space)
    return false;

  if (current_environment.shell_variables.get_string("current_build_path",
                                                     m3exp_path)) {
    string td = tmp_directory();

    if (!st_overwrite_directory(td))
      return false;

    st_design_space *newds = current_environment.current_design_space->copy();

    if (objectives)
      newds->convert_objectives_into_metrics(&current_environment);

    newds->write_to_file(&current_environment, (td + "/design_space.xml"));

    ofstream file_out((td + "/tmp_rep.scr").c_str(), ios::out);

    if (current_environment.get_database(db_name)) {
      if (current_environment.get_database(db_name)->count_points() == 0) {
        prs_display_error("Can't generate HTML report - Empty database");
        return false;
      }
      if (objectives)
        current_environment.get_database(db_name)->write_to_m3_file_objectives(
            (td + "/tmp_m3.db").c_str());
      else
        current_environment.get_database(db_name)->write_to_m3_file(
            (td + "/tmp_m3.db").c_str());
    } else
      return false;

    file_out << "set clean_directory_on_exit = \"true\"" << endl;
    file_out << "db_read \"" + td + "/tmp_m3.db\"" << endl;
    file_out << "set objectives = { ";

    if (!objectives) {
      for (int i = 0;
           i < current_environment.current_design_space->metric_names.size();
           i++) {
        file_out
            << "\"" +
                   current_environment.current_design_space->metric_names[i] +
                   "\" ";
      }
    } else {
      vector<st_objective *>::iterator k;
      for (k = current_environment.optimization_objectives.begin();
           k != current_environment.optimization_objectives.end(); k++) {
        file_out << "\"" + (*k)->name + +"\" ";
      }
    }
    file_out << " } " << endl;

    file_out << "set architecture_info = { \"MOST - Report\" }" << endl;
    file_out << "db_report_html \"" + td + "/report\"" << endl;
    file_out.close();

    m3exp_path = m3exp_path + "/tools/m3explorer/bin";

    string command = m3exp_path + "/m3explorer -x " + td + "/design_space.xml" +
                     " -f " + td + "/tmp_rep.scr";
    cout << command << endl;
    if (!shell_command(command)) {
      // st_delete_directory(td);
      prs_display_error("Command failed.");
      return false;
    }
    command = "cp -R " + td + "/report " + rn;
    if (!shell_command(command)) {
      // st_delete_directory(td);
      prs_display_error("Command failed.");
      return false;
    }
    // st_delete_directory(td);
    prs_display_message("Report generated.");
    return true;
  } else
    prs_display_error("Please define an XML driver.");

  return false;
}

bool prs_command_plot(st_map *metrics) {

  string graphic = st_parse_command_get_opt_string(metrics, "graphic");
  string plevel = st_parse_command_get_opt_string(metrics, "plevel");
  string effect = st_parse_command_get_opt_string(metrics, "effect");
  string override = st_parse_command_get_opt_string(metrics, "override");
  int relative = st_parse_command_get_opt_integer(metrics, "relative");

  int clusters = st_parse_command_get_opt_integer(metrics, "clusters");
  int wd = st_parse_command_get_opt_integer(metrics, "use_working_dir");
  int allinone = st_parse_command_get_opt_integer(metrics, "onepage");
  int bubble = st_parse_command_get_opt_integer(metrics, "bubble");

  string title = st_parse_command_get_opt_string(metrics, "title");

  int color_violating =
      st_parse_command_get_opt_integer(metrics, "color_violating");

  st_object *databases = st_parse_command_get_arg_object(metrics);
  vector<string> db_names;
  if (to<st_string *>(databases)) {
    if (current_environment.get_database(
            to<st_string *>(databases)->get_string()))
      db_names.push_back(to<st_string *>(databases)->get_string());
  } else {
    st_list *l = to<st_list *>(databases);
    if (!l) {
      prs_display_error("Please specify a valid list of databases");
      return 0;
    }
    list<st_object *>::iterator i;
    for (i = l->begin(); i != l->end(); i++) {
      if (to<st_string *>(*i)) {
        if (current_environment.get_database(to<st_string *>(*i)->get_string()))
          db_names.push_back(to<st_string *>(*i)->get_string());
      }
    }
  }

  if (!current_environment.current_driver) {
    prs_display_error("Please load a driver before plotting");
    return 0;
  }

  if (db_names.size() == 0) {
    prs_display_error("Please specify a valid list of databases");
    return 0;
  }

  string file = st_parse_command_get_opt_string(metrics, "output");
  string root_path;
  if (wd && file != "") {
    char *root_path_c = getcwd(NULL, 0);
    root_path = root_path_c;
    free(root_path_c);

    string cmd = "mkdir -p ./" + file;
    system(cmd.c_str());
    chdir(("./" + file).c_str());
  }

  ofstream file_out("st_shell_gplot_script.scr", ios::out);
  file_out << "unset grid" << endl;

  string ix = st_parse_command_get_opt_string(metrics, "xaxis");
  string iy = st_parse_command_get_opt_string(metrics, "yaxis");
  string iz = st_parse_command_get_opt_string(metrics, "zaxis");
  string key = st_parse_command_get_opt_string(metrics, "key");
  int monochrome = st_parse_command_get_opt_integer(metrics, "monochrome");
  if (key == "")
    key = "right";
  bool dummy = false;
  if (file != "") {
    if (!monochrome) {
      file_out << "set terminal pdf color" << endl;
    } else {
      file_out << "set terminal pdf monochrome" << endl;
    }
    file_out << "set output \"" << file << ".pdf\"" << endl;
  }
  file_out << "set key " << key << endl;
  if (iz != "") {
    file_out << "set xlabel \"" << ix << "\"" << endl;
    file_out << "set ylabel \"" << iy << "\"" << endl;
    file_out << "set zlabel \"" << iz << "\"" << endl;
    file_out << "set title \"" << title << "\"" << endl;
    if (override != "") {
      file_out.close();
      if (!shell_command("cat " + override + " >> st_shell_gplot_script.scr")) {
        prs_display_error("Override file not found.");
      }
      file_out.open("st_shell_gplot_script.scr", ios::out | ios::app);
    }
    if (!clusters) {
      if (!color_violating) {
        if (plevel == "") {
          for (int i = 0; i < db_names.size(); i++) {
            current_environment.get_database(db_names[i])
                ->cache_update(&current_environment);
            prs_write_data_and_script(
                metrics, file_out,
                current_environment.get_database(db_names[i]), db_names[i],
                "splot", graphic, dummy, allinone, bubble);
          }
        } else {
          if (current_environment.current_design_space->is_scalarf(plevel)) {
            int minl = current_environment.current_design_space->get_scalar_min(
                &current_environment, plevel);
            int maxl = current_environment.current_design_space->get_scalar_max(
                &current_environment, plevel);
            for (int k = 0; k < db_names.size(); k++) {
              for (int i = minl; i <= maxl; i++) {
                st_database copy;
                int line_color;
                if (maxl != minl)
                  line_color =
                      ((double)(i - minl)) / ((double)(maxl - minl)) * 8.0 + 1;
                else
                  line_color = 1;
                copy = *current_environment.get_database(db_names[k]);
                copy.filter_parameter_level(&current_environment, plevel, i);
                string num = current_environment.current_design_space
                                 ->get_symbol_from_scalar_level(plevel, i);
                string filename =
                    db_names[k] + string("-") + plevel + "-" + num;
                prs_write_data_and_script(metrics, file_out, &copy, filename,
                                          "splot", graphic, dummy, allinone,
                                          bubble, true, line_color);
              }
            }
          } else {
            prs_display_error("The specified parameter is not a scalar.");
          }
        }
      } else {
        for (int i = 0; i < db_names.size(); i++) {
          st_database copy;
          copy = *current_environment.get_database(db_names[i]);
          copy.filter_points(&current_environment, true);
          prs_write_data_and_script(metrics, file_out, &copy,
                                    db_names[i] + "_violators", "splot",
                                    graphic, dummy, allinone, bubble);

          copy = *current_environment.get_database(db_names[i]);
          copy.filter_points(&current_environment, false);
          prs_write_data_and_script(metrics, file_out, &copy,
                                    db_names[i] + "_good", "splot", graphic,
                                    dummy, allinone, bubble);
        }
      }
    } else {
      for (int k = 0; k < db_names.size(); k++) {
        for (int i = 0; i < clusters; i++) {
          st_database copy;
          copy = *current_environment.get_database(db_names[k]);
          copy.filter_cluster(i);
          ostringstream str;
          str << i;
          string num = str.str();
          string filename = db_names[k] + string("_cluster_") + num;
          prs_write_data_and_script(metrics, file_out, &copy, filename, "splot",
                                    graphic, dummy, allinone, bubble);
        }
      }
    }
  } else {
    file_out << "set xlabel \"" << ix << "\"" << endl;
    file_out << "set ylabel \"" << iy << "\"" << endl;
    file_out << "set title \"" << title << "\"" << endl;
    if (override != "") {
      file_out.close();
      if (!shell_command("cat " + override + " >> st_shell_gplot_script.scr")) {
        prs_display_error("Override file not found.");
      }
      file_out.open("st_shell_gplot_script.scr", ios::out | ios::app);
    }
    if (!clusters) {
      if (!color_violating) {
        if (plevel == "") {
          for (int i = 0; i < db_names.size(); i++) {
            current_environment.get_database(db_names[i])
                ->cache_update(&current_environment);
            if (effect == "")
              prs_write_data_and_script(
                  metrics, file_out,
                  current_environment.get_database(db_names[i]), db_names[i],
                  "plot", graphic, dummy, allinone);
            else
              prs_write_data_and_script(
                  metrics, file_out,
                  current_environment.get_database(db_names[i]), db_names[i],
                  "plot", graphic, dummy, allinone, false, false, 0, true,
                  effect, relative);
          }
        } else {
          if (current_environment.current_design_space->is_scalarf(plevel)) {
            int minl = current_environment.current_design_space->get_scalar_min(
                &current_environment, plevel);
            int maxl = current_environment.current_design_space->get_scalar_max(
                &current_environment, plevel);
            for (int k = 0; k < db_names.size(); k++) {
              for (int i = minl; i <= maxl; i++) {
                st_database copy;
                int line_color;
                if (maxl != minl)
                  line_color =
                      ((double)(i - minl)) / ((double)(maxl - minl)) * 8.0 + 1;
                else
                  line_color = 1;
                copy = *current_environment.get_database(db_names[k]);
                copy.filter_parameter_level(&current_environment, plevel, i);
                string num = current_environment.current_design_space
                                 ->get_symbol_from_scalar_level(plevel, i);
                string filename =
                    db_names[k] + string("-") + plevel + "-" + num;
                prs_write_data_and_script(metrics, file_out, &copy, filename,
                                          "plot", graphic, dummy, allinone,
                                          false, true, line_color);
              }
            }
          } else {
            prs_display_error("The specified parameter is not a scalar.");
          }
        }
      } else {
        for (int i = 0; i < db_names.size(); i++) {
          st_database copy;
          copy = *current_environment.get_database(db_names[i]);
          copy.filter_points(&current_environment, true);
          prs_write_data_and_script(metrics, file_out, &copy,
                                    db_names[i] + "_violators", "plot", graphic,
                                    dummy, allinone);

          copy = *current_environment.get_database(db_names[i]);
          copy.filter_points(&current_environment, false);
          prs_write_data_and_script(metrics, file_out, &copy,
                                    db_names[i] + "_good", "plot", graphic,
                                    dummy, allinone);
        }
      }
    } else {
      for (int k = 0; k < db_names.size(); k++) {
        for (int i = 0; i < clusters; i++) {
          st_database copy;
          copy = *current_environment.get_database(db_names[k]);
          copy.filter_cluster(i);
          ostringstream str;
          str << i;
          string num = str.str();
          string filename = db_names[k] + string("_cluster_") + num;
          prs_write_data_and_script(metrics, file_out, &copy, filename, "plot",
                                    graphic, dummy, allinone);
        }
      }
    }
  }
  if (allinone) {
    file_out << endl;
  }
  file_out.close();
  if (dummy) {
    if (file == "")
      system("gnuplot -persist st_shell_gplot_script.scr");
    else
      system("gnuplot -persist st_shell_gplot_script.scr");
  }
  if (wd && file != "") {
    chdir(root_path.c_str());
  }
  return true;
}

extern void st_init_rsm_command_help();

void st_init_parse_command() {
  st_init_commands();
  st_init_rsm_command_help();
  st_setup_command_line_completion();
}

string st_parse_command_get_opt_string(st_map *parameter_list, string n) {
  const st_object *a;
  if (!parameter_list->get(n, a)) {
    return "";
  }
  const st_string *p = to<const st_string *>(a);
  if (p == NULL) {
    return "";
  } else
    return p->get_string();
}

string st_parse_command_get_arg_string(st_map *parameter_list) {
  return st_parse_command_get_opt_string(parameter_list, "");
}

int st_parse_command_get_arg_integer(st_map *parameter_list) {
  string n = "";
  const st_object *a;
  if (!parameter_list->get(n, a)) {
    return 0;
  }
  const st_integer *p = to<const st_integer *>(a);
  if (p == NULL) {
    return 0;
  } else
    return p->get_integer();
  return 0;
}

int st_parse_command_get_opt_integer(st_map *parameter_list, string n) {
  const st_object *a;
  if (!parameter_list->get(n, a)) {
    return 0;
  }
  const st_integer *p = to<const st_integer *>(a);
  if (p == NULL) {
    return 0;
  } else
    return p->get_integer();
  return 0;
}

double st_parse_command_get_opt_double(st_map *parameter_list, string n,
                                       double def) {
  const st_object *a;
  if (!parameter_list->get(n, a)) {
    return def;
  }
  const st_double *p = to<const st_double *>(a);
  if (p == NULL) {
    return def;
  } else
    return p->get_double();

  return def;
}

st_object *st_parse_command_get_opt_object(st_map *parameter_list, string n) {
  const st_object *a;
  if (!parameter_list->get(n, a)) {
    return NULL;
  }
  return a->gen_copy();
}

st_object *st_parse_command_get_arg_object(st_map *parameter_list) {
  string n = "";
  const st_object *a;
  if (!parameter_list->get(n, a)) {
    return NULL;
  }
  return a->gen_copy();
}

string st_get_ro_database_name(int n) {
  ostringstream ssuffix;
  ssuffix << "db_ro" << n;
  string suffix = ssuffix.str();
  return suffix;
}

extern st_command *rsm_command_help(string rsm_name);

void st_parse_command(__PRS_CMDN__, __PRS_ARGS__) {

  __PRS_COMMAND_ACTION("help") {
    if (__PRS_NO_ARGS) {
      map<string, st_command>::iterator i;
      for (i = st_commands.begin(); i != st_commands.end(); i++) {
        i->second.gen_short_man(i->first);
      }
      cout << endl;
      __PRS_RETURN_SUCCESS;
    } else {
      string cmd = __PRS_GET_ARG_S;
      string opt = __PRS_GET_OPT_S("model");
      if (st_commands.count(cmd)) {
        if (cmd != "db_train_rsm" || opt == "")
          st_commands[cmd].gen_man(cmd);
        else {
          st_command *rsm = rsm_command_help(opt);
          if (!rsm) {
            prs_display_error("Invalid RSM specified");
            __PRS_RETURN_FAIL;
          } else {
            rsm->gen_man("db_train_rsm");
            __PRS_RETURN_SUCCESS;
          }
        }

      } else {
        st_command *cm = st_get_help_from_module("lib" + cmd + ".so");
        if (cm) {
          cm->gen_man(cmd);
          delete cm;
          __PRS_RETURN_SUCCESS;
        } else {
          prs_display_error("Unknown command specified.");
          __PRS_RETURN_FAIL;
        }
      }
      cout << endl;
    }
    __PRS_RETURN_FAIL;
  }

  __PRS_COMMAND_ACTION("read_script") {
    if (prs_current_parser_session_is_interactive()) {
      prs_display_error("Cannot use read_script in interactive mode");
      __PRS_RETURN_FAIL;
    }
    if (__PRS_NO_ARGS) {
      prs_display_error("Please specify a script name");
      __PRS_RETURN_FAIL;
    } else {
      string par = __PRS_GET_ARG_S;
      bool v = prs_command_read(par, false);
      __PRS_RETURN_SUCCESS_V(v);
    }
    __PRS_RETURN_FAIL;
  }

  __PRS_COMMAND_ACTION("read_object") {
    if (prs_current_parser_session_is_interactive()) {
      prs_display_error("Cannot use read_object in interactive mode");
      __PRS_RETURN_FAIL;
    }
    if (__PRS_NO_ARGS) {
      prs_display_error("Please specify a file name");
      __PRS_RETURN_FAIL;
    } else {
      string par = __PRS_GET_ARG_S;
      bool v = prs_command_read(par, true);
      __PRS_RETURN_SUCCESS_V(v);
    }
  }

  __PRS_COMMAND_ACTION("db_read") {
    if (__PRS_NO_ARGS) {
      prs_display_error("Please specify a database name");
      __PRS_RETURN_FAIL;
    } else {
      string destination = __PRS_GET_OPT_S("destination");
      string par = __PRS_GET_ARG_S;
      bool result = prs_command_db_read(par, destination);

      __PRS_RETURN_SUCCESS_V(result);
    }
  }

  __PRS_COMMAND_ACTION("db_write") {
    if (__PRS_NO_ARGS) {
      prs_display_error("Please specify a database name");
      __PRS_RETURN_FAIL;
    } else {
      string database = __PRS_GET_ARG_S;
      string filename = __PRS_GET_OPT_S("file_name");

      bool result = prs_command_db_write(filename, database);

      __PRS_RETURN_SUCCESS_V(result);
    }
  }

  __PRS_COMMAND_ACTION("write_object") {
    if (__PRS_NO_ARGS) {
      prs_display_error(
          "Please specify a valid expression and a destination file");
      __PRS_RETURN_FAIL;
    } else {
      string filename = __PRS_GET_OPT_S("file_name");
      st_object *object = __PRS_GET_ARG_O;

      bool result = prs_command_write_object(filename, object);
      __PRS_RETURN_SUCCESS_V(result);
    }
  }

  __PRS_COMMAND_ACTION("opt_update_cache") {
    current_environment.optimization_timestamp++;
    __PRS_RETURN_SUCCESS;
  }

  __PRS_COMMAND_ACTION("opt_report_size") {
    current_environment.current_design_space->lazy_compute_size(
        &current_environment);
    __PRS_RETURN_SUCCESS;
  }

  __PRS_COMMAND_ACTION("opt_remove_objective") {
    if (__PRS_NO_ARGS) {
      current_environment.clear_objectives();
      current_environment.shell_variables.set_integer(
          "objectives_number",
          current_environment.optimization_objectives.size());

      __PRS_RETURN_SUCCESS;
    } else {
      string par = __PRS_GET_ARG_S;
      if (!current_environment.remove_objective(par)) {
        prs_display_error("Objective not found.");

        __PRS_RETURN_FAIL;
      } else {
        prs_display_message("Objective removed.");
        current_environment.shell_variables.set_integer(
            "objectives_number",
            current_environment.optimization_objectives.size());

        __PRS_RETURN_SUCCESS;
      }
    }
  }

  __PRS_COMMAND_ACTION("opt_remove_constraint") {
    if (__PRS_NO_ARGS) {
      current_environment.clear_constraints();

      __PRS_RETURN_SUCCESS;
    } else {
      string par = __PRS_GET_ARG_S;
      if (!current_environment.remove_constraint(par)) {
        prs_display_error("Constraint not found.");

        __PRS_RETURN_FAIL;
      } else {
        prs_display_message("Constraint removed.");

        __PRS_RETURN_SUCCESS;
      }
    }
  }

  __PRS_COMMAND_ACTION("db_create") {
    if (__PRS_NO_ARGS) {
      prs_display_error("Please specify a database name");

      __PRS_RETURN_FAIL;
    } else {
      string par = __PRS_GET_ARG_S;
      if (current_environment.available_dbs.count(par)) {
        prs_display_error(
            "Database already exists. Please specify another name");

        __PRS_RETURN_FAIL;
      } else {
        prs_display_message("Created database " + par);
        current_environment.available_dbs[par] = new st_database;

        __PRS_RETURN_SUCCESS;
      }
    }
  }

  __PRS_COMMAND_ACTION("opt_tune") {
    string destdb = __PRS_GET_ARG_S;
    string source = __PRS_GET_OPT_S("source");

    if (!current_environment.change_current_destination_db_to(destdb)) {
      prs_display_error(
          "Please specify a destination database for the optimization results");
      __PRS_RETURN_FAIL;
    }
    if (!current_environment.available_dbs.count(source)) {
      current_environment.source_database =
          current_environment.available_dbs["root"];
    } else {
      current_environment.source_database =
          current_environment.available_dbs[source];
    }

    if (st_mpi_environment_active()) {
      st_mpi_broadcast_send_command("SYNCHRONIZE_SHELL_VARIABLES");
      st_mpi_broadcast_send_data(current_environment.shell_variables.print());
    }

    try {
      int res = prs_command_tune();
      __PRS_RETURN_SUCCESS_V(res);
    } catch (exception &e) {
      prs_display_error(string("Problems in the optimization phase: ") +
                        e.what());
      __PRS_RETURN_FAIL;
    }
  }

  __PRS_COMMAND_ACTION("db_report") {
    string database = __PRS_GET_ARG_S;

    if (!current_environment.get_database(database)) {
      prs_display_error("Please specify a valid database");
      __PRS_RETURN_FAIL;
    }

    bool only_size = __PRS_GET_OPT_I("only_size");
    bool show_clust = __PRS_GET_OPT_I("show_cluster");
    bool only_objectives_n_constraint = __PRS_GET_OPT_I("only_objectives");
    bool show_rpath = __PRS_GET_OPT_I("show_path");
    bool describe = __PRS_GET_OPT_I("describe");

    if (!only_size) {
      try {
        prs_display_message("Database " + database + " content follows");
        current_environment.get_database(database)->report(
            &current_environment, only_objectives_n_constraint, show_clust,
            show_rpath, describe);
        __PRS_RETURN_SUCCESS;
      } catch (exception &e) {
        prs_display_error(e.what());
        __PRS_RETURN_FAIL;
      }
    } else {
      int size = current_environment.get_database(database)->count_points();
      prs_display_message_n_value_m("Current database has ", size, " points");

      __PRS_RETURN_SUCCESS_V(size);
    }
  }

  __PRS_COMMAND_ACTION("db_clear") {
    string database = __PRS_GET_ARG_S;

    if (!current_environment.get_database(database)) {
      prs_display_error("Please specify a valid database");

      __PRS_RETURN_FAIL;
    }

    prs_display_message("Clearing database " + database);
    current_environment.get_database(database)->clear();

    __PRS_RETURN_SUCCESS;
  }

  __PRS_COMMAND_ACTION("db_train_rsm") {
    string destdb = __PRS_GET_ARG_S;

    if (!current_environment.change_current_destination_db_to(destdb)) {
      prs_display_error(
          "Please specify a destination database for the rsm results");
      __PRS_RETURN_FAIL;
    }

    if (!current_environment.current_driver) {
      prs_display_error("Please define the driver before training an RSM");
      __PRS_RETURN_FAIL;
    }

    try {
      prs_command_train_rsm(__PRS_ARGS_N);
      __PRS_RETURN_SUCCESS;
    } catch (exception &e) {
      prs_display_error(e.what());
      __PRS_RETURN_FAIL;
    }
  }

  __PRS_COMMAND_ACTION("show_vars") {
    prs_command_show_variables();
    __PRS_RETURN_SUCCESS;
  }

  __PRS_COMMAND_ACTION("opt_load_optimizer") {
    if (__PRS_NO_ARGS) {
      prs_display_error("Please specify an optimizer name");
      __PRS_RETURN_FAIL;
    } else {
      string par = __PRS_GET_ARG_S;
      bool result = prs_command_load_optimizer(par);
      __PRS_RETURN_SUCCESS_V(result);
    }
  }

  __PRS_COMMAND_ACTION("doe_load_doe") {
    if (__PRS_NO_ARGS) {
      prs_display_error("Please specify a valid DoE name");
      __PRS_RETURN_FAIL;
    } else {
      string par = __PRS_GET_ARG_S;
      bool result = prs_command_load_doe(par);
      __PRS_RETURN_SUCCESS_V(result);
    }
  }

  __PRS_COMMAND_ACTION("drv_write_xml") {
    if (__PRS_NO_ARGS) {
      prs_display_error("Please specify a filename");
      __PRS_RETURN_FAIL;
    } else {
      string par = __PRS_GET_ARG_S;

      if (current_environment.current_design_space->write_to_file(
              &current_environment, par)) {
        __PRS_RETURN_SUCCESS;
      } else {
        __PRS_RETURN_FAIL;
      }
    }
  }

  __PRS_COMMAND_ACTION("drv_load_driver") {
    if (__PRS_NO_ARGS) {
      prs_display_error("Please specify a driver name");
      __PRS_RETURN_FAIL;
    } else {
      string par = __PRS_GET_ARG_S;

      if (prs_command_activate_driver(par)) {
        __PRS_RETURN_SUCCESS;
      } else {
        __PRS_RETURN_FAIL;
      }
    }
  }

  /*
  __PRS_COMMAND_ACTION ("drv_recenter_design_space")
  {
      if(__PRS_NO_ARGS)
      {
         prs_display_error("Please specify at least a point in the design
  space");
         __PRS_RETURN_FAIL;
      }
      else
      {
          if(prs_command_recenter_design_space(parameter_list))
              __PRS_RETURN_SUCCESS;
          else
              __PRS_RETURN_FAIL;
      }
      return;
  }
  */

  __PRS_COMMAND_ACTION("db_filter_pareto") {
    bool result = prs_command_filter_pareto(__PRS_ARGS_N);
    __PRS_RETURN_SUCCESS_V(result);
  }

  __PRS_COMMAND_ACTION("db_filter_valid") {
    string database = __PRS_GET_ARG_S;
    bool violating = __PRS_GET_OPT_I("violating");
    if (!current_environment.get_database(database)) {
      prs_display_error("Please specify a valid database");
      __PRS_RETURN_FAIL;
    }
    current_environment.get_database(database)->filter_valid(violating);
    __PRS_RETURN_SUCCESS;
  }

  __PRS_COMMAND_ACTION("db_filter_cluster") {
    string database = __PRS_GET_ARG_S;
    if (!current_environment.get_database(database)) {
      prs_display_error("Please specify a valid database");
      __PRS_RETURN_FAIL;
    }

    if (__PRS_NO_ARGS) {
      prs_display_error("Please specify a valid database");
      __PRS_RETURN_FAIL;
    }

    int cluster = __PRS_GET_OPT_I("cluster");
    current_environment.get_database(database)->filter_cluster(cluster);

    __PRS_RETURN_SUCCESS;
  }

  __PRS_COMMAND_ACTION("db_compute_kmeans_clusters") {
    bool res = prs_command_compute_kmeans_clusters(__PRS_ARGS_N);

    __PRS_RETURN_SUCCESS_V(res);
  }

  __PRS_COMMAND_ACTION("db_report_html") {
    if (__PRS_NO_ARGS) {
      prs_display_error("Please specify a database name");
      __PRS_RETURN_FAIL;
    }

    /* Check for HTML */
    if (prs_gen_html_report(__PRS_ARGS_N)) {
      __PRS_RETURN_SUCCESS;
    } else {
      __PRS_RETURN_FAIL;
    }
  }

  __PRS_COMMAND_ACTION("db_plot") {
    if (__PRS_NO_ARGS) {
      prs_display_error("Please specify a least one metric name");
      __PRS_RETURN_FAIL;
    } else {
      try {
        if (prs_command_plot((__PRS_ARGS_N))) {
          __PRS_RETURN_SUCCESS;
        } else {
          __PRS_RETURN_FAIL;
        }
      } catch (exception &e) {
        prs_display_error("Failed to compute objectives.");
        __PRS_RETURN_FAIL;
      }
    }
  }

  __PRS_COMMAND_ACTION("db_plot_vector") {
    if (__PRS_NO_ARGS) {
      prs_display_error("Please specify a least one metric name");
      __PRS_RETURN_FAIL;
    } else {
      try {
        if (prs_command_plot_vector((__PRS_ARGS_N))) {
          __PRS_RETURN_SUCCESS;
        } else {
          __PRS_RETURN_FAIL;
        }
      } catch (exception &e) {
        prs_display_error("Failed to compute objectives.");
        __PRS_RETURN_FAIL;
      }
    }
  }

  __PRS_COMMAND_ACTION("db_normalize") {
    if (__PRS_ARGS_N->size() < 2) {
      prs_display_error("Please specify a least two database names ");
      __PRS_RETURN_FAIL;
    } else {
      if (!current_environment.available_dbs.count(__PRS_GET_ARG_S) ||
          !current_environment.available_dbs.count(__PRS_GET_OPT_S("with"))) {
        prs_display_error("Please specify a least two valid database names ");
        __PRS_RETURN_FAIL;
      }

      st_database *a = current_environment.available_dbs[__PRS_GET_ARG_S];
      st_database *b =
          current_environment.available_dbs[__PRS_GET_OPT_S("with")];

      if (!a || !b) {
        prs_display_error("Please specify a least two valid database names ");
        __PRS_RETURN_FAIL;
      }

      int valid = __PRS_GET_OPT_I("valid");

      sim_normalize_databases(&current_environment, b, a, valid);

      __PRS_RETURN_SUCCESS;
    }
  }

  try {

    __PRS_COMMAND_ACTION("db_compute_coverage") {
      if (__PRS_ARGS_N->size() < 2) {
        prs_display_error("Please specify a least two database names ");
        __PRS_RETURN_FAIL;
      } else {
        if (!current_environment.available_dbs.count(__PRS_GET_ARG_S) ||
            !current_environment.available_dbs.count(
                __PRS_GET_OPT_S("reference"))) {
          prs_display_error("Please specify a least two valid database names ");
          __PRS_RETURN_FAIL;
        }

        if (__PRS_GET_OPT_I("count")) {
          int c = sim_compute_how_many_points_in_A_are_in_B(
              &current_environment,
              current_environment.available_dbs[__PRS_GET_ARG_S],
              current_environment.available_dbs[__PRS_GET_OPT_S("reference")]);

          cout << __PRS_GET_ARG_S << " has " << c << " designs in ";
          cout << __PRS_GET_OPT_S("reference") << endl;

          __PRS_RETURN_SUCCESS_V(c);

        } else {
          bool verbose = (bool)__PRS_GET_OPT_I("verbose");

          double c = sim_compute_two_set_coverage_metric(
              &current_environment,
              current_environment.available_dbs[__PRS_GET_ARG_S],
              current_environment.available_dbs[__PRS_GET_OPT_S("reference")],
              verbose);

          cout << __PRS_GET_ARG_S << " is covered by ";
          cout << __PRS_GET_OPT_S("reference") << " at " << c * 100
               << " percent " << endl;

          __PRS_RETURN_SUCCESS_D(c * 100);
        }
      }
    }

    __PRS_COMMAND_ACTION("db_compute_ADRS") {
      if (__PRS_ARGS_N->size() < 2) {
        prs_display_error("Please specify a least two database names ");
        __PRS_RETURN_FAIL;
      } else {
        if (!current_environment.available_dbs.count(__PRS_GET_ARG_S) ||
            !current_environment.available_dbs.count(
                __PRS_GET_OPT_S("reference"))) {
          prs_display_error("Please specify a least two valid database names ");
          __PRS_RETURN_FAIL;
        }

        double c = sim_compute_ADRS(
            &current_environment,
            current_environment.available_dbs[__PRS_GET_ARG_S],
            current_environment.available_dbs[__PRS_GET_OPT_S("reference")]);

        cout << __PRS_GET_ARG_S << " is distant from  ";
        cout << __PRS_GET_OPT_S("reference") << " at " << c << endl;

        __PRS_RETURN_SUCCESS_D(c);
      }
    }

    __PRS_COMMAND_ACTION("db_compute_distance") {
      if (__PRS_ARGS_N->size() < 2) {
        prs_display_error("Please specify a least two database names ");
        __PRS_RETURN_FAIL;
      } else {
        if (!current_environment.available_dbs.count(__PRS_GET_ARG_S) ||
            !current_environment.available_dbs.count(
                __PRS_GET_OPT_S("reference"))) {
          prs_display_error("Please specify a least two valid database names ");
          __PRS_RETURN_FAIL;
        }

        double c = sim_compute_avg_euclidean_distance(
            &current_environment,
            current_environment.available_dbs[__PRS_GET_ARG_S],
            current_environment.available_dbs[__PRS_GET_OPT_S("reference")]);

        cout << __PRS_GET_ARG_S << " is distant from  ";
        cout << __PRS_GET_OPT_S("reference") << " at " << c << endl;

        __PRS_RETURN_SUCCESS_D(c);
      }
      return;
    }

    __PRS_COMMAND_ACTION("db_compute_median_distance") {
      if (__PRS_ARGS_N->size() < 2) {
        prs_display_error("Please specify a least two database names ");
        __PRS_RETURN_FAIL;
      } else {
        if (!current_environment.available_dbs.count(__PRS_GET_ARG_S) ||
            !current_environment.available_dbs.count(
                __PRS_GET_OPT_S("reference"))) {
          prs_display_error("Please specify a least two valid database names ");
          __PRS_RETURN_FAIL;
        }

        double c = sim_compute_median_euclidean_distance(
            &current_environment,
            current_environment.available_dbs[__PRS_GET_ARG_S],
            current_environment.available_dbs[__PRS_GET_OPT_S("reference")]);

        cout << __PRS_GET_ARG_S << " is distant from  ";
        cout << __PRS_GET_OPT_S("reference") << " at " << c << endl;

        __PRS_RETURN_SUCCESS_D(c);
      }
    }
  } catch (exception &e) {
    prs_display_error(e.what());
    __PRS_RETURN_FAIL;
  }

  __PRS_COMMAND_ACTION("db_copy") {
    if (__PRS_ARGS_N->size() < 2) {
      prs_display_error("Please specify a least two database names to copy");
      __PRS_RETURN_FAIL;
    } else {
      string destination = __PRS_GET_OPT_S("destination");
      string src = __PRS_GET_ARG_S;

      if (destination == "") {
        prs_display_error("Please specify a valid destination database name");
        __PRS_RETURN_FAIL;
      }

      if (!current_environment.available_dbs.count(src)) {
        prs_display_error(
            "Please specify a valid source database name to copy");
        __PRS_RETURN_FAIL;
      }

      if (!current_environment.available_dbs.count(destination)) {
        current_environment.insert_new_database(new st_database(), destination);
      }

      st_database *source = current_environment.available_dbs[src];
      st_database *dst = current_environment.available_dbs[destination];

      prs_display_message("Copying database " + src + " into " + destination);

      if (__PRS_GET_OPT_I("attach"))
        dst->attach(*source);
      else
        *dst = *source;

      __PRS_RETURN_SUCCESS;
    }
  }

  __PRS_COMMAND_ACTION("db_insert_point") {
    string db = __PRS_GET_OPT_S("destination");
    st_object *point = __PRS_GET_ARG_O;

    if (!prs_command_db_insert_point(point, db)) {
      __PRS_RETURN_FAIL;
    } else {
      __PRS_RETURN_SUCCESS;
    }
  }

  __PRS_COMMAND_ACTION("db_export_xml") {
    if (__PRS_NO_ARGS) {
      prs_display_error("Please specify the file name");
      __PRS_RETURN_FAIL;
    } else {
      if (prs_command_db_export_xml(__PRS_ARGS_N)) {
        __PRS_RETURN_SUCCESS;
      } else {
        __PRS_RETURN_FAIL;
      }
    }
  }

  __PRS_COMMAND_ACTION("db_export") {
    if (__PRS_NO_ARGS) {
      prs_display_error("Please specify the file name");
      __PRS_RETURN_FAIL;
    } else {
      if (prs_command_db_export(__PRS_ARGS_N)) {
        __PRS_RETURN_SUCCESS;
      } else {
        __PRS_RETURN_FAIL;
      }
    }
    return;
  }

  __PRS_COMMAND_ACTION("db_compute_corr") {
    if (__PRS_NO_ARGS) {
      prs_display_error("Please specify a database name");
      __PRS_RETURN_FAIL;
    } else {
      if (prs_compute_corr(__PRS_ARGS_N)) {
        __PRS_RETURN_SUCCESS;
      } else {
        __PRS_RETURN_FAIL;
      }
    }
    return;
  }

  __PRS_COMMAND_ACTION("db_import") {
    if (__PRS_ARGS_N->size() < 2) {
      prs_display_error(
          "Please specify at least the destination database name and the "
          "option \"--file_name\". Optionally, you can specify the option "
          "\"--use_symbols\"");

      __PRS_RETURN_FAIL;
    } else {
      string database = __PRS_GET_ARG_S;
      string filename = __PRS_GET_OPT_S("file_name");
      bool use_symbols = __PRS_GET_OPT_I("use_symbols") > 0;

      int result = prs_command_db_import(filename, database, use_symbols);

      __PRS_RETURN_SUCCESS_V(result);
    }
  }

  __PRS_COMMAND_ACTION("opt_estimate") {
    st_object *point = __PRS_GET_ARG_O;
    if (!point) {
      prs_display_error("Please display a valid point");
      __PRS_RETURN_FAIL;
    }
    try {
      st_object *new_point = prs_command_opt_estimate(point);
      if (new_point)
        __PRS_RETURN_SUCCESS_OD(new_point);
    } catch (exception &e) {
      prs_display_error(e.what());
      __PRS_RETURN_FAIL;
    }
  }

  prs_display_error(
      ("Unknown command '" + __PRS_CMDN_N->get_string() + "'").c_str());
  __PRS_RETURN_FAIL;
}

#include <libxml/parser.h>
#include <libxml/xmlschemas.h>
#include <libxml/xpath.h>

bool prs_command_db_export_xml(st_map *parameters) {
  string file_name = st_parse_command_get_opt_string(parameters, "file_name");
  bool use_objectives =
      st_parse_command_get_opt_integer(parameters, "use_objectives");
  string db_name = st_parse_command_get_arg_string(parameters);

  if (current_environment.current_design_space == NULL) {
    prs_display_error("Design Space not defined, format unknown");
    return false;
  }

  st_database *cdb;

  if (!(cdb = current_environment.get_database(db_name))) {
    prs_display_error("Please specify a valid database");
    return false;
  }

  prs_display_message("Creating sorting criteria...");
  vector<st_objective *>::iterator k;

  /*

  sorting_metric_names.clear();

  for(k = current_environment.optimization_objectives.begin();
          k != current_environment.optimization_objectives.end();
          k++)
  {
      sorting_metric_names.push_back((*k)->name);
  }
  */

  prs_display_message("Saving the database in XML format..");

  xmlDocPtr output;
  xmlNodePtr root, xpoint, xparams, xparam, xitem, xmetrics, xmetric;
  string error;
  map<string, int>::iterator it;
  int i = 0, j;
  st_design_space *design_space = current_environment.current_design_space;

  xmlKeepBlanksDefault(1);
  xmlThrDefIndentTreeOutput(2);
  xmlThrDefTreeIndentString("\t");
  output = xmlNewDoc((xmlChar *)"1.0");
  root = xmlNewNode(NULL, (xmlChar *)"points");
  xmlNewProp(root, (xmlChar *)"xmlns", (xmlChar *)"http://www.multicube.eu/");
  xmlNewProp(root, (xmlChar *)"version", (xmlChar *)"1.3");
  xmlDocSetRootElement(output, root);

  // set<st_sortable_point> sorted_db;
  set<st_point> sorted_db;

  st_point_set::iterator ci;
  st_point_set *database_list = cdb->get_set_of_points();

  for (ci = database_list->begin(); ci != database_list->end(); ci++) {
    st_point &e = *(ci->second);
    if (e.check_consistency(&current_environment) && !e.get_error()) {
      sorted_db.insert(st_point(e));
    } else {
      prs_display_error("Omitting point due to consistency problems..");
    }
  }

  set<st_point>::iterator c;
  for (c = sorted_db.begin(); c != sorted_db.end(); c++) {
    st_point p = *c;
    xpoint = xmlNewChild(root, NULL, (xmlChar *)"point", NULL);
    xparams = xmlNewChild(xpoint, NULL, (xmlChar *)"parameters", NULL);
    xmetrics = xmlNewChild(xpoint, NULL, (xmlChar *)"system_metrics", NULL);

    for (it = design_space->ds_parameters_index.begin();
         it != design_space->ds_parameters_index.end(); it++) {
      xparam = xmlNewChild(xparams, NULL, (xmlChar *)"parameter", NULL);
      xmlNewProp(xparam, (xmlChar *)"name", (xmlChar *)it->first.c_str());

      int type = design_space->ds_parameters[it->second].type;
      if (type == ST_DS_PERMUTATION || type == ST_DS_ON_OFF_MASK) {
        vector<int> v;
        if (type == ST_DS_PERMUTATION)
          v = design_space->get_permutation(&current_environment, &p,
                                            it->first);
        else
          v = design_space->get_mask(&current_environment, &p, it->first);
        for (j = 0; j < v.size(); ++j) {
          xitem = xmlNewChild(xparam, NULL, (xmlChar *)"item", NULL);

          ostringstream temp;
          temp << j + 1;
          string jj = temp.str();
          ostringstream temp1;
          temp << v[j];
          string vj = temp1.str();
          xmlNewProp(xitem, (xmlChar *)"position", (xmlChar *)jj.c_str());
          xmlNewProp(xitem, (xmlChar *)"value", (xmlChar *)vj.c_str());
        }
      } else {
        string c_rep = design_space->get_parameter_representation(
            &current_environment, p, it->first);
        xmlNewProp(xparam, (xmlChar *)"value", (xmlChar *)c_rep.c_str());
      }
    }

    if (!use_objectives) {
      for (it = design_space->metric_index.begin();
           it != design_space->metric_index.end(); it++) {
        xmetric = xmlNewChild(xmetrics, NULL, (xmlChar *)"system_metric", NULL);
        xmlNewProp(xmetric, (xmlChar *)"name", (xmlChar *)it->first.c_str());

        char m_rep[32];
        sprintf(m_rep, "%f", p.get_metrics(it->second));
        xmlNewProp(xmetric, (xmlChar *)"value", (xmlChar *)m_rep);
      }
    } else {
      for (k = current_environment.optimization_objectives.begin();
           k != current_environment.optimization_objectives.end(); k++) {
        int index = current_environment.get_objective_index((*k)->name);

        xmetric = xmlNewChild(xmetrics, NULL, (xmlChar *)"system_metric", NULL);
        xmlNewProp(xmetric, (xmlChar *)"name", (xmlChar *)(*k)->name.c_str());

        char m_rep[32];
        sprintf(m_rep, "%f",
                current_environment.optimization_objectives[index]->eval(
                    &p, index));
        xmlNewProp(xmetric, (xmlChar *)"value", (xmlChar *)m_rep);
      }
    }
  }
  xmlSaveFormatFileEnc(file_name.c_str(), output, "UTF-8", 1);
  return true;
}
