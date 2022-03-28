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

#include <st_list.h>

void st_list::copy(st_list const *from, st_list *to) const {
  list<st_object *>::const_iterator i;
  for (i = to->begin(); i != to->end(); i++) {
    if (*i)
      delete (*i);
  }
  for (i = from->the_list.begin(); i != from->the_list.end(); i++) {
    to->the_list.push_back((*i)->gen_copy());
  }
}

st_list::st_list(){obj_create(num_obj)}

st_list::st_list(const st_list &l)
    : st_object(l) {
  obj_create(num_obj);
  copy(&const_cast<st_list &>(l), this);
}

st_object *st_list::gen_copy() const {
  st_list *nl = new st_list(*this);
  return nl;
}

st_list::~st_list() {
  list<st_object *>::const_iterator i;
  for (i = the_list.begin(); i != the_list.end(); i++) {
    if (*i)
      delete (*i);
  }
}

void st_list::remove(st_object *p) {
  delete p;
  the_list.remove(p);
}

void st_list::erase(list<st_object *>::iterator &i) {
  delete (*i);
  the_list.erase(i);
}

void st_list::insert(const st_object &o) {
  the_list.push_back(const_cast<st_object &>(o).gen_copy());
}

string st_list::print() const {
  list<st_object *>::const_iterator i;
  string s = "{ ";
  for (i = the_list.begin(); i != the_list.end(); i++) {
    if (i != the_list.begin())
      s = s + ", ";
    s = s + (*i)->print();
  }
  s = s + " }";
  return s;
}

string st_list::full_print() const {
  list<st_object *>::const_iterator i;
  string s = "_obj_repr_ ";
  s = s + "{ ";
  for (i = the_list.begin(); i != the_list.end(); i++) {
    if (i != the_list.begin())
      s = s + ", ";
    s = s + (*i)->full_print() + " ";
  }
  s = s + "}";
  s = s + depth_print(properties);
  return s;
}

list<st_object *>::iterator st_list::begin() { return the_list.begin(); }

list<st_object *>::iterator st_list::end() { return the_list.end(); }

st_object const *st_list::back() { return the_list.back(); }

bool const st_list::empty() { return the_list.empty(); }

size_t st_list::size() { return the_list.size(); }

void st_list::concat(st_list const *l) {
  list<st_object *>::const_iterator it;
  for (it = l->the_list.begin(); it != l->the_list.end(); it++) {
    the_list.insert(the_list.end(), (*it)->gen_copy());
  }
}
