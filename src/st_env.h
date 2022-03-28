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

#ifndef ST_ENV
#define ST_ENV

class st_env;

#include "st_companion_metrics.h" 
#include "st_database.h" 
#include "st_driver.h" 
#include "st_doe.h" 
#include "st_job_dispatcher.h"   
#include "st_objectives_constraints.h" 
#include "st_optimizer.h" 
#include "st_rsm.h"
#include "st_shell_variables.h"

using namespace std;

class st_env {
private:
public:
  /** Map containing the shared variables */
  st_shell_variables shell_variables;

  /** Database containing the all the explorated points */
  st_database *source_database;

  /** Current destination database name */
  string current_destination_database_name;

  /** Available databases in memory */
  map<string, st_database *> available_dbs;

  /** Current driver linked to stshell */
  st_driver *current_driver;

  /** Current optimizer algorthm linked to stshell */
  st_optimizer *current_optimizer;

  /** Current doe */
  st_doe *current_doe;

  /** Current RSMs available */
  map<string, st_rsm *> current_rsms;

  /** Current parallel job dispatcher */
  st_job_dispatcher *current_dispatcher;

  /** Vector containing optimization objective expressions */
  vector<st_objective *> optimization_objectives;

  /** Vector containing optimization objective expressions */
  vector<st_constraint *> optimization_constraints;

  /** Vector containing companion metrics */
  vector<st_companion_metric *> companion_metrics;

  st_design_space *current_design_space;

  int optimization_timestamp;

  /** Constructor */
  st_env();

  /** Destructor */
  ~st_env();

  /** Changes the current db to another */
  bool change_current_destination_db_to(string name);

  /** Inserts a new database in the set of available dbs */
  void insert_new_database(st_database *db, string name);

  st_database *get_database(string name);

  /** Creates a new db, optionally populated */
  st_database *get_pre_populated_database();

  /** Removes an objective */
  bool remove_objective(string n);

  /** Removes an objective */
  bool remove_companion_metric(string n);

  /** Removes all objectives */
  void clear_objectives();

  /* */
  void clear_companion_metrics();

  /** Gets an objective index */
  int get_objective_index(string n);

  /** */
  int get_companion_metric_index(string n);

  /** Removes an objective */
  bool remove_constraint(string n);

  /** Removes all objectives */
  void clear_constraints();

  /** Destruction methods */
  void destroy_constraints();
  void destroy_objectives();
  void destroy_companion_metrics();

  /** Gets an objective index */
  int get_constraint_index(string n);
};

extern st_env current_environment;

#endif
