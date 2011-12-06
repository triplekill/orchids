/**
 ** @file mod_rawsnare.h
 ** Definitions for mod_rawsnare.c
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 ** @date  Started on: Fri Feb 14 18:36:36 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef MOD_RAWSNARE_H
#define MOD_RAWSNARE_H

#include "orchids.h"
#include "lang.h"
#include "rawsnare.h"

#define LIN24_SYSCALL_MAX 243

#define RAWSNARE_FIELDS 35
#define F_TIME      0
#define F_CLASS     1
#define F_SYSCALL   2
#define F_RUID      3
#define F_RGID      4
#define F_EUID      5
#define F_EGID      6
#define F_PID       7
#define F_PPID      8
#define F_PROCNAME  9
#define F_RETCODE   10
#define F_WORKDIR   11
#define F_PATH      12
#define F_MODE      13
#define F_CREATEMODE 14
#define F_CMDLINE   15
#define F_SRCPATH   16
#define F_DSTPATH   17
#define F_SOCKCALL  18
#define F_DSTIP     19
#define F_DSTPORT   20
#define F_SRCIP     21
#define F_SRCPORT   22
#define F_OWNERUID  23
#define F_OWNERGID  24
#define F_TARGETID  25
#define F_TARGETRID 26
#define F_TARGETSID 27
#define F_MODNAME   28
#define F_PTRACEREQ 29
#define F_PTRACEPID 30
#define F_PTRACEADDR 31
#define F_PTRACEDATA 32
#define F_KILLPID 33
#define F_KILLSIG 34


static int
read_io(ovm_var_t *attr[RAWSNARE_FIELDS], header_token_t *hdr);


static int
read_pc(ovm_var_t *attr[RAWSNARE_FIELDS], header_token_t *hdr);


static int
read_exec(ovm_var_t *attr[RAWSNARE_FIELDS], header_token_t *hdr);


static int
read_net(ovm_var_t *attr[RAWSNARE_FIELDS], header_token_t *hdr);


static int
read_pt(ovm_var_t *attr[RAWSNARE_FIELDS], header_token_t *hdr);


static int
read_kill(ovm_var_t *attr[RAWSNARE_FIELDS], header_token_t *hdr);


static int
read_ch(ovm_var_t *attr[RAWSNARE_FIELDS], header_token_t *hdr);


static int
read_cp(ovm_var_t *attr[RAWSNARE_FIELDS], header_token_t *hdr);


static int
read_su(ovm_var_t *attr[RAWSNARE_FIELDS], header_token_t *hdr);


static int
read_ad(ovm_var_t *attr[RAWSNARE_FIELDS], header_token_t *hdr);


static int
rawsnare_dissect(orchids_t *ctx, mod_entry_t *mod, event_t *event, void *data);


static void *
rawsnare_preconfig(orchids_t *ctx, mod_entry_t *mod);


static void
add_udp_source(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir);



#endif /* MOD_RAWSNARE_H */

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
