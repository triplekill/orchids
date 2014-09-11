/**
 ** @file mod_syslog.c
 ** Listen to syslog event on a udp socket.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 ** @date  Started on: Wed Jan 15 17:21:55 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <ctype.h>


#include "orchids.h"

#include "orchids_api.h"

#include "mod_syslog.h"

input_module_t mod_syslog;

/*
** priority = facility * 8 + severity
** (extracted from rfc3164)
*/
static char *syslog_severity_g[] = {
  "(0) Emergency: system is unusable"           ,
  "(1) Alert: action must be taken immediately" ,
  "(2) Critical: critical conditions"           ,
  "(3) Error: error conditions"                 ,
  "(4) Warning: warning conditions"             ,
  "(5) Notice: normal but significant condition",
  "(6) Informational: informational messages"   ,
  "(7) Debug: debug-level messages"             ,
  NULL
};

static char *syslog_facility_g[] = {
  "(0) kernel messages"                          ,
  "(1) user-level messages"                      ,
  "(2) mail system"                              ,
  "(3) system daemons"                           ,
  "(4) security/authorization messages"          ,
  "(5) messages generated internally by syslogd" ,
  "(6) line printer subsystem"                   ,
  "(7) network news subsystem"                   ,
  "(8) UUCP subsystem"                           ,
  "(9) clock daemon (and cron/at)"               ,
  "(10) security/authorization messages"         ,
  "(11) FTP daemon"                              ,
  "(12) NTP subsystem"                           ,
  "(13) log audit "                              ,
  "(14) log alert "                              ,
  "(15) clock daemon"                            ,
  "(16) local use 0  (local0)"                   ,
  "(17) local use 1  (local1)"                   ,
  "(18) local use 2  (local2)"                   ,
  "(19) local use 3  (local3)"                   ,
  "(20) local use 4  (local4)"                   ,
  "(21) local use 5  (local5)"                   ,
  "(22) local use 6  (local6)"                   ,
  "(23) local use 7  (local7)"                   ,
  NULL
};


int generic_dissect(orchids_t *ctx, mod_entry_t *mod, event_t *event, void *data)
{
  return dissect_syslog(ctx, mod, event, data);
}


static int dissect_syslog(orchids_t *ctx, mod_entry_t *mod, event_t *event,
			  void *data)
{
  struct tm *t;
  char *txt_line;
  int txt_len;
  size_t token_size;
  long syslog_priority;
  long n;
  int ret;
  ovm_var_t *val;
  gc_t *gc_ctx = ctx->gc_ctx;

  DebugLog(DF_MOD, DS_DEBUG, "syslog_dissector()\n");

  if (event->value==NULL)
    {
      DebugLog(DF_MOD, DS_DEBUG, "NULL event value\n");
      return -1;
    }
  switch (TYPE(event->value))
    {
    case T_STR: txt_line = STR(event->value); txt_len = STRLEN(event->value);
      break;
    case T_VSTR: txt_line = VSTR(event->value); txt_len = VSTRLEN(event->value);
      break;
    default:
      DebugLog(DF_MOD, DS_DEBUG, "event value not a string\n");
      return -1;
    }

  GC_START(gc_ctx, SYSLOG_FIELDS+1);
  GC_UPDATE(gc_ctx, SYSLOG_FIELDS, event);
  ret = 0;

  /* check if we have a raw syslog line (with encoded facility and priotity) */
  if (txt_line[0] == '<')
    {
      token_size = get_next_int(txt_line + 1, &syslog_priority, txt_len);
      if (syslog_priority > 191)
	{ /* 23 (local7) * 8 + 7 (debug) == 191 */
	  DebugLog(DF_MOD, DS_WARN, "PRI error.\n");
	  ret = 1;
	  goto end;
	}
      val = ovm_vstr_new (gc_ctx, NULL);
      VSTR(val) = syslog_facility_g[syslog_priority >> 3];
      VSTRLEN(val)= strlen(VSTR(val));
      GC_UPDATE(gc_ctx, F_FACILITY, val);

      val = ovm_vstr_new (gc_ctx, NULL);
      VSTR(val) = syslog_severity_g[syslog_priority & 0x07];
      VSTRLEN(val)= strlen(VSTR(val));
      GC_UPDATE(gc_ctx, F_SEVERITY, val);

      txt_line += token_size + 2; /* number size and '<' '>' */
      txt_len -= token_size + 2;
    }

  if ((txt_len > 15) &&
      (txt_line[3] == ' ') && (txt_line[6] == ' ') &&
      (txt_line[9] == ':') && (txt_line[12] == ':') &&
      (txt_line[15] == ' '))
    {
      t = syslog_getdate(txt_line);
      if (t == NULL)
	{
	  DebugLog(DF_MOD, DS_WARN, "time format error.\n");
	  ret = 1;
	  goto end;
	}
      val = ovm_ctime_new(gc_ctx, mktime(t));
      GC_UPDATE(gc_ctx, F_TIME, val);

      txt_line += 16;
      txt_len -= 16;
    }
  else
    {
      DebugLog(DF_MOD, DS_INFO, "no date present.\n");
    }

  if (txt_len <= 0)
    {
      DebugLog(DF_MOD, DS_WARN, "syntax error. ignoring event\n");
      ret = 1;
      goto end;
    }

  token_size = my_strspn(txt_line, " [:", txt_len);
#ifdef NO_GCONFD_HACK
  if (txt_line[token_size] == ' ') {
#else
  if (txt_line[token_size] == ' ' && txt_line[token_size+1] != '(') {
#endif
    DebugLog(DF_MOD, DS_DEBUG, "read host.\n");

    val = ovm_vstr_new (gc_ctx, event->value);
    VSTR(val) = txt_line;
    VSTRLEN(val) = token_size;
    GC_UPDATE(gc_ctx, F_HOST, val);

    ++token_size; /* skip separator */
    txt_line += token_size;
    txt_len -= token_size;
    /* XXX check sizes are always positive */
    if (txt_len <= 0)
      {
	DebugLog(DF_MOD, DS_WARN, "syntax error. ignoring event\n");
	ret = 1;
	goto end;
      }
  }
  else
    {
      DebugLog(DF_MOD, DS_DEBUG, "read src retry.\n");
    }

  /* Handle syslog message repetition */
  if (!strncmp("last message repeated ", txt_line, 22))
    {
      token_size = my_strspn(txt_line, "\r\n", txt_len);

      val = ovm_vstr_new (gc_ctx, event->value);
      VSTR(val) = txt_line;
      VSTRLEN(val) = token_size;
      GC_UPDATE(gc_ctx, F_MSG, val);

      txt_line += 22;
      txt_len -= 22;

      token_size = get_next_int(txt_line, &n, txt_len);
      token_size++;
      val = ovm_int_new (gc_ctx, n);
      GC_UPDATE(gc_ctx, F_REPEAT, val);

      txt_line += token_size;
      txt_len -= token_size;

      val = ovm_str_new (gc_ctx, 6);
      strcpy(STR(val), "syslog");
      GC_UPDATE(gc_ctx, F_PROG, val);

      REGISTER_EVENTS(ctx, mod, SYSLOG_FIELDS);
      goto end;
    }

  token_size = my_strspn(txt_line, "[:", txt_len);

  val = ovm_vstr_new (gc_ctx, event->value);
  VSTR(val) = txt_line;
#ifndef NO_GCONFD_HACK
  if (!strncmp("gconfd", txt_line, 6)) {
    VSTRLEN(val) = 6;
  }
  else {
#endif
    VSTRLEN(val) = token_size;
#ifndef NO_GCONFD_HACK
  }
#endif
  GC_UPDATE(gc_ctx, F_PROG, val);

  txt_line += token_size;
  txt_len -= token_size;

  /* XXX check sizes are always positives */
  if (txt_len <= 0)
    {
      DebugLog(DF_MOD, DS_WARN, "syntax error. ignoring event\n");
      ret = 1;
      goto end;
    }

  /* look ahead */
  if (*txt_line == '[')
    { /* fill pid */
      token_size = get_next_int(txt_line + 1, &n, txt_len);
      val = ovm_int_new (gc_ctx, n);
      GC_UPDATE(gc_ctx, F_PID, val);

      if (strncmp(txt_line + token_size + 1, "]: ", 3))
	{
	  DebugLog(DF_MOD, DS_WARN, "syntax error.\n");
	  /* XXX: Drop event ??? */
	}
      txt_line += token_size + 4; /* 1 for "[" and 3 for "]: " */
      txt_len -= token_size + 4;
    }
  else if (*txt_line == ':')
    { /* Skip ": " */
      txt_line += 2;
      txt_len -= 2;
    }

  if (txt_len <= 0)
    {
      DebugLog(DF_MOD, DS_WARN, "syntax error. ignoring event\n");
      ret = 1;
      goto end;
    }

  /* remaining string is the syslog message */
  val = ovm_vstr_new (gc_ctx, event->value);
  token_size = my_strspn(txt_line, "\r\n", txt_len);
  VSTR(val) = txt_line;
  VSTRLEN(val) = token_size;
  GC_UPDATE(gc_ctx, F_MSG, val);

  REGISTER_EVENTS(ctx, mod, SYSLOG_FIELDS);

  end:
  GC_END(gc_ctx);
  return ret;
}

field_t syslog_fields[] = {
  { "syslog.facility", T_VSTR,  "source of the message"          },
  { "syslog.severity", T_VSTR,  "severity of the message"        },
  { "syslog.time",     T_CTIME, "date of the event"              },
  { "syslog.host",     T_VSTR,  "host"                           },
  { "syslog.repeat",   T_INT,   "message repetition"             },
  { "syslog.pid",      T_INT,   "process id of the event source" },
  { "syslog.prog",     T_VSTR,  "program name"                   },
  { "syslog.msg",      T_VSTR,  "the message"                    }
};


static void *
syslog_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  DebugLog(DF_MOD, DS_INFO, "load() syslog@%p\n", (void *) &mod_syslog);

  register_fields(ctx, mod, syslog_fields, SYSLOG_FIELDS);

  return (NULL);
}

#ifdef UNUSED
static char *syslog_deps[] = {
  "udp",
  "textfile",
  NULL
};
#endif

input_module_t mod_syslog = {
  MOD_MAGIC,
  ORCHIDS_VERSION,
  "syslog",
  "CeCILL2",
  NULL,
  NULL,
  syslog_preconfig,
  NULL,
  NULL
};


/*
** date conversion
*/

struct tm *
syslog_getdate(const char *date)
{
  static struct tm t;
  int day;

  /* XXX hard coded year */
  t.tm_year = 107;

  switch (date[0]) {
    case 'A': /* Aug, Avr */
      switch (date[1]) {
        case 'u':
          t.tm_mon = MOD_SYSLOG_AUG;
          break;
        case 'v':
          t.tm_mon = MOD_SYSLOG_AVR;
          break;
      }
      break;

    case 'D': /* Dec */
      t.tm_mon = MOD_SYSLOG_DEC;
      break;

    case 'F': /* Feb */
      t.tm_mon = MOD_SYSLOG_FEB;
      break;

    case 'J': /* Jan, Jul, Jun */
      if (date[1] == 'a') {
        t.tm_mon = MOD_SYSLOG_JAN;
      }
      else if (date[1] == 'u' && date[2] == 'l') {
        t.tm_mon = MOD_SYSLOG_JUL;
      }
      else if (date[1] == 'u' && date[2] == 'n') {
        t.tm_mon = MOD_SYSLOG_JUN;
      }
      break;

    case 'M': /* Mar, May */
      switch (date[2]) {
      case 'r':
        t.tm_mon = MOD_SYSLOG_MAR;
        break;
      case 'y':
        t.tm_mon = MOD_SYSLOG_MAY;
        break;
      }
      break;

    case 'N': /* Nov */
      t.tm_mon = MOD_SYSLOG_NOV;
      break;

    case 'O': /* Oct */
      t.tm_mon = MOD_SYSLOG_OCT;
      break;

    case 'S': /* Sep */
      t.tm_mon = MOD_SYSLOG_SEP;
      break;

    default:
      return (NULL);
      break;
    }

  if (date[4] != ' ') {
    day = (date[4] - '0') * 10;
  }
  else {
    day = 0;
  }
  day += date[5] - '0';
  t.tm_mday = day;

  t.tm_hour = (date[7] - '0') * 10 + (date[8] - '0');
  t.tm_min = (date[10] - '0') * 10 + (date[11] - '0');
  t.tm_sec = (date[13] - '0') * 10 + (date[14] - '0');

  return (&t);
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
