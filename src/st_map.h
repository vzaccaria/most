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
#ifndef ST_MAP
#define ST_MAP

#include <algorithm>
#include <iostream>
#include <list>
#include <map>
#include "st_object.h"
#include "st_object_utils.h"
#include <string>
#include <vector>

#define OBJP_TO_MAP(p) dynamic_cast<st_map *>(p)

typedef map<string, st_object *>::const_iterator stl_map_const_iter;

struct st_map_const_iterator : public stl_map_const_iter {
  st_map_const_iterator(){};
  st_map_const_iterator(const stl_map_const_iter &i) : stl_map_const_iter(i) {}
};

class st_map : public st_object {
  map<string, st_object *> the_map;
  void copy(st_map const *from, st_map *to) const;

public:
  st_map();
  st_map(const st_map &l);
  ~st_map();

  void insert(string n, const st_object &o);
  void insert_dont_copy(string n, st_object *);
  bool get(string, st_object const *&) const;
  size_t size() const { return the_map.size(); }
  string print() const;
  string full_print() const;
  st_map_const_iterator begin() const { return the_map.begin(); }
  st_map_const_iterator end() const { return the_map.end(); }
  void fill_properties_of(st_object *o) const;
  int count(string n);
  st_object *gen_copy() const;
  bool empty() const;
  void set_global(string n);
  void discard_temporary();

  /* Simpler accessor methods, added Sept. 2011 */
  string get_string(string n);
  int get_integer(string n);
};

#endif
