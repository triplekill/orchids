/**
 ** @file mod_auditd.c
 ** The auditd module.
 **
 ** @author Hedi BENZINA <benzina@lsv.ens-cachan.fr>
 ** @author Jean Goubault-Larrecq <goubault@lsv.ens-cachan.fr>
 ** @version 0.2
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

#ifdef HAVE_LIBAUDIT_H

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
  unsigned int serial;
  ovm_var_t *var;
  gc_t *gc_ctx = octx->ctx->gc_ctx;
  GC_START (gc_ctx, 2);

  t = time_convert(s, end, &time);
  serial = 0;
  if (t<end && *t==':') /* found it */
    {
      t = action_atoi_unsigned (t+1, end, &serial);
      if (t<end && *t==')')
	t++;
    }
  var = ovm_timeval_new (gc_ctx);
  TIMEVAL(var) = time;
  GC_UPDATE (gc_ctx, 0, var);

  var = ovm_int_new (gc_ctx, (int)serial);
  GC_UPDATE (gc_ctx, 1, var);

  FILL_EVENT(octx, F_AUDITD_TIME, 2);
  GC_END (gc_ctx);
  return t;
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
  { "a0=", F_AUDITD_A0, action_doer_uint_hex },
  { "a1=", F_AUDITD_A1, action_doer_uint_hex },
  { "a2=", F_AUDITD_A2, action_doer_uint_hex },
  { "a3=", F_AUDITD_A3, action_doer_uint_hex },
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
  { "cwd=", F_AUDITD_CWD, action_doer_string },
  { NULL, 0 }
};

static int dissect_auditd(orchids_t *ctx, mod_entry_t *mod,
			  event_t *event, void *data)
{
  char *txt_line;
  int txt_len;
  actions_orchids_ctx_t *octx = mod->config; // data
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
  post_event(ctx, mod, *octx.out_event);
  GC_END(gc_ctx);
  return (0);
}

static field_t auditd_fields[] = {
  {"auditd.node",     T_VSTR,     "auditd node"                         },
  {"auditd.type",     T_VSTR,     "type of auditd event"                },
  {"auditd.time",     T_TIMEVAL,  "auditd event time"                   },
  {"auditd.serial",   T_UINT,      "event serial number"                 },
  {"auditd.arch",     T_UINT,      "the elf architecture flags"          },
  {"auditd.syscall",  T_UINT,      "syscall number"                      },
  {"auditd.success",  T_VSTR,     "syscall success"                     },
  {"auditd.exit",     T_UINT,      "exit value"                          },
  {"auditd.varzero",  T_UINT,      "syscall argument"                    },
  {"auditd.varone",   T_UINT,      "syscall argument"                    },	
  {"auditd.vartwo",   T_UINT,      "syscall argument"                    },	
  {"auditd.varthree", T_UINT,      "syscall argument"                    },	
  {"auditd.items",    T_UINT,      "number of path records in the event" },
  {"auditd.ppid",     T_UINT,      "parent pid"                          },
  {"auditd.pid",      T_UINT,      "process id"                          },
  {"auditd.auid",     T_UINT,      "process auid"                        },
  {"auditd.uid",      T_UINT,      "user id"                             },
  {"auditd.gid",      T_UINT,      "group id"                            },
  {"auditd.euid",     T_UINT,      "effective user id"                   },	
  {"auditd.suid",     T_UINT,      "set user id"                         },	
  {"auditd.fsuid",    T_UINT,      "file system user id"                 },	
  {"auditd.egid",     T_UINT,      "effective group id"                  },	
  {"auditd.sgid",     T_UINT,      "set group id"                        },	
  {"auditd.fsgid",    T_UINT,      "file system group id"                },	
  {"auditd.tty",      T_VSTR,     "tty interface"                       },
  {"auditd.ses",      T_UINT,      "user's SE Linux user account"        },
  {"auditd.comm",     T_VSTR,     "command line program name"           },
  {"auditd.exe",      T_VSTR,     "executable name"                     },
  {"auditd.subj",     T_VSTR,     "lspp subject's context string"       },
  {"auditd.key",      T_VSTR,     "tty interface"                       },
  {"auditd.item",     T_UINT,      "file path: item"                     },
  {"auditd.name",     T_UINT,      "file path: name"                     },
  {"auditd.inode",    T_UINT,      "file path: inode"                    },
  {"auditd.mode",     T_UINT,      "file path: mode"                     },
  {"auditd.dev",      T_UINT,      "file path: device (major and minor)" },
  {"auditd.ouid",     T_UINT,      "file path: originator uid"           },
  {"auditd.ogid",     T_UINT,      "file path: originator gid"           },
  {"auditd.rdev",     T_UINT,      "file path: real device (major, minor)" },
  {"auditd.cwd",      T_VSTR,     "file cwd: the current working directory" },
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
  "auditd",                 /* module name */
  "CeCILL2",                /* module license */
  NULL,
  NULL,
  auditd_preconfig,         /* called just after module registration */
  NULL,
  NULL,
  dissect_auditd	    /* auditd conditional dissector */
};

#endif /* HAVE_LIBAUDIT_H */

/*
** Copyright (c) 2002-2005 by Julien OLIVAIN, Laboratoire Spécification
** et Vérification (LSV), CNRS UMR 8643 & ENS Cachan.
**
** Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
**
vv** This software is a computer program whose purpose is to detect intrusions
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


