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
#ifndef ST_MPI_UTILS
#define ST_MPI_UTILS

#include <iostream>

void st_mpi_initialize();
void st_mpi_end();
int st_mpi_get_node();
string st_mpi_get_string_representation_of_node();
int st_mpi_get_number_of_nodes();
void st_mpi_send_integer(int data, int node);
void st_mpi_send_data(string data, int node);
int st_mpi_receive_integer(int node);
string st_mpi_receive_data(int node);
void st_mpi_send_command(string command, int node);
string st_mpi_receive_command(int node);
void st_mpi_broadcast_send_data(string name);
void st_mpi_broadcast_send_command(string command);
bool st_mpi_environment_active();
int st_mpi_get_number_of_required_nodes();

#endif
