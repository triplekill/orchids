/**
 ** @file mod_auditd.h
 ** Definitions for mod_auditd.c
 **
 **
 ** @author Hedi BENZINA <benzina@lsv.ens-cachan.fr>
 **  
 ** @version 0.1
 ** @ingroup modules
 ** 
 **
 ** @date  Started on: Fri Feb  7 11:07:42 2003
 ** @date Last update: Sat Sep  8 19:54:48 2007
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef MOD_AUDITD_H
#define MOD_AUDITD_H

#include "orchids.h"


#include "auditd_queue.h"


#include <sys/uio.h>  /*for iovec structure*/
#include <libaudit.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>


#define SOCK_PATH "/var/run/audispd_events"

#define ACTION_AUDIT 1
#define ACTION_INT 2
#define ACTION_INTREL 3
#define ACTION_HEX 4
#define ACTION_STRING 5
#define ACTION_ID ACTION_STRING
#define ACTION_SUBJ ACTION_STRING
/* The last ACTION_* must always be the following (dummy) one: */
#define ACTION_LIMIT 6


#define F_TIME      0
#define F_SERIAL    1
#define F_ARCH      2
#define F_SYSCALL   3
#define F_SUCCESS   4
#define F_EXIT      5
#define F_A0        6
#define F_A1        7
#define F_A2        8
#define F_A3        9
#define F_ITEMS     10
#define F_PPID      11
#define F_PID       12
#define F_AUID      13
#define F_UID       14
#define F_GID       15
#define F_EUID      16
#define F_SUID      17
#define F_FSUID     18
#define F_EGID      19
#define F_SGID      20
#define F_FSGID     21
#define F_TTY       22
#define F_SES       23
#define F_COMM      24
#define F_EXE       25
#define F_SUBJ      26
#define F_KEY       27

#define AUDITD_FIELDS 28

/*****************************************/
typedef struct syscall_event_s
{
	char     *time;
	int      serial;
	int      arch;
	int      syscall;
	char     *success;
	int      exit;
	char     *a0;
	char     *a1;
	char     *a2;
	char     *a3;
	int      items;
	int      ppid;
	int      pid;
	int      auid;
	int      uid;
	int      gid;
	int      euid;
	int      suid;
	int      fsuid;
	int      egid;
	int      sgid;
	int      fsgid;
	char     *tty;
	int      ses;
	char     *comm;
	char     *exe;
	char     *subj;
	char     *key;

}syscall_event_t;


/***********************************************/

static int
auditd_callback(orchids_t *ctx, mod_entry_t *mod, int sd, void *data);

static void *
auditd_preconfig(orchids_t *ctx, mod_entry_t *mod);

static void
auditd_postconfig(orchids_t *ctx, mod_entry_t *mod);


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
