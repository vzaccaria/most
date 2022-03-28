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
#include <iostream>
#include <string>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <cmath>
#include <unistd.h>
#include <st_shell_variables.h>
#include <st_parser.h>
#include <st_sim_utils.h>
#include <st_driver.h>

using namespace std;

class st_wattch_driver: public st_driver
{
    public:
        string get_information();
        st_vector * get_design_space_parameters();
        st_vector * get_metrics();
        st_vector * get_statistics();
        st_point  * simulate(st_point &, st_env *);
        string get_name();
        st_wattch_driver() {};
        ~st_wattch_driver() {};
};

st_double const_compute_area(const st_point &, st_env *) ;
double compute_area(st_point &) ;
int get_int_from_string(string);
bool read_wattch_report_file(st_point *, int, st_vector *, st_vector *, st_env *);
void write_wattch_config_file(map<string, int> parameters);

// Cache parameters

// Caches dimension range
st_string s_1k("1K");
st_string s_2k("2K");
st_string s_4k("4K");
st_string s_8k("8K");
st_string s_16k("16K");
st_string s_32k("32K");
st_string s_64k("64K");
st_string s_128k("128K");
st_string s_256k("256K");

// Caches Block dimension range
st_string s_8b("8");
st_string s_16b("16");
st_string s_32b("32");
st_string s_64b("64");
st_string s_128b("128");
st_string s_256b("256");

// Caches Associativity range
st_string s_1w("1");
st_string s_2w("2");
st_string s_4w("4");
st_string s_8w("8");
st_string s_16w("16");
////////////////////////////////////////////////

////////////////////////////////////////////////
// Processor units range
// Issue width range
st_string s_1iw("1");
st_string s_2iw("2");
st_string s_4iw("4");
st_string s_8iw("8");

// Integer ALU range
st_string s_1ia("1");
st_string s_2ia("2");
st_string s_4ia("4");

// Float ALU range
st_string s_1fpa("1");
st_string s_2fpa("2");
st_string s_4fpa("4");

// Integer Multiplier range
st_string s_1im("1");
st_string s_2im("2");
st_string s_4im("4");

// Float Multiplier range
st_string s_1fpm("1");
st_string s_2fpm("2");
st_string s_4fpm("4");
////////////////////////////////////////////////

#define CACHE_SIZE_RANGE 9 
#define CACHE_BLOCK_RANGE 6
#define CACHE_ASSOCIATIVITY_RANGE 5
#define ISSUE_WIDTH_RANGE  4
#define INT_ALUS_RANGE     3
#define FLOAT_ALUS_RANGE   3
#define INT_MULTS_RANGE    3
#define FLOAT_MULTS_RANGE  3

//#define L2_SIZE_RANGE 5   
//#define L2_BLOCK_RANGE 6 
//#define L2_ASSOCIATIVITY_RANGE 5 

int get_int_from_string(string s)
{
   
   if (s == s_1w.get_string())
      return(1);
   else if ( s == s_2w.get_string())
      return(2);
   else if ( s == s_4w.get_string())
      return(4);
   else if ( s == s_8w.get_string())
      return(8);
   else if ( s == s_16b.get_string())
      return(16);
   else if ( s == s_32b.get_string())
      return(32);
   else if ( s == s_64b.get_string())
      return(64);
   else if ( s == s_128b.get_string())
      return(128);
   else if ( s == s_256b.get_string())
      return(256);
   else if ( s == s_1k.get_string())
      return(1024);
   else if ( s == s_2k.get_string())
      return(2048);
   else if ( s == s_4k.get_string())
      return(4096);
   else if ( s == s_8k.get_string())
      return(8192);
   else if ( s == s_16k.get_string())
      return(16384);
   else if ( s == s_32k.get_string())
      return(32768);
   else if ( s == s_64k.get_string())
      return(65536);
   else if ( s == s_128k.get_string())
      return(131072);
   else if ( s == s_256k.get_string())
      return(262144);
}

string st_wattch_driver::get_information()
{
	return "Wattch superscalar performance and energy simulator\n";
}

st_vector *st_wattch_driver::get_design_space_parameters()
{

   const st_object *cache_sizes[CACHE_SIZE_RANGE]={ & s_1k, & s_2k, & s_4k, & s_8k, & s_16k, & s_32k, & s_64k, & s_128k, & s_256k};

   const st_object *cache_blocks[CACHE_BLOCK_RANGE] = {  & s_8b, & s_16b, & s_32b, & s_64b, & s_128b, & s_256b };

   const st_object *cache_associativity[CACHE_ASSOCIATIVITY_RANGE] = {  & s_1w, & s_2w, & s_4w, & s_8w , & s_16w};

   const st_object *l2_sizes[CACHE_SIZE_RANGE]={  & s_1k, & s_2k, & s_4k, & s_8k, & s_16k, & s_32k, & s_64k, & s_128k, & s_256k };

   const st_object *l2_blocks[CACHE_BLOCK_RANGE] = { &s_8b, & s_16b, & s_32b, & s_64b, & s_128b, & s_256b };

   const st_object *l2_associativity[CACHE_ASSOCIATIVITY_RANGE] = { & s_1w, & s_2w, & s_4w, & s_8w, & s_16w };

   const st_object *issue_widths[ISSUE_WIDTH_RANGE] = { & s_1iw, & s_2iw, & s_4iw, & s_8iw };

   const st_object *int_alus[INT_ALUS_RANGE] = { & s_1ia, & s_2ia, & s_4ia };

   const st_object *float_alus[FLOAT_ALUS_RANGE] = { & s_1fpa, & s_2fpa, & s_4fpa };

   const st_object *int_mults[INT_MULTS_RANGE] = { & s_1im, & s_2im,  & s_4im };

   const st_object *float_mults[FLOAT_MULTS_RANGE] = { & s_1fpm, & s_2fpm, & s_4fpm };

   st_vector *dse= new st_vector();

   st_vector data_cache_size(cache_sizes, CACHE_SIZE_RANGE);
   st_vector data_cache_block(cache_blocks, CACHE_BLOCK_RANGE);
   st_vector data_cache_associativity(cache_associativity, CACHE_ASSOCIATIVITY_RANGE);

   st_vector instruction_cache_size(cache_sizes, CACHE_SIZE_RANGE);
   st_vector instruction_cache_block(cache_blocks, CACHE_BLOCK_RANGE);
   st_vector instruction_cache_associativity(cache_associativity, CACHE_ASSOCIATIVITY_RANGE);

   st_vector l2_cache_size(l2_sizes, CACHE_SIZE_RANGE);
   st_vector l2_cache_block(l2_blocks, CACHE_BLOCK_RANGE);
   st_vector l2_cache_associativity(l2_associativity, CACHE_ASSOCIATIVITY_RANGE);

   st_vector issue_width(issue_widths, ISSUE_WIDTH_RANGE);

   st_vector int_alu_number(int_alus, INT_ALUS_RANGE);

   st_vector float_alu_number(float_alus, FLOAT_ALUS_RANGE);

   st_vector int_mult_number(int_mults, INT_MULTS_RANGE);

   st_vector float_mult_number(float_mults, FLOAT_MULTS_RANGE);

   data_cache_size.set_properties("name",st_string("dcs"));
   data_cache_block.set_properties("name",st_string("dbs"));
   data_cache_associativity.set_properties("name",st_string("dw"));

   instruction_cache_size.set_properties("name",st_string("ics"));
   instruction_cache_block.set_properties("name",st_string("ibs"));
   instruction_cache_associativity.set_properties("name",st_string("iw"));

   l2_cache_size.set_properties("name",st_string("l2cs"));
   l2_cache_block.set_properties("name",st_string("l2bs"));
   l2_cache_associativity.set_properties("name",st_string("l2w"));

   issue_width.set_properties("name",st_string("iwidth"));

   int_alu_number.set_properties("name",st_string("ialus"));

   float_alu_number.set_properties("name",st_string("fpalus"));

   int_mult_number.set_properties("name",st_string("imults"));

   float_mult_number.set_properties("name",st_string("fpmults"));

   dse->insert(0,instruction_cache_size);
   dse->insert(1,instruction_cache_block);
   dse->insert(2,instruction_cache_associativity);
   dse->insert(3,data_cache_size);
   dse->insert(4,data_cache_block);
   dse->insert(5,data_cache_associativity);
   dse->insert(6,l2_cache_size);
   dse->insert(7,l2_cache_block);
   dse->insert(8,l2_cache_associativity);
   dse->insert(9,issue_width);
   dse->insert(10,int_alu_number);
   dse->insert(11,float_alu_number);
   dse->insert(12,int_mult_number);
   dse->insert(13,float_mult_number);
   return dse;
}

st_vector *st_wattch_driver::get_metrics()
{
	st_vector *metrics= new st_vector();

    int scenario = 0;
    int scen_num = 0;
    if(!current_environment.shell_variables.get_integer("number_of_scenarios", scen_num))
    {
        prs_display_message("Assuming number_of_scenarios=1, please define it before loading the driver");
        scen_num = 1;
    }

    for(scenario=0; scenario < scen_num; scenario++)
    {
        string metric_string;
        ostringstream scenario_str;
        scenario_str << scenario;

        metric_string = ("total_energy_"+scenario_str.str());
        st_string total_energy(metric_string);
        
        metric_string = ("total_delay_"+scenario_str.str());
        st_string total_delay(metric_string);
        
        metric_string = ("total_area_"+scenario_str.str());
        st_string total_area(metric_string);

        metrics->insert(3*scenario+0,total_energy);
        metrics->insert(3*scenario+1,total_delay);
        metrics->insert(3*scenario+2,total_area);
    }
	return metrics;
}



st_vector *st_wattch_driver::get_statistics()
{
	st_vector *statistics= new st_vector();
    
    int scenario = 0;
    int scen_num = 0;

    if(!current_environment.shell_variables.get_integer("number_of_scenarios", scen_num))
    {
        prs_display_message("Assuming number_of_scenarios=1, please define it before loading the driver");
        scen_num = 1;
    }
    for(scenario=0; scenario < scen_num; scenario++)
    {
        string statistic_string;
        ostringstream scenario_str;
        scenario_str << scenario;

        statistic_string = ("il1_miss_rate_"+scenario_str.str());
        st_string il1_miss_rate(statistic_string);
        
        statistic_string = ("dl1_miss_rate_"+scenario_str.str());
        st_string dl1_miss_rate(statistic_string);
        
        statistic_string = ("l2_miss_rate"+scenario_str.str());
        st_string l2_miss_rate(statistic_string);

        statistics->insert(3*scenario+0, il1_miss_rate);
        statistics->insert(3*scenario+1, dl1_miss_rate);
        statistics->insert(3*scenario+2, l2_miss_rate);
    }
	return statistics;
}

string get_wattch_path(st_env *env)
{
	string exec_path;
	if(!env->shell_variables.get_string("wattch_path",exec_path)) 
	{
		/*prs_display_error("Please define wattch path; now using defaults...");*/
		return "wattch";
	}
	else return exec_path;
}

st_point *st_wattch_driver::simulate(st_point &point, st_env *env)       
{
   st_object const *dsp_obj;
   st_object const *ds_obj;
   map<string, int> m_par; 
   string str_key,str_value; // these strings are used to set the system configuration for the simulator
   int value;
   int found=0;
    
   st_point lower_bound;
   st_point upper_bound;

   lower_bound = opt_get_lower_bound(env);
   upper_bound = opt_get_upper_bound(env);
   
   string wattch_path=get_wattch_path(env);
   st_point *simulated_point = new st_point(point);

   
   // Read parameter values for the iss configuration file
   
   const st_vector *ds=opt_get_design_space_vector(env);

   if(point.size() != ds->size())
   {
        simulated_point->set_error(ST_ERR_CONSTR_ERR);
        return simulated_point;
   }
   
   for(int j=0; j<ds->size(); j++)
   {
      str_key=sim_get_parameter_name_by_index(env,j).get_string(); 
      if(point[j]>upper_bound[j] || point[j]<lower_bound[j])
      {
          simulated_point->set_error(ST_ERR_CONSTR_ERR);
          return simulated_point;
      }
      bool res;
      str_value= sim_get_sym_value_by_parname_and_level(env, str_key, point[j], res );
      if(!res)
      {
          simulated_point->set_error(ST_ERR_CONSTR_ERR);
          return simulated_point;
      }
      value = get_int_from_string(str_value);
      m_par.insert(make_pair(str_key,value));
   }
   m_par["ics"]=m_par["ics"]/(m_par["ibs"]*m_par["iw"]);
   m_par["dcs"]=m_par["dcs"]/(m_par["dbs"]*m_par["dw"]);
   m_par["l2cs"]=m_par["l2cs"]/(m_par["l2bs"]*m_par["l2w"]);
   
   if(m_par["l2bs"]<m_par["ibs"] || m_par["l2bs"]<m_par["dbs"])
   {
      /** These act like a constraint on the problem */
      simulated_point->set_error(ST_ERR_CONSTR_ERR);
      return simulated_point;
   }
   
   write_wattch_config_file(m_par);
   st_object const *command_line;
   bool local_command_line = false;
   st_vector *metrics = new st_vector;
   st_vector *statistics = new st_vector;
   int scen_num = 0;

   if(!current_environment.shell_variables.get_integer("number_of_scenarios", scen_num))
   {
       scen_num = 1;
       string single_command_line;
       if (!env->shell_variables.get_string("wattch_command_line", single_command_line))        
       {
           simulated_point->set_error(ST_ERR_SIM_ERR);
           return simulated_point;
       }
       st_object *new_command_line = new st_list();
       to<st_list *>(new_command_line)->insert(st_string(single_command_line));
       command_line = new_command_line;
       local_command_line = true;
   }
   else
   {
       if (!env->shell_variables.get("wattch_command_lines", command_line))
       {
           prs_display_error("Please define a command line list for wattch");
           simulated_point->set_error(ST_ERR_CONSTR_ERR);
           return simulated_point;
       }
       if (!is_a<st_list const *>(command_line))
       {
           prs_display_error("Invalid command line list for wattch");
           simulated_point->set_error(ST_ERR_CONSTR_ERR);
           return simulated_point;
       }
   }

    int scenario = 0;


    st_list const *command_line_list_c = to<st_list const *>(command_line);
    st_list *command_line_list = const_cast<st_list *>(command_line_list_c);
    list<st_object *>::iterator command_line_list_iterator;

    for (command_line_list_iterator = command_line_list->begin(); 
            command_line_list_iterator != command_line_list->end(); 
            command_line_list_iterator++)
    {
        if (is_a<st_string const *>((*command_line_list_iterator)))
        {
           string the_command_line = to<st_string *>(*command_line_list_iterator)->get_string();
           // Launch Wattch to simulate the application execution
           cout << endl << wattch_path +" -config configuration" + st_get_unique_string_identifier() +".cfg " + the_command_line << endl;
           system((wattch_path+" -config configuration"+st_get_unique_string_identifier()+".cfg " + the_command_line +" 2> results"+st_get_unique_string_identifier()+".txt > temp.txt").c_str());
           
        // Reading report file to set the simulation statistics
            if(!read_wattch_report_file(simulated_point, scenario, metrics, statistics, env))
            {
                simulated_point->set_error(ST_ERR_SIM_ERR);
                return simulated_point;
            }
            scenario++;
        }
        else
        {
            prs_display_error("Something is wrong in the specification of the command_line");
            simulated_point->set_error(ST_ERR_SIM_ERR);
            return simulated_point;
        }
    }
    simulated_point->set_properties("metrics", *metrics);
    simulated_point->set_properties("statistics", *statistics);
    
    if(local_command_line) 
       delete command_line; 

    delete metrics;
    delete statistics;

   
   return simulated_point;
}


void write_wattch_config_file(map<string, int> parameters)
{
   ofstream fout(("./configuration"+st_get_unique_string_identifier()+".cfg").c_str());
   fout << "# load configuration from a file" << endl;
   fout << "# -config               " << endl;
   fout << "" << endl;
   fout << "# dump configuration to a file" << endl;
   fout << "# -dumpconfig           " << endl;
   fout << "" << endl;
   fout << "# print help message" << endl;
   fout << "# -h                          false " << endl;
   fout << "" << endl;
   fout << "# verbose operation" << endl;
   fout << "# -v                          false " << endl;
   fout << "" << endl;
   fout << "# enable debug message" << endl;
   fout << "# -d                          false " << endl;
   fout << "" << endl;
   fout << "# start in Dlite debugger" << endl;
   fout << "# -i                          false " << endl;
   fout << "" << endl;
   fout << "# random number generator seed (0 for timer seed)" << endl;
   fout << "-seed                             1 " << endl;
   fout << "" << endl;
   fout << "# initialize and terminate immediately" << endl;
   fout << "# -q                          false " << endl;
   fout << "" << endl;
   fout << "# generate pipetrace, i.e., <fname|stdout|stderr> <range>" << endl;
   fout << "# -ptrace                    <null> " << endl;
   fout << "" << endl;
   fout << "# instruction fetch queue size (in insts)" << endl;
   fout << "-fetch:ifqsize                    4 " << endl;
   fout << "" << endl;
   fout << "# extra branch mis-prediction latency" << endl;
   fout << "-fetch:mplat                      3 " << endl;
   fout << "" << endl;
   fout << "# speed of front-end of machine relative to execution core" << endl;
   fout << "-fetch:speed                      1 " << endl;
   fout << "" << endl;
   fout << "# branch predictor type {nottaken|taken|perfect|bimod|2lev|comb}" << endl;
   fout << "-bpred                        bimod " << endl;
   fout << "" << endl;
   fout << "# bimodal predictor config (<table size>)" << endl;
   fout << "-bpred:bimod           2048 " << endl;
   fout << "" << endl;
   fout << "# 2-level predictor config (<l1size> <l2size> <hist_size> <xor>)" << endl;
   fout << "-bpred:2lev            1 1024 8 0 " << endl;
   fout << "" << endl;
   fout << "# combining predictor config (<meta_table_size>)" << endl;
   fout << "-bpred:comb            1024 " << endl;
   fout << "" << endl;
   fout << "# return address stack size (0 for no return stack)" << endl;
   fout << "-bpred:ras                        8 " << endl;
   fout << "" << endl;
   fout << "# BTB config (<num_sets> <associativity>)" << endl;
   fout << "-bpred:btb             512 4 " << endl;
   fout << "" << endl;
   fout << "# speculative predictors update in {ID|WB} (default non-spec)" << endl;
   fout << "# -bpred:spec_update         <null> " << endl;
   fout << "" << endl;
   fout << "# instruction decode B/W (insts/cycle)" << endl;
   fout << "-decode:width                     4 " << endl;
   fout << "" << endl;
   fout << "# instruction issue B/W (insts/cycle)" << endl;
   fout << "-issue:width                    " << parameters["iwidth"]  << endl;
   fout << "" << endl;
   fout << "# run pipeline with in-order issue" << endl;
   fout << "-issue:inorder                false " << endl;
   fout << "" << endl;
   fout << "# issue instructions down wrong execution paths" << endl;
   fout << "-issue:wrongpath               true " << endl;
   fout << "" << endl;
   fout << "# instruction commit B/W (insts/cycle)" << endl;
   fout << "-commit:width                     4 " << endl;
   fout << "" << endl;
   fout << "# register update unit (RUU) size" << endl;
   fout << "-ruu:size                        16 " << endl;
   fout << "" << endl;
   fout << "# load/store queue (LSQ) size" << endl;
   fout << "-lsq:size                         8 " << endl;
   fout << "" << endl;
   fout << "" << endl;
   fout << "# l1 data cache hit latency (in cycles)" << endl;
   fout << "-cache:dl1lat                     1 " << endl;
   fout << "" << endl;
   fout << "# l2 data cache config, i.e., {<config>|none}" << endl;
   fout << "-cache:dl2             ul2" << ":" << parameters["l2cs"] << ":" << parameters["l2bs"] << ":" << parameters["l2w"] << ":l " << endl;
   fout << "" << endl;
   fout << "# l2 data cache hit latency (in cycles)" << endl;
   fout << "-cache:dl2lat                     6 " << endl;
   fout << "" << endl;
   fout << "" << endl;
   fout << "# l1 instruction cache hit latency (in cycles)" << endl;
   fout << "-cache:il1lat                     1 " << endl;
   fout << "" << endl;
   fout << "# l2 instruction cache config, i.e., {<config>|dl2|none}" << endl;
   fout << "-cache:il2                      dl2 " << endl;
   fout << "" << endl;
   fout << "# l2 instruction cache hit latency (in cycles)" << endl;
   fout << "-cache:il2lat                     6 " << endl;
   fout << "" << endl;
   fout << "# flush caches on system calls" << endl;
   fout << "-cache:flush                  false " << endl;
   fout << "" << endl;
   fout << "# convert 64-bit inst addresses to 32-bit inst equivalents" << endl;
   fout << "-cache:icompress              false " << endl;
   fout << "" << endl;
   fout << "# memory access latency (<first_chunk> <inter_chunk>)" << endl;
   fout << "-mem:lat               18 2 " << endl;
   fout << "" << endl;
   fout << "# memory access bus width (in bytes)" << endl;
   fout << "-mem:width                        8 " << endl;
   fout << "" << endl;
   fout << "# instruction TLB config, i.e., {<config>|none}" << endl;
   fout << "-tlb:itlb              itlb:16:4096:4:l " << endl;
   fout << "" << endl;
   fout << "# data TLB config, i.e., {<config>|none}" << endl;
   fout << "-tlb:dtlb              dtlb:32:4096:4:l " << endl;
   fout << "" << endl;
   fout << "# inst/data TLB miss latency (in cycles)" << endl;
   fout << "-tlb:lat                         30 " << endl;
   fout << "" << endl;
   fout << "# total number of integer ALU's available" << endl;
   fout << "-res:ialu                         " << parameters["ialus"]  << endl;
   fout << "" << endl;
   fout << "# total number of integer multiplier/dividers available" << endl;
   fout << "-res:imult                        " << parameters["imults"] << endl;
   fout << "" << endl;
   fout << "# total number of memory system ports available (to CPU)" << endl;
   fout << "-res:memport                      2 " << endl;
   fout << "" << endl;
   fout << "# total number of floating point ALU's available" << endl;
   fout << "-res:fpalu                        " << parameters["fpalus"] << endl;
   fout << "" << endl;
   fout << "# total number of floating point multiplier/dividers available" << endl;
   fout << "-res:fpmult                       " << parameters["fpmults"]  << endl;
   fout << "" << endl;
   fout << "# profile stat(s) against text addr's (mult uses ok)" << endl;
   fout << "# -pcstat                    <null> " << endl;
   fout << "" << endl;
   fout << "# operate in backward-compatible bugs mode (for testing only)" << endl;
   fout << "-bugcompat                    false " << endl;
   fout << "" << endl;
   fout << "-cache:dl1               dl1:"<< parameters["dcs"] << ":" << parameters["dbs"] << ":" << parameters["dw"] <<":l"
<< endl;

   fout << "-cache:il1          il1:"<< parameters["ics"] << ":" << parameters["ibs"] << ":"<< parameters["iw"] << ":l" << endl;
   fout.close();
}


bool read_wattch_report_file(st_point *point, int scenario, st_vector *metrics, st_vector *statistics, st_env *e)
{
   string str;
   char line[150];
   double num=0.0,il1_mr=0.0,dl1_mr=0.0,ul2_mr;
   double enrg=0.0, delay=0;
   line[0]='\0';
   FILE *file = fopen(("./results"+st_get_unique_string_identifier()+".txt").c_str(),"r");
    if(!file)
    {
        return false;
    }
   bool found1=false;
   bool found2=false;
   while(fgets(line, sizeof(line), file))
   {
      std::istringstream sin(line);
      if (sin >> str >> ws >> num) 
      {
         if (!str.compare("sim_cycle")) 
	     {
            delay = num;
            num = 0.0;
            found1=true;
         }
	 if (!str.compare("total_power")) 
	 {
            enrg = num;
            num=0.0;
            found2=true;
         }
         else if (!str.compare("il1.miss_rate")) 
	 {
            il1_mr = num;
            num=0.0;
         }
         else if (!str.compare("dl1.miss_rate")) 
	 {
            dl1_mr = num;
            num=0.0;
         }
         else if (!str.compare("ul2.miss_rate")) 
	 {
            ul2_mr = num;
            num=0.0;
         }
      }
   }
   if(!found1 || !found2)
   {
       return false;
   }

   fclose(file);
   metrics->insert(0+3*scenario,st_double(enrg));
   metrics->insert(1+3*scenario,st_double(delay));
   metrics->insert(2+3*scenario,const_compute_area(*point,e));
   statistics->insert(0+3*scenario,st_double(il1_mr));
   statistics->insert(1+3*scenario,st_double(dl1_mr));
   statistics->insert(2+3*scenario,st_double(ul2_mr));
   return true;
}

st_double const_compute_area(const st_point &p, st_env *e)
{
   int c;
   st_point cpoint(p);
   double A;

   st_object const * metrics;
   st_object const *dsp_obj;
   st_object const *ds_obj;
   string name,str_value;

   e->shell_variables.get("ds_parameters", dsp_obj);
   e->shell_variables.get("design_space", ds_obj);

   st_vector const * dsp_v = to<st_vector const *>(dsp_obj);
   st_vector const * ds_v = to<st_vector const *>(ds_obj);

   /**
    * Gianluca, what the hell are you doing here..?!
    */

   for(int j=0; j<dsp_v->size(); j++)
   {
       st_string const & parameter = to<st_string const &>(dsp_v->get(j));
       if(!parameter.get_string().compare("iwidth"))
       {
           continue;
       }
       st_vector const & par_value = to<st_vector const &>(ds_v->get(j));
       st_string const & sym_value = to<st_string const &>(par_value.get(p[j]));
       str_value = sym_value.get_string();
       cpoint.insert(j,get_int_from_string(str_value));
   }

   A=0E0;
   A=compute_area(cpoint);
   return st_double(A);
}


double compute_area(st_point & current_point){
  int size=0, bsize=0, assoc=0;
  int nalus=0, nmults=0;
  float ADL1=0.0 ,AIL1=0.0 , AL2=0.0, AIEU=0.0, AFEU=0.0;
  const float Fcache=3.56E-5;
  const float Feu=2.21E-4;

  size=current_point[0];
  bsize=current_point[1];
  assoc=current_point[2];
  AIL1=0.3*Fcache*size/bsize*(34+8*bsize-log(((double) (size/assoc)))/log(2.0)); // Level 1 Instruction cache area
  
  size=current_point[3];
  bsize=current_point[4];
  assoc=current_point[5];
  ADL1=0.3*Fcache*size/bsize*(34+8*bsize-log(((double) (size/assoc)))/log(2.0)); // Level 1 Data Cache area
  
  size=current_point[6];
  bsize=current_point[7];
  assoc=current_point[8];
  AL2=0.3*Fcache*size/bsize*(34+8*bsize-log(((double) (size/assoc)))/log(2.0)); // Level 2 Cache area
  
  nalus=current_point[10];
  nmults=current_point[12];
  AIEU=(800*nalus+12300*nmults)*Feu; // Integer Unit area
  
  nalus=current_point[11];
  nmults=current_point[13];
  AFEU=(900*nalus+12500*nmults)*Feu; // Floating Point Unit area
  return(ADL1+AIL1+AL2+AIEU+AFEU+71.08);
}

string st_wattch_driver::get_name()
{
	return "Wattch driver";		
}

extern "C" 
{
    st_driver *drv_generate_driver()
    {
        prs_display_message("creating the wattch driver");
        return new st_wattch_driver();
    }
}
