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
#include <stdint.h>

#include "orchids.h"
#include "ovm.h"
#include "strhash.h"

#include "mod_sharedvars.h"


input_module_t mod_sharedvars;

static sharedvars_config_t *mod_sharedvars_cfg_g = NULL;


static void issdl_get_shared_var(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *varname;
  char *key;
  ovm_var_t *value;

  varname = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  if (varname==NULL || (TYPE(varname)!=T_STR && TYPE(varname)!=T_VSTR))
    {
      DebugLog(DF_MOD, DS_ERROR, "parameter type error\n");
      STACK_DROP(ctx->ovm_stack, 1);
      PUSH_VALUE(ctx, NULL);
      return;
    }

  key = ovm_strdup(ctx->gc_ctx, varname);

  value = strhash_get(mod_sharedvars_cfg_g->vars_hash, key);
  /* XXX should really implement a strnhash_get, taking a char * and
     its len, and eliminate the call to ovm_strdup(). */
  STACK_DROP(ctx->ovm_stack, 1);
  PUSH_VALUE(ctx, value);
  gc_base_free(key);
}


static void issdl_del_shared_var(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *varname;
  char *key;
  ovm_var_t *value;

  varname = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  if (varname==NULL || (TYPE(varname)!=T_STR && TYPE(varname)!=T_VSTR))
    {
      DebugLog(DF_MOD, DS_ERROR, "parameter type error\n");
      STACK_DROP(ctx->ovm_stack, 1);
      PUSH_RETURN_FALSE(ctx);
      return;
    }

  key = ovm_strdup(ctx->gc_ctx, varname);

  value = gc_strhash_del(mod_sharedvars_cfg_g->vars_hash, key);
  //GC_TOUCH (ctx->gc_ctx, mod_sharedvars_cfg_g);

  if (value!=NULL)
    {
      STACK_DROP(ctx->ovm_stack, 1);
      PUSH_RETURN_TRUE(ctx);
    }
  else
    {
      DebugLog(DF_MOD, DS_ERROR,
	       "Try to delete an non-existent variable '%s'\n",
	       key);
      STACK_DROP(ctx->ovm_stack, 1);
      PUSH_RETURN_FALSE(ctx);
    }
  gc_base_free(key);
}


static void issdl_set_shared_var(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *varname;
  ovm_var_t *value;
  char *key;

  varname = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  if (varname==NULL || (TYPE(varname)!=T_STR && TYPE(varname)!=T_VSTR))
    {
      DebugLog(DF_MOD, DS_ERROR, "shared var name not a string\n");
      STACK_DROP(ctx->ovm_stack, 2);
      PUSH_RETURN_FALSE(ctx);
      return;
    }
  value = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  if (value==NULL || (TYPE(value)!=T_STR && TYPE(value)!=T_VSTR))
    {
      DebugLog(DF_MOD, DS_ERROR, "shared value not a string\n");
      STACK_DROP(ctx->ovm_stack, 2);
      PUSH_RETURN_FALSE(ctx);
      return;
    }

  key = ovm_strdup(ctx->gc_ctx, varname);
  DebugLog(DF_MOD, DS_INFO, "Setting shared var '%s'\n", key);

  (void) gc_strhash_update_or_add(ctx->gc_ctx,
				  mod_sharedvars_cfg_g->vars_hash,
				  value, key);
  GC_TOUCH (ctx->gc_ctx, mod_sharedvars_cfg_g);
  PUSH_RETURN_TRUE(ctx);
}

static int sharedvars_config_mark_hash_walk_func (void *elt, void *data)
{
  GC_TOUCH ((gc_t *)data, elt);
  return 0;
}

static void sharedvars_config_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  sharedvars_config_t *ctx = (sharedvars_config_t *)p;

  if (ctx->vars_hash!=NULL)
    (void) strhash_walk (ctx->vars_hash,
			 sharedvars_config_mark_hash_walk_func,
			 (void *)gc_ctx);
}

static void sharedvars_config_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  sharedvars_config_t *ctx = (sharedvars_config_t *)p;

  if (ctx->vars_hash!=NULL)
    gc_free_strhash (ctx->vars_hash, NULL);
}

struct shv_data {
  gc_traverse_ctx_t *gtc;
  void *data;
};

static int sharedvars_config_traverse_hash_walk_func (void *elt, void *data)
{
  struct shv_data *walk_data = (struct shv_data *)data;

  return (*walk_data->gtc->do_subfield) (walk_data->gtc, (gc_header_t *)elt,
					 walk_data->data);
}

static int sharedvars_config_traverse (gc_traverse_ctx_t *gtc, gc_header_t *p,
				   void *data)
{
  sharedvars_config_t *ctx = (sharedvars_config_t *)p;
  int err = 0;
  struct shv_data walk_data = { gtc, data };

  if (ctx->vars_hash!=NULL)
    err = strhash_walk (ctx->vars_hash,
			sharedvars_config_traverse_hash_walk_func,
			(void *)&walk_data);
  return err;
}

static gc_class_t sharedvars_config_class = {
  GC_ID('s','h','r','d'),
  sharedvars_config_mark_subfields,
  sharedvars_config_finalize,
  sharedvars_config_traverse
};

static const type_t *shget_sig[] = { &t_str, &t_str };
static const type_t **shget_sigs[] = { shget_sig, NULL };

static const type_t *shdel_sig[] = { &t_int, &t_str }; /* returns 0 or 1, in fact */
static const type_t **shdel_sigs[] = { shdel_sig, NULL };

static const type_t *shset_sig[] = { &t_int, &t_str, &t_str }; /* returns 0 or 1, in fact */
static const type_t **shset_sigs[] = { shset_sig, NULL };

struct node_expr_s; /* in rule_compiler.h */
monotony m_shset (rule_compiler_t *ctx, struct node_expr_s *e, monotony m[])
{
  m[0] |= MONO_THRASH; /* thrashes the first argument (shared variable) */
  return MONO_UNKNOWN | MONO_THRASH;
}

static void *sharedvars_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  sharedvars_config_t *cfg;

  DebugLog(DF_MOD, DS_INFO, "load() sharedvars@%p\n", &mod_sharedvars);

  /* allocate some memory for module configuration
  ** and initialize default configuration. */
  cfg = gc_alloc(ctx->gc_ctx, sizeof (sharedvars_config_t),
		 &sharedvars_config_class);
  cfg->gc.type = T_NULL;
  cfg->hash_size = DEFAULT_SHAREDVARS_HASH_SZ;
  cfg->vars_hash = NULL;
  mod_sharedvars_cfg_g = cfg;
  gc_add_root(ctx->gc_ctx, (gc_header_t **)&mod_sharedvars_cfg_g);

  register_lang_function(ctx,
                         issdl_set_shared_var,
                         "set_shared_var",
			 2, shset_sigs,
			 m_shset,
                         "Set or update a shared variable");

  register_lang_function(ctx,
                         issdl_get_shared_var,
                         "get_shared_var",
			 1, shget_sigs,
			 m_unknown_1,
                         "Get a shared variable");

  register_lang_function(ctx,
                         issdl_del_shared_var,
                         "del_shared_var",
			 1, shdel_sigs,
			 m_shset,
                         "Delete a shared variable");

  /* return config structure, for module manager */
  return cfg;
}


static void sharedvars_postconfig(orchids_t *ctx, mod_entry_t *mod)
{
  sharedvars_config_t *cfg;

  cfg = (sharedvars_config_t *)mod->config;
  cfg->vars_hash = new_strhash(ctx->gc_ctx, cfg->hash_size);
  GC_TOUCH (ctx->gc_ctx, cfg);
}


static void sharedvars_postcompil(orchids_t *ctx, mod_entry_t *mod)
{
  /* Do all thing needed _AFTER_ rule compilation. */
}


static void set_hash_size(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  mod_sharedvars_cfg_g->hash_size = strtol(dir->args, (char **)NULL, 10);
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
  0,			    /* flags */
  "sharedvars",             /* module name */
  "CeCILL2",                /* module license */
  NULL               ,      /* module dependencies */
  sharedvars_config_commands,   /* module configuration commands,
                               for core config parser */
  sharedvars_preconfig,         /* called just after module registration */
  sharedvars_postconfig,        /* called after all mods preconfig,
                               and after all module configuration*/
  sharedvars_postcompil,
  NULL,
  NULL,
  NULL
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
