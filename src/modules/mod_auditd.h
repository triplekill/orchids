/**
 ** @file mod_auditd.h
 ** Definitions for mod_auditd.c
 **
 **
 ** @author Hedi BENZINA <benzina@lsv.ens-cachan.fr>
 ** @author Jean Goubault-Larrecq <goubault@lsv.ens-cachan.fr>
 **  
 ** @version 0.2
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

#ifdef OBSOLETE
#include "auditd_queue.h"
#endif


#define ACTION_AUDIT 1
// #define ACTION_UNSIGNED_INT 2
#define ACTION_INT 2
#define ACTION_HEX 3
#define ACTION_STRING 4
#define ACTION_ID ACTION_STRING
#define ACTION_SUBJ ACTION_STRING
#define ACTION_DEV 5
/* The last ACTION_* must always be the following (dummy) one: */
#define ACTION_LIMIT 6


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
#define F_AUDITD_A0        8
#define F_AUDITD_A1        9
#define F_AUDITD_A2        10
#define F_AUDITD_A3        11
#define F_AUDITD_ITEMS     12
#define F_AUDITD_PPID      13
#define F_AUDITD_PID       14
#define F_AUDITD_AUID      15
#define F_AUDITD_UID       16
#define F_AUDITD_GID       17
#define F_AUDITD_EUID      18
#define F_AUDITD_SUID      19
#define F_AUDITD_FSUID     20
#define F_AUDITD_EGID      21
#define F_AUDITD_SGID      22
#define F_AUDITD_FSGID     23
#define F_AUDITD_TTY       24
#define F_AUDITD_SES       25
#define F_AUDITD_COMM      26
#define F_AUDITD_EXE       27
#define F_AUDITD_SUBJ      28
#define F_AUDITD_KEY       29
#define F_AUDITD_ITEM      30
#define F_AUDITD_NAME      31
#define F_AUDITD_INODE     32
#define F_AUDITD_MODE      33
#define F_AUDITD_DEV       34
#define F_AUDITD_OUID      35
#define F_AUDITD_OGID      36
#define F_AUDITD_RDEV      37
#define F_AUDITD_CWD       38

#define AUDITD_FIELDS 39

typedef struct auditd_cfg_s {
  struct action_ctx *actx; // internal context data used by auditd_callback()
} auditd_cfg_t;

/***********************************************/

#ifdef OBSOLETE
static int
auditd_callback(orchids_t *ctx, mod_entry_t *mod, int sd, void *data);
#endif

static void *
auditd_preconfig(orchids_t *ctx, mod_entry_t *mod);

#ifdef OBSOLETE
static void
auditd_postconfig(orchids_t *ctx, mod_entry_t *mod);
#endif


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
