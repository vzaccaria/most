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

#ifndef COMMANDS_H
#define COMMANDS_H

using namespace std;

#include "st_list.h"
#include "st_map.h"
#include "st_object.h"

void prs_initialize_readline();
void prs_command_quit();
void prs_command_echo(st_object *, string, string);
void prs_command_set(st_object *, st_object *);
// void prs_command_set_property(st_object *,st_object *,st_object *);
st_object *prs_command_read_variable(st_object *, bool);
void prs_command_db_insert_point(st_object *);
void prs_command_db_write(st_object *);
void prs_command_read(st_object *);
void prs_command_db_read(st_object *);
int prs_command_drv_load_driver(string n);
bool prs_command_drv_instantiate_driver();
bool prs_command_activate_driver(string name);
void prs_command_show_variables();
void prs_drv_show_info();
void prs_opt_show_info();
void prs_command_define_optimizer(st_object *n);
void prs_command_define_application(st_object *n);
void prs_command_filter_pareto(st_object *, st_object *);
void st_init_parse_command();
void st_parse_command(st_string *command_name, st_map *parameter_list);
string st_parse_command_get_opt_string(st_map *parameter_list, string n);
string st_parse_command_get_arg_string(st_map *parameter_list);
int st_parse_command_get_arg_integer(st_map *parameter_list);
int st_parse_command_get_opt_integer(st_map *parameter_list, string n);
st_object *st_parse_command_get_arg_object(st_map *parameter_list);
st_object *st_parse_command_get_opt_object(st_map *parameter_list, string n);
double st_parse_command_get_opt_double(st_map *parameter_list, string n,
                                       double def);

void st_add_possible_command(string c);
void st_remove_possible_command(string c);

#endif
