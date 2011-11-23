/**
 ** @file mod_snmptrap.h
 ** Definitions for mod_snmptrap.c
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

#ifndef MOD_SNMPTRAP_H
#define MOD_SNMPTRAP_H

#ifdef HAVE_NETSNMP

#define SNMPTRAP_FIELDS   15
#define F_SEQ 0
#define F_BIND 1
#define F_VERSION     2
#define F_COMMUNITY   3
#define F_PDUTYPE     4
#define F_OBJECTID    5
#define F_VALUE       6

#define F_ENTERPRISE 7
#define F_AGENT_ADDR 8
#define F_TRAPTYPE 9
#define F_SPECIFIC_TRAPTYPE 10
#define F_TIMESTAMP 11

#define F_REQUESTID   12
#define F_ERRORSTATUS 13
#define F_ERRORINDEX  14

typedef struct snmptrap_config_s snmptrap_config_t;
struct snmptrap_config_s
{
  int some_flag;
  int some_option;
};


static int
snmptrap_dissector(orchids_t *ctx, mod_entry_t *mod, event_t *e, void *data);


static void *
snmptrap_preconfig(orchids_t *ctx, mod_entry_t *mod);


static void
snmptrap_postconfig(orchids_t *ctx, mod_entry_t *mod);


static void
add_mib_dir(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir);


static void
add_mib(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir);


static void
add_mib_file(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir);







#endif /* HAVE_NETSNMP */

#endif /* MOD_SNMPTRAP_H */

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
