/**
 ** @file mod_pcap.h
 ** Definitions for mod_pcap.c
 ** 
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 ** 
 ** @version 0.1.0
 ** 
 ** @date  Started on: Fri May 25 14:29:20 2007
 **/

#ifndef MOD_PCAP_H
#define MOD_PCAP_H

#define PCAP_FIELDS       6
#define F_TIME            0
#define F_LEN             1
#define F_CAPLEN          2
#define F_INTERFACE       3
#define F_DATALINK        4
#define F_PACKET          5

#define MODPCAP_DEFAULT_SNAPLEN 65535
#define MODPCAP_DEFAULT_PROMISC 1

#define MODPCAP_READ_TIMEOUT 0

typedef struct pcap_pkthdr pcap_pkthdr_t;

typedef struct mod_pcap_if_s mod_pcap_if_t;
typedef struct pcap_cb_data_s pcap_cb_data_t;

struct pcap_cb_data_s
{
  orchids_t      *ctx;
  mod_entry_t    *mod;
  mod_pcap_if_t  *pcapif;
};

struct mod_pcap_if_s
{
  char   *name;
  int     promisc;
  int     snaplen;
  pcap_t *pcap;
  int     fd;
  int     datalink;
};

static void
libpcap_callback(u_char *data,
                 const pcap_pkthdr_t *pkthdr,
                 const u_char *pkt);

static int
modpcap_callback(orchids_t *ctx, mod_entry_t *mod, int fd, void *data);

static void *
pcap_preconfig(orchids_t *ctx, mod_entry_t *mod);

static void
add_device(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir);


#endif /* MOD_PCAP_H */

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
