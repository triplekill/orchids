/**
 ** @file mod_cisco.c
 ** A parser for system message of cisco equipments.
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

#include "mod_cisco.h"

input_module_t mod_cisco;

static int
cisco_dissector(orchids_t *ctx, mod_entry_t *mod, event_t *e, void *data)
{
  DebugLog(DF_MOD, DS_DEBUG, "cisco_dissector()\n");

  /* dissect event top attribute here, and add them to it */

  /* then, post resulting event */
  post_event(ctx, mod, e);

  return (0);
}

static field_t cisco_fields[] = {
  { "cisco.msg_type", T_INT,  "Cisco message type"   },
  { "cisco.acl",      T_VSTR, "Access control list" },
  { "cisco.action",   T_VSTR, "Action (permitted/denied)" },
  { "cisco.proto",    T_VSTR, "Protocol" },
  { "cisco.sip",      T_IPV4, "Source IP address" },
  { "cisco.dip",      T_IPV4, "Destiantion IP address" },
  { "cisco.spt",      T_INT,  "Source port" },
  { "cisco.dpt",      T_INT,  "Destination port" },
  { "cisco.packets",  T_INT,  "Number of packets" },
};

static void *
cisco_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  DebugLog(DF_MOD, DS_DEBUG, "load() cisco@%p\n", &mod_cisco);

  /* hard coded callback registration.
  ** optionnal goes in config directives */
  //register_dissector(ctx, "parent", cisco_dissector);
  //register_conditional_dissector(ctx, "parent", (void *)"messages", 8,
  //                               dissect_syslog);

  register_fields(ctx, mod, cisco_fields, CISCO_FIELDS);

  do {
    dissect_t dummy;
    dummy = cisco_dissector;
  } while (0);

  return (NULL);
}

static void
cisco_postconfig(orchids_t *ctx, mod_entry_t *mod)
{
  /* Do all thing needed _AFTER_ module configuration.
  ** (register configurable callbacks for examples) */
}

static void
cisco_postcompil(orchids_t *ctx, mod_entry_t *mod)
{
  /* Do all thing needed _AFTER_ rule compilation. */
}


input_module_t mod_cisco = {
  MOD_MAGIC,                /* Magic number */
  ORCHIDS_VERSION,          /* Module version */
  "cisco",                  /* module name */
  "CeCILL2",                /* module license */
  NULL,                     /* module dependencies */
  NULL,                     /* module configuration commands,
                               for core config parser */
  cisco_preconfig,          /* called just after module registration */
  cisco_postconfig,         /* called after all mods preconfig,
                               and after all module configuration*/
  cisco_postcompil,
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
