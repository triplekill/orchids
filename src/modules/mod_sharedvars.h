/**
 ** @file mod_sharedvars.h
 ** Definitions for mod_sharedvars.c
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

#ifndef MOD_SHAREDVARS_H
#define MOD_SHAREDVARS_H

#define DEFAULT_SHAREDVARS_HASH_SZ 262144


typedef struct sharedvars_config_s sharedvars_config_t;
struct sharedvars_config_s
{
  int hash_size;
  strhash_t *vars_hash;
};


static void
issdl_get_shared_var(orchids_t *ctx, state_instance_t *state);


static void
issdl_del_shared_var(orchids_t *ctx, state_instance_t *state);


static void
issdl_set_shared_var(orchids_t *ctx, state_instance_t *state);


static void *
sharedvars_preconfig(orchids_t *ctx, mod_entry_t *mod);


static void
sharedvars_postconfig(orchids_t *ctx, mod_entry_t *mod);


static void
sharedvars_postcompil(orchids_t *ctx, mod_entry_t *mod);


static void
set_hash_size(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir);


#endif /* MOD_SHAREDVARS_H */

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
