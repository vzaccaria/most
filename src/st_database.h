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
#ifndef ST_DATABASE
#define ST_DATABASE

#include <iomanip>
#include <map>
#include <math.h>
#include <sstream>
#include "st_list.h"
#include "st_object.h"
#include "st_point.h"

class lt_point {
public:
  bool operator()(const vector<int> &v1, const vector<int> &v2) const {
    if (v1.size() != v2.size())
      return false;
    for (int i = 0; i < v1.size(); i++) {
      if (v1[i] < v2[i])
        return true;
      if (v1[i] > v2[i])
        return false;
    }
    return false;
  }
};

class st_point_set : public map<vector<int>, st_point *, lt_point> {
  int size;

public:
  st_point_set() { size = 0; };
  st_point_set(st_point_set &p);
  ~st_point_set();
  void erase_point(iterator i) {
    if (i->second)
      delete i->second;
    erase(i);
    size--;
  }
  void insert_point(st_point *x);
  int get_size() { return size; };
};

class st_preprocess_pipeline {
public:
  bool disabled;

  double box_cox_transform;
  bool box_cox_is_log;

  double mean;
  double std;

  st_preprocess_pipeline();
  st_preprocess_pipeline(double box_cox_transform_v, bool box_cox_is_log_v);
  double pre_process(double value);
  double post_process(double value);
};

class st_doe;

class st_database {
  st_point_set *points; // list of points simulated

  void copy_from(st_point_set *points);

public:
  st_database();
  st_database(st_database &);
  ~st_database();
  st_database(st_env *, st_doe *);

  void insert_point(const st_point *);
  int write_to_file(const char *filename);
  int write_to_m3_file(const char *filename);
  int write_to_m3_file_objectives(const char *filename);
  int write_octave(const char *filename);
  int read_from_file(const char *filename);
  int import_from_file(st_env *env, const char *filename, bool use_symbols);
  bool acquire_database_record(string &line, map<int, pair<int, int> > mapping,
                               st_env *env, bool use_symbols);
  int count_points();

  void report(st_env *, bool, bool, bool, bool);
  void clear();
  st_point_set *get_set_of_points();
  st_database &operator=(st_database &orig);
  void attach(st_database &orig);
  st_point *look_for_point(st_point *s);
  void filter_cluster(int c);
  void filter_points(st_env *, bool violating);
  void filter_valid(bool violating);
  void filter_parameter_level(st_env *, string parname, int level);
  void cache_update(st_env *env);
  bool set_outer_parameter(st_env *env, string parname);

  /** RSM related methods, to be used after xdr_init has been called */
  /* The following initializes mean and std in the pipes */
  void xdr_init(st_env *, double box_cox, bool box_cox_is_log);
  void xdr_init_predictors_only(st_env *);
  bool xdr_write(st_env *, string file_name);
  bool xdr_write_predictors_only(st_env *, string file_name);
  bool xdr_read(st_env *, string file_name);
  void xdr_mean_and_std(vector<double> &mean, vector<double> &std);

  int xdr_n_metrics;
  int xdr_n_statistics;
  int xdr_n_predictors;
  vector<st_preprocess_pipeline> xdr_metric_pipes;
  vector<st_preprocess_pipeline> xdr_statistic_pipes;
};

/*
 * New pretty print functionality, to gradually supersede old reporting (Sep.
 * 2011)
 */

extern void st_printf_tabbed(string a, int maxsize);
extern void st_printf_tabbed_rev(string a, int maxsize);
extern void st_printf_tabbed_cen(string a, int maxsize);

#define cut(sa, sz) ((sa.size() > sz) ? (sa.substr(0, sz - 2) + "..") : sa)
#define cutstrong(sa, sz) ((sa.size() > sz) ? ("*") : sa)

#define JUST_LEFT 0
#define JUST_RIGHT 1
#define JUST_CENTER 2

#define TYPE_FLOAT 0
#define TYPE_STRING 1

#define DEFAULT_FLOAT_DECIMALS 2

class st_table {
  string header;
  int columns;
  bool large_floats;
  int size;

  int tmp_col;

  vector<int> column_size;
  vector<string> column_name;
  vector<int> justify;
  vector<int> type;

public:
  void clear() {
    column_name.clear();
    column_size.clear();
    justify.clear();
    type.clear();
    columns = 0;
    size = 0;
  }

  st_table() {
    columns = 0;
    large_floats = false;
    size = 0;
  }

  void set_header(string hd) { header = hd; }

  void add_column(string iname, int isize, int itype, int ijustify) {
    column_name.push_back(iname);
    column_size.push_back(isize);
    justify.push_back(ijustify);
    type.push_back(itype);
    size += isize;
    columns++;
  }

  void begin_row() {
    cout << "|";
    tmp_col = 0;
  }

  void end_row() { cout << endl; }

  void add_column_string(string nm) {
    if (justify[tmp_col] == JUST_LEFT)
      st_printf_tabbed(cutstrong(nm, column_size[tmp_col]),
                       column_size[tmp_col]);
    else {
      if (justify[tmp_col] == JUST_RIGHT)
        st_printf_tabbed_rev(cutstrong(nm, column_size[tmp_col]),
                             column_size[tmp_col]);
      else
        st_printf_tabbed_cen(cutstrong(nm, column_size[tmp_col]),
                             column_size[tmp_col]);
    }
    cout << "|";
    tmp_col++;
  }

  void add_column_float(double nm) {
    ostringstream str;
    str << fixed << setprecision(DEFAULT_FLOAT_DECIMALS) << nm;
    add_column_string(str.str() + " ");
  }

  void finish_header() {
    cout << "+";
    for (int i = 0; i < columns; i++) {
      for (int j = 0; j < column_size[i]; j++)
        cout << "-";
      cout << "+";
    }
    cout << endl;
  }

  void print_header() {
    int total_chars = size + columns + 1;
    int mid = total_chars / 2 - (header.size() + 4) / 2;
    int c = 0;
    if (mid < 0) {
      /* dont print header */
    } else {
      cout << "+";
      c++;
      for (int i = 1; i < mid; i++) {
        cout << "-";
        c++;
      }
      cout << "[ " << header << " ]";
      c += 4 + header.size();
      for (; c < (total_chars - 1); c++) {
        cout << "-";
      }
      cout << "+" << endl;
    }
    cout << "|";
    for (int i = 0; i < columns; i++) {
      st_printf_tabbed_cen(cut(column_name[i], column_size[i]), column_size[i]);
      cout << "|";
    }
    cout << endl;
    finish_header();
  }
};
#endif
