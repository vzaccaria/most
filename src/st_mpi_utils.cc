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
#include <mpi.h>
#include <cstring>
#include <st_env.h>
#include <string>

using namespace std;

static int tag = 201; /** Stolen from MPI examples :-) */
#define MAXCOMMANDSIZE 50

int st_mpi_get_size() {
  int s;
  MPI_Comm_size(MPI_COMM_WORLD, &s);
  return s;
}

void st_mpi_initialize() { MPI_Init(NULL, NULL); }

void st_mpi_end() { MPI_Finalize(); }

int st_mpi_get_node() {
  int r;
  MPI_Comm_rank(MPI_COMM_WORLD, &r);
  return r;
}

string st_mpi_get_string_representation_of_node() {
  string s;
  char tmp[20];
  sprintf(tmp, "%d", st_mpi_get_node());
  s = tmp;
  return s;
}

bool st_mpi_environment_active() { return (st_mpi_get_size()) > 1; }

int st_mpi_get_number_of_required_nodes() {
  int physical_nodes = st_mpi_get_size();
  return physical_nodes;
}

/* The following is the number of acquired nodes taken from the node manager */
extern int acquired_nodes;

/* This is used when we have to shutdown nodes, regardless of whether they are
 * licensed or not */
int st_mpi_get_number_of_physical_nodes() {
  int physical_nodes = st_mpi_get_size();
  return physical_nodes;
}

extern bool bypass_license_node_manager;

int st_mpi_get_number_of_nodes() {

  int physical_nodes = st_mpi_get_size();

  if (bypass_license_node_manager)
    return physical_nodes;

  if (physical_nodes > acquired_nodes)
    return acquired_nodes;
  else
    return physical_nodes;
}

void st_mpi_send_integer(int data, int node) {
  MPI_Send(&data, 1, MPI_INT, node, tag, MPI_COMM_WORLD);
  // cout << "Node " << st_mpi_get_node() << " sending " << data << " to " <<
  // node << endl;
}

void st_mpi_send_data(string data, int node) {
  st_mpi_send_integer(strlen(data.c_str()) + 1, node);
  MPI_Send(data.c_str(), strlen(data.c_str()) + 1, MPI_CHAR, node, tag,
           MPI_COMM_WORLD);
  // cout << "Node " << st_mpi_get_node() << " sending " << data << " to " <<
  // node << endl;
}

int st_mpi_receive_integer(int node) {
  int data;
  // cout << "Node " << st_mpi_get_node() << " trying to receive from " << node
  // << endl;
  MPI_Recv(&data, 1, MPI_INT, node, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  // cout << "Node " << st_mpi_get_node() << " receiving " << data << " from "
  // << node << endl;
  return data;
}

string st_mpi_receive_data(int node) {
  // cout << "Node " << st_mpi_get_node() << " trying to receive from " << node
  // << endl;
  int len = st_mpi_receive_integer(node);
  char *buf = (char *)malloc(len);
  if (!buf)
    return "BAD ALLOC ERROR";
  MPI_Recv(buf, len, MPI_CHAR, node, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  // cout << "Node " << st_mpi_get_node() << " receiving " << buf << " from " <<
  // node << endl;
  return buf;
}

void st_mpi_send_command(string command, int node) {
  st_mpi_send_data(command, node);
}

string st_mpi_receive_command(int node) { return st_mpi_receive_data(node); }

void st_mpi_broadcast_send_data(string name) {
  int i;
  // cout << "Node " << st_mpi_get_node() << " broadcasting " << name << endl;
  for (i = 1; i < st_mpi_get_number_of_physical_nodes(); i++) {
    st_mpi_send_data(name, i);
  }
}

void st_mpi_broadcast_send_command(string command) {
  st_mpi_broadcast_send_data(command);
}
