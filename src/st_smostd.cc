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
#include <libgen.h>
#include <math.h>
#include <pthread.h>
#include <regex.h>
#include <signal.h>
#include <st_lm.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/unistd.h>
#include <time.h>
#include <unistd.h>
#include <vector>

using namespace std;

void mostd_display_message(string s) { cout << "mostd msg: " << s << endl; }

void mostd_display_error(string s) { cout << "mostd err: " << s << endl; }

int shared_memory_id = 0;
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

bool st_snode_manager_is_active() {
  if (!st_check_if_node_manager_is_running())
    return false;

  int shmid = PRB_MOST_SHM;

  if (shmid < 0 && errno == EEXIST)
    return true;

  if (shmid >= 0)
    shmctl(shmid, IPC_RMID, NULL);

  return false;
}

void st_rm_dump_state(ofstream &fout, st_node_information *ni);
void st_rm_dump_state_cout(st_node_information *ni);

int main(int argc, char **argv) {
  if (!st_snode_manager_is_active()) {
    mostd_display_error("Mostd node manager not active.");
    exit(1);
  }
  int shmid = GET_MOST_SHM;

  if (shmid < 0) {
#if defined(_MOST_DEBUG_LICENSE_)
    prs_display_error("Cannot reach node manager");
#endif
    return -1;
  }

  current_ni = (st_node_information *)shmat(shmid, NULL, 0);

  if (current_ni == (st_node_information *)-1) {
#if defined(_MOST_DEBUG_LICENSE_)
    prs_display_error("Cannot attach to the node manager");
#endif
    return -1;
  }

  cout << "* Most node manager appears to be up and running *" << endl;
  cout << "Currently licensed nodes: " << (current_ni->licensed_nodes) << endl;
  cout << "Available nodes         : " << (current_ni->available_nodes) << endl;

  st_rm_dump_state_cout(current_ni);

  shmdt(current_ni);

  if (argc >= 2 && string("log") == argv[1])
    system("cat /var/run/mostd.log");
}
