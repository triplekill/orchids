/**
 ** @file mod_snare.h
 ** Definitions for mod_snare.c
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 ** @date  Started on: Thu Feb 13 13:03:07 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */


#ifndef MOD_SNARE_H
#define MOD_SNARE_H

#include "orchids.h"

#define SNARE_FIELDS 32
#define F_TIME      0
#define F_CLASS     1
#define F_SYSCALL   2
#define F_RUID      3
#define F_RGID      4
#define F_EUID      5
#define F_EGID      6
#define F_PID       7
#define F_PROCNAME  8
#define F_RETCODE   9
#define F_WORKDIR   10
#define F_PATH      11
#define F_MODE      12
#define F_CREATEMODE 13
#define F_CMDLINE   14
#define F_SRCPATH   15
#define F_DSTPATH   16
#define F_SOCKCALL  17
#define F_DSTIP     18
#define F_DSTPORT   19
#define F_SRCIP     20
#define F_SRCPORT   21
#define F_OWNERUID  22
#define F_OWNERGID  23
#define F_TARGETID  24
#define F_TARGETRID 25
#define F_TARGETSID 26
#define F_MODNAME   27
#define F_SEQUENCE  28
#define F_DEVMAJ    29
#define F_DEVMIN    30
#define F_OFFSET    31


extern void snareparse_set_str(char *str, size_t s);
extern void snareparse_set_attrs(ovm_var_t **attr_fields);
extern int snareparse(void);
extern void snareparse_set_attrs(ovm_var_t **attr_fields);
extern void snareparse_reset(void);


static int
snare_dissect(orchids_t *ctx, mod_entry_t *mod, event_t *event, void *data);


static void *
snare_preconfig(orchids_t *ctx, mod_entry_t *mod);


//static void
//add_udp_source(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir);
// not defined in mod_snare.c

//static void
//add_textfile_source(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir);
// not defined in mod_snare.c



#endif /* MOD_SNARE_H */

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
