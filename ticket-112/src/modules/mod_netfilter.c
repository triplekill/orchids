/**
 ** @file mod_netfilter.c
 ** Netfilter log module.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 ** @date  Started on: Wed Nov  5 16:18:07 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "orchids.h"

#include "orchids_api.h"

static char *nf_hook_g = NULL;

extern void netfilter_set_vstr_base(char *str);
extern void netfilter_set_str(char *str, size_t s);
extern int netfilterparse(void);
extern void netfilter_set_attrs(ovm_var_t **attr_fields);
extern void netfilter_reset(void);

#define NETFILTER_FIELDS 30
#define F_IN           0
#define F_PHYSIN       1
#define F_OUT          2
#define F_PHYSOUT      3
#define F_MAC          4
#define F_SRC          5
#define F_DST          6
#define F_IPLEN        7
#define F_TOS          8
#define F_PREC         9
#define F_TTL         10
#define F_IPID        11
#define F_IPFLAGS     12
#define F_FRAG        13
#define F_IPOPTS      14
#define F_PROTO       15
#define F_SPT         16
#define F_DPT         17
#define F_SEQ         18
#define F_ACK         19
#define F_WINDOW      20
#define F_RES         21
#define F_TCPFLAGS    22
#define F_URGP        23
#define F_UDPLEN      24
#define F_ICMPTYPE    25
#define F_ICMPCODE    26
#define F_ICMPPARAM   27
#define F_ICMPGATEWAY 28
#define F_ICMPMTU     29

input_module_t mod_netfilter;


static int
netfilter_dissect(orchids_t *ctx, mod_entry_t *mod, event_t *event, void *data)
{
  char *txt_line;
  size_t txt_len;
  ovm_var_t *attr[NETFILTER_FIELDS];
  int ret;

  DebugLog(DF_MOD, DS_DEBUG, "netfilter_dissect()\n");

  memset(attr, 0, sizeof(attr));

  if ( TYPE(event->value) == T_STR ) {
    txt_line =    STR(event->value);
    txt_len  = STRLEN(event->value);
  } else if ( TYPE(event->value) == T_VSTR ) {
    txt_line =    VSTR(event->value);
    txt_len  = VSTRLEN(event->value);
  } else {
    DebugLog(DF_MOD, DS_WARN, "type error\n");
/*     exit(EXIT_FAILURE); */
    return (1);
  }

  netfilter_set_str(txt_line, txt_len);
  netfilter_set_vstr_base(txt_line);
  netfilter_set_attrs(attr);
  ret = netfilterparse();
  netfilter_reset();
  if (ret != 0) {
    DebugLog(DF_MOD, DS_WARN, "parse error\n");
    free_fields(attr, NETFILTER_FIELDS);
    return (1);
  }

  add_fields_to_event(ctx, mod, &event, attr, NETFILTER_FIELDS);

  post_event(ctx, mod, event);

  return (0);
}


static field_t netfilter_fields[] = {
  { "netfilter.in",           T_VSTR, "Input interface"                      },
  { "netfilter.physin",       T_VSTR, "Physical input interface"             },
  { "netfilter.out",          T_VSTR, "Output interface"                     },
  { "netfilter.physout",      T_VSTR, "Physical output interface"            },
  { "netfilter.mac",          T_VSTR, "Ethernet header"                      },
  { "netfilter.src",          T_IPV4, "IP source address"                    },
  { "netfilter.dst",          T_IPV4, "IP destination address"               },
  { "netfilter.ip_len",       T_INT,  "IP Packet length"                     },
  { "netfilter.tos",          T_INT,  "Type of service ('type' field)"       },
  { "netfilter.prec",         T_INT,  "Type of service ('precedence' field)" },
  { "netfilter.ttl",          T_INT,  "Time to live"                         },
  { "netfilter.ipid",         T_INT,  "Packet identifier (ipid)"             },
  { "netfilter.ip_flags",     T_INT,  "IP flags: CE/DF/MF"                   },
  { "netfilter.frag",         T_INT,  "IP Framgentation offset"              },
  { "netfilter.ip_opts",      T_VSTR, "IP Options"                           },
  { "netfilter.proto",        T_VSTR, "Protocol encapsuled in IP"            },
  { "netfilter.spt",          T_INT,  "TCP/UDP source port"                  },
  { "netfilter.dpt",          T_INT,  "TCP/UDP destination port"             },
  { "netfilter.seq",          T_INT,  "TCP sequence number"                  },
  { "netfilter.ack",          T_INT,  "TCP ackownledged seq. number"         },
  { "netfilter.window",       T_INT,  "TCP window size"                      },
  { "netfilter.res",          T_INT,  "TCP reserved bits"                    },
  { "netfilter.tcp_flags",    T_INT,  "CWR/ECE/URG/ACK/PSH/RST/SYN/FIN"      },
  { "netfilter.urgp",         T_INT,  "TCP urgent pointer"                   },
  { "netfilter.udp_len",      T_INT,  "UDP Packet length"                    },
  { "netfilter.icmp_type",    T_INT,  "ICMP message type"                    },
  { "netfilter.icmp_code",    T_INT,  "ICMP message code"                    },
  { "netfilter.icmp_id",      T_INT,  "ICMP ID"                              },
  { "netfilter.icmp_seq",     T_INT,  "ICMP sequence number"                 },
  { "netfilter.icmp_param",   T_INT,  "ICMP parameter problem code"          },
  { "netfilter.icmp_gateway", T_IPV4, "ICMP redirect gateway address"        },
  { "netfilter.icmp_mtu",     T_INT,  "ICMP Frag. needed MTU"                },
};


static void *
netfilter_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  DebugLog(DF_MOD, DS_DEBUG, "load() netfilter@%p\n", (void *) &mod_netfilter);

  return (NULL);
}

int
generic_dissect(orchids_t *ctx, mod_entry_t *mod, event_t *event, void *data)
{
  return netfilter_dissect(ctx, mod, event, data);
}

static void
add_hook(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  DebugLog(DF_MOD, DS_DEBUG, "Adding unconditionnal hook on module %s\n",
           dir->args);

  /* register at post-config time, once vmod is created */
  nf_hook_g = dir->args;
}

static void
netfilter_postconfig(orchids_t *ctx, mod_entry_t *mod)
{
  DebugLog(DF_MOD, DS_TRACE,
           "netfilter_postconfig() -- registering hook.\n");

  register_fields(ctx, mod, netfilter_fields, NETFILTER_FIELDS);

  if (nf_hook_g)
    register_dissector(ctx, mod, nf_hook_g, netfilter_dissect, NULL);
}

static mod_cfg_cmd_t netfilter_cfgcmds[] =
{
  { "AddHook", add_hook, "Add an unconditionnal hook" },
  { NULL, NULL }
};

static char *netfilter_deps[] = {
  "udp",
  "textfile",
  NULL
};

input_module_t mod_netfilter = {
  MOD_MAGIC,
  ORCHIDS_VERSION,
  "netfilter",
  "CeCILL2",
  netfilter_deps,
  netfilter_cfgcmds,
  netfilter_preconfig,
  netfilter_postconfig,
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
