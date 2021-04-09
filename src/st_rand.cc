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

#include <errno.h>
#include <fstream>
#include <iostream>
#include <math.h>
#include <pthread.h>
#include "st_env.h"
#include "st_object_utils.h"
#include "st_sim_utils.h"
#include "st_xdr_api.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/unistd.h>
#include <vector>

ofstream rand_out;
ifstream rand_in;

pthread_mutex_t rand_lock;

int st_rnd_flat(int a, int b) {
  vector<double> xstat;
  vector<double> ystat;

  pthread_mutex_lock(&rand_lock);
  xstat.push_back(a);
  xstat.push_back(b);

  if (!rand_out.good()) {
    pthread_mutex_unlock(&rand_lock);
    throw std::logic_error("Problems at the interface with the randomizer");
  }
  st_xdr_write_raw_data(rand_out, xstat, ystat);

  vector<double> raw_data;
  if (!rand_in.good()) {
    pthread_mutex_unlock(&rand_lock);
    throw std::logic_error("Problems at the interface with the randomizer");
  }

  if (!st_xdr_read_vector(rand_in, raw_data)) {
    pthread_mutex_unlock(&rand_lock);
    throw std::logic_error("Problems at the interface with the randomizer");
  }

  if (raw_data.size() < 1) {
    pthread_mutex_unlock(&rand_lock);
    throw std::logic_error("Problems at the interface with the randomizer");
  }
  pthread_mutex_unlock(&rand_lock);
  int pos = (int)floor((raw_data[0]));
  if (pos < a)
    pos = a;
  if (pos > b)
    pos = b;
  return pos;
}

double st_pow_double_to_int(double x, int y) { return pow(x, y); }

bool st_initialize_rand(st_env *env) {
  int res = mkfifo("rand_input", S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
  if ((res == -1) && (errno != EEXIST)) {
    return false;
  }
  res = mkfifo("rand_output", S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
  if ((res == -1) && (errno != EEXIST)) {
    return false;
  }
  string command = "rand";
  string cbp;
  if (env->shell_variables.get_string("current_build_path", cbp)) {
    command = cbp + "/bin/" + command;
  }
  if (!fork()) {
    /*prs_display_message(("Opening "+command).c_str());*/
#if !defined(__MAC_OSX__)
    if (!shell_command(command + " rand_output rand_input")) {
      cout << "Problems with the random number generator\n";
      exit(0);
    }
    exit(0);
#else
    int res =
        execl(command.c_str(), command.c_str(), "rand_output", "rand_input");
    if (res == -1) {
      cout << "Problems with the random number generator \n";
      exit(0);
    }
    exit(0);
#endif
  }
  rand_out.open("rand_output");
  rand_in.open("rand_input");
  pthread_mutex_init(&rand_lock, NULL);
  return true;
}
