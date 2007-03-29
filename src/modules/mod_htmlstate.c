/**
 ** @file mod_htmlstate.c
 ** Generate HTML pages of the internal state of Orchids.
 ** Used to test/debug/orchids internals at runtime.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 ** @date  Started on: Mon Jan 27 17:32:49 2003
 ** @date Last update: Thu Mar 29 15:24:33 2007
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>

#include "orchids.h"

input_module_t mod_htmlstate;

typedef struct htmlstatecfg_s htmlstatecfg_t;
struct htmlstatecfg_s
{
  int enable_cache;
  int fork;
};

static void *
htmlstate_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  htmlstatecfg_t *mod_cfg;

  DebugLog(DF_MOD, DS_INFO,
           "loading htmlstate module @ %p\n", (void *) &mod_htmlstate);

  mod_cfg = Xzmalloc(sizeof (htmlstatecfg_t));
  /* default initialisation here */

  html_output_preconfig(ctx); /* XXX: Move this in a module */

  return (mod_cfg);
}

static void
htmlstate_postconfig(orchids_t *ctx, mod_entry_t *mod)
{
  /* configure cache */
}

static void
enable_cache(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  ((htmlstatecfg_t *)mod->config)->enable_cache = 1;
}

static mod_cfg_cmd_t htmlstate_dir[] = 
{
  { "EnableCache", enable_cache, "Enable file cache" },
  { NULL, NULL }
};

input_module_t mod_htmlstate = {
  MOD_MAGIC,
  ORCHIDS_VERSION,
  "htmlstate",
  "CeCILL2",
  NULL,
  htmlstate_dir,
  htmlstate_preconfig,
  htmlstate_postconfig,
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
