/**
 ** @file mod_syslog.h
 ** Definitions for mod_syslog.c
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 ** @date  Started on: Wed Jan 15 17:21:55 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef MOD_SYSLOG_H
#define MOD_SYSLOG_H

#include <time.h>

#define SYSLOG_FIELDS 8
#define F_FACILITY 0
#define F_SEVERITY 1
#define F_TIME 2
#define F_HOST 3
#define F_REPEAT 4
#define F_PID 5
#define F_PROG 6
#define F_MSG 7

/* for date convertion */
#define MOD_SYSLOG_JAN  0
#define MOD_SYSLOG_FEB  1
#define MOD_SYSLOG_MAR  2
#define MOD_SYSLOG_AVR  3
#define MOD_SYSLOG_MAY  4
#define MOD_SYSLOG_JUN  5
#define MOD_SYSLOG_JUL  6
#define MOD_SYSLOG_AUG  7
#define MOD_SYSLOG_SEP  8
#define MOD_SYSLOG_OCT  9
#define MOD_SYSLOG_NOV 10
#define MOD_SYSLOG_DEC 11


struct tm *
syslog_getdate(const char *date);


static int
dissect_syslog(orchids_t *ctx, mod_entry_t *mod, event_t *event, void *data);


static void *
syslog_preconfig(orchids_t *ctx, mod_entry_t *mod);

#endif /* MOD_SYSLOG_H */

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

