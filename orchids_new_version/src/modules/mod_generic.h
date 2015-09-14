/**
 ** @file mod_generic.h
 ** Definitions for mod_generic.c.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 ** @author Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
 **
 ** @version 0.2
 ** @ingroup modules
 **
 ** @date  Started on: Wed Jan 15 17:08:11 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef MOD_GENERIC_H
#define MOD_GENERIC_H

#include "stailq.h"

typedef struct generic_field_s generic_field_t;
struct generic_field_s
{
  STAILQ_ENTRY(generic_field_t) fields;
  STAILQ_ENTRY(generic_field_t) globfields;
  int field_id;
  type_t *type;
  char *name;
  int substring;
  char *description;
};


typedef struct generic_match_s generic_match_t;
struct generic_match_s
{
  STAILQ_ENTRY(generic_match_t) matches;
  /* field list */
  STAILQ_HEAD(fields, generic_field_t) field_list;
  int fields;
  char *regex_str;
  regex_t regex;
};

typedef struct generic_vmod_s generic_vmod_t;
struct generic_vmod_s
{
  STAILQ_ENTRY(generic_vmod_t) vmods;
  STAILQ_ENTRY(generic_vmod_t) globvmods;
  /* match list */
  STAILQ_HEAD(matches, generic_match_t) match_list;
  STAILQ_HEAD(globfields, generic_field_t) field_globlist;
  char *name;
  int mod_id;
  input_module_t mod_entry;
  size_t nfields;
  field_t *field_array;
  strhash_t *field_hash;
  field_table_t *fields;
};

typedef struct generic_hook_s generic_hook_t;
struct generic_hook_s
{
/*   STAILQ_ENTRY(generic_hook_t) hooks; */
  /* vmod list */
  STAILQ_HEAD(vmods, generic_vmod_t) vmod_list;
  char *module;
  char *condition;
  char *file;
  uint32_t line;
};


typedef struct mod_generic_cfg_s mod_generic_cfg_t;
struct mod_generic_cfg_s {
  /* hook array */
  generic_hook_t *hook_array;
  int used_hook;
  strhash_t *mod_hash;
  int mods;
  STAILQ_HEAD(globvmods, generic_vmod_t) vmod_globlist;
};

static int generic_dissect(orchids_t *ctx, mod_entry_t *mod, event_t *event, void *data,
			   int dissection_level);


static void *
generic_preconfig(orchids_t *ctx, mod_entry_t *mod);


static void
generic_postconfig(orchids_t *ctx, mod_entry_t *mod);


static void
add_field(orchids_t *ctx,
          generic_vmod_t *v,
          generic_match_t *m,
          config_directive_t *field_dir);


static void
add_fmatch(orchids_t *ctx, generic_vmod_t *v, config_directive_t *fmatch_dir);


static void
add_vmod(orchids_t *ctx, generic_hook_t *h, config_directive_t *vmod_dir);


static void generic_add_hook(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir);



#endif /* MOD_GENERIC_H */

/*
** Copyright (c) 2002-2005 by Julien OLIVAIN, Laboratoire Spécification
** et Vérification (LSV), CNRS UMR 8643 & ENS Cachan.
** Copyright (c) 2013-2015 by Jean GOUBAULT-LARRECQ, Laboratoire Spécification
** et Vérification (LSV), CNRS UMR 8643 & ENS Cachan.
**
** Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
** Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
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
