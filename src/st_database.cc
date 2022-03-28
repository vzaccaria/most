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

#include <fstream>
#include <iomanip>
#include <pthread.h>
#include <regex.h>
#include <sstream>
#include "st_database.h"
#include "st_doe.h"
#include "st_objectives_constraints.h"
#include "st_opt_utils.h"
#include "st_parser.h"
#include "st_sim_utils.h"
#include "st_xdr_api.h"
#include "vector"
#include "st_design_space.h"
/* #include <malloc.h> */
#include <algorithm>
#include "st_common_utils.h"

#define S_MIN 0
#define S_25 1
#define S_50 2
#define S_75 3
#define S_MAX 4
#define S_AVG 5

vector<vector<double> > statistics;
vector<vector<double> > final_stats;

bool lesst(double i, double j) { return (i < j); }

void st_init_stats(int num) {
  statistics.resize(num);
  for (int i = 0; i < num; i++) {
    statistics[i].resize(0);
  }
}

void st_set_stats(int column, double value) {
  statistics[column].push_back(value);
}

double st_percentile(vector<double> &v, double perc) {
  int sz = v.size() - 1;
  int index = (sz * perc);
  return v[index];
}

void st_get_stats() {
  final_stats.resize(statistics.size());
  for (int column = 0; column < statistics.size(); column++) {
    final_stats[column].resize(S_AVG + 1);

    if (statistics[column].size() == 0)
      return;

    std::sort(statistics[column].begin(), statistics[column].end());

    final_stats[column][S_MIN] = statistics[column][0];
    final_stats[column][S_MAX] =
        statistics[column][statistics[column].size() - 1];
    final_stats[column][S_AVG] = statistics[column][0];

    for (int i = 1; i < statistics[column].size(); i++) {
      final_stats[column][S_AVG] += statistics[column][i];
    }
    final_stats[column][S_AVG] =
        final_stats[column][S_AVG] / statistics[column].size();
    /* the following ones are to be computed correctly */
    final_stats[column][S_25] = st_percentile(statistics[column], 0.25);
    final_stats[column][S_50] = st_percentile(statistics[column], 0.50);
    final_stats[column][S_75] = st_percentile(statistics[column], 0.75);
  }
}

st_point_set::st_point_set(st_point_set &s) {
  st_point_set::iterator i;
  int n = 0;
  for (i = s.begin(); i != s.end(); i++) {
    pair<vector<int>, st_point *> p;
    p.first = i->first;
    p.second = to<st_point *>(i->second->gen_copy());
    insert(p);
    opt_print_percentage("Copying database", n, s.size);
    n++;
  }
  size = s.size;
}

st_point_set::~st_point_set() {
  st_point_set::iterator i;
  for (i = begin(); i != end(); i++) {
    delete i->second;
  }
}

st_database::st_database() { points = new st_point_set(); };

st_database::st_database(st_database &s) {
  points = new st_point_set(*s.points);
};

st_database::st_database(st_env *env, st_doe *doe) {
  points = new st_point_set();
  int doe_point = 0;
  st_vector *db_doe = doe->generate_doe(env);
  while (doe_point < db_doe->size()) {
    st_point &po = dynamic_cast<st_point &>(
        const_cast<st_object &>(db_doe->get(doe_point)));
    insert_point(&po);
    doe_point++;
  }
  delete db_doe;
}

st_database::~st_database() { delete points; };

int count_lines_file(string fname) {
  FILE *file;
  file = fopen(fname.c_str(), "rb");
  if (file == NULL)
    return 0;

  int nl = 0;
  int c;
  while ((c = getc(file)) != EOF) {
    if (c == '\n')
      ++nl;
  }
  fclose(file);
  return nl;
}

void st_point_set::insert_point(st_point *p) {
  vector<int> key = *p;
  st_point_set::iterator i;
  if ((i = find(key)) != end()) {
    erase_point(i);
  }
  pair<vector<int>, st_point *> pair;
  pair.first = *p;
  pair.second = to<st_point *>(p->gen_copy());
  insert(pair);
  size++;
}

void st_database::insert_point(const st_point *p) {
  points->insert_point(const_cast<st_point *>(p));
};

int st_database::write_octave(const char *filename) {
  ofstream file_out(filename, ios::out);

  if (file_out.fail())
    return 0;

  st_point_set::iterator i = points->begin();

  int size = points->get_size();
  int n = 0;

  for (; i != points->end(); i++) {
    if ((i->second)->check_consistency(&current_environment) &&
        !(i->second)->get_error()) {
      file_out << "" << (i->second)->print_octave(&current_environment)
               << " \n";
      opt_print_percentage("Writing database", n, size);
    }
    n++;
  }
  return 1;
}

int st_database::write_to_m3_file_objectives(const char *filename) {
  ofstream file_out(filename, ios::out);

  if (file_out.fail())
    return 0;

  st_point_set::iterator i = points->begin();

  int size = points->get_size();
  int n = 0;

  for (; i != points->end(); i++) {
    file_out << "db_insert_point "
             << (i->second)->print_m3_canonical_objectives(&current_environment)
             << " \n";
    opt_print_percentage("Writing database", n, size);
    n++;
  }
  return 1;
}

int st_database::write_to_m3_file(const char *filename) {
  ofstream file_out(filename, ios::out);

  if (file_out.fail())
    return 0;

  st_point_set::iterator i = points->begin();

  int size = points->get_size();
  int n = 0;

  for (; i != points->end(); i++) {
    file_out << "db_insert_point " << (i->second)->print_m3_canonical()
             << " \n";
    opt_print_percentage("Writing database", n, size);
    n++;
  }
  return 1;
}

int st_database::write_to_file(const char *filename) {
  ofstream file_out(filename, ios::out);
  if (file_out.fail())
    return 0;
  st_point_set::iterator i = points->begin();

  int size = points->get_size();
  int n = 0;

  for (; i != points->end(); i++) {
    file_out << "db_insert_point " << (i->second)->print_canonical() << " \n";
    opt_print_percentage("Writing database", n, size);
    n++;
  }
  return 1;
};

/**************************************** IMPORT FROM FILE - BEGIN
 * ************************************************/
#define RE_SEPARATOR ";"
#define RE_SPECIFICATION_EL RE_SPACE RE_IDENTIFIER RE_SPACE RE_SEPARATOR
#define RE_VECTOR_MASK_EL                                                      \
  "[(]"                                                                        \
  "[01]"                                                                       \
  "(-[01])*"                                                                   \
  "[)]"
#define RE_VECTOR_PERM_EL                                                      \
  "@" RE_INTEGER "(-" RE_INTEGER ")*"                                          \
  "@"
#define RE_VECTOR_EL "(" RE_VECTOR_MASK_EL "|" RE_VECTOR_PERM_EL ")"
#define RE_RECORD_EL                                                           \
  "(\")?" RE_SPACE "(" RE_INTEGER "|" RE_DOUBLE "|" RE_VECTOR_EL "|" RE_SPACE  \
  ")" RE_SPACE "(\")?" RE_SEPARATOR
#define RE_SPECIFICATION_LINE "^(" RE_SPECIFICATION_EL ")+$"
#define RE_RECORD_LINE "^(" RE_RECORD_EL ")+$"

#define DS_PARAMETER 0
#define METRIC 1
#define IGNORE 2

#define DEBUG

regmatch_t m[1];
regex_t reg_buf_specification_el;
regex_t reg_buf_specification_line;
regex_t reg_buf_record_el;
regex_t reg_buf_record_line;

typedef map<int, pair<int, int> > data_mapping;

bool read_line_from_file(FILE *file, string &line, bool &is_eof) {
  int index = 0;
  int c;
  line.clear();
  is_eof = false;
  do {
    c = fgetc(file);
    if (c == EOF) {
      if (!feof(file)) {
        prs_display_error("Something goes wrong while reading the file.");
        return false;
      }
      is_eof = true;
    } else if ((char)c == '\n') {
      return true;
    } else {
      line.push_back((char)c);
    }
  } while (c != EOF && ((char)c) != '\n');
  return true;
}

bool acquire_specification_line(string &line,
                                vector<string> &specification_elements) {
  specification_elements.clear();
  if (regexec(&reg_buf_specification_line, line.c_str(), 1, m, 0) ==
      REG_NOMATCH)
    return false;
  string specification_element;
  string remaining = line;
  do {
    if (regexec(&reg_buf_specification_el, remaining.c_str(), 1, m,
                REG_NOTBOL | REG_NOTEOL) == REG_NOMATCH)
      return true;
    if ((int)m[0].rm_so != -1) {
      int length = (int)m[0].rm_eo - (int)m[0].rm_so;
      specification_element = remaining.substr((int)m[0].rm_so, length);
      remaining = remaining.substr(
          (int)m[0].rm_eo, remaining.size() - specification_element.size());
      st_find_first_and_replace(&specification_element, RE_SEPARATOR, "");
      specification_elements.push_back(specification_element);
    } else
      return true;
  } while (true);
}

bool ds_and_metrics_mapping(vector<string> &specification_elements,
                            data_mapping &mapping,
                            map<string, int> ds_parameters_index,
                            map<string, int> metric_index) {
  int recognized_parameters = 0;
  int recognized_metrics = 0;
  mapping.clear();
  for (int i = 0; i < specification_elements.size(); i++) {
    map<string, int>::iterator it;
    for (it = ds_parameters_index.begin(); it != ds_parameters_index.end();
         it++) {
      if (specification_elements[i] == (*it).first) {
        mapping[i] = make_pair(DS_PARAMETER, (*it).second);
        recognized_parameters++;
        break;
      }
    }
    if (mapping.size() != (i + 1)) {
      for (it = metric_index.begin(); it != metric_index.end(); it++) {
        if (specification_elements[i] == (*it).first) {
          mapping[i] = make_pair(METRIC, (*it).second);
          recognized_metrics++;
          break;
        }
      }
    }
    if (mapping.size() != (i + 1))
      mapping[i] = make_pair(IGNORE, 0);
  }
  if (recognized_parameters != ds_parameters_index.size())
    return false;
  if (recognized_metrics != metric_index.size())
    return false;
  /*for( int i = 0; i < specification_elements.size(); i++ )
  {
          cout << "specification_element[ " << i << " ] = \"" <<
  specification_elements[ i ] << "\" is "; if( mapping[ i ].first ==
  DS_PARAMETER ) cout << "a parameter\n"; else if( mapping[ i ].first == METRIC
  ) cout << "a metric\n"; else cout << "to ignore\n";
  }*/
  return true;
}

bool st_database::acquire_database_record(string &line, data_mapping mapping,
                                          st_env *env, bool use_symbols) {
  if (regexec(&reg_buf_record_line, line.c_str(), 1, m, 0) == REG_NOMATCH)
    return false;
  string record_element;
  string remaining = line;
  vector<string> record_elements;
  do {
    if (regexec(&reg_buf_record_el, remaining.c_str(), 1, m,
                REG_NOTBOL | REG_NOTEOL) == REG_NOMATCH)
      break;
    if ((int)m[0].rm_so != -1) {
      int length = (int)m[0].rm_eo - (int)m[0].rm_so;
      record_element = remaining.substr((int)m[0].rm_so, length);
      remaining = remaining.substr((int)m[0].rm_eo,
                                   remaining.size() - record_element.size());
      st_find_first_and_replace(&record_element, RE_SEPARATOR, "");
      st_find_and_replace(&record_element, "\"", "", NULL);
      record_elements.push_back(record_element);
    } else
      break;
  } while (true);
  /*for( int i = 0; i < record_elements.size(); i++ )
          cout << "record_elements[ " << i << " ] = \"" + record_elements[ i ]
     << "\"\n";*/
  int number_of_parameters = env->current_design_space->ds_parameters.size();
  int number_of_metrics = env->current_design_space->metric_names.size();
  if (record_elements.size() < number_of_parameters) {
    // skip the point
    return false;
  }
  /*if( record_elements.size() != mapping.size() )
  {
          prs_display_error( "Wrong file data format!" );
          return false;
  }*/
  st_point new_point = st_point(number_of_parameters);
  bool mark_error = false;
  int parameters_count = 0;
  int metrics_count = 0;
  st_vector metrics_vector;
  for (int i = 0; i < record_elements.size(); i++) {
    int data_type = mapping[i].first;
    if (data_type == DS_PARAMETER) {
      string ds_parameter_name =
          env->current_design_space->ds_parameters[mapping[i].second].name;
      int ds_parameter_type =
          env->current_design_space->ds_parameters[mapping[i].second].type;
      int ds_parameter_number = mapping[i].second;
      int scalar_type;
      int param_level;
      int param_max_level;
      int param_min_level;
      switch (ds_parameter_type) {
      case ST_DS_SCALAR:
        scalar_type =
            env->current_design_space->scalar_parameters[ds_parameter_name]
                .type;
        switch (scalar_type) {
        case ST_SCALAR_TYPE_INTEGER:
          if (!st_recognize_integer(&record_elements[i],
                                    &new_point[ds_parameter_number], NULL,
                                    NULL)) {
#ifdef DEBUG
            prs_display_error("Wrong parameter #" + st_to_string(i));
#endif
            return false;
          }
          break;
        case ST_SCALAR_TYPE_LIST:
          if (use_symbols) {
            try {
              new_point[ds_parameter_number] =
                  env->current_design_space->get_scalar_level(
                      env, ds_parameter_name, record_elements[i]);
            } catch (st_design_space_exception e) {
#ifdef DEBUG
              prs_display_error("Wrong parameter #" + st_to_string(i));
#endif
              return false;
            }
          } else {
            if (!st_recognize_integer(&record_elements[i],
                                      &new_point[ds_parameter_number], NULL,
                                      NULL)) {
#ifdef DEBUG
              prs_display_error("Wrong parameter #" + st_to_string(i));
#endif
              return false;
            }
          }
          break;
        }
        param_level = new_point[ds_parameter_number];
        param_max_level =
            env->current_design_space->get_scalar_max(env, ds_parameter_name);
        param_min_level =
            env->current_design_space->get_scalar_min(env, ds_parameter_name);
        if (param_level > param_max_level || param_level < param_min_level) {
#ifdef DEBUG
          prs_display_error("Wrong parameter #" + st_to_string(i));
#endif
          return false;
        }
        break;
      case ST_DS_PERMUTATION:
        /* FIXME - add the instructions to read this parameter type */
        break;
      case ST_DS_ON_OFF_MASK:
        /* FIXME - add the instructions to read this parameter type */
        break;
      }
      parameters_count++;
    } else if (data_type == METRIC) {
      int metric_index = mapping[i].second;
      double metric_value;
      if (!st_recognize_double(&record_elements[i], &metric_value, NULL,
                               NULL)) {
        if (!st_recognize_error(&record_elements[i], NULL, NULL)) {
          if (!st_recognize_space(&record_elements[i], NULL, NULL))
            return false;
          else
            mark_error = true;
        }
      } else {
        if (st_isnan(metric_value) || st_isinf(metric_value)) {
          mark_error = true;
        }
        st_double obj(metric_value);
        metrics_vector.insert(metric_index, obj);
        metrics_count++;
      }
    }
  }
  if (parameters_count < number_of_parameters)
    return false;
  if (metrics_count == number_of_metrics)
    new_point.set_properties("metrics", metrics_vector);
  if (mark_error) {
    new_point.set_error(ST_POINT_NON_FATAL_ERROR);
  }
  insert_point(&new_point);
  return true;
}

int st_database::import_from_file(st_env *env, const char *filename,
                                  bool use_symbols) {
  if (regcomp(&reg_buf_specification_line, RE_SPECIFICATION_LINE,
              REG_EXTENDED | REG_ICASE) != 0) {
    prs_display_error(
        "Problems with internal specifications. Please, contact the vendor.");
    return 0;
  }
  if (regcomp(&reg_buf_record_line, RE_RECORD_LINE, REG_EXTENDED | REG_ICASE) !=
      0) {
    prs_display_error(
        "Problems with internal specifications. Please, contact the vendor.");
    return 0;
  }
  if (regcomp(&reg_buf_specification_el, RE_SPECIFICATION_EL,
              REG_EXTENDED | REG_ICASE) != 0) {
    prs_display_error(
        "Problems with internal specifications. Please, contact the vendor.");
    return 0;
  }
  if (regcomp(&reg_buf_record_el, RE_RECORD_EL, REG_EXTENDED | REG_ICASE) !=
      0) {
    prs_display_error(
        "Problems with internal specifications. Please, contact the vendor.");
    return 0;
  }
  string line;
  bool is_eof;
  FILE *source;
  vector<int> ds_parameters_mapping;
  vector<int> metrics_mapping;
  source = fopen(filename, "r");
  if (source == NULL) {
    prs_display_error("Impossible to open the file \"" + string(filename) +
                      "\"");
    return 0;
  }
  if (!read_line_from_file(source, line, is_eof)) {
    prs_display_error("Problems while reading \"" + string(filename) + "\"");
    fclose(source);
    return 0;
  }
  vector<string> specification_elements;
  if (!acquire_specification_line(line, specification_elements)) {
    prs_display_error(
        "Problems while recognizing the (mandatory) specification line.");
    fclose(source);
    return 0;
  }
  data_mapping mapping;
  ds_and_metrics_mapping(specification_elements, mapping,
                         env->current_design_space->ds_parameters_index,
                         env->current_design_space->metric_index);
  int inserted_points = 0;
  int points_number = 0;
  do {
    if (!read_line_from_file(source, line, is_eof)) {
      prs_display_error("Problems while reading \"" + string(filename) + "\"");
      fclose(source);
      return 0;
    }
    if (!is_eof && line != "") {
      if (acquire_database_record(line, mapping, env, use_symbols))
        inserted_points++;
      else {
#ifdef DEBUG
        cout << "Skipped the point at line: " << points_number << ", line: \""
             << line << "\"" << endl;
#endif
      }
      points_number++;
    }
  } while (!is_eof);
  fclose(source);
  prs_display_message(
      "Read " + st_to_string(inserted_points) + " points, skipped " +
      st_to_string((points_number - inserted_points)) + " points.");
  return 1;
}

/**************************************** IMPORT FROM FILE - END
 * ************************************************/

int st_database::read_from_file(const char *filename) {
  string pattern_int = "[0-9][0-9]*";
  string pattern_point = "%[ 0-9]*%";
  /** FIXME should add and test [+-] to pattern_metrics and pattern_statistics
   */
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
    return 0;
  }

  char line[1000];

  string input;

  regmatch_t positions[1];

  int nlines = count_lines_file(filename);

  FILE *file = fopen(filename, "r");
  if (!file) {
    prs_display_error("File does not exist");
    return 0;
  }

  int n = 0;
  while (fgets(line, sizeof(line), file)) {
    string point_part;
    string metrics_part;
    string statistics_part;
    string errors_part;
    string clusters_part;
    string rpaths_part;

    st_point current_point;
    st_vector current_metrics;
    st_vector current_statistics;

    input = line;

    opt_print_percentage("Reading db", n, nlines);
    n++;

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

    if (!regexec(&pattern_errors_comp, input.c_str(), 1, positions, 0)) {
      int length = positions[0].rm_eo - positions[0].rm_so;
      errors_part = input.substr(positions[0].rm_so, length);
    }

    int error = 0;

    if (!regexec(&pattern_error_comp, errors_part.c_str(), 1, positions, 0)) {
      int length = positions[0].rm_eo - positions[0].rm_so;
      error = atoi(errors_part.substr(positions[0].rm_so, length).c_str());
    }

    if (!regexec(&pattern_clusters_comp, input.c_str(), 1, positions, 0)) {
      int length = positions[0].rm_eo - positions[0].rm_so;
      clusters_part = input.substr(positions[0].rm_so, length);
    }

    if (!regexec(&pattern_cluster_comp, clusters_part.c_str(), 1, positions,
                 0)) {
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
    insert_point(&current_point);
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

  fclose(file);
  return 1;
};

int st_database::count_points() { return points->get_size(); }

#define PRINT_CONF_SIZE 30
#define PRINT_GAP 2
#define PRINT_METRIC_SIZE 10
#define PRINT_STATS_SIZE 10

void st_printf_tabbed_cen(string a, int maxsize) {
  int mid = maxsize / 2 - a.size() / 2;

  if (2 * mid + a.size() > maxsize)
    mid--;

  if (a.size() < maxsize) {
    for (int i = 0; i < mid; i++)
      cout << " ";
  }
  cout << a;

  for (int i = mid + a.size(); i < maxsize; i++)
    cout << " ";
}

void st_printf_tabbed_rev(string a, int maxsize) {
  if (a.size() < maxsize) {
    for (int i = 0; i < (maxsize - a.size()); i++)
      cout << " ";
  }
  cout << a;
}

void st_printf_tabbed(string a, int maxsize) {
  cout << a;
  if (a.size() < maxsize) {
    for (int i = 0; i < (maxsize - a.size()); i++)
      cout << " ";
  }
}

static st_table tb;

void st_print_header(st_env *env, bool nometrics, bool clus, bool rp) {
  int confcolsize;
  int metriccolsize;

  tb.clear();
  tb.set_header("Database Report");

  if (!env->shell_variables.get_integer("db_report_conf_col_size",
                                        confcolsize)) {
    confcolsize = PRINT_CONF_SIZE;
    prs_display_message_n_value_m(
        "Assuming db_report_conf_col_size = ", confcolsize, "");
  }
  if (!env->shell_variables.get_integer("db_report_metric_col_size",
                                        metriccolsize)) {
    metriccolsize = PRINT_METRIC_SIZE;
    prs_display_message_n_value_m(
        "Assuming db_report_metric_col_size = ", metriccolsize, "");
  }

  tb.add_column("Configuration", confcolsize, TYPE_STRING, JUST_CENTER);
  int nums = 0;

  if (!nometrics) {
    vector<string> &metric_names = env->current_design_space->metric_names;
    for (int i = 0; i < metric_names.size(); i++) {
      string s = metric_names[i];
      tb.add_column(s, metriccolsize, TYPE_FLOAT, JUST_RIGHT);
      nums++;
    }
    for (int i = 0; i < env->companion_metrics.size(); i++) {
      tb.add_column(env->companion_metrics[i]->name, metriccolsize, TYPE_FLOAT,
                    JUST_RIGHT);
    }
  }
  for (int i = 0; i < env->optimization_objectives.size(); i++) {
    string s = env->optimization_objectives[i]->name;
    tb.add_column(s, metriccolsize, TYPE_FLOAT, JUST_RIGHT);
    nums++;
  }
  tb.add_column("Constraint", metriccolsize, TYPE_FLOAT, JUST_CENTER);
  tb.add_column("Penalty", metriccolsize, TYPE_FLOAT, JUST_CENTER);

  st_init_stats(nums);

  if (clus) {

    tb.add_column("Cluster", metriccolsize, TYPE_FLOAT, JUST_CENTER);
  }
  if (rp) {
    tb.add_column("Path", confcolsize, TYPE_STRING, JUST_LEFT);
  }
  // cout << endl;
  tb.print_header();
}

void st_print_stats(st_env *env, string stat, int sv, bool nometrics, bool clus,
                    bool rp) {
  int confcolsize;
  int metriccolsize;

  if (!env->shell_variables.get_integer("db_report_conf_col_size", confcolsize))
    confcolsize = PRINT_CONF_SIZE;

  if (!env->shell_variables.get_integer("db_report_metric_col_size",
                                        metriccolsize))
    metriccolsize = PRINT_METRIC_SIZE;

  int use_levels = false;
  if (!env->shell_variables.get_integer("db_report_use_levels", use_levels))
    use_levels = false;

  int reverse = false;

  if (!env->shell_variables.get_integer("db_report_pretty_print", reverse))
    reverse = false;

  tb.begin_row();

  tb.add_column_string(stat);

  int nums = 0;
  if (!nometrics) {
    vector<string> &metric_names = env->current_design_space->metric_names;
    for (int i = 0; i < metric_names.size(); i++) {
      double metric = final_stats[nums++][sv];
      ostringstream str;
      str << metric;
      tb.add_column_float(metric);
    }
    for (int i = 0; i < env->companion_metrics.size(); i++) {
      tb.add_column_string("");
    }
  }
  for (int i = 0; i < env->optimization_objectives.size(); i++) {
    double value = final_stats[nums++][sv];
    tb.add_column_float(value);

    ostringstream str;
    str << value;
  }
  tb.add_column_string(string(""));
  tb.add_column_string(string(""));

  if (clus) {
    tb.add_column_string("");
  }
  if (rp) {
    tb.add_column_string("");
  }
  // cout << endl;
  tb.end_row();
}

void st_print_point(st_env *env, st_point *p, bool nometrics, bool clus,
                    bool rp) {
  int confcolsize;
  int metriccolsize;

  if (!env->shell_variables.get_integer("db_report_conf_col_size", confcolsize))
    confcolsize = PRINT_CONF_SIZE;

  if (!env->shell_variables.get_integer("db_report_metric_col_size",
                                        metriccolsize))
    metriccolsize = PRINT_METRIC_SIZE;

  int use_levels = false;
  if (!env->shell_variables.get_integer("db_report_use_levels", use_levels))
    use_levels = false;

  int reverse = false;

  if (!env->shell_variables.get_integer("db_report_pretty_print", reverse))
    reverse = false;

  tb.begin_row();

  if (!use_levels) {
    tb.add_column_string(
        env->current_design_space->get_point_representation(env, *p));
  } else {
    tb.add_column_string(p->print());
  }

  if (!p->get_error()) {
    int nums = 0;
    if (!nometrics) {
      vector<string> &metric_names = env->current_design_space->metric_names;
      for (int i = 0; i < metric_names.size(); i++) {
        double metric = p->get_metrics(i);
        ostringstream str;
        str << metric;
        tb.add_column_float(metric);
        st_set_stats(nums++, metric);
      }
      for (int i = 0; i < env->companion_metrics.size(); i++) {
        st_object *value = env->companion_metrics[i]->eval(p, i);
        string p = value->print();

        tb.add_column_string(p);

        delete value;
      }
    }
    for (int i = 0; i < env->optimization_objectives.size(); i++) {
      double value = env->optimization_objectives[i]->eval(p, i);
      tb.add_column_float(value);
      st_set_stats(nums++, value);

      ostringstream str;
      str << value;
    }
    int rank;
    double penalty;
    bool cn = opt_check_constraints(*p, env, rank, penalty);

    if (cn) {
      tb.add_column_string(string(""));
    } else {
      tb.add_column_string(string("KO"));
    }

    ostringstream str;
    str << rank << ", " << penalty;

    tb.add_column_string(str.str());

    if (clus) {
      ostringstream str1;
      str1 << p->get_cluster();
      tb.add_column_string(str1.str());
    }
    if (rp) {
      string pt = p->get_rpath();
      tb.add_column_string(pt);
    }
    // cout << endl;
  } else {
    if (p->get_error() == ST_POINT_FATAL_ERROR) {
      cout << ("Fatal error: ") << p->get_error_description();
      // tb.add_column_string(string(("Fatal error: ") +
      // p->get_error_description()));
    }

    if (p->get_error() == ST_POINT_NON_FATAL_ERROR) {
      cout << ("Non fatal error: ") << p->get_error_description();
      // tb.add_column_string(string(("Non fatal error: ") +
      // p->get_error_description()));
    }

    // cout << endl;
  }
  tb.end_row();
}

typedef vector<vector<st_point *> > st_workload;

struct st_thread_workload {
  int thread_index;
  vector<st_point *> *wk;
  st_env *env;
};

void st_fill_workload(st_database *db, st_env *env, st_workload &v,
                      int num_of_workloads) {
  v.resize(num_of_workloads);
  int sz = db->count_points();
  st_point_set *ps = db->get_set_of_points();
  st_point_set::iterator p = ps->begin();
  int wk = 0;
  int osize = env->optimization_objectives.size();
  int msize = env->companion_metrics.size();
  for (int n = 0; n < sz; n++) {
    if (p->second->o_cache.size() < osize)
      p->second->o_cache.resize(osize);

    if (p->second->m_cache.size() < msize)
      p->second->m_cache.resize(msize);

    if (p->second->check_consistency(env) && !p->second->get_error()) {
      v[wk].push_back(p->second);
      wk = (wk + 1) % num_of_workloads;
    }
    p++;
    opt_print_percentage("Cache update, redistributing workload", n, sz);
  }
}

void *st_update_cache_thread_body(void *arg) {
  st_thread_workload *twk = (st_thread_workload *)arg;
  int sz = twk->wk->size();
  int index = twk->thread_index;
  for (int i = 0; i < twk->wk->size(); i++) {
    for (int s = 0; s < twk->env->companion_metrics.size(); s++) {
      try {
        st_object *res = twk->env->companion_metrics[s]->eval((*twk->wk)[i], s);
        if (res)
          delete res;
      } catch (exception &e) {
        /*cout << "Thread: " << index << " received exception " << e.what() <<
         * endl;*/
        continue;
      }
    }
    for (int s = 0; s < twk->env->optimization_objectives.size(); s++) {
      try {
        twk->env->optimization_objectives[s]->eval((*twk->wk)[i], s);
      } catch (exception &e) {
        /*cout << "Thread: " << index << " received exception " << e.what() <<
         * endl;*/
        continue;
      }
    }
    int rank;
    double penalty;

    try {
      opt_check_constraints(*(*twk->wk)[i], twk->env, rank, penalty);
    } catch (exception &e) {
      /*cout << "Thread: " << index << " received exception " << e.what() <<
       * endl;*/
    }

    if (twk->thread_index == 0)
      opt_print_percentage("Cache update, performing update", i, sz);
  }
  return NULL;
}

#ifndef ST_NO_THREADS
void st_database::cache_update(st_env *env) {
  int local_parallelism;
  if (!env->shell_variables.get_integer("local_parallelism", local_parallelism))
    local_parallelism = 1;

  if (local_parallelism < 1)
    local_parallelism = 1;

  st_workload wk;

  vector<pthread_t *> ptv;
  vector<st_thread_workload> twv;

  st_fill_workload(this, env, wk, local_parallelism);
  twv.resize(local_parallelism);
  for (int i = 0; i < local_parallelism; i++) {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    pthread_t *pt = (pthread_t *)malloc(sizeof(pthread_t));
    ptv.push_back(pt);
    st_thread_workload twk;
    twv[i].thread_index = i;
    twv[i].wk = &wk[i];
    twv[i].env = env;
    pthread_create(pt, &attr, st_update_cache_thread_body, &twv[i]);
  }
  for (int i = 0; i < local_parallelism; i++) {
    pthread_join(*ptv[i], NULL);
  }
}

#else

void st_database::cache_update(st_env *env) {
  int sz = count_points();
  st_point_set *ps = get_set_of_points();
  st_point_set::iterator p = ps->begin();
  int wk = 0;
  int osize = env->optimization_objectives.size();
  int msize = env->companion_metrics.size();
  for (int n = 0; n < sz; n++) {
    if (p->second->o_cache.size() < osize)
      p->second->o_cache.resize(osize);

    if (p->second->m_cache.size() < msize)
      p->second->m_cache.resize(msize);

    if (p->second->check_consistency(env) && !p->second->get_error()) {
      for (int s = 0; s < env->companion_metrics.size(); s++) {
        try {
          st_object *res = env->companion_metrics[s]->eval(p->second, s);
          if (res)
            delete res;
        } catch (exception &e) {
          throw std::logic_error(e.what());
        }
      }

      for (int s = 0; s < env->optimization_objectives.size(); s++) {
        try {
          env->optimization_objectives[s]->eval(p->second, s);
        } catch (exception &e) {
          throw std::logic_error(e.what());
        }
      }

      int rank;
      double penalty;

      try {
        opt_check_constraints(*(p->second), env, rank, penalty);
      } catch (exception &e) {
        throw std::logic_error(e.what());
      }
      p++;
      opt_print_percentage("Cache update (threads disabled)", n, sz);
    }
  }
}

#endif

void st_database::report(st_env *env, bool nometrics, bool showcluster, bool rp,
                         bool ds) {
  if (!env->current_driver) {
    prs_display_error("Please define a driver.");
    return;
  }
  cache_update(env);
  st_point_set::iterator i = points->begin();
  int fail = 0;
  int n = 0;
  st_print_header(env, nometrics, showcluster, rp);
  for (; i != points->end(); i++) {
    st_point const *p = (i->second);
    n++;
    if (p->check_consistency(env)) {
      try {
        st_print_point(env, const_cast<st_point *>(p), nometrics, showcluster,
                       rp);
      } catch (exception &e) {
        prs_display_error(e.what());
        return;
      }
    } else {
      fail++;
    }
    // cout << "  ->  " <<  opt_compute_cost_function(*p,this) << "\n";
  }
  st_get_stats();
  tb.finish_header();
  if (ds) {
    st_print_stats(env, "Min", S_MIN, nometrics, showcluster, rp);
    st_print_stats(env, "25th", S_25, nometrics, showcluster, rp);
    st_print_stats(env, "50th", S_50, nometrics, showcluster, rp);
    st_print_stats(env, "75th", S_75, nometrics, showcluster, rp);
    st_print_stats(env, "Max", S_MAX, nometrics, showcluster, rp);
    tb.finish_header();
    st_print_stats(env, "Average", S_AVG, nometrics, showcluster, rp);
    tb.finish_header();
  }
  cout << "Number of points in the DB: " << n << "\n";
  if (fail)
    cout << "Some points have not been shown due to inconsistencies in the "
            "database.\n"
         << endl;
}

void st_database::clear() {
#ifdef ST_DATABASE_MALLOC_DEBUG
  cout << "Memory allocation before destruction of database - SIZE = "
       << points->get_size() << endl;
  cout << "------------------------------------------------" << endl;
  malloc_stats();
#endif
  if (points)
    delete points;
  points = new st_point_set();
#ifdef ST_DATABASE_MALLOC_DEBUG
  cout << "Memory allocation after destruction of database - SIZE = "
       << points->get_size() << endl;
  cout << "------------------------------------------------" << endl;
  malloc_stats();
#endif
#ifdef ST_DATABASE_MALLOC_TRIM
  /* Does not work at the moment */
#endif
}

st_point_set *st_database::get_set_of_points() { return points; }

void st_database::copy_from(st_point_set *l) {
  if (!l) {
    prs_display_error("Copy from buffer failed since source buffer is empty");
    return;
  }
  clear();
  points = new st_point_set(*l);
}

st_database &st_database::operator=(st_database &orig) {
  copy_from(orig.points);
  return *this;
}

void st_database::attach(st_database &orig) {
  st_point_set *ops = orig.points;
  st_point_set::iterator i;
  for (i = ops->begin(); i != ops->end(); i++) {
    insert_point(i->second);
  }
}

st_point *st_database::look_for_point(st_point *s) {
  st_point_set::iterator i = points->find(*s);
  if (i == points->end())
    return NULL;
  else
    return i->second;
}

void st_database::filter_points(st_env *env, bool violating) {
  cache_update(env);
  st_point_set *set = get_set_of_points();
  st_point_set::iterator i;
  for (i = set->begin(); i != set->end();) {
    int rank_p;
    double penalty_p;
    bool feasible;
    bool error = false;
    if (!i->second->get_error()) {
      feasible = opt_check_constraints(*(i->second), env, rank_p, penalty_p);
    } else {
      error = true;
    }

    if ((violating && feasible) || (!violating && !feasible) || error) {
      st_point_set::iterator k = i;
      k++;
      set->erase_point(i);
      i = k;
    } else {
      i++;
    }
  }
}

void st_database::filter_parameter_level(st_env *env, string par, int level) {
  int par_index = env->current_design_space->ds_parameters_index[par];
  cache_update(&current_environment);
  st_point_set *set = get_set_of_points();
  st_point_set::iterator i;
  for (i = set->begin(); i != set->end();) {
    if ((*(i->second))[par_index] != level) {
      st_point_set::iterator k = i;
      k++;
      set->erase_point(i);
      i = k;
    } else {
      i++;
    }
  }
}

void st_database::filter_cluster(int cluster) {
  cache_update(&current_environment);
  st_point_set *set = get_set_of_points();
  st_point_set::iterator i;
  for (i = set->begin(); i != set->end();) {
    if (i->second->get_cluster() != cluster) {
      st_point_set::iterator k = i;
      k++;
      set->erase_point(i);
      i = k;
    } else {
      i++;
    }
  }
}

void st_database::filter_valid(bool violating) {
  cache_update(&current_environment);
  st_point_set *set = get_set_of_points();
  st_point_set::iterator i;
  for (i = set->begin(); i != set->end();) {
    if (i->second->get_error()) {
      st_point_set::iterator k = i;
      k++;
      set->erase_point(i);
      i = k;
    } else {
      try {
        int rank_p;
        double penalty_p;
        for (int k = 0; k < current_environment.optimization_objectives.size();
             k++) {
          current_environment.optimization_objectives[k]->eval(i->second, k);
        }
        bool feasible = opt_check_constraints(
            *(i->second), &current_environment, rank_p, penalty_p);
        if (!feasible && violating) {
          throw std::logic_error("Point violating constraints");
        }
      } catch (exception &e) {
        st_point_set::iterator k = i;
        k++;
        set->erase_point(i);
        i = k;
        continue;
      }
      i++;
    }
  }
}

#define BOX(v) ((box_cox_is_log) ? log(v) : pow((v), box_cox))

void st_database::xdr_init(st_env *env, double box_cox, bool box_cox_is_log) {
  xdr_init_predictors_only(env);

  xdr_metric_pipes.resize(xdr_n_metrics);
  xdr_statistic_pipes.resize(xdr_n_statistics);

  for (int i = 0; i < xdr_n_metrics; i++) {
    xdr_metric_pipes[i].box_cox_transform = box_cox;
    xdr_metric_pipes[i].box_cox_is_log = box_cox_is_log;
  }

  for (int i = 0; i < xdr_n_statistics; i++) {
    xdr_statistic_pipes[i].box_cox_transform = box_cox;
    xdr_statistic_pipes[i].box_cox_is_log = box_cox_is_log;
  }

  int n = 0;

  for (int i = 0; i < xdr_n_metrics; i++) {
    xdr_metric_pipes[i].disabled = true;
    xdr_metric_pipes[i].mean = 0;
    xdr_metric_pipes[i].std = 0;
  }

  for (int i = 0; i < xdr_n_statistics; i++) {
    xdr_statistic_pipes[i].disabled = true;
    xdr_statistic_pipes[i].mean = 0;
    xdr_statistic_pipes[i].std = 0;
  }

  st_point_set::iterator i = points->begin();

  for (; i != points->end(); i++) {
    st_point *p = (i->second);
    if (p->check_consistency(env) && !p->get_error()) {
      for (int i = 0; i < xdr_n_metrics; i++) {
        xdr_metric_pipes[i].mean += BOX(p->get_metrics(i));
      }

      for (int i = 0; i < xdr_n_statistics; i++) {
        xdr_statistic_pipes[i].mean += BOX(p->get_metrics(i));
      }
      n++;
    }
  }

  if (n > 0) {
    for (int i = 0; i < xdr_n_metrics; i++) {
      xdr_metric_pipes[i].mean = xdr_metric_pipes[i].mean / n;
    }

    for (int i = 0; i < xdr_n_statistics; i++) {
      xdr_statistic_pipes[i].mean = xdr_statistic_pipes[i].mean / n;
    }

    st_point_set::iterator i = points->begin();

    for (; i != points->end(); i++) {
      st_point *p = (i->second);
      if (p->check_consistency(env) && !p->get_error()) {
        for (int i = 0; i < xdr_n_metrics; i++) {
          xdr_metric_pipes[i].std +=
              pow(BOX(p->get_metrics(i)) - xdr_metric_pipes[i].mean, 2);
        }

        for (int i = 0; i < xdr_n_statistics; i++) {
          xdr_statistic_pipes[i].std +=
              pow(BOX(p->get_metrics(i)) - xdr_statistic_pipes[i].mean, 2);
        }
      }
    }

    for (int i = 0; i < xdr_n_metrics; i++) {
      xdr_metric_pipes[i].std = sqrt(xdr_metric_pipes[i].std / n);
      if (xdr_metric_pipes[i].std > 0.000000001)
        xdr_metric_pipes[i].disabled = false;
    }

    for (int i = 0; i < xdr_n_statistics; i++) {
      xdr_statistic_pipes[i].std = sqrt(xdr_statistic_pipes[i].std / n);
      if (xdr_metric_pipes[i].std > 0.000000001)
        xdr_metric_pipes[i].disabled = false;
    }
  }
}

void st_database::xdr_init_predictors_only(st_env *env) {
  xdr_metric_pipes.clear();
  xdr_statistic_pipes.clear();

  xdr_n_predictors = env->current_design_space->ds_parameters.size();

  xdr_n_metrics = 0;
  xdr_n_statistics = 0;

  xdr_n_metrics = env->current_design_space->metric_names.size();
}

bool st_database::xdr_write(st_env *env, string file_name) {
  ofstream file_out(file_name.c_str(), ios::out);

  if (file_out.fail())
    return false;

  st_point_set::iterator i = points->begin();

  int size = points->get_size();
  int n = 0;

  for (; i != points->end(); i++) {
    st_point *p = (i->second);
    if (p->check_consistency(env) && !p->get_error()) {
      vector<int> design = (*p);
      vector<double> observations;

      for (int i = 0; i < xdr_n_metrics; i++) {
        double new_metric = xdr_metric_pipes[i].pre_process(p->get_metrics(i));
        observations.push_back(new_metric);
      }

      for (int i = 0; i < xdr_n_statistics; i++)
        observations.push_back(
            xdr_statistic_pipes[i].pre_process(p->get_statistics(i)));

      st_xdr_write_design(file_out, design, observations);
    }
  }
  return true;
}

bool st_database::xdr_write_predictors_only(st_env *env, string file_name) {
  ofstream file_out(file_name.c_str(), ios::out);

  if (file_out.fail())
    return false;

  st_point_set::iterator i = points->begin();

  int size = points->get_size();
  int n = 0;

  for (; i != points->end(); i++) {
    vector<double> observations;
    st_point *p = (i->second);
    vector<int> design = (*p);
    st_xdr_write_design(file_out, design, observations);
  }
  return true;
}

bool st_database::xdr_read(st_env *, string file_name) {
  clear();
  vector<double> design;
  vector<double> predictions;

  ifstream inp(file_name.c_str());
  while (st_xdr_read_design(inp, design, predictions)) {
    if (design.size() != xdr_n_predictors)
      return false;
    if (predictions.size() != (xdr_n_metrics + xdr_n_statistics))
      return false;

    vector<double> metrics;
    vector<double> statistics;

    st_point actual(xdr_n_predictors);

    for (int i = 0; i < xdr_n_predictors; i++) {
      actual[i] = (int)design[i];
    }
    int j = 0;
    int k = 0;
    for (; j < xdr_n_metrics; j++) {
      metrics.push_back(xdr_metric_pipes[k].post_process(predictions[j]));
      k++;
    }
    k = 0;
    for (; j < (xdr_n_statistics + xdr_n_metrics); j++) {
      statistics.push_back(xdr_statistic_pipes[k].post_process(predictions[j]));
      k++;
    }
    st_vector p_metrics(metrics);
    st_vector p_statistics(statistics);
    actual.set_properties("metrics", p_metrics);
    actual.set_properties("statistics", p_statistics);
    // cout << "Introducing: " << actual.print_canonical() << endl;
    insert_point(&actual);
  }
  inp.close();
  return true;
}

st_preprocess_pipeline::st_preprocess_pipeline() { disabled = true; }
st_preprocess_pipeline::st_preprocess_pipeline(double box_cox_transform_v,
                                               bool box_cox_is_log_v) {
  disabled = true;
  if (box_cox_transform_v == 0.0)
    box_cox_transform_v = 1.0;

  box_cox_transform = box_cox_transform_v;
  box_cox_is_log = box_cox_is_log_v;
}
double st_preprocess_pipeline::pre_process(double value) {
  if (!disabled) {
    if (box_cox_is_log)
      value = log(value);
    else
      value = pow(value, box_cox_transform);

    value = (value - mean) / std;
    return value;
  } else {
    return value;
  }
}
double st_preprocess_pipeline::post_process(double value) {
  if (!disabled) {
    value = (value * std) + mean;

    if (box_cox_is_log)
      value = exp(value);
    else
      value = pow(value, 1 / box_cox_transform);

    return value;
  } else return value;
}

bool st_database::set_outer_parameter(st_env *env, string parname) {
  if (!env->current_design_space->ds_parameters_index.count(parname)) {
    return false;
  }
  if (!env->current_design_space->scalar_parameters.count(parname)) {
    return false;
  }
  int mn = env->current_design_space->get_scalar_min(env, parname);
  int mx = env->current_design_space->get_scalar_max(env, parname);
  int lv = env->current_design_space->get_number_of_scalar_levels(env, parname);

  int index = env->current_design_space->ds_parameters_index[parname];
  return true;
}
