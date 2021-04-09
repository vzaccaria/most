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

#include <st_env.h>
#include <st_parser.h>
#include <st_shell_variables.h>
#include <string.h>

int show_progress = 0;

void st_display_message_assigned(st_env *env, string name,
                                 const st_object *val) {
  int sil;
  env->shell_variables.get_integer("silent", sil);
  if (name != "current_build_path" && name != "progress" && name != "?" &&
      !sil && name != "kernel_evaluations_number" &&
      name != "kernel_realistic_simulations" && name != "kernel_root_hits" &&
      name != "xml_design_space_file" &&
      name != "kernel_simulations_number") /** Hidden for make tests work :-P */
    prs_display_message("Assigned value " + val->print() + " to " + name);
}

extern int old_style_representation;

void st_set_fast_integer(const st_object *o, int *dest) {
  if (dynamic_cast<st_integer *>(const_cast<st_object *>(o)))
    *dest =
        dynamic_cast<st_integer *>(const_cast<st_object *>(o))->get_integer();
}

extern void st_set_realistic_simulation_mode(bool mode);
extern void sim_set_tcad_dominance(bool value);

void st_set_fast_variables(string name, const st_object &o) {
  if (name == "progress")
    st_set_fast_integer(&o, &show_progress);

  if (name == "old_style_representation")
    st_set_fast_integer(&o, &old_style_representation);

  if (name == "realistic_simulation_mode") {
    bool mode = false;
    if (dynamic_cast<st_integer *>(const_cast<st_object *>(&o)))
      mode = dynamic_cast<st_integer *>(const_cast<st_object *>(&o))
                 ->get_integer();

    st_set_realistic_simulation_mode(mode);
  }
  if (name == "tcad_dominance") {
    bool mode = false;
    if (dynamic_cast<st_integer *>(const_cast<st_object *>(&o)))
      mode = dynamic_cast<st_integer *>(const_cast<st_object *>(&o))
                 ->get_integer();

    prs_display_message("Setting TCAD dominance mode to '" +
                        string((mode) ? "true" : "false") + "'");
    sim_set_tcad_dominance(mode);
  }
}

void st_shell_variables::insert(string name, const st_object &o) {

  ((st_map *)(this))->insert(name, o);
  ((st_map *)(this))->set_global(name);

  st_set_fast_variables(name, o);

  st_display_message_assigned(&current_environment, name, &o);
}

void st_shell_variables::insert_dont_copy(string name, st_object *o) {

  ((st_map *)(this))->insert_dont_copy(name, o);
  ((st_map *)(this))->set_global(name);

  st_set_fast_variables(name, *o);

  st_display_message_assigned(&current_environment, name, o);
}

void st_shell_variables::set_integer(string name, int number) {
  insert(name, st_integer(number));
}

void st_shell_variables::set_double(string name, double number) {
  insert(name, st_double(number));
}

void st_shell_variables::set_string(string name, const char *s) {
  insert(name, st_string(s));
}

bool st_shell_variables::get_integer(string name, int &number) {
  st_object const *obj;
  if (!get(name.c_str(), obj)) {
    number = 0;
    return false;
  }
  if (!is_a<st_integer const *>(obj))
    return false;
  number = to<st_integer const *>(obj)->get_integer();
  return true;
}

bool st_shell_variables::get_string(string name, string &str) {

  st_object const *obj;
  if (!get(name.c_str(), obj)) {
    str = "";
    return false;
  }
  if (!is_a<st_string const *>(obj))
    return false;
  str = to<st_string const *>(obj)->get_string();
  return true;
}

bool st_shell_variables::get_double(string name, double &d) {

  st_object const *obj;
  if (!get(name.c_str(), obj)) {
    d = 0;
    return false;
  }
  if (!is_a<st_double const *>(obj))
    return false;
  d = to<st_double const *>(obj)->get_double();
  return true;
}

extern int mpi_max_parseable_size;

string st_shell_variables::print() {
  st_map_const_iterator k;
  string output = "";
  for (k = begin(); k != end(); k++) {
    string name = k->first;
    st_object *obj = k->second;
    st_assert(obj);
    string out = "set " + name + " = " + obj->full_print() + "\n";
    st_assert(strlen(out.c_str()) < mpi_max_parseable_size);
    output = output + out;
  }
  return output;
}

bool st_shell_variables::get_vector(string name, st_vector *&d) {

  st_object const *obj;
  if (!get(name.c_str(), obj)) {
    d = NULL;
    return false;
  }
  if (!is_a<st_vector const *>(obj))
    return false;

  d = const_cast<st_vector *>((to<st_vector const *>(obj)));

  return true;
}
