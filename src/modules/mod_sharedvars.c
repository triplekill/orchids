/**
 ** @file mod_sharedvars.c
 ** Add the ability to use global shared variables between rule instances.
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


#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>

#include "orchids.h"

#include "strhash.h"

#include "mod_sharedvars.h"


input_module_t mod_sharedvars;

static sharedvars_config_t *mod_sharedvars_cfg_g = NULL;


static void
issdl_get_shared_var(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *varname;
  char *key;
  ovm_var_t *value;

  varname = stack_pop(ctx->ovm_stack);
  if (TYPE(varname) != T_STR) {
    DebugLog(DF_ENG, DS_ERROR, "parameter type error\n");
    ISSDL_RETURN_FALSE(ctx, state);
    return ;
  }

  key = ovm_strdup(varname);

  value = strhash_get(mod_sharedvars_cfg_g->vars_hash, key);

  if (!value)
    ISSDL_RETURN_FALSE(ctx, state);
  else
  {
    value = issdl_clone(value);
    FLAGS(value) |= TYPE_CANFREE | TYPE_NOTBOUND;
    stack_push(ctx->ovm_stack, value);
  }
  Xfree(key);
}


static void
issdl_del_shared_var(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *varname;
  char *key;
  ovm_var_t *value;

  varname = stack_pop(ctx->ovm_stack);
  if (TYPE(varname) != T_STR) {
    DebugLog(DF_ENG, DS_ERROR, "parameter type error\n");
    return ;
  }

  key = ovm_strdup(varname);

  value = strhash_del(mod_sharedvars_cfg_g->vars_hash, key);

  if (value) {
    Xfree(value);
  }
  else {
    DebugLog(DF_ENG, DS_ERROR,
             "Try to delete an non-existent variable '%s'\n",
             key);
  }

  Xfree(key);
  ISSDL_RETURN_TRUE(ctx, state);
}


static void
issdl_set_shared_var(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *varname;
  ovm_var_t *value;
  char *key;
  ovm_var_t *old_value;
  ovm_var_t *new_value;

  varname = stack_pop(ctx->ovm_stack);
  if (TYPE(varname) != T_STR) {
    DebugLog(DF_ENG, DS_ERROR, "parameter type error\n");
    ISSDL_RETURN_FALSE(ctx, state);
    return ;
  }

  value = stack_pop(ctx->ovm_stack);

  DebugLog(DF_ENG, DS_INFO,
           "Setting shared var '%s'\n", STR(varname));

  key = ovm_strdup(varname);
  new_value = issdl_clone(value);
  /* This new cloned variable MUST NOT be marked with CANFREE (which
   * is the default of issdl_clone()) because its memory is not under
   * the control of the virtual machine.  It is this module which
   * decide when allocate/deallocate it. */
  FLAGS(new_value) &= ~TYPE_CANFREE;
  old_value = strhash_update_or_add(mod_sharedvars_cfg_g->vars_hash,
                                    new_value, key);
  if (old_value) {
    Xfree(old_value);
    Xfree(key);
  }
  ISSDL_RETURN_TRUE(ctx, state);
}


static void *
sharedvars_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  sharedvars_config_t *cfg;

  DebugLog(DF_MOD, DS_INFO, "load() sharedvars@%p\n", &mod_sharedvars);

  /* allocate some memory for module configuration
  ** and initialize default configuration. */
  cfg = Xzmalloc(sizeof (sharedvars_config_t));
  mod_sharedvars_cfg_g = cfg;
  cfg->hash_size = DEFAULT_SHAREDVARS_HASH_SZ;

  register_lang_function(ctx,
                         issdl_set_shared_var,
                         "set_shared_var", 2,
                         "Set or update a shared variable");

  register_lang_function(ctx,
                         issdl_get_shared_var,
                         "get_shared_var", 1,
                         "Get a shared variable");

  register_lang_function(ctx,
                         issdl_del_shared_var,
                         "del_shared_var", 1,
                         "Delete a shared variable");

  /* return config structure, for module manager */
  return (cfg);
}


static void
sharedvars_postconfig(orchids_t *ctx, mod_entry_t *mod)
{
  sharedvars_config_t *cfg;

  cfg = (sharedvars_config_t *)mod->config;
  cfg->vars_hash = new_strhash( cfg->hash_size );
}


static void
sharedvars_postcompil(orchids_t *ctx, mod_entry_t *mod)
{
  /* Do all thing needed _AFTER_ rule compilation. */
}


static void
set_hash_size(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  mod_sharedvars_cfg_g->hash_size = atoi(dir->args);
  DebugLog(DF_MOD, DS_INFO,
           "setting some_option to %i\n",
           mod_sharedvars_cfg_g->hash_size);
}


static mod_cfg_cmd_t sharedvars_config_commands[] = {
  { "HashSize", set_hash_size, "Set the size of the hash" },
  { NULL, NULL, NULL }
};


input_module_t mod_sharedvars = {
  MOD_MAGIC,                /* Magic number */
  ORCHIDS_VERSION,          /* Module version */
  "sharedvars",             /* module name */
  "CeCILL2",                /* module license */
  NULL               ,      /* module dependencies */
  sharedvars_config_commands,   /* module configuration commands,
                               for core config parser */
  sharedvars_preconfig,         /* called just after module registration */
  sharedvars_postconfig,        /* called after all mods preconfig,
                               and after all module configuration*/
  sharedvars_postcompil
};


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
