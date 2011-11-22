/**
 ** @file mod_prolog.h
 ** Definitions for mod_prolog.h
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 ** @date  Started on: Fri Feb  7 11:07:42 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef MOD_PROLOG_H
#define MOD_PROLOG_H

#include "orchids_types.h"
#include "orchids.h"
#include "html_output.h"

typedef struct prolog_cfg_s prolog_cfg_t;

struct prolog_cfg_s {
  timeval_t last_db_update;
};

static int
pl_init(char *name, char *bootfile);

int
pl_execute(const char *goal);

static char *
pl_execute_var(const char *goal_str, const char *var_name);

static int
set_prolog_io(const char *in, const char *out, const char *err);


/* static int pl_dump_dyndb(const char *file); */


static int
pl_consult(const char *file);


static void
issdl_prolog(orchids_t *ctx, state_instance_t *state);


static void *
prolog_preconfig(orchids_t *ctx, mod_entry_t *mod);


static void
consult_plfile(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir);


static int
prolog_htmloutput(orchids_t *ctx, mod_entry_t *mod, FILE *menufp, html_output_cfg_t *htmlcfg);


#endif /* MOD_PROLOG_H */

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
