/**
 ** @file mod_snare.c
 ** Module for parsing Linux-Snare text log files.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 ** @author Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
 **
 ** @version 0.2
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
#include <stdint.h>

/* #include "snare.h" */

#include "orchids.h"

#include "orchids_api.h"

#include "mod_snare.h"


input_module_t mod_snare;

static int snare_dissect(orchids_t *ctx, mod_entry_t *mod, event_t *event,
			 void *data, int dissector_level);

static void *snare_preconfig(orchids_t *ctx, mod_entry_t *mod);


static int snare_dissect(orchids_t *ctx, mod_entry_t *mod, event_t *event,
			 void *data, int dissector_level)
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
    REGISTER_EVENTS(ctx, mod, SNARE_FIELDS, dissector_level);
  GC_END(gc_ctx);
  return ret;
}


static field_t snare_fields[] = {
  { "snare.time",       &t_timeval, MONO_MONO,  "event time"           },
  { "snare.class",      &t_uint, MONO_UNKNOWN,      "snare event class"    },
  { "snare.syscall",    &t_uint, MONO_UNKNOWN,      "system call number"   },
  { "snare.ruid",       &t_uint, MONO_UNKNOWN,      "user id"              },
  { "snare.rgid",       &t_uint, MONO_UNKNOWN,      "main group id"        },
  { "snare.euid",       &t_uint, MONO_UNKNOWN,      "effective user id"    },
  { "snare.egid",       &t_uint, MONO_UNKNOWN,      "effective id"         },
  { "snare.pid",        &t_uint, MONO_UNKNOWN,      "process id"           },
  { "snare.procname",   &t_str, MONO_UNKNOWN,     "process name"         },
  { "snare.retcode",    &t_int, MONO_UNKNOWN,      "return code"          },
  { "snare.workdir",    &t_str, MONO_UNKNOWN,     "working directory"    },
  { "snare.path",       &t_str, MONO_UNKNOWN,     "path"                 },
  { "snare.mode",       &t_int, MONO_UNKNOWN,      "permissions"          },
  { "snare.createmode", &t_int, MONO_UNKNOWN,      "creation permissions" },
  { "snare.cmdline",    &t_str, MONO_UNKNOWN,     "command line"         },
  { "snare.src_path",   &t_str, MONO_UNKNOWN,     "source path"          },
  { "snare.dst_path",   &t_str, MONO_UNKNOWN,     "destination path"     },
  { "snare.sockcall",   &t_uint, MONO_UNKNOWN,      "socket_call number"   },
  { "snare.dst_ip",     &t_ipv4, MONO_UNKNOWN,     "destination ip"       },
  { "snare.dst_port",   &t_uint, MONO_UNKNOWN,      "destination port"     },
  { "snare.src_ip",     &t_ipv4, MONO_UNKNOWN,     "source ip"            },
  { "snare.src_port",   &t_uint, MONO_UNKNOWN,      "source port"          },
  { "snare.owner_uid",  &t_uint, MONO_UNKNOWN,      "owner user id"        },
  { "snare.owner_gid",  &t_uint, MONO_UNKNOWN,      "owner group id"       },
  { "snare.target_id",  &t_uint, MONO_UNKNOWN,      "caller user/group id" },
  { "snare.target_rid", &t_uint, MONO_UNKNOWN,      "real user/group id"   },
  { "snare.target_sid", &t_uint, MONO_UNKNOWN,      "saved user/group id"  },
  { "snare.mod_name",   &t_str, MONO_UNKNOWN,     "module name"          },
  { "snare.sequence",   &t_uint, MONO_UNKNOWN,      "sequence number"      },
  { "snare.devmaj",     &t_uint, MONO_UNKNOWN,      "device major number"  },
  { "snare.devmin",     &t_uint, MONO_UNKNOWN,      "device minor number"  },
  { "snare.offset",     &t_uint, MONO_UNKNOWN,      "truncate offset"      },
};


static void *snare_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  DebugLog(DF_MOD, DS_INFO, "load() snare@%p\n", (void *) &mod_snare);

  register_fields(ctx, mod, snare_fields, SNARE_FIELDS);
  return NULL;
}


input_module_t mod_snare = {
  MOD_MAGIC,
  ORCHIDS_VERSION,
  0,			    /* flags */
  "snare",
  "CeCILL2",
  NULL,
  NULL,
  snare_preconfig,
  NULL,
  NULL,
  NULL,
  snare_dissect,
  &t_str,		    /* type of fields it expects to dissect */
  NULL, /* save */
  NULL, /* restore */
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
