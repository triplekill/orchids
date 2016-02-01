/**
 ** @file mod_auditd.c
 ** The auditd module.
 **
 ** @author Hedi BENZINA <benzina@lsv.ens-cachan.fr>
 ** @author Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
 ** @version 1.0
 ** @ingroup modules
 ** 
 **
 ** @date  Started on: Fri Feb  7 11:07:42 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#ifdef OBSOLETE
#include <libaudit.h>
#include <sys/socket.h>
#include <sys/un.h>
#endif

#include <sys/types.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "orchids.h"
#include "orchids_api.h"
#include "mod_utils.h"

#include "mod_auditd.h"

input_module_t mod_auditd;

/********************************************************************************************/

#define FILL_EVENT(octx,n,len) add_fields_to_event_stride(octx->ctx, octx->mod, octx->out_event, (ovm_var_t **)GC_DATA(), n, n+len)

static char *action_doer_audit (action_orchids_ctx_t *octx,
				char *s, char *end,
				int field_num)
{
  /* s is of the form 12345.678:1234 followed possibly
     by other characters, where 12345.678 is a time,
     and 1234 is a serial number.
  */
  char *t;
  struct timeval time;
  unsigned long serial;
  ovm_var_t *var;
  gc_t *gc_ctx = octx->ctx->gc_ctx;
  GC_START (gc_ctx, 2);

  t = time_convert(s, end, &time);
  serial = 0;
  if (t<end && *t==':') /* found it */
    {
      t = orchids_atoui (t+1, end-t-1, &serial);
      if (t<end && *t==')')
	t++;
    }
  var = ovm_timeval_new (gc_ctx);
  TIMEVAL(var) = time;
  GC_UPDATE (gc_ctx, 0, var);

  var = ovm_uint_new (gc_ctx, serial);
  GC_UPDATE (gc_ctx, 1, var);

  FILL_EVENT(octx, F_AUDITD_TIME, 2);
  GC_END (gc_ctx);
  return t;
}

static char *action_doer_a0 (action_orchids_ctx_t *octx,
			     char *s, char *end,
			     int field_num)
{ /* a0= fields are sometimes followed by uints in hex (as in type=SYSCALL events)
     and sometimes by strings, starting with a quote (as in type=EXECVE events)...
  */
  if (s < end && *s == '"') /* it's a string */
    return action_doer_string (octx, s, end, field_num + 1);
  else return action_doer_uint_hex (octx, s, end, field_num);
}

static action_t auditd_actions[] = {
  /* Don't put any empty word here, or any word
     that is prefix of another one! */
  { "node=", F_AUDITD_NODE, action_doer_string },
  { "type=", F_AUDITD_TYPE, action_doer_string },
  { "audit(", 0 /*unused*/, action_doer_audit }, // this is what we get in 'binary' format
  { "msg=audit(", 0 /*unused*/, action_doer_audit }, // this is what we get in 'string' format
  { "arch=", F_AUDITD_ARCH, action_doer_uint },
  { "syscall=", F_AUDITD_SYSCALL, action_doer_uint },
  { "success=", F_AUDITD_SUCCESS, action_doer_string },
  { "exit=", F_AUDITD_EXIT, action_doer_uint },
  { "items=", F_AUDITD_ITEMS, action_doer_uint },
  { "ppid=", F_AUDITD_PPID, action_doer_uint },
  { "pid=", F_AUDITD_PID, action_doer_uint },
  { "auid=", F_AUDITD_AUID, action_doer_uint },
  { "uid=", F_AUDITD_UID, action_doer_uint },
  { "gid=", F_AUDITD_GID, action_doer_uint },
  { "euid=", F_AUDITD_EUID, action_doer_uint },
  { "suid=", F_AUDITD_SUID, action_doer_uint },
  { "fsuid=", F_AUDITD_FSUID, action_doer_uint },
  { "egid=", F_AUDITD_EGID, action_doer_uint },
  { "sgid=", F_AUDITD_SGID, action_doer_uint },
  { "fsgid=", F_AUDITD_FSGID, action_doer_uint },
  { "tty=", F_AUDITD_TTY, action_doer_id },
  { "ses=", F_AUDITD_SES, action_doer_uint },
  { "comm=", F_AUDITD_COMM, action_doer_string },
  { "exe=", F_AUDITD_EXE, action_doer_string },
  { "subj=", F_AUDITD_SUBJ, action_doer_string },
  { "key=", F_AUDITD_KEY, action_doer_string},
  { "item=", F_AUDITD_ITEM, action_doer_uint },
  { "name=", F_AUDITD_NAME, action_doer_string},
  { "inode=", F_AUDITD_INODE, action_doer_uint },
  { "mode=", F_AUDITD_MODE, action_doer_uint },
  { "dev=",  F_AUDITD_DEV, action_doer_dev },
  { "ouid=", F_AUDITD_OUID, action_doer_uint },
  { "ogid=", F_AUDITD_OGID, action_doer_uint },
  { "rdev=",  F_AUDITD_RDEV, action_doer_dev },
  { "nametype=", F_AUDITD_NAMETYPE, action_doer_string },
  { "cwd=", F_AUDITD_CWD, action_doer_string },
  /* For type=EXECVE: argc= a0= a1= a2= */
  /* However, a0, a1, a2, ..., are strings here */
  /* Example:
     type=EXECVE msg=audit(1454046118.818:17159): argc=3 a0="tail" a1="-f" a2="/var/log/aud\
it/audit.log"
     type=CWD msg=audit(1454046118.818:17159):  cwd="/home/lambda/orchids"
     type=PATH msg=audit(1454046118.818:17159): item=0 name="/usr/bin/tail" inode=10133 dev=00:10 mode=0100755 ouid=0 ogid=0 rdev=00:00 nametype=NORMAL
     type=PATH msg=audit(1454046118.818:17159): item=1 name=(null) inode=11552 dev=00:10 mode=0100755 ouid=0 ogid=0 rdev=00:00 nametype=NORMAL
     type=PROCTITLE msg=audit(1454046118.818:17159): proctitle=7461696C002D66002F7661722F6C6F672F61756469742F61756469742E6C6F67
   */
  { "argc=", F_AUDITD_ARGC, action_doer_uint },
  { "proctitle=", F_AUDITD_PROCTITLE, action_doer_string },
  { "sig=", F_AUDITD_SIG, action_doer_uint },
#include "fields_auditd.h"
  { NULL, 0 }
};

/*
type=PROCTITLE msg=audit(1454046123.718:17160): proctitle=2F62696E2F62617368002D630065786563202F7573722F6C6F63616C2F62696E2F6F72636869647320
type=ANOM_ABEND msg=audit(1454046123.822:17161): auid=1000 uid=65534 gid=65534 ses=2 pid=9299 comm="orchids" exe="/usr/local/bin/orchids" sig=11
*/

static int dissect_auditd(orchids_t *ctx, mod_entry_t *mod,
			  event_t *event, void *data,
			  int dissection_level)
{
  char *txt_line;
  int txt_len;
  action_orchids_ctx_t *octx = mod->config; // data
  gc_t *gc_ctx = ctx->gc_ctx;

  GC_START(gc_ctx, 1);
  octx->in_event = event;
  octx->out_event = (event_t **)&GC_LOOKUP(0);
  GC_UPDATE(gc_ctx, 0, event);

  DebugLog(DF_MOD, DS_TRACE, "auditd_callback()\n");
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

  action_parse_event (octx, txt_line, txt_line+txt_len);

  /* then, post the Orchids event */
  post_event(ctx, mod, *octx->out_event, dissection_level);
  GC_END(gc_ctx);
  return (0);
}

static field_t auditd_fields[] = {
  {"auditd.node",     &t_str, MONO_UNKNOWN,    "auditd node"                         },
  {"auditd.type",     &t_str, MONO_UNKNOWN,    "type of auditd event"                },
  {"auditd.time",     &t_timeval, MONO_MONO, "auditd event time"                   },
  {"auditd.serial",   &t_uint, MONO_MONO,     "event serial number"                 },
  {"auditd.arch",     &t_uint, MONO_UNKNOWN,     "Elf architecture flags"          },
  {"auditd.syscall",  &t_uint, MONO_UNKNOWN,      "syscall number"                      },
  {"auditd.success",  &t_str, MONO_UNKNOWN,     "syscall success"                     },
  {"auditd.exit",     &t_uint, MONO_UNKNOWN,      "exit value"                          },
  {"auditd.items",    &t_uint, MONO_UNKNOWN,      "number of path records in the event" },
  {"auditd.ppid",     &t_uint, MONO_UNKNOWN,      "parent pid"                          },
  {"auditd.pid",      &t_uint, MONO_UNKNOWN,      "process id"                          },
  {"auditd.auid",     &t_uint, MONO_UNKNOWN,      "process auid"                        },
  {"auditd.uid",      &t_uint, MONO_UNKNOWN,      "user id"                             },
  {"auditd.gid",      &t_uint, MONO_UNKNOWN,      "group id"                            },
  {"auditd.euid",     &t_uint, MONO_UNKNOWN,      "effective user id"                   },	
  {"auditd.suid",     &t_uint, MONO_UNKNOWN,      "set user id"                         },	
  {"auditd.fsuid",    &t_uint, MONO_UNKNOWN,      "file system user id"                 },	
  {"auditd.egid",     &t_uint, MONO_UNKNOWN,      "effective group id"                  },	
  {"auditd.sgid",     &t_uint, MONO_UNKNOWN,      "set group id"                        },	
  {"auditd.fsgid",    &t_uint, MONO_UNKNOWN,      "file system group id"                },	
  {"auditd.tty",      &t_str, MONO_UNKNOWN,     "tty interface"                       },
  {"auditd.ses",      &t_uint, MONO_UNKNOWN,      "user's SE Linux user account"        },
  {"auditd.comm",     &t_str, MONO_UNKNOWN,     "command line program name"           },
  {"auditd.exe",      &t_str, MONO_UNKNOWN,     "executable name"                     },
  {"auditd.subj",     &t_str, MONO_UNKNOWN,     "lspp subject's context string"       },
  {"auditd.key",      &t_str, MONO_UNKNOWN,     "tty interface"                       },
  {"auditd.item",     &t_uint, MONO_UNKNOWN,      "file path: item"                     },
  {"auditd.name",     &t_str, MONO_UNKNOWN,      "file path: name"                     },
  {"auditd.inode",    &t_uint, MONO_UNKNOWN,      "file path: inode"                    },
  {"auditd.mode",     &t_uint, MONO_UNKNOWN,      "file path: mode"                     },
  {"auditd.dev",      &t_uint, MONO_UNKNOWN,      "file path: device (major and minor)" },
  {"auditd.ouid",     &t_uint, MONO_UNKNOWN,      "file path: originator uid"           },
  {"auditd.ogid",     &t_uint, MONO_UNKNOWN,      "file path: originator gid"           },
  {"auditd.rdev",     &t_uint, MONO_UNKNOWN,      "file path: real device (major, minor)" },
  {"auditd.nametype", &t_str, MONO_UNKNOWN,       "file path: name type" },
  {"auditd.cwd",      &t_str, MONO_UNKNOWN,     "file cwd: the current working directory" },
  {"auditd.argc",     &t_uint, MONO_UNKNOWN,     "execve: number of arguments" },
  {"auditd.proctitle", &t_str, MONO_UNKNOWN,     "process title or identifier" },
  {"auditd.sig",      &t_uint, MONO_UNKNOWN,     "anom abend: signal number" },
#include "defs_auditd.h"
};



static void *auditd_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  action_orchids_ctx_t *octx;

  DebugLog(DF_MOD, DS_INFO, "load() auditd@%p\n", &mod_auditd);
 
  register_fields(ctx, mod, auditd_fields, AUDITD_FIELDS);

  octx = gc_base_malloc(ctx->gc_ctx, sizeof(action_orchids_ctx_t));
  octx->ctx = ctx;
  octx->mod = mod;
  octx->atree = compile_actions(ctx->gc_ctx, auditd_actions);
  octx->in_event = NULL;
  octx->out_event = NULL;
  return octx;
}


input_module_t mod_auditd = {
  MOD_MAGIC,                /* Magic number */
  ORCHIDS_VERSION,          /* Module version */
  0,			    /* flags */
  "auditd",                 /* module name */
  "CeCILL2",                /* module license */
  NULL,
  NULL,
  auditd_preconfig,         /* called just after module registration */
  NULL,
  NULL,
  NULL,
  dissect_auditd,	    /* auditd conditional dissector */
  &t_str,		    /* type of fields it expects to dissect */
  NULL,
  NULL,
};

/*
** Copyright (c) 2002-2005 by Julien OLIVAIN, Laboratoire Spécification
** et Vérification (LSV), CNRS UMR 8643 & ENS Cachan.
** Copyright (c) 2013-2015 by Jean GOUBAULT-LARRECQ, Laboratoire Spécification
** et Vérification (LSV), CNRS UMR 8643 & ENS Cachan.
**
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


