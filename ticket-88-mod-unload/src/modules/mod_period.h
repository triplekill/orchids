/**
 ** @file mod_period.h
 ** Definitions for mod_period.c
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 **
 ** @date  Started on: Fri Feb  7 11:07:42 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef MOD_PERIOD_H
#define MOD_PERIOD_H


#include <stdio.h>

#include "period.h"
#include "strhash.h"
#include "orchids.h"
#include "html_output.h"

typedef struct modperiod_ctx_s modperiod_ctx_t;
struct modperiod_ctx_s
{
  char *name;
  periodctx_t *periodctx;
  phasectx_t *phasectx;
};


typedef struct period_config_s period_config_t;
struct period_config_s
{
  strhash_t *contexts;
};


static int
qsort_strcmp(const void *a, const void *b);


static int
period_htmloutput(orchids_t *ctx, mod_entry_t *mod, FILE *menufp, html_output_cfg_t *htmlcfg);


static void
issdl_temporal(orchids_t *ctx, state_instance_t *state);


static void *
period_preconfig(orchids_t *ctx, mod_entry_t *mod);


static void
period_postconfig(orchids_t *ctx, mod_entry_t *mod);


static void
period_postcompil(orchids_t *ctx, mod_entry_t *mod);




#endif /* MOD_PERIOD_H */

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
