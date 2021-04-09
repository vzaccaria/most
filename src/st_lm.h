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
#include "st_sign.h"
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/unistd.h>
#include <time.h>
#include <unistd.h>
#include <vector>

using namespace std;

typedef struct {
  int nodes;
  long int hostid;
  tm to_date;
  tm from_date;
  string licensed_to;
} st_licensed_data;

typedef struct {
  int valid;
  int pid;
  int poke;
  int acquired_nodes;
} st_image_info;

#define MAX_RUNNABLE_IMAGES 8
#define ST_INIT_POKES 5
#define ST_SLICED_POKE_TIME 2

typedef struct {
  pthread_mutex_t ni_lock;
  int licensed_nodes;
  int available_nodes;
  st_image_info images[MAX_RUNNABLE_IMAGES];
} st_node_information;

#define st_ri_mask_full(m) (m == 0L)
#define st_ri_get_first_bit_set_m(m) (m & ~(m - 1))
#define st_ri_get_first_bit_set_v(m) (__builtin_ctz(m))

extern void st_rm_init_images(st_node_information *ni);
extern bool st_rm_get_nodes(st_node_information *ni, int nodes,
                            int *image_index);
extern bool st_rm_rel_nodes(st_node_information *ni, int image_index);
extern bool st_rm_poke(st_node_information *ni, int image_index);
extern bool st_rm_get_first_valid_expired_image(st_node_information *ni,
                                                int *expired);

#define PER 0770

#define PRB_MOST_SHM                                                           \
  shmget(st_get_unique_most_key(), sizeof(st_node_information),                \
         IPC_CREAT | IPC_EXCL | PER);

#define GET_MOST_SHM                                                           \
  shmget(st_get_unique_most_key(), sizeof(st_node_information), PER);

#define CRT_MOST_SHM                                                           \
  shmget(st_get_unique_most_key(), sizeof(st_node_information),                \
         IPC_CREAT | PER);

bool st_node_manager_is_active();
