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

#include <st_map.h>

void st_map::copy(st_map const *from, st_map *to) const {
  map<string, st_object *>::iterator i;

  for (i = to->the_map.begin(); i != to->the_map.end(); i++) {
    if ((*i).second) {
      delete ((*i).second);
    }
  }
  to->the_map.erase(to->the_map.begin(), to->the_map.end());
  map<string, st_object *>::const_iterator j;
  for (j = from->the_map.begin(); j != from->the_map.end(); j++) {
    pair<string, st_object *> p(j->first, j->second->gen_copy());
    to->the_map.insert(p);
  }
}

st_map::st_map(){obj_create(num_obj)};

st_map::st_map(const st_map &l) : st_object(l) {
  obj_create(num_obj) copy(&l, this);
}

st_map::~st_map() {
  map<string, st_object *>::iterator i;
  for (i = the_map.begin(); i != the_map.end(); i++) {
    // cout << "Deleting object " << i->second << " at key " << i->first <<
    // endl;
    if (i->second)
      delete (i->second);
  }
}

void st_map::discard_temporary() {
  // cout << "Begin discarding temporaries" << endl;
  map<string, st_object *>::iterator i;
  for (i = the_map.begin(); i != the_map.end(); i++) {
    // cout << "Discarding object " << i->second << " Key: " << i->first <<
    // endl;
    st_object_discard(i->second);
    i->second = NULL;
  }
}

void st_map::set_global(string n) {
  map<string, st_object *>::iterator i;
  if ((i = the_map.find(n)) != the_map.end()) {
    i->second->set_global();
  }
}

void st_map::insert_dont_copy(string n, st_object *o) {
  map<string, st_object *>::iterator i;
  // cout << "Inserting object " << n << " address " << o << endl;
  if ((i = the_map.find(n)) != the_map.end()) {
    delete i->second;
  }
  the_map.erase(n);
  the_map.insert(map<string, st_object *>::value_type(n, o));
}

void st_map::insert(string n, const st_object &o) {

  map<string, st_object *>::iterator i;
  st_object *copy = o.gen_copy();

  // cout << "Creating copy element " << n << " address " << copy << endl;
  if ((i = the_map.find(n)) != the_map.end()) {
    delete i->second;
  }
  the_map.erase(n);
  the_map.insert(map<string, st_object *>::value_type(n, copy));
}

int st_map::count(string n) { return the_map.count(n); }

string st_map::print() const {
  map<string, st_object *>::const_iterator i;
  string s = "(* ";
  for (i = the_map.begin(); i != the_map.end(); i++) {
    s = s + i->first + "=" + i->second->print() + " ";
  }
  s = s + " *)";
  return s;
}

string st_map::full_print() const {
  map<string, st_object *>::const_iterator i;
  string s = "_obj_repr_ ";
  s = s + "(* ";
  for (i = the_map.begin(); i != the_map.end(); i++) {
    s = s + i->first + "=" + i->second->full_print() + " ";
  }
  s = s + " *)";
  s = s + depth_print(properties);
  return s;
}

void st_map::fill_properties_of(st_object *o) const {
  map<string, st_object *>::const_iterator i;
  for (i = the_map.begin(); i != the_map.end(); i++) {
    o->set_properties(i->first, *i->second);
  }
}

bool st_map::get(string n, st_object const *&p) const {
  map<string, st_object *>::const_iterator j;
  if (!the_map.count(n)) {
    p = NULL;
    return false;
  }
  j = the_map.find(n);
  p = (j->second);
  return true;
}

int st_map::get_integer(string n) {
  map<string, st_object *>::const_iterator j;
  st_object const *p;
  int r;
  if (!the_map.count(n)) {
    cout << print() << endl;
    throw std::logic_error("The map key " + n + " does not exist");
  }
  j = the_map.find(n);
  p = (j->second);

  if (!to<st_integer const *>(j->second)) {
    throw std::logic_error("The map value is not an integer ");
  }
  r = to<st_integer const *>(j->second)->get_integer();
  return r;
}

string st_map::get_string(string n) {
  map<string, st_object *>::const_iterator j;
  st_object const *p;
  string r;
  if (!the_map.count(n)) {
    throw std::logic_error("The map key does not exist");
  }
  j = the_map.find(n);
  p = (j->second);

  if (!to<st_string const *>(j->second)) {
    throw std::logic_error("The map value is not a string");
  }
  r = to<st_string const *>(j->second)->get_string();
  return r;
}

st_object *st_map::gen_copy() const {
  st_map *v = new st_map(*this);
  return v;
}

bool st_map::empty() const { return (the_map.size() == 0); }
