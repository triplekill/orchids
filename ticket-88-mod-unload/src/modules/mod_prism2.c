/**
 ** @file mod_prism2.c
 ** Wifi Prism2 header decoding.
 ** This include some code of the initial mod_wifi module made by Romdhane.
 ** 
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 ** 
 ** @version 0.1.0
 ** 
 ** @date  Started on: Fri Jun 15 09:08:12 2007
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <pcap.h>

#include "orchids.h"

#include "orchids_api.h"

#include "mod_prism2.h"

#define PRISM2_FIELDS   14
#define F_MSGCODE        0
#define F_MSGLEN         1
#define F_DEVNAME        2
#define F_HOSTTIME       3
#define F_MACTIME        4
#define F_CHANNEL        5
#define F_RSSI           6
#define F_SQ             7
#define F_SIGNAL         8
#define F_NOISE          9
#define F_RATE          10
#define F_ISTX          11
#define F_FRMLEN        12
#define F_FRAME         13

input_module_t mod_prism2;

static int
prism2_callback(orchids_t *ctx, mod_entry_t *mod, event_t *event, void *data)
{
  ovm_var_t *attr[PRISM2_FIELDS];
  u_int8_t *packet;
  size_t packet_len;
  prism2_hdr_t *prism2_hdr;

  DebugLog(DF_MOD, DS_DEBUG, "Prism2 Callback\n");

  memset(attr, 0, sizeof (attr));

  packet_len = BSTRLEN(event->value);

  if (packet_len < sizeof (prism2_hdr_t)) {
    DebugLog(DF_MOD, DS_ERROR, "Frame is too small to contain Prism2 header\n");
    return (-1);
  }

  packet = BSTR(event->value);
  prism2_hdr = (prism2_hdr_t *) packet;

  attr[F_MSGCODE] = ovm_uint_new();
  UINT(attr[F_MSGCODE]) = prism2_hdr->msgcode;

  attr[F_MSGLEN] = ovm_uint_new();
  UINT(attr[F_MSGLEN]) = prism2_hdr->msglen;

  DECODE_PRISM_VALUE(attr[F_HOSTTIME], &prism2_hdr->hosttime);
  DECODE_PRISM_VALUE(attr[F_MACTIME],  &prism2_hdr->mactime);
  DECODE_PRISM_VALUE(attr[F_CHANNEL],  &prism2_hdr->channel);
  DECODE_PRISM_VALUE(attr[F_RSSI],     &prism2_hdr->rssi);
  DECODE_PRISM_VALUE(attr[F_SQ],       &prism2_hdr->sq);
  DECODE_PRISM_VALUE(attr[F_SIGNAL],   &prism2_hdr->signal);
  DECODE_PRISM_VALUE(attr[F_NOISE],    &prism2_hdr->noise);
  DECODE_PRISM_VALUE(attr[F_RATE],     &prism2_hdr->rate);
  DECODE_PRISM_VALUE(attr[F_ISTX],     &prism2_hdr->istx);

  attr[F_FRAME] = ovm_vbstr_new();
  VBSTR(attr[F_FRAME])    = packet     + sizeof (prism2_hdr_t);
  VBSTRLEN(attr[F_FRAME]) = packet_len - sizeof (prism2_hdr_t);

  add_fields_to_event(ctx, mod, &event, attr, PRISM2_FIELDS);

  post_event(ctx, mod, event);

  return (0);
}


static field_t prism2_fields[] = {
  { "prism2.msgcode",   T_UINT,  "Prism message code"                    },
  { "prism2.msglen",    T_UINT,  "Prism message length"                  },
  { "prism2.devname",   T_VSTR,  "Device name (interface)"               },
  { "prism2.hosttime",  T_UINT,  "Host time"                             },
  { "prism2.mactime",   T_UINT,  "MAC time"                              },
  { "prism2.channel",   T_UINT,  "Wifi channel"                          },
  { "prism2.rssi",      T_UINT,  "Receivied Signal Strength Indication"  },
  { "prism2.sq",        T_UINT,  "SQ"                                    },
  { "prism2.signal",    T_UINT,  "Signal"                                },
  { "prism2.noise",     T_UINT,  "Noise"                                 },
  { "prism2.rate",      T_UINT,  "Rate"                                  },
  { "prism2.istx",      T_UINT,  "IsTX"                                  },
  { "prism2.framelen",  T_UINT,  "Frame length"                          },
  { "prism2.frame",     T_VBSTR, "Frame"                                 }
};

static void *
prism2_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  int *dl;

  DebugLog(DF_MOD, DS_DEBUG, "load() prism2@%p\n", (void *) &mod_prism2);

  register_fields(ctx, mod, prism2_fields, PRISM2_FIELDS);

  dl = Xmalloc(sizeof (int));
  *dl = pcap_datalink_name_to_val("PRISM_HEADER");
  register_conditional_dissector(ctx, mod, "pcap",
                                 (void *)dl, sizeof (int),
                                 prism2_callback, NULL);

  return (NULL);
}

input_module_t mod_prism2 = {
  MOD_MAGIC,
  ORCHIDS_VERSION,
  "prism2",
  NULL,
  NULL,
  NULL,
  prism2_preconfig,
  NULL,
  NULL
};

/*
** Copyright (c) 2002-2007 by Julien OLIVAIN, Laboratoire Spécification
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
