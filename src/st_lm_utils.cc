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
#define __LICENSE_TIMEOUT__
#include <st_lm.h>

void st_rm_init_images(st_node_information *ni) {
  for (int i = 0; i < MAX_RUNNABLE_IMAGES; i++) {
    ni->images[i].valid = 0;
  }
}

bool st_rm_start_keep_alive_loop(st_node_information *ni, int image_index);

bool st_rm_get_nodes(st_node_information *ni, int nodes, int *image_index) {
  if (nodes > (ni->licensed_nodes)) {
    return false;
  }

  pthread_mutex_lock(&ni->ni_lock);
  if ((ni->available_nodes) < nodes) {
    pthread_mutex_unlock(&ni->ni_lock);
    return false;
  }

  bool found = false;
  for (int i = 0; i < MAX_RUNNABLE_IMAGES; i++) {
    if (ni->images[i].valid == 0) {
      *image_index = i;
      ni->images[i].valid = 1;
      ni->images[i].poke = ST_INIT_POKES;
      ni->images[i].acquired_nodes = nodes;
      ni->images[i].pid = getpid();
      ni->available_nodes = ((ni->available_nodes) - nodes);
      st_rm_start_keep_alive_loop(ni, i);
      pthread_mutex_unlock(&ni->ni_lock);
      return true;
    }
  }
  pthread_mutex_unlock(&ni->ni_lock);
  return false;
}

void st_rm_sub_nodes(st_node_information *ni, int image_index) {
  ni->available_nodes =
      ((ni->available_nodes) + ni->images[image_index].acquired_nodes);
  ni->images[image_index].valid = 0;

  if ((ni->available_nodes) > (ni->licensed_nodes))
    ni->available_nodes = ni->licensed_nodes;
}

bool st_rm_ka_shutdown();

bool st_rm_rel_nodes(st_node_information *ni, int image_index) {
  st_rm_ka_shutdown();

  pthread_mutex_lock(&ni->ni_lock);

  if (getpid() != ni->images[image_index].pid) {
    pthread_mutex_unlock(&ni->ni_lock);
    return false;
  }

  st_rm_sub_nodes(ni, image_index);

  pthread_mutex_unlock(&ni->ni_lock);

  return true;
}

bool st_rm_poke(st_node_information *ni, int image_index) {
  pthread_mutex_lock(&ni->ni_lock);

  if (getpid() != ni->images[image_index].pid) {
    pthread_mutex_unlock(&ni->ni_lock);
    return false;
  }
  ni->images[image_index].poke++;
  pthread_mutex_unlock(&ni->ni_lock);
}

void st_rm_dump_state_cout(st_node_information *ni) {
  pthread_mutex_lock(&ni->ni_lock);
  for (int i = 0; i < MAX_RUNNABLE_IMAGES; i++) {
    if (ni->images[i].valid == 1) {
      cout << "Image [" << i << "]: PID " << ni->images[i].pid
           << ", acquired nodes " << ni->images[i].acquired_nodes
           << " life count:" << ni->images[i].poke << endl;
    } else {
      cout << "Image [" << i << "]: Not allocated." << endl;
    }
  }
  pthread_mutex_unlock(&ni->ni_lock);
}

void st_rm_dump_state(ofstream &fout, st_node_information *ni) {
  pthread_mutex_lock(&ni->ni_lock);
  for (int i = 0; i < MAX_RUNNABLE_IMAGES; i++) {
    if (ni->images[i].valid == 1) {
      fout << "Image [" << i << "]: PID " << ni->images[i].pid
           << ", acquired nodes " << ni->images[i].acquired_nodes
           << " life count:" << ni->images[i].poke << endl;
    } else {
      fout << "Image [" << i << "]: Not allocated." << endl;
    }
  }
  pthread_mutex_unlock(&ni->ni_lock);
}

static pthread_t ka_thread;
static pthread_attr_t ka_attr;
static st_node_information *ka_ni;
static int ka_ii;

void *st_rm_ka_thread(void *arg) {
  while (true) {
    sleep(ST_SLICED_POKE_TIME);
    st_rm_poke(ka_ni, ka_ii);
  }
}

bool st_rm_start_keep_alive_loop(st_node_information *ni, int image_index) {
  pthread_attr_init(&ka_attr);
  pthread_attr_setscope(&ka_attr, PTHREAD_SCOPE_SYSTEM);
  pthread_attr_setschedpolicy(&ka_attr, SCHED_FIFO);
  ka_ni = ni;
  ka_ii = image_index;
  pthread_create(&ka_thread, &ka_attr, st_rm_ka_thread, NULL);
}

bool st_rm_ka_shutdown() {
  /* Should send a signal to the thread, it should go down anyway :) */
}

bool st_rm_decrease_pokes(st_node_information *ni) {
  pthread_mutex_lock(&ni->ni_lock);
  for (int i = 0; i < MAX_RUNNABLE_IMAGES; i++) {
    if (ni->images[i].valid == 1) {
      if (ni->images[i].poke > 0) {
        ni->images[i].poke--;
      } else {
        st_rm_sub_nodes(ni, i);
      }
    }
  }
  pthread_mutex_unlock(&ni->ni_lock);
}

void st_rm_death_loop(st_node_information *ni) {
  sleep(ST_SLICED_POKE_TIME);
  st_rm_decrease_pokes(ni);
}
