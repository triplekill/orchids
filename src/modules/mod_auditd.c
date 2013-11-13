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

struct action {
  char *name;
  int evtno; /* orchids event number */
  int code; /* action to undertake */
};

static struct action actions[] = {
  /* !!! Don't put any empty word here, or any word
     that is prefix of another one! */
  { "node=", F_AUDITD_NODE, ACTION_STRING },
  { "type=", F_AUDITD_TYPE, ACTION_STRING },
  { "audit(", 0 /*unused*/, ACTION_AUDIT }, // this is what we get in 'binary' format
  { "msg=audit(", 0 /*unused*/, ACTION_AUDIT }, // this is what we get in 'string' format
  { "arch=", F_AUDITD_ARCH, ACTION_INT },
  { "syscall=", F_AUDITD_SYSCALL, ACTION_INT },
  { "success=", F_AUDITD_SUCCESS, ACTION_STRING },
  { "exit=", F_AUDITD_EXIT, ACTION_INT },
  { "a0=", F_AUDITD_A0, ACTION_HEX },
  { "a1=", F_AUDITD_A1, ACTION_HEX },
  { "a2=", F_AUDITD_A2, ACTION_HEX },
  { "a3=", F_AUDITD_A3, ACTION_HEX },
  { "items=", F_AUDITD_ITEMS, ACTION_INT },
  { "ppid=", F_AUDITD_PPID, ACTION_INT },
  { "pid=", F_AUDITD_PID, ACTION_INT },
  { "auid=", F_AUDITD_AUID, ACTION_INT },
  { "uid=", F_AUDITD_UID, ACTION_INT },
  { "gid=", F_AUDITD_GID, ACTION_INT },
  { "euid=", F_AUDITD_EUID, ACTION_INT },
  { "suid=", F_AUDITD_SUID, ACTION_INT },
  { "fsuid=", F_AUDITD_FSUID, ACTION_INT },
  { "egid=", F_AUDITD_EGID, ACTION_INT },
  { "sgid=", F_AUDITD_SGID, ACTION_INT },
  { "fsgid=", F_AUDITD_FSGID, ACTION_INT },
  { "tty=", F_AUDITD_TTY, ACTION_ID },
  { "ses=", F_AUDITD_SES, ACTION_INT },
  { "comm=", F_AUDITD_COMM, ACTION_STRING },
  { "exe=", F_AUDITD_EXE, ACTION_STRING },
  { "subj=", F_AUDITD_SUBJ, ACTION_SUBJ },
  { "key=", F_AUDITD_KEY, ACTION_STRING},
  { "item=", F_AUDITD_ITEM, ACTION_INT },
  { "name=", F_AUDITD_NAME, ACTION_STRING},
  { "inode=", F_AUDITD_INODE, ACTION_INT },
  { "mode=", F_AUDITD_MODE, ACTION_INT },
  { "dev=",  F_AUDITD_DEV, ACTION_DEV },
  { "ouid=", F_AUDITD_OUID, ACTION_INT },
  { "ogid=", F_AUDITD_OGID, ACTION_INT },
  { "rdev=",  F_AUDITD_RDEV, ACTION_DEV },
  { "cwd=", F_AUDITD_CWD, ACTION_STRING },
  { NULL, 0 }
};

struct action_tree {
#define TAG_PROCEED 1
#define TAG_END 2
  int tag;
  union {
    struct action_tree *proceed[256];
    struct { int val; /* among ACTION_INT, etc. */
      int evtno;
      int dummy;
    } code;
  } what;
};

struct action_orchids_ctx {
  orchids_t *ctx;
  mod_entry_t *mod;
  event_t *event;
};
#define FILL_EVENT(octx, attr,n,len) add_fields_to_event_stride(octx->ctx, octx->mod, &octx->event, (attr), n, n+len)

struct action_ctx {
  struct action_tree *tree;
  char *(*action_doer[ACTION_LIMIT]) (struct action_ctx *actx, char *s,
				      struct action_orchids_ctx *octx,
				      int n);
};

static void 
action_insert (struct action_ctx *actx, struct action* ap)
{
  char *s, c;
  struct action_tree **atpp = &actx->tree;
  struct action_tree *atp = *atpp;
  int i;

  for (s=ap->name; (c = *s++)!=0; )
    { /* atp should always be non-NULL here. */
      i = (int)(unsigned int)(unsigned char)c;
      atpp = &atp->what.proceed[i];
      atp = *atpp;
      if (atp==NULL)
	{
	  atp = *atpp = Xmalloc (sizeof (struct action_tree));
	  atp->tag = TAG_PROCEED;
	  for (i=0; i<256; i++)
	    atp->what.proceed[i] = NULL;
	}
    }
  atp = *atpp = Xmalloc (offsetof(struct action_tree, what.code.dummy));
  atp->tag = TAG_END;
  atp->what.code.val = ap->code;
  atp->what.code.evtno = ap->evtno;
}

char *action_atoi_hex (char *s, int *ip)
{
  int i = 0;
  int j;
  char c;

  while (c = *s, isxdigit (c))
    {
      if (isdigit (c))
	j = ((int)c) - '0';
      else j = (((int)c) - 'A' + 10) & 0x1f;
      i = 16*i + j;
      s++;
    }
  *ip = i;
  return s;
}

char *action_atoi_unsigned (char *s, int *ip)
{
  int i = 0;
  char c;

  c = *s;
  if (c=='0') // octal or hex
    {
      c = *++s;
      if (c=='x') // hex
        {
          int j;

	  s++;
          while (c = *s, isxdigit (c))
            {
              if (isdigit (c))
        	j = ((int)c) - '0';
              else j = (((int)c) - 'A' + 10) & 0x1f;
              i = 16*i + j;
              s++;
            }
        }
      else // octal
        while (c = *s, isdigit (c) && c<'8')
          {
            i = 8*i + (((int)c) - '0');
            s++;
          }
    }
  else while (c = *s, isdigit (c))
    {
      i = 10*i + (((int)c) - '0');
      s++;
    }
  *ip = i;
  return s;
}

char *action_atoi_signed (char *s, int *ip)
{
  int i;
  int negate = 0;

  if (*s == '-')
    {
      negate = 1;
      s++;
    }
  s = action_atoi_unsigned (s, &i);
  if (negate)
    i = -i;
  *ip = i;
  return s;
}

char *time_convert(char *str, struct timeval *tv)
{
  char c, *s;
  double secs, floor_part;

  secs = strtod(str,&s);
  c = *s;
  if (c==':' || c==')')
    {
      *s++ = '\0';
      floor_part = floor(secs);
      tv->tv_sec = floor_part;
      tv->tv_usec = 1000000.0 * (secs - floor_part);
      return s;
    }
  else return s-1;
}

char *action_doer_audit (struct action_ctx *actx, char *s,
			 struct action_orchids_ctx *octx, int n)
{
  /* s is of the form 12345.678:1234 followed possibly
     by other characters, where 12345.678 is a time,
     and 1234 is a serial number.
  */
  char *t;
  struct timeval time;
  int serial;
  ovm_var_t *attr[2];

  t = time_convert(s, &time);
  serial = 0;
  if (*t!='\0') /* found it */
    {
      t = action_atoi_unsigned (t+1, &serial);
      if (*t==')') t++;
    }
  attr[0] = ovm_timeval_new();
  TIMEVAL(attr[0]) = time;
  attr[0]->flags = TYPE_MONO;

  attr[1] = ovm_int_new();
  INT(attr[1]) = serial;
  attr[1]->flags |= TYPE_MONO;

  FILL_EVENT(octx, attr, F_AUDITD_TIME, 2);
  return t;
}

char *action_doer_int (struct action_ctx *actx, char *s,
		       struct action_orchids_ctx *octx, int n)
{
  int i;
  ovm_var_t *v;
  char *t;

  t = action_atoi_signed (s, &i);
  v = ovm_int_new();
  INT(v) = i;
  FILL_EVENT(octx, &v, n, 1);
  return t;
}

char *action_doer_unsigned_int (struct action_ctx *actx, char *s,
		                struct action_orchids_ctx *octx, int n)
{
  int i;
  ovm_var_t *v;
  char *t;

  t = action_atoi_unsigned (s, &i);
  v = ovm_int_new();
  INT(v) = i;
  FILL_EVENT(octx, &v, n, 1);
  return t;
}

char *action_doer_hex (struct action_ctx *actx, char *s,
		       struct action_orchids_ctx *octx, int n)
{
  int i;
  ovm_var_t *v;
  char *t;

  t = action_atoi_hex (s, &i);
  v = ovm_int_new();
  INT(v) = i;
  FILL_EVENT(octx, &v, n, 1);
  return t;
}

char *action_doer_dev (struct action_ctx *actx, char *s,
                       struct action_orchids_ctx *octx, int n)
{
  int major=0, minor=0, i;
  ovm_var_t *v;
  char *t, c;

  while (c = *s, isdigit (c))
    {
      major = 10*major + (((int)c) - '0');
      s++;
    }
  if (c==':')
    {
       s++;
       while (c = *s, isdigit (c))
         {
            minor = 10*minor + (((int)c) - '0');
            s++;
         }
    }
  i = (major << 6) | minor;
  t = action_atoi_unsigned (s, &i);
  v = ovm_int_new();
  INT(v) = i;
  FILL_EVENT(octx, &v, n, 1);
  return t;
}



char *action_doer_id (struct action_ctx *actx, char *s,
		      struct action_orchids_ctx *octx, int n)
{
  char c, *t;
  ovm_var_t *v;

  v = ovm_vstr_new();
  VSTR(v) = s;
  for (t=s; c = *t, c!=0 && !isspace (c); t++);
  VSTRLEN(v) = t-s;
  if (c!=0)
    *t++ = 0; /* insert end-of-string marker */
  FILL_EVENT(octx, &v, n, 1);
  return t;
}

char *action_doer_string (struct action_ctx *actx, char *s,
		          struct action_orchids_ctx *octx, int n)
{
  char c;
  char *to;
  ovm_var_t *v;

  if (*s != '"')
    return action_doer_id (actx, s, octx, n);
  to = s++;
  v = ovm_vstr_new();
  VSTR(v) = to;
  while (1)
    {
      switch (c = *s++)
	{
	case 0: s--; goto end;
	case '"': goto end;
	case '\\':
	  c = *s++;
	  switch (c)
	    {
	    case 0: case '"':
	      goto end;
	    case 'n': *to++ = '\n'; break;
	    case 'r': *to++ = '\r'; break;
	    case 't': *to++ = '\t'; break;
	    case '\\': *to++ = '\\'; break;
	    case '0': case '1': case '2': case '3':
	    case '4': case '5': case '6': case '7':
	      {
		/* start of octal number */
		int i = (c - '0');

		c = *s++;
		switch (c)
		  {
		  case '0': case '1': case '2': case '3':
		  case '4': case '5': case '6': case '7':
		    i = 8*i + (c - '0');
		    c = *s++;
		    switch (c)
		      {
		      case '0': case '1': case '2': case '3':
		      case '4': case '5': case '6': case '7':
			i = 8*i + (c - '0');
			break;
		      default: s--; break;
		      }
		    break;
		  default: s--; break;
		  }
		*to++ = (char)i;
	      }
	      break;
	    default: *to++ = c; break;
	    }
	  break;
	default:
	  *to++ = c;
	  break;
	}
    }
 end:
  *to = 0;
  VSTRLEN(v) = to-VSTR(v);
  FILL_EVENT(octx, &v, n, 1);

  return s;
}

static void
action_init (struct action_ctx *actx)
{
  struct action *ap;

  {
    struct action_tree *atp;
    int i;

    atp = actx->tree = Xmalloc (sizeof (struct action_tree));
    atp->tag = TAG_PROCEED;
    for (i=0; i<256; i++)
      atp->what.proceed[i] = NULL;
  }
  for (ap=actions; ap->name!=NULL; ap++)
    action_insert (actx, ap);
  /* Check that every entry in actx->action_doer[] is filled in! */
  actx->action_doer[ACTION_AUDIT] = action_doer_audit;
  actx->action_doer[ACTION_INT] = action_doer_int;
  //actx->action_doer[ACTION_UNSIGNED_INT] = action_doer_unsigned_int;
  actx->action_doer[ACTION_HEX] = action_doer_hex;
  actx->action_doer[ACTION_STRING] = action_doer_string;
  actx->action_doer[ACTION_DEV] = action_doer_dev;
}

void action_parse_event (struct action_ctx *actx, char *data,
			 struct action_orchids_ctx *octx)
{
  char *s;
  char c;
  int i;
  struct action_tree *atp;


  atp = actx->tree; /* NULL or with tag=TAG_PROCEED */
  if (atp==NULL)
    return;
  /* For the rest of the procedure, atp is always non-NULL,
     or with tag=TAG_PROCEED.
     This assumes that atcx->tree does not just store
     the empty word.
  */
  for (s = data; (c = *s++)!=0; )
    {
      /* skip spaces */
      if (isspace (c))
	continue;
      /* Try to recognize keyword=value (or audit(...)). */
      goto again;
    start:
      if ((c = *s++)==0)
	break;
    again:
      i = (int)(unsigned int)(unsigned char)c;
      atp = atp->what.proceed[i];
      if (atp==NULL) /* Foiled: no such keyword. */
	atp = actx->tree; /* Ignore this illegal sequence. */
      else if (atp->tag==TAG_END) /* We found something to do! */
	{
	  s = (*actx->action_doer[atp->what.code.val]) (actx, s, octx, atp->what.code.evtno);
	  atp = actx->tree;
	}
      else goto start;
    }
}



static int
dissect_auditd(orchids_t *ctx, mod_entry_t *mod, event_t *event, void *data)
{
  char *txt_line;
  int txt_len;
  auditd_cfg_t *cfg = mod->config; // (auditd_cfg_t *)data;
  struct action_orchids_ctx octx = { ctx, mod, event };

  DebugLog(DF_MOD, DS_TRACE, "auditd_callback()\n");
  // memset(attr, 0, sizeof(attr));

  txt_line = STR(event->value);
  //printf("mod_auditd: %s\n", txt_line);
  txt_len = STRLEN(event->value);

  action_parse_event (cfg->actx, txt_line, &octx);

//!!! missing dev, rdev

  /* then, post the Orchids event */
  post_event(ctx, mod, octx.event);

  return (0);
}

int
generic_dissect(orchids_t *ctx, mod_entry_t *mod, event_t *event, void *data)
{
  return dissect_auditd(ctx, mod, event, data);
}

static field_t auditd_fields[] = {
  {"auditd.node",     T_VSTR,     "auditd node"                         },
  {"auditd.type",     T_VSTR,     "type of auditd event"                },
  {"auditd.time",     T_TIMEVAL,  "auditd event time"                   },
  {"auditd.serial",   T_INT,      "event serial number"                 },
  {"auditd.arch",     T_INT,      "the elf architecture flags"          },
  {"auditd.syscall",  T_INT,      "syscall number"                      },
  {"auditd.success",  T_VSTR,     "syscall success"                     },
  {"auditd.exit",     T_INT,      "exit value"                          },
  {"auditd.varzero",  T_INT,      "syscall argument"                    },
  {"auditd.varone",   T_INT,      "syscall argument"                    },	
  {"auditd.vartwo",   T_INT,      "syscall argument"                    },	
  {"auditd.varthree", T_INT,      "syscall argument"                    },	
  {"auditd.items",    T_INT,      "number of path records in the event" },
  {"auditd.ppid",     T_INT,      "parent pid"                          },
  {"auditd.pid",      T_INT,      "process id"                          },
  {"auditd.auid",     T_INT,      "process auid"                        },
  {"auditd.uid",      T_INT,      "user id"                             },
  {"auditd.gid",      T_INT,      "group id"                            },
  {"auditd.euid",     T_INT,      "effective user id"                   },	
  {"auditd.suid",     T_INT,      "set user id"                         },	
  {"auditd.fsuid",    T_INT,      "file system user id"                 },	
  {"auditd.egid",     T_INT,      "effective group id"                  },	
  {"auditd.sgid",     T_INT,      "set group id"                        },	
  {"auditd.fsgid",    T_INT,      "file system group id"                },	
  {"auditd.tty",      T_VSTR,     "tty interface"                       },
  {"auditd.ses",      T_INT,      "user's SE Linux user account"        },
  {"auditd.comm",     T_VSTR,     "command line program name"           },
  {"auditd.exe",      T_VSTR,     "executable name"                     },
  {"auditd.subj",     T_VSTR,     "lspp subject's context string"       },
  {"auditd.key",      T_VSTR,     "tty interface"                       },
  {"auditd.item",     T_INT,      "file path: item"                     },
  {"auditd.name",     T_INT,      "file path: name"                     },
  {"auditd.inode",    T_INT,      "file path: inode"                    },
  {"auditd.mode",     T_INT,      "file path: mode"                     },
  {"auditd.dev",      T_INT,      "file path: device (major and minor)" },
  {"auditd.ouid",     T_INT,      "file path: originator uid"           },
  {"auditd.ogid",     T_INT,      "file path: originator gid"           },
  {"auditd.rdev",     T_INT,      "file path: real device (major, minor)" },
  {"auditd.cwd",      T_VSTR,     "file cwd: the current working directory" },
};



static void *
auditd_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  DebugLog(DF_MOD, DS_INFO, "load() auditd@%p\n", &mod_auditd);
 
  register_fields(ctx, mod, auditd_fields, AUDITD_FIELDS);

  auditd_cfg_t *cfg;

  cfg = Xmalloc(sizeof(auditd_cfg_t));
  cfg->actx = Xmalloc(sizeof(struct action_ctx));
  memset(cfg->actx, 0, sizeof(struct action_ctx));
  action_init (cfg->actx);	
  return cfg;
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
  NULL
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


