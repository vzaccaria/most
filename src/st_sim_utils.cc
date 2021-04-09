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

#include <algorithm>
#include <math.h>
#include <st_object.h>
#include <st_parser.h>
#include <st_rand.h>
#include <st_shell_command.h>
#include <st_sim_utils.h>
#include <st_design_space.h>

st_object *sim_generate_dummy_point(st_env *env) {
  st_point lower(env->current_design_space->ds_parameters.size());
  lower.set_error(ST_ERR_CONSTR_ERR);
  return lower.gen_copy();
}

extern void st_print_point(st_env *env, st_point *p);

bool sim_is_purely_dominated_by(st_env *env, st_point &e, st_point &p) {
  bool p_no_worse_than_e = true;
  bool p_once_better_than_e = false;
  for (int i = 0; i < env->optimization_objectives.size(); i++) {
    double e_m_value = env->optimization_objectives[i]->eval(&e, i);
    double p_m_value = env->optimization_objectives[i]->eval(&p, i);

    if (e_m_value < p_m_value)
      p_no_worse_than_e = false;
    if (e_m_value > p_m_value)
      p_once_better_than_e = true;
  }
  if (p_no_worse_than_e && p_once_better_than_e) {
    return true;
  }
  return false;
}

/** The following code is inconsistent with TCAD'09 when considering unfeasible
 * points. It also has transitive property problems. In fact, assume:
 *
 *  rank for ever point = 1
 *
 *  A = [ 0 1 penalty=2 ]
 *  B = [ 1 2 penalty=0 ]
 *  C = [ 2 0 penalty=1 ]
 *
 *  Then, given the following code, C dominates A, since
 *  the starting condition is that C is always better than A (step 1) and this
 * condition is not impacted by the computation of pure dominance (step 2). So
 * the only thing that impacts the dominance relation is the penalty which is
 * better for C.
 *
 *  Same relations that we can find are: B dominates C
 *
 *  TRANSITIVITY PROBLEM:
 *  From this, it should follow that B dominates A, but step 2 tells that B is
 * not always better than A (being purely dominated by it) but it has a better
 * penalty. So the domination relationship does not hold in one sense.
 *
 *  In the other sense, A doesnt dominate B since it is 'better' from the point
 * of view of the pure dominance but worst in terms of penalty.
 *
 *  PROBLEMS ENCOUNTERED:
 *  As of August '09, this code presented problems during ReSPIR validation
 * since when p_F_1 contains C, it conveys a coverage with respect to p_F_0 >=0
 * (due to A). C is then put into F_0 but is pruned at the next pareto filter
 * due to the presence of B. Neither A or B are removed during pareto filtering
 * since there is no domination relationship between the two. The ReSPIR
 * algorithm continues to spin over a coverage > 0 which is presumably due to
 * the lack of transitive properties of the following function.
 *
 *  SOLUTION:
 *  Go for the solution presented in TCAD, with a relaxed dominance across
 * unfeasible points. In this case B will not dominate C and C will not dominate
 * A.
 */

bool sim_is_strictly_dominated_classic(st_env *env, st_point &e, st_point &p,
                                       bool &feasible_e, bool &feasible_p,
                                       int &rank_e, int &rank_p,
                                       double &penalty_e, double &penalty_p) {

  feasible_e = opt_check_constraints(e, env, rank_e, penalty_e);
  feasible_p = opt_check_constraints(p, env, rank_p, penalty_p);

  if (feasible_e && !feasible_p) {
    return false;
  }

  if (feasible_p) {
    return (sim_is_purely_dominated_by(env, e, p));
  }

  if (!feasible_e && !feasible_p) {
    /** Step 1: starting conditions */
    bool p_no_worse_than_e = true;
    bool p_once_better_than_e = false;

    bool p_min_e = sim_is_purely_dominated_by(env, e, p);
    bool e_min_p = sim_is_purely_dominated_by(env, p, e);

    if (rank_p > rank_e)
      p_no_worse_than_e = false;
    if (rank_p < rank_e)
      p_once_better_than_e = true;

    if (penalty_p > penalty_e)
      p_no_worse_than_e = false;
    if (penalty_p < penalty_e)
      p_once_better_than_e = true;

    /** Step 2: pure dominance computation */
    if (e_min_p)
      p_no_worse_than_e = false;
    if (p_min_e)
      p_once_better_than_e = true;

    if (p_no_worse_than_e && p_once_better_than_e) {
      return true;
    } else
      return false;
  }
  return false;
}

bool sim_is_strictly_dominated_tcad(st_env *env, st_point &e, st_point &p,
                                    bool &feasible_e, bool &feasible_p,
                                    int &rank_e, int &rank_p, double &penalty_e,
                                    double &penalty_p) {

  feasible_e = opt_check_constraints(e, env, rank_e, penalty_e);
  feasible_p = opt_check_constraints(p, env, rank_p, penalty_p);

  if (feasible_e && !feasible_p) {
    return false;
  }

  if (feasible_p) {
    return (sim_is_purely_dominated_by(env, e, p));
  }

  if (!feasible_e && !feasible_p) {
    /** Step 1: starting conditions */
    bool p_no_worse_than_e = true;
    bool p_once_better_than_e = false;

    for (int i = 0; i < env->optimization_objectives.size(); i++) {
      double e_m_value = env->optimization_objectives[i]->eval(&e, i);
      double p_m_value = env->optimization_objectives[i]->eval(&p, i);

      if (e_m_value < p_m_value)
        p_no_worse_than_e = false;
      if (e_m_value > p_m_value)
        p_once_better_than_e = true;
    }

    if (rank_p > rank_e)
      p_no_worse_than_e = false;
    if (rank_p < rank_e)
      p_once_better_than_e = true;

    if (penalty_p > penalty_e)
      p_no_worse_than_e = false;
    if (penalty_p < penalty_e)
      p_once_better_than_e = true;

    if (p_no_worse_than_e && p_once_better_than_e) {
      return true;
    } else
      return false;
  }
  return false;
}

bool use_tcad_dominance = false;

void sim_set_tcad_dominance(bool value) { use_tcad_dominance = value; }

bool sim_is_strictly_dominated(st_env *env, st_point &e, st_point &p,
                               bool &feasible_e, bool &feasible_p, int &rank_e,
                               int &rank_p, double &penalty_e,
                               double &penalty_p) {
  /* As of october 2009, tcad dominance seems compromising the quality of the
   * results of both nsga-ii and respir algorithms (the last one being afflicted
   * by poor convergence problems). As an intermediate solution, nsga-ii will
   * run with the classic operator, while respir will use the new operator until
   * it is modified to run without consistency problems with the old operator.
   * [VZ]*/

  if (use_tcad_dominance)
    return sim_is_strictly_dominated_tcad(env, e, p, feasible_e, feasible_p,
                                          rank_e, rank_p, penalty_e, penalty_p);
  else
    return sim_is_strictly_dominated_classic(env, e, p, feasible_e, feasible_p,
                                             rank_e, rank_p, penalty_e,
                                             penalty_p);
}

bool pareto_verbose = false;

/** Note, p should be consistent. Check for integrity */
bool sim_recompute_pareto_with_new_point(st_env *env, st_database *the_database,
                                         st_point &p) {
  st_point_set::iterator c;
  st_point_set *database_list = the_database->get_set_of_points();
  bool is_pareto = true;
  bool some_real_feasible_removed = false;
  for (c = database_list->begin(); c != database_list->end();) {
    st_point &e = *(c->second);

    /** e cannot have errors nor being inconsistent. */

    bool e_dominates_p = false;
    bool p_dominates_e = false;

    int rank_e;
    double penalty_e;

    int rank_p;
    double penalty_p;

    bool feasible_e;
    bool feasible_p;

    try {
      e_dominates_p =
          sim_is_strictly_dominated(env, p, e, feasible_p, feasible_e, rank_p,
                                    rank_e, penalty_p, penalty_e);
      p_dominates_e =
          sim_is_strictly_dominated(env, e, p, feasible_e, feasible_p, rank_e,
                                    rank_p, penalty_e, penalty_p);
    } catch (exception &e) {
      /** The database contains all the points for which meaningfull objectives
       * have been computed */
      return false;
    }

    if (p_dominates_e) {
      if (pareto_verbose) {
        string ps = p.print();
        string es = e.print();
        cout << "Information: " << ps << " dominates " << es << endl;
      }
      /** Remove e */
      st_point_set::iterator j = c;
      j++;
      /* Delete the point */
      database_list->erase_point(c);
      c = j;
      if (feasible_e)
        some_real_feasible_removed = true;
    } else
      c++;

    if (e_dominates_p) {
      if (pareto_verbose) {
        string ps = p.print();
        string es = e.print();
        cout << "Information: " << es << " dominates " << ps << endl;
      }
      st_assert(!some_real_feasible_removed);
      is_pareto = false;
      break;
    }
  }
  return is_pareto;
}

void sim_compute_pareto_nth(st_env *env, st_database *the_database, int level,
                            bool valid) {
  int n = 0;
  st_database *entire_pareto_db = new st_database();
  while (n < level) {
    st_database *pareto_db = new st_database();
    *pareto_db = *the_database;
    prs_display_message("Computing pareto front");
    sim_compute_pareto(env, pareto_db, valid);

    /** Remove pareto_db from the_database */
    prs_display_message("Removing pareto front from db");
    st_point_set *subset = pareto_db->get_set_of_points();
    map<vector<int>, st_point *, lt_point>::iterator i;
    for (i = subset->begin(); i != subset->end(); i++) {
      st_point_set *fullset = the_database->get_set_of_points();
      map<vector<int>, st_point *, lt_point>::iterator k;
      for (k = fullset->begin(); k != fullset->end();) {
        st_point *pi = i->second;
        st_point *pk = k->second;
        if (*pi == *pk) {
          map<vector<int>, st_point *, lt_point>::iterator next = k;
          next++;
          fullset->erase_point(k);
          k = next;
        } else {
          k++;
        }
      }
    }

    /** Copy pareto_db into entire_pareto_db */
    prs_display_message("Copying pareto front to n-level pareto front");
    for (i = subset->begin(); i != subset->end(); i++) {
      st_point *pi = i->second;
      entire_pareto_db->insert_point(pi);
    }
    delete pareto_db;
    n++;
  }
  the_database->clear();
  *the_database = *entire_pareto_db;
  delete entire_pareto_db;
}

void sim_compute_pareto(st_env *env, st_database *the_database, bool valid) {
  st_database *new_database = new st_database();
  st_point_set *database_list = the_database->get_set_of_points();
  int sz = database_list->get_size();
  int n = 0;
  opt_print_percentage("Computing pareto:", n, sz);
  st_point_set::iterator c;
  for (c = database_list->begin(); c != database_list->end();) {
    st_point &p = *(c->second);
    st_assert(p.check_consistency(env));
    if (!p.get_error()) {
      int rank_p;
      double penalty_p;
      bool consider = true;
      bool feasible;
      bool error = false;
      try {
        feasible = opt_check_constraints(p, env, rank_p, penalty_p);
      } catch (exception &e) {
        error = true;
      }

      if (valid && !feasible) {
        if (pareto_verbose) {
          string ps = p.print();
          cout << "Information: " << ps << " is not feasible " << endl;
        }
        consider = false;
      }

      if (error)
        consider = false;

      if (consider) {
        if (sim_recompute_pareto_with_new_point(env, new_database, p)) {
          /* Point p is a pareto point, we insert it by making a copy of it */
          new_database->insert_point(&p);
        }
      }
    }
    c++;
    n++;
    opt_print_percentage("Computing Pareto", n, sz);
  }
  the_database->clear();
  *the_database = *new_database;
  delete new_database;
}

double sim_compute_point_distance(st_env *env, st_point *a, st_point *x_p) {
  double distance = 0;
  for (int i = 0; i < env->optimization_objectives.size(); i++) {
    double xp_m_value = env->optimization_objectives[i]->eval(x_p, i);
    double a_m_value = env->optimization_objectives[i]->eval(a, i);

    double r_d = max(0.0, (a_m_value - xp_m_value) / (xp_m_value));

    distance = max(distance, r_d);
  }
  return distance;
}

double sim_compute_ADRS(st_env *env, st_database *A, st_database *X) {
  st_list *pareto_metrics;

  unsigned int X_norm = 0;
  st_point_set *X_list = X->get_set_of_points();
  st_point_set *A_list = A->get_set_of_points();

  A->cache_update(env);
  X->cache_update(env);

  st_point_set::iterator iA;
  st_point_set::iterator iX;

  double total_sum = 0;

  for (iX = X_list->begin(); iX != X_list->end(); iX++) {
    double candidate_value = INFINITY;
    st_point xp = *(iX->second);
    st_assert(xp.check_consistency(env));
    if (!xp.get_error()) {
      for (iA = A_list->begin(); iA != A_list->end(); iA++) {
        st_point a = *(iA->second);
        st_assert(a.check_consistency(env));
        if (!a.get_error()) {
          double d_xp_a = sim_compute_point_distance(env, &a, &xp);
          candidate_value = min(candidate_value, d_xp_a);
        }
      }
      total_sum += candidate_value;
      X_norm++;
    }
  }
  if (X_norm == 0)
    throw std::logic_error("Source database is empty");
  return total_sum / X_norm;
}

double sim_compute_euclidean_point_distance(st_env *env, st_point *a,
                                            st_point *x_p) {
  double distance = 0;
  for (int i = 0; i < env->optimization_objectives.size(); i++) {
    double xp_m_value = env->optimization_objectives[i]->eval(x_p, i);
    double a_m_value = env->optimization_objectives[i]->eval(a, i);

    double r_d = pow_double_to_int((a_m_value - xp_m_value) / xp_m_value, 2);

    distance += r_d;
  }
  return sqrt(distance);
}

double sim_compute_avg_euclidean_distance(st_env *env, st_database *A,
                                          st_database *X) {
  st_list *pareto_metrics;

  unsigned int X_norm = 0;
  st_point_set *X_list = X->get_set_of_points();
  st_point_set *A_list = A->get_set_of_points();

  A->cache_update(env);
  X->cache_update(env);

  st_point_set::iterator iA;
  st_point_set::iterator iX;

  double total_sum = 0;

  for (iX = X_list->begin(); iX != X_list->end(); iX++) {
    st_point xp = *(iX->second);
    st_assert(xp.check_consistency(env));
    if (!xp.get_error()) {
      double candidate_value = INFINITY;
      for (iA = A_list->begin(); iA != A_list->end(); iA++) {
        st_point a = *(iA->second);
        st_assert(a.check_consistency(env));
        if (!a.get_error()) {
          double d_xp_a = sim_compute_euclidean_point_distance(env, &a, &xp);
          candidate_value = min(candidate_value, d_xp_a);
        }
      }
      total_sum += candidate_value;
      X_norm++;
    }
  }
  if (X_norm == 0)
    throw std::logic_error("Reference database is empty");
  return total_sum / X_norm;
}

/**
 * For useful usage of this function, X should be the approximated pareto set,
 * while A should be the real Pareto set.
 */

double sim_compute_median_euclidean_distance(st_env *env, st_database *A,
                                             st_database *X) {
  vector<double> observations;

  unsigned int X_norm = 0;
  st_point_set *X_list = X->get_set_of_points();
  st_point_set *A_list = A->get_set_of_points();

  A->cache_update(env);
  X->cache_update(env);

  st_point_set::iterator iA;
  st_point_set::iterator iX;

  double total_sum = 0;

  for (iX = X_list->begin(); iX != X_list->end(); iX++) {
    st_point xp = *(iX->second);
    st_assert(xp.check_consistency(env));
    if (!xp.get_error()) {
      double candidate_value = INFINITY;
      for (iA = A_list->begin(); iA != A_list->end(); iA++) {
        st_point a = *(iA->second);
        st_assert(a.check_consistency(env));
        if (!a.get_error()) {
          double d_xp_a = sim_compute_euclidean_point_distance(env, &a, &xp);
          candidate_value = min(candidate_value, d_xp_a);
        }
      }
      observations.push_back(candidate_value);
    }
  }
  sort(observations.begin(), observations.end());
  int sz = observations.size();
  if (sz == 0)
    throw std::logic_error("Reference database is empty");
  int mid = (int)floor(((double)sz) / 2);
  if (!(sz % 2))
    return (observations[mid - 1] + observations[mid]) / 2.0;
  else
    return observations[mid];
}

int sim_compute_how_many_points_in_A_are_in_B(st_env *env, st_database *a,
                                              st_database *b) {
  st_point_set *la = a->get_set_of_points();
  st_point_set::iterator ia;
  int n = 0;
  for (ia = la->begin(); ia != la->end(); ia++) {
    st_point *p = b->look_for_point(ia->second);
    if (p)
      n++;
  }
  return n;
}

void sim_normalize_databases(st_env *env, st_database *b, st_database *a,
                             bool valid) {
  st_point_set *la = a->get_set_of_points();
  st_point_set *lb = b->get_set_of_points();

  st_point_set::iterator ib;
  for (ib = lb->begin(); ib != lb->end();) {
    st_point *pb = ib->second;
    st_point *pa = a->look_for_point(pb);
    if (!pa || !pb->check_consistency(env) || (valid && pb->get_error())) {
      st_point_set::iterator in = ib;
      in++;
      lb->erase_point(ib);
      ib = in;
    } else
      ib++;
  }
  for (ib = la->begin(); ib != la->end();) {
    st_point *pb = ib->second;
    st_point *pa = b->look_for_point(pb);
    if (!pa || !pb->check_consistency(env) || (valid && pb->get_error())) {
      st_point_set::iterator in = ib;
      in++;
      la->erase_point(ib);
      ib = in;
    } else
      ib++;
  }
}

extern void st_print_point(st_env *env, st_point *p, bool nometrics, bool clus,
                           bool rp);
extern void st_print_header(st_env *env, bool nometrics, bool clus, bool rp);

/** Computes C(a,b) that is 'b is covered by a' percentage */
double sim_compute_two_set_coverage_metric(st_env *env, st_database *b,
                                           st_database *a, bool verbose) {
  st_point_set *la = a->get_set_of_points();
  st_point_set *lb = b->get_set_of_points();

  st_point_set::iterator ia;
  st_point_set::iterator ib;

  a->cache_update(env);
  b->cache_update(env);

  int sb = 0;
  int db = 0;
  if (verbose) {
    st_print_header(env, true, false, false);
  }
  for (ib = lb->begin(); ib != lb->end(); ib++) {
    st_point pb = *(ib->second);
    st_assert(pb.check_consistency(env));
    if (!pb.get_error()) {
      for (ia = la->begin(); ia != la->end(); ia++) {
        st_point pa = *(ia->second);
        st_assert(pa.check_consistency(env));
        if (!pa.get_error()) {
          bool feasible_b;
          bool feasible_a;
          int rank_b;
          int rank_a;
          double penalty_b;
          double penalty_a;
          if (sim_is_strictly_dominated(env, pb, pa, feasible_b, feasible_a,
                                        rank_b, rank_a, penalty_b, penalty_a)) {
            if (verbose) {
              cout << "dominating point (following by dominated)" << endl;
              st_print_point(env, &pa, true, false, false);
              st_print_point(env, &pb, true, false, false);
            }
            db++;
            break;
          }
        }
      }
      sb++;
    }
  }
  if (!sb)
    throw std::logic_error("Source database is empty");
  return ((double)db) / ((double)sb);
}

#define cut(sa, sz) ((sa.size() > sz) ? (sa.substr(0, sz - 2) + "..") : sa)
#define CSIZE 15

extern void st_printf_tabbed(string a, int maxsize);
extern void st_printf_tabbed_rev(string a, int maxsize);

void sim_print_centroid_head(st_env *env, vector<double> &v) {
  cout << "{";
  for (int i = 0; i < v.size(); i++) {
    cout << (i ? ", " : " ");
    st_printf_tabbed(cut(env->optimization_objectives[i]->name, CSIZE), CSIZE);
    cout << " ";
  }
  cout << "}";
}

#include <st_common_utils.h>

void sim_print_centroid(st_env *env, vector<double> &v) {
  cout << "{";
  for (int i = 0; i < v.size(); i++) {
    cout << (i ? ", " : " ");
    st_printf_tabbed_rev(cut(st_to_string(v[i]), CSIZE), CSIZE);
    cout << " ";
  }
  cout << "}";
}

#define normalize(i, v)                                                        \
  ((v - minimum_value[i]) / (maximum_value[i] - minimum_value[i]))

double sim_compute_kmeans_clusters(st_env *env, int klusters, int iterations,
                                   st_database *the_database) {
  st_list *pareto_metrics;

  if (env->optimization_objectives.size() < 1) {
    prs_display_error(
        "Please, set a consistent set of objectives to be clustered.");
    return 0.0;
  }

  if (klusters == 0 || iterations == 0)
    return 0.0;

  double current_cluster_goodness = 0.0;

  vector<vector<double> > klusters_centroids;

  vector<vector<double> > cumulative_value;
  vector<double> cumulative_square_distance;
  vector<int> cumulative_count;

  klusters_centroids.resize(klusters);

  cumulative_value.resize(klusters);
  cumulative_count.resize(klusters, 0);

  vector<double> minimum_value;
  vector<double> maximum_value;

  minimum_value.resize(env->optimization_objectives.size(), INFINITY);
  maximum_value.resize(env->optimization_objectives.size(), 0.0);

  the_database->cache_update(env);
  st_point_set *set = the_database->get_set_of_points();
  st_point_set::iterator p;
  for (p = set->begin(); p != set->end(); p++) {
    st_point *x = p->second;
    st_assert(x->check_consistency(env));
    if (!x->get_error()) {
      int k = 0;
      for (int i = 0; i < env->optimization_objectives.size(); i++) {
        double mxv = env->optimization_objectives[i]->eval(x, i);

        if (mxv > maximum_value[k])
          maximum_value[k] = mxv;

        if (mxv < minimum_value[k])
          minimum_value[k] = mxv;

        k++;
      }
    }
  }

  klusters_centroids.resize(klusters);
  cumulative_square_distance.resize(klusters, 0.0);

  for (int kl = 0; kl < klusters; kl++) {
    klusters_centroids[kl].resize(env->optimization_objectives.size());
    cumulative_value[kl].resize(env->optimization_objectives.size(), 0.0);

    if (kl == 0) {
      cout << "Metrics    ";
      sim_print_centroid_head(env, klusters_centroids[0]);
      cout << endl;
    }

    for (int i = 0; i < env->optimization_objectives.size(); i++) {
      // klusters_centroids[kl][i]= minimum_value[i] + rnd_flat_float() *
      // (maximum_value[i] - minimum_value[i]);
      klusters_centroids[kl][i] = rnd_flat_float();
    }
    cout << "Centroid " << kl << " ";
    sim_print_centroid(env, klusters_centroids[kl]);
    cout << endl;
  }

  vector<double> best_distance_per_cluster;
  vector<st_point *> current_nearest_centroid_per_cluster;

  best_distance_per_cluster.resize(klusters, INFINITY);
  current_nearest_centroid_per_cluster.resize(klusters, NULL);

  st_database *full = new st_database();

  for (int it = 0; it < iterations; it++) {
    cout << "--" << endl;
    st_point_set *set = the_database->get_set_of_points();
    st_point_set::iterator p;
    for (p = set->begin(); p != set->end(); p++) {
      st_point *x = p->second;
      if (!x->get_error()) {
        double current_best_distance = INFINITY;
        int current_best_cluster = 0;
        for (int kl = 0; kl < klusters; kl++) {
          double distance = 0;
          int s = 0;
          for (int i = 0; i < env->optimization_objectives.size(); i++) {
            double mxv =
                normalize(i, env->optimization_objectives[i]->eval(x, i));
            double r_d = pow_double_to_int(mxv - klusters_centroids[kl][s], 2);
            distance += r_d;
            s++;
          }

          if (distance < current_best_distance) {
            current_best_distance = distance;
            current_best_cluster = kl;
          }
        }
        if (current_best_distance <
            best_distance_per_cluster[current_best_cluster]) {
          if (current_nearest_centroid_per_cluster[current_best_cluster])
            delete current_nearest_centroid_per_cluster[current_best_cluster];

          current_nearest_centroid_per_cluster[current_best_cluster] =
              x->gen_copy_point();
        }

        x->set_cluster(current_best_cluster);
      }
    }
    /** Initialize counters */
    for (int kl = 0; kl < klusters; kl++) {
      cumulative_count[kl] = 0;
      cumulative_square_distance[kl] = 0;
      int s;
      for (s = 0; s < env->optimization_objectives.size(); s++) {
        cumulative_value[kl][s] = 0;
      }
    }
    /** Compute cumulative counts */
    for (p = set->begin(); p != set->end(); p++) {
      st_point *x = p->second;
      if (!x->get_error()) {
        int cluster_associated = x->get_cluster();
        cumulative_count[cluster_associated]++;
        int s = 0;
        for (int c = 0; c < env->optimization_objectives.size(); c++) {
          double mxv =
              normalize(c, env->optimization_objectives[c]->eval(x, c));

          cumulative_value[cluster_associated][s] += mxv;

          cumulative_square_distance[cluster_associated] = pow_double_to_int(
              mxv - klusters_centroids[cluster_associated][s], 2);
          s++;
        }
      }
    }

    current_cluster_goodness = 0;
    /** Compute new centroids */
    for (int kl = 0; kl < klusters; kl++) {
      current_cluster_goodness += cumulative_square_distance[kl];
    }

    /** Compute new centroids */
    for (int kl = 0; kl < klusters; kl++) {
      if (cumulative_count[kl]) {
        for (int i = 0; i < env->optimization_objectives.size(); i++) {
          klusters_centroids[kl][i] =
              cumulative_value[kl][i] / ((double)cumulative_count[kl]);
        }
      }
      cout << "Centroid " << kl << " ";
      sim_print_centroid(env, klusters_centroids[kl]);
      cout << " CN: " << current_cluster_goodness;
      cout << endl;
    }
  }
  /** Compute new centroids */
  for (int kl = 0; kl < klusters; kl++) {
    cout << "Centroid " << kl << " ";
    sim_print_centroid(env, klusters_centroids[kl]);
    cout << endl;
    if (current_nearest_centroid_per_cluster[kl]) {
      full->insert_point(current_nearest_centroid_per_cluster[kl]);
      delete current_nearest_centroid_per_cluster[kl];
    }
  }
  env->insert_new_database(full, "kmean_clusters");
  return current_cluster_goodness;
}
