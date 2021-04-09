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

#include "config.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#if !defined(__COMMAND_LIST_H__)

using namespace std;

typedef enum st_command_type {
  no_arg,
  simple_arg,
  multiple_opts,
  special_arg
} st_command_type;

#define BOLD "\\fB"
#define PREV "\\fP"
#define ITAL "\\fI"

extern bool shell_command(string command);
extern bool shell_command(string command, bool);

extern string local_build_path;

extern vector<string> st_rsm_get_available_rsms();

typedef class st_command {
public:
  st_command_type type;
  string command_help;
  string short_help;
  string return_value;

  bool help_on_file;
  string help_file_name;

  string command_arg_name;
  vector<string> command_options_name;
  vector<string> command_options_formal_name;
  vector<string> command_options_help;

  string alt_command_name; /* this is used for creating an alternative title for
                              RSM */
  string alt_command_synopsys; /* this is used for an alternative synopsys */
  string alt_command_type;     /* can be 'doe' or 'optimizer' */

  string substitute(string s, string local, bool color = true) {
    int p = s.find("#t");
    while (p != string::npos) {
      if (color)
        s.insert(p + 2, ITAL "true" PREV);
      else
        s.insert(p + 2, "true");

      s.erase(p, 2);
      p = s.find("#t");
    }

    p = s.find("#f");
    while (p != string::npos) {
      if (color)
        s.insert(p + 2, ITAL "false" PREV);
      else
        s.insert(p + 2, "false");
      s.erase(p, 2);
      p = s.find("#f");
    }

    p = s.find("@");
    while (p != string::npos) {
      if (color)
        s.insert(p + 1, ITAL + command_arg_name + PREV);
      else
        s.insert(p + 1, command_arg_name);
      s.erase(p, 1);
      p = s.find("@");
    }
    p = s.find("!");
    while (p != string::npos) {
      if (color)
        s.insert(p + 1, ITAL + local + PREV);
      else
        s.insert(p + 1, local);
      s.erase(p, 1);
      p = s.find("!");
    }

    {
      string patt = "^arg(";
      int ln = patt.size();
      p = s.find(patt);
      while (p != string::npos) {
        int n = s.find(")", p + 1);
        // cout << "pos ) = " << n << ", where p is " << p << endl;

        if (n == string::npos)
          break;

        string num = s.substr(p + ln, n - (p + ln));
        int i = atoi(num.c_str());

        s.erase(n, 1);
        s.erase(p, ln + num.size());
        // cout << num << " " << num.size() << endl;

        if (color)
          s.insert(p, ITAL + command_options_formal_name[i] + PREV);
        else
          s.insert(p, command_options_formal_name[i]);

        p = s.find(patt, p + 1);
      }
    }
    {
      string patt = "^opt(";
      int ln = patt.size();
      p = s.find(patt);
      while (p != string::npos) {
        int n = s.find(")", p + 1);
        // cout << "pos ) = " << n << ", where p is " << p << endl;

        if (n == string::npos)
          break;

        string num = s.substr(p + ln, n - (p + ln));
        int i = atoi(num.c_str());

        s.erase(n, 1);
        s.erase(p, ln + num.size());
        // cout << num << " " << num.size() << endl;

        if (color)
          s.insert(p, ITAL + command_options_name[i] + PREV);
        else
          s.insert(p, command_options_name[i]);

        p = s.find(patt, p + 1);
      }
    }
    return s;
  }

  void construct(st_command_type i_type, const char *i_command_help,
                 const char *i_command_arg_name,
                 const char **i_command_options_name,
                 const char **i_command_options_help) {
    type = i_type;
    command_help = i_command_help;
    if (!(type == no_arg) && i_command_arg_name)
      command_arg_name = i_command_arg_name;

    if (i_command_options_name) {
      int i = 0;
      while (i_command_options_name[i]) {
        string opt = i_command_options_name[i];
        int p = opt.find("=");
        if ((p == string::npos) && (p != 0)) {
          command_options_name.push_back(opt);
          command_options_formal_name.push_back(opt);
        } else {
          string o_n = opt.substr(0, p);
          string o_fn = opt.substr(p + 1, opt.size());
          command_options_name.push_back(o_n);
          command_options_formal_name.push_back(o_fn);
        }
        i++;
      }
    }
    if (i_command_options_help) {
      int i = 0;
      while (i_command_options_help[i]) {
        command_options_help.push_back(string(i_command_options_help[i]));
        i++;
      }
    }
  }
  st_command(const char *i_short_help, const char *i_long_help) {
    short_help = i_short_help;
    construct(special_arg, i_long_help, NULL, NULL, NULL);
  }

  st_command(st_command_type i_type, const char *i_command_help,
             const char *i_command_arg_name,
             const char **i_command_options_name,
             const char **i_command_options_help, string i_short_help,
             string i_return_value) {
    short_help = i_short_help;
    return_value = i_return_value;
    construct(i_type, i_command_help, i_command_arg_name,
              i_command_options_name, i_command_options_help);
  }
  st_command(st_command_type i_type, const char *i_command_help,
             const char *i_command_arg_name,
             const char **i_command_options_name,
             const char **i_command_options_help, string i_short_help) {
    short_help = i_short_help;
    construct(i_type, i_command_help, i_command_arg_name,
              i_command_options_name, i_command_options_help);
  }

  st_command(st_command_type i_type, const char *i_command_help,
             const char *i_command_arg_name,
             const char **i_command_options_name,
             const char **i_command_options_help) {
    construct(i_type, i_command_help, i_command_arg_name,
              i_command_options_name, i_command_options_help);
  }
  bool gen_short_man(string cmd) {
    string help = substitute(short_help, "", false);
    printf("%-30s %-20s  \n", cmd.c_str(), help.c_str());
    return true;
  }

  bool gen_man(string cmd) { return gen_man(cmd, false, ""); }

  bool gen_man(string cmd, bool gen_text) {
    return gen_man(cmd, gen_text, "manual.txt");
  }

  bool gen_man(string cmd, bool gen_text, string text_name) {
    string file_name = "tmp.man";
    ofstream file_out(file_name.c_str(), ios::out);

    if (file_out.fail())
      return false;

    if (type != special_arg) {
      file_out << ".TH " << cmd << " 1 \" MOST Shell - " << __STSHELL_DATE__
               << "\"" << endl;
      file_out << ".SH NAME" << endl;

      if (alt_command_name != "")
        file_out << alt_command_name;
      else
        file_out << cmd;

      if (short_help != "")
        file_out << " \\- " << short_help << endl;
      else
        file_out << endl;
      file_out << ".SH SYNOPSYS" << endl;

      string the_command;

      if (alt_command_synopsys != "")
        the_command = alt_command_synopsys;
      else
        the_command = cmd;

      if (alt_command_type == "opt" || alt_command_type == "doe") {
        file_out << the_command.c_str() << endl;
      } else {
        switch (type) {
        case no_arg:
          file_out << BOLD << the_command.c_str() << PREV << endl;
          break;

        case simple_arg:
          file_out << BOLD << the_command.c_str() << PREV << " ";
          file_out << ITAL << command_arg_name << PREV << endl;
          break;

        default:
          file_out << BOLD << the_command.c_str() << PREV << " ";
          file_out << ITAL << command_arg_name << PREV << " [ " << ITAL
                   << " options " << PREV << " ] " << endl;
        }
      }
      file_out << ".SH DESCRIPTION" << endl;
      file_out << substitute(command_help, "") << endl;
      if (type == multiple_opts) {
        if (alt_command_type == "opt" || alt_command_type == "doe")
          file_out << ".SS SHELL VARIABLES" << endl;
        else
          file_out << ".SS OPTIONS" << endl;

        int i = 0;
        for (; i < command_options_name.size(); i++) {
          file_out << ".TP" << endl;
          if (alt_command_type == "opt" || alt_command_type == "doe")
            file_out << BOLD << command_options_name[i] << PREV << endl;
          else
            file_out << BOLD << command_options_name[i] << PREV << "=" << ITAL
                     << command_options_formal_name[i] << PREV << endl;
          file_out << substitute(command_options_help[i],
                                 command_options_formal_name[i])
                   << endl;
        }
      }
      if (cmd == "db_train_rsm") {
        file_out << ".SH AVAILABLE MODELS" << endl;
        vector<string> v = st_rsm_get_available_rsms();

        for (int j = 0; j < v.size(); j++) {
          file_out << v[j] << ((j != v.size() - 1) ? "," : "") << endl;
        }
        file_out << endl;
      }
      file_out << ".SH RETURN VALUE" << endl;
      file_out << substitute(return_value, "") << endl;
      file_out.close();
      if (!gen_text)
        shell_command("groff -man -Tascii " + file_name + " | less", true);
      else {
        shell_command(string("cat tmp.man | sed -f ") +
                      __STSHELL_HARDWIRED_BUILD_PATH__ +
                      "/scripts/mmd/mmd.sed >> " + text_name);
      }

    } else {
      file_out << ".TH " << cmd << " 1 \" MOST Shell - " << __STSHELL_DATE__
               << "\"" << endl;
      file_out << ".SH NAME" << endl;

      if (alt_command_name != "")
        file_out << alt_command_name;
      else
        file_out << cmd;

      if (short_help != "")
        file_out << " \\- " << short_help << endl;
      else
        file_out << endl;
      file_out.close();
#if !defined(__ST_GEN_MAN__)
      if (!gen_text) {
        shell_command("cat " + local_build_path + "/man/" + command_help +
                      " >> " + file_name);
        shell_command("groff -man -Tascii " + file_name + " | less", true);
      } else
#endif
      {
        shell_command(string("cat ") + __STSHELL_HARDWIRED_BUILD_PATH__ +
                      "/man/" + command_help + " | sed -f " +
                      __STSHELL_HARDWIRED_BUILD_PATH__ +
                      "/scripts/mmd/mmd.sed >> " + text_name);
      }
    }
    return true;
  }

  void print_help(string cmd) {
    /* This should be removed, I guess VZ */
    if (type == no_arg || type == simple_arg)
      printf("Usage: %s %s \n", cmd.c_str(), command_arg_name.c_str());
    else
      printf("Usage: %s %s [OPTIONS] \n", cmd.c_str(),
             command_arg_name.c_str());
    printf("\n%s \n", command_help.c_str());
    int i = 0;
    if (type == multiple_opts)
      cout << "\nOptions:\n";
    for (; i < command_options_name.size(); i++) {
      printf("    %-30s   %s\n", command_options_name[i].c_str(),
             command_options_help[i].c_str());
    }
  }
  st_command(){};

} st_command;

#define __COMMAND_LIST_H__
#endif

#define STDRET "The return variable $? is set to #t on success, #f otherwise."

#if defined(__NEED_INIT_COMMANDS__)
map<string, st_command> st_commands;

void st_init_commands() {
  st_commands["help"] =
      st_command(simple_arg,
                 "When no arguments are specified, it prints all available "
                 "commands on the shell. Otherwise, "
                 "prints the manual of @ command.",
                 "CMD_NAME", NULL, NULL,
                 "Prints a general help of " __STSHELL_CODENAME__
                 " " __STSHELL_RELEASE_NAME__,
                 STDRET);

  st_commands["quit"] =
      st_command(no_arg, "Quits the current most session.", NULL, NULL, NULL,
                 "Quit the current session.");
  st_commands["exit"] =
      st_command(no_arg, "Quits the current most session.", NULL, NULL, NULL,
                 "Quits the current session.");

  st_commands["read_script"] = st_command(
      simple_arg,
      "Read and executes commands from the script file named @. "
      "It can't be used in " __STSHELL_CODENAME__
      " interactive mode, i.e., it can be used only in scripts executed with "
      "the -f command line of " __STSHELL_CODENAME__ ".",
      "FILE", NULL, NULL, "Reads and executes a script", STDRET);

  st_commands["read_object"] =
      st_command(simple_arg,
                 "Read a data object from file @. The data object should have "
                 "been saved with the write_object command."
                 "The data object is one of the basic types available in "
                 "the " __STSHELL_CODENAME__ " type system.",
                 "FILE", NULL, NULL, "Reads a data object from disk.",
                 "On success, the object is stored in the return variable $?. "
                 "Otherwise, the return variable is set to #f.");

  st_commands["set"] =
      st_command("Sets the value of a shell variable", "special_set.man.src");
  st_commands["set_objective"] = st_command("Sets a minimization objective",
                                            "special_set_objective.man.src");
  st_commands["set_metric"] =
      st_command("Sets a new metric", "special_set_metric.man.src");
  st_commands["set_function"] =
      st_command("Defines a function", "special_set_function.man.src");
  st_commands["set_constraint"] =
      st_command("Sets a constraint", "special_set_constraint.man.src");

  st_commands["echo"] =
      st_command(simple_arg,
                 "Prints the value of expression @. Values are printed "
                 "according to the MOST language syntax.",
                 "EXPR", NULL, NULL, "Prints the value of an expression.");

  const char *rd[] = {"--destination=DB", NULL};
  const char *rd_help[] = {
      "! is the string containing the name of the destination database", NULL};

  st_commands["db_read"] =
      st_command(multiple_opts,
                 "Read database from disk file @. The contents of the file are "
                 "inserted into the database ^arg(0) "
                 "specified with the ^opt(0) option. The database file should "
                 "have been previously written with the "
                 "db_write command. ",
                 "FILE", rd, rd_help, "Read a database from disk", STDRET);

  const char *wt[] = {"--file_name=FILE", NULL};
  const char *wt_help[] = {"! is the name of the destination file", NULL};

  st_commands["db_write"] =
      st_command(multiple_opts,
                 "Write database @ into the disk file ^arg(0) specified with "
                 "the ^opt(0) option. The database is written into a text "
                 "file which can be manually inspected. This may change in "
                 "future versions but backward compatibility will be ensured.",
                 "DB", wt, wt_help, "Write a database on disk.", STDRET);

  st_commands["write_object"] =
      st_command(multiple_opts,
                 "Write the value of expression @ into disk file ^arg(0) "
                 "specified with the ^opt(0) option.",
                 "OBJ", wt, wt_help, "Write an object on disk.", STDRET);

  st_commands["db_create"] = st_command(
      simple_arg,
      "Create a design point database named @ in memory. Note, on-line "
      "databases are volatile; to preserve the data please use the db_write "
      "command.",
      "DB_NAME", NULL, NULL, "Creates a new, empty database", STDRET);

  st_commands["opt_tune"] = st_command(
      simple_arg,
      "Invokes the algorithm specified with the command **opt_load_optimizer** "
      "(and, optionally, the design of experiment specified with "
      "**doe_load_doe**). Each optimization algorithm heuristically tries to "
      "identify the Pareto frontier associated with the minimization of the "
      "objectives. Objectives should be specified with the command "
      "**set_objective** and can be more than 1. The designs associated with "
      "the Pareto set are inserted in database named @. If the database "
      "exists, it will be overwritten. ",
      "DB", NULL, NULL, "Start the optimization process.", STDRET);

  st_commands["opt_estimate"] =
      st_command(simple_arg,
                 "Estimate the metrics of point @ by using the current driver. "
                 "The return value will contain "
                 "the point with the new estimated metrics. ",
                 "P", NULL, NULL, "Estimates a single point",
                 "On success, $? contains the point with the new estimated "
                 "metrics. #f otherwise.");

  st_commands["opt_remove_objective"] =
      st_command(simple_arg,
                 "Remove the objective named @. If the objective is not "
                 "specified, removes all the objectives.",
                 "OBJ", NULL, NULL, "Removes an objective", STDRET);

  st_commands["opt_remove_constraint"] =
      st_command(simple_arg,
                 "Remove the constraint named @. If the constraint is not "
                 "specified, removes all the constraints.",
                 "CONS", NULL, NULL, "Removes a constraint", STDRET);

  st_commands["opt_update_cache"] = st_command(
      no_arg,
      "Updates the content of the constraint and objective cache. This command "
      "should be invoked whenever objectives or constraints change.",
      "", NULL, NULL, "Refreshes the content of the cache", STDRET);

  st_commands["opt_load_optimizer"] = st_command(
      simple_arg,
      "Load the optimizer module named @. The module should be an .so object "
      "located in the search path of " __STSHELL_CODENAME__
      ". The search path is composed by the \\fIlib\\fP directory of the "
      "program distribution package plus the value of the search_path variable "
      "(the latter variable can be a string or a list of strings).",
      "OPT_NAME", NULL, NULL, "Loads the specified optimization module.",
      STDRET);

  st_commands["doe_load_doe"] =
      st_command(simple_arg,
                 "Load the Design Of Experiments module module @. See the "
                 "considerations on the "
                 " search path with 'help opt_load_optimizer'. ",
                 "DOE_NAME", NULL, NULL,
                 "Load the specified optimization module.", STDRET);

  st_commands["drv_load_driver"] = st_command(
      simple_arg,
      "Load the driver module named @. See the considerations on the search "
      "path with 'help opt_load_optimizer'. ",
      "DRV_NAME", NULL, NULL, "Load the specified driver module.", STDRET);
  st_commands["drv_write_xml"] =
      st_command(simple_arg,
                 "Writes the current design space into file @. A design space "
                 "should be instantiated before invoking this command.",
                 "FILENAME", NULL, NULL,
                 "Writes the current design space into a file.", STDRET);

  const char *ref[] = {"--reference=REFERENCE", "--count=BOOL", NULL};
  const char *ref_help[] = {"! is an ideal pareto front database",
                            "Instead of computing pure coverage, forces "
                            "counting how many points of @ are in REFERENCE",
                            NULL};

  st_commands["db_compute_coverage"] = st_command(
      multiple_opts,
      "Compute the coverage of database ^arg(0) over database @. Note that the "
      "objectives should be set before calling this command.",
      "DB", ref, ref_help, "Computes the coverage of two databases.",
      "Returns the coverage as a float number between 0 and 1. Lower is "
      "better.");

  const char *nref[] = {"--reference=REFERENCE", NULL};
  const char *nref_help[] = {"! is an ideal pareto front database", NULL};
  st_commands["db_compute_ADRS"] = st_command(
      multiple_opts,
      "Compute the Average Distance From Reference Set between the database "
      "^arg(0) (ideally an exact Pareto database) and database @. "
      "Note that the objectives should be set before calling this command.",
      "DB", nref, nref_help,
      "Compute the ADRS of a database with respect to a reference.",
      "On success it returns the ADRS, #f otherwise (i.e., whenever the "
      "databases do not exist or REFERENCE is empty).");

  st_commands["db_compute_distance"] = st_command(
      multiple_opts,
      "Compute the average euclidean distance between the ^arg(0) database "
      "(ideally a full Pareto db) and database @. "
      "Note that the objectives should be set before calling this command.",
      "DB", nref, nref_help, "Compute the euclidean distance",
      "On success it returns the euclidean distance, #f otherwise (i.e., "
      "whenever the databases do not exist or REFERENCE is empty).");

  st_commands["db_compute_median_distance"] = st_command(
      multiple_opts,
      "Compute the median distance between the database named ^arg(0) (ideally "
      "a full pareto db) and database @. Note that the objectives should be "
      "set before calling this command.",
      "DB", nref, nref_help, "Computes the median distance",
      "On success it returns the euclidean distance, #f otherwise (i.e., "
      "whenever the databases do not exist or REFERENCE is empty).");

  const char *rsm[] = {"--model=MODEL", "--source=SOURCEDB", NULL};
  const char *rsm_help[] = {"! should be one of the existing models.",
                            "! is the name of the source DB", NULL};
  st_commands["db_train_rsm"] = st_command(
      multiple_opts,
      "Train a RSM by using model ^arg(0) and creates a set of predictions "
      "(over the full space or DoE depending on the model type) "
      " which are put in database @. Additional options depend on the RSM "
      "used. Use \"help db_train_rsm --model=^arg(0)\" for a "
      " detailed help on the selected ^arg(0). ^arg(1) may be used as an "
      "alternative source for training design points.",
      "DEST", rsm, rsm_help, "Trains and RSM", STDRET);

  const char *dst[] = {"--destination=DST", "--attach=BOOL", NULL};
  const char *dst_help[] = {
      "! is an existing destination database. Note that ! will be erased (if "
      "attach not specified).",
      "If #t, the destination database is not erased and SRC is copied in it",
      NULL};

  st_commands["db_copy"] =
      st_command(multiple_opts,
                 "Copy a database @ into the destination database ^arg(0). If "
                 "^opt(1) is set to true, it "
                 "merges @ and ^arg(0) into ^arg(0).",
                 "SRC", dst, dst_help, "Copy or merge a database.", STDRET);

  const char *rep[] = {"--only_size=BOOL", "--only_objectives=BOOL",
                       "--show_cluster=BOOL", NULL};
  const char *rep_help[] = {"Dump only the size if ! is #t",
                            "Dump only the objectives if ! is #t",
                            "Show the cluster number if ! is #t", NULL};

  st_commands["db_report"] = st_command(
      multiple_opts,
      "Reports the contents of database @. For each configuration, it shows "
      "the corresponding metrics and statistics toghether with the associated "
      "objectives. The cluster number for each configuration can be shown by "
      "enabling the corresponding option.",
      "DB", rep, rep_help, "Print out the contents of the database.");

  st_commands["db_clear"] =
      st_command(simple_arg, "Deletes the contents of database @.", "DBNAME",
                 NULL, NULL, "Delete the contents of a database.", STDRET);

  // const char *recenter[]={"--range=N", NULL };
  // const char *recenter_help[]={"! specifies the range around the point to be
  // considered as a design space", NULL };

  // st_commands["drv_recenter_design_space"]  = st_command( multiple_opts,
  // "Recenter design space around @", "POINT", recenter, recenter_help,
  // "Recenter design space", STDRET);

  const char *filter[] = {"--level=N", "--valid=BOOL", NULL};
  const char *filter_help[] = {"N is the level of depth of the pareto curve",
                               "#t if only valid points should be considered",
                               NULL};

  st_commands["db_filter_pareto"] = st_command(
      multiple_opts,
      "Filter database @ for Pareto points. Points which violate the "
      "constraints are still kept in the database if the ^opt(1) option is not "
      "used or #f. Objectives should be previously specified. Pareto ranking "
      "can be specified with the ^opt(0) option.",
      "DB", filter, filter_help, "Filter dominated points.", STDRET);

  const char *norm[] = {"--with=DB", "--valid=BOOL", NULL};
  const char *norm_help[] = {
      "@ is the reference database for computing normalization",
      "#t if only valid points should be retained in the databases", NULL};

  st_commands["db_normalize"] =
      st_command(multiple_opts,
                 "Filters the input databases @ and the database ^arg(0)) in "
                 "order to have the same points.",
                 "DBR", norm, norm_help, "Equalize two databases.", STDRET);

  const char *clus[] = {"--cluster=K", NULL};
  const char *clus_help[] = {"! is the number of clusters that should be "
                             "considered in the filtering action",
                             NULL};
  st_commands["db_filter_cluster"] = st_command(
      simple_arg,
      "Eliminates from @ the points that do not belong to cluster ^arg(0).",
      "DB", clus, clus_help, "Eliminates points not belonging to a cluster.",
      STDRET);

  st_commands["db_filter_valid"] = st_command(
      simple_arg, "Eliminates from database @ the points that have errors.",
      "DB", NULL, NULL, "Eliminates invalid points.", STDRET);

  const char *kmeans[] = {"--clusters=K", "--iterations=N", NULL};
  const char *kmeans_help[] = {
      "! is the number of clusters",
      "! is the number of iterations used by the clustering algorithm", NULL};
  st_commands["db_compute_kmeans_clusters"] =
      st_command(multiple_opts,
                 "Clusters points of database @ based on the objectives "
                 "previously specified.",
                 "DB", kmeans, kmeans_help,
                 "Creates clusters of points within a database", STDRET);

  const char *plot[] = {
      "--xaxis=XMETRIC",   "--yaxis=YMETRIC",  "--zaxis=ZMETRIC",
      "--output=FILENAME", "--clusters=NCLUS", "--color_violating=BOOL",
      "--key=KEY",         "--onepage=BOOL",   "--bubble=BOOL",
      "--plevel=par",      "--override=FILE",  NULL};
  const char *plot_help[] = {
      "XMETRIC is a feasible metric of the design",
      "YMETRIC is a feasible metric of the design",
      "ZMETRIC is a feasible metric of the design. This is optional.",
      "FILENAME is the prefix of the postscript file. Optional",
      "NCLUS is the number of clusters to be displayed",
      "BOOL is true if constraint violators should be highlighted",
      "String indicating the position of the legend",
      "TRUE if plot should be in a single page",
      "TRUE if plotting a bubble plot instead of a 3D plot",
      "highlights different levels of !",
      "Use ! to override default options",
      NULL};

  st_commands["db_plot"] = st_command(
      multiple_opts,
      "Plot a graph of database @ for given metrics. @ can be also a list of "
      "database names",
      "DB", plot, plot_help, "Creates a plot from a database", STDRET);

  const char *plot_v[] = {"--xaxis=xmetric",
                          "--yaxis=ymetric",
                          "--output=filename",
                          "--key=key",
                          "--onepage=cond",
                          "--box=cond",
                          "--yrange=y_axis_range",
                          "--xrange=x_axis_range",
                          "--use_classes=cond",
                          "--override=FILE",
                          NULL};
  const char *plot_v_help[] = {
      "! is the label to be associated to the x-axis",
      "! is the label to be associated to the y-axis",
      "! is the prefix of the postscript file (optional)",
      "! indicates the position of the legend within the plot",
      "! is #t if the plot should be put in a single page",
      "! is #t if the desired plot is box-plot",
      "! is a list of two numbers representing the minimum (the first number) "
      "and the maximum (the second number) value of the y axis",
      "! is a list of two numbers representing the minimum (the first number) "
      "and the maximum (the second number) value of the x axis",
      "! is true if the tics on the x axis should be equispaced over the axis. "
      "This option is available only for box plots.",
      "Use ! to override default options",
      NULL};

  st_commands["db_plot_vector"] = st_command(
      multiple_opts,
      "Plot a scatter graph for @. @ should be composed by a list of [" ITAL
      "v1, v2, name" PREV "] vectors, where " ITAL "v1" PREV
      " contains the x-axis coords, " ITAL "v2" PREV
      " contains the y-axis coords and " ITAL "name" PREV
      " is used as a label for the corresponding (x,y) points. It is possible "
      "to plot a box-plot graph for @, where @ is a list of maps with the "
      "following keys: \"x\", \"sample_min\", \"Q1\", \"Med\", \"Q3\", "
      "\"sample_max\" and \"outliers\". The object related to \"outliers\" "
      "must be a vector of numbers, while the objects related to the remaining "
      "keys must be numbers.",
      "vec", plot_v, plot_v_help,
      "Creates a scatter plot from a given set of vectors or a box-plot from a "
      "list of maps.",
      STDRET);

  st_commands["show_vars"] = st_command(
      no_arg,
      "Shows the state of the current shell, containing the shell variables, "
      "the current objectives and constraints and the databases in memory.",
      NULL, NULL, NULL, "Shows the state of the shell.");

  const char *export_mode[] = {"--mode_frontier=BOOL", "--data_tank=BOOL",
                               "--file_name=NAME", NULL};
  const char *export_mode_help[] = {
      "BOOL is true if exporting for mode_frontier",
      "BOOL is true if exporting to data_tank",
      "NAME is the name of the output file", NULL};

  st_commands["db_export"] =
      st_command(multiple_opts, "Export @ into a csv file named ^arg(2).", "DB",
                 export_mode, export_mode_help, "Export a database", STDRET);
  const char *export_xml_mode[] = {"--objectives=BOOL", "--file_name=NAME",
                                   NULL};
  const char *export_xml_mode_help[] = {
      "BOOL if exporting objectives instead of metrics",
      "NAME is the name of the output file", NULL};

  st_commands["db_export_xml"] = st_command(
      multiple_opts, "Export @ into a csv file named ^arg(1).", "DB",
      export_xml_mode, export_xml_mode_help, "Export a database", STDRET);

  const char *html_mode[] = {"--name=NAME", "--objectives=BOOL", NULL};
  const char *html_mode_help[] = {"Create a report named NAME",
                                  "BOOL should be true if only if the report "
                                  "should contain only objectives",
                                  NULL};

  st_commands["db_report_html"] = st_command(
      multiple_opts,
      "Invokes Multicube Explorer R1.0 to create an HTML report of database "
      "'@' into the current directory. The Multicube Explorer path should be "
      "available in the 'multicube_explorer_path' shell variable. The current "
      "driver should be the XML driver.",
      "DB", html_mode, html_mode_help,
      "Creates an HTML report of the specified database", STDRET);

  const char *import_mode[] = {"--file_name=NAME", "--use_symbols=BOOL", NULL};
  const char *import_mode_help[] = {
      "NAME is the name of the file to import",
      "BOOL is true if data in the .csv file represents symbols", NULL};
  st_commands["db_import"] =
      st_command(multiple_opts, "Import a .csv file named ^arg(0) into @.",
                 "DB", import_mode, import_mode_help,
                 "Import a .csv file into a database", STDRET);

  st_commands["db_compute_corr"] = st_command(
      simple_arg, "Compute correlation for database @.", "DB", NULL, NULL,
      "Compute a correlation between parameters and metrics", STDRET);
}
#else
extern map<string, st_command> st_commands;
extern void st_init_commands();

#endif
