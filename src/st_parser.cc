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
#include <regex.h>
#include "st_common_utils.h"
#include "st_env.h"
#include "st_list.h"
#include "st_map.h"
#include "st_mpi_utils.h"
#include "st_parser.h"
#include "st_point.h"
#include "st_vector.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

bool mpi_verbose = false;
bool display_time = false;
bool silent_codi = false;
bool never_fail = false;

/* Common Optimizer Display Interface functions */

time_t diff;

void st_codi_reset_time() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  diff = tv.tv_sec;
}

time_t st_codi_get_time() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec;
}

#define compute_eta(eta) ((1.0 - (eta)) * (st_codi_get_time() - diff))
#define d_elaps()                                                              \
  " Information: Elapsed time: " << (st_codi_get_time() - diff) << " (sec), "

void st_codi_display_iep(float impr_percentage, float eta, string phase) {
  if (silent_codi)
    return;
  cout << prs_get_time() << d_elaps();
  cout << "Improvement: " << ((int)impr_percentage * 100) << " (%), ";
  cout << "Eta: " << compute_eta(eta) << " (sec), ";
  cout << "Phase: " << phase << endl;
}

void st_codi_display_ip(float impr_percentage, string phase) {
  if (silent_codi)
    return;
  cout << prs_get_time() << d_elaps();
  cout << "Improvement: " << ((int)impr_percentage * 100) << " (%), ";
  cout << "Phase: " << phase << endl;
}

void st_codi_display_p(string phase) {
  if (silent_codi)
    return;
  cout << prs_get_time() << d_elaps();
  cout << "Eta: ? (sec), ";
  cout << "Phase: " << phase << endl;
}

void st_codi_display_ep(float eta, string phase) {
  if (silent_codi)
    return;
  cout << prs_get_time() << d_elaps();
  cout << "Eta: " << compute_eta(eta) << " (sec), ";
  cout << "Phase: " << phase << endl;
}

string prs_get_time() {
  char buffer[30];
  struct timeval tv;

  time_t curtime;

  gettimeofday(&tv, NULL);
  curtime = tv.tv_sec;
  strftime(buffer, 30, "%d/%m/%Y %T", localtime(&curtime));
  return string(buffer);
}

void prs_display_time() {
  if (st_mpi_get_node() == 0 || mpi_verbose) {
    if (display_time)
      cout << prs_get_time() << " ";
  }
}

extern string prs_get_last_line();
extern string prs_get_last_file();
extern bool prs_get_last_file_interactive();

void prs_display_error(const string &s) {
  if (st_mpi_get_node() == 0 || mpi_verbose) {
    if (display_time)
      cout << prs_get_time() << " ";

    if (!prs_get_last_file_interactive()) {
      cout << "Error: file '" + prs_get_last_file() + "', line " +
                  prs_get_last_line() + ", "
           << s << endl;
    } else {
      cout << "Error: " << s << endl;
    }
  }
}

void prs_display_error_plain(const string &s) {
  if (st_mpi_get_node() == 0 || mpi_verbose) {
    if (display_time)
      cout << prs_get_time() << " ";
    cout << "Error: " << s << endl;
  }
}

void prs_display_message(const string &s) {
  if (st_mpi_get_node() == 0 || mpi_verbose) {
    string verbose;
    bool v = current_environment.shell_variables.get_string("verbose", verbose);
    if (!v || verbose == "true") {
      if (display_time)
        cout << prs_get_time() << " ";
      cout << "Information: " << s << endl;
    }
  }
}

st_object *prs_add_coord_to_point(st_object *point, st_object *c) {
  st_assert(is_a<st_point *>(point));
  st_assert(is_a<st_integer *>(c));

  st_point *p = to<st_point *>(point);
  st_integer *i = to<st_integer *>(c);
  p->resize(p->size() + 1);
  (*p)[p->size() - 1] = i->get_integer();
  return p;
}

st_object *prs_list_concat(st_object *l1, st_object *l2) {
  st_assert(is_a<st_list *>(l1));
  st_assert(is_a<st_list *>(l2));
  st_list *list1 = to<st_list *>(l1);
  st_list *list2 = to<st_list *>(l2);
  list1->concat(list2);
  return list1;
}

st_object *prs_insert_in_list(st_object *l1, st_object *o) {
  st_assert(is_a<st_list *>(l1));
  to<st_list *>(l1)->insert(*o);
  return l1;
}

st_object *prs_insert_in_vector(st_object *l1, st_object *o) {
  st_assert(is_a<st_vector *>(l1));
  int size = to<st_vector *>(l1)->size();
  to<st_vector *>(l1)->insert(size, *o);
  return l1;
}

st_object *prs_add_element_to_map(st_object *m, st_object *s, st_object *o) {
  st_assert(is_a<st_map *>(m));
  st_assert(is_a<st_string *>(s));
  to<st_map *>(m)->insert(to<st_string *>(s)->get_string(), *o);
  return m;
}

st_object *prs_insert_map_as_property(st_object *o, st_object *m) {
  st_assert(is_a<st_map *>(m));
  to<st_map *>(m)->fill_properties_of(o);
  return o;
}

/* This is a fast routine for reading a point from a string.
 * Note that this is not generic (can't be used for parsing generic points from
 * a script). It's a copy of the functionality in st_database.read_from_file()
 */

st_object *prs_read_point_from_string(string input) {
  regmatch_t positions[1];

  string pattern_int = "[0-9][0-9]*";
  string pattern_point = "%[ 0-9]*%";
  string pattern_metrics = "metrics[ ]*=[ ]*\\[[ 0-9\\.,]*\\]";
  string pattern_metric = "[+-]*[0-9][0-9]*\\.[0-9]*";
  string pattern_statistics = "statistics[ ]*=[ ]*\\[[ 0-9\\.,]*\\]";
  string pattern_statistic = "[+-]*[0-9][0-9]*\\.[0-9]*";

  string pattern_errors = "error[ ]*=[ ]*[ 0-9]*";
  string pattern_error = "[0-9][0-9]*";

  string pattern_clusters = "cluster[ ]*=[ ]*[ 0-9]*";
  string pattern_cluster = "[0-9][0-9]*";

  string pattern_rpaths = "rpath[ ]*=[ ]*" RE_QUOTED_STRING;
  string pattern_rpath = RE_QUOTED_STRING;

  regex_t pattern_point_comp;
  regex_t pattern_int_comp;
  regex_t pattern_metrics_comp;
  regex_t pattern_metric_comp;
  regex_t pattern_statistics_comp;
  regex_t pattern_statistic_comp;

  regex_t pattern_errors_comp;
  regex_t pattern_error_comp;

  regex_t pattern_clusters_comp;
  regex_t pattern_cluster_comp;

  regex_t pattern_rpaths_comp;
  regex_t pattern_rpath_comp;

  string point_part;
  string metrics_part;
  string statistics_part;
  string errors_part;
  string clusters_part;
  string rpaths_part;

  if (regcomp(&pattern_point_comp, pattern_point.c_str(), 0) ||
      (regcomp(&pattern_int_comp, pattern_int.c_str(), 0)) ||
      (regcomp(&pattern_metrics_comp, pattern_metrics.c_str(), 0)) ||
      (regcomp(&pattern_metric_comp, pattern_metric.c_str(), 0)) ||
      (regcomp(&pattern_statistics_comp, pattern_statistics.c_str(), 0)) ||
      (regcomp(&pattern_statistic_comp, pattern_statistic.c_str(), 0)) ||
      (regcomp(&pattern_errors_comp, pattern_errors.c_str(), 0)) ||
      (regcomp(&pattern_error_comp, pattern_error.c_str(), 0)) ||
      (regcomp(&pattern_clusters_comp, pattern_clusters.c_str(), 0)) ||
      (regcomp(&pattern_cluster_comp, pattern_cluster.c_str(), 0)) ||
      (regcomp(&pattern_rpaths_comp, pattern_rpaths.c_str(),
               REG_EXTENDED | REG_ICASE)) ||
      (regcomp(&pattern_rpath_comp, pattern_rpath.c_str(),
               REG_EXTENDED | REG_ICASE))) {
    cout << "Problems compiling the expression;" << endl;
    return NULL;
  }
  st_point current_point;
  st_vector current_metrics;
  st_vector current_statistics;

  if (!regexec(&pattern_point_comp, input.c_str(), 1, positions, 0)) {
    int length = positions[0].rm_eo - positions[0].rm_so;
    point_part = input.substr(positions[0].rm_so, length);
  }

  while (!regexec(&pattern_int_comp, point_part.c_str(), 1, positions, 0)) {
    int length = positions[0].rm_eo - positions[0].rm_so;
    st_integer conv =
        atoi(point_part.substr(positions[0].rm_so, length).c_str());
    prs_add_coord_to_point(&current_point, &conv);

    point_part = point_part.substr(positions[0].rm_eo,
                                   point_part.length() - positions[0].rm_eo);
  }
  if (!regexec(&pattern_metrics_comp, input.c_str(), 1, positions, 0)) {
    int length = positions[0].rm_eo - positions[0].rm_so;
    metrics_part = input.substr(positions[0].rm_so, length);
  }

  while (
      !regexec(&pattern_metric_comp, metrics_part.c_str(), 1, positions, 0)) {
    int length = positions[0].rm_eo - positions[0].rm_so;
    st_double conv =
        atof(metrics_part.substr(positions[0].rm_so, length).c_str());
    prs_insert_in_vector(&current_metrics, &conv);

    metrics_part = metrics_part.substr(
        positions[0].rm_eo, metrics_part.length() - positions[0].rm_eo);
  }

  if (!regexec(&pattern_statistics_comp, input.c_str(), 1, positions, 0)) {
    int length = positions[0].rm_eo - positions[0].rm_so;
    statistics_part = input.substr(positions[0].rm_so, length);
  }

  while (!regexec(&pattern_statistic_comp, statistics_part.c_str(), 1,
                  positions, 0)) {
    int length = positions[0].rm_eo - positions[0].rm_so;
    st_double conv =
        atof(statistics_part.substr(positions[0].rm_so, length).c_str());
    prs_insert_in_vector(&current_statistics, &conv);

    statistics_part = statistics_part.substr(
        positions[0].rm_eo, statistics_part.length() - positions[0].rm_eo);
  }
  int error = 0;

  if (!regexec(&pattern_errors_comp, input.c_str(), 1, positions, 0)) {
    int length = positions[0].rm_eo - positions[0].rm_so;
    errors_part = input.substr(positions[0].rm_so, length);
  }

  if (!regexec(&pattern_error_comp, errors_part.c_str(), 1, positions, 0)) {
    int length = positions[0].rm_eo - positions[0].rm_so;
    error = atoi(errors_part.substr(positions[0].rm_so, length).c_str());
  }
  if (!regexec(&pattern_clusters_comp, input.c_str(), 1, positions, 0)) {
    int length = positions[0].rm_eo - positions[0].rm_so;
    clusters_part = input.substr(positions[0].rm_so, length);
  }

  if (!regexec(&pattern_cluster_comp, clusters_part.c_str(), 1, positions, 0)) {
    int length = positions[0].rm_eo - positions[0].rm_so;
    int cluster =
        atoi(clusters_part.substr(positions[0].rm_so, length).c_str());

    current_point.set_cluster(cluster);
  }

  if (!regexec(&pattern_rpaths_comp, input.c_str(), 1, positions, 0)) {
    int length = positions[0].rm_eo - positions[0].rm_so;
    rpaths_part = input.substr(positions[0].rm_so, length);
  }

  if (!regexec(&pattern_rpath_comp, rpaths_part.c_str(), 1, positions, 0)) {
    int length = positions[0].rm_eo - positions[0].rm_so;
    string rpath = (rpaths_part.substr(positions[0].rm_so + 1, length - 2));
    current_point.set_rpath(rpath);
  }

  if (!error) {

    current_point.set_properties("metrics", current_metrics);
    current_point.set_properties("statistics", current_statistics);
  } else {
    current_point.set_error(error);
  }

  regfree(&pattern_point_comp);
  regfree(&pattern_int_comp);
  regfree(&pattern_metrics_comp);
  regfree(&pattern_metric_comp);
  regfree(&pattern_statistics_comp);
  regfree(&pattern_statistic_comp);
  regfree(&pattern_errors_comp);
  regfree(&pattern_error_comp);
  regfree(&pattern_clusters_comp);
  regfree(&pattern_cluster_comp);
  regfree(&pattern_rpaths_comp);
  regfree(&pattern_rpath_comp);

  return current_point.gen_copy();
}
