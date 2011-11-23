/**
 ** @file mod_stats.c
 ** Provide statistics of the whole analysis engine.
 ** Used to test/debug/orchids internals at runtime.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 ** @date  Started on: Mon Jan 27 18:37:45 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>

#include "orchids.h"

input_module_t mod_stats;

#define DEFAULT_STATS_PORT 10000

typedef struct statscfg_s statscfg_t;
struct statscfg_s
{
  int listen_port;

  /* some statistics ... */
};

static void
set_listen_port(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  int port;

  port = atoi(dir->args);
  DebugLog(DF_MOD, DS_INFO, "setting tcp listen port on %i\n", port);

  if (port > 0)
    ((radmcfg_t *)mod->config)->listen_port = port;
}

static mod_cfg_cmd_t stats_dir[] = {
  { "ListenPort", set_listen_port, "Set listen port for remonte admin" },
  { NULL, NULL }
};

input_module_t mod_remoteadm = {
  MOD_MAGIC,
  ORCHIDS_VERSION,
  "stats",
  "CeCILL2",
  NULL,
  stats_dir,
  NULL,
  NULL,
  NULL
};


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
