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

#include <config.h>
#include <st_command_list.h>
#include <st_common_utils.h>

extern void st_init_rsm_command_help();
extern map<string, st_command> rsm_c_help;

// st_command *st_get_help_from_module(string module);
// set<string> st_get_modules_in_search_path();

int st_generate_manual() {
  /* Initialize the commands and modules for which the help is available */
  st_init_commands();
  st_init_rsm_command_help();

  map<string, st_command>::iterator index;
  for (index = st_commands.begin(); index != st_commands.end(); index++) {
    (index)->second.gen_man(index->first, true);
  }

  for (index = rsm_c_help.begin(); index != rsm_c_help.end(); index++) {
    (index)->second.gen_man(index->first, true, "manual_rsm.txt");
  }

  set<string> modules = st_get_modules_in_search_path();
  set<string>::iterator id;

  for (id = modules.begin(); id != modules.end(); id++) {
    st_command *ch = st_get_help_from_module(*id);
    if (ch) {
      if (ch->alt_command_type == "opt")
        ch->gen_man(*id, true, "manual_optimizers.txt");
      else
        ch->gen_man(*id, true, "manual_doe.txt");
      delete ch;
    }
  }

  return 0;
}
