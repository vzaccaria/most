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
#ifndef ST_PARSER
#define ST_PARSER

#include "st_ast.h"
#include "st_object.h"
#include <string>

extern int yyparse();
extern int including;

bool prs_parse_and_execute_file(const char *file_name);
bool prs_go_interactive();
void prs_display_error(const string &);
void prs_display_error_plain(const string &);
void prs_display_time();
void prs_display_message(const string &s);
bool prs_current_parser_session_is_interactive();
st_ast **prs_current_parser_session_return_ast();

st_object *prs_add_coord_to_point(st_object *, st_object *);
st_object *prs_list_concat(st_object *, st_object *);
st_object *prs_insert_in_list(st_object *, st_object *);
st_object *prs_insert_in_vector(st_object *, st_object *);
st_object *prs_add_element_to_map(st_object *, st_object *, st_object *);
st_object *prs_insert_map_as_property(st_object *, st_object *);
st_object *prs_read_point_from_string(string input);
extern string prs_get_time();

extern bool mpi_verbose;
extern bool display_time;

#define prs_display_message_n_action_m(S1, A, S2)                              \
  {                                                                            \
    if (st_mpi_get_node() == 0 || mpi_verbose) {                               \
      string verbose;                                                          \
      bool v =                                                                 \
          current_environment.shell_variables.get_string("verbose", verbose);  \
      if (!v || verbose == "true") {                                           \
        if (display_time)                                                      \
          cout << prs_get_time() << " ";                                       \
        cout << "Information: " << S1;                                         \
        A;                                                                     \
        cout << S2 << endl;                                                    \
      }                                                                        \
    }                                                                          \
  }

#define prs_display_message_n_value_m(S1, V, S2)                               \
  {                                                                            \
    if (st_mpi_get_node() == 0 || mpi_verbose) {                               \
      string verbose;                                                          \
      bool v =                                                                 \
          current_environment.shell_variables.get_string("verbose", verbose);  \
      if (!v || verbose == "true") {                                           \
        if (display_time)                                                      \
          cout << prs_get_time() << " ";                                       \
        cout << "Information: " << S1 << V << S2 << endl;                      \
      }                                                                        \
    }                                                                          \
  }

void st_codi_reset_time();
void st_codi_display_iep(float impr_percentage, float eta, string phase);
void st_codi_display_ip(float impr_percentage, string phase);
void st_codi_display_p(string phase);
void st_codi_display_ep(float eta, string phase);

#endif
