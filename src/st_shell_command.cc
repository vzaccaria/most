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

#include <st_shell_command.h>
#include <stdlib.h>
#include <string>

//#define DEBUG

bool shell_command(string command, bool out);

bool shell_command(string command) { return shell_command(command, false); }

bool shell_command(string command, bool out) {
  string outs;
  if (!out)
    outs = " 2>&1 > /dev/null";
  string sh_com = "/bin/bash -c \"(" + command + ")" + outs + " \"";
#if defined(DEBUG)
  cout << sh_com << " : executing " << endl;
#endif
  int rs = system(sh_com.c_str());
  if (rs == -1 || rs == 127) {
#if defined(DEBUG)
    cout << sh_com << " : failed - " << rs << endl;
#endif
    return false;
  }
  /** The upper 8 bits contain the exit status */
  rs = rs >> 8;
#if defined(DEBUG)
  cout << sh_com << " : " << ((rs) ? "FAIL" : "OK") << endl;
#endif
  if (rs)
    return false;
  return true;
}
