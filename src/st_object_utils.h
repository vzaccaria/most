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
#ifndef ST_UTIL
#define ST_UTIL

using namespace std;

#include <assert.h>
#include <stdexcept>

class st_object;

/*
 * Defines macros to deal with unallocated memory objects
 * and other functions to deal with possible exceptions during the execution
 */
#ifdef DEBUG
#define obj_destroy(N) destroy(N);
#else
#define obj_destroy(N)
#endif

#ifdef DEBUG
#define obj_create(N)                                                          \
  { create_new_object(N, ((st_object *)this)); }
#else
#define obj_create(N)
#endif

#ifdef DEBUG
#define obj_destroy_n(N, x) destroy_and_print_name(N, x);
#else
#define obj_destroy_n(N, x)
#endif

extern void destroy_and_print_name(int &, st_object *);
extern void create_new_object(int &, st_object *);
extern string obj_name(st_object *);

/*
 * assertions and weak assertions
 */

#ifdef DEBUG
#define st_assert(expr)                                                        \
  if (!(expr))                                                                 \
    throw std::logic_error(#expr);
#else
#define st_assert(expr) assert(expr)
#endif
#endif
