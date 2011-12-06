/**
 ** @file mod_snmptrap.c
 ** A snmptrap module.
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

/*
 * send a test trap with
 * $ snmptrap -v 1 -c public dell67001 \
      "" "" 0 0  ""  \
      1.3.6.1.4.1.3.1.1 i 123 \
      1.3.6.1.4.1.3.1.2 i 456
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_NETSNMP

#include <stdlib.h>
#include <stdio.h>

#include <string.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

#include "orchids.h"

#include "orchids_api.h"

#include "mod_snmptrap.h"

input_module_t mod_snmptrap;


/* static long seq_g = 0; */

static int
snmptrap_dissector(orchids_t *ctx, mod_entry_t *mod, event_t *e, void *data)
{
  ovm_var_t *attr[SNMPTRAP_FIELDS];
  unsigned char *packet;
  size_t packet_len;
  netsnmp_pdu pdu;
  netsnmp_variable_list *vars;
  unsigned char type;
  /* long version; */
  int ret;
  unsigned char community[COMMUNITY_MAX_LEN];
  size_t community_len = COMMUNITY_MAX_LEN;
  int var;

  packet = BSTR(e->value);
  packet_len = BSTRLEN(e->value);
  type = 0;
  memset(attr, 0, sizeof(attr));

  DebugLog(DF_MOD, DS_TRACE, "snmptrap_dissector()\n");

  /* -0- parse asn.1 sequence */
  packet = asn_parse_sequence(packet, &packet_len, &type,
                            (ASN_SEQUENCE | ASN_CONSTRUCTOR),
                            "auth message");
  if (packet == NULL) {
    DebugLog(DF_MOD, DS_ERROR, "Bad ASN.1 sequence\n");
    return (1);
  }

  /* -1- parse asn.1 int version */
  attr[F_VERSION] = ovm_int_new();
  packet = asn_parse_int(packet, &packet_len, &type,
                       &INT(attr[F_VERSION]), sizeof(INT(attr[F_VERSION])));
  if (packet == NULL) {
    DebugLog(DF_MOD, DS_ERROR, "Bad ASN.1 VERSION\n");
    free_fields(attr, SNMPTRAP_FIELDS);
    return (1);
  }

  /* handle only snmptrap v1 */
  if (INT(attr[F_VERSION]) != SNMP_VERSION_1) {
    /* && INT(attr[F_VERSION]) != SNMP_VERSION_2c */
    DebugLog(DF_MOD, DS_ERROR, "Bad SNMP VERSION %li\n", INT(attr[F_VERSION]));
    free_fields(attr, SNMPTRAP_FIELDS);
    return (1);
  }

  DebugLog(DF_MOD, DS_DEBUG, "Read SNMP VERSION %li\n", INT(attr[F_VERSION]));

  /* -3- v1 v2c: read community string object */
  packet = asn_parse_string(packet, &packet_len, &type, community, &community_len);
  if (packet == NULL) {
    DebugLog(DF_MOD, DS_ERROR, "Bad community name\n");
    free_fields(attr, SNMPTRAP_FIELDS);
    return (1);
  }
  DebugLog(DF_MOD, DS_INFO, "Read community %s\n", community);
  attr[F_COMMUNITY] = ovm_str_new( community_len );
  memcpy(STR(attr[F_COMMUNITY]), community, community_len);

  /* -4- Parse PDU */
  /* dissect event top attribute here, and add them to it */
  memset(&pdu, 0, sizeof (struct snmp_pdu));
  ret = snmp_pdu_parse(&pdu, packet, &packet_len);
  if (ret != 0) {
    DebugLog(DF_MOD, DS_ERROR, "snmp_pdu_parse() error: ret=%i\n", ret);
    free_fields(attr, SNMPTRAP_FIELDS);
    return (1);
  }

  attr[F_SEQ] = ovm_int_new();
  INT(attr[F_SEQ]) = mod->posts;

  attr[F_ENTERPRISE] = ovm_snmpoid_new(pdu.enterprise_length);
  memcpy(SNMPOID(attr[F_ENTERPRISE]),
         pdu.enterprise,
         pdu.enterprise_length * sizeof (oid_t));

  attr[F_TRAPTYPE] = ovm_int_new();
  INT(attr[F_TRAPTYPE]) = pdu.trap_type;

  attr[F_SPECIFIC_TRAPTYPE] = ovm_int_new();
  INT(attr[F_SPECIFIC_TRAPTYPE]) = pdu.specific_type;

  attr[F_AGENT_ADDR] = ovm_ipv4_new();
  memcpy(&IPV4(attr[F_AGENT_ADDR]), pdu.agent_addr, sizeof (struct in_addr));

  attr[F_TIMESTAMP] = ovm_int_new();
  INT(attr[F_TIMESTAMP]) = pdu.time;

/*   for (var = 0, vars = pdu.variables; vars; vars = vars->next_variable) { */
  vars = pdu.variables;
  var = 0;
  if (vars) {
    DebugLog(DF_MOD, DS_DEBUG, "Process variable %i\n", var);

    attr[F_OBJECTID] = ovm_snmpoid_new( vars->name_length );
    memcpy(SNMPOID(attr[F_OBJECTID]),
           vars->name,
           vars->name_length * sizeof (oid_t));
  }

  /* then, post resulting event */
  add_fields_to_event(ctx, mod, &e, attr, SNMPTRAP_FIELDS);
  post_event(ctx, mod, e);

  return (0);
}

static field_t snmptrap_fields[] = {
  /* common fields */
  { "snmptrap.seq",          T_INT, "Sequence number" },
  { "snmptrap.bind",         T_INT, "Index of binding" },
  { "snmptrap.version",      T_INT, "Protocol version" },
  { "snmptrap.community",    T_STR, "Community name" },
  { "snmptrap.pdu_type",     T_VSTR, "Protocol Data Unit type" },
  { "snmptrap.object_id",    T_SNMPOID, "Object identifier" },
  { "snmptrap.value",        T_VBSTR, "Object value" },

  /* Version 1 Specific */
  { "snmptrap.enterprise",   T_SNMPOID, "Identifies the trap source" },
  { "snmptrap.agent_addr",   T_IPV4, "Source address" },
  { "snmptrap.trap_type",    T_INT, "Trap type" },
  { "snmptrap.specific_trap_type",    T_INT, "Specific trap type" },
  { "snmptrap.timestamp",    T_INT, "Timestamp" },

  /* Version 2 Specific */
  { "snmptrap.request_id",   T_UINT, "Request ID" },
  { "snmptrap.error_status", T_UINT, "Error status" },
  { "snmptrap.error_index",  T_UINT, "Error index" },
};

static void *
snmptrap_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  int *port;

  DebugLog(DF_MOD, DS_INFO, "load() snmptrap@%p\n", &mod_snmptrap);

/*   init_snmp("mod_snmptrap"); */
  init_mib();
#ifdef ORCHIDS_DEBUG
  snmp_set_do_debugging(10);
#endif /* ORCHIDS_DEBUG */

  /* hard coded callback registration.
  ** optionnal goes in config directives */
  port = Xzmalloc( sizeof (int) );
  *port = 162;
  register_conditional_dissector(ctx, mod, "udp", (void *)port, sizeof (int),
                                 snmptrap_dissector, NULL);

  register_fields(ctx, mod, snmptrap_fields, SNMPTRAP_FIELDS);

  /* return config structure, for module manager */
  return (NULL);
}

static void
snmptrap_postconfig(orchids_t *ctx, mod_entry_t *mod)
{
  /* Do all thing needed _AFTER_ module configuration.
  ** (register configurable callbacks for examples) */
}

static void
add_mib_dir(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  int ret;

  DebugLog(DF_MOD, DS_INFO, "Adding MIB directory '%s'\n", dir->args);

  ret = add_mibdir(dir->args);
  if (ret == -1) {
    DebugLog(DF_MOD, DS_ERROR, "add_mibdir(%s) error.\n", dir->args);
  }
  else {
    DebugLog(DF_MOD, DS_ERROR, "Found %i files in '%s'.\n", ret, dir->args);
  }
}

static void
add_mib(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  struct tree *t;

  DebugLog(DF_MOD, DS_INFO, "Adding MIB module '%s'\n", dir->args);

  t = read_module(dir->args);
  if (!t) {
    DebugLog(DF_MOD, DS_ERROR, "Error while adding MIB module '%s'\n", dir->args);
  }
}

static void
add_mib_file(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  struct tree *t;

  DebugLog(DF_MOD, DS_INFO, "Adding MIB file '%s'\n", dir->args);

  t = read_mib(dir->args);
  if (!t) {
    DebugLog(DF_MOD, DS_ERROR, "Error while adding MIB file '%s'\n", dir->args);
  }
}

static mod_cfg_cmd_t snmptrap_config_commands[] = {
  { "AddMibDir", add_mib_dir, "Add a MIB directory" },
  { "AddMib", add_mib, "Add a MIB module" },
  { "AddMibFile", add_mib_file, "Add a MIB file" },
  { NULL, NULL, NULL }
};

static char *snmptrap_dependencies[] = {
  "udp",
};

input_module_t mod_snmptrap = {
  MOD_MAGIC,                /* Magic number */
  ORCHIDS_VERSION,          /* Module version */
  "snmptrap",               /* module name */
  "CeCILL2",                /* module license */
  snmptrap_dependencies,    /* module dependencies */
  snmptrap_config_commands, /* module configuration commands,
                               for core config parser */
  snmptrap_preconfig,       /* called just after module registration */
  snmptrap_postconfig,       /* called after all mods preconfig,
                               and after all module configuration*/
  NULL
};

#endif /* HAVE_SNMP */



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
