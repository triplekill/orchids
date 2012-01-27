/**
 ** @file mod_bsm.h
 ** Definitions for mod_bsm.c
 **
 **
 ** @author Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
 **  
 ** @version 0.1
 ** @ingroup modules
 ** 
 **
 ** @date  Started on: Sam 14 jan 2012 14:25:59 CET
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef MOD_BSM_H
#define MOD_BSM_H

#include "orchids.h"


#include <bsm/libbsm.h>


#include <sys/types.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

/*****************************************/
#define F_BSM_HDR_VERSION 0
#define F_BSM_HDR_TIME 1
#define F_BSM_HDR_TYPE 2
#define F_BSM_HDR_MODIFIER 3
#define F_BSM_HDR_ADTYPE 4
#define F_BSM_HDR_IP0 5
#define F_BSM_HDR_IP1 6
#define F_BSM_HDR_IP2 7
#define F_BSM_HDR_IP3 8
#define F_BSM_PATH 9
#define F_BSM_SUBJ_AUID 10
#define F_BSM_SUBJ_EUID 11
#define F_BSM_SUBJ_EGID 12
#define F_BSM_SUBJ_RUID 13
#define F_BSM_SUBJ_RGID 14
#define F_BSM_SUBJ_PID 15
#define F_BSM_SUBJ_SID 16
#define F_BSM_SUBJ_PORT 17
#define F_BSM_SUBJ_ADDR 18
#define F_BSM_RET_VAL 19
#define F_BSM_RET_STATUS 20
#define F_BSM_ATTR_MODE 21
#define F_BSM_ATTR_UID 22
#define F_BSM_ATTR_GID 23
#define F_BSM_ATTR_FSID 24
#define F_BSM_ATTR_NID 25
#define F_BSM_ATTR_DEV 26
#define F_BSM_FILE_TIME 27
#define F_BSM_FILE_NAME 28

#define F_BSM_ARG_START 29
#define F_BSM_MAX_ARGS 16
// arguments have entries from F_BSM_ARG_START (arg1)
// through F_BSM_ARG_START+F_BSM_MAX_ARGS-1 (arg<F_BSM_MAX_ARGS>)
// no F_BSM field should be added after these
// If F_BSM_MAX_ARGS is changed, one should also update the last lines
// of the bsm_fields[] array in mod_bsm.c

#define BSM_FIELDS (F_BSM_ARG_START+F_BSM_MAX_ARGS)

/***********************************************/

static int
bsm_callback(orchids_t *ctx, mod_entry_t *mod, int sd, void *data);

static void *
bsm_preconfig(orchids_t *ctx, mod_entry_t *mod);

static void
bsm_postconfig(orchids_t *ctx, mod_entry_t *mod);


#endif /* MOD_BSM_H */

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
