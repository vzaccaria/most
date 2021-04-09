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
#include <assert.h>
#include <math.h>
#include <sstream>
#include "st_command_list.h"
#include "st_commands.h"
#include "st_opt_utils.h"
#include "st_rand.h"
#include "st_rsm.h"
#include "st_sim_utils.h"
#include "st_xdr_api.h"
#include "st_design_space.h"

#define RSM_CODE_PLAIN 0
#define RSM_CODE_EXP 4

void st_rsm_compute_outstring(st_env *env, bool interaction, bool square,
                              vector<int> &exclude, vector<string> &outstring) {
  outstring.clear();
  for (int i = 0; i < exclude.size(); i++) {
    if (!exclude[i]) {
      outstring.push_back(env->current_design_space->ds_parameters[i].name);
    }
  }
  if (square) {
    int sz = outstring.size();
    for (int i = 0; i < sz; i++) {
      outstring.push_back(outstring[i] + "^2");
    }
  }
  if (interaction) {
    if (outstring.size() > 1) {
      int sz = outstring.size();
      for (int i = 0; i < sz - 1; i++) {
        for (int k = i + 1; k < sz; k++) {
          outstring.push_back(outstring[i] + "*" + outstring[k]);
        }
      }
    }
  }
}

void st_rsm_compute_coded(st_point &p, vector<int> &out, st_env *env, int code,
                          bool interaction, bool square, vector<int> &exclude) {
  out.clear();
  for (int i = 0; i < p.size(); i++) {
    if (!exclude[i]) {
      switch (code) {
      case RSM_CODE_PLAIN:
        out.push_back(p[i]);
        break;
      case RSM_CODE_EXP:
        out.push_back((int)pow(2, p[i]));
        break;
      }
    }
  }
  if (square) {
    int sz = out.size();
    for (int i = 0; i < sz; i++) {
      out.push_back(out[i] * out[i]);
    }
  }
  if (interaction) {
    if (out.size() > 1) {
      int sz = out.size();
      for (int i = 0; i < sz - 1; i++) {
        for (int k = i + 1; k < sz; k++) {
          out.push_back(out[i] * out[k]);
        }
      }
    }
  }
}

double st_rsm_compute_figure(st_env *env, vector<int> &design,
                             vector<double> &coeff, int number_of_metrics,
                             int number_of_statistics, double &mean,
                             double &stdev, double &maxm, int normax,
                             double preprocess, bool use_log) {
  double j = coeff[0];
  for (int k = 0; k < design.size(); k++) {
    j = j + coeff[1 + k] * design[k];
  }

  if (!normax)
    j = j * stdev + mean;
  else
    j = j * maxm;

  if (!use_log)
    j = pow(j, 1 / preprocess);
  else
    j = exp(j);

  return j;
}

void st_rsm_parse_exclusion_list(st_env *env, st_object *exclusion,
                                 vector<int> &exclude, int &num_excluded) {
  exclude.clear();

  int num_of_parameters = env->current_design_space->ds_parameters.size();

  exclude.resize(num_of_parameters, 0);
  num_excluded = 0;
  if (!exclusion)
    return;
  if (!to<st_list *>(exclusion)) {
    prs_display_error(
        "Exclusion parameters should be a list. Now excluding nothing");
    return;
  }
  st_list *l = to<st_list *>(exclusion);
  list<st_object *>::iterator i;
  bool error = false;
  string m = " - ";
  for (i = l->begin(); i != l->end(); i++) {
    st_object *x = *i;
    if (!to<st_string *>(x)) {
      error = true;
      continue;
    }
    st_string *s = to<st_string *>(x);
    string sss = s->get_string();
    if (env->current_design_space->ds_parameters_index.count(sss)) {
      int index = env->current_design_space->ds_parameters_index[sss];
      exclude[index] = 1;
      m = m + s->get_string() + " ";
      num_excluded += 1;
    } else
      error = true;
  }
  if (error)
    prs_display_error("Some errors in the computation of the exclusion list");

  prs_display_message(("Excluding " + m).c_str());
}

st_object *get_opt(st_map *parameters, string name, int index) {
  st_object *v = st_parse_command_get_opt_object(parameters, name);

  if (!v)
    return NULL;

  if (!to<st_vector *>(v))
    return v;

  st_vector *vect = to<st_vector *>(v);

  if (index >= vect->size())
    return NULL;

  st_object *el = vect->get(index).gen_copy();

  delete vect;

  return el;
}

int get_opt_integer(st_map *parameters, string name, int index) {
  int res = 0;
  st_object *obj = get_opt(parameters, name, index);
  if (obj) {
    if (to<st_integer *>(obj)) {
      res = to<st_integer *>(obj)->get_integer();
    } else {
      prs_display_message("Expecting integer or vector of integers at '" +
                          name + "'");
    }
    delete obj;
  }
  return res;
}

string get_opt_string(st_map *parameters, string name, int index) {
  string res = "";
  st_object *obj = get_opt(parameters, name, index);
  if (obj) {
    if (to<st_string *>(obj)) {
      res = to<st_string *>(obj)->get_string();
    } else {
      prs_display_message("Expecting string or vector of strings at '" + name +
                          "'");
    }
    delete obj;
  }
  return res;
}

double get_opt_double(st_map *parameters, string name, int index, double def) {
  double res = def;
  st_object *obj = get_opt(parameters, name, index);
  if (obj) {
    if (to<st_double *>(obj)) {
      res = to<st_double *>(obj)->get_double();
    } else {
      prs_display_message("Expecting double or vector of doubles at '" + name +
                          "'");
    }
    delete obj;
  }
  return res;
}

typedef struct regress_data {
  int number_of_metrics;
  int number_of_statistics;
  int number_of_parameters;
  vector<double> coeff;
  double Radj;
  double F;
  double PF;
  vector<double> T;
  vector<double> PT;
  double mean;
  double variance;
  double stdev;
  double max;
  bool normax;
  bool square;
  vector<int> exclusion_list;
  bool interaction;
  bool use_log;
  double preprocess;
  int code;
  vector<double> residuals;
  vector<double> zj;
} regress_data;

#define NUM_INTERCEPT 1
#define NUM_CHISQ 0
#define NUM_PREDICTORS (num_predictors)
#define NUM_PARS (NUM_PREDICTORS + NUM_INTERCEPT)
#define NUM_F_P_RAdj 3
#define NUM_T_PT ((NUM_PARS)*2)
#define TOT_NUM (NUM_CHISQ + NUM_PARS + NUM_F_P_RAdj + NUM_T_PT)

bool st_rsm_read_from_raw_data(vector<double> &raw_data, int num_predictors,
                               vector<double> &coeff, double &Radj, double &F,
                               double &PF, vector<double> &T,
                               vector<double> &PT) {
  if (raw_data.size() < TOT_NUM) {
    return false;
  }
  coeff.clear();
  int start_pos = NUM_CHISQ;
  for (int ind = 0; ind < NUM_PARS; ind++) {
    cout << "coefficient " << ind << ": " << raw_data[start_pos + ind] << endl;
    coeff.push_back(raw_data[start_pos + ind]);
  }
  F = (raw_data[start_pos + NUM_PARS]);
  PF = (raw_data[start_pos + NUM_PARS + 1]);
  Radj = (raw_data[start_pos + NUM_PARS + 2]);
  start_pos += NUM_PARS + NUM_F_P_RAdj;
  T.clear();
  PT.clear();
  for (int ind = 0; ind < NUM_PARS; ind++) {
    T.push_back(raw_data[start_pos + 2 * ind]);
    PT.push_back(raw_data[start_pos + 2 * ind + 1]);
  }
  return true;
}

bool st_rsm_linear_estimate_metric_or_statistic(st_env *env, st_map *parameters,
                                                st_database *source,
                                                int figure_index,
                                                regress_data &data) {
  string cs;
  data.interaction = get_opt_integer(parameters, "interaction", figure_index);
  data.square = get_opt_integer(parameters, "order", figure_index) > 1;
  data.normax = get_opt_integer(parameters, "normax", figure_index);
  string prep = get_opt_string(parameters, "preprocess", figure_index);
  int verbose = get_opt_integer(parameters, "test", figure_index);
  if (verbose)
    prs_display_message("Verbose on");
  data.use_log = false;

  if (prep == "log")
    data.use_log = true;

  double accept_prob = get_opt_double(parameters, "filter", figure_index, 1.0);

  int accept_percentage = (int)ceil(accept_prob * 100.0);
  if (accept_percentage < 0 || accept_percentage > 100)
    accept_percentage = 100;

  data.preprocess =
      (get_opt_double(parameters, "preprocess", figure_index, 1.0));
  if (data.preprocess == 0.0) {
    data.preprocess = 1.0;
  }
  if (data.use_log)
    prs_display_message("Using preprocess = log ");
  else {
    prs_display_message_n_value_m("Using preprocess = ", data.preprocess, " ");
  }

  vector<string> outstring;
  cs = get_opt_string(parameters, "code", figure_index);
  st_object *exclusion = get_opt(parameters, "exclude", figure_index);
  int num_excluded;
  st_rsm_parse_exclusion_list(env, exclusion, data.exclusion_list,
                              num_excluded);
  delete exclusion;

  if (cs == "")
    cs = "plain";
  if (cs == "plain")
    data.code = RSM_CODE_PLAIN;
  if (cs == "exp")
    data.code = RSM_CODE_EXP;
  if (cs != "exp" && cs != "three" && cs != "two" && cs != "two_zero" &&
      cs != "plain") {
    prs_display_error("No valid coding specified.");
    return false;
  }

  ostringstream linestream;
  linestream << figure_index;
  string number = linestream.str();
  string file_out = "st_rsm_out_" + number + ".dat";
  string file_in = "st_rsm_in_" + number + ".dat";
  string command = "rm -rf " + file_out;
  if (!shell_command(command)) {
    prs_display_message("Failed to remove temporary files");
    return false;
  }
  ofstream fx(file_out.c_str());
  if (!fx.good()) {
    prs_display_message("Failed to write the regressor input file");
    return false;
  }
  data.number_of_parameters = env->current_design_space->ds_parameters.size();
  data.number_of_metrics = env->current_design_space->metric_index.size();
  data.number_of_statistics = 0;

  st_database *pdb = NULL;
  st_point_set *points;
  st_point_set::iterator i;

  if (accept_percentage < 100) {
    pdb = new st_database();

    st_point_set *src_points = source->get_set_of_points();
    for (i = src_points->begin(); i != src_points->end(); i++) {
      int p = rand() % 100 + 1;
      if (p <= accept_percentage)
        pdb->insert_point(i->second);
    }
    points = pdb->get_set_of_points();
  } else {
    points = source->get_set_of_points();
  }

#define GETV(v) ((use_log) ? log(v) : pow((v), preprocess))
#define GETM(p, k) (GETV((p)->get_metrics(k)))
#define GETS(p, k) (GETV((p)->get_statistics(k)))
#define SETV(v) ((use_log) ? exp(v) : pow((v), 1 / preprocess))

#define PGETV(v) (v)
#define PGETM(p, k) (PGETV((p)->get_metrics(k)))
#define PGETS(p, k) (PGETV((p)->get_statistics(k)))
#define PGET(p, index)                                                         \
  (((index) < data.number_of_metrics)                                          \
       ? PGETM(p, (index))                                                     \
       : PGETS(p, (index) - (data.number_of_metrics)))

#define DGETV(v) ((data.use_log) ? log(v) : pow((v), data.preprocess))
#define DGETM(p, k) (DGETV((p)->get_metrics(k)))
#define DGETS(p, k) (DGETV((p)->get_statistics(k)))
#define DGET(p, index)                                                         \
  (((index) < data.number_of_metrics)                                          \
       ? DGETM(p, (index))                                                     \
       : DGETS(p, (index) - (data.number_of_metrics)))
#define DSET(metrics, statistics, value, index)                                \
  (((index) < number_of_metrics)                                               \
       ? metrics->insert(index, st_double(value))                              \
       : statistics->insert(index - number_of_metrics, st_double(value)))
  int num = 0;
  data.mean = 0;
  for (i = points->begin(); i != points->end(); i++) {
    if (!i->second->get_error()) {
      // cout << "Converting " << PGET(i->second, figure_index) << " into " <<
      // DGET(i->second, figure_index) << endl;

      double value = DGET(i->second, figure_index);
      if (isnan(value) || isinf(value)) {
        if (pdb)
          delete pdb;
        prs_display_error("Data cannot be preprocessed/normalized");
        return false;
      }
      data.mean += value;
      data.max = max(data.max, fabs(value));
      num++;
    }
  }
  if (num < 2) {
    if (pdb)
      delete pdb;
    prs_display_error("Cannot normalize");
    return false;
  }
  data.mean = data.mean / num;
  data.variance = 0;
  for (i = points->begin(); i != points->end(); i++) {
    if (!i->second->get_error()) {
      data.variance += pow(DGET(i->second, figure_index) - data.mean, 2);
    }
  }
  data.stdev = sqrt(data.variance / (num - 1));
  int num_predictors = 0;
  for (i = points->begin(); i != points->end(); i++) {
    if (!i->second->get_error()) {
      assert(i->second->check_consistency(env));
      double observation;
      if (!data.normax)
        observation = (DGET(i->second, figure_index) - data.mean) / data.stdev;
      else
        observation = (DGET(i->second, figure_index) / data.max);
      st_point *p = i->second;
      vector<int> design;
      st_rsm_compute_coded(*p, design, env, data.code, data.interaction,
                           data.square, data.exclusion_list);
      num_predictors = design.size();
      vector<double> observations;
      observations.push_back(observation);
      st_xdr_write_design(fx, design, observations);
    }
  }
  fx.close();
  command = "regressor";
  string cbp;
  if (env->shell_variables.get_string("current_build_path", cbp)) {
    command = cbp + "/bin/" + command;
  }
  command = command + " " + file_out + " " + file_in;
  shell_command(("rm -rf " + file_in));
  if (!shell_command(command)) {
    if (pdb)
      delete pdb;

    throw std::logic_error("Failed to run the regressor");
    return false;
  }
  vector<double> raw_data;
  ifstream ifx(file_in.c_str());
  if (!st_xdr_read_vector(ifx, raw_data)) {
    if (pdb)
      delete pdb;
    throw std::logic_error("Unable to read raw data from the regressor");
    return false;
  }
  st_xdr_read_vector(ifx, data.residuals);
  st_xdr_read_vector(ifx, data.zj);
  ifx.close();
  if (!st_rsm_read_from_raw_data(raw_data, num_predictors, data.coeff,
                                 data.Radj, data.F, data.PF, data.T, data.PT)) {
    if (pdb)
      delete pdb;
    throw std::logic_error("Unable to parse raw data from the regressor");
    return false;
  }

  if (verbose) {
    st_rsm_compute_outstring(env, data.interaction, data.square,
                             data.exclusion_list, outstring);
#define MET 0
#define STA 1
#define OTH 2
    int m = MET;
    if (figure_index == data.number_of_metrics)
      m = STA;

    bool res;
    string name;
    if (m == MET) {
      name = env->current_design_space->metric_names[figure_index];
    } else {
      bool rsm_condition_that_should_never_happen = false;
      st_assert(rsm_condition_that_should_never_happen);
    }
    printf("Metric name: %s\n",
           env->current_design_space->metric_names[figure_index].c_str());
    printf("data.variance: %.20f\n", data.variance);
    printf("data.stdev: %.20f\n", data.stdev);
    printf("data.mean: %.20f\n", data.mean);
    printf("'%s' significance test \n", name.c_str());
    printf("F: %e p(f>F):%f Radj:%f \n", data.F, data.PF, data.Radj);
    printf("----------------------------\n");
    printf("%-30s   %-15s   %-15s   %-15s \n", "Coefficient", "Value", "t0",
           "p(|t|>t0)");
    printf("%-30s   %-15f   %-15e   %-15f \n", "Intercept", data.coeff[0],
           data.T[0], data.PT[0]);
    for (int j = 0; j < outstring.size(); j++) {
      printf("%-30s   %-15f   %-15e   %-15f \n", outstring[j].c_str(),
             data.coeff[j + 1], data.T[j + 1], data.PT[j + 1]);
    }
    printf("\n");
  }
  if (pdb)
    delete pdb;
  return true;
}

class st_rsm_linear_regression : public st_rsm {
public:
  st_object *train_rsm(st_map *parameters, st_database *source, st_env *env) {
    /** Initialize the random number generator*/
    srand(time(0));

    vector<regress_data> tot_data;
    int replicate = st_parse_command_get_opt_integer(parameters, "replicate");

    st_database *dest = new st_database();

    prs_display_message(
        "Invoking Regressor and Computing Linear Regression Coefficients.");
    int number_of_metrics = env->current_design_space->metric_names.size();
    int number_of_statistics = 0;

    int total_figures = number_of_metrics + number_of_statistics;
    for (int ff = 0; ff < total_figures; ff++) {
      regress_data data;
      bool res = st_rsm_linear_estimate_metric_or_statistic(env, parameters,
                                                            source, ff, data);
      if (!res) {
        env->insert_new_database(dest, env->current_destination_database_name);
        return new st_integer(0);
      }
      tot_data.push_back(data);
    }

    int l = 0;
    st_point_set *points = source->get_set_of_points();
    st_point_set::iterator i;
    if (replicate) {
      for (i = points->begin(); i != points->end(); i++) {
        if (!i->second->get_error()) {
          vector<int> design;
          vector<double> figures;
          for (int ff = 0; ff < total_figures; ff++) {
            vector<vector<int> > design_per_metrics;
            vector<int> design;
            st_rsm_compute_coded(*i->second, design, env, tot_data[ff].code,
                                 tot_data[ff].interaction, tot_data[ff].square,
                                 tot_data[ff].exclusion_list);
            double value = st_rsm_compute_figure(
                env, design, tot_data[ff].coeff, number_of_metrics,
                number_of_statistics, tot_data[ff].mean, tot_data[ff].stdev,
                tot_data[ff].max, tot_data[ff].normax, tot_data[ff].preprocess,
                tot_data[ff].use_log);
            figures.push_back(value);
          }
          st_vector *metrics = new st_vector();
          st_vector *statistics = new st_vector();
          vector<int> the_point_coord = *(i->second);
          st_point *actual = new st_point(the_point_coord);
          for (int ff = 0; ff < total_figures; ff++) {
            DSET(metrics, statistics, figures[ff], ff);
          }
          actual->set_properties("metrics", *metrics);
          actual->set_properties("statistics", *statistics);
          delete metrics;
          delete statistics;
          dest->insert_point(actual);
          delete actual;
        }
      }

    } else {
      bool finished = false;
      st_point actual_point = env->current_design_space->begin(env);
      while (!finished) {
        vector<int> design;
        vector<double> figures;
        for (int ff = 0; ff < total_figures; ff++) {
          vector<vector<int> > design_per_metrics;
          vector<int> design;
          st_rsm_compute_coded(actual_point, design, env, tot_data[ff].code,
                               tot_data[ff].interaction, tot_data[ff].square,
                               tot_data[ff].exclusion_list);
          double value = st_rsm_compute_figure(
              env, design, tot_data[ff].coeff, number_of_metrics,
              number_of_statistics, tot_data[ff].mean, tot_data[ff].stdev,
              tot_data[ff].max, tot_data[ff].normax, tot_data[ff].preprocess,
              tot_data[ff].use_log);
          figures.push_back(value);
        }
        st_vector *metrics = new st_vector();
        st_vector *statistics = new st_vector();
        vector<int> the_point_coord = (actual_point);
        st_point *actual = new st_point(the_point_coord);
        for (int ff = 0; ff < total_figures; ff++) {
          DSET(metrics, statistics, figures[ff], ff);
        }
        actual->set_properties("metrics", *metrics);
        actual->set_properties("statistics", *statistics);
        delete metrics;
        delete statistics;
        dest->insert_point(actual);
        delete actual;
        finished = !env->current_design_space->next(env, actual_point);
        l++;
      }
    }
    env->insert_new_database(dest, env->current_destination_database_name);
    st_vector *res = new st_vector();
    for (int j = 0; j < tot_data.size(); j++) {
      st_map *m = new st_map();
      m->insert("f", st_double(tot_data[j].F));
      m->insert("pf", st_double(tot_data[j].PF));
      m->insert("radj", st_double(tot_data[j].Radj));
      m->insert("residuals", st_vector(tot_data[j].residuals));
      m->insert("zj", st_vector(tot_data[j].zj));
      res->insert(j, *m);
      delete m;
    }
    return res;
  };
  string print_information() {
    return "--order={1,2} --interaction={true, false} "
           "--code={\"plain\",\"two\",\"two_zero\",\"three\",\"exp\"} "
           "--normax={true,false} --preprocess={FLOAT, \"log\"} --filter=FLOAT";
  };
};

class st_rsm_shepard : public st_rsm {
public:
  st_object *train_rsm(st_map *parameters, st_database *source, st_env *env) {

    st_database *RSM_DB = new st_database();

    int num_an = 0;

    string prep = st_parse_command_get_opt_string(parameters, "preprocess");
    bool use_log = false;

    if (prep == "log")
      use_log = true;

    double preprocess =
        (st_parse_command_get_opt_double(parameters, "preprocess", 1.0));
    if (preprocess == 0.0) {
      preprocess = 1.0;
    }

    if (use_log)
      prs_display_message("Using preprocess = log ");
    else {
      prs_display_message_n_value_m("Using preprocess = ", preprocess, " ");
    }

    st_point_set *database_list = source->get_set_of_points();

    bool finished = false;

    int num_of_prs = env->current_design_space->ds_parameters.size();

    int number_of_metrics = env->current_design_space->metric_names.size();
    int number_of_statistics = 0;

    st_point actual_point(num_of_prs);

    actual_point = env->current_design_space->begin(env);

    int power = st_parse_command_get_opt_integer(parameters, "power");
    if (!power)
      power = 2;
    int neg_power = 0 - power;

    while (!finished) {
      double overall_weighted_distance = 0;
      st_point_set::iterator p;
      st_vector *metrics = new st_vector;
      st_vector *statistics = new st_vector;

      st_point *my_point = source->look_for_point(&actual_point);

      bool to_be_evaluated = false;

      if (!my_point)
        to_be_evaluated = true;
      else {
        if (my_point->get_error())
          to_be_evaluated = false;
      }

      if (!to_be_evaluated) {
        RSM_DB->insert_point(my_point);
      } else {
        vector<double> partial_metric_vector;
        vector<double> partial_statistic_vector;
        partial_metric_vector.resize(number_of_metrics, 0);
        partial_statistic_vector.resize(number_of_statistics, 0);

        for (p = database_list->begin(); p != database_list->end(); p++) {
          double distance_from_actual = 0;
          st_point *x = p->second;
          st_assert(x->check_consistency(env));
          if (!x->get_error()) {
            for (int parameter = 0; parameter < num_of_prs;
                 parameter++) // x->size
            {
              distance_from_actual += pow_double_to_int(
                  ((*x)[parameter] - actual_point[parameter]), 2);
            }
            distance_from_actual = sqrt(distance_from_actual);
            overall_weighted_distance += pow(distance_from_actual, neg_power);
          }
        }

        for (p = database_list->begin(); p != database_list->end(); p++) {
          double distance_from_actual = 0;
          double weighted_distance_from_actual = 0;
          st_point *x = p->second;
          if (!x->get_error()) {
            for (int parameter = 0; parameter < num_of_prs; parameter++) {
              distance_from_actual += pow_double_to_int(
                  (*x)[parameter] - actual_point[parameter], 2);
            }
            distance_from_actual = sqrt(distance_from_actual);

            weighted_distance_from_actual =
                pow(distance_from_actual, neg_power);

            for (int pos = 0; pos < number_of_metrics; pos++) {
              double x_m_pos = GETM(x, pos);
              partial_metric_vector[pos] +=
                  ((weighted_distance_from_actual / overall_weighted_distance) *
                   (x_m_pos));
            }
            for (int pos = 0; pos < number_of_statistics; pos++) {
              double x_s_pos = GETS(x, pos);
              partial_statistic_vector[pos] +=
                  ((weighted_distance_from_actual / overall_weighted_distance) *
                   (x_s_pos));
            }
          }
        }

        for (int pos = 0; pos < number_of_metrics; pos++) {
          metrics->insert(pos, st_double(SETV(partial_metric_vector[pos])));
        }

        for (int pos = 0; pos < number_of_statistics; pos++) {
          statistics->insert(pos,
                             st_double(SETV(partial_statistic_vector[pos])));
        }

        actual_point.set_properties("metrics", *metrics);
        actual_point.set_properties("statistics", *statistics);
        delete metrics;
        delete statistics;
        RSM_DB->insert_point(&actual_point);
      }
      finished = !env->current_design_space->next(env, actual_point);
      num_an++;
    }

    env->insert_new_database(RSM_DB, env->current_destination_database_name);
    return new st_integer(1);
  };

  string print_information() {
    return "--power=<value> --preprocess={N, \"log\"}";
  };
};

#include <iomanip>
#include <sstream>

string itos(int i) {
  ostringstream str;
  str << i;
  return str.str();
}

class st_rsm_rbf : public st_rsm {
public:
  st_object *train_rsm(st_map *parameters, st_database *source, st_env *env) {
    string prep = st_parse_command_get_opt_string(parameters, "preprocess");
    bool use_log = false;

    if (prep == "log")
      use_log = true;

    double preprocess =
        (st_parse_command_get_opt_double(parameters, "preprocess", 1.0));
    if (preprocess == 0.0) {
      preprocess = 1.0;
    }

    if (use_log)
      prs_display_message("Using preprocess = log ");
    else {
      prs_display_message_n_value_m("Using preprocess = ", preprocess, " ");
    }
    string type = st_parse_command_get_opt_string(parameters, "type");
    int the_rbf_type = 1;

    if (type == "power")
      the_rbf_type = 1;
    if (type == "power_log")
      the_rbf_type = 2;
    if (type == "sqrt")
      the_rbf_type = 3;
    if (type == "inv_sqrt")
      the_rbf_type = 4;
    if (type == "exp")
      the_rbf_type = 5;

    int parameter = st_parse_command_get_opt_integer(parameters, "parameter");

    st_database *internal_source = new st_database(*source);
    internal_source->xdr_init(env, preprocess, use_log);
    internal_source->xdr_write(env, "rbf_source_database");
    if (!env->current_doe) {
      prs_display_error("Please define a DoE before training this RSM");
      return new st_integer(0);
    }
    st_database predictors(env, env->current_doe);
    predictors.xdr_init_predictors_only(env);
    predictors.xdr_write_predictors_only(env, "rbf_predictors");

    string command = "rbf_interpolator";
    string cbp;
    if (env->shell_variables.get_string("current_build_path", cbp)) {
      command = cbp + "/bin/" + command;
    }

    int num_estimates = predictors.xdr_n_metrics + predictors.xdr_n_statistics;

    command = command + " " + itos(num_estimates);

    for (int i = 0; i < num_estimates; i++)
      command = command + " " + itos(the_rbf_type);

    for (int i = 0; i < num_estimates; i++)
      command = command + " " + itos(parameter);

    command = command + " rbf_source_database rbf_predictors rbf_predictions";

    if (!shell_command(command)) {
      prs_display_error("The rbf interpolator returned an error");
      return new st_integer(0);
    }

    if (!internal_source->xdr_read(env, "rbf_predictions")) {
      prs_display_error(
          "The rbf interpolator generated an unreadable set of predictions");
      return new st_integer(0);
    }

    env->insert_new_database(internal_source,
                             env->current_destination_database_name);

    return new st_integer(1);
  };
  string print_information() {
    return "--type={\"power\", \"power_log\", \"sqrt\", \"inv_sqrt\", \"exp\" "
           "} --parameter=INT --preprocess={FLOAT, \"log\"}";
  };
};

class st_rsm_nn : public st_rsm {
public:
  st_object *train_rsm(st_map *parameters, st_database *source, st_env *env) {
    string prep = st_parse_command_get_opt_string(parameters, "preprocess");
    bool use_log = false;

    if (prep == "log")
      use_log = true;

    double preprocess =
        (st_parse_command_get_opt_double(parameters, "preprocess", 1.0));
    if (preprocess == 0.0) {
      preprocess = 1.0;
    }

    if (use_log)
      prs_display_message("Using preprocess = log ");
    else {
      prs_display_message_n_value_m("Using preprocess = ", preprocess, " ");
    }
    string type = st_parse_command_get_opt_string(parameters, "effort");
    if (type != "fast" && type != "low" && type != "medium" && type != "high")
      type = "fast";

    st_database *internal_source = new st_database(*source);
    internal_source->xdr_init(env, preprocess, use_log);
    internal_source->xdr_write(env, "nn_source_database");
    if (!env->current_doe) {
      prs_display_error("Please define a DoE before training this RSM");
      return new st_integer(0);
    }
    st_database predictors(env, env->current_doe);
    predictors.xdr_init_predictors_only(env);
    predictors.xdr_write_predictors_only(env, "nn_predictors");

    string command = "nn";
    string cbp;
    if (env->shell_variables.get_string("current_build_path", cbp)) {
      command = cbp + "/bin/" + command;
    }

    // > nn --all training_filename input_to_compute network_filename
    // output_filename 'constr'

    int num_estimates = predictors.xdr_n_metrics + predictors.xdr_n_statistics;

    command =
        command +
        " --all nn_source_database nn_predictors nn_topology nn_predictions " +
        type;

    if (!shell_command(command)) {
      prs_display_error("The neural network interpolator returned an error");
      return new st_integer(0);
    }

    if (!internal_source->xdr_read(env, "nn_predictions")) {
      prs_display_error(
          "The rbf interpolator generated an unreadable set of predictions");
      return new st_integer(0);
    }

    env->insert_new_database(internal_source,
                             env->current_destination_database_name);

    return new st_integer(1);
  };
  string print_information() {
    return "--effort={\"fast\", \"low\", \"medium\", \"high\" } "
           "--preprocess={FLOAT, \"log\"}";
  };
};

class st_rsm_sh_i : public st_rsm {
public:
  st_object *train_rsm(st_map *parameters, st_database *source, st_env *env) {
    string prep = st_parse_command_get_opt_string(parameters, "preprocess");
    bool use_log = false;

    if (prep == "log")
      use_log = true;

    double preprocess =
        (st_parse_command_get_opt_double(parameters, "preprocess", 1.0));
    if (preprocess == 0.0) {
      preprocess = 1.0;
    }

    if (use_log)
      prs_display_message("Using preprocess = log ");
    else {
      prs_display_message_n_value_m("Using preprocess = ", preprocess, " ");
    }

    int power = st_parse_command_get_opt_integer(parameters, "power");

    st_database *internal_source = new st_database(*source);
    internal_source->xdr_init(env, preprocess, use_log);
    internal_source->xdr_write(env, "shepard_source_database");
    if (!env->current_doe) {
      prs_display_error("Please define a DoE before training this RSM");
      return new st_integer(0);
    }
    st_database predictors(env, env->current_doe);
    predictors.xdr_init_predictors_only(env);
    predictors.xdr_write_predictors_only(env, "shepard_predictors");

    string command = "sh_i";
    string cbp;
    if (env->shell_variables.get_string("current_build_path", cbp)) {
      command = cbp + "/bin/" + command;
    }

    int num_estimates = predictors.xdr_n_metrics + predictors.xdr_n_statistics;

    command = command +
              " shepard_source_database shepard_predictors shepard_predictions";
    command = command + " " + itos(power);

    if (!shell_command(command)) {
      prs_display_error("The shepard interpolator returned an error");
      return new st_integer(0);
    }

    if (!internal_source->xdr_read(env, "shepard_predictions")) {
      prs_display_error("The shepard interpolator generated an unreadable set "
                        "of predictions");
      return new st_integer(0);
    }

    env->insert_new_database(internal_source,
                             env->current_destination_database_name);

    return new st_integer(1);
  };
  string print_information() {
    return "--power=INT --preprocess={FLOAT, \"log\"}";
  };
};

st_rsm_linear_regression linear_regression;
st_rsm_shepard shepard;
st_rsm_rbf rbf;
st_rsm_nn nn;
st_rsm_sh_i sh_i;

class st_rsm_spline : public st_rsm {
public:
  st_object *train_rsm(st_map *parameters, st_database *source, st_env *env) {
    string prep = st_parse_command_get_opt_string(parameters, "preprocess");
    bool use_log = false;

    if (prep == "log")
      use_log = true;

    double preprocess =
        (st_parse_command_get_opt_double(parameters, "preprocess", 1.0));
    if (preprocess == 0.0) {
      preprocess = 1.0;
    }

    if (use_log)
      prs_display_message("Using preprocess = log ");
    else {
      prs_display_message_n_value_m("Using preprocess = ", preprocess, " ");
    }

    st_database *internal_source = new st_database(*source);
    internal_source->xdr_init(env, preprocess, use_log);
    internal_source->xdr_write(env, "spline_source_database.db");
    if (!env->current_doe) {
      prs_display_error("Please define a DoE before training this RSM");
      return new st_integer(0);
    }
    st_database predictors(env, env->current_doe);
    predictors.xdr_init_predictors_only(env);
    predictors.xdr_write_predictors_only(env, "spline_predictors.db");

    string command = "spline";
    string cbp;
    if (env->shell_variables.get_string("current_build_path", cbp)) {
      command = cbp + "/bin/" + command;
    }

    int num_estimates = predictors.xdr_n_metrics + predictors.xdr_n_statistics;

    command = command + " --trai spline_source_database.db --pred "
                        "spline_predictors.db --output spline_predictions.db";

    shell_command("rm -rf  spline_predictions.db");

    if (!shell_command(command)) {
      prs_display_error("The spline interpolator returned an error");
      return new st_integer(0);
    }

    if (!internal_source->xdr_read(env, "spline_predictions.db")) {
      prs_display_error(
          "The spline interpolator generated an unreadable set of predictions");
      return new st_integer(0);
    }

    env->insert_new_database(internal_source,
                             env->current_destination_database_name);

    return new st_integer(1);
  };
  string print_information() { return "No parameters."; };
};

st_rsm_spline spline_i;

void prs_command_train_rsm_help() {
  printf("\nAvailable models: \n");
  map<string, st_rsm *>::iterator i;
  for (i = current_environment.current_rsms.begin();
       i != current_environment.current_rsms.end(); i++) {
    printf("%s", ("--model=\"" + i->first + "\" " + i->second->print_information())
               .c_str());
    printf("\n");
  }
}

bool prs_command_train_rsm(st_map *parameters) {
  string model_name = st_parse_command_get_opt_string(parameters, "model");
  if (!current_environment.current_rsms.count(model_name)) {
    prs_display_error("RSM model not existing.");
    return false;
  } else {
    prs_display_message("Training RSM");
    string source = st_parse_command_get_opt_string(parameters, "source");
    if (!current_environment.get_database(source)) {
      prs_display_error(
          "Please specify a valid source database for the rsm results");
      current_environment.shell_variables.insert("?", st_integer(0));
      return false;
    }
    st_object *res = current_environment.current_rsms[model_name]->train_rsm(
        parameters, current_environment.get_database(source),
        &current_environment);
    current_environment.shell_variables.insert("?", *res);
    delete res;
  }
  return true;
}

void rsm_init_rsms(st_env *env) {
  env->current_rsms["LINEAR"] = &linear_regression;
  env->current_rsms["SHEPARD"] = &shepard;
  env->current_rsms["RBF_ON_DOE"] = &rbf;
  env->current_rsms["NN_ON_DOE"] = &nn;
  env->current_rsms["SHEPARD_ON_DOE"] = &sh_i;
  env->current_rsms["SPLINE_ON_DOE"] = &spline_i;
}
