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
/**
 * This an stshell driver for SESC Multiprocessor simulator
 * downlodable at http://sesc.sourceforge.net/
 *
 * Author: Gianluca Palermo, 2007
 */

#include <iostream>
#include <string>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <cmath>
#include <cstdlib>
#include <unistd.h>
#include <st_design_space.h>
#include <st_shell_variables.h>
#include <st_parser.h>
#include <st_sim_utils.h>
#include <st_driver.h>

using namespace std;

void write_sesc_config_file(map<string, int> parameters);
bool read_sesc_report_file(st_env *, int, st_vector *, st_vector *);

class st_sesc_driver : public st_driver {
public:
  st_design_space *get_design_space(st_env *);

  st_point *simulate(st_point &, st_env *);
  bool is_valid(st_point &, st_env *);
  bool is_thread_safe() { return true; };

  string get_information();
  string get_name();
  st_sesc_driver(){};
  ~st_sesc_driver(){};
};

string st_sesc_driver::get_information() {
  return "Sesc Multiprocessor simulator\n";
}

#include <sstream>
#define kilo (1024)
#define mega (kilo * kilo)

vector<string> create_exp2_vec(int min, int max) {
  vector<string> elements;
  string sep = "";
  int x;
  int y;
  for (x = min; x <= max; x = x * 2) {
    ostringstream str;
    if ((x / mega) >= 1) {
      y = x / mega;
      sep = "M";
    } else {
      if ((x / kilo) >= 1) {
        y = x / kilo;
        sep = "K";
      } else {
        y = x;
        sep = "";
      }
    }
    str << y;
    elements.push_back(str.str() + sep);
  }
  return elements;
}

#define string_modifier(s) ((s).c_str()[(s).size() - 1])

int get_int_from_string(string s, string &mod) {
  mod = "";
  if (string_modifier(s) == 'k') {
    s.erase((s).size() - 1);
    return atoi(s.c_str());
  } else {
    if (string_modifier(s) == 'M') {
      mod = "k";
      s.erase((s).size() - 1);
      return atoi(s.c_str()) * kilo;
    } else {
      return atoi(s.c_str());
    }
  }
}

st_design_space *st_sesc_driver::get_design_space(st_env *env) {
  st_design_space *design_space = new st_design_space();

  vector<string> cs = create_exp2_vec(2 * kilo, 64 * kilo);
  design_space->insert_scalar(env, "ics", ST_SCALAR_TYPE_LIST, 0, cs.size() - 1,
                              cs);
  design_space->insert_scalar(env, "dcs", ST_SCALAR_TYPE_LIST, 0, cs.size() - 1,
                              cs);

  vector<string> l2cs = create_exp2_vec(32 * kilo, 1 * mega);
  design_space->insert_scalar(env, "l2cs", ST_SCALAR_TYPE_LIST, 0,
                              l2cs.size() - 1, l2cs);

  vector<string> ways = create_exp2_vec(1, 16);
  design_space->insert_scalar(env, "icw", ST_SCALAR_TYPE_LIST, 0,
                              ways.size() - 1, ways);
  design_space->insert_scalar(env, "dcw", ST_SCALAR_TYPE_LIST, 0,
                              ways.size() - 1, ways);
  design_space->insert_scalar(env, "l2cw", ST_SCALAR_TYPE_LIST, 0,
                              ways.size() - 1, ways);

  vector<string> iw = create_exp2_vec(1, 8);
  design_space->insert_scalar(env, "iwidth", ST_SCALAR_TYPE_LIST, 0,
                              iw.size() - 1, iw);

  vector<string> cbs = create_exp2_vec(16, 128);
  design_space->insert_scalar(env, "cbs", ST_SCALAR_TYPE_LIST, 0,
                              cbs.size() - 1, cbs);

  vector<string> pn = create_exp2_vec(2, 16);
  design_space->insert_scalar(env, "pn", ST_SCALAR_TYPE_LIST, 0, pn.size() - 1,
                              pn);

  int scenario = 0;
  int scen_num = 0;

  if (!current_environment.shell_variables.get_integer("number_of_scenarios",
                                                       scen_num)) {
    prs_display_message("Assuming number_of_scenarios=1, please define it "
                        "before loading the driver");
    scen_num = 1;
  }

  for (scenario = 0; scenario < scen_num; scenario++) {
    string metric_string;
    ostringstream scenario_str;
    scenario_str << scenario;
    metric_string = ("total_energy_" + scenario_str.str());
    design_space->insert_metric(env, metric_string, "J"); /* To be verified */
    metric_string = ("total_delay_" + scenario_str.str());
    design_space->insert_metric(env, metric_string,
                                "cycles"); /* To be verified */
  }
  return design_space;
}

string get_sesc_path(st_env *env) {
  string exec_path;
  if (!env->shell_variables.get_string("sesc_path", exec_path)) {
    /*prs_display_error("Please define sesc path; now using defaults ./");*/
    return "";
  } else
    return exec_path;
}

bool create_par_map(st_point &point, st_env *env, map<string, int> &m_par) {
  int design_space_size = env->current_design_space->ds_parameters.size();
  string mod;

  for (int j = 0; j < design_space_size; j++) {
    string par_name = env->current_design_space->ds_parameters[j].name;
    st_scalar the_parameter =
        env->current_design_space->scalar_parameters[par_name];

    if (the_parameter.list.size() <= point[j]) {
      return false;
    }

    string par_value_string = the_parameter.list[point[j]];
    int par_value = get_int_from_string(par_value_string, mod);
    m_par.insert(make_pair(par_name, par_value));
  }
  return true;
}

bool st_sesc_driver::is_valid(st_point &point, st_env *env) {
  map<string, int> m_par;
  int design_space_size = env->current_design_space->ds_parameters.size();
  if (point.size() != design_space_size) {
    return false;
  }

  bool sc = create_par_map(point, env, m_par);

  if (!sc)
    return false;

  /*due to memory hierarchy*/
  if ((m_par["ics"] >= m_par["l2cs"]) || (m_par["dcs"] >= m_par["l2cs"]))
    return false;

  /*due to Cacti Limits*/
  int i_cache_lines = (m_par["ics"] * kilo) / (m_par["cbs"] * m_par["icw"]);
  int d_cache_lines = (m_par["dcs"] * kilo) / (m_par["cbs"] * m_par["dcw"]);

  if ((i_cache_lines < 8) || (d_cache_lines < 8))
    return false;

  // cout << m_par["ics"] << " " << m_par["dcs"] << " " << m_par["l2cs"] << " "
  // << m_par["icw"] << " " << m_par["dcw"]; cout << " " << m_par["l2cw"] << " "
  // << m_par["iwidth"] << " " << m_par["cbs"] << " " << m_par["pn"] << endl;

  return true;
}

st_point *st_sesc_driver::simulate(st_point &point, st_env *env) {
  st_point *simulated_point = new st_point(point);
  if (!is_valid(point, env)) {
    simulated_point->set_error(ST_POINT_NON_FATAL_ERROR);
    return simulated_point;
  }

  map<string, int> m_par;
  create_par_map(point, env, m_par);

  string sesc_path = get_sesc_path(env);

  st_object const *command_line;
  bool local_command_line = false;
  st_vector *metrics = new st_vector;
  st_vector *statistics = new st_vector;
  int scenario = 0;

  int scen_num = 0;

  if (!current_environment.shell_variables.get_integer("number_of_scenarios",
                                                       scen_num)) {
    scen_num = 1;
    string single_command_line;
    if (!env->shell_variables.get_string("sesc_command_line",
                                         single_command_line)) {
      /* prs_display_error("Please define a command line for sesc");*/
      simulated_point->set_error(ST_POINT_FATAL_ERROR);
      return simulated_point;
    }
    st_object *new_command_line = new st_list();
    to<st_list *>(new_command_line)->insert(st_string(single_command_line));
    command_line = new_command_line;
    local_command_line = true;
  } else {
    if (!env->shell_variables.get("sesc_command_lines", command_line)) {
      /*prs_display_error("Please define a command line list for sesc");*/
      simulated_point->set_error(ST_POINT_FATAL_ERROR);
      return simulated_point;
    }
    if (!is_a<st_list const *>(command_line)) {
      /* prs_display_error("Invalid command line list for sesc"); */
      simulated_point->set_error(ST_POINT_FATAL_ERROR);
      return simulated_point;
    }
  }

  st_list const *command_line_list_c = to<st_list const *>(command_line);
  st_list *command_line_list = const_cast<st_list *>(command_line_list_c);
  list<st_object *>::iterator command_line_list_iterator;

  for (command_line_list_iterator = command_line_list->begin();
       command_line_list_iterator != command_line_list->end();
       command_line_list_iterator++) {
    if (is_a<st_string const *>((*command_line_list_iterator))) {
      write_sesc_config_file(m_par);

      // string value of the processor number.
      bool res;
      string pn = env->current_design_space->get_parameter_representation(
          env, point, "pn");
      string wattch =
          (sesc_path + "wattchify ./_sesc_configuration." +
           st_get_unique_string_identifier() + ".cfg ./_sesc_w_configuration." +
           st_get_unique_string_identifier() + ".cfg &> /dev/null");

      string cacti = (sesc_path + "cactify ./_sesc_w_configuration." +
                      st_get_unique_string_identifier() +
                      ".cfg ./_sesc_wc_configuration." +
                      st_get_unique_string_identifier() + ".cfg &> /dev/null");

      string sesc =
          (sesc_path + "sesc.smp -c _sesc_wc_configuration." +
           st_get_unique_string_identifier() + ".cfg -d _sesc_out." +
           st_get_unique_string_identifier() + " -f txt  " +
           to<st_string const *>((*command_line_list_iterator))->get_string() +
           " -p" + pn + " &> /dev/null");
      /*           string sesc = (sesc_path + "sesc.smp -c
       * _sesc_wc_configuration." + st_get_unique_string_identifier() + ".cfg -d
       * _sesc_out." + st_get_unique_string_identifier() + " -f txt  " +
       * to<st_string const *>((*command_line_list_iterator))->get_string() +
       * "mpeg2dec"+ pn + " -b "+ to<st_string const
       * *>((*command_line_list_iterator))->get_string()+"video_640_480.m2v " +
       * " &> /dev/null" ); */

      cout << wattch << endl;
      shell_command(wattch.c_str());

      cout << cacti << endl;
      shell_command(cacti.c_str());

      cout << sesc << endl;
      shell_command(sesc.c_str());

      // Reading report file to set the simulation statistics
      if (!read_sesc_report_file(env, scenario, metrics, statistics)) {
        string sctemp =
            "rm _sesc*." + st_get_unique_string_identifier() + ".* -f";
        cout << sctemp << endl;
        // removing all the sesc tmp files;
        shell_command(sctemp.c_str());

        simulated_point->set_error(ST_POINT_NON_FATAL_ERROR);
        return simulated_point;
      }

      int drtf = false;
      env->shell_variables.get_integer("dont_remove_temporary_files", drtf);

      if (!drtf) {
        string sctemp =
            "rm _sesc*." + st_get_unique_string_identifier() + ".* -f";
        cout << sctemp << endl;
        shell_command(sctemp.c_str());

        sctemp = "rm core*.* -f";
        shell_command(sctemp.c_str());
      }
      scenario++;
    }
  }

  // inserting point metrics and statistics
  simulated_point->set_properties("metrics", *metrics);
  simulated_point->set_properties("statistics", *statistics);

  if (local_command_line)
    delete command_line;

  delete metrics;
  delete statistics;

  return simulated_point;
}

void write_sesc_config_file(map<string, int> parameters) {
  ofstream fout(
      ("./_sesc_configuration." + st_get_unique_string_identifier() + ".cfg")
          .c_str());
  fout << "procsPerNode  = " << parameters["pn"] << " " << endl;
  fout << "cacheLineSize = " << parameters["cbs"] << " " << endl;
  fout << "" << endl;
  fout << "issue         = " << parameters["iwidth"]
       << "    # processor issue width " << endl;
  fout << "cpucore[0:$(procsPerNode)-1] = 'issueX'  " << endl;
  fout << "" << endl;
  fout << "#<shared.conf> (contents below) " << endl;
  fout << "" << endl;
  fout << "############################## " << endl;
  fout << "# SYSTEM                     # " << endl;
  fout << "############################## " << endl;
  fout << "" << endl;
  fout << "enableICache   = true " << endl;
  fout << "NoMigration    = true " << endl;
  fout << "tech           = 0.10 " << endl;
  fout << "pageSize       = 4096 " << endl;
  fout << "fetchPolicy    = 'outorder' " << endl;
  fout << "issueWrongPath = true " << endl;
  fout << "" << endl;
  fout << "technology = 'techParam' " << endl;
  fout << "" << endl;
  fout << "############################### " << endl;
  fout << "# clock-panalyzer input       # " << endl;
  fout << "############################### " << endl;
  fout << "[techParam] " << endl;
  fout << "clockTreeStyle = 1  # 1 for Htree or 2 for balHtree " << endl;
  fout << "tech       = 70   # nm " << endl;
  fout << "frequency  = 1e9    # Hz " << endl;
  fout << "skewBudget = 20   # in ps " << endl;
  fout << "areaOfChip = 200  # in mm^2 " << endl;
  fout << "loadInClockNode = 20  # in pF " << endl;
  fout << "optimalNumberOfBuffer = 3 " << endl;
  fout << "" << endl;
  fout << "############################## " << endl;
  fout << "# PROCESSORS' CONFIGURATION  # " << endl;
  fout << "############################## " << endl;
  fout << "" << endl;
  fout << "[issueX] " << endl;
  fout << "frequency       = 1e9 " << endl;
  fout << "areaFactor      = ($(issue)*0.17+0.32)/4  # Area compared to "
          "IBM-POWER5 "
       << endl;
  fout << "inorder         = false " << endl;
  fout << "fetchWidth      = $(issue) " << endl;
  fout << "instQueueSize   = 2*$(issue) " << endl;
  fout << "issueWidth      = $(issue) " << endl;
  fout << "retireWidth     = $(issue)+1 " << endl;
  fout << "decodeDelay     = 6 " << endl;
  fout << "renameDelay     = 3 " << endl;
  fout << "wakeupDelay     = 6                 # -> 6+3+6+1+1=17 branch "
          "mispred. penalty"
       << endl;
  fout << "maxBranches     = 16*$(issue) " << endl;
  fout << "bb4Cycle        = 1 " << endl;
  fout << "maxIRequests    = 4 " << endl;
  fout << "interClusterLat = 2 " << endl;
  fout << "intraClusterLat = 1 " << endl;
  fout << "cluster[0]      = 'FXClusterIssueX' " << endl;
  fout << "cluster[1]      = 'FPClusterIssueX' " << endl;
  fout << "stForwardDelay  = 2  " << endl;
  fout << "maxLoads        = 10*$(issue)+16 " << endl;
  fout << "maxStores       = 10*$(issue)+16 " << endl;
  fout << "regFileDelay    = 3 " << endl;
  fout << "robSize         = 36*$(issue)+32 " << endl;
  fout << "intRegs         = 32+16*$(issue) " << endl;
  fout << "fpRegs          = 32+12*$(issue) " << endl;
  fout << "bpred           = 'BPredIssueX' " << endl;
  fout << "dtlb            = 'FXDTLB' " << endl;
  fout << "itlb            = 'FXITLB' " << endl;
  fout << "dataSource      = \"DMemory DL1\" " << endl;
  fout << "instrSource     = \"IMemory IL1\" " << endl;
  fout << "enableICache    = true " << endl;
  fout << "OSType          = 'dummy' " << endl;
  fout << "" << endl;
  fout << "" << endl;
  fout << "# integer functional units " << endl;
  fout << "" << endl;
  fout << "[FXClusterIssueX] " << endl;
  fout << "winSize    = 12*$(Issue)+32 # number of entries in window " << endl;
  fout << "recycleAt  = 'Execute' " << endl;
  fout << "schedNumPorts = 4 " << endl;
  fout << "schedPortOccp = 1 " << endl;
  fout << "wakeUpNumPorts= 4 " << endl;
  fout << "wakeUpPortOccp= 1 " << endl;
  fout << "wakeupDelay   = 3 " << endl;
  fout << "schedDelay    = 1 # Minimum latency like a intraClusterLat " << endl;
  fout << "iStoreLat  = 1 " << endl;
  fout << "iStoreUnit = 'LDSTIssueX' " << endl;
  fout << "iLoadLat   = 1 " << endl;
  fout << "iLoadUnit  = 'LDSTIssueX' " << endl;
  fout << "iALULat    = 1 " << endl;
  fout << "iALUUnit   = 'ALUIssueX' " << endl;
  fout << "iBJLat     = 1 " << endl;
  fout << "iBJUnit    = 'ALUIssueX' " << endl;
  fout << "iDivLat    = 12 " << endl;
  fout << "iDivUnit   = 'ALUIssueX' " << endl;
  fout << "iMultLat   = 4 " << endl;
  fout << "iMultUnit  = 'ALUIssueX' " << endl;
  fout << "" << endl;
  fout << "[LDSTIssueX] " << endl;
  fout << "Num = $(issue)/3+1 " << endl;
  fout << "Occ = 1 " << endl;
  fout << "" << endl;
  fout << "[ALUIssueX] " << endl;
  fout << "Num = $(issue)/3+1 " << endl;
  fout << "Occ = 1 " << endl;
  fout << "" << endl;
  fout << "# floating point functional units " << endl;
  fout << "" << endl;
  fout << "[FPClusterIssueX] " << endl;
  fout << "winSize    = 8*$(issue) " << endl;
  fout << "recycleAt  = 'Execute' " << endl;
  fout << "schedNumPorts = 4 " << endl;
  fout << "schedPortOccp = 1 " << endl;
  fout << "wakeUpNumPorts= 4 " << endl;
  fout << "wakeUpPortOccp= 1 " << endl;
  fout << "wakeupDelay   = 3 " << endl;
  fout << "schedDelay    = 1 # Minimum latency like a intraClusterLat " << endl;
  fout << "fpALULat   = 1 " << endl;
  fout << "fpALUUnit  = 'FPIssueX' " << endl;
  fout << "fpMultLat  = 2 " << endl;
  fout << "fpMultUnit = 'FPIssueX' " << endl;
  fout << "fpDivLat   = 10 " << endl;
  fout << "fpDivUnit  = 'FPIssueX' " << endl;
  fout << "" << endl;
  fout << "[FPIssueX] " << endl;
  fout << "Num = $(issue)/2+1 " << endl;
  fout << "Occ = 1 " << endl;
  fout << "" << endl;
  fout << "# branch prediction mechanism " << endl;
  fout << "" << endl;
  fout << "[BPredIssueX]" << endl;
  fout << "type          = \"hybrid\" " << endl;
  fout << "BTACDelay     = 0 " << endl;
  fout << "l1size        = 1 " << endl;
  fout << "l2size        = 16*1024 " << endl;
  fout << "l2Bits        = 1 " << endl;
  fout << "historySize   = 11 " << endl;
  fout << "Metasize      = 16*1024 " << endl;
  fout << "MetaBits      = 2 " << endl;
  fout << "localSize     = 16*1024 " << endl;
  fout << "localBits     = 2 " << endl;
  fout << "btbSize       = 2048 " << endl;
  fout << "btbBsize      = 1 " << endl;
  fout << "btbAssoc      = 2 " << endl;
  fout << "btbReplPolicy = 'LRU' " << endl;
  fout << "btbHistory    = 0 " << endl;
  fout << "rasSize       = 32 " << endl;
  fout << "" << endl;
  fout << "# memory translation mechanism " << endl;
  fout << "" << endl;
  fout << "[FXDTLB] " << endl;
  fout << "deviceType = 'cache' " << endl;
  fout << "size       = 64*8 " << endl;
  fout << "assoc      = 4 " << endl;
  fout << "bsize      = 8 " << endl;
  fout << "numPorts   = 2 " << endl;
  fout << "replPolicy = 'LRU' " << endl;
  fout << "" << endl;
  fout << "[FXITLB] " << endl;
  fout << "deviceType = 'cache' " << endl;
  fout << "size       = 64*8 " << endl;
  fout << "assoc      = 4 " << endl;
  fout << "bsize      = 8 " << endl;
  fout << "numPorts   = 2 " << endl;
  fout << "replPolicy = 'LRU' " << endl;
  fout << "" << endl;
  fout << "############################## " << endl;
  fout << "# MEMORY SUBSYSTEM           # " << endl;
  fout << "############################## " << endl;
  fout << "" << endl;
  fout << "# instruction source " << endl;
  fout << "[IMemory] " << endl;
  fout << "deviceType    = 'icache' " << endl;
  fout << "size          = " << parameters["ics"] << "*1024 " << endl;
  fout << "assoc         = " << parameters["icw"] << " " << endl;
  fout << "bsize         = $(cacheLineSize) " << endl;
  fout << "writePolicy   = 'WT' " << endl;
  fout << "replPolicy    = 'LRU' " << endl;
  fout << "numPorts      = 2 " << endl;
  fout << "portOccp      = 1 " << endl;
  fout << "hitDelay      = 2 " << endl;
  fout << "missDelay     = 1                # this number is added to the "
          "hitDelay "
       << endl;
  fout << "MSHR          = \"iMSHR\" " << endl;
  fout << "lowerLevel    = \"L1L2Bus L1L2\" " << endl;
  fout << "" << endl;
  fout << "[iMSHR]" << endl;
  fout << "type = 'single'" << endl;
  fout << "size = 32 " << endl;
  fout << "bsize = $(cacheLineSize) " << endl;
  fout << "" << endl;
  fout << "# data source " << endl;
  fout << "[DMemory] " << endl;
  fout << "deviceType    = 'cache' " << endl;
  fout << "size          = " << parameters["dcs"] << "*1024 " << endl;
  fout << "assoc         = " << parameters["dcw"] << " " << endl;
  fout << "bsize         = $(cacheLineSize) " << endl;
  fout << "writePolicy   = 'WT' " << endl;
  fout << "replPolicy    = 'LRU' " << endl;
  fout << "numPorts      = $(issue)/3+1 " << endl;
  fout << "portOccp      = 1 " << endl;
  fout << "hitDelay      = 2 " << endl;
  fout << "missDelay     = 1                #this number is added to the "
          "hitDelay "
       << endl;
  fout << "maxWrites     = 8 " << endl;
  fout << "MSHR          = \"DMSHR\" " << endl;
  fout << "lowerLevel    = \"L1L2Bus L1L2\" " << endl;
  fout << "" << endl;
  fout << "[DMSHR] " << endl;
  fout << "type = 'single' " << endl;
  fout << "size = 64 " << endl;
  fout << "bsize = $(cacheLineSize) " << endl;
  fout << "" << endl;
  fout << "# bus between L1s and L2 " << endl;
  fout << "[L1L2Bus] " << endl;
  fout << "deviceType = 'bus' " << endl;
  fout << "numPorts   = 1 " << endl;
  fout << "portOccp   = 1                   # assuming 256 bit bus" << endl;
  fout << "delay      = 1 " << endl;
  fout << "lowerLevel = \"L2Cache L2 sharedBy 1\" " << endl;
  fout << "" << endl;
  fout << "# private L2 " << endl;
  fout << "[L2Cache] " << endl;
  fout << "deviceType    = 'smpcache' " << endl;
  fout << "size          = " << parameters["l2cs"] << "*1024 " << endl;
  fout << "assoc         = " << parameters["l2cw"] << " " << endl;
  fout << "bsize         = $(cacheLineSize) " << endl;
  fout << "writePolicy   = 'WB' " << endl;
  fout << "replPolicy    = 'LRU' " << endl;
  fout << "protocol      = 'MESI' " << endl;
  fout << "numPorts      = 2                # one for L1, one for snooping "
       << endl;
  fout << "portOccp      = 2 " << endl;
  fout << "hitDelay      = 9 " << endl;
  fout << "missDelay     = 11               # exclusive, i.e., not added to "
          "hitDelay "
       << endl;
  fout << "displNotify   = false " << endl;
  fout << "MSHR          = 'L2MSHR' " << endl;
  fout << "lowerLevel    = \"SystemBus SysBus sharedBy 32\" " << endl;
  fout << "" << endl;
  fout << "[L2MSHR] " << endl;
  fout << "size =  64 " << endl;
  fout << "type = 'single' " << endl;
  fout << "bsize = $(cacheLineSize) " << endl;
  fout << "" << endl;
  fout << "[SystemBus] " << endl;
  fout << "deviceType    = 'systembus' " << endl;
  fout << "numPorts      = 1 " << endl;
  fout << "portOccp      = 1 " << endl;
  fout << "delay         = 1 " << endl;
  fout << "lowerLevel    = \"MemoryBus MemoryBus\" " << endl;
  fout << "BusEnergy     = 0.03 " << endl;
  fout << "" << endl;
  fout << "[MemoryBus] " << endl;
  fout << "deviceType    = 'bus' " << endl;
  fout << "numPorts      = 1 " << endl;
  fout << "portOccp      = $(cacheLineSize) / 4   # assuming 4 bytes/cycle bw  "
       << endl;
  fout << "delay         = 15 " << endl;
  fout << "lowerLevel    = \"Memory Memory\" " << endl;
  fout << "" << endl;
  fout << "[Memory] " << endl;
  fout << "deviceType    = 'niceCache' " << endl;
  fout << "size          = 64 " << endl;
  fout << "assoc         = 1 " << endl;
  fout << "bsize         = 64 " << endl;
  fout << "writePolicy   = 'WB' " << endl;
  fout << "replPolicy    = 'LRU' " << endl;
  fout << "numPorts      = 1 " << endl;
  fout << "portOccp      = 1 " << endl;
  fout << "hitDelay      = 500 - 31  # 5.0GHz: 100ns is 500 cycles RTT - 16 "
          "busData  "
       << endl;
  fout << "missDelay     = 500 - 31  # - 15 memory bus => 500 - 31 " << endl;
  fout << "MSHR          = NoMSHR " << endl;
  fout << "lowerLevel    = 'voidDevice' " << endl;
  fout << "" << endl;
  fout << "[NoMSHR] " << endl;
  fout << "type = 'none' " << endl;
  fout << "size = 128 " << endl;
  fout << "bsize = 64 " << endl;
  fout << "" << endl;
  fout << "[voidDevice] " << endl;
  fout << "deviceType    = 'void' " << endl;
  fout << "" << endl;
  fout.close();
}

bool read_sesc_report_file(st_env *env, int scenario, st_vector *metrics,
                           st_vector *statistics) {

  string build_path;
  if (!(env)->shell_variables.get_string("current_build_path", build_path)) {
    prs_display_error(
        "Cannot state the current build path. This is a MAJOR error");
    return false;
  }
  string sesc_pack_path = build_path + "/scripts/sesc_stub";

  ofstream fout(
      ("./_sesc_parse_out." + st_get_unique_string_identifier() + ".pl")
          .c_str());
  fout << "#!/usr/bin/env perl" << endl;
  fout << "" << endl;
  fout << "use sesc;" << endl;
  fout << "" << endl;
  fout << "my $dataFile = $ARGV[0];" << endl;
  fout << "my  $cf = sesc->new($dataFile);" << endl;
  fout << "my  $nCycles = $cf->getResultField(\"OSSim\",\"nCycles\");" << endl;
  fout << "my $totEnergy  = "
          "1e-9*$cf->getResultField(\"EnergyMgr\",\"totEnergy\");"
       << endl;
  fout << "print \"sim_cycle $nCycles\n\";" << endl;
  fout << "print \"total_energy $totEnergy\n\";" << endl;
  fout << "" << endl;
  fout.close();

  string perl_command = "perl -I" + sesc_pack_path + " ./_sesc_parse_out." +
                        st_get_unique_string_identifier() + ".pl ./_sesc_out." +
                        st_get_unique_string_identifier() +
                        ".txt >> ./_sesc_parser_out." +
                        st_get_unique_string_identifier() + ".txt";
  // cout << perl_command << endl;
  shell_command(perl_command.c_str());

  string str;
  char line[150];
  double num = 0, enrg = 0, delay = 0;

  line[0] = '\0';
  FILE *file =
      fopen(("./_sesc_parser_out." + st_get_unique_string_identifier() + ".txt")
                .c_str(),
            "r");
  if (!file) {
    return false;
  }
  bool found1 = false;
  bool found2 = false;
  while (fgets(line, sizeof(line), file)) {
    std::istringstream sin(line);
    if (sin >> str >> ws >> num) {
      if (!str.compare("sim_cycle")) {
        delay = num;
        num = 0.0;
        found1 = true;
      }
      if (!str.compare("total_energy")) {
        enrg = num;
        num = 0.0;
        found2 = true;
      }
    }
  }
  if (!found1 || !found2) {
    return false;
  }
  fclose(file);
  metrics->insert(2 * scenario + 0, st_double(enrg));
  metrics->insert(2 * scenario + 1, st_double(delay));

  return true;
}
/*Here is ok*/

string st_sesc_driver::get_name() { return "SESC Driver - Multiple Scenarios"; }

extern "C" {
st_driver *drv_generate_driver() {
  prs_display_message("Instantiating SESC driver - multiple scenarios");
  return new st_sesc_driver();
}
}
