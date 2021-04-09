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
#include <iostream>
#include <string>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <cmath>
#include <unistd.h>
#include <st_shell_variables.h>
#include <st_parser.h>
#include <st_sim_utils.h>
#include <st_driver.h>
#include <stdlib.h>
#include <stdio.h>
#include <regex.h>
#include <vector>
#include <st_exception.h>
#include <st_design_space.h>
#include <st_common_utils.h>

//#define DEBUG

// Sections tags delimiters

#define SECTION_TAG_BEGIN "##"
#define SECTION_TAG_END ""

#define DS_PARAMETERS_SECTION_BEGIN                                            \
  SECTION_TAG_BEGIN "DESIGN_SPACE_PARAMETERS_SECTION_BEGIN" SECTION_TAG_END
#define DS_PARAMETERS_SECTION_END                                              \
  SECTION_TAG_BEGIN "DESIGN_SPACE_PARAMETERS_SECTION_END" SECTION_TAG_END

#define CONFIGURATION_FILE_SECTION_BEGIN                                       \
  SECTION_TAG_BEGIN "CONFIGURATION_FILE_SECTION_BEGIN" SECTION_TAG_END
#define CONFIGURATION_FILE_SECTION_END                                         \
  SECTION_TAG_BEGIN "CONFIGURATION_FILE_SECTION_END" SECTION_TAG_END

#define EXTERNAL_CONFIGURATION_FILES_SECTION_BEGIN                             \
  SECTION_TAG_BEGIN "EXTERNAL_CONFIGURATION_FILES_SECTION_"                    \
                    "BEGIN" SECTION_TAG_END
#define EXTERNAL_CONFIGURATION_FILES_SECTION_END                               \
  SECTION_TAG_BEGIN "EXTERNAL_CONFIGURATION_FILES_SECTION_END" SECTION_TAG_END

#define COMMAND_LINE_SECTION_BEGIN                                             \
  SECTION_TAG_BEGIN "COMMAND_LINE_SECTION_BEGIN" SECTION_TAG_END
#define COMMAND_LINE_SECTION_END                                               \
  SECTION_TAG_BEGIN "COMMAND_LINE_SECTION_END" SECTION_TAG_END

#define METRICS_SECTION_BEGIN                                                  \
  SECTION_TAG_BEGIN "METRICS_SECTION_BEGIN" SECTION_TAG_END
#define METRICS_SECTION_END                                                    \
  SECTION_TAG_BEGIN "METRICS_SECTION_END" SECTION_TAG_END

#define RULES_SECTION_BEGIN                                                    \
  SECTION_TAG_BEGIN "RULES_SECTION_BEGIN" SECTION_TAG_END
#define RULES_SECTION_END SECTION_TAG_BEGIN "RULES_SECTION_END" SECTION_TAG_END

// Configuration file identifiers
#define RULE_READBACK_DEFAULT "RULE_READBACK"
#define METRIC_READBACK_DEFAULT "METRIC_READBACK"
#define ERROR_READBACK_DEFAULT "ERROR_READBACK"
#define SIMULATOR_INPUT_DEFAULT "SIMULATOR_INPUT"
#define SIMULATOR_OUTPUT_DEFAULT "SIMULATOR_OUTPUT"
#define METRIC_IDENTIFIER "METRIC_VALUE"
#define PATTERN_MATCHED "PATTERN_MATCHED"
#define ERROR_MATCHED "ERROR_MATCHED"
#define TAG_BEGIN "<<"
#define TAG_END ">>"
#define TAG_CLOSE "/"

// Driver constants
#define SIMULATOR_INPUT_DEFAULT_BASE_NAME "most_sim_input"
#define SIMULATOR_INPUT_DEFAULT_EXTENSION ".cfg"
#define SHELL_VARIABLE_NAME_SIMULATOR_INPUT_USER_DEFINED                       \
  "generic_driver_simulator_input"

#define SIMULATOR_OUTPUT_DEFAULT_BASE_NAME "most_sim_output"
#define SIMULATOR_OUTPUT_DEFAULT_EXTENSION ".res"
#define SHELL_VARIABLE_NAME_SIMULATOR_OUTPUT_USER_DEFINED                      \
  "generic_driver_simulator_output"

#define METRIC_DEFAULT_BASE_NAME "most_metric_readback"
#define METRIC_DEFAULT_EXTENSION ".met"
#define SHELL_VARIABLE_NAME_METRIC_READBACK_USER_DEFINED                       \
  "generic_driver_metric_readback"

#define RULE_DEFAULT_BASE_NAME "most_rule_readback"
#define RULE_DEFAULT_EXTENSION ".rul"
#define SHELL_VARIABLE_NAME_RULE_READBACK_USER_DEFINED                         \
  "generic_driver_rule_readback"

#define SHELL_VARIABLE_NAME_WARNING_ENABLED "generic_driver_warning_enabled"

#define SHELL_VARIABLE_NAME_DEBUG_MODE "generic_driver_debug_mode"

#define SHELL_VARIABLE_NAME_TEMP_DIRECTORY "generic_driver_temp_directory"
#define TEMP_DIR_BASE_NAME "/most.node."

#define SHELL_VARIABLE_NAME_TEST "generic_driver_test"

#define REGMATCH_SIZE 1

// Regular expressions used for parsing the configuration file

#define MAP_BEGIN "\\(\\*"
#define MAP_BEGIN_SYMBOL "(*"

#define MAP_END "\\*\\)"
#define MAP_END_SYMBOL "*)"

#define RE_COMMENT "(#(.)*)?"
#define RE_IGNORE_LINE "^" RE_SPACE RE_COMMENT "$"

#define RE_ASSOCIATION                                                         \
  RE_SPACE RE_IDENTIFIER RE_SPACE "=" RE_SPACE RE_QUOTED_STRING RE_SPACE
#define RE_VARIABLE                                                            \
  "^" RE_SPACE RE_IDENTIFIER RE_SPACE "=" RE_SPACE MAP_BEGIN                   \
  "(" RE_ASSOCIATION ")+" MAP_END RE_SPACE RE_COMMENT "$"

using namespace std;

static st_design_space *local_design_space;

typedef struct mt {
  string unit;
  string description;
  bool is_pattern;
  string pattern;
  regex_t reg_buf_pattern;
  regex_t reg_buf_first_part;
  string command;

  bool error_defined;
  bool is_error_pattern;
  regex_t reg_buf_error_pattern;
  string error_command;
  string error_pattern;
} metric_type;

typedef struct rt {
  bool is_rule;
  string rule;
  string command;
} rule_type;

typedef struct association {
  string name;
  string value;
} association_type;

typedef struct configuration_file {
  string template_filename; // può essere relativo alla posizione del file di
                            // configurazione del driver
  bool configuration_filename_user_defined; // definisce se il nome del file
                                            // viene specificato dall'utente
  string simulator_input_configuration_filename; // mantiene il nome del file??
} configuration_file_type;

class st_generic_driver : public st_driver {
public:
  st_design_space *get_design_space(st_env *);
  st_point *simulate(st_point &, st_env *);
  bool is_valid(st_point &, st_env *);
  bool is_thread_safe() { return false; };
  string get_information();
  string get_name();
  st_generic_driver();
  ~st_generic_driver();

private:
  // command to call the simulator
  string simulator_command_line;

  // simulator input file data
  string simulator_input_configuration_filename;
  bool simulator_input_configuration_with_file;
  string simulator_input_configuration_file;
  string simulator_output_filename;
  string metric_filename;
  string rule_filename;
  bool debug_mode;
  string get_path_name();
  string get_simulator_input_configuration_filename(st_point &point);
  string get_simulator_output_filename(st_point &point);
  string get_metric_filename(st_point &point, string metric_name);
  string get_error_filename(st_point &point, string metric_name);
  string get_rule_filename(st_point &point, string rule_name);

  // simulator stuff
  bool read_simulator_output_file(st_point &point, bool *error_found);

  // simulator input configuration file stuff
  bool acquire_simulator_input_configuration_file(string text);
  bool generate_configuration_file(string filename,
                                   vector<association_type> parameters);
  void configuration_file_check(map<string, bool> *controlled_parameters);
  void control_parameters_check();

  // general use stuff
  bool warning_enabled;
  bool get_warning_enabled();
  bool get_debug_mode();
  bool initialize_reg_exps();

  // command line stuff
  bool generate_command_line(vector<association_type> parameters,
                             string *command_line);
  bool acquire_command_line(string text);
  void command_line_check(map<string, bool> *controlled_parameters);

  // rules stuff
  bool add_rule(string line);
  bool acquire_rules(string text);
  bool read_rule_is_valid_from_file(string rule_filename, bool &is_valid);

  map<string, rule_type> rules;

  // design space stuff
  bool is_exp2(int n);
  bool add_ds_parameter_integer(string identifier,
                                vector<association_type> &associations);
  bool add_ds_parameter_string(string identifier,
                               vector<association_type> &associations);
  bool add_ds_parameter_exp2(string identifier,
                             vector<association_type> &associations);
  bool add_ds_parameter(string line);
  bool acquire_design_space(string text);

  // initialization stuff
  bool acquire_section(string &text, string begin_tag, string end_tag,
                       bool mandatory);
  string get_simulator_output_filename_definition();
  string get_simulator_input_filename_definition();
  string get_metric_readback_filename_definition();
  string get_rule_readback_filename_definition();
  string temp_directory;
  bool init_path();

  // Regular expressions stuff
  regmatch_t m[REGMATCH_SIZE];

  regex_t reg_buf_null_line;
  regex_t reg_buf_variable;
  regex_t reg_buf_association;
  bool recognize_association(string *text, string *identifier, string *value);
  bool recognize_variable(string text, string *identifier,
                          vector<association_type> *associations_list);
  bool recognize_metric_from_pattern(regex_t *reg_buf_pattern,
                                     regex_t *reg_buf_first_part,
                                     string *readback_data,
                                     string *metric_pattern, string *debug_text,
                                     double *double_value);
  // metrics stuff
  bool add_ds_metric(string *text);
  bool acquire_metrics(string text);
  bool read_metric_value(st_point &point, string metric_name,
                         double *metric_value, bool *error_found);
  bool recognize_metric_error_from_pattern(regex_t *reg_buf_pattern,
                                           string *text, string *debug_text);
  bool recognize_metric_error_from_command(string command,
                                           string simulator_output,
                                           string error_readback);
  map<string, metric_type> metrics;

  // testing
  bool doing_test;
  bool user_defined_simulator_output_filename;
  bool user_defined_simulator_input_filename;
  void prs_display_warning(string warning);
  void driver_configuration_test();

  string get_point_representation(st_point &point);
  string get_path_name(st_point &point);
  bool find_in_file_A_and_replace_in_file_B(string filename_A,
                                            string filename_B, string to_find,
                                            string to_replace,
                                            int *substitutions_count);
  bool find_in_file(string filename, string to_find, int &num);
  bool
  external_configuration_file_check(string filename,
                                    map<string, bool> *controlled_parameters);
  string
  get_simulator_input_configuration_filename(st_point &point,
                                             configuration_file_type &conf);
  bool add_external_configuration_file(string &line);
  bool acquire_external_simulator_configuration_files(string text);
  bool
  generate_external_configuration_files(st_point &point,
                                        vector<association_type> parameters);
  string get_temp_filename_for_simulator_input_configuration(
      st_point &point, configuration_file_type &conf, int version);
  bool copy_file(string source_filename, string destination_filename);
  map<string, configuration_file_type> configuration_files;
};

#ifdef DEBUG
void message(string text) { cout << "Debug: " << text << endl; }

void qmessage(string text) { cout << "Debug: '" << text << "'" << endl; }

void dqmessage(string description, string text) {
  cout << "Debug - " << description << ": '" << text << "'" << endl;
}

void print_scalar(struct st_scalar *scalar) {
  if (scalar->type == ST_SCALAR_TYPE_LIST) {
    cout << "Scalar type: LIST\n";
    for (int i = 0; i < scalar->list_size; i++)
      cout << "\t'" << scalar->list[i] << "'\n";
    cout << "\tlist_size: '" << scalar->list_size << "'\n";
    return;
  }
  cout << "Scalar type: INTEGER\n";
  cout << "\tmin: " << scalar->min << "\n";
  cout << "\tmax: " << scalar->max << "\n";
}
#endif

inline string get_elements_as_string(vector<int> input) {
  ostringstream result;
  int i;

  for (i = 0; i < input.size(); ++i) {
    if (i > 0)
      result << "-";
    result << input[i];
  }

  return result.str();
}

inline string to_string(int value) {
  ostringstream temp;

  temp << value;
  return temp.str();
}

inline string get_unique_identifier() {
  return st_get_unique_string_identifier();
}

bool association_exists(vector<association_type> *associations_list,
                        string name) {
  for (int i = 0; i < associations_list->size(); i++)
    if ((*associations_list)[i].name == name)
      return true;
  return false;
}

string association_get(vector<association_type> *associations_list,
                       string name) {
  for (int i = 0; i < associations_list->size(); i++)
    if ((*associations_list)[i].name == name)
      return (*associations_list)[i].value;
  return "";
}

string get_current_dir() {
  char sz[400];
  string s = getcwd(sz, 400);
  return s;
}

void st_generic_driver::prs_display_warning(string warning) {
  if (warning_enabled)
    cout << "Warning: " << warning << endl;
}

st_design_space *st_generic_driver::get_design_space(st_env *env) {
  return local_design_space;
}

bool st_generic_driver::copy_file(string source_filename,
                                  string destination_filename) {
  FILE *source_file, *destination_file;
  int c;
  source_file = fopen(source_filename.c_str(), "r");
  if (source_file == NULL) {
    return false;
  }
  destination_file = fopen(destination_filename.c_str(), "w");
  if (destination_file == NULL) {
    fclose(source_file);
    return false;
  }
  while ((c = fgetc(source_file)) != EOF) {

    if (fputc(c, destination_file) == EOF) {
      fclose(source_file);
      fclose(destination_file);
      return false;
    }
  }
  fclose(source_file);
  fclose(destination_file);
  return true;
}

bool st_generic_driver::generate_external_configuration_files(
    st_point &point, vector<association_type> parameters) {
  string pattern, parameter_value;
  map<string, configuration_file_type>::iterator it;
  vector<string> temp_filenames;
  temp_filenames.push_back(get_temp_filename_for_simulator_input_configuration(
      point, (*it).second, 0));
  temp_filenames.push_back(get_temp_filename_for_simulator_input_configuration(
      point, (*it).second, 1));
  for (it = configuration_files.begin(); it != configuration_files.end();
       it++) {
    if (!copy_file((*it).second.template_filename, temp_filenames[0])) {
      return false;
    }
    int i;
    for (i = 0; i < parameters.size(); i++) {
      pattern = string(TAG_BEGIN) + parameters[i].name + string(TAG_END);
      if (!find_in_file_A_and_replace_in_file_B(
              temp_filenames[i % 2], temp_filenames[(i + 1) % 2], pattern,
              parameters[i].value, NULL)) {
        return false;
      }
    }
    string configuration_filename =
        get_simulator_input_configuration_filename(point, (*it).second);
    copy_file(temp_filenames[i % 2], configuration_filename);
  }
  remove(temp_filenames[0].c_str());
  remove(temp_filenames[1].c_str());
  return true;
}

bool st_generic_driver::generate_configuration_file(
    string filename, vector<association_type> parameters) {
  string parameter, parameter_value;
  string config_text = simulator_input_configuration_file;
  ofstream fout(filename.c_str());
  if (fout.fail()) {
#ifdef DEBUG
    // message( "" );
    message("\t\t*** generate_configuration_file ***");
    dqmessage("\t\t\tImpossible to generate the simulation input file",
              filename.c_str());
#endif
    return false;
  }
  for (int i = 0; i < parameters.size(); i++) {
    parameter = parameters[i].name;
    parameter_value = parameters[i].value;
    st_find_and_replace(&config_text, TAG_BEGIN + parameter + TAG_END,
                        parameter_value, NULL);
  }
  fout << config_text;
  fout.close();
#ifdef DEBUG
  // message( "" );
  message("\t\t*** generate_configuration_file ***");
  dqmessage("\t\t\tSimulator configuration file generated successfully",
            filename.c_str());
#endif
  return true;
}

bool st_generic_driver::generate_command_line(
    vector<association_type> parameters, string *command_line) {
  string parameter, parameter_value;
  *command_line = simulator_command_line;
  for (int i = 0; i < parameters.size(); i++) {
    parameter = parameters[i].name;
    parameter_value = parameters[i].value;
    st_find_and_replace(command_line, TAG_BEGIN + parameter + TAG_END,
                        parameter_value, NULL);
  }
#ifdef DEBUG
  // message( "" );
  message("\t\t*** generate_command_line ***");
  dqmessage("\t\t\tcommand_line", *command_line);
#endif
  return true;
}

bool st_generic_driver::external_configuration_file_check(
    string filename, map<string, bool> *controlled_parameters) {
  int num;
  string parameter_name;
  for (int i = 0; i < local_design_space->ds_parameters_names.size(); i++) {
    parameter_name = local_design_space->ds_parameters_names[i];
    if (find_in_file(filename,
                     string(TAG_BEGIN) + parameter_name + string(TAG_END),
                     num)) {
      if (num > 0)
        (*controlled_parameters)[parameter_name] = true;
      else if (controlled_parameters->find(parameter_name) ==
               controlled_parameters->end())
        (*controlled_parameters)[parameter_name] = false;
    } else {
      prs_display_error("Problems while trying to read file: \"" + filename +
                        "\"");
      return false;
    }
  }
  return true;
}

void st_generic_driver::configuration_file_check(
    map<string, bool> *controlled_parameters) {
  string parameter_name;
  for (int i = 0; i < local_design_space->ds_parameters_names.size(); i++) {
    parameter_name = local_design_space->ds_parameters_names[i];
    if (simulator_input_configuration_file.find(
            string(TAG_BEGIN) + parameter_name + string(TAG_END)) !=
        string::npos) {
      (*controlled_parameters)[parameter_name] = true;
    } else {
      if (controlled_parameters->find(parameter_name) ==
          controlled_parameters->end())
        (*controlled_parameters)[parameter_name] = false;
    }
  }
}

void st_generic_driver::command_line_check(
    map<string, bool> *controlled_parameters) {
  string parameter_name;
  for (int i = 0; i < local_design_space->ds_parameters_names.size(); i++) {
    parameter_name = local_design_space->ds_parameters_names[i];
    if (simulator_command_line.find(string(TAG_BEGIN) + parameter_name +
                                    string(TAG_END)) != string::npos) {
      (*controlled_parameters)[parameter_name] = true;
    } else {
      if (controlled_parameters->find(parameter_name) ==
          controlled_parameters->end())
        (*controlled_parameters)[parameter_name] = false;
    }
  }
}

string st_generic_driver::get_temp_filename_for_simulator_input_configuration(
    st_point &point, configuration_file_type &conf, int version) {
  return get_path_name(point) + string(SIMULATOR_INPUT_DEFAULT_BASE_NAME) +
         "_temp_" + std::to_string(version) +
         string(SIMULATOR_INPUT_DEFAULT_EXTENSION);
}

string st_generic_driver::get_simulator_input_configuration_filename(
    st_point &point, configuration_file_type &conf) {
  if (conf.configuration_filename_user_defined)
    return conf.simulator_input_configuration_filename;
  else
    return get_path_name(point) + conf.simulator_input_configuration_filename;
}

void st_generic_driver::control_parameters_check() {
  map<string, bool> controlled_parameters;
  map<string, configuration_file_type>::iterator it;
  // configuration files check for design space parameters
  configuration_file_check(&controlled_parameters);
  for (it = configuration_files.begin(); it != configuration_files.end();
       it++) {
    if (!external_configuration_file_check((*it).second.template_filename,
                                           &controlled_parameters)) {
      prs_display_error(
          "Impossible to check the simulator control parameters.");
      return;
    }
  }
  // command line check for design space parameters
  command_line_check(&controlled_parameters);
  // global design space parameters verification
  for (int i = 0; i < local_design_space->ds_parameters_names.size(); i++) {
    string parameter_name = local_design_space->ds_parameters_names[i];
    if (controlled_parameters[parameter_name] == false)
      prs_display_warning("Parameter \"" + parameter_name +
                          "\" not used to control the simulator");
  }
  // configuration files links verification
  for (it = configuration_files.begin(); it != configuration_files.end();
       it++) {
    string identifier = string(TAG_BEGIN) + (*it).first + string(TAG_END);
    if (simulator_command_line.find(identifier) == string::npos)
      prs_display_warning("Identifier \"" + identifier +
                          "\" not found in the command line specification");
  }
}

bool launch_command(string command) {

#ifdef DEBUG
  message("\t*** launch_command ***");
#endif
  int rs = system(command.c_str());
  if (rs == -1 || rs == 127) {
#ifdef DEBUG

    message("\t\t\"" + command + "\" : failed - " + to_string(rs));
#endif
    return false;
  }
  /** The upper 8 bits contain the exit status */
  rs = rs >> 8;
#ifdef DEBUG
  string result = ((rs) ? "FAIL" : "OK");
  message("\t\t\"" + command + "\" : " + result);
#endif
  if (rs)
    return false;
  return true;
}

bool st_generic_driver::recognize_metric_error_from_command(
    string command, string simulator_output, string error_readback) {
  st_find_and_replace(&command, TAG_BEGIN ERROR_READBACK_DEFAULT TAG_END,
                      error_readback, NULL);
  st_find_and_replace(&command, TAG_BEGIN SIMULATOR_OUTPUT_DEFAULT TAG_END,
                      simulator_output, NULL);
  if (!launch_command(command))
    return true;
  ifstream error_file(error_readback.c_str());
  if (error_file.fail()) {
#ifdef DEBUG
    message("\t\t*** metric error evaluation ***");
    dqmessage("\t\t\tImpossible to readback the error file",
              error_readback.c_str());
#endif
    return false;
  }
  stringstream ss;
  ss << error_file.rdbuf();
  error_file.close();
  string bool_text = ss.str();
  bool bool_value;
  if (!st_recognize_boolean(&bool_text, &bool_value, NULL, NULL))
    return true;
  return bool_value;
}

bool st_generic_driver::recognize_metric_error_from_pattern(
    regex_t *reg_buf_pattern, string *text, string *debug_text) {
  string matched, before, after;
  if (st_recognize_re(text, &matched, &before, &after, *reg_buf_pattern)) {
    if (debug_text != NULL)
      (*debug_text) = before + string(TAG_BEGIN) + string(ERROR_MATCHED) +
                      string(TAG_END) + matched + string(TAG_BEGIN) +
                      string(TAG_CLOSE) + string(ERROR_MATCHED) +
                      string(TAG_END) + after;
    return true;
  }
  return false;
}

bool st_generic_driver::recognize_metric_from_pattern(
    regex_t *reg_buf_pattern, regex_t *reg_buf_first_part,
    string *readback_data, string *metric_pattern, string *debug_text,
    double *double_value) {
  string first_part, left_part, right_part, last_part;
  if (regexec(reg_buf_pattern, readback_data->c_str(), REGMATCH_SIZE, m,
              REG_NOTBOL | REG_NOTEOL) == REG_NOMATCH)
    return false;
  if ((int)m[0].rm_so != -1) {
    int length = (int)m[0].rm_eo - (int)m[0].rm_so;
    *metric_pattern = readback_data->substr((int)m[0].rm_so, length);
    first_part = readback_data->substr(0, (int)m[0].rm_so);
    last_part = readback_data->substr((int)m[0].rm_eo,
                                      readback_data->size() - (int)m[0].rm_eo);
  } else
    return false;

  if (regexec(reg_buf_first_part, metric_pattern->c_str(), REGMATCH_SIZE, m,
              REG_NOTBOL | REG_NOTEOL) == REG_NOMATCH)
    return false;

  if ((int)m[0].rm_so != -1) {
    int length = metric_pattern->size() - (int)m[0].rm_eo;
    left_part = metric_pattern->substr(0, (int)m[0].rm_eo);
    *metric_pattern = metric_pattern->substr((int)m[0].rm_eo, length);
  } else
    return false;

  string left_side_double, right_side_double;
  if (!st_recognize_double(metric_pattern, double_value, &left_side_double,
                           &right_side_double))
    return false;

  left_part += left_side_double;
  right_part = right_side_double;

  if (debug_text != NULL) {
    (*debug_text) = first_part + string(TAG_BEGIN) + string(PATTERN_MATCHED) +
                    string(TAG_END);
    (*debug_text) += left_part + string(TAG_BEGIN) + string(METRIC_IDENTIFIER) +
                     string(TAG_END);
    (*debug_text) += (*metric_pattern) + string(TAG_BEGIN) + string(TAG_CLOSE) +
                     string(METRIC_IDENTIFIER) + string(TAG_END);
    (*debug_text) += right_part + string(TAG_BEGIN) + string(TAG_CLOSE) +
                     string(PATTERN_MATCHED) + string(TAG_END);
    (*debug_text) += last_part;
  }
  return true;
}

bool st_generic_driver::read_metric_value(st_point &point, string metric_name,
                                          double *metric_value,
                                          bool *error_found) {
  string double_value;
  *error_found = true;
  if (metrics[metric_name].is_pattern) {
    string readback_data;
    ifstream output_file(get_simulator_output_filename(point).c_str());
    if (output_file.fail()) {
#ifdef DEBUG
      message("\t\t*** metric_value ***");
      dqmessage("\t\t\tImpossible to open the simulator output file",
                get_simulator_output_filename(point).c_str());
#endif
      return false;
    }
    stringstream ss;
    ss << output_file.rdbuf();
    output_file.close();
    readback_data = ss.str();

    string debug_text;
    if (!recognize_metric_from_pattern(&metrics[metric_name].reg_buf_pattern,
                                       &metrics[metric_name].reg_buf_first_part,
                                       &readback_data, &double_value,
                                       &debug_text, metric_value))
      return false;
    if (debug_mode) {
      ofstream metric_file(get_metric_filename(point, metric_name).c_str());
      if (metric_file.fail()) {
#ifdef DEBUG
        message("\t\t*** metric_value ***");
        dqmessage("\t\t\tImpossible to generate the file",
                  get_metric_filename(point, metric_name).c_str());
#endif
        return false;
      }
      metric_file << debug_text;
      metric_file.close();
    }
  } else {
    string command_line;
    command_line = metrics[metric_name].command;
    st_find_and_replace(&command_line,
                        TAG_BEGIN METRIC_READBACK_DEFAULT TAG_END,
                        get_metric_filename(point, metric_name), NULL);
    st_find_and_replace(&command_line,
                        TAG_BEGIN SIMULATOR_OUTPUT_DEFAULT TAG_END,
                        get_simulator_output_filename(point), NULL);
    if (!launch_command(command_line))
      return false;
    ifstream metric_file(get_metric_filename(point, metric_name).c_str());
    if (metric_file.fail()) {
#ifdef DEBUG
      message("\t\t*** metric_value ***");
      dqmessage("\t\t\tImpossible to generate the file",
                get_metric_filename(point, metric_name).c_str());
#endif
      return false;
    }
    stringstream ss;
    ss << metric_file.rdbuf();
    metric_file.close();
    double_value = ss.str();
    if (!st_recognize_double(&double_value, metric_value, NULL, NULL))
      return false;
  }
  if (metrics[metric_name].error_defined) {
    if (metrics[metric_name].is_error_pattern) {
      string readback_data, debug_error_text;
      ifstream output_file(get_simulator_output_filename(point).c_str());
      if (output_file.fail()) {
#ifdef DEBUG
        message("\t\t*** error tagging ***");
        dqmessage("\t\t\tImpossible to open the simulator output file",
                  get_simulator_output_filename(point).c_str());
#endif
        return false;
      }
      stringstream ss;
      ss << output_file.rdbuf();
      output_file.close();
      readback_data = ss.str();

      if (!recognize_metric_error_from_pattern(
              &metrics[metric_name].reg_buf_error_pattern, &readback_data,
              &debug_error_text)) {
        *error_found = false;
      } else {
        if (debug_mode) {
          ofstream error_file(get_error_filename(point, metric_name).c_str());
          if (error_file.fail()) {
#ifdef DEBUG
            message("\t\t*** error tagging ***");
            dqmessage("\t\t\tImpossible to generate the file",
                      get_error_filename(point, metric_name).c_str());
#endif
            return false;
          }
          error_file << debug_error_text;
          error_file.close();
        }
      }
    } else {
      if (!recognize_metric_error_from_command(
              metrics[metric_name].error_command,
              get_simulator_output_filename(point),
              get_error_filename(point, metric_name))) {
        *error_found = false;
      }
    }
  } else {
    *error_found = false;
  }
  return true;
}

bool st_generic_driver::read_simulator_output_file(st_point &point,
                                                   bool *error_found) {
  st_vector metrics_vector;
  string metric_name;
  double metric_value;
  *error_found = false;
  for (int i = 0; i < local_design_space->metric_names.size(); i++) {
    metric_name = local_design_space->metric_names[i];
    bool error;
    if (!read_metric_value(point, metric_name, &metric_value, &error)) {
      prs_display_error("Impossible to read the value for metric: \"" +
                        local_design_space->metric_names[i] + "\".");
      return false;
    } else {
      if (error)
        *error_found = true;
      st_double obj(metric_value);
      metrics_vector.insert(i, obj);
    }
  }
  point.set_properties("metrics", metrics_vector);
#ifdef DEBUG
  message("");
  message("*** read_simulator_output_file ***");
  for (int i = 0; i < local_design_space->metric_names.size(); i++) {
    metric_name = local_design_space->metric_names[i];
    message("Metric: \"" + metric_name + "\"");
    dqmessage("\tUnit", metrics[metric_name].unit);
    dqmessage("\tDescription", metrics[metric_name].description);
    if (metrics[metric_name].is_pattern)
      dqmessage("\tPattern", metrics[metric_name].pattern);
    else
      dqmessage("\tCommand", metrics[metric_name].command);
    dqmessage("\tValue", metrics_vector.get(i).print());
  }
#endif
  return true;
}

st_point *st_generic_driver::simulate(st_point &point, st_env *env) {
  st_point *simulated_point = new st_point(point);
  vector<association_type> parameters;
  if (point.size() != local_design_space->size()) {
    simulated_point->set_error(ST_POINT_NON_FATAL_ERROR);
    return simulated_point;
  }
  if (!is_valid(point, env)) {
    simulated_point->set_error(point.get_error());
    return simulated_point;
  }
  association_type new_association;
  for (int i = 0; i < local_design_space->size(); i++) {
    st_parameter par = local_design_space->ds_parameters[i];
    if (par.type == ST_DS_PERMUTATION) {
      vector<int> permutation =
          local_design_space->get_permutation(env, &point, par.name);
      new_association.name = par.name;
      new_association.value = get_elements_as_string(permutation);
      parameters.push_back(new_association);
    } else if (par.type == ST_DS_ON_OFF_MASK) {
      vector<int> mask = local_design_space->get_mask(env, &point, par.name);
      new_association.name = par.name;
      new_association.value = get_elements_as_string(mask);
      parameters.push_back(new_association);
    } else {
      if (local_design_space->scalar_parameters[par.name].list.size() == 0) {
        new_association.name = par.name;
        new_association.value = std::to_string(point[i]);
        parameters.push_back(new_association);
      } else {
        new_association.name = par.name;
        new_association.value =
            local_design_space->scalar_parameters[par.name].list[point[i]];
        parameters.push_back(new_association);
      }
    }
  }
  new_association.name = SIMULATOR_OUTPUT_DEFAULT;
  new_association.value = get_simulator_output_filename(point);
  parameters.push_back(new_association);

  new_association.name = SIMULATOR_INPUT_DEFAULT;
  new_association.value = get_simulator_input_configuration_filename(point);
  parameters.push_back(new_association);
#ifdef DEBUG
  {
    message("");
    message("*** simulate ***");
    message("\tParameters:");
    for (int i = 0; i < parameters.size(); i++)
      dqmessage("\t\t" + parameters[i].name, parameters[i].value);
  }
#endif
  if (simulator_input_configuration_with_file) {
    if (!generate_configuration_file(
            get_simulator_input_configuration_filename(point), parameters)) {
      simulated_point->set_error(ST_POINT_FATAL_ERROR);
      return simulated_point;
    }
  }
  if (!generate_external_configuration_files(point, parameters)) {
    simulated_point->set_error(ST_POINT_FATAL_ERROR);
    return simulated_point;
  }
  string command_line;
  if (!generate_command_line(parameters, &command_line)) {
    simulated_point->set_error(ST_POINT_FATAL_ERROR);
    return simulated_point;
  }
  if (!doing_test || !user_defined_simulator_output_filename) {
    if (!launch_command(command_line)) {
#ifdef DEBUG
      message("\tAn error occurs while calling the simulator.");
#endif
      simulated_point->set_error(ST_POINT_FATAL_ERROR);
      return simulated_point;
    }
  }
  bool error_found;
  if (!read_simulator_output_file(*simulated_point, &error_found)) {
#ifdef DEBUG
    message("\tImpossible to readback metrics value");
#endif
    simulated_point->set_error(ST_POINT_FATAL_ERROR);
    return simulated_point;
  }
  if (error_found)
    simulated_point->set_error(ST_POINT_NON_FATAL_ERROR);
  return simulated_point;
}

bool st_generic_driver::read_rule_is_valid_from_file(string rule_filename,
                                                     bool &is_valid) {
  ifstream rule_file(rule_filename.c_str());
  if (rule_file.fail()) {
    is_valid = false;
    return false;
  }
  string boolean;
  stringstream ss;
  ss << rule_file.rdbuf();
  rule_file.close();
  string rule_result = ss.str();
  return st_recognize_boolean(&rule_result, &is_valid, NULL, NULL);
}

string st_generic_driver::get_point_representation(st_point &point) {
  string representation = "";
  for (int i = 0; i < local_design_space->size(); i++) {
    st_parameter par = local_design_space->ds_parameters[i];
    if (par.type == ST_DS_PERMUTATION) {
      /* FIXME - to fill when permutations will be added */
    } else if (par.type == ST_DS_ON_OFF_MASK) {
      /* FIXME - to fill when masks will be added */
    } else {
      representation += "_" + std::to_string(point[i]);
    }
  }
  return representation;
}

string st_generic_driver::get_path_name(st_point &point) {
  return temp_directory + "/point" + get_point_representation(point) + "/";
}

bool st_generic_driver::is_valid(st_point &point, st_env *env) {
  map<string, rule_type>::iterator it;
#ifdef DEBUG
  message("");
  message("*** is_valid ***");
#endif
  if (!st_overwrite_directory(get_path_name(point))) {
    point.set_error(ST_POINT_FATAL_ERROR);
    return false;
  }
  for (it = this->rules.begin(); it != this->rules.end(); it++) {
    string command_line;
    if ((*it).second.is_rule)
      command_line =
          "echo \"if( " + (*it).second.rule +
          ") \\\"true\\\" else \\\"false\\\"\" | bc > <<RULE_READBACK>>";
    else
      command_line = (*it).second.command;
    string parameter_representation;
    for (int i = 0; i < local_design_space->size(); i++) {
      st_parameter par = local_design_space->ds_parameters[i];
      if (par.type == ST_DS_PERMUTATION) {
        vector<int> permutation =
            local_design_space->get_permutation(env, &point, par.name);
        parameter_representation = get_elements_as_string(permutation);
      } else if (par.type == ST_DS_ON_OFF_MASK) {
        vector<int> mask = local_design_space->get_mask(env, &point, par.name);
        parameter_representation = get_elements_as_string(mask);
      } else {
        if (local_design_space->scalar_parameters[par.name].list.size() == 0)
          parameter_representation = std::to_string(point[i]);
        else
          parameter_representation =
              local_design_space->scalar_parameters[par.name].list[point[i]];
      }
      st_find_and_replace(&command_line, TAG_BEGIN + par.name + TAG_END,
                          parameter_representation, NULL);
    }
    st_find_and_replace(&command_line, TAG_BEGIN RULE_READBACK_DEFAULT TAG_END,
                        get_rule_filename(point, (*it).first), NULL);
#ifdef DEBUG
    dqmessage("\trule", (*it).first);
#endif
    if (!launch_command(command_line)) {
#ifdef DEBUG
      message("\t\tImpossible to verify the rule.");
#endif
      point.set_error(ST_POINT_FATAL_ERROR);
      return false;
    }
    bool is_valid;
    if (!read_rule_is_valid_from_file(get_rule_filename(point, (*it).first),
                                      is_valid)) {
#ifdef DEBUG
      message("\t\tImpossible to readback the rule satisfaction state.");
#endif
      point.set_error(ST_POINT_FATAL_ERROR);
      return false;
    }
    if (!is_valid) {
#ifdef DEBUG
      message("\t\tThe rule is not satisfied.");
#endif
      point.set_error(ST_POINT_NON_FATAL_ERROR);
      return false;
    }
#ifdef DEBUG
    message("\t\tRule satisfied.");
#endif
  }
#ifdef DEBUG
  message("\tAll rules satisfied.");
#endif
  return true;
}

string st_generic_driver::get_information() { return "Generic Driver"; }

string st_generic_driver::get_name() { return "Generic Driver"; }

bool st_generic_driver::is_exp2(int n) {
  if (n < 1)
    return false;
  float exp2 = log(n) / log(2);
  int res = (int)pow((float)2, exp2);
  return n == res;
}

bool st_generic_driver::add_ds_parameter_exp2(
    string identifier, vector<association_type> &associations) {
  int min, max;
  string text;
  vector<association_type> string_associations;
  string_associations.clear();

  if (!association_exists(&associations, "min"))
    return false;
  text = association_get(&associations, "min");
  if (!st_recognize_integer(&text, &min, NULL, NULL))
    return false;
  if (!is_exp2(min))
    return false;

  if (!association_exists(&associations, "max"))
    return false;
  text = association_get(&associations, "max");
  if (!st_recognize_integer(&text, &max, NULL, NULL))
    return false;
  if (!is_exp2(max))
    return false;

  if ((max <= min) || (associations.size() > 3))
    return false;

  for (int i = min; i <= max; i *= 2) {
    association_type new_association;
    new_association.value = std::to_string(i);
    new_association.name = "value_" + new_association.value;
    string_associations.push_back(new_association);
  }

  add_ds_parameter_string(identifier, string_associations);
  return true;
}

bool st_generic_driver::add_ds_parameter_string(
    string identifier, vector<association_type> &associations) {
  vector<string> list;
  for (int i = 0; i < associations.size(); i++) {
    if (associations[i].name != "type")
      list.push_back(associations[i].value);
  }
  local_design_space->insert_scalar(&current_environment, identifier,
                                    ST_SCALAR_TYPE_LIST, 0, 0, list);
#ifdef DEBUG
  message("");
  message("*** add_ds_parameter_string ***");
  for (int i = 0; i < list.size(); i++)
    dqmessage("\tAdded", list[i]);
#endif
  return true;
}

bool st_generic_driver::add_ds_parameter_integer(
    string identifier, vector<association_type> &associations) {
  int min, max, step, additional;
  string text;
  additional = 0;

  if (!association_exists(&associations, "min"))
    return false;
  text = association_get(&associations, "min");
  if (!st_recognize_integer(&text, &min, NULL, NULL))
    return false;

  if (!association_exists(&associations, "max"))
    return false;
  text = association_get(&associations, "max");
  if (!st_recognize_integer(&text, &max, NULL, NULL))
    return false;
  if (max <= min)
    return false;
  if (association_exists(&associations, "step")) {
    text = association_get(&associations, "step");
    if (!st_recognize_integer(&text, &step, NULL, NULL))
      return false;
    if (step != 1) {
      if (step <= 0)
        return false;
      if (((max - min) % step) != 0)
        return false;
      vector<association_type> string_associations;
      string representation;
      string_associations.clear();
      for (int i = min; i <= max; i += step) {
        representation = std::to_string(i);
        association_type new_association;
        new_association.name = "value_" + representation;
        new_association.value = representation;
        string_associations.push_back(new_association);
      }
      return add_ds_parameter_string(identifier, string_associations);
    }
    additional = 1;
  }
  if (associations.size() > (3 + additional))
    return false;
  local_design_space->insert_scalar(&current_environment, identifier,
                                    ST_SCALAR_TYPE_INTEGER, min, max,
                                    vector<string>());
  return true;
}

inline int find_in_vector(string element, vector<string> &elements) {
  for (int i = 0; i < elements.size(); i++)
    if (elements[i] == element)
      return i;
  return -1;
}

bool st_generic_driver::add_ds_parameter(string line) {
  string identifier;
  vector<association_type> associations;
  if (!recognize_variable(line, &identifier, &associations))
    return false;
  if (find_in_vector(identifier, local_design_space->ds_parameters_names) != -1)
    return false;

  if (!association_exists(&associations, "type"))
    return false;
  if (st_tolower(association_get(&associations, "type")) == "integer") {
    if (!add_ds_parameter_integer(identifier, associations))
      return false;
  } else if (st_tolower(association_get(&associations, "type")) == "exp2") {
    if (!add_ds_parameter_exp2(identifier, associations))
      return false;
  } else if (st_tolower(association_get(&associations, "type")) == "string") {
    if (!add_ds_parameter_string(identifier, associations))
      return false;
  } else {
    /* FIXME - add the remaining parameters */
    prs_display_error("Wrong specification file format: \"" + line + "\"");
    return false;
  }
  return true;
}

bool st_generic_driver::initialize_reg_exps() {
  if (regcomp(&reg_buf_null_line, RE_IGNORE_LINE, REG_EXTENDED | REG_ICASE) !=
      0)
    return false;
  if (regcomp(&reg_buf_association, RE_ASSOCIATION, REG_EXTENDED | REG_ICASE) !=
      0)
    return false;
  if (regcomp(&reg_buf_variable, RE_VARIABLE, REG_EXTENDED | REG_ICASE) != 0)
    return false;
  return true;
}

int find_last_not_space_char_index(string text) {
  for (int i = text.size() - 1; i >= 0; i--)
    if (text[i] != ' ' && text[i] != '\t')
      return i;
  return -1;
}

bool st_generic_driver::acquire_design_space(string text) {
  string buffer;
  if (local_design_space != NULL)
    delete local_design_space;
  local_design_space = new st_design_space();
  int symbol_position = text.find("\n");
  string remaining = text;
  while (symbol_position != string::npos) {
    string line = remaining.substr(0, symbol_position);
    remaining = remaining.substr(symbol_position + 1,
                                 remaining.size() - symbol_position - 1);
    if (regexec(&reg_buf_null_line, line.c_str(), REGMATCH_SIZE, m, 0) ==
        REG_NOMATCH) {
      if (!add_ds_parameter(line)) {
        int last_useful_char = find_last_not_space_char_index(line);
        if (last_useful_char != -1 && line[last_useful_char] == '\\') {
          buffer += line.substr(0, last_useful_char) + " ";
        } else if (last_useful_char != -1 && line[last_useful_char] != '\\' &&
                   buffer.size() > 0) {
          buffer += line;
          remaining = buffer + "\n" + remaining;
          buffer.clear();
        } else {
          prs_display_error("Wrong configuration file format: \"" + line +
                            "\"");
          return false;
        }
      }
    } else if (buffer.size() != 0) {
      prs_display_error("Wrong configuration file format: \n" + buffer + "\n" +
                        line + "\n");
      return false;
    }
    symbol_position = remaining.find("\n");
  }
  return true;
}

bool st_generic_driver::add_rule(string line) {
  string identifier;
  vector<association_type> associations;
  if (!recognize_variable(line, &identifier, &associations))
    return false;
  if (rules.find(identifier) != rules.end())
    return false;
  if (associations.size() > 1)
    return false;
  if (association_exists(&associations, "rule")) {
    rules[identifier].is_rule = true;
    rules[identifier].rule = association_get(&associations, "rule");
  } else if (association_exists(&associations, "command")) {
    rules[identifier].is_rule = false;
    rules[identifier].command = association_get(&associations, "command");
  } else
    return false;
  return true;
}

bool st_generic_driver::acquire_rules(string text) {
  string buffer;
  rules.clear();
  int symbol_position = text.find("\n");
  string remaining = text;
  while (symbol_position != string::npos) {
    string line = remaining.substr(0, symbol_position);
    remaining = remaining.substr(symbol_position + 1,
                                 remaining.size() - symbol_position - 1);
    if (regexec(&reg_buf_null_line, line.c_str(), REGMATCH_SIZE, m, 0) ==
        REG_NOMATCH) {
      if (!add_rule(line)) {
        int last_useful_char = find_last_not_space_char_index(line);
        if (last_useful_char != -1 && line[last_useful_char] == '\\') {
          buffer += line.substr(0, last_useful_char) + " ";
        } else if (last_useful_char != -1 && line[last_useful_char] != '\\' &&
                   buffer.size() > 0) {
          buffer += line;
          remaining = buffer + "\n" + remaining;
          buffer.clear();
        } else {
          prs_display_error("Wrong configuration file format: \"" + line +
                            "\"");
          return false;
        }
      }
    } else if (buffer.size() != 0) {
      prs_display_error("Wrong configuration file format: \n" + buffer + "\n" +
                        line + "\n");
      return false;
    }
    symbol_position = remaining.find("\n");
  }

  return true;
}

bool st_generic_driver::find_in_file(string filename, string to_find,
                                     int &num) {
  FILE *file;
  int c, i, to_find_size;
  file = fopen(filename.c_str(), "r");
  if (file == NULL)
    return false;
  i = 0;
  num = 0;
  to_find_size = to_find.size();
  do {
    c = fgetc(file);
    if (c == to_find[i]) {
      if (i == to_find_size - 1) {
        num++;
        i = 0;
      } else
        i++;
    }
  } while (c != EOF);
  fclose(file);
  return true;
}

bool st_generic_driver::find_in_file_A_and_replace_in_file_B(
    string filename_A, string filename_B, string to_find, string to_replace,
    int *substitutions_count) {
  FILE *file_A, *file_B;
  int c, i, j, to_find_size, to_replace_size;
  file_A = fopen(filename_A.c_str(), "r");
  if (file_A == NULL)
    return false;
  file_B = fopen(filename_B.c_str(), "w");
  if (file_B == NULL) {
    fclose(file_A);
    return false;
  }
  i = 0;
  to_replace_size = to_replace.size();
  to_find_size = to_find.size();
  do {
    c = fgetc(file_A);
    if (c == to_find[i]) {
      if (i == to_find_size - 1) {
        j = 0;
        while (j != to_replace_size) {
          if (fputc(to_replace[j], file_B) == EOF) {
            fclose(file_A);
            fclose(file_B);
            return false;
          }
          j++;
        }
        i = 0;
      } else {
        i++;
      }
    } else if (c != EOF) {
      j = 0;
      while (i > 0) {
        if (fputc(to_find[j], file_B) == EOF) {
          fclose(file_A);
          fclose(file_B);
          return false;
        }
        j++;
        i--;
      }
      if (fputc(c, file_B) == EOF) {
        fclose(file_A);
        fclose(file_B);
        return false;
      }
    }
  } while (c != EOF);
  fclose(file_A);
  fclose(file_B);
  return true;
}

bool st_generic_driver::recognize_association(string *text, string *identifier,
                                              string *value) {
  int length;
  string association;
  string remaining;
#ifdef DEBUG
  string received;
  received = *text;
#endif
  if (regexec(&reg_buf_association, text->c_str(), REGMATCH_SIZE, m,
              REG_NOTBOL | REG_NOTEOL) == REG_NOMATCH)
    return false;
  if ((int)m[0].rm_so != -1) {
    length = (int)m[0].rm_eo - (int)m[0].rm_so;
    association = text->substr((int)m[0].rm_so, length);
  } else
    return false;
  remaining = text->substr(0, (int)m[0].rm_so) +
              text->substr((int)m[0].rm_eo, text->length() - (int)m[0].rm_eo);
  *identifier = association;
  if (!st_recognize_identifier(identifier, NULL, NULL))
    return false;
  //*value		= association;
  string quoted_text = association;
  if (!st_recognize_quoted_string(&quoted_text, value, NULL, NULL))
    return false;
  *text = remaining;
#ifdef DEBUG
  message("*** recognize_association ***");
  dqmessage("\tReceived", received);
  dqmessage("\tAssociation", association);
  dqmessage("\tidentifier", *identifier);
  dqmessage("\tvalue", *value);
  dqmessage("\tremaining", remaining);
#endif
  return true;
}

bool st_generic_driver::recognize_variable(
    string text, string *identifier,
    vector<association_type> *associations_list) {
  string remaining, temp;
  if (regexec(&reg_buf_variable, text.c_str(), REGMATCH_SIZE, m, 0) ==
      REG_NOMATCH)
    return false;
  // recognizing identifier
  int symbol_position = text.find("=");
  if (symbol_position == string::npos)
    return false;
  temp = text.substr(0, symbol_position);
  if (!st_recognize_identifier(&temp, NULL, NULL))
    return false;
  *identifier = temp;
  remaining =
      text.substr(symbol_position + 1, text.size() - symbol_position - 1);
  temp = remaining;

  // recognizing associations
  associations_list->clear();
  if (!st_find_first_and_replace(&remaining, MAP_BEGIN_SYMBOL, ""))
    return false;
  string identifier_ass, value_ass;
  bool last_step = false;
  while (regexec(&reg_buf_null_line, remaining.c_str(), REGMATCH_SIZE, m, 0) ==
         REG_NOMATCH) {
    if (last_step)
      return false;
    if (!recognize_association(&remaining, &identifier_ass, &value_ass)) {
      if (!last_step) {
        last_step = true;
        if (!st_find_first_and_replace(&remaining, MAP_END_SYMBOL, ""))
          return false;
      }
    } else {
      if (association_exists(associations_list, identifier_ass))
        return false;
      association_type new_association;
      new_association.name = identifier_ass;
      new_association.value = value_ass;
      associations_list->push_back(new_association);
    }
  }
  if (associations_list->size() == 0)
    return false;
#ifdef DEBUG
  message("*** recognize_variable ***");
  dqmessage("\tVariable identifier", *identifier);
  for (int i = 0; i < associations_list->size(); i++)
    dqmessage("\t\t" + (*associations_list)[i].name,
              (*associations_list)[i].value);
#endif
  return true;
}

bool st_generic_driver::add_ds_metric(string *text) {
  string identifier;
  metric_type new_metric;
  vector<association_type> associations;
  if (!recognize_variable(*text, &identifier, &associations))
    return false;
  if (metrics.find(identifier) != metrics.end())
    return false;
  if (!association_exists(&associations, "unit"))
    return false;
  new_metric.unit = association_get(&associations, "unit");

  if (association_exists(&associations, "command")) {
    new_metric.is_pattern = false;
    new_metric.command = association_get(&associations, "command");
  } else if (association_exists(&associations, "pattern")) {
    new_metric.is_pattern = true;
    new_metric.pattern = association_get(&associations, "pattern");
    string pattern = new_metric.pattern;
    int count = 0;
    while (st_find_first_and_replace(
        &pattern, TAG_BEGIN METRIC_IDENTIFIER TAG_END, RE_DOUBLE))
      count++;
    if (count != 1)
      return false;
    if (regcomp(&new_metric.reg_buf_pattern, pattern.c_str(),
                REG_EXTENDED | REG_ICASE) != 0)
      return false;
    string first_part = new_metric.pattern.substr(
        0, new_metric.pattern.find(TAG_BEGIN METRIC_IDENTIFIER TAG_END));
    if (regcomp(&new_metric.reg_buf_first_part, first_part.c_str(),
                REG_EXTENDED | REG_ICASE) != 0)
      return false;
  } else
    return false;
  int additional = 0;
  if (!association_exists(&associations, "description"))
    new_metric.description = "";
  else {
    additional++;
    new_metric.description = association_get(&associations, "description");
  }

  if (association_exists(&associations, "error_pattern")) {
    additional++;
    new_metric.error_defined = true;
    new_metric.is_error_pattern = true;
    new_metric.error_pattern = association_get(&associations, "error_pattern");
    if (regcomp(&new_metric.reg_buf_error_pattern,
                new_metric.error_pattern.c_str(),
                REG_EXTENDED | REG_ICASE) != 0)
      return false;
  } else if (association_exists(&associations, "error_command")) {
    additional++;
    new_metric.error_defined = true;
    new_metric.is_error_pattern = false;
    new_metric.error_command = association_get(&associations, "error_command");
  } else {
    new_metric.error_defined = false;
    new_metric.is_error_pattern = false;
  }

  if (associations.size() > 2 + additional)
    return false;
  metrics[identifier] = new_metric;
  local_design_space->insert_metric(&current_environment, identifier,
                                    new_metric.unit);
  return true;
}

bool st_generic_driver::acquire_metrics(string text) {
  string buffer;
  if (local_design_space == NULL)
    return false;
  metrics.clear();
  int symbol_position = text.find("\n");
  string remaining = text;
  while (symbol_position != string::npos) {
    string line = remaining.substr(0, symbol_position);
    remaining = remaining.substr(symbol_position + 1,
                                 remaining.size() - symbol_position - 1);
    if (regexec(&reg_buf_null_line, line.c_str(), REGMATCH_SIZE, m, 0) ==
        REG_NOMATCH) {
      if (!add_ds_metric(&line)) {
        int last_useful_char = find_last_not_space_char_index(line);
        if (last_useful_char != -1 && line[last_useful_char] == '\\') {
          buffer += line.substr(0, last_useful_char) + " ";
        } else if (last_useful_char != -1 && line[last_useful_char] != '\\' &&
                   buffer.size() > 0) {
          buffer += line;
          remaining = buffer + "\n" + remaining;
          buffer.clear();
        } else {
          prs_display_error("Wrong configuration file format: \"" + line +
                            "\"");
          return false;
        }
      }
    } else if (buffer.size() != 0) {
      prs_display_error("Wrong configuration file format: \n" + buffer + "\n" +
                        line + "\n");
      return false;
    }
    symbol_position = remaining.find("\n");
  }
  return true;
}

bool st_generic_driver::add_external_configuration_file(string &line) {
  string identifier;
  configuration_file_type new_configuration_file;
  vector<association_type> associations;
  if (!recognize_variable(line, &identifier, &associations))
    return false;
  if (configuration_files.find(identifier) != configuration_files.end())
    return false;
  if (!association_exists(&associations, "template"))
    return false;
  new_configuration_file.template_filename =
      association_get(&associations, "template");

  int additional = 0;
  if (association_exists(&associations, "configuration_filename")) {
    new_configuration_file.configuration_filename_user_defined = true;
    new_configuration_file.simulator_input_configuration_filename =
        association_get(&associations, "configuration_filename");
    additional++;
  } else {
    new_configuration_file.configuration_filename_user_defined = false;
    new_configuration_file.simulator_input_configuration_filename =
        string(SIMULATOR_INPUT_DEFAULT_BASE_NAME) + "_" + identifier +
        string(SIMULATOR_INPUT_DEFAULT_EXTENSION);
  }

  if (associations.size() > 1 + additional)
    return false;
  configuration_files[identifier] = new_configuration_file;
  return true;
}

bool st_generic_driver::acquire_external_simulator_configuration_files(
    string text) {
  string buffer;
  int symbol_position = text.find("\n");
  string remaining = text;
  configuration_files.clear();
  while (symbol_position != string::npos) {
    string line = remaining.substr(0, symbol_position);
    remaining = remaining.substr(symbol_position + 1,
                                 remaining.size() - symbol_position - 1);
    if (regexec(&reg_buf_null_line, line.c_str(), REGMATCH_SIZE, m, 0) ==
        REG_NOMATCH) {
      if (!add_external_configuration_file(line)) {
        int last_useful_char = find_last_not_space_char_index(line);
        if (last_useful_char != -1 && line[last_useful_char] == '\\') {
          buffer += line.substr(0, last_useful_char) + " ";
        } else if (last_useful_char != -1 && line[last_useful_char] != '\\' &&
                   buffer.size() > 0) {
          buffer += line;
          remaining = buffer + "\n" + remaining;
          buffer.clear();
        } else {
          prs_display_error("Wrong configuration file format: \"" + line +
                            "\"");
          return false;
        }
      }
    } else if (buffer.size() != 0) {
      prs_display_error("Wrong configuration file format: \n" + buffer + "\n" +
                        line + "\n");
      return false;
    }
    symbol_position = remaining.find("\n");
  }
  return true;
}

bool st_generic_driver::acquire_command_line(string text) {
  simulator_command_line = text;
#ifdef DEBUG
  message("*** acquire_command_line ***");
  qmessage(simulator_command_line);
#endif
  return true;
}

bool st_generic_driver::acquire_simulator_input_configuration_file(
    string text) {
  simulator_input_configuration_file = text;
#ifdef DEBUG
  message("*** acquire_simulator_input_configuration_file ***");
  qmessage(simulator_input_configuration_file);
#endif
  return true;
}

bool st_generic_driver::acquire_section(string &text, string begin_tag,
                                        string end_tag, bool mandatory) {
  int index_start, index_end;
  index_start = text.find(begin_tag);
  index_end = text.find(end_tag);
  if (index_start == string::npos || index_end == string::npos ||
      index_start > index_end) {
    if (!mandatory && index_start == string::npos &&
        index_end == string::npos) {
      text = "";
      return true;
    }
    prs_display_error("Wrong configuration file format.");
    return false;
  }
  int delta_begin, delta_end;
  int i = index_start - 1;
  while (i >= 0 && text[i] != '\n') {
    if (text[i] != ' ' && text[i] != '\t') {
      prs_display_error("Wrong configuration file format.");
      return false;
    }
    i--;
  }
  i = index_start + begin_tag.length();
  while (i < text.size() && text[i] != '\n') {
    if (text[i] != ' ' && text[i] != '\t') {
      prs_display_error("Wrong configuration file format.");
      return false;
    }
    i++;
  }
  i = index_end - 1;
  while (text[i] != '\n') {
    if (text[i] != ' ' && text[i] != '\t') {
      prs_display_error("Wrong configuration file format.");
      return false;
    }
    i--;
  }
  i = index_end + end_tag.length();
  while (i < text.size() && text[i] != '\n') {
    if (text[i] != ' ' && text[i] != '\t') {
      prs_display_error("Wrong configuration file format.");
      return false;
    }
    i++;
  }
  text = text.substr(index_start + begin_tag.length() + 1,
                     index_end - index_start - begin_tag.length() - 1);
#ifdef DEBUG
  message("\"" + begin_tag + "\"");
  message("\"" + text + "\"");
  message("\"" + end_tag + "\"");
#endif
  return true;
}

string st_generic_driver::get_simulator_input_filename_definition() {
  string name;
  if (current_environment.shell_variables.get_string(
          SHELL_VARIABLE_NAME_SIMULATOR_INPUT_USER_DEFINED, name)) {
    user_defined_simulator_input_filename = true;
    return name;
  }
  user_defined_simulator_input_filename = false;
  name = SIMULATOR_INPUT_DEFAULT_BASE_NAME "_" + get_unique_identifier() +
         string(SIMULATOR_INPUT_DEFAULT_EXTENSION);
  return name;
}

string st_generic_driver::get_simulator_output_filename_definition() {
  string name;
  if (current_environment.shell_variables.get_string(
          SHELL_VARIABLE_NAME_SIMULATOR_OUTPUT_USER_DEFINED, name)) {
    user_defined_simulator_output_filename = true;
    return name;
  }
  user_defined_simulator_output_filename = false;
  name = SIMULATOR_OUTPUT_DEFAULT_BASE_NAME "_" + get_unique_identifier() +
         string(SIMULATOR_OUTPUT_DEFAULT_EXTENSION);
  return name;
}

string st_generic_driver::get_metric_readback_filename_definition() {
  string name;
  if (current_environment.shell_variables.get_string(
          SHELL_VARIABLE_NAME_METRIC_READBACK_USER_DEFINED, name))
    return name;
  name = METRIC_DEFAULT_BASE_NAME "_" + get_unique_identifier() +
         string(METRIC_DEFAULT_EXTENSION);
  return name;
}

string st_generic_driver::get_rule_readback_filename_definition() {
  string name;
  if (current_environment.shell_variables.get_string(
          SHELL_VARIABLE_NAME_RULE_READBACK_USER_DEFINED, name))
    return name;
  name = RULE_DEFAULT_BASE_NAME "_" + get_unique_identifier() +
         string(RULE_DEFAULT_EXTENSION);
  return name;
}

bool st_generic_driver::get_warning_enabled() {
  int value;
  if (current_environment.shell_variables.get_integer(
          SHELL_VARIABLE_NAME_WARNING_ENABLED, value))
    if (value == 1)
      return true;
  return false;
}

bool st_generic_driver::get_debug_mode() {
  int value;
  if (current_environment.shell_variables.get_integer(
          SHELL_VARIABLE_NAME_DEBUG_MODE, value))
    if (value == 1)
      return true;
  return false;
}

string
st_generic_driver::get_simulator_input_configuration_filename(st_point &point) {
  if (user_defined_simulator_input_filename)
    return simulator_input_configuration_filename;
  else
    return get_path_name(point) + simulator_input_configuration_filename;
}

string st_generic_driver::get_simulator_output_filename(st_point &point) {
  if (user_defined_simulator_output_filename)
    return simulator_output_filename;
  else
    return get_path_name(point) + simulator_output_filename;
}

string st_generic_driver::get_error_filename(st_point &point,
                                             string metric_name) {
  return get_metric_filename(point, metric_name) + ".error";
}

string st_generic_driver::get_metric_filename(st_point &point,
                                              string metric_name) {
  return get_path_name(point) + metric_name + "_" + metric_filename;
}

string st_generic_driver::get_rule_filename(st_point &point, string rule_name) {
  return get_path_name(point) + rule_name + "_" + rule_filename;
}

bool st_generic_driver::init_path() {
  string directory_name;
  if (current_environment.shell_variables.get_string(
          SHELL_VARIABLE_NAME_TEMP_DIRECTORY, directory_name))
    temp_directory = directory_name;
  else
    temp_directory = get_current_dir();
  temp_directory =
      temp_directory + string(TEMP_DIR_BASE_NAME) + get_unique_identifier();
  if (!st_overwrite_directory(temp_directory)) {
    prs_display_error("Impossible to create temporary directory: " +
                      temp_directory);
    return false;
  }
  return true;
}

void st_generic_driver::driver_configuration_test() {
  bool previous_debug_mode, previous_warning_enabled;
  int driver_test, repeat = true;
  doing_test = false;

  if (current_environment.shell_variables.get_integer(SHELL_VARIABLE_NAME_TEST,
                                                      driver_test))
    if (driver_test == 1)
      doing_test = true;
    else
      return;
  else
    return;

  previous_debug_mode = debug_mode;
  previous_warning_enabled = warning_enabled;
  debug_mode = true;
  warning_enabled = true;
  prs_display_message("Generic driver configuration file test - STARTED");
  while (repeat) {
    st_point test_point =
        local_design_space->random_point(&current_environment);
    prs_display_message("Generated the random test point: \"" +
                        test_point.print() + "\"");
    if (user_defined_simulator_output_filename)
      prs_display_message("Testing the simulation phase with the user defined "
                          "simulator output file...");
    else
      prs_display_message(
          "Testing the simulation phase calling the simulator...");
    st_point *simulated_point =
        this->simulate(test_point, &current_environment);
    if (simulated_point->get_error() == ST_POINT_NO_ERROR ||
        simulated_point->get_error() == ST_POINT_NON_FATAL_ERROR) {
      repeat = false;
      prs_display_message("Test done successfully.");
    }
    /*else if( simulated_point->get_error() == ST_POINT_NON_FATAL_ERROR )
    {
            prs_display_message( "The selected point is invalid. Trying another
    point..." );
    }*/
    else {
      repeat = false;
      prs_display_error("Test failed.");
    }
  }
  prs_display_message("Generic driver configuration file test - STOPPED");
  doing_test = false;
  debug_mode = previous_debug_mode;
  warning_enabled = previous_warning_enabled;
}

st_generic_driver::st_generic_driver() {
  int index_start, index_end;
  string conf_text, input_config, strcfg;

  string conf_file_begin(CONFIGURATION_FILE_SECTION_BEGIN);
  string conf_file_end(CONFIGURATION_FILE_SECTION_END);

  string ext_conf_files_begin(EXTERNAL_CONFIGURATION_FILES_SECTION_BEGIN);
  string ext_conf_files_end(EXTERNAL_CONFIGURATION_FILES_SECTION_END);

  string ds_params_begin(DS_PARAMETERS_SECTION_BEGIN);
  string ds_params_end(DS_PARAMETERS_SECTION_END);

  string command_line_begin(COMMAND_LINE_SECTION_BEGIN);
  string command_line_end(COMMAND_LINE_SECTION_END);

  string metrics_begin(METRICS_SECTION_BEGIN);
  string metrics_end(METRICS_SECTION_END);

  string rules_begin(RULES_SECTION_BEGIN);
  string rules_end(RULES_SECTION_END);

  simulator_input_configuration_filename =
      get_simulator_input_filename_definition();
  simulator_output_filename = get_simulator_output_filename_definition();
  metric_filename = get_metric_readback_filename_definition();
  rule_filename = get_rule_readback_filename_definition();
  warning_enabled = get_warning_enabled();
  debug_mode = get_debug_mode();

  // Internal initialization
  if (!initialize_reg_exps()) {
    prs_display_error(
        "Problems with internal specifications. Please, contact the vendor.");
    return;
  }
  if (!init_path()) {
    return;
  }

  // Reading the configuration file name
  if (!current_environment.shell_variables.get_string("input_config_file",
                                                      input_config)) {
    prs_display_error(
        "Please, specify the configuration file name into the variable "
        "\"input_config_file\" before to load the driver.");
    return;
  }

  // Opening the configuration file
  ifstream input_file(input_config.c_str());
  if (input_file.fail()) {
    prs_display_error("Impossibile to read \"" + input_config + "\"");
    return;
  }
  stringstream ss;
  ss << input_file.rdbuf();
  input_file.close();
  strcfg = ss.str();
  prs_display_message("Loading the generic driver");

  // Recognizing design space parameters section
  conf_text = strcfg;
  if (!acquire_section(conf_text, ds_params_begin, ds_params_end, true))
    return;
  if (!acquire_design_space(conf_text))
    return;

  // Recognizing metrics section
  conf_text = strcfg;
  if (!acquire_section(conf_text, metrics_begin, metrics_end, true))
    return;
  if (!acquire_metrics(conf_text))
    return;

  // Recognizing rules section
  conf_text = strcfg;
  if (acquire_section(conf_text, rules_begin, rules_end, false))
    if (!acquire_rules(conf_text))
      return;

  // Recognizing configuration file section
  conf_text = strcfg;
  if (acquire_section(conf_text, conf_file_begin, conf_file_end, false)) {
    if (strcfg.find(conf_file_begin) == string::npos)
      simulator_input_configuration_with_file = false;
    else {
      if (!acquire_simulator_input_configuration_file(conf_text)) {
        prs_display_error("Wrong configuration file format.");
        return;
      }
      simulator_input_configuration_with_file = true;
    }
  } else
    return;

  // Recognizing external configuration file section
  conf_text = strcfg;
  if (acquire_section(conf_text, ext_conf_files_begin, ext_conf_files_end,
                      false)) {
    if (strcfg.find(ext_conf_files_begin) != string::npos)
      if (!acquire_external_simulator_configuration_files(conf_text)) {
        prs_display_error("Wrong configuration file format.");
        return;
      }
  } else
    return;

  // Recognizing command line section
  conf_text = strcfg;
  if (!acquire_section(conf_text, command_line_begin, command_line_end, true))
    return;
  if (!acquire_command_line(conf_text))
    return;

  control_parameters_check();

  prs_display_message("Driver configured succesfully");

  driver_configuration_test();

#ifdef DEBUG
  debug_mode = true;
  warning_enabled = true;
#endif
}

st_generic_driver::~st_generic_driver() {
  if (!debug_mode)
    st_delete_directory(temp_directory);
}

extern "C" {
st_generic_driver *drv_generate_driver() {
  prs_display_message("Creating the generic driver");
  return new st_generic_driver();
}
}
