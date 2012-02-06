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
#define F_BSM_HDR_START 0
# define F_BSM_HDR_VERSION 0
# define F_BSM_HDR_TIME 1
# define F_BSM_HDR_TYPE 2
# define F_BSM_HDR_MODIFIER 3
# define F_BSM_HDR_ADTYPE 4
# define F_BSM_HDR_IP0 5
# define F_BSM_HDR_IP1 6
# define F_BSM_HDR_IP2 7
# define F_BSM_HDR_IP3 8
#define F_BSM_HDR_SIZE 9
#define F_BSM_HDR_END (F_BSM_HDR_START+F_BSM_HDR_SIZE)

#define F_BSM_PATH_START F_BSM_HDR_END
# define F_BSM_PATH 0
#define F_BSM_PATH_SIZE 1
#define F_BSM_PATH_END (F_BSM_PATH_START+F_BSM_PATH_SIZE)

#define F_BSM_SUBJ_START F_BSM_PATH_END
# define F_BSM_SUBJ_AUID 0
# define F_BSM_SUBJ_EUID 1
# define F_BSM_SUBJ_EGID 2
# define F_BSM_SUBJ_RUID 3
# define F_BSM_SUBJ_RGID 4
# define F_BSM_SUBJ_PID 5
# define F_BSM_SUBJ_SID 6
# define F_BSM_SUBJ_PORT 7
# define F_BSM_SUBJ_ADDR 8
#define F_BSM_SUBJ_SIZE 9
#define F_BSM_SUBJ_END (F_BSM_SUBJ_START+F_BSM_SUBJ_SIZE)

#define F_BSM_RET_START F_BSM_SUBJ_END
# define F_BSM_RET_VAL 0
# define F_BSM_RET_STATUS 1
#define F_BSM_RET_SIZE 2
#define F_BSM_RET_END (F_BSM_RET_START+F_BSM_RET_SIZE)

#define F_BSM_ATTR_START F_BSM_RET_END
# define F_BSM_ATTR_MODE 0
# define F_BSM_ATTR_UID 1
# define F_BSM_ATTR_GID 2
# define F_BSM_ATTR_FSID 3
# define F_BSM_ATTR_NID 4
# define F_BSM_ATTR_DEV 5
#define F_BSM_ATTR_SIZE 6
#define F_BSM_ATTR_END (F_BSM_ATTR_START+F_BSM_ATTR_SIZE)

#define F_BSM_FILE_START F_BSM_ATTR_END
# define F_BSM_FILE_TIME 0
# define F_BSM_FILE_NAME 1
#define F_BSM_FILE_SIZE 2
#define F_BSM_FILE_END (F_BSM_FILE_START+F_BSM_FILE_SIZE)

#define F_BSM_TEXT_START F_BSM_FILE_END
# define F_BSM_TEXT 0
#define F_BSM_TEXT_SIZE 1
#define F_BSM_TEXT_END (F_BSM_TEXT_START+F_BSM_TEXT_SIZE)

#define F_BSM_INADDR_START F_BSM_TEXT_END
# define F_BSM_INADDR 0
#define F_BSM_INADDR_SIZE 1
#define F_BSM_INADDR_END (F_BSM_INADDR_START+F_BSM_INADDR_SIZE)

#define F_BSM_IPORT_START F_BSM_INADDR_SIZE
# define F_BSM_IPORT 0
#define F_BSM_IPORT_SIZE 1
#define F_BSM_IPORT_END (F_BSM_IPORT_START+F_BSM_IPORT_SIZE)

#define F_BSM_SOCK_START F_BSM_IPORT_END
# define F_BSM_SOCK_TYPE 0
# define F_BSM_SOCK_LPORT 1
# define F_BSM_SOCK_LADDR 2
# define F_BSM_SOCK_LPORT 3
# define F_BSM_SOCK_LADDR 4
#define F_BSM_SOCK_SIZE 5
#define F_BSM_SOCK_END (F_BSM_SOCK_START+F_BSM_SOCK_SIZE)

#define F_BSM_IPC_START F_BSM_SOCK_END
# define F_BSM_IPC_TYPE 0
# define F_BSM_IPC_ID 1
#define F_BSM_IPC_SIZE 2
#define F_BSM_IPC_END (F_BSM_IPC_START+F_BSM_IPC_SIZE)

#define F_BSM_XATPATH_START F_BSM_IPC_END
# define F_BSM_XATPATH 0
#define F_BSM_XATPATH_SIZE 1
#define F_BSM_XATPATH_END (F_BSM_XATPATH_START+F_BSM_XATPATH_SIZE)


#define F_BSM_IPCPERM_START F_BSM_XATPATH_END
# define F_BSM_IPCPERM_UID 0
# define F_BSM_IPCPERM_GID 1
# define F_BSM_IPCPERM_PUID 2
# define F_BSM_IPCPERM_PGID 3
# define F_BSM_IPCPERM_MOD 4
# define F_BSM_IPCPERM_SEQ 5
# define F_BSM_IPCPERM_KEY 6
#define F_BSM_IPCPERM_SIZE 7
#define F_BSM_IPCPERM_END (F_BSM_IPCPERM_START+F_BSM_IPCPERM_SIZE)

#define F_BSM_PROC_START F_BSM_IPCPERM_END
# define F_BSM_PROC_AUID 0
# define F_BSM_PROC_EUID 1
# define F_BSM_PROC_EGID 2
# define F_BSM_PROC_RUID 3
# define F_BSM_PROC_RGID 4
# define F_BSM_PROC_PID 5
# define F_BSM_PROC_SID 6
# define F_BSM_PROC_PORT 7
# define F_BSM_PROC_IP 8
#define F_BSM_PROC_SIZE 9
#define F_BSM_PROC_END (F_BSM_PROC_START+F_BSM_PROC_SIZE)

#define F_BSM_IP_START F_BSM_PROC_END
# define F_BSM_IP_VERSION 0
# define F_BSM_IP_TOS 1
# define F_BSM_IP_LEN 2
# define F_BSM_IP_ID 3
# define F_BSM_IP_OFFSET 4
# define F_BSM_IP_TTL 5
# define F_BSM_IP_PROT 6
# define F_BSM_IP_CHKSM 7
# define F_BSM_IP_SRC 8
# define F_BSM_IP_DEST 9
#define F_BSM_IP_SIZE 10
#define F_BSM_IP_END (F_BSM_IP_START+F_BSM_IP_SIZE)

#define F_BSM_SEQ_START F_BSM_IP_END
# define F_BSM_SEQ 0
#define F_BSM_SEQ_SIZE 1
#define F_BSM_SEQ_END (F_BSM_SEQ_START+F_BSM_SEQ_SIZE)

#define F_BSM_INVALID_START F_BSM_SEQ_END
# define F_BSM_INVALID 0
#define F_BSM_INVALID_SIZE 1
#define F_BSM_INVALID_END (F_BSM_INVALID_START+F_BSM_INVALID_SIZE)

#define F_BSM_EXIT_START F_BSM_INVALID_END
# define F_BSM_EXIT_STATUS 0
# define F_BSM_EXIT_RET 1
#define F_BSM_EXIT_SIZE 2
#define F_BSM_EXIT_END (F_BSM_EXIT_START+F_BSM_EXIT_SIZE)

#define F_BSM_GROUPS_START F_BSM_EXIT_END
# define F_BSM_GROUPS_LEN 0
#define F_BSM_GROUPS_SIZE (AUDIT_MAX_GROUPS+1)
// AUDIT_MAX_GROUPS defined in bsm/audit_record.h
#define F_BSM_GROUPS_END (F_BSM_GROUPS_START+F_BSM_GROUPS_SIZE)

#define F_BSM_NEWGROUPS_START F_BSM_GROUPS_END
# define F_BSM_NEWGROUPS_LEN 0
#define F_BSM_NEWGROUPS_SIZE (AUDIT_MAX_GROUPS+1)
// AUDIT_MAX_GROUPS defined in bsm/audit_record.h
#define F_BSM_NEWGROUPS_END (F_BSM_NEWGROUPS_START+F_BSM_NEWGROUPS_SIZE)

#define F_BSM_EXECARGS_START F_BSM_NEWGROUPS_END
# define F_BSM_EXECARGS_LEN 0
#define F_BSM_EXECARGS_SIZE (AUDIT_MAX_ARGS+1)
#define F_BSM_EXECARGS_END (F_BSM_EXECARGS_START+F_BSM_EXECARGS_SIZE)

#define F_BSM_EXECENV_START F_BSM_EXECARGS_END
# define F_BSM_EXECENV_LEN 0
#define F_BSM_EXECENV_SIZE (AUDIT_MAX_ENV+1)
#define F_BSM_EXECENV_END (F_BSM_EXECENV_START+F_BSM_EXECENV_SIZE)

#define F_BSM_ARG_START F_BSM_EXECENV_END
#define F_BSM_MAX_ARGS AUDIT_MAX_ARGS
// arguments have entries from F_BSM_ARG_START (arg1)
// through F_BSM_ARG_START+F_BSM_MAX_ARGS-1 (arg<F_BSM_MAX_ARGS>)
// no F_BSM field should be added after these
// F_BSM_MAX_ARGS should not be changed
// If really necessary, then the last loop in gen_mod_bsm.c should
// go from 0 to F_BSM_MAX_ARGS, always.

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
