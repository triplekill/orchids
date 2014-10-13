/**
 ** @file mod_newauditd.c
 ** Dissection module for auditd, new version
 **
 ** @author Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
 ** @author Pierre-Arnaud SENTUCQ <pasentucq@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 ** @date  Started on: Lun 13 oct 2014 08:30:51 UTC
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "orchids.h"
#include "orchids_api.h"
#include "mod_utils.h"

input_module_t mod_newauditd;


static char *action_doer_audit (action_orchids_ctx_t *octx,
				char *s, char *end,
				int field_num)
{
  /* s is of the form 12345.678:1234 followed possibly
     by other characters, where 12345.678 is a time,
     and 1234 is a serial number.
  */
  char *t;
  struct timeval time;
  ovm_var_t *var;
  gc_t *gc_ctx = octx->ctx->gc_ctx;

  t = time_convert(s, end, &time);
  serial = 0;
  if (t<end && *t==':') /* found it */
    {
      t = orchids_atoui (t+1, end-s, &serial);
      if (t<end && *t==')')
	t++;
    }
  /*!!! do something with serial */
  GC_START (gc_ctx, 1);
  var = ovm_timeval_new (gc_ctx);
  TIMEVAL(var) = time;
  GC_UPDATE (gc_ctx, 0, var);

  FILL_EVENT(octx, field_num, 1);
  GC_END (gc_ctx);
  return t;
}

static struct action newauditd_actions[] = {
  /* !!! Don't put any empty word here, or any word
     that is prefix of another one! */
  { "audit(", F_AUDITD_TIME, action_doer_audit },
  { "node=", F_OPENBSM_HDR_FILE, action_doer_string },
  //!!!


  { "type=", F_AUDITD_TYPE, ACTION_STRING },
  { "arch=", F_AUDITD_ARCH, ACTION_INT },
  { "syscall=", F_AUDITD_SYSCALL, ACTION_INT },
  { "success=", F_AUDITD_SUCCESS, ACTION_STRING },
  { "exit=", F_AUDITD_EXIT, ACTION_INT },
  { "a0=", F_AUDITD_A0, ACTION_HEX },
  { "a1=", F_AUDITD_A1, ACTION_HEX },
  { "a2=", F_AUDITD_A2, ACTION_HEX },
  { "a3=", F_AUDITD_A3, ACTION_HEX },
  { "items=", F_AUDITD_ITEMS, ACTION_INT },
  { "ppid=", F_AUDITD_PPID, ACTION_INT },
  { "pid=", F_AUDITD_PID, ACTION_INT },
  { "auid=", F_AUDITD_AUID, ACTION_INT },
  { "uid=", F_AUDITD_UID, ACTION_INT },
  { "gid=", F_AUDITD_GID, ACTION_INT },
  { "euid=", F_AUDITD_EUID, ACTION_INT },
  { "suid=", F_AUDITD_SUID, ACTION_INT },
  { "fsuid=", F_AUDITD_FSUID, ACTION_INT },
  { "egid=", F_AUDITD_EGID, ACTION_INT },
  { "sgid=", F_AUDITD_SGID, ACTION_INT },
  { "fsgid=", F_AUDITD_FSGID, ACTION_INT },
  { "tty=", F_AUDITD_TTY, ACTION_ID },
  { "ses=", F_AUDITD_SES, ACTION_INT },
  { "comm=", F_AUDITD_COMM, ACTION_STRING },
  { "exe=", F_AUDITD_EXE, ACTION_STRING },
  { "subj=", F_AUDITD_SUBJ, ACTION_SUBJ },
  { "key=", F_AUDITD_KEY, ACTION_STRING},
  { "item=", F_AUDITD_ITEM, ACTION_INT },
  { "name=", F_AUDITD_NAME, ACTION_STRING},
  { "inode=", F_AUDITD_INODE, ACTION_INT },
  { "mode=", F_AUDITD_MODE, ACTION_INT },
  { "dev=",  F_AUDITD_DEV, ACTION_DEV },
  { "ouid=", F_AUDITD_OUID, ACTION_INT },
  { "ogid=", F_AUDITD_OGID, ACTION_INT },
  { "rdev=",  F_AUDITD_RDEV, ACTION_DEV },
  { "cwd=", F_AUDITD_CWD, ACTION_STRING },
  { NULL, 0, 0 }
};

input_module_t mod_newauditd = {
  MOD_MAGIC,                /* Magic number */
  ORCHIDS_VERSION,          /* Module version */
  "newauditd",              /* module name */
  "CeCILL2",                /* module license */
  NULL,                     /* dependencies */
  NULL,
  newauditd_preconfig,      /* called just after module registration */
  NULL,
  NULL,
  dissect_newauditd	    /* auditd conditional dissector */
};

/*
** Copyright (c) 2014 by Jean GOUBAULT-LARRECQ and Pierre-Arnaud SENTUCQ, Laboratoire Spécification
** et Vérification (LSV), CNRS UMR 8643 & ENS Cachan.
**
** Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
** Pierre-Arnaud SENTUCQ <pasentucq@lsv.ens-cachan.fr>
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


