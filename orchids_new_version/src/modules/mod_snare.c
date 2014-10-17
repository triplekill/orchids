/**
 ** @file mod_snare.c
 ** Module for parsing Linux-Snare text log files.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 ** @date  Started on: Thu Feb 13 13:03:07 2003
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

/* #include "snare.h" */

#include "orchids.h"

#include "orchids_api.h"

#include "mod_snare.h"


input_module_t mod_snare;


static int snare_dissect(orchids_t *ctx, mod_entry_t *mod, event_t *event,
			 void *data)
{
  char *txt_line;
  size_t txt_len;
  int ret;
  gc_t *gc_ctx = ctx->gc_ctx;
  GC_START(gc_ctx, SNARE_FIELDS+1);
  GC_UPDATE(gc_ctx, SNARE_FIELDS, event);

  DebugLog(DF_MOD, DS_DEBUG, "snare_dissect()\n");

  txt_line = STR(event->value);
  txt_len = STRLEN(event->value);

  /* XXX parse here */
  snareparse_set_gc_ctx(gc_ctx);
  snareparse_set_delegate(event->value);
  snareparse_set_str(txt_line, txt_len);
  snareparse_set_attrs((ovm_var_t **)&GC_LOOKUP(0));
  ret = snareparse();
  snareparse_reset();
  if (ret != 0)
    DebugLog(DF_MOD, DS_WARN, "parse error\n");
  else
    REGISTER_EVENTS(ctx, mod, SNARE_FIELDS);
  GC_END(gc_ctx);
  return ret;
}


static field_t snare_fields[] = {
  { "snare.time",       &t_timeval,  "event time"           },
  { "snare.class",      &t_int,      "snare event class"    },
  { "snare.syscall",    &t_int,      "system call number"   },
  { "snare.ruid",       &t_int,      "user id"              },
  { "snare.rgid",       &t_int,      "main group id"        },
  { "snare.euid",       &t_int,      "effective user id"    },
  { "snare.egid",       &t_int,      "effective id"         },
  { "snare.pid",        &t_int,      "process id"           },
  { "snare.procname",   &t_str,     "process name"         },
  { "snare.retcode",    &t_int,      "return code"          },
  { "snare.workdir",    &t_str,     "working directory"    },
  { "snare.path",       &t_str,     "path"                 },
  { "snare.mode",       &t_int,      "permissions"          },
  { "snare.createmode", &t_int,      "creation permissions" },
  { "snare.cmdline",    &t_str,     "command line"         },
  { "snare.src_path",   &t_str,     "source path"          },
  { "snare.dst_path",   &t_str,     "destination path"     },
  { "snare.sockcall",   &t_int,      "socket_call number"   },
  { "snare.dst_ip",     &t_ipv4,     "destination ip"       },
  { "snare.dst_port",   &t_int,      "destination port"     },
  { "snare.src_ip",     &t_ipv4,     "source ip"            },
  { "snare.src_port",   &t_int,      "source port"          },
  { "snare.owner_uid",  &t_int,      "owner user id"        },
  { "snare.owner_gid",  &t_int,      "owner group id"       },
  { "snare.target_id",  &t_int,      "caller user/group id" },
  { "snare.target_rid", &t_int,      "real user/group id"   },
  { "snare.target_sid", &t_int,      "saved user/group id"  },
  { "snare.mod_name",   &t_str,     "module name"          },
  { "snare.sequence",   &t_int,      "sequence number"      },
  { "snare.devmaj",     &t_int,      "device major number"  },
  { "snare.devmin",     &t_int,      "device minor number"  },
  { "snare.offest",     &t_int,      "truncate offset"      },
};


static void *snare_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  DebugLog(DF_MOD, DS_INFO, "load() snare@%p\n", (void *) &mod_snare);

  register_fields(ctx, mod, snare_fields, SNARE_FIELDS);
  return NULL;
}

int generic_dissect(orchids_t *ctx, mod_entry_t *mod, event_t *event,
		    void *data)
{
  return snare_dissect(ctx, mod, event, data);
}

static char *snare_deps[] = {
  "udp",
  "textfile",
  NULL
};


input_module_t mod_snare = {
  MOD_MAGIC,
  ORCHIDS_VERSION,
  "snare",
  "CeCILL2",
  snare_deps,
  NULL,
  snare_preconfig,
  NULL,
  NULL,
  NULL
};


#if 0
int
main(int argc, char *argv[])
{
  static char buffer[4096];
  int ret;

/*   if ( snareparse() ) */
/*     printf("parse error\n"); */
/*   else */
/*     printf("OK.\n"); */

  while (fgets(buffer, 4096, stdin) != NULL)
    {
      DPRINTF( ("read: %s", buffer) );
      ret = parse_event(buffer, strlen(buffer));
      if (ret != 0)
        {
          DPRINTF( ("bad line !\n") );
        }
    }

  return (0);
}

#endif /* 0 */


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
