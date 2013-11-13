/**
 ** @file debug.c
 ** Generic debug facility
 ** 
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 ** 
 ** @version 0.1.0
 ** @ingroup util
 ** 
 ** @date  Started on: Tue Feb  3 16:59:30 2004
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef ENABLE_DEBUGLOG

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#include "debuglog.h"

static debug_facility_t debug_facility_g[] = {
  { "core", "Orchids core module", DEF_SEV_LVL },
  { "olc", "Orchids Language Compiler module", DEF_SEV_LVL },
  { "ovm", "Orchids Virtual Machine module", DEF_SEV_LVL },
  { "eng", "Orchids engine module", DEF_SEV_LVL },
  { "mod", "Orchids Input module", DEF_SEV_LVL },
  { NULL, NULL, 0 }
};

static char *debug_severity_g[] = {
  "fatal",
  "alert",
  "crit",
  "error",
  "warning",
  "notice",
  "info",
  "debug",
  "trace",
  "notset",
  "unknown",
  NULL
};

#ifdef ORCHIDS_DEBUG
static libdebug_config_t libdebug_config_g = { NULL, 1 };
#else
static libdebug_config_t libdebug_config_g = { NULL, 0 };
#endif

void
libdebug_openlog(char *ident, const char *file, int options)
{
  FILE *fd;
  time_t now;
  char asc_time[32];

  if (options & DLO_NEWLOG)
    fd = fopen(file, "w");
  else
    fd = fopen(file, "a");
  if (fd == NULL) {
    fprintf(stderr, "fopen(%s): %s\n", file, strerror(errno));
    exit(EXIT_FAILURE);
  }
  now = time(NULL);
  strftime(asc_time, 32, "%a %b %d %H:%M:%S %Y", localtime(&now));
  fprintf(fd, "------ Debuglog opened by %s[%i] (%s) ------\n",
          ident, getpid(), asc_time);
  fflush(fd);
  if (options & DLO_LINEBUFF)
    setvbuf(fd, (char *)NULL, _IOLBF, 0);
  libdebug_config_g.logfile = fd;
}

void
libdebug_set_level(const int fac, const int sevlevel)
{
  debug_facility_g[ fac ].level = sevlevel;
}

void
libdebug_log(int fac, int sev, const char *file,
             int line, const char *format, ...)
{
  va_list ap;

  va_start(ap, format);
  libdebug_log_va(fac, sev, file, line, format, ap);
  va_end(ap);
}

void
libdebug_log_va(int fac, int sev, const char *file,
		int line, const char *format, va_list ap)
{
  if ( sev <= debug_facility_g[ fac ].level ) {
    if ( libdebug_config_g.log_to_stderr) {
      va_list aq;

      va_copy(aq,ap);
      fprintf(stderr, "%16s:%-4d:%-4s:%-7s: ",
              file, line, debug_facility_g[ fac ].name,
              debug_severity_g[ sev ] );
      vfprintf(stderr, format, aq);
      va_end(aq);
    }

    if (libdebug_config_g.logfile) {
      va_list aq;

      va_copy(aq,ap);
      fprintf(libdebug_config_g.logfile, "%16s:%-4d:%-4s:%-7s: ",
              file, line, debug_facility_g[ fac ].name,
              debug_severity_g[ sev ] );
      vfprintf(libdebug_config_g.logfile, format, aq);
      va_end(aq);
    }
  }
  va_end(ap);
}

void
libdebug_log_func(const int fac, const int sev, const char *file,
                  const int line, log_func_t func, void *data)
{
  if ( sev <= debug_facility_g[ fac ].level ) {
    if ( libdebug_config_g.log_to_stderr) {
      fprintf(stderr, "%16s:%-4d:%-4s:%-7s: --- BEGIN MULTILINE ---\n",
              file, line, debug_facility_g[ fac ].name,
              debug_severity_g[ sev ] );
      if (func)
        func(stderr, data);
      fprintf(stderr, "%16s:%-4d:%-4s:%-7s: --- END MULTILINE ---\n",
              file, line, debug_facility_g[ fac ].name,
              debug_severity_g[ sev ] );
    }

    if (libdebug_config_g.logfile) {
      fprintf(libdebug_config_g.logfile, "%16s:%-4d:%-4s:%-7s: --- BEGIN MULTILINE ---\n",
              file, line, debug_facility_g[ fac ].name,
              debug_severity_g[ sev ] );
      if (func)
        func(libdebug_config_g.logfile, data);
      fprintf(libdebug_config_g.logfile, "%16s:%-4d:%-4s:%-7s: --- END MULTILINE ---\n",
              file, line, debug_facility_g[ fac ].name,
              debug_severity_g[ sev ] );
    }
  }
}


void
libdebug_show_info(FILE *fp)
{
  int i;

  fprintf(fp, "Present modules are:\n");
  fprintf(fp, "   all | All modules\n");
  for (i = 0; debug_facility_g[i].name; i++) {
    fprintf(fp, "  %4s | %s\n", debug_facility_g[i].name, debug_facility_g[i].description);
  }
  fprintf(fp, "\n");
  fprintf(fp, "Possible severities are:\n");
  for (i = 0; debug_severity_g[i]; i++) {
    fprintf(fp, "  %2i | %s\n", i, debug_severity_g[i]);
  }
}

static int
get_fac_id(const char *name, const size_t sz)
{
  int i;
  char fac[32];

  if (sz == strlen("all") && !strncmp(name, "all", sz))
    return (DF_ALL);

  i = 0;
  while (debug_facility_g[i].name
         && ((strlen(debug_facility_g[i].name) != sz)
             || strncmp(name, debug_facility_g[i].name, sz)))
    i++;
  if (debug_facility_g[i].name)
    return (i);

  strncpy(fac, name, sz);
  if (sz > sizeof (fac))
    fac[ sizeof(fac) - 1] = '\0';
  else
    fac[ sz ] = '\0';
  fprintf(stderr, "error: facility \"%s\" doesn't exist.\n", fac);
  libdebug_show_info(stderr);
  exit(EXIT_FAILURE);

  return (-1);
}

static int
get_sev_id(const char *name, const size_t sz)
{
  int i;
  char sev[32];

  if (sz == strlen("none") && !strncmp(name, "none", sz))
    return (DS_NONE);

  if (sz == strlen("all") && !strncmp(name, "all", sz))
    return (DS_ALL);

  i = 0;
  while (debug_severity_g[i]
         && ((strlen(debug_severity_g[i]) != sz)
             || strncmp(name, debug_severity_g[i], sz)))
    i++;
  if (debug_severity_g[i])
    return (i);

  strncpy(sev, name, sz);
  if (sz > sizeof (sev))
    sev[ sizeof(sev) - 1] = '\0';
  else
    sev[ sz ] = '\0';
  fprintf(stderr, "error: severity \"%s\" doesn't exist\n", sev);
  libdebug_show_info(stderr);
  exit(EXIT_FAILURE);

  return (-1);
}

void
debuglog_enable_all(void)
{
  int i;

  for (i = 0; debug_facility_g[i].name; i++) {
    debug_facility_g[i].level = DS_ALL;
  }
}

void
libdebug_setopt(char *optstr)
{
  char *pos;
  char *tok;
  size_t str_sz;
  int fac, sev, i;

  pos = optstr;
  str_sz = strlen(optstr);
  for (;;) {
    tok = pos;
    while (*pos >= 'a' && *pos <= 'z')
      pos++;
    fac = get_fac_id(tok, pos - tok);

    if (*pos++ != ':') {
      fprintf(stderr, "syntax error on char %td (missing ':')\n", pos - optstr);
      libdebug_show_info(stderr);
      exit(EXIT_FAILURE);
    }

    tok = pos;
    while (*pos >= 'a' && *pos <= 'z')
      pos++;
    sev = get_sev_id(tok, pos - tok);

    if (fac == DF_ALL) {
      for (i = 0; i < DF_MAX; i++)
        debug_facility_g[ i ].level = sev;
    }
    else {
      debug_facility_g[ fac ].level = sev;
    }

    if (pos == (optstr + str_sz)) {
      return ;
    } else if (*pos++ != ',') {
      fprintf(stderr, "syntax error on char %td (missing ',')\n", pos - optstr);
      libdebug_show_info(stderr);
      exit(EXIT_FAILURE);
    }
  }
}




#if 0
int
testlog(FILE *fp, void *data)
{
  int i;

  for (i=0; i<10; i++) {
    fprintf(fp, "blah blah %i\n", i);
    fprintf(fp, "blohbloh %i\n", i*(int)data);
  }

  return (0);
}

int
main(int argc, char *argv[])
{
  debug_facility_g[0].level = DS_ALL;

  libdebug_setopt("core:trace,olc:deug,ovm:notice");

  libdebug_openlog("debug", "test.log", DLO_NEWLOG|DLO_LINEBUFF);
  
  DebugLog(DF_CORE, DS_TRACE, "blah blah %i\n", 123);

  DebugLogFunc(DF_OVM, DS_WARN, testlog, (void *) 12345);

  return (0);
}
#endif

#endif /* ENABLE_DEBUGLOG */

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
