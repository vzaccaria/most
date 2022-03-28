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

#include <st_vector.h>

void st_vector::copy(st_vector const *from, st_vector *to) const {
  int i;

  for (i = 0; i < to->the_vector.size(); i++) {
    if (to->the_vector[i]) {
      delete (to->the_vector[i]);
      to->the_vector[i] = NULL;
    }
  }
  to->the_vector.resize(from->the_vector.size(), NULL);
  for (i = 0; i < from->the_vector.size(); i++) {
    if (from->the_vector[i])
      to->the_vector[i] = from->the_vector[i]->gen_copy();
  }
}

st_vector::st_vector(){obj_create(num_obj)};

st_vector::st_vector(const st_vector &l) : st_object(l) {
  obj_create(num_obj) copy(&l, this);
}

st_vector::st_vector(const vector<double> &x) {
  obj_create(num_obj);
  for (int i = 0; i < x.size(); i++) {
    insert(i, st_double(x[i]));
  }
}

st_vector::st_vector(const st_object **v, int n) {
  obj_create(num_obj);
  for (int i = 0; i < n; i++) {
    insert(i, *(v[i]));
  }
}

st_vector::~st_vector() {
  vector<st_object *>::iterator i;
  for (i = the_vector.begin(); i != the_vector.end(); i++) {
    if (*i)
      delete (*i);
  }
}

size_t st_vector::size() const { return the_vector.size(); }

void st_vector::insert(int n, const st_object &o) {
  if (n < the_vector.size()) {
    if (the_vector[n])
      delete the_vector[n];
  }
  if (n >= the_vector.size()) {
    the_vector.resize(n + 1, NULL);
  }
  st_object *obj = o.gen_copy();
  the_vector[n] = obj;
}

string st_vector::print() const {
  vector<st_object *>::const_iterator i;
  string s = "[ ";
  for (i = the_vector.begin(); i != the_vector.end(); i++) {
    if (i != the_vector.begin())
      s = s + ", ";
    if (*i)
      s = s + (*i)->print();
    else
      s = s + " / ";
  }
  s = s + " ]";
  return s;
}

string st_vector::full_print() const {
  vector<st_object *>::const_iterator i;
  string s = "_obj_repr_ ";
  s = s + "[ ";
  for (i = the_vector.begin(); i != the_vector.end(); i++) {
    if (i != the_vector.begin())
      s = s + ", ";
    if (*i)
      s = s + (*i)->full_print() + " ";
    else
      s = s + " / ";
  }
  s = s + "]";
  s = s + depth_print(properties);
  return s;
}

st_object const &st_vector::get(int n) const {
  st_assert(the_vector[n]);
  return (*the_vector[n]);
}

st_object *st_vector::gen_copy() const {
  st_vector *v = new st_vector(*this);
  return v;
}

string st_vector::get_string_at(int index) {
  if (the_vector.size() < index)
    throw std::logic_error("Index is out of bounds");

  if (!is_a<st_string *>(the_vector[index]))
    throw std::logic_error("Get string failed");

  return to<st_string *>(the_vector[index])->get_string();
}

#include <st_exception.h>
#include <st_list.h>

vector<string> get_string_vector(st_object *v) {
  if (is_a<st_vector *>(v)) {
    vector<st_object *> &the_vector = to<st_vector *>(v)->the_vector;
    vector<string> ret;
    vector<st_object *>::const_iterator i;
    for (i = the_vector.begin(); i != the_vector.end(); i++) {
      if (!is_a<st_string *>(*i))
        throw st_exception("cannot convert into a string vector");
      ret.push_back(to<st_string *>(*i)->get_string());
    }
    return ret;
  } else {
    st_assert(is_a<st_list *>(v));
    {
      list<st_object *> &the_vector = to<st_list *>(v)->the_list;
      vector<string> ret;
      list<st_object *>::const_iterator i;
      for (i = the_vector.begin(); i != the_vector.end(); i++) {
        if (!is_a<st_string *>(*i))
          throw st_exception("cannot convert into a string vector");
        ret.push_back(to<st_string *>(*i)->get_string());
      }
      return ret;
    }
  }
}
