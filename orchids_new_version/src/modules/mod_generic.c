/**
 ** @file mod_generic.c
 ** Generic event generator from misc text sources.
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "orchids.h"

#include "orchids_api.h"
#include "mod_mgr.h"
#include "stailq.h"

#include "mod_generic.h"

input_module_t mod_generic;

mod_generic_cfg_t *gen_cfg_g;


static int generic_dissect(orchids_t *ctx, mod_entry_t *mod, event_t *event,
			   void *data)
{
  char *txt_line;
  int txt_len;
  generic_vmod_t *vmod;
  generic_field_t *field, *field2;
  generic_match_t *match;
  regmatch_t regmatch[256];
  int ret;
#ifndef HAVE_REGNEXEC
  char *buf;
#endif
  generic_hook_t *hook;
  field_table_t *fields;
  int err;

  err = 0;
  if (event->value==NULL)
    {
      DebugLog(DF_MOD, DS_DEBUG, "NULL event value\n");
      return -1;
    }
  switch (TYPE(event->value))
    {
    case T_STR: txt_line = STR(event->value); txt_len = STRLEN(event->value);
      break;
    case T_VSTR: txt_line = VSTR(event->value); txt_len = VSTRLEN(event->value);
      break;
    default:
      DebugLog(DF_MOD, DS_DEBUG, "event value not a string\n");
      return -1;
    }

  hook = data;

#ifndef HAVE_REGNEXEC
  /* The following is costly, and can be avoided if we have regnexec()
     instead of regexec().
  */
  buf = ovm_strdup (ctx->gc_ctx, event->value);
  DebugLog(DF_MOD, DS_DEBUG, "mod_generic: process line [%s]\n", buf);
#else
  DebugLog(DF_MOD, DS_DEBUG, "mod_generic: process line\n");
#endif

  GC_START (ctx->gc_ctx, 1);
  GC_UPDATE (ctx->gc_ctx, 0, event);

  STAILQ_FOREACH(vmod, &hook->vmod_list, vmods)
    {
      DebugLog(DF_MOD, DS_DEBUG, "enter vmod [%s]\n", vmod->name);

      STAILQ_FOREACH(match, &vmod->match_list, matches)
	{
	  DebugLog(DF_MOD, DS_DEBUG, "  enter match [%s]\n", match->regex_str);

#ifdef HAVE_REGNEXEC
	  ret = regnexec(&match->regex, txt_line, txt_len, 255, regmatch, 0);
#else
	  ret = regexec(&match->regex, buf, 255, regmatch, 0);
#endif
	  if (ret)
	    {
	      char err_buf[64];

	      regerror(ret, &match->regex, err_buf, sizeof (err_buf));
	      DebugLog(DF_MOD, DS_DEBUG, "regexec() error (%s)\n", err_buf);
	      continue;
	    }

	  DebugLog(DF_MOD, DS_DEBUG, "regexec() MATCH\n");

	  fields = vmod->fields;
	  /* Should reset all fields to NULL,
	     but instead we shall reset only the fields read to NULL
	     on successful match. */
#if 0
	  for (i=0, n = fields->nfields; i<n; i++)
	    fields->field_values[i] = NULL;
#endif

	  STAILQ_FOREACH(field, &match->field_list, fields)
	    {
	      char *buff;
	      size_t res_sz;
	      ovm_var_t *res;

	      res_sz = regmatch[ field->substring ].rm_eo
		- regmatch[ field->substring ].rm_so;
	      buff = &txt_line[regmatch[ field->substring ].rm_so];
	      DebugLog(DF_MOD, DS_DEBUG,
                 "field '%s' %i (%i)\n",
		       field->name, field->substring, field->field_id);

	      switch (field->type->tag)
		{
		case T_STR:
		  res = ovm_vstr_new(ctx->gc_ctx, event->value);
		  VSTR(res) = &txt_line[regmatch[ field->substring ].rm_so];
		  VSTRLEN(res) = res_sz;
		  break;
		case T_INT:
		  {
		    long i;
		    (void) orchids_atoi(buff, res_sz, &i);
		    res = ovm_int_new(ctx->gc_ctx, i);
		  }
		  break;
		case T_IPV4:
		  res = ovm_ipv4_new(ctx->gc_ctx);
		  if (inet_pton(AF_INET, buff, &IPV4(res)) != 1)
		    {
		      DebugLog(DF_MOD, DS_ERROR,
			       "Error in IPV4 conversion\n");
		      err = 1;
		      goto end;
		    }
		  break;
		case T_IPV6:
		  res = ovm_ipv6_new(ctx->gc_ctx);
		  if (inet_pton(AF_INET6, buff, &IPV4(res)) != 1)
		    {
		      DebugLog(DF_MOD, DS_ERROR,
			       "Error in IPV6 conversion\n");
		      err = 1;
		      goto end;
		    }
		  break;
		case T_FLOAT:
		  {
		    double x;

		    (void) orchids_atof (buff, res_sz, &x);
		    res = ovm_float_new(ctx->gc_ctx, x);
		  }
		  break;
		default:
		  DebugLog(DF_MOD, DS_ERROR, "Unknown field type %s\n", field->type->name);
		  /* Reset the written to fields to NULL */
		  STAILQ_FOREACH(field2, &match->field_list, fields)
		    {
		      if (field2==field)
			break;
		      vmod->fields->field_values[field2->field_id] = NULL;
		    }
		  err = 1;
		  goto end;
		}
	      GC_TOUCH (ctx->gc_ctx,
			vmod->fields->field_values[field->field_id] = res);
	    }
      add_fields_to_event(ctx,
                          &ctx->mods[vmod->mod_id],
                          (event_t **)&GC_LOOKUP(0),
                          vmod->fields->field_values,
                          vmod->fields->nfields);
      /* Reset the written to fields to NULL */
      STAILQ_FOREACH(field2, &match->field_list, fields)
	{
	  vmod->fields->field_values[field2->field_id] = NULL;
	}
      post_event(ctx, &ctx->mods[vmod->mod_id], (event_t *)GC_LOOKUP(0));
      goto end;
    }
  }
  DebugLog(DF_MOD, DS_DEBUG, "No match\n");
  err = 1;
 end:
#ifndef HAVE_REGNEXEC
  gc_base_free(buf);
#endif
  GC_END(ctx->gc_ctx);
  return err; /* 1 = E_NOMATCH */
}


static void * generic_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  mod_generic_cfg_t *mod_cfg;

  DebugLog(DF_MOD, DS_DEBUG, "load() generic@%p\n", (void *) &mod_generic);

  mod_cfg = gc_base_malloc(ctx->gc_ctx, sizeof (mod_generic_cfg_t));
  gen_cfg_g = mod_cfg;

  mod_cfg->hook_array = NULL;
  mod_cfg->used_hook = 0;
  mod_cfg->mod_hash = new_strhash(ctx->gc_ctx, 257);
  mod_cfg->mods = 0;
  STAILQ_INIT(&mod_cfg->vmod_globlist);

  return mod_cfg;
}


static void generic_postconfig(orchids_t *ctx, mod_entry_t *mod)
{
  generic_hook_t *hook;
  generic_vmod_t *vmod;
  generic_field_t *field;
  size_t i, n;
  char *field_name;

  /* register hooks (as generic modules)
   * then, the registered callback will post event as the virtual mod matching the field */

  STAILQ_FOREACH(vmod, &gen_cfg_g->vmod_globlist, globvmods)
    {
      DebugLog(DF_MOD, DS_DEBUG, "*** adding mod [%s]\n", vmod->name);

      n = vmod->nfields;
      vmod->field_array = gc_base_malloc(ctx->gc_ctx, n*sizeof (field_t));

      GC_TOUCH (ctx->gc_ctx, vmod->fields = new_field_table(ctx->gc_ctx, n));
      gc_add_root(ctx->gc_ctx, (gc_header_t **)&vmod->fields);

      /* build field array */
      i = 0;
      STAILQ_FOREACH(field, &vmod->field_globlist, globfields)
	{
	  n = strlen (vmod->name) + strlen (field->name) + 2;
	  field_name = gc_base_malloc (ctx->gc_ctx, n);
	  snprintf(field_name, n,
		   "%s.%s",
		   vmod->name, field->name);
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

      /* register fields */
      register_fields(ctx,
		      &ctx->mods[vmod->mod_id],
		      vmod->field_array,
		      vmod->fields->nfields);
    }

  /* register vmod hook stubs */
  for (i = 0, hook = gen_cfg_g->hook_array;
       (i < gen_cfg_g->used_hook); hook++, i++)
    {
      DebugLog(DF_MOD, DS_DEBUG, "Registering hook: module=%s condition=%s\n", 
	       hook->module, hook->condition);
      register_conditional_dissector(ctx, mod, hook->module, hook->condition,
				     strlen(hook->condition),
				     generic_dissect, hook);
    }

  return;
}


static void add_field(orchids_t *ctx,
		      generic_vmod_t *v,
		      generic_match_t *m,
		      config_directive_t *field_dir)
{
  generic_field_t *f, *f2;
  char buf[64]; /* <- argh ! */
  char comment_buf[256]; /* <- argh ! */
  /* add a field match here */

  f = gc_base_malloc (ctx->gc_ctx, sizeof (generic_field_t));
  f->field_id = 0;
  f->type = 0;
  f->name = NULL;
  f->substring = 0;
  f->description = NULL;

  if (!strcmp(field_dir->directive, "str_field"))
    f->type = &t_str;
  else if (!strcmp(field_dir->directive, "int_field"))
    f->type = &t_int;
  else if (!strcmp(field_dir->directive, "ip4_field"))
    f->type = &t_ipv4;
  else if (!strcmp(field_dir->directive, "ip6_field"))
    f->type = &t_ipv6;
  else if (!strcmp(field_dir->directive, "flt_field"))
    f->type = &t_float;
  else
    {
      DebugLog(DF_MOD, DS_FATAL,
	       "Unimplemented type '%s'\n",
	       field_dir->directive);
      exit(EXIT_FAILURE);
    }

  buf[0] = '\0';
  comment_buf[0] = '\0';
  sscanf(field_dir->args,
         "%64s %i %256[^\n]",
         buf, &f->substring, comment_buf);
  f->name = gc_strdup(ctx->gc_ctx, buf);
  if (comment_buf[0] != '\0')
    f->description = gc_strdup(ctx->gc_ctx, comment_buf);

  STAILQ_INSERT_TAIL(&m->field_list, f, fields);
  m->fields++;

  if ((f2 = strhash_get(v->field_hash, f->name)) == NULL)
    {
      DebugLog(DF_MOD, DS_DEBUG,
	       "    Adding field [%s] into hash and globlist\n",
	       f->name, f->substring);
      strhash_add(v->field_hash, f, f->name);
      f->field_id = v->nfields++;
      STAILQ_INSERT_TAIL(&v->field_globlist, f, globfields);
    }
  else
    {
      f->field_id = f2->field_id;
    }

  DebugLog(DF_MOD, DS_DEBUG,
           "    Adding field [%s] %i\n",
           f->name, f->substring);
}


static void add_fmatch(orchids_t *ctx, generic_vmod_t *v,
		       config_directive_t *fmatch_dir)
{
  generic_match_t *m;
  config_directive_t *field_dir;
  int ret;
  size_t len;

  m = gc_base_malloc (ctx->gc_ctx, sizeof (generic_match_t));
  STAILQ_INIT(&m->field_list);
  m->fields = 0;
  m->regex_str = gc_strdup(ctx->gc_ctx, fmatch_dir->args + 1);
  len = strlen(m->regex_str);
  if (len>=2)
    m->regex_str[len - 2] = '\0'; /* !!! ??? */

  ret = regcomp(&m->regex, m->regex_str, REG_EXTENDED);
  if (ret)
    {
      char err_buf[64];

      DebugLog(DF_MOD, DS_FATAL,
	       "regex compilation error (%s)\n",
	       m->regex_str);
      regerror(ret, &m->regex, err_buf, sizeof (err_buf));
      exit(EXIT_FAILURE);
    }

  DebugLog(DF_MOD, DS_DEBUG, "    Adding field match [%s]\n", m->regex_str);

  for (field_dir = fmatch_dir->first_child;
       field_dir!=NULL;
       field_dir = field_dir->next)
    {
      add_field(ctx, v, m, field_dir);
    }
  STAILQ_INSERT_TAIL(&v->match_list, m, matches);
}


static void add_vmod(orchids_t *ctx, generic_hook_t *h,
		     config_directive_t *vmod_dir)
{
  generic_vmod_t *vmod;
  config_directive_t *fmatch_dir;
  char *mod_name;
  size_t len;
  /* add a virtual module */

  mod_name = gc_strdup(ctx->gc_ctx, vmod_dir->args);
  len = strlen(mod_name);
  if (len>=1)
  mod_name[len - 1] = '\0';

  vmod = strhash_get(gen_cfg_g->mod_hash, mod_name);

  if (vmod == NULL)
    {
      vmod = gc_base_malloc (ctx->gc_ctx, sizeof (generic_vmod_t));
      STAILQ_INIT(&vmod->match_list);
      STAILQ_INIT(&vmod->field_globlist);
      vmod->name = mod_name;
      vmod->mod_id = 0; /* temporary */
      vmod->mod_entry.magic = MOD_MAGIC;
      vmod->mod_entry.version = ORCHIDS_VERSION;
      vmod->mod_entry.license = mod_generic.license;
      vmod->mod_entry.name = vmod->name;
      vmod->mod_entry.dependencies = NULL;
      vmod->mod_entry.cfg_cmds = NULL;
      vmod->mod_entry.pre_config = NULL;
      vmod->mod_entry.post_config = NULL;
      vmod->mod_entry.post_compil = NULL;
      vmod->nfields = 0;
      vmod->field_array = NULL;
      vmod->field_hash = new_strhash(ctx->gc_ctx, 257);
      vmod->fields = NULL;

      DebugLog(DF_MOD, DS_DEBUG, "  Adding virtual module [%s]\n", vmod->name);

      if (find_module(ctx, vmod->name) != NULL)
	{
	  DebugLog(DF_MOD, DS_DEBUG,
		   "warning! module [%s] already loaded...\n",
		   vmod->name);
	  exit(EXIT_FAILURE);
	}

      strhash_add(gen_cfg_g->mod_hash, vmod, vmod->name);
      gen_cfg_g->mods++;
      STAILQ_INSERT_TAIL(&gen_cfg_g->vmod_globlist, vmod, globvmods);
    }
  else
    {
      DebugLog(DF_MOD, DS_DEBUG, "  Reuse virtual module [%s]\n", vmod->name);
    }

  STAILQ_INSERT_TAIL(&h->vmod_list, vmod, vmods);

  for (fmatch_dir = vmod_dir->first_child;
       fmatch_dir!=NULL;
       fmatch_dir = fmatch_dir->next)
    {
      if (strcmp(fmatch_dir->directive, "<fieldmatch"))
	{
	  fprintf(stderr,
		  "bad directive ('%s' instead of '<fieldmatch')\n",
		  fmatch_dir->directive);
	  exit(EXIT_FAILURE);
	}
      add_fmatch(ctx, vmod, fmatch_dir);
    }
  DebugLog(DF_MOD, DS_DEBUG, "end\n");
}

static void generic_add_hook(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  generic_hook_t *h;
  config_directive_t *vmod_dir;
  char mod_buf[64], mod_cond[256];
  size_t new_array_size;

  new_array_size = (gen_cfg_g->used_hook + 1) * sizeof (generic_hook_t);
  gen_cfg_g->hook_array = gc_base_realloc(ctx->gc_ctx,
					  gen_cfg_g->hook_array,
					  new_array_size);

  DebugLog(DF_MOD, DS_DEBUG,
           "Adding hook #%i [%s]\n",
           gen_cfg_g->used_hook, dir->args);

  h = &gen_cfg_g->hook_array[ gen_cfg_g->used_hook++ ];
  STAILQ_INIT(&h->vmod_list);
  sscanf(dir->args, "%64s \"%256[^\"]\"", mod_buf, mod_cond);
  h->module = gc_strdup(ctx->gc_ctx, mod_buf);
  h->condition = gc_strdup(ctx->gc_ctx, mod_cond);

  for (vmod_dir = dir->first_child; vmod_dir!=NULL; vmod_dir = vmod_dir->next)
    {
      if (strcmp(vmod_dir->directive, "<vmod"))
	{
	  fprintf(stderr,
		  "bad directive ('%s' instead of '<vmod')\n", 
		  vmod_dir->directive);
	  exit(EXIT_FAILURE);
	}
      add_vmod(ctx, h, vmod_dir);
    }
}


/* static mod_cfg_cmd_t fieldmatch_config_commands[] = { */
/*   { "str_field", NULL, "add a string field descriptor" }, */
/*   { "int_field", NULL, "Add an integer field descriptor"} */
/* }; */

/* static mod_cfg_cmd_t vmod_config_commands[] = { */
/*   { "<fieldmatch", NULL, "Add a matching rule" } */
/* }; */

/* static mod_cfg_cmd_t hook_config_commands[] = { */
/*   { "<vmod", NULL, "Add a matching rule" } */
/* }; */

static mod_cfg_cmd_t generic_config_commands[] = 
{
  { "<hook", generic_add_hook, "Begin a (conditionnal or unconditional) hook section" },
  { NULL, NULL, NULL }
};

input_module_t mod_generic = {
  MOD_MAGIC,
  ORCHIDS_VERSION,
  0,			    /* flags */
  "generic",
  "CeCILL2",
  NULL,
  generic_config_commands,
  generic_preconfig,
  generic_postconfig,
  NULL,
  generic_dissect,
  &t_str		    /* type of fields it expects to dissect */
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
