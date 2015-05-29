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
#include <errno.h>

#include "orchids.h"

#include "orchids_api.h"

#include "mod_syslog.h"

input_module_t mod_syslog;

/*
** priority = facility * 8 + severity
** (extracted from rfc3164)
*/
#ifdef OBSOLETE
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
#endif

/*
int generic_dissect(orchids_t *ctx, mod_entry_t *mod, event_t *event, void *data)
{
  return syslog_dissect(ctx, mod, event, data);
}
*/


static int syslog_dissect(orchids_t *ctx, mod_entry_t *mod, event_t *event,
			  void *data)
{
  syslog_data_t *syslog_cfg = mod->config;
  struct tm *t;
  char *txt_line;
  int txt_len;
  char *tag;
  int tag_len;
  size_t token_size;
  unsigned long syslog_priority;
  unsigned long n;
  int ret;
  int diff;
  int year_present, bytes_read;
  syslog_time_tracker_t *tt;
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

  /* check if we have a raw syslog line (with encoded facility and priority) */
  if (txt_line[0] == '<')
    {
      token_size = get_next_uint(txt_line + 1, &syslog_priority, txt_len);
      if (syslog_priority > 191)
	{ /* 23 (local7) * 8 + 7 (debug) == 191 */
	  DebugLog(DF_MOD, DS_WARN, "PRI error.\n");
	  ret = 1;
	  goto end;
	}
      GC_UPDATE(gc_ctx, F_FACILITY, syslog_cfg->syslog_facility[syslog_priority >> 3]);
      GC_UPDATE(gc_ctx, F_SEVERITY, syslog_cfg->syslog_severity[syslog_priority & 0x07]);

      txt_line += token_size + 2; /* number size and '<' '>' */
      txt_len -= token_size + 2;
    }
  
  t = syslog_getdate(txt_line, txt_len, &year_present, &bytes_read);
  if (t == NULL)
    {
      DebugLog(DF_MOD, DS_WARN, "time format error.\n");
      /*
	ret = 1;
	goto end;
      */
      val = ovm_ctime_new (gc_ctx, time(NULL));
      // in case of error, take current time
    }
  else
    {
      // Deal with the standard syslog problem: the year is usually missing
      // Get dissection tag:
      tag = NULL;
      tag_len = 0;
      if (event->next!=NULL && event->next->value!=NULL)
	switch (TYPE(event->next->value))
	  {
	  case T_STR: tag = STR(event->next->value);
	    tag_len = STRLEN(event->next->value);
	    break;
	  case T_VSTR: tag = VSTR(event->next->value);
	    tag_len = VSTRLEN(event->next->value);
	    break;
	  }
      tt = NULL;
      if (tag!=NULL)
	tt = hash_get (syslog_cfg->year_table, tag, tag_len);
      if (tt==NULL)
	tt = &syslog_cfg->default_year;
      if (tt->flags && SYSLOG_TT_FIRST)
	{
	  tt->flags &= ~SYSLOG_TT_FIRST;
	}
      else
	{ /* We decide that year has advanced by 1 if month has decreased
	     by too much, namely if t->tm_mon - tt->last.tm_mon
	     is between 6 and 11 modulo 12.
	  */
	  diff = t->tm_mon - tt->last.tm_mon;
	  if (diff<0)
	    diff += 12;
	  if (diff>=6)
	    tt->year++;
	}
      t->tm_year = tt->year;
      tt->last.tm_mon = t->tm_mon; /* Only record the month, for speed */
      val = ovm_ctime_new(gc_ctx, mktime(t));
    }
  GC_UPDATE(gc_ctx, F_TIME, val);

  txt_line += bytes_read;
  txt_len -= bytes_read;

  if (txt_len <= 0)
    {
      DebugLog(DF_MOD, DS_WARN, "line terminated after time field, ignoring event\n");
      ret = 1;
      goto end;
    }

  token_size = my_strspn(txt_line, " ", txt_len);
  txt_line += token_size;
  txt_len -= token_size;
  token_size = get_next_token(txt_line, ' ', txt_len);
  if (token_size==0)
    {
      DebugLog(DF_MOD, DS_WARN, "host name absent, ignoring event\n");
      ret = 1;
      goto end;
    }
  val = ovm_vstr_new (gc_ctx, event->value);
  VSTR(val) = txt_line;
  VSTRLEN(val) = token_size;
  GC_UPDATE(gc_ctx, F_HOST, val);

  txt_line += token_size;
  txt_len -= token_size;
  token_size = my_strspn(txt_line, " ", txt_len);
  txt_line += token_size;
  txt_len -= token_size;
  /* We now expect 'prog[pid]' (where prog is a program... or a user id!)
     but gconfd uses the non-standard syntax '(prog-pid)'.
     Also, this is the place where we can expect to see the infamous
     "last message repeated n times" */
  if (txt_len>=22 && strncmp("last message repeated ", txt_line, 22)==0)
    {
      token_size = my_strspn(txt_line, "\r\n", txt_len);
      val = ovm_vstr_new (gc_ctx, event->value);
      VSTR(val) = txt_line;
      VSTRLEN(val) = token_size;
      GC_UPDATE(gc_ctx, F_MSG, val);

      txt_line += 22;
      txt_len -= 22;
      token_size = my_strspn(txt_line, " ", txt_len);
      txt_line += token_size;
      txt_len -= token_size;
      token_size = get_next_uint(txt_line, &n, txt_len);
      val = ovm_uint_new (gc_ctx, n);
      GC_UPDATE(gc_ctx, F_REPEAT, val);
      GC_UPDATE (gc_ctx, F_PROG, syslog_cfg->syslog_str);
      REGISTER_EVENTS(ctx, mod, SYSLOG_FIELDS);
      goto end;
    }
  else if (txt_len>=1 && txt_line[0]=='(') /* gconfd syntax */
    {
      txt_line++;
      txt_len--;
      token_size = my_strcspn(txt_line, "-)", txt_len);
      if (token_size==0)
	{
	  DebugLog(DF_MOD, DS_WARN, "prog name absent, ignoring event\n");
	  ret = 1;
	  goto end;
	}
      val = ovm_vstr_new (gc_ctx, event->value);
      VSTR(val) = txt_line;
      VSTRLEN(val) = token_size;
      GC_UPDATE(gc_ctx, F_PROG, val);
      txt_line += token_size;
      txt_len -= token_size;
      if (txt_len>=1 && txt_line[0]=='-')
	{
	  txt_line++;
	  txt_len--;
	  token_size = get_next_uint(txt_line, &n, txt_len);
	  val = ovm_uint_new (gc_ctx, n);
	  GC_UPDATE(gc_ctx, F_PID, val);
	  txt_line += token_size;
	  txt_len -= token_size;
	}
      if (txt_len>=1 && txt_line[0]==')')
	{
	  txt_line++;
	  txt_len--;
	}
    }
  else /* usual syntax 'prog[pid]' */
    {
      token_size = my_strcspn (txt_line, " [:", txt_len);
      if (token_size==0)
	{
	  DebugLog(DF_MOD, DS_WARN, "prog name absent, ignoring event\n");
	  ret = 1;
	  goto end;
	}
      val = ovm_vstr_new (gc_ctx, event->value);
      VSTR(val) = txt_line;
      VSTRLEN(val) = token_size;
      GC_UPDATE(gc_ctx, F_PROG, val);
      txt_line += token_size;
      txt_len -= token_size;
      if (txt_len>=1 && txt_line[0]=='[')
	{
	  txt_line++;
	  txt_len--;
	  token_size = get_next_uint (txt_line, &n, txt_len);
	  val = ovm_uint_new (gc_ctx, n);
	  GC_UPDATE(gc_ctx, F_PID, val);
	  txt_line += token_size;
	  txt_len -= token_size;
	  if (txt_len>=1 && txt_line[0]==']')
	    {
	      txt_line++;
	      txt_len--;
	    }
	}
    }

  token_size = my_strspn(txt_line, ": ", txt_len);
  txt_line += token_size;
  txt_len -= token_size;

  for (token_size = txt_len; token_size>=1
	 && (txt_line[token_size-1]=='\n' || txt_line[token_size-1]=='\r'); )
    token_size--;
  val = ovm_vstr_new (gc_ctx, event->value);
  VSTR(val) = txt_line;
  VSTRLEN(val) = token_size;
  GC_UPDATE(gc_ctx, F_MSG, val);

  REGISTER_EVENTS(ctx, mod, SYSLOG_FIELDS);

 end:
  GC_END(gc_ctx);
  return ret;
}

field_t syslog_fields[] = {
  { "syslog.facility", &t_uint, MONO_UNKNOWN,  "source of the message"       },
  { "syslog.severity", &t_uint, MONO_UNKNOWN,  "severity of the message"     },
  { "syslog.time",     &t_ctime, MONO_MONO, "date of the event"              },
  { "syslog.host",     &t_str, MONO_UNKNOWN,  "host"                         },
  { "syslog.repeat",   &t_uint, MONO_UNKNOWN,   "message repetition"         },
  { "syslog.pid",      &t_uint, MONO_UNKNOWN,   "process id of the event source" },
  { "syslog.prog",     &t_str, MONO_UNKNOWN,  "program name"                 },
  { "syslog.msg",      &t_str, MONO_UNKNOWN,  "the message"                  }
};


static void *syslog_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  syslog_data_t *data;
  unsigned int i;
  //size_t len;
  ovm_var_t *val;
  struct tm *t;
  time_t now;

  DebugLog(DF_MOD, DS_INFO, "load() syslog@%p\n", (void *) &mod_syslog);
 
  data = Xmalloc (sizeof(syslog_data_t));
  now = time (NULL);
  t = gmtime (&now);
  data->default_year.year = t->tm_year;
  data->default_year.flags = SYSLOG_TT_FIRST;
  data->year_table = new_hash (101); /* XXX hard-coded hash table size */
  GC_TOUCH (ctx->gc_ctx, val = ovm_vstr_new (ctx->gc_ctx, NULL));
  VSTR(val) = "syslog";
  VSTRLEN(val) = strlen(VSTR(val));
  data->syslog_str = val;
  gc_add_root(ctx->gc_ctx, (gc_header_t **)&data->syslog_str);

  for (i=0; i<SYSLOG_MAX_SEVERITY; i++)
    {
      GC_TOUCH (ctx->gc_ctx, val = ovm_uint_new (ctx->gc_ctx, i));
      /*
      len = strlen(syslog_severity_g[i]);
      GC_TOUCH (ctx->gc_ctx, val = ovm_vstr_new (ctx->gc_ctx, NULL));
      VSTR(val) = syslog_severity_g[i];
      VSTRLEN(val) = strlen(VSTR(val));
      */
      data->syslog_severity[i] = val;
      gc_add_root(ctx->gc_ctx, (gc_header_t **)&data->syslog_severity[i]);
    }
  for (i=0; i<SYSLOG_MAX_FACILITY; i++)
    {
      GC_TOUCH (ctx->gc_ctx, val = ovm_uint_new (ctx->gc_ctx, i));
      /*
      len = strlen(syslog_facility_g[i]);
      GC_TOUCH (ctx->gc_ctx, val = ovm_vstr_new (ctx->gc_ctx, NULL));
      VSTR(val) = syslog_facility_g[i];
      VSTRLEN(val) = strlen(VSTR(val));
      */
      data->syslog_facility[i] = val;
      gc_add_root(ctx->gc_ctx, (gc_header_t **)&data->syslog_facility[i]);
    }
  register_fields(ctx, mod, syslog_fields, SYSLOG_FIELDS);

  return data;
}

#ifdef UNUSED
static char *syslog_deps[] = {
  "udp",
  "textfile",
  NULL
};
#endif

static void syslog_set_default_year (orchids_t *ctx, mod_entry_t * mod, config_directive_t *dir)
{
  syslog_data_t *data = mod->config;
  int year, n;
  time_t now;

  errno = 0;
  switch (dir->args[0])
    {
    case '+':
      n = strtol (dir->args+1, NULL, 10);
      if (errno)
	{
	err:
	  DebugLog(DF_MOD, DS_ERROR, "SetDefaultYear %s: bad argument, errno=%d\n", dir->args, errno);
	  return;
	}
      now = time (NULL);
      year = gmtime (&now)->tm_year + n;
      break;
    case '-':
      n = strtol (dir->args+1, NULL, 10);
      if (errno)
	goto err;
      now = time (NULL);
      year = gmtime (&now)->tm_year - n;
      break;
    default:
      year = strtol (dir->args, NULL, 10);
      if (errno)
	goto err;
      break;
    }
  data->default_year.year = year;
}

static void syslog_set_year (orchids_t *ctx, mod_entry_t * mod, config_directive_t *dir)
{
  syslog_data_t *data = mod->config;
  int year, n;
  char *tag, *end;
  long len;
  syslog_time_tracker_t *tt;
  time_t now;

  tag = dir->args;
  for (end=tag; end[0]!=0 && !isspace(end[0]); end++);
  len = end-tag;
  while (isspace(end[0])) end++;
  errno = 0;
  tt = hash_get(data->year_table,tag,len);
  if (tt!=NULL)
    {
      char c = tag[len];

      tag[len] = 0;
      DebugLog(DF_MOD, DS_ERROR, "SetYear already specified for tag %s\n", tag);
      tag[len] = c;
      return;
    }
  switch (end[0])
    {
    case '+':
      n = strtol (end+1, NULL, 10);
      if (errno)
	{
	err:
	  DebugLog(DF_MOD, DS_ERROR, "SetYear %s: bad argument, errno=%d\n", dir->args, errno);
	  return;
	}
      now = time (NULL);
      year = gmtime (&now)->tm_year + n;
      break;
    case '-':
      n = strtol (end+1, NULL, 10);
      if (errno)
	goto err;
      now = time (NULL);
      year = gmtime (&now)->tm_year - n;
      break;
    default:
      year = strtol (end, NULL, 10);
      if (errno)
	goto err;
      break;
    }
  tt = Xmalloc (sizeof(syslog_time_tracker_t));
  tt->year = year;
  tt->flags = SYSLOG_TT_FIRST;
  hash_add (data->year_table, tt, tag, len);
}

static mod_cfg_cmd_t syslog_dir[] =
{
  { "SetDefaultYear", syslog_set_default_year, "Set default starting year" },
  { "SetYear", syslog_set_year, "Set starting year for given dissection tag" },
  { NULL, NULL, NULL }
};

input_module_t mod_syslog = {
  MOD_MAGIC,
  ORCHIDS_VERSION,
  MODULE_DISSECTABLE,
  "syslog",
  "CeCILL2",
  NULL,
  syslog_dir,
  syslog_preconfig,
  NULL,
  NULL,
  syslog_dissect,
  &t_str		    /* type of fields it expects to dissect */
};


/*
** date conversion
*/

struct tm *syslog_getdate(const char *date, int date_len,
			   int *year_present, int *bytes_read)
{
  static struct tm t;
  int year;
  int i;

  *year_present = 0; /* year is missing by default */
  *bytes_read = 0;
  if (date_len<4)
    return NULL;
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

  date += 4;
  date_len -= 4;
  *bytes_read += 4;
  if (date_len<3)
    return NULL;
  if (isspace(date[0]) && isdigit(date[1]) && isspace(date[2]))
    t.tm_mday = (date[1]-'0');
  else if (isdigit(date[0]) && isdigit(date[1]) && isspace(date[2]))
    t.tm_mday = (date[0]-'0')*10 + (date[1]-'0');
  else return NULL;

  date += 3;
  date_len -= 3;
  *bytes_read += 3;

  if (date_len<3)
    return NULL;
  if (date[2]!=':') // then assume that year is given
    {
      *year_present = 1;
      year = 0;
      for (i=0; i<date_len && isdigit(date[i]); i++)
	{
	  year = 10*year + (date[i] - '0');
	}
      t.tm_year = year;
      for (; i<date_len && date[i]==' '; i++);
      date += i;
      date_len -= i;
      *bytes_read += i;
    }
  if (date_len < 8)
    return NULL;
  if (isdigit(date[0]) && isdigit(date[1]) && date[2]==':' &&
      isdigit(date[3]) && isdigit(date[4]) && date[5]==':' &&
      isdigit(date[6]) && isdigit(date[7]))
    {
      t.tm_hour = (date[0] - '0') * 10 + (date[1] - '0');
      t.tm_min = (date[3] - '0') * 10 + (date[4] - '0');
      t.tm_sec = (date[6] - '0') * 10 + (date[7] - '0');
    }
  else return NULL;
  date += 8;
  date_len -= 8;
  *bytes_read += 8;
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
