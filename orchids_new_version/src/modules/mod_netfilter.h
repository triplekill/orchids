/**
 ** @file mod_netfilter.h
 ** Netfilter log module.
 **
 ** @author Pierre-Arnaud SENTUCQ <peirre-arnaud.sentucq@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 ** @date  Started on: Wed Dec 17 14:29:15 CET 2014
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef MOD_NETFILTER_H
#define MOD_NETFILTER_H

#include "orchids.h"

/* netfilter log fields */
#define NETFILTER_FIELDS 30
#define F_IN             0
#define F_PHYSIN         1
#define F_OUT            2
#define F_PHYSOUT        3
#define F_MAC            4
#define F_SRC            5
#define F_DST            6
#define F_IPLEN          7
#define F_TOS            8
#define F_PREC           9
#define F_TTL            10
#define F_IPID           11
#define F_IPFLAGS        12
#define F_FRAG           13
#define F_IPOPTS         14
#define F_PROTO          15
#define F_SPT            16
#define F_DPT            17
#define F_SEQ            18
#define F_ACK            19
#define F_WINDOW         20
#define F_RES            21
#define F_TCPFLAGS       22
#define F_URGP           23
#define F_UDPLEN         24
#define F_ICMPTYPE       25
#define F_ICMPCODE       26
#define F_ICMPPARAM      27
#define F_ICMPGATEWAY    28
#define F_ICMPMTU        29


/******************************************/

static void *netfilter_preconfig(orchids_t *ctx, mod_entry_t *mod);

#endif /* MOD_NETFILTER_H */

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
