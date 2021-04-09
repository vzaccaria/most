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
#include <cerrno>
#include <dirent.h>
#include <dlfcn.h>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <st_common_utils.h>
#include <st_database.h>
#include <st_opt_utils.h>
#include <st_parser.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
//#include <unistd.h>
#include <dirent.h>
//#include <fcntl.h>
#include <string.h>

/** FIXME: merge this function with the similar function in opt_select_optimizer
 */

list<string> st_get_complete_path_list(st_env *env) {
  list<string> path_list;

  st_object const *search_path;
  st_object const *cbp;

  if (!env->shell_variables.get("search_path", search_path)) {
    path_list.push_back(".");
    // cout << "Adding ." << endl;
  } else {
    list<st_object *>::iterator i;
    st_list const *sp;

    if (!is_a<st_list const *>(search_path)) {
      prs_display_error("Invalid search path");
      return path_list;
    }

    sp = to<st_list const *>(search_path);

    st_list *spnc = const_cast<st_list *>(sp);

    for (i = spnc->begin(); i != spnc->end(); i++) {
      if (is_a<st_string const *>((*i))) {
        path_list.push_back(to<st_string const *>((*i))->get_string());
        // cout << "Adding " << to<st_string const *>((*i))->get_string() <<
        // endl;
      }
    }
  }

  if (env->shell_variables.get("current_build_path", cbp)) {
    if (!is_a<st_string const *>(cbp)) {
      prs_display_error("Invalid current_build_path");
      return path_list;
    }
    path_list.push_back(to<st_string const *>(cbp)->get_string() + "/lib");
    // cout << "Adding" << to<st_string const *>(cbp)->get_string() + "/lib" <<
    // endl;
  }
  return path_list;
}

string st_get_current_build_path(st_env *env) {
  st_object const *cbp;
  if (env->shell_variables.get("current_build_path", cbp)) {
    if (!is_a<st_string const *>(cbp)) {
      prs_display_error("Invalid current_build_path");
      return "";
    }
    return to<st_string const *>(cbp)->get_string();
  }
  return "";
}

//#define DEBUG

bool st_look_for_filename_in_search_path(st_env *env, string const &file_name,
                                         string &complete_path) {
  string the_file_name = "lib" + file_name + ".so";
  list<string> path_list;

  path_list = st_get_complete_path_list(env);

  list<string>::iterator it;
  for (it = path_list.begin(); it != path_list.end(); it++) {
    string path = *it;
    string c_path = path + "/" + the_file_name;
#if defined(DEBUG)
    cout << "Checking for " << c_path << endl;
#endif
    void *h = dlopen(c_path.c_str(), RTLD_NOW);
    if (h) {
      complete_path = c_path;
      dlclose(h);
      return true;
    } else {
#if defined(DEBUG)
      prs_display_error(string("Unable to find the specified module: ") +
                        dlerror());
#endif
    }
  }
  return false;
}

set<string> st_get_libraries_and_db_in_search_path(st_env *env) {
  set<string> filenames;
  list<string> path_list = st_get_complete_path_list(env);
  list<string>::iterator it;
  struct dirent **de;
  map<string, st_database *>::iterator dbi;
  dbi = env->available_dbs.begin();
  for (dbi = env->available_dbs.begin(); dbi != env->available_dbs.end();
       dbi++) {
    filenames.insert(dbi->first);
  }
  for (it = path_list.begin(); it != path_list.end(); it++) {
    int entries = scandir((*it).c_str(), &de, NULL, alphasort);
    if (entries != -1) {
      for (int i = 0; i < entries; i++) {
        string name = de[i]->d_name;
        string pre = name.substr(0, 3);
        if (name.size() >= 3) {
          string post = name.substr(name.size() - 3, 3);
          if (pre == "lib" && post == ".so") {
            string returned = name.substr(3, name.size() - 3 - 3);
            filenames.insert(returned);
          } else {
            if (post == ".db" && (*it) == ".") {
              filenames.insert(name);
            }
          }
        }
        free(de[i]);
        de[i] = NULL;
      }
      free(de);
    }
  }
  return filenames;
}

typedef st_command *(*com_help_fun_type)();

st_command *st_get_help_from_module(string module) {
  string c_module = string(__STSHELL_HARDWIRED_BUILD_PATH__) + "/lib/" + module;
  void *h = dlopen(c_module.c_str(), RTLD_NOW);
  if (h) {
    com_help_fun_type genhelp = (com_help_fun_type)dlsym(h, "get_help");
    if (!genhelp) {
      return NULL;
    } else
      return genhelp();
    dlclose(h);
  }
  return NULL;
}

set<string> st_get_modules_in_search_path() {
  set<string> filenames;
  list<string> path_list = st_get_complete_path_list(&current_environment);
  list<string>::iterator it;
  struct dirent **de;
  for (it = path_list.begin(); it != path_list.end(); it++) {
    int entries = scandir((*it).c_str(), &de, NULL, alphasort);
    if (entries != -1) {
      for (int i = 0; i < entries; i++) {
        string name = de[i]->d_name;
        string pre = name.substr(0, 3);
        if (name.size() >= 3) {
          string post = name.substr(name.size() - 3, 3);
          if (pre == "lib" && post == ".so") {
            string returned = name.substr(3, name.size() - 3 - 3);
            filenames.insert(name);
          }
        }
        free(de[i]);
        de[i] = NULL;
      }
      free(de);
    }
  }
  return filenames;
}

void st_find_and_replace(string *text, string to_find, string to_replace,
                         int *substitutions_count) {
  string first_part;
  int found = text->find(to_find);
  if (found != string::npos) {
    first_part = text->substr(0, found) + to_replace;
    string second_part = text->substr(found + to_find.size(),
                                      text->size() - found - to_find.size());
    if (substitutions_count != NULL)
      substitutions_count++;
    st_find_and_replace(&second_part, to_find, to_replace, substitutions_count);
    *text = first_part + second_part;
  }
}

bool st_find_first_and_replace(string *text, string to_find,
                               string to_replace) {
  int found = text->find(to_find);
  if (found == string::npos)
    return false;
  text->replace(found, to_find.length(), to_replace);
  return true;
}

bool st_find_last_and_replace(string *text, string to_find, string to_replace) {
  int found = text->rfind(to_find);
  if (found == string::npos)
    return false;
  text->replace(found, to_find.length(), to_replace);
  return true;
}

string st_toupper(string the_string) {
  string upcase_string = the_string;
  int string_size = upcase_string.size();
  char delta = 'A' - 'a';
  for (int i = 0; i < string_size; i++)
    if (upcase_string[i] >= 'a' && upcase_string[i] <= 'z')
      upcase_string[i] += delta;
  return upcase_string;
}

string st_tolower(string the_string) {
  string downcase_string = the_string;
  int string_size = downcase_string.size();
  char delta = 'a' - 'A';
  for (int i = 0; i < string_size; i++)
    if (downcase_string[i] >= 'A' && downcase_string[i] <= 'Z')
      downcase_string[i] += delta;
  return downcase_string;
}

string get_current_working_dir() {
  char *root_path_c = getcwd(NULL, 0);
  string root_path = root_path_c;
  free(root_path_c);
  return root_path;
}

#include <sstream>

string tmp_directory() {
  string pt = get_current_working_dir();
  ostringstream dir;
  dir << pt << "/most_tmp_" << getpid();
  return dir.str();
}

string tmp_session(string name) {
  string pt = get_current_working_dir() + "/..";
  ostringstream dir;
  dir << pt << "/session_" + name + "_" << getpid();
  return dir.str();
}

bool set_session(string name) {
  string ts = tmp_session(name);

  cout << "Creating directory " << ts << endl;

  if (!st_create_directory(ts))
    return 0;

  cout << "cp -R " + get_current_working_dir() + " " + ts << endl;

  if (!shell_command("cp -R " + get_current_working_dir() + "/* " + ts))
    return 0;

  if (chdir(ts.c_str()) == -1)
    return 0;

  return 1;
}

bool st_create_directory(string directory_name) {
  // if( mkdir( directory_name.c_str(), S_IRWXU | S_IRWXG | S_IRWXO ) != 0 &&
  // errno != 17 )
  //	if( mkdir( directory_name.c_str(), S_IRWXU | S_IRWXG | S_IRWXO ) != 0 )
  if (mkdir(directory_name.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) != 0)
    return false;
  return true;
}

bool st_overwrite_directory(string directory_name) {
  if (st_directory_already_exists(directory_name))
    if (!st_delete_directory(directory_name))
      return false;
  return st_create_directory(directory_name);
}

bool st_directory_already_exists(string directory_name) {
  DIR *directory;
  directory = opendir(directory_name.c_str());
  if (directory == NULL)
    return false;
  closedir(directory);
  return true;
}

bool st_delete_directory(string directory_name) {
  DIR *directory;
  struct dirent *entry;
  struct stat statbuf;
  directory = opendir(directory_name.c_str());
  if (directory_name[directory_name.size() - 1] != '/')
    directory_name += "/";
  if (directory != NULL) {
    entry = readdir(directory);
    while (entry != NULL) {
      string entry_full_name;
      entry_full_name = directory_name + entry->d_name;
      stat(entry_full_name.c_str(), &statbuf);
      if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
        if (S_ISREG(statbuf.st_mode)) {
          string to_delete = directory_name + entry->d_name;
          remove(to_delete.c_str());
        } else if (S_ISDIR(statbuf.st_mode)) {
          string next_dir = directory_name + entry->d_name;
          if (!st_delete_directory(next_dir.c_str())) {
            if (closedir(directory) != 0)
              prs_display_error("Impossible to close directory: \"" +
                                directory_name + "\".");
            return false;
          }
        } else {
          if (closedir(directory) != 0)
            prs_display_error("Impossible to close directory: \"" +
                              directory_name + "\".");
          return false;
        }
      }
      entry = readdir(directory);
    }
    if (closedir(directory) != 0) {
      prs_display_error("Impossible to close directory: \"" + directory_name +
                        "\".");
      return false;
    }
    if (rmdir(directory_name.c_str()) != 0) {
      prs_display_error("Impossible to delete the directory: \"" +
                        directory_name + "\".");
      return false;
    }
    return true;
  } else {
    if (errno == EACCES) {
      prs_display_error(
          "Read permission is denied for the directory named: \"" +
          directory_name + "\".");
    } else if (errno == EMFILE) {
      prs_display_error("The process has too many files open.");
    } else {
      prs_display_error("\"" + directory_name + "\" is an invalid path.");
    }
    return false;
  }
}

bool st_isnan(double number) { return isnan(number); }

bool st_isnan(float number) { return isnan(number); }

bool st_isinf(double number) { return isinf(number); }

bool st_isinf(float number) { return isinf(number); }

#define REGMATCH_SIZE 1

regmatch_t re_m[REGMATCH_SIZE];

regex_t re_buf_identifier;
regex_t re_buf_bool;
regex_t re_buf_nan;
regex_t re_buf_inf;
regex_t re_buf_error;
regex_t re_buf_integer;
regex_t re_buf_double_number;
regex_t re_buf_double;
regex_t re_buf_quoted_string;
regex_t re_buf_space;

bool st_initialize_regular_expressions() {
  if (regcomp(&re_buf_identifier, RE_IDENTIFIER, REG_EXTENDED | REG_ICASE) != 0)
    return false;
  if (regcomp(&re_buf_bool, RE_BOOL, REG_EXTENDED | REG_ICASE) != 0)
    return false;
  if (regcomp(&re_buf_integer, RE_INTEGER_NUMBER, REG_EXTENDED | REG_ICASE) !=
      0)
    return false;
  if (regcomp(&re_buf_nan, RE_NAN, REG_EXTENDED | REG_ICASE) != 0)
    return false;
  if (regcomp(&re_buf_inf, RE_INF, REG_EXTENDED | REG_ICASE) != 0)
    return false;
  if (regcomp(&re_buf_error, RE_ERROR_STRING, REG_EXTENDED | REG_ICASE) != 0)
    return false;
  if (regcomp(&re_buf_double_number, RE_DOUBLE_NUMBER,
              REG_EXTENDED | REG_ICASE) != 0)
    return false;
  if (regcomp(&re_buf_double, RE_DOUBLE, REG_EXTENDED | REG_ICASE) != 0)
    return false;
  if (regcomp(&re_buf_quoted_string, RE_QUOTED_STRING,
              REG_EXTENDED | REG_ICASE) != 0)
    return false;
  if (regcomp(&re_buf_space, RE_SPACE, REG_EXTENDED | REG_ICASE) != 0)
    return false;
  return true;
}

bool st_recognize_re(string *text, string *before, string *after,
                     regex_t &re_buf) {
  int length;
  if (regexec(&re_buf, text->c_str(), REGMATCH_SIZE, re_m,
              REG_NOTBOL | REG_NOTEOL) == REG_NOMATCH)
    return false;
  if ((int)re_m[0].rm_so != -1) {
    length = (int)re_m[0].rm_eo - (int)re_m[0].rm_so;
    if (before != NULL)
      *before = text->substr(0, (int)re_m[0].rm_so);
    if (after != NULL)
      *after = text->substr((int)re_m[0].rm_so + length,
                            text->size() - (int)re_m[0].rm_so - length);
    *text = text->substr((int)re_m[0].rm_so, length);
    return true;
  } else
    return false;
}

bool st_recognize_re(string *text, string *matched, string *before,
                     string *after, regex_t &re_buf) {
  int length;
  if (regexec(&re_buf, text->c_str(), REGMATCH_SIZE, re_m,
              REG_NOTBOL | REG_NOTEOL) == REG_NOMATCH)
    return false;
  if ((int)re_m[0].rm_so != -1) {
    length = (int)re_m[0].rm_eo - (int)re_m[0].rm_so;
    if (before != NULL)
      *before = text->substr(0, (int)re_m[0].rm_so);
    if (after != NULL)
      *after = text->substr((int)re_m[0].rm_so + length,
                            text->size() - (int)re_m[0].rm_so - length);
    if (matched != NULL)
      *matched = text->substr((int)re_m[0].rm_so, length);
    return true;
  } else
    return false;
}

bool st_recognize_identifier(string *text, string *before, string *after) {
  return st_recognize_re(text, before, after, re_buf_identifier);
}

bool st_recognize_boolean(string *text, bool *boolean, string *before,
                          string *after) {
  if (!st_recognize_re(text, before, after, re_buf_bool))
    return false;
  if (st_tolower(*text) == "true")
    *boolean = true;
  else
    *boolean = false;
  return true;
}

bool st_recognize_integer(string *text, int *integer_value, string *before,
                          string *after) {
  if (!st_recognize_re(text, before, after, re_buf_integer))
    return false;
  *integer_value = atoi(text->c_str());
  return true;
}

bool st_recognize_inf(string *text, double *double_value, string *before,
                      string *after) {
  if (!st_recognize_re(text, before, after, re_buf_inf))
    return false;
  sscanf(text->c_str(), "%lf", double_value);
  return true;
}

bool st_recognize_nan(string *text, double *double_value, string *before,
                      string *after) {
  if (!st_recognize_re(text, before, after, re_buf_nan))
    return false;
  sscanf(text->c_str(), "%lf", double_value);
  return true;
}

bool st_recognize_error(string *text, string *before, string *after) {
  return st_recognize_re(text, before, after, re_buf_error);
}

bool st_recognize_double_number(string *text, double *double_value,
                                string *before, string *after) {
  if (!st_recognize_re(text, before, after, re_buf_double_number))
    return false;
  sscanf(text->c_str(), "%lf", double_value);
  return true;
}

bool st_recognize_double(string *text, double *double_value, string *before,
                         string *after) {
  if (!st_recognize_re(text, before, after, re_buf_double))
    return false;
  sscanf(text->c_str(), "%lf", double_value);
  return true;
}

bool st_recognize_quoted_string(string *text, string *value, string *before,
                                string *after) {
  if (!st_recognize_re(text, before, after, re_buf_quoted_string))
    return false;
  *value = *text;
  st_find_and_replace(value, "\\\"", "\"", NULL);
  *value = value->substr(1, value->size() - 2);
  return true;
}

bool st_recognize_space(string *text, string *before, string *after) {
  return st_recognize_re(text, before, after, re_buf_space);
}

double st_get_time() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (double)tv.tv_sec + (double)tv.tv_usec / 1000000;
}
