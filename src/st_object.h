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
#ifndef ST_OBJECTS
#define ST_OBJECTS

#include <algorithm>
#include <iostream>
#include <list>
#include <map>
#include "st_object_utils.h"
#include <string>
#include <typeinfo>
#include <vector>

using namespace std;

class st_map;
extern int num_obj;

/* All objects support polymorphic trees through gen_copy */
class st_object {

protected:
  map<string, st_object *> properties;
  bool temporary;
  bool local_temporary;

public:
  st_object();
  st_object(const st_object &);

  virtual ~st_object();
  bool get_properties(string s, st_object const *&);
  void set_properties(string s, const st_object &o);
  st_object &operator=(const st_object &);
  virtual string full_print() const = 0;
  virtual string print() const = 0;
  virtual st_object *gen_copy() const = 0;
  string type_name() { return typeid(*this).name(); }
  st_map *get_properties_map();
  void set_global();
  void set_stack_permanent();
  void unset_stack_permanent();
  bool is_global();
  bool is_stack_permanent();
};

/* Deletes an object value. If the value is either global or stack-permanent, it
 * is not deleted */
extern void st_object_discard(st_object *x);

/* Delets object values. If the value is global, it is not deleted.
 * Used by routines that setup and destroy stack frames. May destroy stack
 * permanent values. */
extern void st_object_discard_local(st_object *x);

extern string depth_print(map<string, st_object *> const &properties);

class st_string : public st_object {
  string the_string;

public:
  st_string(const char *);
  st_string(const string &s);
  st_string(const st_string &ss);
  ~st_string();
  string print() const;
  string const &get_string() const;
  st_object *gen_copy() const;
  string full_print() const;
};

class st_integer : public st_object {
  int the_int;

public:
  st_integer(const int &);
  st_integer(const st_integer &);
  ~st_integer();
  string print() const;
  int const &get_integer() const;
  st_object *gen_copy() const;
  string full_print() const;
};

class st_double : public st_object {
  double the_double;

public:
  st_double(const double &);
  st_double(const st_double &);
  ~st_double();
  string print() const;
  double const &get_double() const;
  st_object *gen_copy() const;
  string full_print() const;
};

/* New template framework for conversion between objects and real types */

template <class T> inline bool is_a(st_object const *p) {
  if (!dynamic_cast<T>(p))
    return false;
  return true;
}

template <class T> inline bool is_a(st_object *p) {
  if (!dynamic_cast<T>(p))
    return false;
  return true;
}

template <class T> inline T to(st_object const *p) {
  return dynamic_cast<T>(p);
}

template <class T> inline T to(st_object *p) { return dynamic_cast<T>(p); }

template <class T> inline T to(st_object const &p) {
  return dynamic_cast<T>(p);
}
#endif
