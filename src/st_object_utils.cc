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

#include <iostream>
#include <set>
#include <st_list.h>
#include <st_map.h>
#include <st_object.h>
#include <st_point.h>
#include <st_vector.h>
#include <string>

extern void prs_display_error(const string &);
extern void prs_display_message(const string &);

//#define DEBUG

/*
 * Defines macros to deal with unallocated memory objects
 */

set<st_object *> *allocated = NULL;

string obj_name(st_object *x) {
  if (is_a<st_integer *>(x))
    return "st_integer";
  if (is_a<st_double *>(x))
    return "st_double";
  if (is_a<st_string *>(x))
    return "st_string";
  if (is_a<st_list *>(x))
    return "st_list";
  if (is_a<st_map *>(x))
    return "st_map";
  if (is_a<st_vector *>(x))
    return "st_vector";
  if (is_a<st_point *>(x))
    return "st_point";
  return "unknown object";
}

void create_new_object(int &N, st_object *x) {
  if ((allocated) == NULL)
    allocated = new set<st_object *>();
#if defined(DEBUG)
  cout << "allocating " << x << endl;
#endif
  N++;
  allocated->insert(x);
#if defined(DEBUG)
  cout << "object allocated at " << allocated << "   " << allocated->count(x)
       << " size: " << allocated->size() << "\n";
  cout << "object name " << obj_name(x) << endl;
#endif
}

void destroy_and_print_name(int &N, st_object *x) {
#if defined(DEBUG)
  cout << "destroying " << x << endl;
#endif
  if (allocated->count(x)) {
    allocated->erase(allocated->find(x));
#if defined(DEBUG)
    cout << "object allocated at " << allocated << "   " << allocated->count(x)
         << " size: " << allocated->size() << "\n";
#endif
  }

  else {
#if defined(DEBUG)
    cout << "Warning, destroying an object that was not allocated (" << x << ")"
         << endl;
#endif
  }
  N--;
#if defined(DEBUG)
  if (!allocated->size())
    prs_display_message("Memory correctly deallocated");
#endif
}
