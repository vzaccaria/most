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
/* @M3EXPLORER_LICENSE_START@
 *
 * This file is part of the Multicube Explorer tool.
 *
 * Authors: Vittorio Zaccaria, Gianluca Palermo, Giovanni Mariani
 * Copyright (c) 2008, Politecnico di Milano, Universita' della Svizzera
 * italiana All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * Neither the name of Politecnico di Milano nor Universita' della Svizzera
 * Italiana nor the names of its contributors may be used to endorse or promote
 * products derived from this software without specific prior written
 * permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @M3EXPLORER_LICENSE_END@ */

/* @additional_authors @, Stefano Bolli (2009)@*/

#ifndef ST_DESIGN_SPACE
#define ST_DESIGN_SPACE

#include <algorithm>
#include <iostream>
#include <list>
#include <map>
#include "st_vector.h" 
#include <string>
#include <vector>

class st_design_space;

#include "st_env.h" 
#include "st_exception.h"

#define ST_SCALAR_TYPE_INTEGER 0
#define ST_SCALAR_TYPE_LIST 1

#define TWO_LEVEL_FF_MODE_CLASSIC 0
#define TWO_LEVEL_FF_MODE_SCRAMBLED 1

/** This class represents the scalar parameters within the design space.  */
struct st_scalar {
  /** The type can be either ST_SCALAR_TYPE_INTEGER or ST_SCALAR_TYPE_LIST */
  int type;

  /** Minimum used for ST_SCALAR_TYPE_INTEGER */
  int min;

  /** Maximum used for ST_SCALAR_TYPE_INTEGER */
  int max;

  /** Size of the symbol list associated to ST_SCALAR_TYPE_LIST */
  int list_size;

  /** Symbol list associated to ST_SCALAR_TYPE_LIST */
  vector<string> list;
};

/** This class represents the permutation parameters within the design space. */
struct st_permutation {
  /** True if the permutation has variable dimension */
  bool variable_dimension;

  /** Parameter the dimension depends on */
  string variable_dimension_parameter;

  /** Contains the fixed dimension of the permutation */
  int fixed_dimension;
};

/** This class represents the on off masks within the design space.  */
struct st_on_off_mask {
  /** True if the mask has variable dimension */
  bool variable_dimension;

  /** True if the on set has variable dimension */
  bool variable_on_set_size;

  /** True if the mask does not have a fixed on set size */
  bool no_on_set_size;

  /** Contains the fixed dimension of the mask */
  int fixed_dimension;

  /** Contains the fixed on set size of the mask */
  int fixed_on_set_size;

  /** Parameter the dimension depends on */
  string variable_dimension_parameter;

  /** Parameter the on set size depends on */
  string variable_on_set_size_parameter;
};

/** This class represents a pair of scrambled configuration.
 * This data structure is used both for mask and permutation. */
struct st_low_high_representation {
  /** Contains the low level for the representation */
  int low_level;

  /** Contains the high level for the representation */
  int high_level;
};

/** This class represents the parameters need to identify a low high mask pair.
 */
struct st_mask_low_high_parameter {
  /** The mask name */
  string mask_name;

  /** True if the mask has variable dimension */
  bool variable_dimension;

  /** True if the on set has variable dimension */
  bool variable_on_set_size;

  /** True if the mask does not have a fixed on set size */
  bool no_on_set_size;

  /** Contains the fixed dimension of the mask in a specific configuration */
  int on_set_size;

  /** Contains the fixed on set size of the mask in a specific configuration */
  int dimension;

  /** Contains the index where are available the low and high
      representation for mask parametrized as specified by above parameters */
  int index;
};

/** This class represents the parameters need to identify a low high permutation
 * pair. */
struct st_permutation_low_high_parameter {
  /** The permutation name */
  string permutation_name;

  /** True if the permutation has variable dimension */
  bool variable_dimension;

  /** Contains the fixed on set size of the permutation in a specific
      configuration */
  int dimension;

  /** Contains the index where are available the low and high representation for
      permutation parametrized as specified by above parameters */
  int index;
};

#define ST_DS_SCALAR 0
#define ST_DS_PERMUTATION 1
#define ST_DS_ON_OFF_MASK 2

/** This class represent a generic parameter */
struct st_parameter {
  /** Name of the parameter */
  string name;

  /** Type of the parameter. It can be:
   *
   * ST_DS_SCALAR
   * ST_DS_PERMUTATION
   * ST_DS_ON_OFF_MASK
   */
  int type;
};

//------------------------------------------------------------------------
//  function prototypes for internal/external representation conversion
//------------------------------------------------------------------------

vector<int> factoradic_to_permutation(int fact, int dimension);

int permutation_to_factoradic(vector<int> perm);

vector<int> int_to_bit_vector(int j, int dimension);

int bit_vector_to_int(vector<int> v);

//------------------------------------------------------------------------
//                  function prototypes for distance
//------------------------------------------------------------------------

double st_dice_distance(vector<int> x, vector<int> y);

int levenshtein_distance(vector<int> x, vector<int> y);

double normalized_levenshtein_distance(vector<int> x, vector<int> y);

int hamming_distance(vector<int> x, vector<int> y);

double normalized_hamming_distance(vector<int> x, vector<int> y);

double shift_factor(vector<int> x, vector<int> y);

double normalized_shift_factor(vector<int> x, vector<int> y);

double normalized_average_mask_distance(vector<int> x, vector<int> y);

double normalized_average_permutation_distance(vector<int> x, vector<int> y);

//------------------------------------------------------------------------
//                   others function prototypes
//------------------------------------------------------------------------
int on_set_size(vector<int> vect);

void print_vector(vector<int> x);

//------------------------------------------------------------------------

/** This class represents the design space associated to the current use case.
 * It contains the API for generating and navigating points
 * witin the design space. It also contains a set of API functions for
 * evolutionary algorithms.
 * */
class st_design_space {
public:
  vector<st_parameter> ds_parameters;
  vector<string> ds_parameters_names;
  map<string, st_scalar> scalar_parameters;
  map<string, st_permutation> permutation_parameters;
  map<string, st_on_off_mask> on_off_mask_parameters;
  map<string, int> ds_parameters_index;

  vector<string> metric_names;
  vector<string> unit_names;
  map<string, int> metric_index;

  /** The following will be removed */
  vector<string> objectives;
  vector<int> objectives_idx;

  vector<st_permutation_low_high_parameter> permutation_opposite_level_index;
  /** permutation parameter to the index at which the opposite configuration are
   * available in permutation_opposite_level */

  vector<st_low_high_representation> permutation_opposite_level;
  /** Contain the opposite 'low' and 'high' levels for the permutation */

  vector<vector<int> > permutation_scrambled_level;
  /** Contain the multiple scrambled value for scramble doe construction */

  vector<st_mask_low_high_parameter> mask_opposite_level_index;
  /** Map from mask parameter to the index at which the opposite configuration
   * are available in mask_opposite_level */

  vector<st_low_high_representation> mask_opposite_level;
  /** Contain the opposite 'low' and 'high' levels for the mask */

  vector<vector<int> > mask_scrambled_level;
  /** Contain the multiple scrambled value for scramble doe construction */

  st_design_space();

  int size() { return ds_parameters.size(); };

  void insert_metric(st_env *env, string name, string unit) {
    metric_names.push_back(name);
    unit_names.push_back(unit);
    metric_index.insert(pair<string, int>(name, metric_names.size() - 1));
  }

  /* DESIGN SPACE DEFINITION */

  void insert_scalar(st_env *env, string name, int type, int min, int max,
                     vector<string> list_of_values);
  void insert_integer(st_env *env, string name, int min, int max);
  void insert_permutation(st_env *env, string name, bool variable_dimension,
                          string variable_dimension_parameter,
                          int fixed_dimension);

  void insert_on_off_mask(st_env *env, string name, bool variable_dimension,
                          string variable_dimension_parameter,
                          int fixed_dimension, bool no_on_set_size,
                          bool variable_on_set_size,
                          string variable_on_set_size_parameter,
                          int fixed_on_set_size);

  int get_scalar_level(st_env *env, string par, string symbol);
  int get_number_of_scalar_levels(st_env *env, string par);
  string get_scalar_min_symbol(st_env *env, string parname);
  string get_scalar_max_symbol(st_env *env, string parname);
  int get_scalar_min(st_env *env, string parname);
  int get_scalar_max(st_env *env, string parname);
  int get_permutation_dimension(st_env *env, string parname, st_point *point);
  int get_permutation_min(st_env *env, string parname, st_point *point);
  int get_permutation_max(st_env *env, string parname, st_point *point);
  int get_mask_min(st_env *env, string parname, st_point *point);
  int get_mask_max(st_env *env, string parname, st_point *point);

  void set_permutation(st_env *env, st_point *point, string parname,
                       vector<int> v);
  void set_mask(st_env *env, st_point *point, string parname, vector<int> v);
  int get_mask_dimension(st_env *env, string parname, st_point *point);
  int get_mask_on_set_size(st_env *env, string parname, st_point *p);

  /** SUPPORT FOR VALIDATION */
  int look_for_next_valid_mask(st_env *env, st_point *point, string mask_name);
  bool is_valid(st_env *env, st_point *point);
  st_point consolidate(st_env *env, st_point point);
  bool is_within_scalar_bounds(st_env *env, st_point point);
  vector<string> get_scalar_range(st_env *env, string parname);
  int lazy_compute_size(st_env *env);

  /* SUPPORT FOR FULL SEARCH */
  st_point begin(st_env *env);
  bool next(st_env *env, st_point &point);

  /* SUPPORT FOR RANDOM SEARCH */
  st_point random_point_unsafe(st_env *env);
  st_point random_point(st_env *env);

  /* NEIGHBORHOOD SEARCH */
  void get_closest_points(st_env *env, st_point *point,
                          vector<st_point> &nearest);
  void get_closest_permutations(vector<int> start_permutation,
                                vector<int> &close_permutations);
  void get_closest_masks(vector<int> start_mask, bool no_on_set_size,
                         vector<int> &close_masks);

  void get_points_at_distance_N(st_env *env, st_point *point, int N,
                                vector<st_point> &nearest);
  st_point *get_random_point_at_distance_N(st_env *env, st_point *point, int N);
  st_point get_random_point_along_a_random_parameter(st_env *env, st_point &);

  /* FACTORIAL DESIGNS */
  void convert_two_level_factorial_representation_from_int(st_env *env,
                                                           st_point &point,
                                                           int i, string &res,
                                                           int mode);
  void convert_two_level_factorial_representation_from_int_multi(
      st_env *env, vector<st_point> &points, int i, string &res);
  bool convert_factorial_representation_multi(st_env *env,
                                              vector<st_point> &points,
                                              vector<string> the_string);

  bool convert_factorial_representation(st_env *env, st_point &point,
                                        vector<string> fact, int mode);
  void add_full_factorial_designs(st_env *env, st_vector *doe);
  void add_full_factorial_extended_designs(st_env *env, st_vector *doe);

  /* INTERNAL TO EXTERNAL REPRESENTATIONS */
  void print_factorial_representation(st_env *env, vector<string> &fact);
  void print_factorial_design(st_env *env, vector<string> &fact, int n);
  string get_point_representation(st_env *env, st_point &point);
  string get_point_representation_csv_i(st_env *env, st_point &current);
  string get_point_representation_csv_s(st_env *env, st_point &current);
  string get_parameter_representation(st_env *env, st_point &point,
                                      string parname);
  string get_vector_representation(st_env *env, vector<int> vector);
  vector<int> get_permutation(st_env *env, st_point *point, string parname);
  vector<int> get_mask(st_env *env, st_point *point, string parname);
  st_point string_vector_to_point(st_env *env, st_vector *vector);

  /* Extended database plot with levels */
  bool is_scalarf(string parameter_name);
  /** Use get_scalar_min and get_scalar_max */
  string get_symbol_from_scalar_level(string parameter_name, int level);

  bool evaluate(st_env *env, st_point *point);
  st_design_space *copy();
  void print(st_env *env);
  bool write_to_file(st_env *env, string name);
  void print_metric_index(st_env *env);
  bool convert_objectives_into_metrics(st_env *env);

  /* SCRAMBLED DOES */
  int get_permutation_opposite_level_index(st_permutation_low_high_parameter t);
  int get_permutation_opposite_level_index(st_env *env, st_point *p,
                                           string parname);
  int get_permutation_multi_opposite_level_index(st_env *env, st_point *p,
                                                 string parname);
  int get_mask_opposite_level_index(st_mask_low_high_parameter t);
  int get_mask_opposite_level_index(st_env *env, st_point *p, string parname);
  int get_mask_multi_opposite_level_index(st_env *env, st_point *p,
                                          string parname);
  int scramble_permutation_generator(
      st_env *env, string parname, st_point *p,
      st_permutation_low_high_parameter cur_permutation);
  int scramble_multi_permutation_generator(
      st_env *env, string parname, st_point *p,
      st_permutation_low_high_parameter cur_permutation);
  vector<int>
  permutation_scrambler(vector<int> start_permutation,
                        st_permutation_low_high_parameter cur_permutation);
  int scramble_mask_generator(st_env *env, string parname, st_point *p,
                              st_mask_low_high_parameter cur_mask);
  int scramble_multi_mask_generator(st_env *env, string parname, st_point *p,
                                    st_mask_low_high_parameter cur_mask);
  vector<int> mask_scrambler(vector<int> start_mask,
                             st_mask_low_high_parameter cur_mask);

  /* EVOLUTIONARY ALGORITHMS */
  void ea_init_operators();

private:
  enum crossover_operator_type {
    CROSSOVER_OPERATOR_01 = 0,
    CROSSOVER_OPERATOR_02
  } current_crossover_operator;

public:
  void ea_crossover_operator_selection(crossover_operator_type cot);
  st_point ea_crossover(st_env *env, st_point *parent_0, st_point *parent_1);

private:
  enum mutation_operator_type {
    MUTATION_OPERATOR_01 = 0,
    MUTATION_OPERATOR_02
  } current_mutation_operator;
  double mutation_operator_01_parameter;

public:
  void ea_mutation_operator_selection(mutation_operator_type mot);
  st_point ea_mutation(st_env *env, st_point *mutant);

  void ea_set_mutation_operator_01_parameter(double parameter);
  st_point genetic_crossover(st_env *env, st_point *parent_0,
                             st_point *parent_1);
  st_point genetic_mutation(st_env *env, st_point *mutant,
                            double p_gen_mutate = 0.2);
  void crossover_permutation(st_env *env, string perm_name, st_point *parent_0,
                             st_point *parent_1, st_point &child);
  void crossover_mask(st_env *env, string mask_name, st_point *parent_0,
                      st_point *parent_1, st_point &child);
  void mutate_permutation(st_env *env, st_point *point, string perm_name);
  void mutate_mask(st_env *env, st_point *point, string mask_name);
};

/** Design space exception class. */
class st_design_space_exception : public st_exception {
public:
  /** Constructor.
   * @param description of the exception */
  st_design_space_exception(string description);

  /** Destructur */
  ~st_design_space_exception() throw();
};

#endif
