/**
 ** @file main_priv.h
 ** Private definitions for main.c
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup core
 **
 ** @date  Started on: Mon Jan 13 10:05:09 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef MAIN_PRIV_H
#define MAIN_PRIV_H

#include "orchids.h"

/**
 * Parse command line arguments.
 * @param ctx   A pointer to the Orchids application context.
 * @param argc  The argument counter.
 * @param argv  The argument vector.
 **/
static void
parse_cmdline(orchids_t *ctx, int argc, char **argv);


/**
 * Display the usage page.
 * @param prg  The program name.
 **/
static void
orchids_usage(char *prg);

/**
 * Daemonize the process.  This is done following the common BSD way:
 * detach the terminal, become leader of the process group.  Standard
 * output and error are reopened corresponding the given parameters.
 * @param stdout_file   The file were the standard output will be
 *                      redirected (if NULL is given, "/dev/null" will
 *                      be used).
 * @param stderr_file   The file were the standard error output will be
 *                      redirected (if NULL is given, "/dev/null" will
 *                      be used).
 */
static void
daemonize(const char *stdout_file, const char *stderr_file);


/**
 * The SIGCHLD signal handler.  This is a minimal process reaper for
 * fork()ed children.  Orchids does not fork() very often.  This signal
 * handler function was introduced for the mod_htmlstate html output
 * module, which fork Orchids to dump the Orchids state in background,
 * trying to not interfere with the main detection process.
 * @param signo  The signal number.
 **/
static void
sig_chld(int signo);


/**
 * Change the current runtime user.  This function is used to drop
 * administrative root sometimes required (depending of the Orchids
 * configuration), for security reasons.
 * @param username  The user to change to for runtime operation.
 **/
static void
change_run_id(char *username);

#endif /* MAIN_PRIV_H */

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
