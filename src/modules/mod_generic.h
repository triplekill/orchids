/**
 ** @file mod_generic.h
 ** Definitions for mod_generic.c.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
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
  int type;
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
  field_t *field_array;
  ovm_var_t **field_values;
  strhash_t *field_hash;
  int fields;
};

typedef struct generic_hook_s generic_hook_t;
struct generic_hook_s
{
/*   STAILQ_ENTRY(generic_hook_t) hooks; */
  /* vmod list */
  STAILQ_HEAD(vmods, generic_vmod_t) vmod_list;
  char *module;
  char *condition;
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

static int
generic_dissect(orchids_t *ctx, mod_entry_t *mod, event_t *event, void *data);;


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


static void
add_hook(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir);



#endif /* MOD_GENERIC_H */
