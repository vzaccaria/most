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

#ifndef ST_LIST
#define ST_LIST

#include <algorithm>
#include <iostream>
#include <list>
#include <map>
#include "st_object.h"
#include "st_object_utils.h"
#include <string>
#include <vector>

#define OBJP_TO_LIST(p) dynamic_cast<st_list *>(p)
#define OBJR_TO_LIST(r) dynamic_cast<st_list &>(r)

#define CONST_OBJP_TO_LIST(p) dynamic_cast<st_list const *>(p)
#define CONST_OBJR_TO_LIST(r) dynamic_cast<st_list const &>(r)

using namespace std;

class st_list : public st_object {
public:
  void copy(st_list const *from, st_list *to) const;
  list<st_object *> the_list;
  st_list();
  st_list(const st_list &l);
  ~st_list();

  void insert(const st_object &o);
  string print() const;
  string full_print() const;
  list<st_object *>::iterator begin();
  list<st_object *>::iterator end();
  st_object const *back();
  void remove(st_object *);
  void erase(list<st_object *>::iterator &);
  void concat(st_list const *);
  bool const empty();
  size_t size();
  st_object *gen_copy() const;
};

#endif
