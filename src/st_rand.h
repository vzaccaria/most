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

#include <math.h>

#ifndef _RAND_H
#define _RAND_H

double st_pow_double_to_int(double, int);
void st_initialize_rand(st_env *env);
int st_rnd_flat(int a, int b);

#define rnd_flat(a, b) st_rnd_flat(a, b)
#define rnd_init() st_initialize_rand(&current_environment);
#define rnd_float_prec (1000000000.0)

/* The following returns a number between 0 and 1 */
#define rnd_flat_float()                                                       \
  (((float)rnd_flat(0, rnd_float_prec - 1)) / ((float)rnd_float_prec))

#define pow_double_to_int(d, i) st_pow_double_to_int(d, i)

#endif
