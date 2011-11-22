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


static int
snare_dissect(orchids_t *ctx, mod_entry_t *mod, event_t *event, void *data)
{
  char *txt_line;
  size_t txt_len;
  ovm_var_t *attr[SNARE_FIELDS]; /* move into static array ?? */
  int ret;

  DebugLog(DF_MOD, DS_DEBUG, "snare_dissect()\n");

  memset(attr, 0, sizeof(attr));

  txt_line = STR(event->value);
  txt_len = STRLEN(event->value);

  /* XXX parse here */
  snareparse_set_str(txt_line, txt_len);
  snareparse_set_attrs(attr); /* XXX move into static array */
  ret = snareparse();
  snareparse_reset();
  if (ret != 0) {
    DebugLog(DF_MOD, DS_WARN, "parse error\n");
    free_fields(attr, SNARE_FIELDS);
    return (1);
  }

  add_fields_to_event(ctx, mod, &event, attr, SNARE_FIELDS);

  post_event(ctx, mod, event);

  return (0);
}


static field_t snare_fields[] = {
  { "snare.time",       T_TIMEVAL,  "event time"           },
  { "snare.class",      T_INT,      "snare event class"    },
  { "snare.syscall",    T_INT,      "system call number"   },
  { "snare.ruid",       T_INT,      "user id"              },
  { "snare.rgid",       T_INT,      "main group id"        },
  { "snare.euid",       T_INT,      "effective user id"    },
  { "snare.egid",       T_INT,      "effective id"         },
  { "snare.pid",        T_INT,      "process id"           },
  { "snare.procname",   T_VSTR,     "process name"         },
  { "snare.retcode",    T_INT,      "return code"          },
  { "snare.workdir",    T_VSTR,     "working directory"    },
  { "snare.path",       T_VSTR,     "path"                 },
  { "snare.mode",       T_INT,      "permissions"          },
  { "snare.createmode", T_INT,      "creation permissions" },
  { "snare.cmdline",    T_VSTR,     "command line"         },
  { "snare.src_path",   T_VSTR,     "source path"          },
  { "snare.dst_path",   T_VSTR,     "destination path"     },
  { "snare.sockcall",   T_INT,      "socket_call number"   },
  { "snare.dst_ip",     T_IPV4,     "destination ip"       },
  { "snare.dst_port",   T_INT,      "destination port"     },
  { "snare.src_ip",     T_IPV4,     "source ip"            },
  { "snare.src_port",   T_INT,      "source port"          },
  { "snare.owner_uid",  T_INT,      "owner user id"        },
  { "snare.owner_gid",  T_INT,      "owner group id"       },
  { "snare.target_id",  T_INT,      "caller user/group id" },
  { "snare.target_rid", T_INT,      "real user/group id"   },
  { "snare.target_sid", T_INT,      "saved user/group id"  },
  { "snare.mod_name",   T_VSTR,     "module name"          },
  { "snare.sequence",   T_INT,      "sequence number"      },
  { "snare.devmaj",     T_INT,      "device major number"  },
  { "snare.devmin",     T_INT,      "device minor number"  },
  { "snare.offest",     T_INT,      "truncate offset"      },
};


static void *
snare_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  DebugLog(DF_MOD, DS_INFO, "load() snare@%p\n", (void *) &mod_snare);

  register_fields(ctx, mod, snare_fields, SNARE_FIELDS);

  return (NULL);
}

int
generic_dissect(orchids_t *ctx, mod_entry_t *mod, event_t *event, void *data)
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
