/**
 ** @file main.c
 ** Main entry point for Orchids.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup core
 **
 ** @date  Started on: Mon Jan 13 10:05:09 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <pwd.h>
#include <sys/types.h> /* for stat() */
#include <limits.h> /* for PATH_MAX */

#include "orchids.h"

#include "hash.h"
#include "evt_mgr.h"
#include "orchids_api.h"
#include "rule_compiler.h"

#include "main_priv.h"


static void
sig_chld(int signo)
{
  while ( waitpid(-1, NULL, WNOHANG) > 0 )
    ;
}


static void
change_run_id(char *username)
{
  struct passwd *pwent;
  int ret;

  pwent = getpwnam(username);
  if (pwent == NULL) {
    fprintf(stderr, "error: user '%s' doesn't exist.\n", username);
    exit(EXIT_FAILURE);
  }

  DebugLog(DF_CORE, DS_NOTICE,
           "Changing RunID to '%s' (uid=%i gid=%i)\n",
           username, pwent->pw_uid, pwent->pw_gid);

  ret = setgid(pwent->pw_gid);
  if (ret != 0) {
    perror("setgid()");
    exit(EXIT_FAILURE);
  }

  ret = setuid(pwent->pw_uid);
  if (ret != 0) {
    perror("setuid()");
    exit(EXIT_FAILURE);
  }
}


/**
 * Main function.
 * Prepare execution context, initialise default values,
 * parse command line options, proceed the configuration file,
 * compile rules and enter in the main loop.
 * @param argc argument counter
 * @param argv argument vector
 **/
int
main(int argc, char *argv[])
{
  orchids_t *ctx;

#ifdef ORCHIDS_DEBUG
  debuglog_enable_all(); /* enable ALL messages in debug mode */
  libdebug_openlog("orchids",
                   LOCALSTATEDIR "/orchids/log/orchids.log",
                   DLO_NEWLOG | DLO_LINEBUFF);
#else
  libdebug_setopt("all:notice,ovm:none");
  libdebug_openlog("orchids",
                   LOCALSTATEDIR "/orchids/log/orchids.log",
                   DLO_LINEBUFF);
#endif /* ORCHIDS_DEBUG */

  DebugLog(DF_CORE, DS_NOTICE, "ORCHIDS -- starting\n");

#ifdef ORCHIDS_DEMO
  printf("%s", ORCHIDS_BANNER);
#endif

  Xsignal(SIGCHLD, sig_chld);

  ctx = new_orchids_context();
  parse_cmdline(ctx, argc, argv);

  if (ctx->daemon)
    daemonize(LOCALSTATEDIR "/orchids/log/orchids.stdout",
              LOCALSTATEDIR "/orchids/log/orchids.stderr");

  proceed_pre_config(ctx);

  orchids_lock(ctx->lockfile);

  proceed_post_config(ctx);
  compile_rules(ctx);
  proceed_post_compil(ctx);

  /* change run id here */
  if (ctx->runtime_user)
    change_run_id(ctx->runtime_user);

  if (ctx->off_line_mode == MODE_ONLINE)
    event_dispatcher_main_loop(ctx);

  return (0);
}

static void
parse_cmdline(orchids_t *ctx, int argc, char **argv)
{
  char opt;

  while ((opt = getopt(argc, argv, "hc:o:b:f:r:d:D")) != -1) {
    switch (opt) {

    case 'h':
      orchids_usage(argv[0]);
      exit(EXIT_FAILURE);
      break;

    case 'c':
      ctx->config_file = strdup(optarg);
      break;

    case 'o':
      DebugLog(DF_CORE, DS_NOTICE, "Requesting off-line mode.\n");
      if (!strcmp("syslog", optarg))
        ctx->off_line_mode = MODE_SYSLOG;
      else if (!strcmp("snare", optarg))
        ctx->off_line_mode = MODE_SNARE;
      else {
        DebugLog(DF_CORE, DS_FATAL, "unknown off-line mode '%s'\n", optarg);
        exit(EXIT_FAILURE);
      }
      break;

    case 'r':
      DebugLog(DF_CORE, DS_NOTICE, "adding the rule file '%s'\n", optarg);
      ctx->rulefile_list = Xzmalloc(sizeof (rulefile_t));
      ctx->last_rulefile = ctx->rulefile_list;
      ctx->rulefile_list->name = strdup(optarg);
      if (ctx->off_line_mode == MODE_ONLINE)
        ctx->off_line_mode = MODE_SYSLOG;
      break;

    case 'f':
      do {
        char filepath[PATH_MAX];

        if (ctx->off_line_input_file) {
          DebugLog(DF_CORE, DS_FATAL,
                   "Offline input file already defined to [%s]\n",
                   ctx->off_line_input_file);
          exit(EXIT_FAILURE);
        }

        Xrealpath(optarg, filepath);
        DebugLog(DF_CORE, DS_NOTICE, "set input file to %s\n", filepath);
        ctx->off_line_input_file = strdup(filepath);

        if (ctx->off_line_mode == MODE_ONLINE)
          ctx->off_line_mode = MODE_SYSLOG;

      } while (0);
      break;

    case 'D':
      ctx->daemon = 1;
      break;

#ifdef ENABLE_DEBUGLOG
    case 'd':
      libdebug_setopt(optarg);
      break;
#endif /* ENABLE_DEBUGLOG */


    default:
      orchids_usage(argv[0]);
      exit(EXIT_FAILURE);
    }
  }
}

static void
orchids_usage(char *prg)
{
  fprintf(stderr, "orchids usage :\n");
  fprintf(stderr, "%s <options>\n", prg);
  fprintf(stderr, "  -h			print this help\n");
  fprintf(stderr, "  -D		        Daemon mode\n");
  fprintf(stderr, "  -c <config_file>   configuration file\n");
  fprintf(stderr, "  -o <off-line_mode> off-line input file type (default syslog)\n");
  fprintf(stderr, "  -f <input_file>    input file\n");
  fprintf(stderr, "  -r <rule_file>     rule file\n");
}


static void
daemonize(const char *stdout_file, const char *stderr_file)
{
  pid_t pid;
  int stdout_fd;
  int stderr_fd;
  const char *devnull_str = "/dev/null";

  if (stdout_file == NULL)
    stdout_file = devnull_str;

  if (stderr_file == NULL)
    stderr_file = devnull_str;

  if ( !strcmp(stdout_file, stderr_file) &&
        strcmp("/dev/null", stdout_file)) {
    fprintf(stderr,
            "WARNING, log may be corrupted if buffered I/O are used.\n");
    /* XXX: activate line buffer ?? */
    /* setline(stdout); */
    /* or, more portable */
    /* setvbuf(stdout, (char *)NULL, _IOLBF, 0); */
  }

  if (getppid() != 1) {
    pid = fork();

    if (pid > 0) /* parent */
      exit(EXIT_SUCCESS);

    if (pid < 0) {
      perror("fork");
      exit(EXIT_FAILURE);
    }
    setsid();
  }

  close(STDOUT_FILENO);
  stdout_fd = open(stdout_file,
                   O_CREAT | O_RDWR,
                   S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
  dup2(stdout_fd, STDOUT_FILENO);
  close(stdout_fd);

  close(STDERR_FILENO);
  stderr_fd = open(stderr_file,
                   O_CREAT | O_RDWR,
                   S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
  dup2(stderr_fd, STDERR_FILENO);
  close(stderr_fd);
}


/*
** Copyright (c) 2002-2005 by Julien OLIVAIN, Laboratoire Spécification
** et Vérification (LSV), CNRS UMR 8643 & ENS Cachan.
**
** Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
**
** This software is a computer program whose purpose is to detect intrusions
** in a computer network.
**
** This software is governed by the CeCILL license under French law and
** abiding by the rules of distribution of free software.  You can use,
** modify and/or redistribute the software under the terms of the CeCILL
** license as circulated by CEA, CNRS and INRIA at the following URL
** "http://www.cecill.info".
**
** As a counterpart to the access to the source code and rights to copy,
** modify and redistribute granted by the license, users are provided
** only with a limited warranty and the software's author, the holder of
** the economic rights, and the successive licensors have only limited
** liability.
**
** In this respect, the user's attention is drawn to the risks associated
** with loading, using, modifying and/or developing or reproducing the
** software by the user in light of its specific status of free software,
** that may mean that it is complicated to manipulate, and that also
** therefore means that it is reserved for developers and experienced
** professionals having in-depth computer knowledge. Users are therefore
** encouraged to load and test the software's suitability as regards
** their requirements in conditions enabling the security of their
** systems and/or data to be ensured and, more generally, to use and
** operate it in the same conditions as regards security.
**
** The fact that you are presently reading this means that you have had
** knowledge of the CeCILL license and that you accept its terms.
*/

/* End-of-file */
