/**
 ** @file mod_snmp.c
 ** A snmp module.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 ** @date  Started on: Fri Feb  7 11:07:42 2003
 ** @date Last update: Sat Sep  8 19:41:28 2007
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

#include "mod_snmp.h"


input_module_t mod_snmp;


static int
snmp_dissector(orchids_t *ctx, mod_entry_t *mod, event_t *e, void *data)
{
  DebugLog(DF_MOD, DS_TRACE, "snmp_dissector()\n");

  /* dissect event top attribute here, and add them to it */

  /* then, post resulting event */
  post_event(ctx, mod, e);

  return (0);
}


static field_t snmp_fields[] = {
  { "snmp.version",      T_VSTR, "Protocol version" },
  { "snmp.community",    T_VSTR, "Community name" },
  { "snmp.pdu_type",     T_VSTR, "Protocol Data Unit type" },
  { "snmp.request_id",   T_UINT, "Request ID" },
  { "snmp.error_status", T_UINT, "Error status" },
  { "snmp.error_index",  T_UINT, "Error index" },
  { "snmp.object_id",    T_SNMPOID, "Object identifier" },
  { "snmp.value",        T_VBINSTR, "Object value" },
};


static void *
snmp_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  snmp_config_t *cfg;

  DebugLog(DF_MOD, DS_INFO, "load() snmp@%p\n", &mod_snmp);

  /* allocate some memory for module configuration
  ** and initialize default configuration. */
  cfg = Xzmalloc(sizeof (snmp_config_t));
  cfg->some_flag = DEFAULT_FLAG;
  cfg->some_option = DEFAULT_OPTION;

  /* hard coded callback registration.
  ** optionnal goes in config directives */
  //register_dissector(ctx, mod, "udp", snmp_dissector, NULL);
  //register_conditional_dissector(ctx, mod, "udp", (void *)"messages", 8,
  //                               dissect_syslog, NULL);

  register_fields(ctx, mod, snmp_fields, SNMP_FIELDS);

  do {
    dissect_t dummy;
    dummy = snmp_dissector;
  } while (0);

  /* return config structure, for module manager */
  return (cfg);
}


static void
snmp_postconfig(orchids_t *ctx, mod_entry_t *mod)
{
  /* Do all thing needed _AFTER_ module configuration.
  ** (register configurable callbacks for examples) */
}


static void
snmp_postcompil(orchids_t *ctx, mod_entry_t *mod)
{
  /* Do all thing needed _AFTER_ rule compilation. */
}


static mod_cfg_cmd_t snmp_config_commands[] = 
{
  { NULL, NULL, NULL }
};


static char *snmp_dependencies[] = {
  NULL
};


input_module_t mod_snmp = {
  MOD_MAGIC,                /* Magic number */
  ORCHIDS_VERSION,          /* Module version */
  "snmp",               /* module name */
  "CeCILL2",               /* module license */
  snmp_dependencies,    /* module dependencies */
  snmp_config_commands, /* module configuration commands,
                               for core config parser */
  snmp_preconfig,       /* called just after module registration */
  snmp_postconfig,       /* called after all mods preconfig,
                               and after all module configuration*/
  snmp_postcompil
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
