/**
 ** @file debuglog.h
 ** 
 ** 
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 ** 
 ** @version 0.1.0
 ** @ingroup util
 ** 
 ** @date  Started on: Tue Feb  3 17:07:44 2004
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef DEBUGLOG_H
#define DEBUGLOG_H

#ifdef ENABLE_DEBUGLOG
#include <stdarg.h>

#define DEBUGLOG_BASENAME 1

#ifdef DEBUGLOG_BASENAME
#include <libgen.h>
#endif /* DEBUGLOG_BASENAME */

#ifndef DEBUGLOG_MINSEV
#define DEBUGLOG_MINSEV 0
#endif

/* Debug facilities (modules) */
#define DF_CORE 0
#define DF_OLC  1
#define DF_OVM  2
#define DF_ENG  3
#define DF_MOD  4
#define DF_MAX  5
#define DF_ALL  DF_MAX

/* Debug severities */
#define DS_NONE    -1
#define DS_FATAL    0
#define DS_ALERT    1
#define DS_CRIT     2
#define DS_ERROR    3
#define DS_WARN     4
#define DS_NOTICE   5
#define DS_INFO     6
#define DS_DEBUG    7
#define DS_TRACE    8
#define DS_NOTSET   9
#define DS_UNKNOWN 10
#define DS_ALL     10

#define DEF_SEV_LVL DS_NONE

/* DebugLogfile open Options */
#define DLO_APPEND 0
#define DLO_NEWLOG 1

#define DLO_FULLBUFF 0
#define DLO_LINEBUFF 2

typedef struct debug_facility_s debug_facility_t;
struct debug_facility_s
{
  char *name;
  char *description;
  int level;
};

typedef struct libdebug_config_s libdebug_config_t;
struct libdebug_config_s
{
  FILE *logfile;
  int log_to_stderr;
};

typedef void (*log_func_t)(FILE *fp, void *data);

void libdebug_openlog(char *ident, const char *file, int options);
void libdebug_set_level(int fac, int sevlevel);
void libdebug_log(int fac, int sev, const char *file,
                  int line, const char *format, ...);
void libdebug_log_va(int fac, int sev, const char *file,
		     int line, const char *format, va_list ap);
void libdebug_log_func(int fac, int sev, const char *file,
                       int line, log_func_t func, void *data);
void libdebug_show_info(FILE *fp);
void debuglog_enable_all(void);
void libdebug_setopt(char *optstr);

#if DEBUGLOG_BASENAME

#define DebugLog(fac, sev, ...) \
  libdebug_log(fac, sev, basename(__FILE__), __LINE__, __VA_ARGS__) \

#define DebugLogVa(fac, sev, format, ap)			    \
  libdebug_log_va(fac, sev, basename(__FILE__), __LINE__, format, ap)  \

#define DebugLogFunc(fac, sev, func, data) \
  libdebug_log_func(fac, sev, basename(__FILE__), __LINE__, func, data)

#else /* DEBUGLOG_BASENAME */

#define DebugLog(fac, sev, ...) \
  libdebug_log(fac, sev, __FILE__, __LINE__, __VA_ARGS__) \

#define DebugLogVa(fac, sev, format, ap)			    \
  libdebug_log_va(fac, sev, __FILE__, __LINE__, format, ap)  \


#define DebugLogFunc(fac, sev, func, data) \
  libdebug_log_func(fac, sev, __FILE__, __LINE__, func, data)

#endif /* DEBUGLOG_BASENAME */

#else /* ENABLE_DEBUGLOG */

#define DebugLog(fac, sev, ...)
#define DebugLogFunc(fac, sev, func, data) 

#endif /* ENABLE_DEBUGLOG */

#endif /* DEBUGLOG_H */


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
