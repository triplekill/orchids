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

extern void netfilter_set_gc_ctx(gc_t *gc_ctx);
extern void netfilter_set_delegate(ovm_var_t *delegate);
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


static int netfilter_dissect(orchids_t *ctx, mod_entry_t *mod,
			     event_t *event, void *data)
{
  char *txt_line;
  size_t txt_len;
  int ret;
  gc_t *gc_ctx = ctx->gc_ctx;

  DebugLog(DF_MOD, DS_DEBUG, "netfilter_dissect()\n");

  if (event->value==NULL)
    {
      DebugLog(DF_MOD, DS_DEBUG, "NULL event value\n");
      return -1;
    }
  switch (TYPE(event->value))
    {
    case T_STR: txt_line = STR(event->value); txt_len = STRLEN(event->value);
      break;
    case T_VSTR: txt_line = VSTR(event->value); txt_len = VSTRLEN(event->value);
      break;
    default:
      DebugLog(DF_MOD, DS_DEBUG, "event value not a string\n");
      return -1;
    }

  GC_START(gc_ctx, NETFILTER_FIELDS+1);
  GC_UPDATE(gc_ctx, NETFILTER_FIELDS, event);
  ret = 0;

  netfilter_set_gc_ctx(gc_ctx);
  netfilter_set_delegate(event->value);
  netfilter_set_str(txt_line, txt_len);
  netfilter_set_vstr_base(txt_line);
  netfilter_set_attrs((ovm_var_t **)&GC_LOOKUP(0));
  ret = netfilterparse();
  netfilter_reset();
  if (ret != 0)
    DebugLog(DF_MOD, DS_WARN, "parse error\n");
  else
    REGISTER_EVENTS(ctx, mod, NETFILTER_FIELDS);
  GC_END(gc_ctx);
  return ret;
}


static field_t netfilter_fields[] = {
  { "netfilter.in",           &t_str, "Input interface"                      },
  { "netfilter.physin",       &t_str, "Physical input interface"             },
  { "netfilter.out",          &t_str, "Output interface"                     },
  { "netfilter.physout",      &t_str, "Physical output interface"            },
  { "netfilter.mac",          &t_str, "Ethernet header"                      },
  { "netfilter.src",          &t_ipv4, "IP source address"                    },
  { "netfilter.dst",          &t_ipv4, "IP destination address"               },
  { "netfilter.ip_len",       &t_int,  "IP Packet length"                     },
  { "netfilter.tos",          &t_int,  "Type of service ('type' field)"       },
  { "netfilter.prec",         &t_int,  "Type of service ('precedence' field)" },
  { "netfilter.ttl",          &t_int,  "Time to live"                         },
  { "netfilter.ipid",         &t_int,  "Packet identifier (ipid)"             },
  { "netfilter.ip_flags",     &t_int,  "IP flags: CE/DF/MF"                   },
  { "netfilter.frag",         &t_int,  "IP Framgentation offset"              },
  { "netfilter.ip_opts",      &t_str, "IP Options"                           },
  { "netfilter.proto",        &t_str, "Protocol encapsuled in IP"            },
  { "netfilter.spt",          &t_int,  "TCP/UDP source port"                  },
  { "netfilter.dpt",          &t_int,  "TCP/UDP destination port"             },
  { "netfilter.seq",          &t_int,  "TCP sequence number"                  },
  { "netfilter.ack",          &t_int,  "TCP ackownledged seq. number"         },
  { "netfilter.window",       &t_int,  "TCP window size"                      },
  { "netfilter.res",          &t_int,  "TCP reserved bits"                    },
  { "netfilter.tcp_flags",    &t_int,  "CWR/ECE/URG/ACK/PSH/RST/SYN/FIN"      },
  { "netfilter.urgp",         &t_int,  "TCP urgent pointer"                   },
  { "netfilter.udp_len",      &t_int,  "UDP Packet length"                    },
  { "netfilter.icmp_type",    &t_int,  "ICMP message type"                    },
  { "netfilter.icmp_code",    &t_int,  "ICMP message code"                    },
  { "netfilter.icmp_id",      &t_int,  "ICMP ID"                              },
  { "netfilter.icmp_seq",     &t_int,  "ICMP sequence number"                 },
  { "netfilter.icmp_param",   &t_int,  "ICMP parameter problem code"          },
  { "netfilter.icmp_gateway", &t_ipv4, "ICMP redirect gateway address"        },
  { "netfilter.icmp_mtu",     &t_int,  "ICMP Frag. needed MTU"                },
};


static void * netfilter_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  DebugLog(DF_MOD, DS_DEBUG, "load() netfilter@%p\n", (void *) &mod_netfilter);
  return NULL;
}

/*
int generic_dissect(orchids_t *ctx, mod_entry_t *mod, event_t *event,
		    void *data)
{
  return netfilter_dissect(ctx, mod, event, data);
}
*/

static void add_hook(orchids_t *ctx, mod_entry_t *mod,
		     config_directive_t *dir)
{
  DebugLog(DF_MOD, DS_DEBUG, "Adding unconditionnal hook on module %s\n",
           dir->args);

  /* register at post-config time, once vmod is created */
  nf_hook_g = dir->args;
}

static void netfilter_postconfig(orchids_t *ctx, mod_entry_t *mod)
{
  DebugLog(DF_MOD, DS_TRACE,
           "netfilter_postconfig() -- registering hook.\n");

  register_fields(ctx, mod, netfilter_fields, NETFILTER_FIELDS);
  if (nf_hook_g!=NULL)
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
  NULL,
  netfilter_dissect
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
