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

#include <errno.h>
#include <fstream>
#include <iostream>
#include <libgen.h>
#include <math.h>
#include <pthread.h>
#include <regex.h>
#include <signal.h>
#include <st_lm.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/unistd.h>
#include <time.h>
#include <unistd.h>
#include <vector>

extern void st_rm_init_images(st_node_information *ni);

string st_get_time() {
  char buffer[30];
  struct timeval tv;

  time_t curtime;

  gettimeofday(&tv, NULL);
  curtime = tv.tv_sec;
  strftime(buffer, 30, "%d/%m/%Y %T", localtime(&curtime));
  return string(buffer);
}

ofstream fout;

using namespace std;

/* The following is the MAC address of this host */

string licensed_hw_addr;

#define PRE_MSG st_get_time() << " Message: "
#define PRE_ERR st_get_time() << " Error  : "

void mostd_display_message(string s) {
  fout << PRE_MSG << s << endl;
  flush(fout);
}

void mostd_display_error(string s) {
  fout << PRE_ERR << " Error: " << s << endl;
  flush(fout);
}

string local_build_path;

int st_str_to_int(string s) {
  int o;
  sscanf(s.c_str(), "%d", &o);
  return o;
}

bool st_str_to_date(string s, tm *date) {
  int o;
  if (!strptime(s.c_str(), "%Y-%m-%d", date))
    return false;
  return true;
}

long int st_str_hex_to_lint(string s) {
  long int o;
  sscanf(s.c_str(), "%lx", &o);
  return o;
}

bool st_print_licensed_data(st_licensed_data *ld) {
  fout << PRE_MSG << "** MOST proprietary license -- data **" << endl;
  fout << PRE_MSG << "Licensed to    : " << ld->licensed_to << endl;
  fout << PRE_MSG << "Nodes          : " << ld->nodes << endl;
  fout << PRE_MSG << "Hostid         : " << licensed_hw_addr << endl;

#define PRINT_DATE(dt)                                                         \
  ld->dt.tm_year + 1900 << "-" << ld->dt.tm_mon + 1 << "-" << ld->dt.tm_mday

  fout << PRE_MSG << "Expiration date: " << PRINT_DATE(to_date) << endl;
  flush(fout);
}

bool st_parse_license(string license, st_licensed_data *ld) {
  mostd_display_message("Parsing license...");

  string spaces = "[ \t]*";
  string sentence = "[\x21-\x7E][\x20-\x7E]*";
  string IT = "[0-9][0-9]*";
  string HX = "[0-9a-f][0-9a-f\x3a]*";
  string HXA = "[0-9a-f][0-9a-f\x3a]*";
  string DT = IT + "-" + IT + "-" + IT;

  string FDT = "Validity from:" + spaces + DT;
  string TDT = "Validity to:" + spaces + DT;
  // string ND  = "Nodes:[ ]*"+IT;
  string ND = "Nodes:" + spaces + IT;
  string LTp = "Licensed to:";
  string LT = LTp + spaces + sentence;
  string HI = "Host-ID:" + spaces + HXA;

  regex_t ITr, HXr, DTr, FDTr, TDTr, NDr, HIr, LTr, SNr;
  if (regcomp(&ITr, IT.c_str(), 0) || regcomp(&HXr, HX.c_str(), 0) ||
      regcomp(&DTr, DT.c_str(), 0) || regcomp(&FDTr, FDT.c_str(), 0) ||
      regcomp(&TDTr, TDT.c_str(), 0) || regcomp(&NDr, ND.c_str(), 0) ||
      regcomp(&LTr, LT.c_str(), 0) || regcomp(&SNr, sentence.c_str(), 0) ||
      regcomp(&HIr, HI.c_str(), 0)) {
    return false;
  }

  regmatch_t positions[1];

#define GET_PATT(line, patt, rs)                                               \
  if (!regexec(&patt, line.c_str(), 1, positions, 0)) {                        \
    int length = positions[0].rm_eo - positions[0].rm_so;                      \
    rs = line.substr(positions[0].rm_so, length);                              \
  } else {                                                                     \
    rs = "NA";                                                                 \
  }

  string ir, nodes, hostid, to_date, from_date, licensed_to;
  GET_PATT(license, NDr, ir);
  GET_PATT(ir, ITr, nodes);

  GET_PATT(license, HIr, ir);
  GET_PATT(ir, HXr, hostid);

  GET_PATT(license, TDTr, ir);
  GET_PATT(ir, DTr, to_date);

  GET_PATT(license, FDTr, ir);
  GET_PATT(ir, DTr, from_date);

  GET_PATT(license, LTr, ir);
  GET_PATT((ir.substr(LTp.size() + 1, ir.size() - LTp.size())), SNr,
           licensed_to);

  ld->nodes = st_str_to_int(nodes);
  fout << PRE_MSG << "Host address in license is '" << hostid << "'" << endl;

  licensed_hw_addr = hostid;

  ld->hostid = st_get_hw_addr_from_s(hostid);
  ld->licensed_to = licensed_to;

  fout << PRE_MSG << "Starting date " << from_date << endl;
  fout << PRE_MSG << "End date " << to_date << endl;
  flush(fout);

  if (!st_str_to_date(to_date, &ld->to_date))
    mostd_display_error("Cant convert " + to_date);

  if (!st_str_to_date(from_date, &ld->from_date))
    mostd_display_error("Cant convert " + from_date);

  st_print_licensed_data(ld);

  return true;
}

#if !defined(__MAC_OSX__)
/**
 * Reads the directory of the executable
 */
string mostd_get_build_path(char *base_stshell_name) {
  int retsz;
  char complete_path[300];
  retsz = readlink("/proc/self/exe", complete_path, 300);
  if (retsz <= 0) {
    fout << PRE_ERR << "Unable to access the /proc filesystem" << endl;
    flush(fout);
    return ".";
  }
  complete_path[retsz] = '\0';

  // fout << "Using complete path: " << complete_path << endl;

  dirname(complete_path);
  dirname(complete_path);
  string final_path = complete_path;
  return final_path;
}

#else

bool mostd_shell_command(string command) {
  string sh_com = "/bin/bash -c \"(" + command + ")\"";
  int rs = system(sh_com.c_str());
  if (rs == -1 || rs == 127) {
#if defined(DEBUG)
    fout << PRE_ERR << sh_com << " : failed - " << rs << endl;
    flush(fout);
#endif
    return false;
  }
  /** The upper 8 bits contain the exit status */
  rs = rs >> 8;
#if defined(DEBUG)
  fout << PRE_ERR << sh_com << " : " << ((rs) ? "FAIL" : "OK") << endl;
  flush(fout);
#endif
  if (rs)
    return false;
  return true;
}

/**
 * Under mac we have to play a little bit more.
 * Reads the directory of the executable.
 */
string mostd_get_build_path(char *base_stshell_name) {
  int retstatus;
  string command = "type -p ";
  FILE *file;

  char complete_path[100];
  char *internal_path;
  command = command + base_stshell_name;
  command = command + " > ._most_tmp_file";

  // fout << "Executing: " << command << endl;

  if (!mostd_command(command)) {
    fout << PRE_ERR << "Cannot state the complete path of most" << endl;
    flush(fout);
    st_graceful_exit(0);
  }

  file = fopen("._most_tmp_file", "r");
  fscanf(file, "%s", complete_path);
  fclose(file);

  internal_path = dirname(complete_path);
  internal_path = dirname(internal_path);

  string final_path = internal_path;
  return final_path;
}

#endif

bool st_load_license(string &clear_data, string &sign_data) {
  string path = local_build_path;
  string license_name = path + "/license/license.lic";

  FILE *file = fopen(license_name.c_str(), "r");
  if (!file) {
    mostd_display_error("Unable to load the license file");
    return false;
  }

  char line[1000];

  int n = 0;
  bool clear = true;
  while (fgets(line, sizeof(line) - 1, file)) {
    if (clear) {
      if (line[0] == '@') {
        clear = false;
        continue;
      } else {
        clear_data = clear_data + string(line);
      }
    } else {
      sign_data = sign_data + string(line);
    }
  }
  fclose(file);
}

bool st_check_date(st_licensed_data &ld) {
  time_t curtime;
  time_t ttime;
  time_t ftime;
  ttime = mktime(&ld.to_date);
  ftime = mktime(&ld.from_date);

  curtime = time(NULL);

  if (curtime > ttime || curtime < ftime) {
    mostd_display_message("Time elapsed?");
    fout << PRE_ERR << curtime << " " << ttime << " " << ftime << endl;
    flush(fout);
    return false;
  }
  return true;
}

bool st_check_license(st_licensed_data &ld) {
#if defined(__MOST_NO_LICENSE__)
  return true;
#endif
  string pl, si;
  st_initialize_ossl();
  st_load_license(pl, si);
  if (!st_verify_signature(pl, si)) {
    mostd_display_error("Signature mismatch - license file");
    return false;
  }

  if (!st_parse_license(pl, &ld)) {
    mostd_display_error("Corrupted license file");
    return false;
  }

  string addr;

  if (!st_get_hw_addr(addr)) {
    mostd_display_error("Unable to get hardware address");
    return false;
  }

  if (licensed_hw_addr != addr) {
    mostd_display_error("License not valid for this node (" + addr + ")");
    /*fout << ld.hostid << endl;
    fout << st_get_hw_addr(addr) << endl; */
    return false;
  }

  if (!st_check_date(ld)) {
    mostd_display_error("Unable to check the date");
    return false;
  }

  mostd_display_message("The license is valid");
  return true;
}

int shared_memory_id = 0;

st_node_information *current_ni = NULL;

void st_node_manager_erase_previous() {
  int shmid = PRB_MOST_SHM;

  if (shmid < 0 && errno == EEXIST) {
    shmid = GET_MOST_SHM;

    if (shmid >= 0)
      shmctl(shmid, IPC_RMID, NULL);
  }
}

bool st_node_manager_create(st_licensed_data *ld) {
  int shmid = CRT_MOST_SHM;

  if (shmid < 0)
    return false;

  current_ni = (st_node_information *)shmat(shmid, NULL, 0);

  if (current_ni == (st_node_information *)-1)
    return false;

  shared_memory_id = shmid;

  pthread_mutex_init(&current_ni->ni_lock, NULL);
  current_ni->licensed_nodes = (ld->nodes);
  current_ni->available_nodes = (ld->nodes);

  return true;
}

int st_acquired_nodes = 0;

bool st_random_check(st_licensed_data &ld) {
  /*
  if(current_ni->pending + (current_ni->available_nodes) !=
  (current_ni->licensed_nodes))
  {
      fout << PRE_ERR << "Pending " << current_ni->pending << ", available " <<
  (current_ni->available_nodes) << ", licensed " << (current_ni->licensed_nodes)
  << endl; return false;
  }*/

  if ((current_ni->licensed_nodes) != ld.nodes) {
    fout << PRE_ERR << "Available " << (current_ni->available_nodes)
         << ", licensed data " << ld.nodes << endl;
    return false;
  }
  if (!st_check_date(ld))
    return false;

  return true;
}

bool st_node_manager_shutdown() {
  if ((current_ni != NULL) && (current_ni != (st_node_information *)-1)) {
    shmdt(current_ni);
    shmctl(shared_memory_id, IPC_RMID, NULL);
  }
  current_ni = NULL;
  return true;
}

void mostd_abort_handler(int signum) {
  mostd_display_message("Signal received, trying to release the license");
  st_node_manager_shutdown();
  exit(1);
}

void mostd_setup_signal_handler() {
  signal(SIGINT, mostd_abort_handler);
  signal(SIGKILL, mostd_abort_handler);
  signal(SIGSTOP, mostd_abort_handler);
  signal(SIGQUIT, mostd_abort_handler);
  signal(SIGTERM, mostd_abort_handler);
}

#include <unistd.h>

/* Here we say the last word on compatibility with BSD */

bool st_check_if_node_manager_is_running() {
  FILE *file = fopen("/var/run/mostd.pid", "r");
  if (!file) {
    return false;
  }
  char pid_line[32];
  if (!fgets(pid_line, sizeof(pid_line), file)) {
    fclose(file);
    return false;
  }
  fclose(file);
  /* fout << "Node manager pid " << strlen(pid_line) << " " << pid_line; */
  pid_line[strlen(pid_line) - 1] = '\0';
  if (access((string("/proc/") + pid_line).c_str(), F_OK) == 0)
    return true;
  else
    return false;
}

bool st_node_manager_is_active() {
  /*
  if(st_check_if_node_manager_is_running())
  {
      return true;
  }
  */
  st_node_manager_erase_previous();
  return false;
}

st_licensed_data data;

/*#if defined(__RHEL__)
#define MOSTD_LOG_FILE "/home/most/mostd.log"
#else*/
#define MOSTD_LOG_FILE "/var/run/mostd.log"
/* #endif */

void st_rm_dump_state(ofstream &fout, st_node_information *ni);
void st_rm_death_loop(st_node_information *ni);

int main(int argc, char **argv) {
  local_build_path = mostd_get_build_path(argv[0]);
  fout.open(MOSTD_LOG_FILE, ios::out | ios::app);
  if (fout.fail()) {
    cout << "Error: Cant write log file, you should run this with appropriate "
            "privileges"
         << endl;
    exit(1);
  }
  mostd_display_message("Starting node manager");
  mostd_setup_signal_handler();
  if (st_node_manager_is_active()) {
    mostd_display_error("A node manager is already running");
    exit(1);
  }
  if (!st_check_license(data)) {
    mostd_display_error("License file not valid, exiting");
    exit(1);
  }

  if (!st_node_manager_create(&data)) {
    mostd_display_error("Can't create Node Manager daemon");
    exit(1);
  }
  /* current_ni has been initialized here */
  st_rm_init_images(current_ni);

  mostd_display_message("Images initialized.");

  st_rm_dump_state(fout, current_ni);

  int i = 0;

  while (true) {
    st_rm_death_loop(current_ni);
    i++;
    if (((i * (ST_SLICED_POKE_TIME)) % 3600 == 0)) {
      mostd_display_message("* Working correctly *");
      i = 0;
    }
    if (!st_random_check(data)) {
      mostd_display_error("* Most node manager encountered an exception *");
      fout << PRE_MSG
           << "Currently licensed nodes: " << (current_ni->licensed_nodes)
           << endl;
      fout << PRE_MSG
           << "Available nodes         : " << (current_ni->available_nodes)
           << endl;
      flush(fout);
      st_node_manager_shutdown();
      exit(1);
    }
  }
}
