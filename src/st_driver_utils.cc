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

#include <dlfcn.h>
#include <iostream>
#include <map>
#include <st_common_utils.h>
#include <st_parser.h>

/** FIXME: merge this function with the similar function in opt_select_optimizer
 */
typedef st_driver *(*fun_type)();

bool drv_select_driver(st_env *env, string const &drv_name) {
  string c_drv_name; // complete driver name
  if (st_look_for_filename_in_search_path(env, drv_name, c_drv_name)) {
    void *h = dlopen(c_drv_name.c_str(), RTLD_NOW);
    if (h) {
      fun_type gendrv = (fun_type)dlsym(h, "drv_generate_driver");
      if (!gendrv) {
        cout << dlerror() << endl;
        return false;
      }
      if (env->current_driver != NULL) {
        delete env->current_driver;
        env->current_driver = NULL;
      }
      try {
        env->current_driver = gendrv();
        env->current_driver->handle = h;
        return true;
      } catch (exception &e) {
        prs_display_error("Exception raised when loading the driver");
        return false;
      }
    } else {
      cout << dlerror() << endl;
    }
  } else {
    prs_display_error("the specified driver is not in the search path");
  }
  return false;
}
