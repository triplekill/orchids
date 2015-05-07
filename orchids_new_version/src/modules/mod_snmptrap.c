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

static int snmptrap_dissect(orchids_t *ctx, mod_entry_t *mod, event_t *e, void *data)
{
  unsigned char *packet;
  size_t packet_len;
  netsnmp_pdu pdu;
  netsnmp_variable_list *vars;
  unsigned char type;
  /* long version; */
  int ret;
  int var;
  gc_t *gc_ctx = ctx->gc_ctx;
  ovm_var_t *val;

  type = 0;
  ret = 0;

  DebugLog(DF_MOD, DS_TRACE, "snmptrap_dissect()\n");

  switch (TYPE(e))
    {
    case T_BSTR: packet = BSTR(e->value);
      packet_len = BSTRLEN(e->value);
      break;
    case T_VBSTR: packet = VBSTR(e->value);
      packet_len = VBSTRLEN(e->value);
      break;
    default:
      DebugLog(DF_MOD, DS_DEBUG, "event not a binary string\n");
      return -1;
    }

  GC_START(gc_ctx, SNMPTRAP_FIELDS+1);
  GC_UPDATE(gc_ctx, SNMPTRAP_FIELDS, e);

  /* -0- parse asn.1 sequence */
  packet = asn_parse_sequence(packet, &packet_len, &type,
			      (ASN_SEQUENCE | ASN_CONSTRUCTOR),
			      "auth message");
  if (packet == NULL)
    {
      DebugLog(DF_MOD, DS_ERROR, "Bad ASN.1 sequence\n");
      ret = 1;
      goto end;
    }
  /* -1- parse asn.1 int version */
  {
    long ver;

    packet = asn_parse_int(packet, &packet_len, &type, &ver, sizeof(ver));
    if (packet == NULL)
      {
	DebugLog(DF_MOD, DS_ERROR, "Bad ASN.1 VERSION\n");
	ret = 1;
	goto end;
      }
    DebugLog(DF_MOD, DS_DEBUG, "Read SNMP VERSION %li\n", ver);
    /* handle only snmptrap v1 */
    if (ver != SNMP_VERSION_1)
      {
	/* && INT(attr[F_VERSION]) != SNMP_VERSION_2c */
	DebugLog(DF_MOD, DS_ERROR,
		 "Bad SNMP VERSION %li\n", ver);
	ret = 1;
	goto end;
      }
    val = ovm_int_new (gc_ctx, ver);
    GC_UPDATE (gc_ctx, F_VERSION, val);
  }

  /* -3- v1 v2c: read community string object */
  val = ovm_str_new (gc_ctx, COMMUNITY_MAX_LEN);
  packet = asn_parse_string(packet, &packet_len, &type,
			    (unsigned char *)&STR(val), &STRLEN(val));
  if (packet == NULL)
    {
      DebugLog(DF_MOD, DS_ERROR, "Bad community name\n");
      ret = 1;
      goto end;
    }
  DebugLog(DF_MOD, DS_INFO, "Read community %s\n", STR(val));
  GC_UPDATE (gc_ctx,F_COMMUNITY, val);

  /* -4- Parse PDU */
  /* dissect event top attribute here, and add them to it */
  memset(&pdu, 0, sizeof (struct snmp_pdu));
  ret = snmp_pdu_parse(&pdu, packet, &packet_len);
  if (ret != 0)
    {
      DebugLog(DF_MOD, DS_ERROR, "snmp_pdu_parse() error: ret=%i\n", ret);
      ret = 1;
      goto end;
    }

  val = ovm_int_new (gc_ctx, mod->posts);
  GC_UPDATE(gc_ctx, F_SEQ, val);

  val = ovm_snmpoid_new(gc_ctx, pdu.enterprise_length);
  GC_UPDATE(gc_ctx, F_ENTERPRISE, val);
  memcpy(SNMPOID(val),
         pdu.enterprise,
         pdu.enterprise_length * sizeof (oid_t));

  val = ovm_int_new (gc_ctx, pdu.trap_type);
  GC_UPDATE(gc_ctx, F_TRAPTYPE, val);

  val = ovm_int_new (gc_ctx, pdu.specific_type);
  GC_UPDATE(gc_ctx, F_SPECIFIC_TRAPTYPE, val);

  val = ovm_ipv4_new (gc_ctx);
  IPV4(val).s_addr = htonl (*(in_addr_t *)pdu.agent_addr);
  /* looking from code at
     http://www.opensource.apple.com/source/net_snmp/net_snmp-9/net-snmp/apps/snmptrapd.c
     the char agent_addr[4] array is just the in_addr_t we look for,
     regardless of endianness;
     (witness the following excerpt of the above:
     host = gethostbyaddr((char *) pdu->agent_addr, 4, AF_INET);
     )
  */
  GC_UPDATE(gc_ctx, F_AGENT_ADDR, val);

  val = ovm_int_new (gc_ctx, pdu.time);
  GC_UPDATE(gc_ctx, F_TIMESTAMP, val);

/*   for (var = 0, vars = pdu.variables; vars; vars = vars->next_variable) { */
  vars = pdu.variables;
  var = 0;
  if (vars!=NULL)
    {
      DebugLog(DF_MOD, DS_DEBUG, "Process variables\n");

      val = ovm_snmpoid_new (gc_ctx, vars->name_length);
      memcpy(SNMPOID(val),
	     vars->name,
	     vars->name_length * sizeof (oid_t));
    }

  /* then, post resulting event */
  REGISTER_EVENTS(ctx, mod, SNMPTRAP_FIELDS);
 end:
  GC_END (gc_ctx);
  return ret;
}

static field_t snmptrap_fields[] = {
  /* common fields */
  { "snmptrap.seq",          &t_int, MONO_MONO, "Sequence number" },
  { "snmptrap.bind",         &t_int, MONO_UNKNOWN, "Index of binding" },
  { "snmptrap.version",      &t_int, MONO_UNKNOWN, "Protocol version" },
  { "snmptrap.community",    &t_str, MONO_UNKNOWN, "Community name" },
  { "snmptrap.pdu_type",     &t_str, MONO_UNKNOWN, "Protocol Data Unit type" },
  { "snmptrap.object_id",    &t_snmpoid, MONO_UNKNOWN, "Object identifier" },
  { "snmptrap.value",        &t_bstr, MONO_UNKNOWN, "Object value" },

  /* Version 1 Specific */
  { "snmptrap.enterprise",   &t_snmpoid, MONO_UNKNOWN, "Identifies the trap source" },
  { "snmptrap.agent_addr",   &t_ipv4, MONO_UNKNOWN, "Source address" },
  { "snmptrap.trap_type",    &t_int, MONO_UNKNOWN, "Trap type" },
  { "snmptrap.specific_trap_type",    &t_int, MONO_UNKNOWN, "Specific trap type" },
  { "snmptrap.timestamp",    &t_int, MONO_UNKNOWN, "Timestamp" },

  /* Version 2 Specific */
  { "snmptrap.request_id",   &t_uint, MONO_UNKNOWN, "Request ID" },
  { "snmptrap.error_status", &t_uint, MONO_UNKNOWN, "Error status" },
  { "snmptrap.error_index",  &t_uint, MONO_UNKNOWN, "Error index" },
};

static void *
snmptrap_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  int *port;

  DebugLog(DF_MOD, DS_INFO, "load() snmptrap@%p\n", &mod_snmptrap);

/*   init_snmp("mod_snmptrap"); */
  netsnmp_init_mib();  /* used to be:  init_mib(),
			  but that seems to be deprecated */
#ifdef ORCHIDS_DEBUG
  snmp_set_do_debugging(10);
#endif /* ORCHIDS_DEBUG */

  /* hard coded callback registration.
  ** optional goes in config directives */
  port = gc_base_malloc(ctx->gc_ctx, sizeof (int));
  *port = 162;
  register_conditional_dissector(ctx, mod, "udp",
				 (void *)port, sizeof (int),
                                 NULL, "snmptrap(no file)", 0);

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

  t = netsnmp_read_module (dir->args); /* was:  t = read_module(dir->args),
					  but that seems to be deprecated */
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
  0,			    /* flags */
  "snmptrap",               /* module name */
  "CeCILL2",                /* module license */
  snmptrap_dependencies,    /* module dependencies */
  snmptrap_config_commands, /* module configuration commands,
                               for core config parser */
  snmptrap_preconfig,       /* called just after module registration */
  snmptrap_postconfig,      /* called after all mods preconfig,
                               and after all module configuration*/
  NULL,
  snmptrap_dissect,
  &t_bstr		    /* type of fields it expects to dissect */
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
