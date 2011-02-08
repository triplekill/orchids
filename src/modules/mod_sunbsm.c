/**
 ** @file mod_sunbsm.c
 ** Module for reading Sun BSM (Security Basic Modules) audit files.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
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

#include "orchids.h"

#include "orchids_api.h"

input_module_t mod_sunbsm;

#define DEFAULT_FLAG 123
#define DEFAULT_OPTION 456

#define SUNBSM_FIELDS 2
#define F_DEMOINT 0
#define F_DEMOSTR 1

typedef struct sunbsm_config_s sunbsm_config_t;
struct sunbsm_config_s
{
  int some_flag;
  int some_option;
};

static int
sunbsm_dissector(orchids_t *ctx, mod_entry_t *mod, event_t *e, void *data)
{
  DebugLog(DF_MOD, DS_TRACE, "sunbsm_dissector()\n");

  /* dissect event top attribute here, and add them to it */

  /* then, post resulting event */
  post_event(ctx, mod, e);

  return (0);
}

static field_t sunbsm_fields[] = {
  { "sunbsm.field_demoint", T_INT,  "an int field"   },
  { "sunbsm.field_demostr", T_VSTR, "a string field" },
};

static void *
sunbsm_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  sunbsm_config_t *cfg;

  DebugLog(DF_MOD, DS_INFO, "load() sunbsm@%p\n", &mod_sunbsm);

  /* allocate some memory for module configuration
  ** and initialize default configuration. */
  cfg = Xzmalloc(sizeof (sunbsm_config_t));
  cfg->some_flag = DEFAULT_FLAG;
  cfg->some_option = DEFAULT_OPTION;

  /* hard coded callback registration.
  ** optionnal goes in config directives */
  //register_dissector(ctx, mod, "parent", sunbsm_dissector, NULL);
  //register_conditional_dissector(ctx, mod, "parent", (void *)"messages", 8,
  //                               dissect_syslog, NULL);

  register_fields(ctx, mod, sunbsm_fields, SUNBSM_FIELDS);

  /* return config structure, for module manager */

  do {
    dissect_t dummy;
    dummy = sunbsm_dissector;
  } while (0);

  return (cfg);
}

static void
sunbsm_postconfig(orchids_t *ctx, mod_entry_t *mod)
{
  /* Do all thing needed _AFTER_ module configuration.
  ** (register configurable callbacks for examples) */
}

static void
sunbsm_postcompil(orchids_t *ctx, mod_entry_t *mod)
{
  /* Do all thing needed _AFTER_ rule ocmpilation. */
}

static void
set_some_flag(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  int someflag;

  someflag = atoi(dir->args);
  DebugLog(DF_MOD, DS_INFO, "setting some_flag to %i\n", someflag);

  ((sunbsm_config_t *)mod->config)->some_flag = someflag;
}

static void
set_some_option(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  int someoption;

  someoption = atoi(dir->args);
  DebugLog(DF_MOD, DS_INFO, "setting some_option to %i\n", someoption);

  ((sunbsm_config_t *)mod->config)->some_option = someoption;
}

static mod_cfg_cmd_t sunbsm_config_commands[] = 
{
  { "SomeFlag", set_some_flag, "Set some_flag value" },
  { "SomeOption", set_some_option, "Set some_option value" },
  { NULL, NULL, NULL }
};

static char *sunbsm_dependencies[] = {
  NULL
};

input_module_t mod_sunbsm = {
  MOD_MAGIC,                /* Magic number */
  ORCHIDS_VERSION,          /* Module version */
  "sunbsm",                 /* module name */
  "CeCILL2",                /* module license */
  sunbsm_dependencies,      /* module dependencies */
  sunbsm_config_commands,   /* module configuration commands,
                               for core config parser */
  sunbsm_preconfig,         /* called just after module registration */
  sunbsm_postconfig,         /* called after all mods preconfig,
                               and after all module configuration*/
  sunbsm_postcompil
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
