/**
 ** @file main.c
 ** Main entry point for Orchids.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 ** @author Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
 **
 ** @version 0.2
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
#include <errno.h>

#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <pwd.h>
#include <sys/types.h> /* for stat() */
#include <limits.h> /* for PATH_MAX */
#ifndef PATH_MAX
#define PATH_MAX 8192
/* PATH_MAX is undefined on systems without a limit of filename length,
   such as GNU/Hurd.  Also, defining _XOPEN_SOURCE on Linux will make
   PATH_MAX undefined.
*/
#endif

#include "orchids.h"

#include "hash.h"
#include "evt_mgr.h"
#include "orchids_api.h"
#include "rule_compiler.h"

#include "main_priv.h"

sig_atomic_t signo_g = 0;

static void sig_chld(int signo, siginfo_t *si, void *uctxt)
{
  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;
}

static void sig_catchall(int signo, siginfo_t *si, void *uctxt)
{
  signo_g = signo;
}

static void install_signals(void)
{
  struct sigaction sa;

  memset(&sa, 0, sizeof(struct sigaction));
  sigemptyset(&sa.sa_mask);
  sa.sa_sigaction = sig_chld;
  sa.sa_flags = SA_SIGINFO;
  /* We are not using the SA_RESTART flag here, although that
     would be a good thing.  The issue is that some systems do
     not have it.  Instead, we have to handle restarting
     system calls that set errno to EINTR ourselves.  According to
     'man sigaction' on Mac OS X 10.9.2, those
     system calls are:
     open(), read(), write(), sendto(), recvfrom(), sendmsg(), recvmsg(),
     wait(), ioctl()
     By experience, there is also select().
     There should also be fopen(), fdopen().
  */
  if (sigaction(SIGCHLD, &sa, NULL) < 0)
    {
    errlab:
      perror("sigaction()");
      exit(EXIT_FAILURE);
    }
  sa.sa_sigaction = sig_catchall;
  if (sigaction(SIGHUP, &sa, NULL) < 0) goto errlab;
  if (sigaction(SIGINT, &sa, NULL) < 0) goto errlab;
  if (sigaction(SIGQUIT, &sa, NULL) < 0) goto errlab;
  /* SIGILL should do the default action: core dump */
  if (sigaction(SIGTRAP, &sa, NULL) < 0) goto errlab;
  /* SIGABRT should do the default action: core dump */
  /* SIGEMT should do the default action: core dump */
  /* SIGFPE should do the default action: core dump */
  /* SIGKILL should do the default action: terminate process
     (cannot be caught anyway) */
  /* SIGBUS should do the default action: core dump */
  /* SIGSEGV should do the default action: core dump */
  /* SIGSYS should do the default action: core dump */
  if (sigaction(SIGALRM, &sa, NULL) < 0) goto errlab;
  if (sigaction(SIGTERM, &sa, NULL) < 0) goto errlab;
  /* SIGURG should do the default action: discard signal */
  /* SIGSTOP should do the default action: stop process
     (cannot be caught anyway) */
  if (sigaction(SIGTSTP, &sa, NULL) < 0) goto errlab;
  /* SIGCONT should do the default action: discard signal */
  if (sigaction(SIGTTIN, &sa, NULL) < 0) goto errlab;
  if (sigaction(SIGTTOU, &sa, NULL) < 0) goto errlab;
  /* SIGIO should do the default action: discard signal */
  if (sigaction(SIGXCPU, &sa, NULL) < 0) goto errlab;
  if (sigaction(SIGXFSZ, &sa, NULL) < 0) goto errlab;
  if (sigaction(SIGVTALRM, &sa, NULL) < 0) goto errlab;
  if (sigaction(SIGPROF, &sa, NULL) < 0) goto errlab;
  /* SIGWINCH should do the default action: discard signal */
  /* SIGINFO should do the default action: discard signal */
  if (sigaction(SIGUSR1, &sa, NULL) < 0) goto errlab;
  if (sigaction(SIGUSR2, &sa, NULL) < 0) goto errlab;
#ifdef SIGSTKFLT
  /* SIGSTKFLT should do the default action (terminate?) */
#endif
#ifdef SIGPWR
  /* SIGPWR should do the default action (discard signal?) */
#endif
#ifdef SIGSYS
  /* SIGSYS should do the default action (terminate?) */
#endif
  sa.sa_handler = SIG_IGN;
  sa.sa_flags = 0; /* unset SA_SIGINFO to install SIG_IGN. */
  /* SIGPIPE should not do the default action (core dump),
   but be ignored */
  if (sigaction(SIGUSR2, &sa, NULL) < 0) goto errlab;
}

static void change_run_id(char *username)
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

static int orchids_periodic_save (orchids_t *ctx, heap_entry_t *he)
{
  timeval_t next;
  
  (void) orchids_save (ctx, ctx->save_file);
  gettimeofday(&he->date, NULL);
  Timer_Add (&next, &he->date, &ctx->save_interval);
  he->date = next;
  register_rtaction (ctx, he);
  return 0;
}

/**
 * Main function.
 * Prepare execution context, initialise default values,
 * parse command line options, proceed the configuration file,
 * compile rules and enter in the main loop.
 * @param argc argument counter
 * @param argv argument vector
 **/
int main(int argc, char *argv[])
{
  orchids_t *ctx;
  int err;
  char *orchidslog;

  ctx = new_orchids_context(argv[0]);
  orchidslog = adjust_path (ctx, REL_LOCALSTATEDIR "/orchids/log/orchids.log");
#ifdef ORCHIDS_DEBUG
  debuglog_enable_all(); /* enable ALL messages in debug mode */
  libdebug_openlog("orchids", orchidslog, DLO_NEWLOG | DLO_LINEBUFF);
#else
  libdebug_setopt("all:notice,ovm:none");
  libdebug_openlog("orchids", orchidslog, DLO_LINEBUFF);
#endif /* ORCHIDS_DEBUG */
  gc_base_free (orchidslog);
  DebugLog(DF_CORE, DS_NOTICE, "ORCHIDS -- starting\n");

  parse_cmdline(ctx, argc, argv);
  if (ctx->verbose)
    {
      fputs (ORCHIDS_BANNER,stdout);
      fflush (stdout);
    }

  install_signals();
  if (ctx->daemon)
    {
      char *out, *err;

      out = adjust_path (ctx, REL_LOCALSTATEDIR "/orchids/log/orchids.stdout");
      err = adjust_path (ctx, REL_LOCALSTATEDIR "/orchids/log/orchids.stderr");
      daemonize(out, err);
      gc_base_free (out);
      gc_base_free (err);
    }

  proceed_pre_config(ctx);

  orchids_lock(ctx->lockfile);

  proceed_post_config(ctx);
  compile_rules(ctx);
  proceed_post_compil(ctx);
  if (ctx->rule_compiler->nerrors)
    {
      fprintf (stderr, "That makes %d error%s: stop.\n",
	       ctx->rule_compiler->nerrors,
	       (ctx->rule_compiler->nerrors==1)?"":"s");
      fflush (stderr);
      exit (10);
    }

  /* change run id here */
  if (ctx->runtime_user)
    change_run_id(ctx->runtime_user);

  if (ctx->save_file!=NULL)
    {
      err = orchids_restore (ctx, ctx->save_file);
      if (err==0)
	{
	  DebugLog(DF_CORE, DS_NOTICE, "Orchids restored from '%s'\n", ctx->save_file);
	}
      else if (err==ENOENT) /* No such save file: silently proceed */
	{
	  DebugLog(DF_CORE, DS_WARN, "Orchids won't restart from save file: file '%s' does not exist.\n",
		   ctx->save_file);
	}
      else
	{
	  DebugLog(DF_CORE, DS_ERROR,
		   "Orchids can't restart from save file '%s': %s\n",
		   ctx->save_file, orchids_strerror(err));
	}
    }
  if (ctx->save_file!=NULL)
    (void) register_rtcallback(ctx, orchids_periodic_save, NULL, NULL,
			       ctx->save_interval.tv_sec, 0);

  if (ctx->off_line_mode == MODE_ONLINE)
    event_dispatcher_main_loop(ctx);

  return (0);
}

static void parse_cmdline(orchids_t *ctx, int argc, char **argv)
{
  char opt;

  while ((opt = getopt(argc, argv, "hc:o:b:f:r:d:Dv:m")) != -1) {
    switch (opt) {

    case 'h':
      orchids_usage(argv[0]);
      exit(EXIT_FAILURE);
      break;

    case 'c':
      if (ctx->config_file!=NULL)
	{
          fprintf (stderr, "-c: configuration file already defined to '%s'\n",
                   ctx->config_file);
          exit(EXIT_FAILURE);
        }
      ctx->config_file = adjust_path(ctx, optarg);
      break;

    case 'o':
      DebugLog(DF_CORE, DS_NOTICE, "requesting off-line mode\n");
      if (!strcmp("syslog", optarg))
        ctx->off_line_mode = MODE_SYSLOG;
      else if (!strcmp("snare", optarg))
        ctx->off_line_mode = MODE_SNARE;
      else {
        fprintf (stderr, "-o: unknown off-line mode '%s'\n", optarg);
        exit(EXIT_FAILURE);
      }
      break;

    case 'r':
      DebugLog(DF_CORE, DS_NOTICE, "adding rule file '%s'\n", optarg);
      ctx->rulefile_list = gc_base_malloc(ctx->gc_ctx, sizeof (rulefile_t));
      ctx->last_rulefile = ctx->rulefile_list;
      ctx->rulefile_list->next = NULL;
      ctx->rulefile_list->name = gc_strdup(ctx->gc_ctx, optarg);
      if (ctx->off_line_mode == MODE_ONLINE)
        ctx->off_line_mode = MODE_SYSLOG;
      break;

    case 'f':
      do {
        char filepath[PATH_MAX];

        if (ctx->off_line_input_file!=NULL)
	  {
	    fprintf (stderr, "-f: offline input file already defined to '%s'\n",
		     ctx->off_line_input_file);
	    exit(EXIT_FAILURE);
	  }

        if (orchids_realpath (ctx, optarg, filepath)==NULL)
	  {
	    fprintf (stderr, "-f cannot expand path '%s'.\n", optarg);
	    exit (EXIT_FAILURE);
	  }
        DebugLog(DF_CORE, DS_NOTICE, "set input file to %s\n", filepath);
        ctx->off_line_input_file = gc_strdup(ctx->gc_ctx, filepath);

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

    case 'v':
      sscanf (optarg, "%i", &ctx->verbose);
      break;

    case 'm':
      ctx->actmon = 1;
      break;

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
  fprintf(stderr, "  -v <level>         verbose mode\n");
  fflush(stderr);
}


static void daemonize(const char *stdout_file, const char *stderr_file)
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
 again_stdout:
  stdout_fd = open(stdout_file,
                   O_CREAT | O_RDWR,
                   S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
  if (errno==EINTR) goto again_stdout;
  dup2(stdout_fd, STDOUT_FILENO);
  close(stdout_fd);

  close(STDERR_FILENO);
 again_stderr:
  stderr_fd = open(stderr_file,
                   O_CREAT | O_RDWR,
                   S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
  if (errno==EINTR) goto again_stderr;
  dup2(stderr_fd, STDERR_FILENO);
  close(stderr_fd);
}


/*
** Copyright (c) 2002-2005 by Julien OLIVAIN, Laboratoire Spécification
** et Vérification (LSV), CNRS UMR 8643 & ENS Cachan.
** Copyright (c) 2014-2015 by Jean GOUBAULT-LARRECQ, Laboratoire Spécification
** et Vérification (LSV), CNRS UMR 8643 & ENS Cachan.
**
** Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
** Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
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
