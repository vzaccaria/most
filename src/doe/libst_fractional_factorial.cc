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

#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <set>
#include <st_design_space.h>
#include <st_object.h>
#include <st_map.h>
#include <st_vector.h>
#include <st_shell_variables.h>
#include <st_parser.h>
#include <st_optimizer.h>
#include <st_doe.h>
#include <st_opt_utils.h>

//  Da aggiungere a st_opt_utils
const st_string *opt_get_complete_fold_over(st_env *p) {
  st_object const *complete_fold_over_obj;
  if (!p->shell_variables.get("complete_fold_over", complete_fold_over_obj))
    return NULL;
  to<st_string const *>(complete_fold_over_obj);
  if (!is_a<st_string const *>(complete_fold_over_obj))
    return NULL;
  return to<st_string const *>(complete_fold_over_obj);
}

// Da aggiungere a st_opt_utils
const st_map *opt_get_generator_strings(st_env *p) {
  st_object const *dse_obj;
  p->shell_variables.get("generator_strings", dse_obj);
  if (!is_a<st_map const *>(dse_obj))
    return NULL;
  return to<st_map const *>(dse_obj);
}

//  Da aggiungere a st_opt_utils
const st_string *opt_get_fold_over_factor(st_env *p) {
  st_object const *fold_over_factor_obj;
  if (!p->shell_variables.get("fold_over_factor", fold_over_factor_obj))
    return NULL;
  to<st_string const *>(fold_over_factor_obj);
  if (!is_a<st_string const *>(fold_over_factor_obj))
    return NULL;
  return to<st_string const *>(fold_over_factor_obj);
}

// Da aggiungere a st_opt_utils
bool opt_alias_factor(st_env *env, st_point &p, st_point &upper_bound,
                      st_point &lower_bound, st_vector vect, int coord) {

  if (coord >= p.size())
    return false;

  int sz = vect.size();
  int num_lower_bound = 0;
  for (int i = 0; i < sz; i++) {
    st_object const *element = &vect.get(i);
    st_assert(is_a<st_string const *>(element));
    int elem_index =
        opt_get_index_of_ds_parameters_sym_within_ds_parameters_vector(
            env, to<st_string const *>(element)->get_string());
    if (elem_index >= p.size())
      return false;
    if (p[elem_index] == lower_bound[elem_index])
      num_lower_bound++;
  }

  if (num_lower_bound % 2)
    p[coord] = lower_bound[coord];
  else
    p[coord] = upper_bound[coord];

  return true;
}

// Da aggiungere a opt_utils
bool opt_fold_over_on_factor(st_vector *vect, st_point &lower_bound,
                             st_point &upper_bound, int factor) {
  if (factor < 0 || factor >= lower_bound.size())
    return false;

  int sz = vect->size();
  for (int i = 0; i < sz; i++) {
    st_object const *element = &(vect->get(i));
    st_assert(is_a<st_point const *>(element));
    st_point actual_point = *(to<st_point *>(const_cast<st_object *>(element)));

    if (actual_point[factor] == lower_bound[factor])
      actual_point[factor] = upper_bound[factor];
    else
      actual_point[factor] = lower_bound[factor];
    vect->insert(i, actual_point);
  }

  return true;
}

// Da aggiungere a opt_utils
bool opt_complete_fold_over(st_vector *vect, st_point &lower_bound,
                            st_point &upper_bound) {

  int sz = vect->size();
  for (int i = 0; i < sz; i++) {
    st_object const *element = &(vect->get(i));
    st_assert(is_a<st_point const *>(element));
    st_point actual_point = *(to<st_point *>(const_cast<st_object *>(element)));
    for (int j = 0; j < actual_point.size(); j++)
      if (actual_point[j] == lower_bound[j])
        actual_point[j] = upper_bound[j];
      else
        actual_point[j] = lower_bound[j];
    vect->insert(i, actual_point);
  }

  return true;
}

// Da aggiungere a opt_utils
string opt_print_doe(st_vector *vect, st_point &lower_bound,
                     st_point &upper_bound) {
  string str;
  int sz = vect->size();
  for (int i = 0; i < sz; i++) {
    st_object const *element = &(vect->get(i));
    st_assert(is_a<st_point const *>(element));
    st_point actual_point = *(to<st_point *>(const_cast<st_object *>(element)));

    stringstream sstr;
    sstr << i;
    str += "Design " + sstr.str() + " :";
    for (int j = 0; j < actual_point.size(); j++)
      str += ((actual_point[j] == lower_bound[j]) ? " - " : " + ");
    str += "\n";
  }

  return str;
}

// Da aggiungere a opt_utils
st_vector opt_prod_vector(st_vector vect1, st_vector vect2) {
  int sz1 = vect1.size();
  int sz2 = vect2.size();
  st_string sdupl("Duplicated");

  st_vector res;
  int nres = 0;
  for (int i = 0; i < sz1; i++) {
    st_object const *elem1 = &vect1.get(i);
    st_assert(is_a<st_string const *>(elem1));
    bool insert = true;
    for (int j = 0; j < sz2; j++) {
      st_object const *elem2 = &vect2.get(j);
      st_assert(is_a<st_string const *>(elem2));
      if (to<st_string const *>(elem1)->get_string() ==
          to<st_string const *>(elem2)->get_string()) {
        insert = false;
        vect2.insert(j, sdupl);
        break;
      }
    }
    if (insert) {
      res.insert(nres, *elem1);
      nres++;
    }
  }

  for (int i = 0; i < sz2; i++) {
    st_object const *elem2 = &vect2.get(i);
    st_assert(is_a<st_string const *>(elem2));
    if (to<st_string const *>(elem2)->get_string() != sdupl.get_string()) {
      res.insert(nres, *elem2);
      nres++;
    }
  }

  return res;
}

// Da aggiungere a opt_utils
bool opt_third_level_of_contrast_words(st_vector &contrast_word) {
  int nsize = contrast_word.size();
  int newelem = nsize;

  for (int i = 0; i < (nsize - 1); i++) {
    st_object const *elem1 = &contrast_word.get(i);
    st_assert(is_a<st_vector const *>(elem1));
    for (int j = i + 1; j < nsize; j++) {
      st_object const *elem2 = &contrast_word.get(j);
      st_assert(is_a<st_vector const *>(elem2));
      contrast_word.insert(newelem,
                           opt_prod_vector(*to<st_vector const *>(elem1),
                                           *to<st_vector const *>(elem2)));
      newelem++;
    }
  }

  int k = nsize;
  for (int i = 1; i < (nsize - 1); i++) {
    for (int j = i + 1; j < nsize; j++) {
      st_object const *elem1 = &contrast_word.get(k);
      st_assert(is_a<st_vector const *>(elem1));
      st_object const *elem2 = &contrast_word.get(j);
      st_assert(is_a<st_vector const *>(elem2));
      contrast_word.insert(newelem,
                           opt_prod_vector(*to<st_vector const *>(elem1),
                                           *to<st_vector const *>(elem2)));
      newelem++;
    }
    k = k + (nsize - i);
  }

  return true;
}

class st_fractional_factorial : public st_doe {
public:
  st_fractional_factorial() {}
  string get_information();
  st_vector *generate_doe(st_env *env);
};

string st_fractional_factorial::get_information() {
  return "Fractional factorial DoE - (generator_strings, complete_fold_over, "
         "fold_over_factor)";
}

bool st_check_consistency_of_generator_string(st_env *env,
                                              const st_map *current_gs) {
  st_map_const_iterator gs_iter;
  set<string> dependent_variables;
  for (gs_iter = current_gs->begin(); gs_iter != current_gs->end(); gs_iter++) {
    if (!opt_is_ds_parameter_existing(env, gs_iter->first)) {
      return false;
    }

    dependent_variables.insert(gs_iter->first);

    if (!is_a<st_vector *>(gs_iter->second))
      return false;

    st_vector *vec_of_params = to<st_vector *>(gs_iter->second);

    for (int i = 0; i < vec_of_params->size(); i++) {
      if (!is_a<st_string const *>(&vec_of_params->get(i)))
        return false;

      string par = to<st_string const *>(&vec_of_params->get(i))->get_string();

      if (!opt_is_ds_parameter_existing(env, par))
        return false;

      if (dependent_variables.count(par))
        return false;
    }
  }
  return true;
}

st_vector *st_fractional_factorial::generate_doe(st_env *env) {
  st_assert(env->current_driver);
  st_vector *doe = new st_vector();
  bool finished = (opt_get_design_space_vector(env)->size() == 0);
  const st_vector *current_ds = opt_get_design_space_vector(env);
  int num_of_prs = current_ds->size();

  st_point actual_point(num_of_prs);
  st_point lower_bound(num_of_prs);
  st_point upper_bound(num_of_prs);

  lower_bound = opt_get_lower_bound(env);
  upper_bound = opt_get_upper_bound(env);

  const st_map *current_gs = opt_get_generator_strings(env);

  st_map empty_gs;

  if (current_gs == NULL) {
    current_gs = &empty_gs;
    prs_display_message(
        "Generator strings not found; generating full factorial DoE");
  } else {
    if (!st_check_consistency_of_generator_string(env, current_gs)) {
      prs_display_message("Failed interpretation of generator strings");
      current_gs = &empty_gs;
    } else {
      prs_display_message("Generating fractional factorial DoE");
    }
  }

  actual_point = lower_bound;

  st_map_const_iterator gs_iter;
  int n = 0;
  int gs_index = -1;

  while (!finished) {
    for (gs_iter = current_gs->begin(); gs_iter != current_gs->end();
         gs_iter++) {
      int gs_col =
          opt_get_index_of_ds_parameters_sym_within_ds_parameters_vector(
              env, gs_iter->first);
      if (gs_col >= 0 && gs_col < num_of_prs) {
        opt_alias_factor(env, actual_point, upper_bound, lower_bound,
                         *to<st_vector *>(gs_iter->second), gs_col);
      }
    }

    doe->insert(n, actual_point);

    for (gs_iter = current_gs->begin(); gs_iter != current_gs->end();
         gs_iter++) {
      gs_index = opt_get_index_of_ds_parameters_sym_within_ds_parameters_vector(
          env, gs_iter->first);
      if (gs_index >= 0 && gs_index < num_of_prs)
        actual_point[gs_index] = upper_bound[gs_index];
    }

    finished = !opt_plus_factor_size(actual_point, upper_bound, lower_bound);
    n++;
  }

  const st_string *factor_st_string = opt_get_fold_over_factor(env);
  int factor_int = -1;

  if (factor_st_string != NULL) {
    if (opt_is_ds_parameter_existing(env, factor_st_string->get_string()))
      factor_int =
          opt_get_index_of_ds_parameters_sym_within_ds_parameters_vector(
              env, *factor_st_string);
  }

  if (factor_int != -1) {
    opt_fold_over_on_factor(doe, lower_bound, upper_bound, factor_int);
    prs_display_message("Fold Over On Factor " + factor_st_string->print());
  }

  const st_string *fold_over = opt_get_complete_fold_over(env);

  if (fold_over != NULL && fold_over->get_string() == "true") {
    opt_complete_fold_over(doe, lower_bound, upper_bound);
    prs_display_message("Complete Fold Over");
  }

  cout << opt_print_doe(doe, lower_bound, upper_bound);

  st_vector contrast_words;
  int ncontrast_words = 0;
  for (gs_iter = current_gs->begin(); gs_iter != current_gs->end(); gs_iter++) {
    int size = (to<st_vector *>(gs_iter->second)->size());
    st_object *new_contrast_word = to<st_vector *>(gs_iter->second)->gen_copy();
    st_string elem(gs_iter->first);
    to<st_vector *>(new_contrast_word)->insert(size, elem);
    contrast_words.insert(ncontrast_words, *new_contrast_word);
    ncontrast_words++;
    delete new_contrast_word;
  }

  opt_third_level_of_contrast_words(contrast_words);
  prs_display_message("Aliases I = " + contrast_words.print());

  return doe;
}

extern "C" {
st_doe *doe_generate_doe() { return new st_fractional_factorial(); }
}
