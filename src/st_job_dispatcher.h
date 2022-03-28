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
#ifndef ST_JOB_DISPATCHER
#define ST_JOB_DISPATCHER

#include <iostream>
#include "st_list.h"
#include "st_map.h"
#include "st_parser.h" 
#include "st_point.h"
#include "st_shell_command.h"
#include "st_vector.h"

class st_job_dispatcher;

#include <pthread.h>
#include <regex.h>
#include "st_env.h" 
#include "st_mpi_utils.h"

struct st_batch_job {
  st_list list_of_points;

  st_batch_job() {}
  st_batch_job(const st_point &p) { list_of_points.insert(p); }
  ~st_batch_job() {}
  size_t size() { return list_of_points.size(); }
  st_point *get_point_at(int n) {
    list<st_object *>::iterator k;
    int i = 0;
    for (k = list_of_points.begin(); k != list_of_points.end(); k++) {
      if (i == n) {
        if ((*k) == NULL)
          return NULL;
        else
          return to<st_point *>((*k)->gen_copy());
      }
      i++;
    }
    return NULL;
  }
  void clear() {
    list<st_object *>::iterator k;
    for (k = list_of_points.begin(); k != list_of_points.end();) {
      list<st_object *>::iterator next = k;
      next++;
      list_of_points.erase(k);
      k = next;
    }
  }
  void send_to_node(int node) {
    st_mpi_send_integer(list_of_points.size(), node);
    list<st_object *>::iterator k;
    for (k = list_of_points.begin(); k != list_of_points.end(); k++) {
      st_object *t = *k;
      st_point *p = to<st_point *>(t);
      st_mpi_send_data(p->print_canonical(), node);
    }
  }
  void receive_from_node(int node) {
    clear();
    int sz = st_mpi_receive_integer(node);
    int i;
    for (i = 0; i < sz; i++) {
      string data = st_mpi_receive_data(node);
      st_object *point = prs_read_point_from_string(data);
      if (point)
        list_of_points.insert(*point);
      delete point;
    }
  }
};

class st_job_dispatcher {
public:
  bool submit(st_env *env, st_batch_job *the_job);
};

string st_get_unique_string_identifier();

class st_job_receiver {
public:
  bool run(st_env *);
};

#endif
