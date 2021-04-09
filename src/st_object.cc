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

#include <iomanip>
#include <sstream>
#include "st_map.h"
#include "st_object.h"
#include "st_object_utils.h"
#include "st_parser.h"

int num_obj;

st_object::st_object() {
  temporary = true;
  local_temporary = true;
  //  cout << "debug: base object created, " << this << endl;
}

st_object::st_object(const st_object &o) {
  temporary = true;
  local_temporary = true;
  map<string, st_object *>::const_iterator i;
  for (i = o.properties.begin(); i != o.properties.end(); i++) {
    properties[i->first] = i->second->gen_copy();
  }
}

void st_object::set_global() { temporary = false; }

void st_object::set_stack_permanent() { local_temporary = false; }

void st_object::unset_stack_permanent() { local_temporary = true; }

bool st_object::is_global() { return !temporary; }

bool st_object::is_stack_permanent() { return !local_temporary; }

void st_object_discard(st_object *x) {
  if (x) {
    if (!(x->is_global()) && !(x->is_stack_permanent())) {
      // cout << "Deleting " << x << " " << x->print() << endl;
      delete x;
    }
  }
}

void st_object_discard_local(st_object *x) {
  if (x) {
    if (!(x->is_global())) {
      // cout << "Deleting " << x << " " << x->print() << endl;
      delete x;
    }
  }
}

st_object &st_object::operator=(const st_object &o) {
  map<string, st_object *>::iterator k;

  for (k = properties.begin(); k != properties.end(); k++) {
    delete ((*k).second);
  }

  properties.erase(properties.begin(), properties.end());

  map<string, st_object *>::const_iterator i;
  for (i = o.properties.begin(); i != o.properties.end(); i++) {
    properties[i->first] = i->second->gen_copy();
  }
  return *this;
}

bool st_object::get_properties(string s, st_object const *&p) {
  if (!properties.count(s)) {
    p = NULL;
    return false;
  } else {
    p = properties[s];
    return true;
  }
}

void st_object::set_properties(string s, const st_object &o) {
  if (properties.count(s))
    delete properties[s];
  properties[s] = o.gen_copy();
}

st_object *dont_destroy = NULL;
st_object::~st_object() {
  if (dont_destroy != NULL && this == dont_destroy)
    assert(false);
  // dont_destroy = this;
  obj_destroy_n(num_obj, this) map<string, st_object *>::iterator i;
  for (i = properties.begin(); i != properties.end(); i++) {
    delete ((*i).second);
  }
}

st_map *st_object::get_properties_map() {
  st_map *m = new st_map();
  map<string, st_object *>::const_iterator i;
  for (i = properties.begin(); i != properties.end(); i++) {
    m->insert(i->first, *(i->second));
  }
  return m;
}

st_string::st_string(const char *s) : the_string(s) { obj_create(num_obj); }

st_string::st_string(const string &s) : the_string(s) { obj_create(num_obj); }

st_string::st_string(const st_string &ss)
    : the_string(ss.the_string), st_object(ss) {
  obj_create(num_obj);
}

st_object *st_string::gen_copy() const { return (new st_string(*this)); }

string st_string::print() const { return "\"" + the_string + "\""; }

string const &st_string::get_string() const { return the_string; }

st_string::~st_string() {
  //  cout << "Deleting string '" << the_string << "'\n";
}

string st_string::full_print() const {
  string s = "_obj_repr_ ";
  s = s + print() + " ";
  s = s + depth_print(properties);
  return s;
}

st_integer::st_integer(const int &i) : the_int(i) { obj_create(num_obj); }

st_integer::st_integer(const st_integer &i) : st_object(i), the_int(i.the_int) {
  obj_create(num_obj)
}

int const &st_integer::get_integer() const { return the_int; }

st_integer::~st_integer() {}

string st_integer::print() const {
  ostringstream str;
  str << the_int;
  return str.str();
}

string st_integer::full_print() const {
  string s = "_obj_repr_ ";
  s = s + print() + " ";
  s = s + depth_print(properties);
  return s;
}

st_object *st_integer::gen_copy() const { return new st_integer(*this); }

st_double::st_double(const double &i) : the_double(i) { obj_create(num_obj); }

st_double::st_double(const st_double &d)
    : st_object(d), the_double(d.the_double) {
  obj_create(num_obj)
}

double const &st_double::get_double() const { return the_double; }

st_double::~st_double() {}

string st_double::print() const {
  ostringstream str;
  str << fixed << setprecision(6) << the_double;
  return str.str();
}

string depth_print(map<string, st_object *> const &properties) {
  map<string, st_object *>::const_iterator i;
  string s = "(* ";
  for (i = properties.begin(); i != properties.end(); i++) {
    s = s + i->first + "=" + i->second->full_print() + " ";
  }
  s = s + " *)";
  return s;
}

string st_double::full_print() const {
  string s = "_obj_repr_ ";
  s = s + print() + " ";
  s = s + depth_print(properties);
  return s;
}

st_object *st_double::gen_copy() const { return new st_double(*this); }
