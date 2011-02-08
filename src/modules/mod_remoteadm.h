/**
 ** @file mod_remoteadm.h
 ** Definitions for mod_remoteadm.c
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 ** @date  Started on: Mon Jan 27 17:32:49 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef MOD_REMOTEADM_H
#define MOD_REMOTEADM_H

#include <stdio.h>

#include "orchids.h"

#define RADM_BANNER \
" _    _____   __         ___         _    _    _\n" \
"| |  / __\\ \\ / /  ___   / _ \\ _ _ __| |_ (_)__| |___\n" \
"| |__\\__ \\\\ V /  |___| | (_) | '_/ _| ' \\| / _` (_-<\n" \
"|____|___/ \\_/          \\___/|_| \\__|_||_|_\\__,_/__/\n" \

/* split ascii picture into 2 parts to be ISO-C89 compliant */
#define RADM_FLOWER1 \
"             \033[31;1m        .-~~-.--.\n" \
"                    :         )\n" \
"              .~ ~ -.\\       /.- ~~ .\n" \
"              >       `.   .'       <\n" \
"             (        \033[33m .- -. \033[31m        )\n" \
"              `- -.-~ \033[33m `- -' \033[31m ~-.- -'\n" \
"               (        :        )      \033[32m   _ _ .-:\n" \
"       \033[31m         ~--.    :    .--~ \033[32m      .-~  .-~  }\n"
#define RADM_FLOWER2 \
"        \033[31m             ~-.-^-.-~\033[32m \\_      .~  .-~   .~\n" \
"                              \\ \\'     \\ '_ _ -~\n" \
"                               `.`.    //\n" \
"                      . - ~ ~-.__`.`-.//\n" \
"                  .-~   . - ~  }~ ~ ~-.~-.\n" \
"                .' .-~      .-~       :/~-.~-./:\n" \
"               /_~_ _ . - ~                 ~-.~-._\n" \
"                                                ~-.<\033[37;1m\033[0m\n"

#define RECV_BUF_SZ 128

#define DEFAULT_RADM_PORT 4242

#define RADM_STATE_LIMIT 50


typedef struct radmcfg_s radmcfg_t;
struct radmcfg_s
{
  int listen_port;
};


typedef struct remoteadm_cmd_s remoteadm_cmd_t;
struct remoteadm_cmd_s
{
  char *name;
  void (*cmd)(FILE *fd, orchids_t *ctx, char *args);
  char *help;
};


static void
show_prompt(FILE *fp);


static int
create_tcplisten_socket(void);


static int
remoteadm_listen_callback(orchids_t *ctx, mod_entry_t *mod, int fd, void *data);


static int
remoteadm_client_callback(orchids_t *ctx, mod_entry_t *mod, int fd, void *data);

static void *
radm_preconfig(orchids_t *ctx, mod_entry_t *mod);


static void
radm_postconfig(orchids_t *ctx, mod_entry_t *mod);


static void
set_listen_port(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir);







/*
 * Remoteadm functions
 */
static void radm_cmd_help(FILE *fp, orchids_t *ctx, char *args);
static void radm_cmd_lsmod(FILE *fp, orchids_t *ctx, char *args);
static void radm_cmd_cfgtree(FILE *fp, orchids_t *ctx, char *args);
static void radm_cmd_showdirs(FILE *fp, orchids_t *ctx, char *args);
static void radm_cmd_showfields(FILE *fp, orchids_t *ctx, char *args);
static void radm_cmd_stats(FILE *fp, orchids_t *ctx, char *args);
static void radm_cmd_lsrules(FILE *fp, orchids_t *ctx, char *args);
static void radm_cmd_lsinstances(FILE *fp, orchids_t *ctx, char *args);
static void radm_cmd_lsthreads(FILE *fp, orchids_t *ctx, char *args);
static void radm_cmd_lsevents(FILE *fp, orchids_t *ctx, char *args);
static void radm_cmd_lsfunctions(FILE *fp, orchids_t *ctx, char *args);
static void radm_cmd_dumprule(FILE *fp, orchids_t *ctx, char *args);
static void radm_cmd_dumpinst(FILE *fp, orchids_t *ctx, char *args);
static void radm_cmd_htmloutput(FILE *fp, orchids_t *ctx, char *args);
static void radm_cmd_exit(FILE *fp, orchids_t *ctx, char *args);
static void radm_cmd_about(FILE *fp, orchids_t *ctx, char *args);
static void radm_cmd_shutdown(FILE *fp, orchids_t *ctx, char *args);
static void radm_cmd_feedback(FILE *fp, orchids_t *ctx, char *args);


#endif /* MOD_REMOTEADM_H */

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
