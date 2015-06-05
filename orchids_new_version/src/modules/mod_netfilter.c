/**
 ** @file mod_netfilter.c
 ** Netfilter log module.
 **
 ** @author Pierre-Arnaud SENTUCQ <pierre-arnaud.sentucq@lsv.ens-cachan.fr>
 **
 ** @version 0.2
 ** @ingroup modules
 **
 ** @date  Started on: Wed Dec 17 14:43:45 CET 2014
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <sys/types.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "orchids.h"
#include "orchids_api.h"
#include "mod_utils.h"

#include "mod_netfilter.h"

input_module_t mod_netfilter;

/*******************************************************************/

static action_t netfilter_actions[] = {
  {"IN=", F_IN, action_doer_string },
  {"PHYSIN=", F_PHYSIN, action_doer_string },
  {"OUT=", F_OUT, action_doer_string },
  {"PHYSOUT=", F_PHYSOUT, action_doer_string },
  {"MAC=", F_MAC, action_doer_string },
  {"SRC=", F_SRC, action_doer_ip },
  {"DST=", F_DST, action_doer_ip },
  {"IPLEN=", F_IPLEN, action_doer_int },
  {"TOS=", F_TOS, action_doer_int },
  {"PREC=", F_PREC, action_doer_int },
  {"TTL=", F_TTL, action_doer_int },
  {"IPID=", F_IPID, action_doer_int },
  {"IPFLAGS=", F_IPFLAGS, action_doer_int },
  {"FRAG=", F_FRAG, action_doer_int },
  {"IPOPTS=", F_IPOPTS, action_doer_string },
  {"PROTO=", F_PROTO, action_doer_string },
  {"SPT=", F_SPT, action_doer_int },
  {"DPT=", F_DPT, action_doer_int },
  {"SEQ=", F_SEQ, action_doer_int },
  {"ACK=", F_ACK, action_doer_int },
  {"WINDOW=", F_WINDOW, action_doer_int },
  {"RES=", F_RES, action_doer_int },
  {"TCPFLAGS=", F_TCPFLAGS, action_doer_int },
  {"URGP=", F_URGP, action_doer_int },
  {"UDPLEN=", F_UDPLEN, action_doer_int },
  {"ICMPTYPE=", F_ICMPTYPE, action_doer_int },
  {"ICMPCODE=", F_ICMPCODE, action_doer_int },
  {"ICMPPARAM=", F_ICMPPARAM, action_doer_int },
  {"ICMPGATEWAY=", F_ICMPGATEWAY, action_doer_ip },
  {"ICMPMTU=", F_ICMPMTU, action_doer_int }
};

static int netfilter_dissect(orchids_t *ctx, mod_entry_t *mod,
		event_t *event, void *data)
{
  char *txt_line;
  int txt_len;
  action_orchids_ctx_t *octx = mod->config;
  gc_t *gc_ctx = ctx->gc_ctx;

  GC_START(gc_ctx, 1);
  octx->in_event = event;
  octx->out_event = (event_t **)&GC_LOOKUP(0);
  GC_UPDATE(gc_ctx, 0, event);

  DebugLog(DF_MOD, DS_TRACE, "netfilter_callback()\n");
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

  action_parse_event (octx, txt_line, txt_line+txt_len);

  /* then, post the Orchids event */
  post_event(ctx, mod, *octx->out_event);
  GC_END(gc_ctx);
  return (0);
}

static field_t netfilter_fields[] = {
  { "netfilter.in",           &t_str,  MONO_UNKNOWN,  "Input interface"                      },
  { "netfilter.physin",       &t_str,  MONO_UNKNOWN,  "Physical input interface"             },
  { "netfilter.out",          &t_str,  MONO_UNKNOWN,  "Output interface"                     },
  { "netfilter.physout",      &t_str,  MONO_UNKNOWN,  "Physical output interface"            },
  { "netfilter.mac",          &t_str,  MONO_UNKNOWN,  "Ethernet header"                      },
  { "netfilter.src",          &t_ipv4, MONO_UNKNOWN,  "IP source address"                    },
  { "netfilter.dst",          &t_ipv4, MONO_UNKNOWN,  "IP destination address"               },
  { "netfilter.ip_len",       &t_int,  MONO_UNKNOWN,  "IP Packet length"                     },
  { "netfilter.tos",          &t_int,  MONO_UNKNOWN,  "Type of service ('type' field)"       },
  { "netfilter.prec",         &t_int,  MONO_UNKNOWN,  "Type of service ('precedence' field)" },
  { "netfilter.ttl",          &t_int,  MONO_UNKNOWN,  "Time to live"                         },
  { "netfilter.ipid",         &t_int,  MONO_UNKNOWN,  "Packet identifier (ipid)"             },
  { "netfilter.ip_flags",     &t_int,  MONO_UNKNOWN,  "IP flags: CE/DF/MF"                   },
  { "netfilter.frag",         &t_int,  MONO_UNKNOWN,  "IP Framgentation offset"              },
  { "netfilter.ip_opts",      &t_str,  MONO_UNKNOWN,  "IP Options"                           },
  { "netfilter.proto",        &t_str,  MONO_UNKNOWN,  "Protocol encapsulated in IP"          },
  { "netfilter.spt",          &t_int,  MONO_UNKNOWN,  "TCP/UDP source port"                  },
  { "netfilter.dpt",          &t_int,  MONO_UNKNOWN,  "TCP/UDP destination port"             },
  { "netfilter.seq",          &t_int,  MONO_UNKNOWN,  "TCP sequence number"                  },
  { "netfilter.ack",          &t_int,  MONO_UNKNOWN,  "TCP ackownledged seq. number"         },
  { "netfilter.window",       &t_int,  MONO_UNKNOWN,  "TCP window size"                      },
  { "netfilter.res",          &t_int,  MONO_UNKNOWN,  "TCP reserved bits"                    },
  { "netfilter.tcp_flags",    &t_int,  MONO_UNKNOWN,  "CWR/ECE/URG/ACK/PSH/RST/SYN/FIN"      },
  { "netfilter.urgp",         &t_int,  MONO_UNKNOWN,  "TCP urgent pointer"                   },
  { "netfilter.udp_len",      &t_int,  MONO_UNKNOWN,  "UDP Packet length"                    },
  { "netfilter.icmp_type",    &t_int,  MONO_UNKNOWN,  "ICMP message type"                    },
  { "netfilter.icmp_code",    &t_int,  MONO_UNKNOWN,  "ICMP message code"                    },
  { "netfilter.icmp_id",      &t_int,  MONO_UNKNOWN,  "ICMP ID"                              },
  { "netfilter.icmp_seq",     &t_int,  MONO_UNKNOWN,  "ICMP sequence number"                 },
  { "netfilter.icmp_param",   &t_int,  MONO_UNKNOWN,  "ICMP parameter problem code"          },
  { "netfilter.icmp_gateway", &t_ipv4, MONO_UNKNOWN,  "ICMP redirect gateway address"        },
  { "netfilter.icmp_mtu",     &t_int,  MONO_UNKNOWN,  "ICMP Frag. needed MTU"                },
};


static void *netfilter_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  action_orchids_ctx_t *octx;

  DebugLog(DF_MOD, DS_INFO, "load() netfilter@%p\n", &mod_netfilter);
  
  register_fields(ctx, mod, netfilter_fields, NETFILTER_FIELDS);

  octx = gc_base_malloc(ctx->gc_ctx, sizeof(action_orchids_ctx_t));
  octx->ctx = ctx;
  octx->mod = mod;
  octx->atree = compile_actions(ctx->gc_ctx, netfilter_actions);
  octx->in_event = NULL;
  octx->out_event = NULL;
  return octx;
  

  return octx;
}

input_module_t mod_netfilter = {
  MOD_MAGIC,                   /* Magic number */
  ORCHIDS_VERSION,             /* Module version */
  0,                           /* Flags */
  "netfilter",                 /* Module name */
  "CeCILL2",                   /* Module license */
  NULL,                        /* Module dependencies */
  NULL,                        /* Configuration directives for modules 
				  specific parameters */
  netfilter_preconfig,         /* Pre-config function */
  NULL,                        /* Post-config function */
  NULL,                        /* Post-compil function */
  NULL,
  netfilter_dissect,           /* netfilter conditional dissector */
  &t_str                       /* type of fields it expects to dissect */
};

/*
** Copyright (c) 2014 by Pierre-Arnaud SENTUCQ, Laboratoire Spécification
** et Vérification (LSV), CNRS UMR 8643 & ENS Cachan.
**
** Pierre-Arnaud SENTUCQ <pierre-arnaud.sentucq@lsv.ens-cachan.fr>
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
