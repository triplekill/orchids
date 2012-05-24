/**
 ** @file mod_auditd.c
 ** The auditd module.
 **
 ** @author Hedi BENZINA <benzina@lsv.ens-cachan.fr>
 ** @author Jean Goubault-Larrecq <goubault@lsv.ens-cachan.fr>
 ** @version 0.1
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

#include "orchids.h"

#include "orchids_api.h"

#include "mod_auditd.h"

input_module_t mod_auditd;

/********************************************************************************************/

struct action {
  char *name;
  int offset; /* where to write the datum, in the
		 syscall_event_t. */
  int code;
};

static struct action actions[] = {
  /* !!! Don't put any empty word here, or any word
     that is prefix of another one! */
  { "audit(", 0 /*unused*/, ACTION_AUDIT },
  { "arch=", offsetof(syscall_event_t,arch), ACTION_INT },
  { "syscall=", offsetof(syscall_event_t,syscall), ACTION_INT },
  { "success=", offsetof(syscall_event_t,success), ACTION_STRING },
  { "exit=", offsetof(syscall_event_t,exit), ACTION_INTREL },
  { "a0=", offsetof(syscall_event_t,a0), ACTION_STRING },
  { "a1=", offsetof(syscall_event_t,a1), ACTION_STRING },
  { "a2=", offsetof(syscall_event_t,a2), ACTION_STRING },
  { "a3=", offsetof(syscall_event_t,a3), ACTION_STRING },
  { "items=", offsetof(syscall_event_t,items), ACTION_INT },
  { "ppid=", offsetof(syscall_event_t,ppid), ACTION_INT },
  { "pid=", offsetof(syscall_event_t,pid), ACTION_INT },
  { "auid=", offsetof(syscall_event_t,auid), ACTION_INT },
  { "uid=", offsetof(syscall_event_t,uid), ACTION_INT },
  { "gid=", offsetof(syscall_event_t,gid), ACTION_INT },
  { "euid=", offsetof(syscall_event_t,euid), ACTION_INT },
  { "suid=", offsetof(syscall_event_t,suid), ACTION_INT },
  { "fsuid=", offsetof(syscall_event_t,fsuid), ACTION_INT },
  { "egid=", offsetof(syscall_event_t,egid), ACTION_INT },
  { "sgid=", offsetof(syscall_event_t,sgid), ACTION_INT },
  { "fsgid=", offsetof(syscall_event_t,fsgid), ACTION_INT },
  { "tty=", offsetof(syscall_event_t,tty), ACTION_ID },
  { "ses=", offsetof(syscall_event_t,ses), ACTION_INT },
  { "comm=", offsetof(syscall_event_t,comm), ACTION_STRING },
  { "exe=", offsetof(syscall_event_t,exe), ACTION_STRING },
  { "subj=", offsetof(syscall_event_t,subj), ACTION_SUBJ },
  { "key=", offsetof(syscall_event_t,key), ACTION_STRING},
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
				      syscall_event_t *evtp, int offset);
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



char *time_convert(char *str)
{
 char *cp;
 char *time;

 cp = strdup (str);
 time = strtok(cp,":");
 free(cp);

return time;
}

char *action_doer_audit (struct action_ctx *actx, char *s,
			 syscall_event_t *evtp, int offset)
{
  /* s is of the form 12345.678:1234 followed possibly
     by other characters, where 12345.678 is a time,
     and 1234 is a serial number.
  */
 char *t;

   evtp->time= time_convert(s);

   // t = strchrnul (s,':');  // This is a GNU extension.  Instead, it is as easy to write our own loop:
   {
      char c;
      for (t=s; c = *t, c!=0 && c!=':'; t++);
   }
   if (*t==':') /* found it */
     {
       t = action_atoi_unsigned (t+1, &evtp->serial);
       if (*t==')') t++;
       if (*t==':') t++;
     }
   return t;
 };

char *action_doer_int (struct action_ctx *actx, char *s,
		       syscall_event_t *evtp, int offset)
{
  return action_atoi_unsigned (s, (int *) (((char *)evtp) + offset));
}

char *action_doer_intrel (struct action_ctx *actx, char *s,
		       syscall_event_t *evtp, int offset)
{
  return action_atoi_signed (s, (int *) (((char *)evtp) + offset));
}

char *action_doer_hex (struct action_ctx *actx, char *s,
		       syscall_event_t *evtp, int offset)
{
  return action_atoi_hex (s, (int *) (((char *)evtp) + offset));
}

char *action_doer_id (struct action_ctx *actx, char *s,
		       syscall_event_t *evtp, int offset)
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
		       syscall_event_t *evtp, int offset)
{
  char c;
  char *to;


  if (*s != '"')
    return action_doer_id (actx, s, evtp, offset);
  to = s++;
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
			 syscall_event_t *evtp)
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
auditd_callback(orchids_t *ctx, mod_entry_t *mod, int sd, void *data)
{
 
  event_t *event; //orchids event

  ovm_var_t *attr[AUDITD_FIELDS];
  DebugLog(DF_MOD, DS_TRACE, "auditd_callback()\n");
  memset(attr, 0, sizeof(attr));

  syscall_event_t *auditd_data; //structure where to store auditd fields 
  auditd_data = Xmalloc(MAX_AUDIT_MESSAGE_LENGTH); 
  memset(auditd_data, 0, MAX_AUDIT_MESSAGE_LENGTH);

  struct action_ctx *actx; //parsing context
  actx = Xmalloc(sizeof(struct action_ctx));
  memset(actx, 0, sizeof(struct action_ctx));


   struct iovec vec;
   int rc;

   auditd_event_t *e = Xmalloc(sizeof(auditd_event_t)); //auditd event (see auditd_queue.h & queue.h :(
   memset(e, 0, sizeof(auditd_event_t));


        //Get header first. It is fixed size 
        vec.iov_base = &e->hdr;
        vec.iov_len = sizeof(struct audit_dispatcher_header);

        rc = readv(sd, &vec, 1);

        if (e->hdr.type != 1300) goto leave;	

printf("%d",e->hdr.type);
	if (rc > 0) {
        //Sanity check 
		if (e->hdr.ver != AUDISP_PROTOCOL_VER ||
				e->hdr.hlen != sizeof(e->hdr) ||
				e->hdr.size > MAX_AUDIT_MESSAGE_LENGTH) {
			DebugLog(DF_MOD, DS_TRACE, "auditd_callback() : Auditd dispatcher protocol mismatch !\n");
		}

	vec.iov_base = e->data;
	vec.iov_len = e->hdr.size;
	
	rc = readv(sd, &vec, 1);

	if (rc > 0)  
		{ 	
			printf("===== Msg From auditd ==========\n%s",e->data);
			action_init (actx);	
			action_parse_event (actx, e->data, auditd_data);
		}

 	}

//   attr[F_TIME] = ovm_vstr_new();
//   VSTR(attr[F_TIME]) = auditd_data->time;
//   VSTRLEN(attr[F_TIME]) = strlen(auditd_data->time);


  attr[F_SERIAL] = ovm_int_new();
  INT( attr[F_SERIAL] ) = auditd_data->serial;

  attr[F_ARCH] = ovm_int_new();
  INT( attr[F_ARCH] ) = auditd_data->arch;

   attr[F_SYSCALL] = ovm_int_new();
  INT( attr[F_SYSCALL] ) = auditd_data->syscall;

  /* Modif faite par NEY 23/05/2012 */
  attr[F_SUCCESS] = ovm_str_new();
  /* VSTR(attr[F_SUCCESS]) = auditd_data->success; */
  STRLEN(attr[F_SUCCESS]) = strlen(auditd_data->success);
  strcpy(attr[F_SUCCESS], auditd_data->success);

  attr[F_EXIT] = ovm_int_new();
  INT( attr[F_EXIT] ) = auditd_data->exit;

  /* Modif faite par NEY 23/05/2012 */
  attr[F_A0] = ovm_str_new();
  /* VSTR(attr[F_A0]) = auditd_data->a0; */
  STRLEN(attr[F_A0]) = strlen(auditd_data->a0);
  strcpy(attr[F_A0], auditd_data->a0);

  /* Modif faite par NEY 23/05/2012 */
  attr[F_A1] = ovm_str_new();
  /* VSTR(attr[F_A1]) = auditd_data->a1; */
  STRLEN(attr[F_A1]) = strlen(auditd_data->a1);
  strcpy(attr[F_A1], auditd_data->a1);

  /* Modif faite par NEY 23/05/2012 */
  attr[F_A2] = ovm_str_new();
  /* R(attr[F_A2]) = auditd_data->a2; */
  STRLEN(attr[F_A2]) = strlen(auditd_data->a2);
  strcpy(attr[F_A2], auditd_data->a2);

  /* Modif faite par NEY 23/05/2012 */
  attr[F_A3] = ovm_str_new();
  /* VSTR(attr[F_A3]) = auditd_data->a3; */
  STRLEN(attr[F_A3]) = strlen(auditd_data->a3);
  strcpy(attr[F_A3], auditd_data->a3);

  attr[F_ITEMS] = ovm_int_new();
  INT( attr[F_ITEMS] ) = auditd_data->items;
 

  attr[F_PPID] = ovm_int_new();
  INT( attr[F_PPID] ) = auditd_data->ppid;

  attr[F_PID] = ovm_int_new();
  INT( attr[F_PID] ) = auditd_data->pid;

  attr[F_AUID] = ovm_int_new();
  INT( attr[F_AUID] ) = auditd_data->auid;

  attr[F_UID] = ovm_int_new();
  INT( attr[F_UID] ) = auditd_data->uid;

  attr[F_GID] = ovm_int_new();
  INT( attr[F_GID] ) = auditd_data->gid;

  attr[F_EUID] = ovm_int_new();
  INT( attr[F_EUID] ) = auditd_data->euid;

  attr[F_SUID] = ovm_int_new();
  INT( attr[F_SUID] ) = auditd_data->suid;

  attr[F_FSUID] = ovm_int_new();
  INT( attr[F_FSUID] ) = auditd_data->fsuid;

  attr[F_EGID] = ovm_int_new();
  INT( attr[F_EGID] ) = auditd_data->egid;
 
  attr[F_SGID] = ovm_int_new();
  INT( attr[F_SGID] ) = auditd_data->sgid;

  attr[F_FSGID] = ovm_int_new();
  INT( attr[F_FSGID] ) = auditd_data->fsgid;

  /* Modif faite par NEY 23/05/2012 */
  attr[F_TTY] = ovm_str_new();
  /* VSTR(attr[F_TTY]) = auditd_data->tty;*/
  STRLEN(attr[F_TTY]) = strlen(auditd_data->tty);
  strcpy(STR(attr[F_TTY]), auditd_data->tty);

  attr[F_SES] = ovm_int_new();
  INT( attr[F_SES] ) = auditd_data->ses;

/*
    attr[F_COMM] = ovm_vstr_new();
    VSTR(attr[F_COMM]) = auditd_data->comm;
    VSTRLEN(attr[F_COMM]) = strlen(auditd_data->comm);

    attr[F_EXE] = ovm_vstr_new();
    VSTR(attr[F_EXE]) = auditd_data->exe;
    VSTRLEN(attr[F_EXE]) = strlen(auditd_data->exe);
 
   attr[F_SUBJ] = ovm_vstr_new();
   VSTR(attr[F_SUBJ]) = auditd_data->subj; 
   VSTRLEN(attr[F_SUBJ]) = strlen(auditd_data->subj);*/

  /* Modif faite par Jean 23/05/2012 */
  attr[F_KEY] = ovm_str_new();
  STRLEN(attr[F_KEY]) = strlen(auditd_data->key);
  strcpy(STR(attr[F_KEY]), auditd_data->key);

  /*  fill in orchids event */
  event = NULL;
  add_fields_to_event(ctx, mod, &event, attr, AUDITD_FIELDS);

  /* then, post the Orchids event */
  post_event(ctx, mod, event);

leave:
  free(e);
  free(auditd_data);
  free(actx);

  return (0);
}

static field_t auditd_fields[] = {
  {"auditd.time",     T_VSTR,     "auditd event time"                   },
  {"auditd.serial",   T_INT,      "event serial number"                 },
  {"auditd.arch",     T_INT,      "the elf architecture flags"          },
  {"auditd.syscall",  T_INT,      "syscall number"                      },
  {"auditd.success",  T_STR,     "syscall success"                     },
  {"auditd.exit",     T_INT,      "exit value"                          },
  {"auditd.varzero",  T_STR,     "syscall argument"                    },
  {"audtehitd.a1",    T_STR,     "syscall argument"                    },	
  {"auditd.a2",       T_STR,     "syscall argument"                    },	
  {"auditd.a3",       T_STR,     "syscall argument"                    },	
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
  {"auditd.tty",      T_STR,     "tty interface"                       },
  {"auditd.ses",      T_INT,      "user's SE Linux user account"        },
  {"auditd.comm",     T_VSTR,     "command line program name"           },
  {"auditd.exe",      T_VSTR,     "executable name"                     },
  {"auditd.subj",     T_VSTR,     "lspp subject's context string"       },
  {"auditd.key",      T_STR,     "tty interface"                       },
};



static void *
auditd_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  DebugLog(DF_MOD, DS_INFO, "load() auditd@%p\n", &mod_auditd);
 
  register_fields(ctx, mod, auditd_fields, AUDITD_FIELDS);

  return (NULL);

}

static int 
connect_sock_auditd()
{
  int sock_fd,len;
  struct sockaddr_un remote;

  sock_fd = Xsocket(AF_UNIX, SOCK_STREAM, 0);
  remote.sun_family = AF_UNIX;
  strcpy(remote.sun_path, SOCK_PATH);
  len = SUN_LEN(&remote); // was strlen(remote.sun_path) + sizeof(remote.sun_family);
  Xconnect(sock_fd, (struct sockaddr *)&remote, len);

  return sock_fd;
}


static void
auditd_postconfig(orchids_t *ctx, mod_entry_t *mod)
{
  int sd;
  sd = connect_sock_auditd();
  add_input_descriptor(ctx, mod, auditd_callback, sd, NULL);

}



input_module_t mod_auditd = {
  MOD_MAGIC,                /* Magic number */
  ORCHIDS_VERSION,          /* Module version */
  "auditd",                 /* module name */
  "CeCILL2",                /* module license */
  NULL,
  NULL,
  auditd_preconfig,         /* called just after module registration */
  auditd_postconfig,
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


