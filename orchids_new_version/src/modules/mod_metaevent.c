/**
 ** @file mod_metaevent.c
 ** The metaevent module.
 **
 ** @author Baptiste GOURDIN <baptiste.gourdin@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 **
 ** @date  Started on: Mon Jun 20 20:40:39 CEST 2011
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "orchids.h"
#include "evt_mgr.h"
#include "mod_mgr.h"
#include "orchids_api.h"
#include "mod_metaevent.h"


input_module_t mod_metaevent;

static void issdl_inject_event(orchids_t *ctx, state_instance_t *state)
{
  static mod_entry_t		*mod = NULL;
  ovm_var_t		*ptr;

  if (mod==NULL)
    mod = find_module_entry(ctx, "metaevent");
  ptr = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  if (ptr!=NULL && TYPE(ptr)==T_EVENT) /* don't inject an empty event */
    {
      register_rtcallback (ctx,
			   rtaction_inject_event,
			   (gc_header_t *)ptr,
			   (void *)mod,
			   0);
    }
  STACK_DROP(ctx->ovm_stack, 1);
}

static int rtaction_inject_event(orchids_t *ctx, heap_entry_t *he)
{
  mod_entry_t *mod;
  event_t *evt;

  DebugLog(DF_MOD, DS_TRACE,
           "Real-time action: Checking event to inject...\n");

  evt = (event_t *)he->gc_data;
  mod = (mod_entry_t *)he->data;
  post_event(ctx, mod, evt);
  gc_base_free (he);
  return 0;
}


static void *metaevent_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  metaevent_config_t *cfg;

  DebugLog(DF_MOD, DS_INFO, "load() metaevent@%p\n", &mod_metaevent);

  cfg = gc_base_malloc(ctx->gc_ctx, sizeof (metaevent_config_t));

  cfg->mod_hash = new_strhash(ctx->gc_ctx, 257);
  STAILQ_INIT(&cfg->vmod_list);

  register_lang_function(ctx,
			 issdl_inject_event,
			 "inject_event", 1,
			 "Inject the event into the orchids engine");
  return cfg;
}

static void metaevent_postconfig(orchids_t *ctx, mod_entry_t *mod)
{
  meta_vmod_t *vmod;
  meta_field_t *field;
  int i;
  size_t vmodlen, namelen;
  char *field_name;
  metaevent_config_t *cfg = mod->config;


  /* register hooks (as generic modules)
   * then, the registered callback will post event as the virtual mod matching the field */

  STAILQ_FOREACH(vmod, &cfg->vmod_list, vmods)
    {
      DebugLog(DF_MOD, DS_DEBUG, "*** adding mod [%s]\n", vmod->name);

      vmod->field_array = gc_base_malloc(ctx->gc_ctx,
					 vmod->nfields * sizeof (field_t));
      GC_TOUCH (ctx->gc_ctx, vmod->fields = new_field_table (ctx->gc_ctx, vmod->nfields));
      gc_add_root (ctx->gc_ctx, (gc_header_t **)&vmod->fields);

      /* build field array */
      i = 0;
      STAILQ_FOREACH(field, &vmod->field_globlist, globfields)
	{
	  vmodlen = strlen(vmod->name);
	  namelen = strlen(field->name);
	  field_name = gc_base_malloc (ctx->gc_ctx, vmodlen+namelen+2);
	  memcpy (field_name, vmod->name, vmodlen);
	  field_name[vmodlen] = '.';
	  strcpy (field_name+vmodlen+1, field->name);
	  DebugLog(DF_MOD, DS_DEBUG,
		   "  *** adding field %i [%s]\n",
		   i, field_name);
	  vmod->field_array[i].name = field_name;
	  vmod->field_array[i].type = field->type;
	  vmod->field_array[i].desc = field->description;
	  i++;
	}

      /* add modules */
      vmod->mod_id = add_module(ctx, &vmod->mod_entry, NULL);
      DebugLog(DF_MOD, DS_DEBUG, "*** adding mod [%s] %i\n",
	       vmod->name, vmod->nfields);
      /* register fields */
      register_fields(ctx,
		      &ctx->mods[vmod->mod_id],
		      vmod->field_array,
		      vmod->nfields);
    }
}

static void metaevent_postcompil(orchids_t *ctx, mod_entry_t *mod)
{
  /* Do all thing needed _AFTER_ rule compilation. */
}

static void add_field(orchids_t *ctx,
		      meta_vmod_t *v,
		      config_directive_t *field_dir)
{
  meta_field_t *f, *f2;
  char buf[64]; /* <- argh ! */
  char comment_buf[256]; /* <- argh ! */
  /* add a field match here */

  f = gc_base_malloc(ctx->gc_ctx, sizeof (meta_field_t));
  STAILQ_NEXT(f, fields) = NULL;
  STAILQ_NEXT(f, globfields) = NULL;
  f->field_id = 0;
  f->type = 0;
  f->name = NULL;
  f->description = NULL;

  if ( !strcmp(field_dir->directive, "str_field") ) {
    f->type = T_VSTR;
  }
  else if ( !strcmp(field_dir->directive, "int_field") ) {
    f->type = T_INT;
  }
  else if ( !strcmp(field_dir->directive, "ip4_field") ) {
    f->type = T_IPV4;
  }
  else if ( !strcmp(field_dir->directive, "flt_field") ) {
    f->type = T_FLOAT;
  }
  else {
    DebugLog(DF_MOD, DS_FATAL,
             "Unimplemented type '%s'\n",
             field_dir->directive);
    exit(EXIT_FAILURE);
  }

  buf[0] = '\0';
  comment_buf[0] = '\0';
  sscanf(field_dir->args, "%64s %256[^\n]", buf, comment_buf);
  f->name = strdup(buf);
  if (comment_buf[0] != '\0')
    f->description = strdup(comment_buf);

  STAILQ_INSERT_TAIL(&v->field_list, f, fields);

  if ((f2 = strhash_get(v->field_hash, f->name)) == NULL) {
    DebugLog(DF_MOD, DS_DEBUG,
             "    Adding field [%s] into hash and globlist\n",
             f->name);
    strhash_add(v->field_hash, f, f->name);
    f->field_id = v->nfields++;
    STAILQ_INSERT_TAIL(&v->field_globlist, f, globfields);
  } else {
    f->field_id = f2->field_id;
  }

  DebugLog(DF_MOD, DS_DEBUG,
           "    Adding field [%s]\n",
           f->name);
}


static void add_vmod(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  meta_vmod_t *vmod;
  char *mod_name;
  metaevent_config_t	*cfg = mod->config;
  config_directive_t *field_dir;

  /* add a virtual module */

  mod_name = strdup(dir->args);
  mod_name[ strlen(mod_name) - 1 ] = '\0';

  vmod = strhash_get(cfg->mod_hash, mod_name);

  if (vmod == NULL)
    {
      vmod = gc_base_malloc(ctx->gc_ctx, sizeof (meta_vmod_t));
      STAILQ_NEXT(vmod, vmods) = NULL;
      STAILQ_NEXT(vmod, globvmods) = NULL;
      STAILQ_INIT(&vmod->field_list);
      STAILQ_INIT(&vmod->field_globlist);
      vmod->name = mod_name;
      vmod->mod_id = 0;
      vmod->mod_entry.magic = MOD_MAGIC;
      vmod->mod_entry.version = ORCHIDS_VERSION;
      vmod->mod_entry.name = vmod->name;
      vmod->mod_entry.license = mod_metaevent.license;
      vmod->mod_entry.dependencies = NULL;
      vmod->mod_entry.cfg_cmds = NULL;
      vmod->mod_entry.pre_config = NULL;
      vmod->mod_entry.post_config = NULL;
      vmod->mod_entry.post_compil = NULL;
      vmod->field_array = NULL;
      vmod->nfields = 0;
      vmod->fields = NULL;
      vmod->field_hash = new_strhash(ctx->gc_ctx, 257);

      DebugLog(DF_MOD, DS_DEBUG, "  Adding virtual module [%s]\n", vmod->name);

      if (find_module(ctx, vmod->name) != NULL)
	{
	  DebugLog(DF_MOD, DS_DEBUG,
		   "warning! module [%s] already loaded...\n",
		   vmod->name);
	  exit(EXIT_FAILURE);
	}

      strhash_add(cfg->mod_hash, vmod, vmod->name);
      cfg->mods++;
      STAILQ_INSERT_TAIL(&cfg->vmod_list, vmod, vmods);
    }
  else
    {
      DebugLog(DF_MOD, DS_DEBUG, "  Reuse virtual module [%s]\n", vmod->name);
    }

  for (field_dir = dir->first_child;
       field_dir!=NULL;
       field_dir = field_dir->next)
    add_field(ctx, vmod, field_dir);

  DebugLog(DF_MOD, DS_DEBUG, "end\n");
}



static mod_cfg_cmd_t metaevent_cfgcmds[] = {
  { "<vmod", add_vmod, "Begin a (conditionnal or unconditional) hook section" },
  { NULL, NULL, NULL }
};


input_module_t mod_metaevent = {
  MOD_MAGIC,                /* Magic number */
  ORCHIDS_VERSION,          /* Module version */
  "metaevent",		    /* module name */
  "CeCILL2",                /* module license */
  NULL,			    /* module dependencies */
  metaevent_cfgcmds,	    /* module configuration commands,
                               for core config parser */
  metaevent_preconfig,       /* called just after module registration */
  metaevent_postconfig,      /* called after all mods preconfig,
                               and after all module configuration*/
  metaevent_postcompil
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
