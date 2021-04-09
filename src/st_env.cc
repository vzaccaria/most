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
#include <algorithm>
#include <st_commands.h>
#include <st_env.h>
#include <st_opt_utils.h>
#include <st_parser.h>

using namespace std;

st_env current_environment;

st_env::st_env() {
  available_dbs["root"] = new st_database();
  current_destination_database_name = "root";
  source_database = available_dbs["root"];
  current_optimizer = NULL;
  current_driver = NULL;
  current_design_space = NULL;
  current_dispatcher = new st_job_dispatcher();
  rsm_init_rsms(&current_environment);
  optimization_timestamp = 0;
}

st_env::~st_env() {
  /* delete current_database; */

  map<string, st_database *>::iterator j;
  for (j = available_dbs.begin(); j != available_dbs.end(); j++) {
    delete j->second;
  }
  delete current_dispatcher;

  destroy_objectives();
  destroy_constraints();
  destroy_companion_metrics();
}

int st_env::get_objective_index(string n) {
  for (int i = 0; i < optimization_objectives.size(); i++) {
    if (optimization_objectives[i]->name == n)
      return i;
  }
  throw std::logic_error("Constraint not found");
}

extern vector<string> possible_commands;

bool st_env::remove_objective(string n) {
  vector<st_objective *>::iterator r;
  bool cont = true;
  bool found = false;
  while (cont) {
    cont = false;
    for (r = optimization_objectives.begin();
         r != optimization_objectives.end(); r++) {
      if ((*r)->name == n) {
        cont = true;
        found = true;
        delete (*r);
        current_environment.optimization_objectives.erase(r);
        st_remove_possible_command(n);
        optimization_timestamp++;
        break;
      }
    }
  }
  return found;
}

bool st_env::remove_companion_metric(string n) {
  vector<st_companion_metric *>::iterator r;
  bool cont = true;
  bool found = false;
  while (cont) {
    cont = false;
    for (r = companion_metrics.begin(); r != companion_metrics.end(); r++) {
      if ((*r)->name == n) {
        cont = true;
        found = true;
        delete (*r);
        current_environment.companion_metrics.erase(r);
        st_remove_possible_command(n);
        optimization_timestamp++;
        break;
      }
    }
  }
  return found;
}

void st_env::destroy_objectives() {
  vector<st_objective *>::iterator r;
  for (r = optimization_objectives.begin(); r != optimization_objectives.end();
       r++) {
    delete (*r);
  }
  optimization_objectives.clear();
}

void st_env::clear_objectives() {
  vector<st_objective *>::iterator r;
  for (r = optimization_objectives.begin(); r != optimization_objectives.end();
       r++) {
    st_remove_possible_command((*r)->name);
    delete (*r);
  }
  optimization_objectives.clear();
  optimization_timestamp++;
}

void st_env::destroy_companion_metrics() {
  vector<st_companion_metric *>::iterator r;
  for (r = companion_metrics.begin(); r != companion_metrics.end(); r++) {
    delete (*r);
  }
  companion_metrics.clear();
}

void st_env::clear_companion_metrics() {
  vector<st_companion_metric *>::iterator r;
  for (r = companion_metrics.begin(); r != companion_metrics.end(); r++) {
    st_remove_possible_command((*r)->name);
    delete (*r);
  }
  companion_metrics.clear();
  optimization_timestamp++;
}

void st_env::destroy_constraints() {
  vector<st_constraint *>::iterator r;
  for (r = optimization_constraints.begin();
       r != optimization_constraints.end(); r++) {
    delete (*r);
  }
  optimization_constraints.clear();
}

void st_env::clear_constraints() {
  vector<st_constraint *>::iterator r;
  for (r = optimization_constraints.begin();
       r != optimization_constraints.end(); r++) {
    delete (*r);
  }
  optimization_constraints.clear();
  optimization_timestamp++;
}

bool st_env::remove_constraint(string n) {
  vector<st_constraint *>::iterator r;
  bool cont = true;
  bool found = false;
  while (cont) {
    cont = false;
    for (r = optimization_constraints.begin();
         r != optimization_constraints.end(); r++) {
      if ((*r)->name == n) {
        cont = true;
        found = true;
        delete (*r);
        optimization_timestamp++;
        current_environment.optimization_constraints.erase(r);
        break;
      }
    }
  }
  return found;
}

int st_env::get_constraint_index(string n) {
  for (int i = 0; i < optimization_constraints.size(); i++) {
    if (optimization_constraints[i]->name == n)
      return i;
  }
  throw std::logic_error("Constraint not found");
}

int st_env::get_companion_metric_index(string n) {
  for (int i = 0; i < companion_metrics.size(); i++) {
    if (companion_metrics[i]->name == n)
      return i;
  }
  throw std::logic_error("Metric not found");
}

bool st_env::change_current_destination_db_to(string name) {
  if (name == "")
    return false;
  current_destination_database_name = name;
  return true;
}

st_database *st_env::get_database(string name) {
  if (available_dbs.count(name.c_str())) {
    return available_dbs[name.c_str()];
  }
  return NULL;
}

st_database *st_env::get_pre_populated_database() {
  string prp_db;
  st_database *F = new st_database;
  if (shell_variables.get_string("prepopulated_db", prp_db)) {
    if (available_dbs.count(prp_db)) {
      prs_display_message(
          ("Using pre-populated database '" + prp_db + "' ").c_str());
      *F = (*available_dbs[prp_db]);
    }
  }
  return F;
}

void st_env::insert_new_database(st_database *db, string name) {
  bool is_current = false;
  if (available_dbs.count(name.c_str())) {
    delete available_dbs[name.c_str()];
  }
  available_dbs[name.c_str()] = db;
  prs_display_message(("Database '" + name + "' created").c_str());
}
