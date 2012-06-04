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
  int offset; /* where to write the datum, in the
		 auditd_syscall_event_t. */
  int code;
};

static struct action actions[] = {
  /* !!! Don't put any empty word here, or any word
     that is prefix of another one! */
  { "node=", offsetof(auditd_syscall_event_t,node), ACTION_STRING },
  { "type=", offsetof(auditd_syscall_event_t,type), ACTION_STRING },
  { "audit(", 0 /*unused*/, ACTION_AUDIT }, // this is what we get in 'binary' format
  { "msg=audit(", 0 /*unused*/, ACTION_AUDIT }, // this is what we get in 'string' format
  { "arch=", offsetof(auditd_syscall_event_t,arch), ACTION_INT },
  { "syscall=", offsetof(auditd_syscall_event_t,syscall), ACTION_INT },
  { "success=", offsetof(auditd_syscall_event_t,success), ACTION_STRING },
  { "exit=", offsetof(auditd_syscall_event_t,exit), ACTION_INTREL },
  { "a0=", offsetof(auditd_syscall_event_t,a0), ACTION_STRING },
  { "a1=", offsetof(auditd_syscall_event_t,a1), ACTION_STRING },
  { "a2=", offsetof(auditd_syscall_event_t,a2), ACTION_STRING },
  { "a3=", offsetof(auditd_syscall_event_t,a3), ACTION_STRING },
  { "items=", offsetof(auditd_syscall_event_t,items), ACTION_INT },
  { "ppid=", offsetof(auditd_syscall_event_t,ppid), ACTION_INT },
  { "pid=", offsetof(auditd_syscall_event_t,pid), ACTION_INT },
  { "auid=", offsetof(auditd_syscall_event_t,auid), ACTION_INT },
  { "uid=", offsetof(auditd_syscall_event_t,uid), ACTION_INT },
  { "gid=", offsetof(auditd_syscall_event_t,gid), ACTION_INT },
  { "euid=", offsetof(auditd_syscall_event_t,euid), ACTION_INT },
  { "suid=", offsetof(auditd_syscall_event_t,suid), ACTION_INT },
  { "fsuid=", offsetof(auditd_syscall_event_t,fsuid), ACTION_INT },
  { "egid=", offsetof(auditd_syscall_event_t,egid), ACTION_INT },
  { "sgid=", offsetof(auditd_syscall_event_t,sgid), ACTION_INT },
  { "fsgid=", offsetof(auditd_syscall_event_t,fsgid), ACTION_INT },
  { "tty=", offsetof(auditd_syscall_event_t,tty), ACTION_ID },
  { "ses=", offsetof(auditd_syscall_event_t,ses), ACTION_INT },
  { "comm=", offsetof(auditd_syscall_event_t,comm), ACTION_STRING },
  { "exe=", offsetof(auditd_syscall_event_t,exe), ACTION_STRING },
  { "subj=", offsetof(auditd_syscall_event_t,subj), ACTION_SUBJ },
  { "key=", offsetof(auditd_syscall_event_t,key), ACTION_STRING},
  { NULL, 0 }
};

struct action_tree {
#define TAG_PROCEED 1
#define TAG_END 2
  int tag;
  union {
    struct action_tree *proceed[256];
    struct { int val; /* among ACTION_INT, etc. */
      int offset;
      int dummy;
    } code;
  } what;
};

struct action_ctx {
  struct action_tree *tree;
  char *(*action_doer[ACTION_LIMIT]) (struct action_ctx *actx, char *s,
				      auditd_syscall_event_t *evtp, int offset);
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
  atp->what.code.offset = ap->offset;
}

char *action_atoi_unsigned (char *s, int *ip)
{
  int i = 0;
  char c;

  while (c = *s, isdigit (c))
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
      i = 10*i + j;
      s++;
    }
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
			 auditd_syscall_event_t *evtp, int offset)
{
  /* s is of the form 12345.678:1234 followed possibly
     by other characters, where 12345.678 is a time,
     and 1234 is a serial number.
  */
 char *t;

 t = time_convert(s, &evtp->time);
 if (*t!='\0') /* found it */
   {
     t = action_atoi_unsigned (t+1, &evtp->serial);
     if (*t==')') t++;
   }
 return t;
}

char *action_doer_int (struct action_ctx *actx, char *s,
		       auditd_syscall_event_t *evtp, int offset)
{
  return action_atoi_unsigned (s, (int *) (((char *)evtp) + offset));
}

char *action_doer_intrel (struct action_ctx *actx, char *s,
		       auditd_syscall_event_t *evtp, int offset)
{
  return action_atoi_signed (s, (int *) (((char *)evtp) + offset));
}

char *action_doer_hex (struct action_ctx *actx, char *s,
		       auditd_syscall_event_t *evtp, int offset)
{
  return action_atoi_hex (s, (int *) (((char *)evtp) + offset));
}

char *action_doer_id (struct action_ctx *actx, char *s,
		       auditd_syscall_event_t *evtp, int offset)
{
  char c;
  *(char **)(((char *)evtp)+offset) = s;
  while (c = *s, c!=0 && !isspace (c))
    s++;
   if (c!=0)
    *s++ = 0; /* insert end-of-string marker */
  return s;
}

char *action_doer_string (struct action_ctx *actx, char *s,
		       auditd_syscall_event_t *evtp, int offset)
{
  char c;
  char *to;


  if (*s != '"')
    return action_doer_id (actx, s, evtp, offset);
  to = s++;
  *(char **)(((char *)evtp)+offset) = to;
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
  actx->action_doer[ACTION_INTREL] = action_doer_intrel;
  actx->action_doer[ACTION_HEX] = action_doer_hex;
  actx->action_doer[ACTION_STRING] = action_doer_string;
}

void action_parse_event (struct action_ctx *actx, char *data,
			 auditd_syscall_event_t *evtp)
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
	  s = (*actx->action_doer[atp->what.code.val]) (actx, s, evtp, atp->what.code.offset);
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
  auditd_syscall_event_t *auditd_data = cfg->auditd_data; 
  ovm_var_t *attr[AUDITD_FIELDS];

  DebugLog(DF_MOD, DS_TRACE, "auditd_callback()\n");
  // memset(attr, 0, sizeof(attr));

  txt_line = STR(event->value);
printf("mod_auditd: %s\n", txt_line);
  txt_len = STRLEN(event->value);

  action_parse_event (cfg->actx, txt_line, auditd_data);

  if (auditd_data->node) {
    attr[F_AUDITD_NODE] = ovm_vstr_new();
    VSTR(attr[F_AUDITD_NODE]) = auditd_data->node;
    VSTRLEN(attr[F_AUDITD_NODE]) = strlen(auditd_data->node);
  }
  else attr[F_AUDITD_NODE] = NULL;

  if (auditd_data->type) {
    attr[F_AUDITD_TYPE] = ovm_vstr_new();
    VSTR(attr[F_AUDITD_TYPE]) = auditd_data->type;
    VSTRLEN(attr[F_AUDITD_TYPE]) = strlen(auditd_data->type);
  }
  else attr[F_AUDITD_TYPE] = NULL;

  attr[F_AUDITD_TIME] = ovm_timeval_new();
  TIMEVAL(attr[F_AUDITD_TIME]) = auditd_data->time;
  attr[F_AUDITD_TIME]->flags = TYPE_MONO;

  attr[F_AUDITD_SERIAL] = ovm_int_new();
  INT( attr[F_AUDITD_SERIAL] ) = auditd_data->serial;
  attr[F_AUDITD_SERIAL]->flags |= TYPE_MONO;

  attr[F_AUDITD_ARCH] = ovm_int_new();
  INT( attr[F_AUDITD_ARCH] ) = auditd_data->arch;

  attr[F_AUDITD_SYSCALL] = ovm_int_new();
  INT( attr[F_AUDITD_SYSCALL] ) = auditd_data->syscall;

  if (auditd_data->success) {
    attr[F_AUDITD_SUCCESS] = ovm_vstr_new();
    VSTR(attr[F_AUDITD_SUCCESS]) = auditd_data->success;
    VSTRLEN(attr[F_AUDITD_SUCCESS]) = strlen(auditd_data->success);
  }
  else attr[F_AUDITD_SUCCESS] = NULL;

  attr[F_AUDITD_EXIT] = ovm_int_new();
  INT( attr[F_AUDITD_EXIT] ) = auditd_data->exit;

  if (auditd_data->a0) {
    attr[F_AUDITD_A0] = ovm_vstr_new();
    VSTR(attr[F_AUDITD_A0]) = auditd_data->a0;
    VSTRLEN(attr[F_AUDITD_A0]) = strlen(auditd_data->a0);
  }
  else attr[F_AUDITD_A0] = NULL;

  if (auditd_data->a1) {
    attr[F_AUDITD_A1] = ovm_vstr_new();
    VSTR(attr[F_AUDITD_A1]) = auditd_data->a1;
    VSTRLEN(attr[F_AUDITD_A1]) = strlen(auditd_data->a1);
  }
  else attr[F_AUDITD_A1] = NULL;

  if (auditd_data->a2) {
    attr[F_AUDITD_A2] = ovm_vstr_new();
    VSTR(attr[F_AUDITD_A2]) = auditd_data->a2;
    VSTRLEN(attr[F_AUDITD_A2]) = strlen(auditd_data->a2);
  }
  else attr[F_AUDITD_A2] = NULL;

  if (auditd_data->a3) {
    attr[F_AUDITD_A3] = ovm_vstr_new();
    VSTR(attr[F_AUDITD_A3]) = auditd_data->a3;
    VSTRLEN(attr[F_AUDITD_A3]) = strlen(auditd_data->a3);
  }
  else attr[F_AUDITD_A3] = NULL;

  attr[F_AUDITD_ITEMS] = ovm_int_new();
  INT( attr[F_AUDITD_ITEMS] ) = auditd_data->items;
 
  attr[F_AUDITD_PPID] = ovm_int_new();
  INT( attr[F_AUDITD_PPID] ) = auditd_data->ppid;

  attr[F_AUDITD_PID] = ovm_int_new();
  INT( attr[F_AUDITD_PID] ) = auditd_data->pid;

  attr[F_AUDITD_AUID] = ovm_int_new();
  INT( attr[F_AUDITD_AUID] ) = auditd_data->auid;

  attr[F_AUDITD_UID] = ovm_int_new();
  INT( attr[F_AUDITD_UID] ) = auditd_data->uid;

  attr[F_AUDITD_GID] = ovm_int_new();
  INT( attr[F_AUDITD_GID] ) = auditd_data->gid;

  attr[F_AUDITD_EUID] = ovm_int_new();
  INT( attr[F_AUDITD_EUID] ) = auditd_data->euid;

  attr[F_AUDITD_SUID] = ovm_int_new();
  INT( attr[F_AUDITD_SUID] ) = auditd_data->suid;

  attr[F_AUDITD_FSUID] = ovm_int_new();
  INT( attr[F_AUDITD_FSUID] ) = auditd_data->fsuid;

  attr[F_AUDITD_EGID] = ovm_int_new();
  INT( attr[F_AUDITD_EGID] ) = auditd_data->egid;
 
  attr[F_AUDITD_SGID] = ovm_int_new();
  INT( attr[F_AUDITD_SGID] ) = auditd_data->sgid;

  attr[F_AUDITD_FSGID] = ovm_int_new();
  INT( attr[F_AUDITD_FSGID] ) = auditd_data->fsgid;

  if (auditd_data->tty) {
    attr[F_AUDITD_TTY] = ovm_vstr_new();
    VSTR(attr[F_AUDITD_TTY]) = auditd_data->tty;
    VSTRLEN(attr[F_AUDITD_TTY]) = strlen(auditd_data->tty);
  }
  else attr[F_AUDITD_TTY] = NULL;

  attr[F_AUDITD_SES] = ovm_int_new();
  INT( attr[F_AUDITD_SES] ) = auditd_data->ses;

  if (auditd_data->comm) {
    attr[F_AUDITD_COMM] = ovm_vstr_new();
    VSTR(attr[F_AUDITD_COMM]) = auditd_data->comm;
    VSTRLEN(attr[F_AUDITD_COMM]) = strlen(auditd_data->comm);
  }
  else attr[F_AUDITD_COMM] = NULL;

  if (auditd_data->exe) {
    attr[F_AUDITD_EXE] = ovm_vstr_new();
    VSTR(attr[F_AUDITD_EXE]) = auditd_data->exe;
    VSTRLEN(attr[F_AUDITD_EXE]) = strlen(auditd_data->exe);
  }
  else attr[F_AUDITD_EXE] = NULL;

  if (auditd_data->subj) {
    attr[F_AUDITD_SUBJ] = ovm_vstr_new();
    VSTR(attr[F_AUDITD_SUBJ]) = auditd_data->subj;
    VSTRLEN(attr[F_AUDITD_SUBJ]) = strlen(auditd_data->subj);
  }
  else attr[F_AUDITD_SUBJ] = NULL;

  if (auditd_data->key) {
    attr[F_AUDITD_KEY] = ovm_vstr_new();
    VSTR(attr[F_AUDITD_KEY]) = auditd_data->key;
    VSTRLEN(attr[F_AUDITD_KEY]) = strlen(auditd_data->key);
  }
  else attr[F_AUDITD_KEY] = NULL;

  /*  fill in orchids event */
  add_fields_to_event(ctx, mod, &event, attr, AUDITD_FIELDS);

  /* then, post the Orchids event */
  post_event(ctx, mod, event);

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
  {"auditd.varzero",  T_VSTR,     "syscall argument"                    },
  {"audtehitd.a1",    T_VSTR,     "syscall argument"                    },	
  {"auditd.a2",       T_VSTR,     "syscall argument"                    },	
  {"auditd.a3",       T_VSTR,     "syscall argument"                    },	
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
};



static void *
auditd_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  DebugLog(DF_MOD, DS_INFO, "load() auditd@%p\n", &mod_auditd);
 
  register_fields(ctx, mod, auditd_fields, AUDITD_FIELDS);

  auditd_cfg_t *cfg;

  cfg = Xmalloc(sizeof(auditd_cfg_t));
  cfg->auditd_data = Xmalloc(sizeof(auditd_syscall_event_t));
  memset(cfg->auditd_data, 0, sizeof(auditd_syscall_event_t));
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


