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

#include <st_map.h>
#include <st_opt_utils.h>
#include <st_point.h>
#include <st_sim_utils.h>
#include <st_design_space.h>

st_point::st_point(){obj_create(num_obj)};

st_point::st_point(const st_point &l)
    : st_object(l), vector<int>(l), o_cache(l.o_cache), c_cache(l.c_cache),
      m_cache(l.m_cache){obj_create(num_obj)}

      st_point::st_point(int n)
    : vector<int>(n){obj_create(num_obj)}

      st_point::st_point(const vector<int> &p)
    : vector<int>(p){obj_create(num_obj)}

      st_point::~st_point() {}

string st_point::print() const {
  string s = "% ";
  int i;
  for (i = 0; i < size(); i++) {
    char buf[10];
    sprintf(buf, "%d", (*this)[i]);
    s = s + buf + " ";
  }
  s = s + "%";
  return s;
}

string st_point::full_print() const {
  string s = "% ";
  int i;
  for (i = 0; i < size(); i++) {
    char buf[10];
    sprintf(buf, "%d", (*this)[i]);
    s = s + buf + " ";
  }
  s = s + "%";
  s = s + depth_print(properties);
  return s;
}

string st_point::print_canonical() const {
  string s = "_obj_repr_";
  s = s + print() + "";
  st_map *m = new st_map();
  map<string, st_object *>::const_iterator i;
  for (i = properties.begin(); i != properties.end(); i++) {
    m->insert(i->first, *(i->second));
  }
  s = s + m->print() + "";
  delete m;
  return s;
}

string st_point::print_octave(st_env *env) {
  string s = "";
  {
    int i;
    for (i = 0; i < size(); i++) {
      char buf[10];
      sprintf(buf, "%d", (*this)[i]);
      s = s + buf + " ";
    }
  }
  s = s + " ";

  s = s + " ";

  vector<st_objective *>::iterator k;
  for (k = env->optimization_objectives.begin();
       k != env->optimization_objectives.end(); k++) {
    s = s + " ";
    int index = env->get_objective_index((*k)->name);

    char m_rep[32];
    sprintf(m_rep, "%f",
            env->optimization_objectives[index]->eval(this, index));

    s = s + string(m_rep);
  }

  s = s + " ";
  return s;
}

string st_point::print_m3_canonical_objectives(st_env *env) {
  string s = "_obj_repr_< ";
  {
    int i;
    for (i = 0; i < size(); i++) {
      char buf[10];
      sprintf(buf, "%d", (*this)[i]);
      s = s + buf + " ";
    }
  }
  s = s + ">";

  s = s + "#( metrics=[";

  vector<st_objective *>::iterator k;
  for (k = env->optimization_objectives.begin();
       k != env->optimization_objectives.end(); k++) {
    s = s + " ";
    int index = env->get_objective_index((*k)->name);

    char m_rep[32];
    sprintf(m_rep, "%f",
            env->optimization_objectives[index]->eval(this, index));

    s = s + string(m_rep);
  }

  s = s + " ] )# ";
  return s;
}

string st_point::print_m3_canonical() const {
  string s = "_obj_repr_< ";
  {
    int i;
    for (i = 0; i < size(); i++) {
      char buf[10];
      sprintf(buf, "%d", (*this)[i]);
      s = s + buf + " ";
    }
  }
  s = s + ">";

  st_object const *point_metrics;

  if (!const_cast<st_point *>(this)->get_properties("metrics", point_metrics))
    return s;

  if (!point_metrics)
    return s;

  st_vector *the_vector =
      to<st_vector *>(const_cast<st_object *>(point_metrics));

  if (!the_vector)
    return s;

  vector<st_object *>::const_iterator i;
  s = s + "#( metrics=[";
  for (i = the_vector->the_vector.begin(); i != the_vector->the_vector.end();
       i++) {
    s = s + " ";

    if (*i)
      s = s + (*i)->print();
    else
      s = s + " / ";
  }
  s = s + " ] )# ";
  return s;
}

void st_point::insert(int pos, int i) {
  if (pos > size())
    resize(pos + 1, 0);
  (*this)[pos] = i;
}

st_object *st_point::gen_copy() const {
  st_point *v = new st_point(*this);
  return v;
}

st_point *st_point::gen_copy_point() const {
  st_point *v = new st_point(*this);
  return v;
}

st_point &st_point::operator=(const st_point &o) {
  *((vector<int> *)this) = (o);
  *((st_object *)this) = (o);
  o_cache = o.o_cache;
  c_cache = o.c_cache;
  return (*this);
}

bool st_point::operator==(const st_point &o) {
  if (size() != o.size())
    return false;
  for (int i = 0; i < size(); i++) {
    if ((*this)[i] != o[i])
      return false;
  }
  return true;
}

void st_point::set_sims(int num) {
  st_integer err = num;
  set_properties("sims", err);
}

int st_point::get_sims() {
  const st_object *err;
  if (!get_properties("sims", err))
    return 0;
  st_integer *errint = to<st_integer *>(const_cast<st_object *>(err));
  if (!errint)
    return 0;
  return errint->get_integer();
}

void st_point::set_error(int errorcode) {
  st_integer err = errorcode;
  set_properties("error", err);
}

void st_point::set_error(int type, string reason) {
  st_integer type3(type);
  st_string reason3(reason);

  set_properties("error", type3);
  /* Warning, the following is not saved in the db
   * neither communicated over MPI */
  set_properties("error_reason", reason3);
}

void st_point::set_rpath(string rpath) {
  st_string rp(rpath);
  set_properties("rpath", rp);
}

string st_point::get_rpath() {
  const st_object *err;
  if (!get_properties("rpath", err))
    return "";
  st_string *errint = to<st_string *>(const_cast<st_object *>(err));
  if (!errint)
    return "";
  return errint->get_string();
}

int st_point::get_error() {
  const st_object *err;
  if (!get_properties("error", err))
    return 0;
  st_integer *errint = to<st_integer *>(const_cast<st_object *>(err));
  if (!errint)
    return 0;
  return errint->get_integer();
}

string st_point::get_error_description() {
  const st_object *err;
  if (!get_properties("error_reason", err))
    return "";
  st_string *errint = to<st_string *>(const_cast<st_object *>(err));
  if (!errint)
    return "";
  return errint->get_string();
}

bool st_point::fatal() { return (get_error() == ST_POINT_FATAL_ERROR); }

void st_point::set_cluster(int cluster) {
  st_integer clus = cluster;
  set_properties("cluster", clus);
}

int st_point::get_cluster() {
  const st_object *clus;
  if (!get_properties("cluster", clus))
    return 0;
  st_integer *clusi = to<st_integer *>(const_cast<st_object *>(clus));
  if (!clusi)
    return 0;
  return clusi->get_integer();
}

/**
 * The following checks for the consistency of the metrics property of the point
 * with respect to the metrics currenlty defined in the system
 */
bool st_point::check_consistency(st_env *env) const {
  if (const_cast<st_point *>(this)->get_error())
    return true;

  st_object const *point_metrics;

  /** Really bad hack here :-) */
  const_cast<st_point *>(this)->get_properties("metrics", point_metrics);

  if (!point_metrics || !is_a<st_vector const *>(point_metrics))
    return false;

  st_vector const *v = to<st_vector const *>(point_metrics);

  if (env->current_design_space->metric_names.size() != v->size())
    return false;

  for (int i = 0; i < v->size(); i++) {
    if (!is_a<st_double const *>(&v->get(i)))
      return false;
  }

  /** Statistics have never been used. They will be unsupported in this version
   *

  sv_metrics = sim_get_stats_vector(env);

  const_cast<st_point *>(this)->get_properties("statistics",point_metrics);

  if(!point_metrics || !is_a<st_vector const *>(point_metrics))
      return false;


  v = to<st_vector const *>(point_metrics);

  if(sv_metrics->size() != v->size())
  {
      //cout << sv_metrics->size() << ";" << v->size() << endl;
      return false;
  }

  for(int i = 0; i<v->size(); i++)
  {
      if(!is_a<st_double const *>(&v->get(i)))
          return false;
  }
  */

  if (env->current_design_space->ds_parameters.size() != size())
    return false;

  // cout << design_space->size() << ";" << size() << endl;
  return true;
}

double st_point::get_metrics(int pos) const {
  st_object const *point_metrics;
  const_cast<st_point *>(this)->get_properties("metrics", point_metrics);
  st_assert(point_metrics);
  return to<st_double const *>(&to<st_vector const *>(point_metrics)->get(pos))
      ->get_double();
}

double st_point::get_metrics(string name, st_env *env) const {
  st_assert(env->current_design_space->metric_index.count(name));
  int pos = env->current_design_space->metric_index[name];
  st_object const *point_metrics;
  const_cast<st_point *>(this)->get_properties("metrics", point_metrics);
  st_assert(point_metrics);
  return to<st_double const *>(&to<st_vector const *>(point_metrics)->get(pos))
      ->get_double();
}

double st_point::get_statistics(int pos) const {
  st_object const *point_metrics;
  const_cast<st_point *>(this)->get_properties("statistics", point_metrics);
  st_assert(point_metrics);
  return to<st_double const *>(&to<st_vector const *>(point_metrics)->get(pos))
      ->get_double();
}
