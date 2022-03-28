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

/*
 *  Note: This source file has been written by Fabrizio Castro while
 *  at Politecnico di Milano in 2008.
 */

#include <iostream>
#include <vector>
#include "st_design_space.h"
#include "st_object.h"
#include "st_map.h"
#include "st_vector.h"
#include "st_shell_variables.h"
#include "st_parser.h"
#include "st_optimizer.h"
#include "st_opt_utils.h"
#include "st_sim_utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <math.h>
#include "st_commands.h"
#include "st_rand.h"
#include <set>
#include <limits>

#define INFINITE numeric_limits<double>::infinity()

#define MIN_POPULATION_SIZE 10
#define DEFAULT_GENERATIONS_NUMBER 10
#define DEFAULT_MAX_TRYING_CONFIGURATIONS 20

//#define DEBUG

typedef map<int, set<st_point *> > nondomination_sorted_set;
typedef map<st_point *, double> crowding_distances_map;
typedef map<st_point *, int> ranks_map;

#define DEFAULT_SELECTION_OPERATOR 1
#define SELECTION_OPERATOR_CHOICES 2
string selection_operators[SELECTION_OPERATOR_CHOICES] = {"CLASSES",
                                                          "BINARY_TOURNAMENT"};

class st_nsgaii : public st_optimizer {
private:
  int N; // population_size
  int generations_number;
  int explored_points;
  int max_configurations;
  map<uint, double> f_max, f_min;

public:
  st_nsgaii() { explored_points = 0; }
  ~st_nsgaii() {}
  string get_information();
  int explore(st_env *env);

private:
  // generic stuff

  void init_internal_data(st_env *env);
  st_database *get_initial_population(st_env *env, st_vector *doe);
  st_point *simulate_point(st_env *env, st_point *point);
  void set_to_database(set<st_point *> *points_set, st_database *database);
  vector<float> get_float_vector_from_shell(st_env *env, string vector_name);

  // NSGAII

  nondomination_sorted_set
  fast_non_dominated_sort(st_database *initial_population, st_env *env,
                          ranks_map *ranks);

  void crowding_distance_assignment(
      st_env *env, set<st_point *> *points_set, int objectives_number,
      crowding_distances_map *distances) throw(exception);

  bool crowded_comparison_operator(st_point *i, st_point *j,
                                   crowding_distances_map *distances,
                                   ranks_map *ranks);

  map<uint, st_point *> sort_objective_ascending(st_env *env,
                                                 set<st_point *> points_set,
                                                 int m) throw(exception);

  map<uint, st_point *>
  sort_distances_descending(set<st_point *> points_set,
                            crowding_distances_map *distances,
                            ranks_map *ranks);

  void update_min_max(st_env *env, st_database *db,
                      int objectives_number) throw(exception);

  void delete_rejected(st_database *Rt, st_database *Pt);

  // EA

  st_database *make_new_population(st_env *env, nondomination_sorted_set *F,
                                   st_database *Pt,
                                   crowding_distances_map *distances,
                                   ranks_map *ranks);

  // selection operator stuff

  enum selection_operator_type {
    SELECTION_CLASSES = 0,
    SELECTION_BINARY_TOURNAMENT
  } selection_operator;

  void identify_population_classes(nondomination_sorted_set *F,
                                   map<uint, st_point *> *best,
                                   map<uint, st_point *> *middle,
                                   map<uint, st_point *> *worst);

  pair<st_point *, st_point *>
  select_parents_from_classes(nondomination_sorted_set *F, char control);

  st_point *select_with_binary_tournament(st_database *Pt,
                                          crowding_distances_map *distances,
                                          ranks_map *ranks);
};

void st_nsgaii::delete_rejected(st_database *Rt, st_database *Pt) {
  if (Rt->count_points() == 0)
    return;
#ifdef DEBUG
  cout << "\n#############delete_rejected#########################\n";
  cout << "	deleted points:\n";
#endif
  st_point_set *complete_population = Rt->get_set_of_points();
  st_point_set::iterator pop_it;
  uint deleted_count = 0;
  while (deleted_count < N) {
    for (pop_it = complete_population->begin();
         pop_it != complete_population->end(); pop_it++)
      if (!Pt->look_for_point(pop_it->second)) {
#ifdef DEBUG
        cout << "		" << pop_it->second->print() << endl;
#endif
        complete_population->erase_point(pop_it);
        deleted_count++;
        break;
      }
  }
}

void st_nsgaii::update_min_max(st_env *env, st_database *db,
                               int objectives_number) throw(exception) {
#ifdef DEBUG
  cout << "\n#############update_min_max#########################\n";
#endif
  st_point_set *db_population = db->get_set_of_points();
  st_point_set::iterator db_it;
  try {
    for (int i = 0; i < objectives_number; i++) {
      double value = env->optimization_objectives[i]->eval(
          db_population->begin()->second, i);
      f_max[i] = value;
      f_min[i] = value;
      for (db_it = db_population->begin(); db_it != db_population->end();
           db_it++) {
        value = env->optimization_objectives[i]->eval(db_it->second, i);
        if (value < f_min[i])
          f_min[i] = value;
        else if (value > f_max[i])
          f_max[i] = value;
      }
#ifdef DEBUG
      cout << "	f_max[" << i << "] = " << f_max[i] << "\n	f_min[" << i
           << "] = " << f_min[i] << endl;
#endif
    }
  } catch (exception &e) {
    prs_display_error("Exception while updating min and max values");
    throw;
  }
}

void st_nsgaii::set_to_database(set<st_point *> *points_set,
                                st_database *database) {
  set<st_point *>::iterator it;
  database->clear();
  for (it = points_set->begin(); it != points_set->end(); it++)
    database->insert_point(*it);
}

bool st_nsgaii::crowded_comparison_operator(st_point *i, st_point *j,
                                            crowding_distances_map *distances,
                                            ranks_map *ranks) {
  if ((*ranks)[i] < (*ranks)[j])
    return true;
  else if ((*ranks)[i] == (*ranks)[j])
    if ((*distances)[i] > (*distances)[j])
      return true;
    else
      return false;
  else
    return false;
}

map<uint, st_point *>
st_nsgaii::sort_distances_descending(set<st_point *> points_set,
                                     crowding_distances_map *distances,
                                     ranks_map *ranks) {
#ifdef DEBUG
  cout << "\n#############sort_distances_descending#########################\n";
  cout << "	original set:\n";
  set<st_point *>::iterator original_it;
  for (original_it = points_set.begin(); original_it != points_set.end();
       original_it++)
    cout << "		" << (*original_it)->print()
         << " distance: " << (*distances)[*original_it]
         << " rank: " << (*ranks)[*original_it] << endl;
#endif
  int set_size = points_set.size();
  map<uint, st_point *> ordered_set;
  ordered_set.clear();
  for (int i = 0; i < set_size; i++) {
    set<st_point *>::iterator it, max;
    max = points_set.begin();
    for (it = points_set.begin(); it != points_set.end(); it++)
      if (crowded_comparison_operator((*it), (*max), distances, ranks))
        max = it;
    ordered_set[i] = *max;
    points_set.erase(max);
  }
#ifdef DEBUG
  cout << "	ordered set:\n";
  for (uint i = 0; i < set_size; i++)
    cout << "		" << ordered_set[i]->print()
         << " distance: " << (*distances)[ordered_set[i]]
         << " rank: " << (*ranks)[ordered_set[i]] << endl;
#endif
  return ordered_set;
}

map<uint, st_point *>
st_nsgaii::sort_objective_ascending(st_env *env, set<st_point *> points_set,
                                    int m) throw(exception) {
  uint set_size = points_set.size();
  map<uint, st_point *> ordered_set;
  ordered_set.clear();
  try {
#ifdef DEBUG
    cout << "\n		"
            "#############sort_objective_ascending#########################\n";
    cout << "			original set:\n";
    set<st_point *>::iterator original_it;
    for (original_it = points_set.begin(); original_it != points_set.end();
         original_it++)
      cout << "				" << (*original_it)->print() << " "
           << env->optimization_objectives[m]->eval(*original_it, m) << endl;
#endif
    for (uint i = 0; i < set_size; i++) {
      set<st_point *>::iterator it, min;
      min = points_set.begin();
      for (it = points_set.begin(); it != points_set.end(); it++)
        if (env->optimization_objectives[m]->eval(*it, m) <
            env->optimization_objectives[m]->eval(*min, m))
          min = it;
      ordered_set[i] = *min;
      points_set.erase(min);
    }
#ifdef DEBUG
    cout << "			ordered set:\n";
    for (uint i = 0; i < set_size; i++)
      cout << "				" << ordered_set[i]->print() << " "
           << env->optimization_objectives[m]->eval(ordered_set[i], m) << endl;
#endif
  } catch (exception &e) {
    prs_display_error("Exception while sorting objective ascending");
    throw;
  }
  return ordered_set;
}

void st_nsgaii::crowding_distance_assignment(
    st_env *env, set<st_point *> *points_set, int objectives_number,
    crowding_distances_map *distances) throw(exception) {
#ifdef DEBUG
  cout << "\n#############crowding_distance_assignment#########################"
          "\n";
#endif
  set<st_point *> I;
  map<uint, st_point *> ordered_I;
  I = *points_set;
  set<st_point *>::iterator points_set_it;
  for (points_set_it = I.begin(); points_set_it != I.end(); points_set_it++)
    (*distances)[*points_set_it] = 0;
  try {
    for (int m = 0; m < objectives_number; m++) {
#ifdef DEBUG
      cout << "\n	Objective number: " << m << endl;
#endif
      ordered_I = sort_objective_ascending(env, I, m);
      (*distances)[ordered_I[0]] = INFINITE;
      (*distances)[ordered_I[ordered_I.size() - 1]] = INFINITE;
      for (int i = 1; i < ordered_I.size() - 1; i++)
        (*distances)[ordered_I[i]] =
            (*distances)[ordered_I[i]] +
            (env->optimization_objectives[m]->eval(ordered_I[i + 1], m) -
             env->optimization_objectives[m]->eval(ordered_I[i - 1], m)) /
                (f_max[m] - f_min[m]);
#ifdef DEBUG
      cout << "\n	distances:\n";
      for (points_set_it = I.begin(); points_set_it != I.end(); points_set_it++)
        cout << "		Point " << (*points_set_it)->print()
             << " distance: " << (*distances)[*points_set_it] << endl;
#endif
    }
  } catch (exception &e) {
    prs_display_error("Exception while assigning distances");
    throw;
  }
}

void st_nsgaii::identify_population_classes(nondomination_sorted_set *F,
                                            map<uint, st_point *> *best,
                                            map<uint, st_point *> *middle,
                                            map<uint, st_point *> *worst) {
#ifdef DEBUG
  cout << "\n	"
          "#############identify_population_classes#########################\n";
#endif
  unsigned int best_count, middle_count, worst_count;
  float remainder;
  remainder = fmod(N, 3);
  if (remainder == 0) {
    best_count = N / 3;
    worst_count = best_count;
    middle_count = best_count;
  } else if (remainder == 1) {
    worst_count = N / 3;
    middle_count = worst_count;
    best_count = worst_count + 1;
  } else {
    worst_count = N / 3;
    middle_count = worst_count + 1;
    best_count = worst_count + 1;
  }
  best->clear();
  worst->clear();
  middle->clear();
  uint rank = 1;
  set<st_point *>::iterator nondominated_set_it;
  nondominated_set_it = (*F)[rank].begin();
#ifdef DEBUG
  cout << "		best set: ( " << best_count << " elements )\n";
#endif
  for (uint i = 0; i < best_count; i++) {
    (*best)[i] = *nondominated_set_it;
#ifdef DEBUG
    cout << "			" << (*nondominated_set_it)->print() << "\n";
#endif
    nondominated_set_it++;
    while (nondominated_set_it == (*F)[rank].end()) {
      rank++;
      nondominated_set_it = (*F)[rank].begin();
    }
  }
#ifdef DEBUG
  cout << "		middle set: ( " << middle_count << " elements )\n";
#endif
  for (uint i = 0; i < middle_count; i++) {
    (*middle)[i] = *nondominated_set_it;
#ifdef DEBUG
    cout << "			" << (*nondominated_set_it)->print() << "\n";
#endif
    nondominated_set_it++;
    while (nondominated_set_it == (*F)[rank].end()) {
      rank++;
      nondominated_set_it = (*F)[rank].begin();
    }
  }
#ifdef DEBUG
  cout << "		worst set: ( " << worst_count << " elements )\n";
#endif
  for (uint i = 0; i < worst_count; i++) {
    (*worst)[i] = *nondominated_set_it;
#ifdef DEBUG
    cout << "			" << (*nondominated_set_it)->print() << "\n";
#endif
    nondominated_set_it++;
    if (i != worst_count - 1)
      while (nondominated_set_it == (*F)[rank].end()) {
        rank++;
        nondominated_set_it = (*F)[rank].begin();
      }
  }
}

pair<st_point *, st_point *>
st_nsgaii::select_parents_from_classes(nondomination_sorted_set *F,
                                       char control) {
  static map<uint, st_point *> best;
  static map<uint, st_point *> middle;
  static map<uint, st_point *> worst;
  static uint combination = 0;
  st_point *father = 0;
  st_point *mother = 0;
  switch (control) {
  case 0: // to init & select
    combination = 0;
    best.clear();
    middle.clear();
    worst.clear();
    identify_population_classes(F, &best, &middle, &worst);
    break;
  case 1: // to clear
    best.clear();
    middle.clear();
    worst.clear();
    return make_pair(father, mother);
    break;
  default: // to select
    if (combination == 8)
      combination = 0;
    else
      combination++;
  }
  map<uint, st_point *> *fathers, *mothers;
#ifdef DEBUG
  cout << "	";
#endif
  switch (combination) {
  case 0:
#ifdef DEBUG
    cout << "(best, best)    ";
#endif
    fathers = &best;
    mothers = &best;
    break;
  case 1:
#ifdef DEBUG
    cout << "(best, middle)  ";
#endif
    fathers = &best;
    mothers = &middle;
    break;
  case 2:
#ifdef DEBUG
    cout << "(best, worst)   ";
#endif
    fathers = &best;
    mothers = &worst;
    break;
  case 3:
#ifdef DEBUG
    cout << "(middle, best)  ";
#endif
    fathers = &middle;
    mothers = &best;
    break;
  case 4:
#ifdef DEBUG
    cout << "(middle, middle)";
#endif
    fathers = &middle;
    mothers = &middle;
    break;
  case 5:
#ifdef DEBUG
    cout << "(middle, worst) ";
#endif
    fathers = &middle;
    mothers = &worst;
    break;
  case 6:
#ifdef DEBUG
    cout << "(worst, best)   ";
#endif
    fathers = &worst;
    mothers = &best;
    break;
  case 7:
#ifdef DEBUG
    cout << "(worst, middle) ";
#endif
    fathers = &worst;
    mothers = &middle;
    break;
  default:
#ifdef DEBUG
    cout << "(worst, worst)  ";
#endif
    fathers = &worst;
    mothers = &worst;
  }
  uint father_index = rnd_flat(0, fathers->size() - 1);
  uint mother_index = rnd_flat(0, mothers->size() - 1);
  if (fathers == mothers)
    while (father_index == mother_index)
      mother_index = rnd_flat(0, mothers->size() - 1);
  father = (*fathers)[father_index];
  mother = (*mothers)[mother_index];
  return make_pair(father, mother);
}

st_point *st_nsgaii::select_with_binary_tournament(
    st_database *Pt, crowding_distances_map *distances, ranks_map *ranks) {
  st_point_set *population = Pt->get_set_of_points();
  uint first_index = rnd_flat(0, population->get_size() - 1);
  uint second_index = first_index;
  while (first_index == second_index)
    second_index = rnd_flat(0, population->get_size() - 1);
  st_point_set::iterator p_point;
  uint index = 0;
  st_point *first = 0;
  st_point *second = 0;
  for (p_point = population->begin(); p_point != population->end(); p_point++) {
    if (index == first_index) {
      first = p_point->second;
      if (second)
        break;
    }
    if (index == second_index) {
      second = p_point->second;
      if (first)
        break;
    }
    index++;
  }
  st_point *selected;
  if (crowded_comparison_operator(first, second, distances, ranks))
    selected = first;
  else
    selected = second;
  return selected;
}

st_database *st_nsgaii::make_new_population(st_env *env,
                                            nondomination_sorted_set *F,
                                            st_database *Pt,
                                            crowding_distances_map *distances,
                                            ranks_map *ranks) {
#ifdef DEBUG
  cout << "\n#############make_new_population#########################\n";
#endif
  st_database *offspring = new st_database;
  bool first = true;
  uint tried_configurations = 0;
  while (offspring->count_points() != N &&
         tried_configurations < max_configurations) {
    st_point *father;
    st_point *mother;
    st_point crossovered_point;
    st_point mutated_point;
    switch (selection_operator) {
    case SELECTION_CLASSES:
      if (first) {
        first = false;
        pair<st_point *, st_point *> couple = select_parents_from_classes(F, 0);
        father = couple.first;
        mother = couple.second;
      } else {
        pair<st_point *, st_point *> couple = select_parents_from_classes(F, 2);
        father = couple.first;
        mother = couple.second;
      }
      break;
    case SELECTION_BINARY_TOURNAMENT:
      father = select_with_binary_tournament(Pt, distances, ranks);
      mother = father;
      while (father == mother)
        mother = select_with_binary_tournament(Pt, distances, ranks);
    }
    crossovered_point =
        env->current_design_space->ea_crossover(env, father, mother);
    mutated_point =
        env->current_design_space->ea_mutation(env, &crossovered_point);
#ifdef DEBUG
    cout << "	#" << offspring->count_points() + 1 << " (F,M): ("
         << father->print() << "," << mother->print() << ") -> "
         << crossovered_point.print() << " -> " << mutated_point.print() << " ";
    bool accepted = false;
#endif
    tried_configurations++;
    if (!offspring->look_for_point(&mutated_point) &&
        !Pt->look_for_point(&mutated_point)) {
      st_point *simulated_child = simulate_point(env, &mutated_point);
      if (simulated_child) {
        offspring->insert_point(simulated_child);
        tried_configurations = 0;
        explored_points++;
#ifdef DEBUG
        accepted = true;
#endif
      } else {
        delete simulated_child;
      }
    }
#ifdef DEBUG
    if (accepted)
      cout << "  accepted\n";
    else
      cout << "  not accepted\n";
#endif
  }
#ifdef DEBUG
  if (tried_configurations == max_configurations)
    cout << "Error: problem creating new population\n";
#endif
  return offspring;
}

nondomination_sorted_set
st_nsgaii::fast_non_dominated_sort(st_database *initial_population, st_env *env,
                                   ranks_map *ranks) {
#ifdef DEBUG
  cout << "\n#############fast_non_dominated_sort#########################\n";
#endif
  st_point_set *population = initial_population->get_set_of_points();
  map<st_point *, int> n;
  map<st_point *, set<st_point *> > S;
  nondomination_sorted_set F;
  F[1].clear();

  st_point_set::iterator p_point, q_point;
  for (p_point = population->begin(); p_point != population->end(); p_point++) {
    st_point *p = p_point->second;
    n[p] = 0;
    S[p].clear();

    for (q_point = population->begin(); q_point != population->end();
         q_point++) {
      st_point *q = q_point->second;
      bool feasible_e;
      bool feasible_p;
      int rank_e;
      int rank_p;
      double penalty_e;
      double penalty_p;
      if (sim_is_strictly_dominated(env, *p, *q, feasible_e, feasible_p, rank_e,
                                    rank_p, penalty_e,
                                    penalty_p)) { // q domina p?
        n[p]++;
      } else if (sim_is_strictly_dominated(env, *q, *p, feasible_e, feasible_p,
                                           rank_e, rank_p, penalty_e,
                                           penalty_p)) { // p domina q?
        S[p].insert(q);
      }
    }
    if (n[p] == 0) {
      (*ranks)[p] = 1;
      F[1].insert(p);
    }
  }

  int i = 1;
  while (F[i].size() != 0) {
    set<st_point *> Q;
    Q.clear();
    set<st_point *>::iterator p, q;
#ifdef DEBUG
    cout << "	F" << i << " :\n";
#endif
    for (p = F[i].begin(); p != F[i].end(); p++) {
#ifdef DEBUG
      cout << "		" << (*p)->print() << " (";
#endif
      for (q = S[*p].begin(); q != S[*p].end(); q++) {
#ifdef DEBUG
        cout << " " << (*q)->print() << " ";
#endif
        n[*q]--;
        if (n[*q] == 0) {
          (*ranks)[*q] = i + 1;
          Q.insert(*q);
        }
      }
#ifdef DEBUG
      cout << ")\n";
#endif
    }
    i++;
    F[i] = Q;
  }
  return F;
}

st_point *st_nsgaii::simulate_point(st_env *env, st_point *point) {
  st_batch_job job(*point);
  env->current_dispatcher->submit(env, &job);
  st_point *sp = job.get_point_at(0);

  if (sp) {
    if (!sp->get_error()) {
      return sp;
    } else {
      delete sp;
      return 0;
    }
  }
  throw "NSGA/Cant simulate point";
}

vector<float> st_nsgaii::get_float_vector_from_shell(st_env *env,
                                                     string vector_name) {
  vector<float> the_vector;
  the_vector.clear();
  const st_object *input_vector;
  if (env->shell_variables.get(vector_name, input_vector)) {
    if (is_a<st_vector *>(input_vector->gen_copy())) {
      st_vector *data_vector = to<st_vector *>(input_vector->gen_copy());
      if (data_vector->size() > 0) {
        for (int i = 0; i < data_vector->size(); i++) {
          float value;
          int result = sscanf(data_vector->get(i).print().data(), "%f", &value);
          if (result == 0 || result == EOF) {
            the_vector.clear();
            break;
          }
          the_vector.push_back(value);
        }
      }
    }
  }
  return the_vector;
}

string st_nsgaii::get_information() {
  return "Nondominated Sorting Genetic Algorithm II optimizer - "
         "(generations_number, max_configurations,  selection_operator, "
         "crossover_operator, crossover_word || crossover_distribution, "
         "mutation_operator, mutations_distribution || mutations_probability )";
}

void st_nsgaii::init_internal_data(st_env *env) {
  string operator_selection;

  if (!env->shell_variables.get_integer("generations_number",
                                        generations_number)) {
    generations_number = DEFAULT_GENERATIONS_NUMBER;
    env->shell_variables.set_integer("generations_number", generations_number);
  }
  if (!env->shell_variables.get_integer("max_configurations",
                                        max_configurations)) {
    max_configurations = DEFAULT_MAX_TRYING_CONFIGURATIONS;
    env->shell_variables.set_integer("max_configurations", max_configurations);
  }

  operator_selection = "";
  if (!env->shell_variables.get_string("selection_operator",
                                       operator_selection)) {
    operator_selection = selection_operators[DEFAULT_SELECTION_OPERATOR];
    selection_operator = (selection_operator_type)DEFAULT_SELECTION_OPERATOR;
    prs_display_message("WARNING: selection_operator not specified. Default "
                        "operator assigned.");
    env->shell_variables.set_string("selection_operator",
                                    operator_selection.data());
  } else {
    bool found = false;
    for (int i = 0; i < SELECTION_OPERATOR_CHOICES; i++) {
      if (operator_selection == selection_operators[i]) {
        selection_operator = (selection_operator_type)i;
        found = true;
        break;
      }
    }
    if (!found) {
      operator_selection = selection_operators[DEFAULT_SELECTION_OPERATOR];
      selection_operator = (selection_operator_type)DEFAULT_SELECTION_OPERATOR;
      prs_display_message("WARNING: wrong selection_operator specification. "
                          "Default operator assigned.");
      env->shell_variables.set_string("selection_operator",
                                      operator_selection.data());
    }
  }
}

st_database *st_nsgaii::get_initial_population(st_env *env, st_vector *doe) {
#ifdef DEBUG
  cout << "\n#############get_initial_population#########################\n";
#endif
  st_database *population = env->get_pre_populated_database();
  int doe_point = 0;
  population->clear();
  while (doe_point < doe->size()) {
    st_point *point =
        simulate_point(env, to<st_point *>(doe->get(doe_point).gen_copy()));
    if (point) {
      population->insert_point(point);
#ifdef DEBUG
      cout << "	doe: " << point->print() << endl;
#endif
      explored_points++;
    }
    doe_point++;
  }
  int tried_configurations = 0;
  while (population->count_points() < N &&
         tried_configurations < max_configurations) {
    st_point random_point = env->current_design_space->random_point(env);
    st_point *point = simulate_point(env, &random_point);
    if (point) {
      population->insert_point(point);
#ifdef DEBUG
      cout << "	random: " << point->print() << endl;
#endif
      explored_points++;
    } else
      tried_configurations++;
  }
  return population;
}

int st_nsgaii::explore(st_env *env) {
  nondomination_sorted_set F, Pt_nondominated;
  crowding_distances_map distances;
  st_database *Pt, *Qt, *Rt;
  ranks_map ranks;
  st_vector *doe;
  int objectives_number;
  int t;
  int i;

  if (!env->current_driver) {
    prs_display_error("no driver defined");
    return 0;
  }
  if (!env->current_doe) {
    prs_display_error("no doe defined");
    return 0;
  }
  doe = env->current_doe->generate_doe(env);
  N = doe->size();
  if (N < MIN_POPULATION_SIZE) {
    prs_display_error("Doe size too small");
    return explored_points;
  }
  init_internal_data(env);
  prs_display_message("Starting with the nsgaii optimization process");
  objectives_number = env->optimization_objectives.size();
  Pt = get_initial_population(env, doe);
  if (Pt->count_points() < N) {
    prs_display_error(
        "Problem with initial population. Try to reduce the doe size.");
    delete Pt;
    return explored_points;
  }
  Pt_nondominated = fast_non_dominated_sort(Pt, env, &ranks);
  if (selection_operator == SELECTION_BINARY_TOURNAMENT) {
    try {
      update_min_max(env, Pt, objectives_number);
    } catch (exception &e) {
      delete Pt;
      throw;
    }
    int front = 1;
    while (Pt_nondominated[front].size() != 0) {
      try {
        crowding_distance_assignment(env, &Pt_nondominated[front],
                                     objectives_number, &distances);
      } catch (exception &e) {
        delete Pt;
        throw;
      }
      front++;
    }
  }
  Rt = new st_database();
  Rt->clear();
  t = 0;
  while (t < generations_number) {
#ifdef DEBUG
    cout << "\n#################################### MAIN LOOP "
            "####################################\n";
    cout << "	t: " << t;
    cout << "\n#################################### MAIN LOOP "
            "####################################\n";
#endif
    Qt = make_new_population(env, &Pt_nondominated, Pt, &distances, &ranks);
    if (Qt->count_points() < N) {
      prs_display_error("Problem making new population. Try to reduce the doe "
                        "size or the generations number.");
      delete Qt;
      delete Pt;
      delete Rt;
      return explored_points;
    }
    delete_rejected(Rt, Pt);
    Rt->clear();
    Rt->attach(*Pt);
    Rt->attach(*Qt);
    Pt->clear();
    Qt->clear();
    ranks.clear();
    F = fast_non_dominated_sort(Rt, env, &ranks);
    i = 1;
    Pt_nondominated.clear();
    try {
      update_min_max(env, Rt, objectives_number);
    } catch (exception &e) {
      delete Qt;
      delete Pt;
      delete Rt;
      throw;
    }
    distances.clear();
    while (Pt->count_points() + F[i].size() <= N) {
      if (selection_operator == SELECTION_BINARY_TOURNAMENT) {
        try {
          crowding_distance_assignment(env, &F[i], objectives_number,
                                       &distances);
        } catch (exception &e) {
          delete Qt;
          delete Pt;
          delete Rt;
          throw;
        }
      }
      st_database *Fi = new st_database;
      set_to_database(&F[i], Fi);
      Pt->attach(*Fi);
      delete Fi;
      Pt_nondominated[i] = F[i];
      i++;
    }
    if (Pt->count_points() < N) {
      try {
        crowding_distance_assignment(env, &F[i], objectives_number, &distances);
      } catch (exception &e) {
        delete Qt;
        delete Pt;
        delete Rt;
        throw;
      }
      map<uint, st_point *> ordered_points =
          sort_distances_descending(F[i], &distances, &ranks);
      uint j = 0;
      while (Pt->count_points() < N) {
        Pt_nondominated[i].insert(ordered_points[j]);
        Pt->insert_point(ordered_points[j]);
        j++;
      }
    }
    t++;
  }
  env->insert_new_database(Pt, env->current_destination_database_name);
  delete Qt;
  delete Rt;
  return explored_points;
}

extern "C" {
st_optimizer *opt_generate_optimizer() { return new st_nsgaii(); }
}
