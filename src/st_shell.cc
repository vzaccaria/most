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
#include "config.h"
#include <iostream>
#include <libgen.h>
#include "st_arg.h"
#include "st_commands.h"
#include "st_common_utils.h"
#include "st_env.h"
#include "st_error.h"
#include "st_mpi_utils.h"
#include "st_parser.h"
#include "st_signal.h"
#include <string.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <libxml/parser.h>
#include <libxml/xmlschemas.h>
#include <libxml/xpath.h>

extern int interrupt_exploration;
extern bool mpi_verbose;
extern bool display_time;
extern bool silent_codi;
extern bool never_fail;
extern int exploring;
string local_build_path;
extern void yyinit();

/* Generate textual files for documentation */
int st_generate_manual();

FILE *static_log;

void st_graceful_exit(int ec);

bool st_has_cwd_write_rights() {
  FILE *check = fopen("./check", "w");
  if (!check)
    return false;
  else
    return true;
}

#if !defined(__MAC_OSX__)
/**
 * Reads the directory of the executable
 */
string st_get_build_path(char *base_stshell_name) {
  int retsz;
  char complete_path[300];
  retsz = readlink("/proc/self/exe", complete_path, 300);
  if (retsz <= 0) {
    cout << "Unable to access the /proc filesystem" << endl;
    return ".";
  }
  complete_path[retsz] = '\0';

  // cout << "Using complete path: " << complete_path << endl;

  dirname(complete_path);
  dirname(complete_path);
  string final_path = complete_path;
  /* cout << "Build path: " << final_path << endl; */
  return final_path;
}

#else

extern bool shell_command(string command);
/**
 * Under mac we have to play a little bit more.
 * Reads the directory of the executable.
 */
string st_get_build_path(char *base_stshell_name) {
  int retstatus;
  string command = "type -p ";
  FILE *file;

  char complete_path[100];
  char *internal_path;
  command = command + base_stshell_name;
  command = command + " > ._most_tmp_file";

  // cout << "Executing: " << command << endl;

  if (!shell_command(command)) {
    cout << "Cannot state the complete path of most" << endl;
    st_graceful_exit(EXIT_ERR_NO_ERR);
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

void st_print_version() {
  cout << "Multi-Objective System Tuner - Version " << __STSHELL_RELEASE_NAME__;
  if (string(__STSHELL_CODENAME__) != "")
    cout << " - Internal codename : " << __STSHELL_CODENAME__ << endl;
  else
    cout << endl;
}

void st_print_welcome_screen() {
  cout << "\n";
  cout << "      __  _______  ___________ \n";
  cout << "     /  |/  / __ \\/ ___/_  __/ \n";
  cout << "    / /|_/ / / / /\\__ \\ / /    \n";
  cout << "   / /  / / /_/ /___/ // /    \n";
  cout << "  /_/  /_/\\____//____//_/     \n";

  cout << " \n";
  cout << "  Multi-Objective System Tuner - Version "
       << __STSHELL_RELEASE_NAME__;
  /*
  if(string(__STSHELL_CODENAME__) != "")
     cout << " - Internal codename : " << __STSHELL_CODENAME__ << endl;
  else
  */
  cout << "  - Build date: " << __STSHELL_DATE__
       << " - Git hash: " << __STSHELL_VERSION__ << endl;
  cout << "  Developed at Politecnico di Milano - 2001-2022" << endl;
  cout << "  Send bug reports to vittorio.zaccaria@polimi.it" << endl;
  cout << "  --\n\n";
}

#include "st_rand.h"

void st_setup(int argc, char **argv) {
  local_build_path = st_get_build_path(argv[0]);
  current_environment.shell_variables.insert("current_build_path",
                                             st_string(local_build_path));
  st_init_parse_command();
  rnd_init();
}

arg_parser current_arg_parser;

extern bool st_disconnect_from_node_manager(st_env *env);
extern bool st_connect_to_node_manager(st_env *env);

void st_print_mpi_information() {
  /* cout << "MPI Nodes available: " << st_mpi_get_number_of_nodes() << endl; */
}

bool bypass_license_node_manager = false;

void st_graceful_exit(int ec) {
  if (!bypass_license_node_manager)
    st_disconnect_from_node_manager(&current_environment);

  if (st_mpi_environment_active()) {
    st_mpi_broadcast_send_command("QUIT");
    st_mpi_end();
  }
  exit(ec);
}

long int st_get_hw_addr();
bool st_verify_script(string &file_name, string &key_name);

int main(int argc, char **argv) {
  bool interactive = false;
  st_mpi_initialize();
  if (!st_initialize_regular_expressions()) {
    prs_display_error(
        "Problems with internal specifications. Please, contact the vendor.");
    exit(EXIT_ERR_GENERIC);
  }
  if (!st_has_cwd_write_rights()) {
    prs_display_error("The current directory should be writable");
    exit(EXIT_ERR_GENERIC);
  }
  /* Initializie XML */

  xmlInitParser();

  if (st_mpi_get_node() == 0) {
    current_arg_parser.prog_name = "most";
    current_arg_parser.parameter_name = "";
    current_arg_parser.bug_report =
        "Report bugs to: <vittorio.zaccaria@gmail.com>, "
        "<gpalermo@elet.polimi.it>";

    current_arg_parser.insert("help", 'H', NULL, "this help", OPTION_SIMPLE);
    current_arg_parser.insert("open_script_file", 'f', "FILE",
                              "specify source script", OPTION_MULTIPLE_ARG);
    current_arg_parser.insert(
        "force_interactive", 'i', NULL,
        "go into interactive mode after reading the script", OPTION_SIMPLE);
    current_arg_parser.insert("include_packages", 'p', NULL,
                              "include available packages", OPTION_SIMPLE);
    current_arg_parser.insert(
        "pipe_server", 's', NULL,
        "launch in pipe server mode, opens a pipe named most_input",
        OPTION_SIMPLE);
    current_arg_parser.insert(
        "time", 'T', NULL, "display time and day information", OPTION_SIMPLE);
    current_arg_parser.insert("silent", 'S', NULL,
                              "silent optimizer information display",
                              OPTION_SIMPLE);
    current_arg_parser.insert("never_fail", 'n', NULL,
                              "Does not exit when in non-interactive mode and "
                              "a command returns an error.",
                              OPTION_SIMPLE);
    current_arg_parser.insert("input_xml_file", 'x', "FILE",
                              "use the XML design space definition in FILE",
                              OPTION_MULTIPLE_ARG);
    current_arg_parser.insert("version", 'v', NULL, "display version",
                              OPTION_SIMPLE);
    current_arg_parser.insert("generate_manual", 'g', NULL,
                              "development only, generate manual",
                              OPTION_SIMPLE);
    current_arg_parser.insert(
        "key", 'k', "FILE",
        "demo mode only; bypass license check with signature",
        OPTION_MULTIPLE_ARG);
    current_arg_parser.insert("independent", 'j', "NAME",
                              "create independent session named NAME. Copies "
                              "recursively current dir above and runs MOST.",
                              OPTION_MULTIPLE_ARG);

    st_setup(argc, argv);

    if (!current_arg_parser.process_options(argc, argv)) {
      current_arg_parser.print_short_help();
      st_graceful_exit(EXIT_ERR_GENERIC);
    }

    if (current_arg_parser.option_is_set("independent")) {
      set<string>::iterator i;
      i = current_arg_parser.options["independent"].values.begin();
      string name = *i;

      if (!set_session(name)) {
        prs_display_error("Can't create session '" + name + "'");
        st_graceful_exit(EXIT_ERR_GENERIC);
      }
    }

    if (current_arg_parser.option_is_set("key") &&
        current_arg_parser.option_is_set("open_script_file")) {
      set<string>::iterator i;
      i = current_arg_parser.options["open_script_file"]
              .values.begin(); /* Take only one script */
      string file_name = *i;

      i = current_arg_parser.options["key"]
              .values.begin(); /* Take only one key */
      string key_name = *i;

      if (!st_verify_script(file_name, key_name)) {
        prs_display_error(
            "Signed script execution requested. Signature provided not valid.");
        st_graceful_exit(EXIT_ERR_GENERIC);
      }
      bypass_license_node_manager = true;
    } else {
      if (!st_connect_to_node_manager(&current_environment)) {
        prs_display_error("Can't connect to the Node Manager");
        st_graceful_exit(EXIT_ERR_GENERIC);
      }
    }

    if (!st_setup_signal_handler()) {
      prs_display_error("Can't initialize signal handler");
      st_graceful_exit(EXIT_ERR_GENERIC);
    }

    if (current_arg_parser.option_is_set("generate_manual")) {
      st_generate_manual();
      st_graceful_exit(EXIT_ERR_NO_ERR);
    }

    if (current_arg_parser.option_is_set("help")) {
      current_arg_parser.print_help();
      st_graceful_exit(EXIT_ERR_NO_ERR);
    }

    if (current_arg_parser.option_is_set("time")) {
      display_time = true;
    }

    if (current_arg_parser.option_is_set("silent")) {
      silent_codi = true;
    }

    if (current_arg_parser.option_is_set("never_fail")) {
      never_fail = true;
    }

    if (current_arg_parser.option_is_set("version")) {
      st_print_version();
      st_graceful_exit(EXIT_ERR_NO_ERR);
    }

    bool go_interactive = true;

    /** Initialize the parser here */
    st_ast_initialize_ast();

    if (!current_arg_parser.option_is_set("open_script_file") ||
        current_arg_parser.option_is_set("force_interactive"))
      st_print_welcome_screen();

    if (current_arg_parser.option_is_set("pipe_server")) {
      int res = mkfifo("most_input", S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
      if ((res == -1) && (errno != EEXIST)) {
        perror("Error:");
        prs_command_quit();
      }
      if (!prs_parse_and_execute_file("most_input")) {
        prs_display_message("Falied opening input fifo");
        prs_command_quit();
      }
      /** FIXME: a handler to the control-C key should be inserted here
       * to shutdown most. */
      go_interactive = false;
    }

    if (current_arg_parser.option_is_set("include_packages")) {
      prs_parse_and_execute_file(
          (local_build_path + "/packages/basic.pack").c_str());
      prs_parse_and_execute_file(
          (local_build_path + "/packages/statistic.pack").c_str());
      prs_parse_and_execute_file(
          (local_build_path + "/packages/functional.pack").c_str());
      prs_parse_and_execute_file(
          (local_build_path + "/packages/report.pack").c_str());
    }

    prs_parse_and_execute_file(".mostrc");

    if (current_arg_parser.option_is_set("input_xml_file")) {
      set<string>::iterator i;
      i = current_arg_parser.options["input_xml_file"].values.begin();

      st_string xml_file_name(*i);

      current_environment.shell_variables.insert("xml_design_space_file",
                                                 xml_file_name);
      if (!prs_command_activate_driver("st_xml")) {
        prs_command_quit();
      }
    }

    st_print_mpi_information();

    if (current_arg_parser.option_is_set("open_script_file")) {
      set<string>::iterator i;
      for (i = current_arg_parser.options["open_script_file"].values.begin();
           i != current_arg_parser.options["open_script_file"].values.end();
           i++) {
        /* prs_display_message("Reading commands from file " + *i); */
        if (!prs_parse_and_execute_file(i->c_str())) {
          prs_display_error("Unable to open file '" + (*i) + "'");
          prs_command_quit();
        }
      }
      static_log = NULL;
      if (!current_arg_parser.option_is_set("force_interactive")) {
        go_interactive = false;
      }
    }
    if (go_interactive) {
      static_log = fopen("./.most_log.scr", "w+");
      string comm = "# This is an automatically generated log of the previous "
                    "interactive session of MOST\n";
      fwrite(comm.c_str(), strlen(comm.c_str()), 1, static_log);
      current_environment.shell_variables.set_integer("progress", 1);
      prs_initialize_readline();
      prs_go_interactive();
    }
    prs_command_quit();
  } else {

    /** Run in server mode */
    prs_display_message("Starting MOST in server mode, server " +
                        st_mpi_get_string_representation_of_node());
    current_environment.shell_variables.insert(
        "current_build_path", st_string(st_get_build_path(argv[0])));
    st_job_receiver job_server;
    job_server.run(&current_environment);
  }
}
