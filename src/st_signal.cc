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
#include <config.h>
#include <iostream>
#include <libgen.h>
#include <pthread.h>
#include <signal.h>
#include <st_arg.h>
#include <st_commands.h>
#include <st_env.h>
#include <st_mpi_utils.h>
#include <st_parser.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

extern void st_graceful_exit(int ec);

static sigset_t signal_mask;

void *st_abort_handler(void *) {
  int sig_caught; /* signal caught       */
  int rc;         /* returned code       */

  for (;;) {
    rc = sigwait(&signal_mask, &sig_caught);
    if (rc != 0) {
      /* what do we do here ? */
      continue;
    }
    switch (sig_caught) {
    case SIGINT:
    case SIGTERM:
    case SIGKILL:
    case SIGSTOP:
    case SIGQUIT:
      prs_display_message("Signal received, trying to release the license");
      st_graceful_exit(1);
      break; /* Do we really need it here? */
    default: /* should normally not happen */
      fprintf(stderr, "\nUnexpected signal %d\n", sig_caught);
      break;
    }
  }
}

bool st_setup_signal_handler() {
  pthread_t sig_thr_id; /* signal handler thread ID */
  int rc;               /* return code              */

  sigemptyset(&signal_mask);
  sigaddset(&signal_mask, SIGINT);
  sigaddset(&signal_mask, SIGTERM);
  sigaddset(&signal_mask, SIGKILL);
  sigaddset(&signal_mask, SIGSTOP);
  sigaddset(&signal_mask, SIGQUIT);

  rc = pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);
  if (rc != 0) {
    return false;
  }
  rc = pthread_create(&sig_thr_id, NULL, st_abort_handler, NULL);
  if (rc != 0) {
    return false;
  }
  /* any newly created threads inherit the signal mask */

  return true;
}
