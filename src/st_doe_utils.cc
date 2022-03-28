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

#include <dlfcn.h>
#include <iostream>
#include <map>
#include <st_common_utils.h>
#include <st_parser.h>

/** FIXME: merge this function with the similar function in opt_select_optimizer
 */
typedef st_doe *(*fun_type)();

bool doe_select_doe(st_env *env, string const &doe_name) {
  string c_doe_name; // complete doe name
  if (st_look_for_filename_in_search_path(env, doe_name, c_doe_name)) {
    void *h = dlopen(c_doe_name.c_str(), RTLD_NOW);
    if (h) {
      fun_type gendoe = (fun_type)dlsym(h, "doe_generate_doe");
      if (!gendoe) {
        return false;
      }
      if (env->current_doe != NULL) {
        delete env->current_doe;
        env->current_doe = NULL;
      }
      env->current_doe = gendoe();
      env->current_doe->handle = h;
      return true;
    }
  }
  return false;
}
