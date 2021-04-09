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
#include "st_common_utils.h"
#include "st_env.h"
#include "st_lm.h"
#include "st_object_utils.h"
#include "st_sign.h"
#include "st_sim_utils.h"
#include "st_xdr_api.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/unistd.h>
#include <time.h>
#include <unistd.h>
#include <vector>

using namespace std;

/* number of acquired_nodes */
int acquired_nodes = 0;
int shared_memory_id = 0;

pthread_mutex_t current_ni_lock;
st_node_information *current_ni = NULL;

#include <unistd.h>

/* Here we say the last word on compatibility with BSD */

bool st_check_if_node_manager_is_running() {
  FILE *file = fopen("/var/run/mostd.pid", "r");
  if (!file) {
    return false;
  }
  char pid_line[32];
  if (!fgets(pid_line, sizeof(pid_line), file)) {
    fclose(file);
    return false;
  }
  fclose(file);
  /* cout << "Node manager pid " << strlen(pid_line) << " " << pid_line; */
  pid_line[strlen(pid_line) - 1] = '\0';
  if (access((string("/proc/") + pid_line).c_str(), F_OK) == 0)
    return true;
  else
    return false;
}

bool st_node_manager_init(st_env *env) {

  int shmid = GET_MOST_SHM;

  if (shmid < 0) {
#if defined(_MOST_DEBUG_LICENSE_)
    prs_display_error("Cannot reach node manager");
#endif
    return false;
  }

  current_ni = (st_node_information *)shmat(shmid, NULL, 0);

  pthread_mutex_init(&current_ni_lock, NULL);

  if (current_ni == (st_node_information *)-1) {
#if defined(_MOST_DEBUG_LICENSE_)
    prs_display_error("Cannot attach to the node manager");
#endif
    return false;
  }

  shared_memory_id = shmid;

  return true;
}

bool st_rm_get_nodes(st_node_information *ni, int nodes, int *image_index);

int image_index;

bool st_node_manager_acquire_licensed_nodes(st_env *env, int n) {
  return st_rm_get_nodes(current_ni, n, &image_index);
}

bool st_rm_rel_nodes(st_node_information *ni, int image_index);

bool st_node_manager_release_licensed_nodes(st_env *env, int n) {
  return st_rm_rel_nodes(current_ni, image_index);
}

bool st_connect_to_node_manager(st_env *env) {
  int required_nodes = 1;
#if defined(__MOST_NO_LICENSE__)
  required_nodes = st_mpi_get_number_of_required_nodes();
  acquired_nodes = required_nodes;
  return true;
#endif
  required_nodes = st_mpi_get_number_of_required_nodes();
  if (st_node_manager_is_active()) {
#if defined(_MOST_DEBUG_LICENSE_)
    prs_display_message("License node manager active");
#endif
    if (!st_node_manager_init(env)) {
      prs_display_message("Can't grab information from license node manager");
      return false;
    }
  } else {
    prs_display_message("Node manager not active");
    return false;
  }

  if (!st_node_manager_acquire_licensed_nodes(env, required_nodes)) {
    return false;
  }
  acquired_nodes = required_nodes;
  return true;
}

bool st_disconnect_from_node_manager(st_env *env) {
#if defined(__MOST_NO_LICENSE__)
  return true;
#endif
  /* We need the following because a signal can be generated when more than one
   * thread is executing */
  pthread_mutex_lock(&current_ni_lock);
  if ((current_ni != NULL) && (current_ni != (st_node_information *)-1)) {
    st_node_manager_release_licensed_nodes(env, acquired_nodes);
    shmdt(current_ni);
  }
  current_ni = NULL;
  pthread_mutex_unlock(&current_ni_lock);
  return true;
}

bool st_node_manager_is_active() {
#if defined(__MOST_NO_LICENSE__)
  return true;
#endif
  if (!st_check_if_node_manager_is_running()) {
    prs_display_error("Node manager process not running");
    return false;
  }

  /* We check for the daemon pid, the following is not needed anymore */

  /*
  int shmid = PRB_MOST_SHM;

  if(shmid < 0 && errno == EEXIST)
      return true;

  if(shmid >=0)
      shmctl(shmid, IPC_RMID, NULL);
  */

  return true;
}
