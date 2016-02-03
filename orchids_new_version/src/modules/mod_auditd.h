/**
 ** @file mod_auditd.h
 ** Definitions for mod_auditd.c
 **
 **
 ** @author Hedi BENZINA <benzina@lsv.ens-cachan.fr>
 ** @author Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
 ** @version 1.0
 ** @ingroup modules
 ** 
 **
 ** @date  Started on: Fri Feb  7 11:07:42 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef MOD_AUDITD_H
#define MOD_AUDITD_H

#include "orchids.h"

#define F_AUDITD_NODE      0
#define F_AUDITD_TYPE      1
// The numbers F_AUDITD_TIME and F_AUDITD_SERIAL should be n and n+1,
// due to the fact that they are handled in action_doer_audit()
#define F_AUDITD_TIME      2
#define F_AUDITD_SERIAL    (F_AUDITD_TIME+1)
#define F_AUDITD_ARCH      4
#define F_AUDITD_SYSCALL   5
#define F_AUDITD_SUCCESS   6
#define F_AUDITD_EXIT      7
#define F_AUDITD_ITEMS     8
#define F_AUDITD_PPID      9
#define F_AUDITD_PID       10
#define F_AUDITD_AUID      11
#define F_AUDITD_UID       12
#define F_AUDITD_GID       13
#define F_AUDITD_EUID      14
#define F_AUDITD_SUID      15
#define F_AUDITD_FSUID     16
#define F_AUDITD_EGID      17
#define F_AUDITD_SGID      18
#define F_AUDITD_FSGID     19
#define F_AUDITD_TTY       20
#define F_AUDITD_SES       21
#define F_AUDITD_COMM      22
#define F_AUDITD_EXE       23
#define F_AUDITD_SUBJ      24
#define F_AUDITD_KEY       25
#define F_AUDITD_ITEM      26
#define F_AUDITD_NAME      27
#define F_AUDITD_INODE     28
#define F_AUDITD_MODE      29
#define F_AUDITD_DEV       30
#define F_AUDITD_OUID      31
#define F_AUDITD_OGID      32
#define F_AUDITD_RDEV      33
#define F_AUDITD_NAMETYPE  34
#define F_AUDITD_CWD       35
#define F_AUDITD_ARGC      36
#define F_AUDITD_PROCTITLE 37
#define F_AUDITD_SIG       38
#define F_AUDITD_OPID      39
#define F_AUDITD_OAUID     40
#define F_AUDITD_OSES      41
#define F_AUDITD_OBJ       42
#define F_AUDITD_OCOMM     43

#define F_AUDITD_REGULAR_END 44
/* F_AUDITD_ARG_END will be defined in fields_auditd.h
 Do not add any field after F_AUDITD_REGULAR_END! */
#define AUDITD_FIELDS F_AUDITD_ARG_END

/***********************************************/

static void *
auditd_preconfig(orchids_t *ctx, mod_entry_t *mod);


#endif /* MOD_AUDITD_H */

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
