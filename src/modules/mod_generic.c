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


static int
generic_dissect(orchids_t *ctx, mod_entry_t *mod, event_t *event, void *data)
{
  char *txt_line;
  int txt_len;
  generic_vmod_t *vmod;
  generic_field_t *field;
  generic_match_t *match;
  regmatch_t regmatch[256];
  int ret;
/*   int i; */
  char buf[4096];
  generic_hook_t *hook;

  if (TYPE(event->value) == T_STR) {
    txt_line = STR(event->value);
    txt_len = STRLEN(event->value);
  } else if (TYPE(event->value) == T_VSTR) {
    txt_line = VSTR(event->value);
    txt_len = VSTRLEN(event->value);
  } else {
    DebugLog(DF_MOD, DS_ERROR, "bad input type\n");
    return (1);
  }

  hook = data;

  memcpy(buf, txt_line, txt_len);
  buf[ txt_len ] = '\0';

  DebugLog(DF_MOD, DS_DEBUG, "process line [%s]\n", buf);

  STAILQ_FOREACH(vmod, &hook->vmod_list, vmods) {
    DebugLog(DF_MOD, DS_DEBUG, "enter vmod [%s]\n", vmod->name);

    STAILQ_FOREACH(match, &vmod->match_list, matches) {
      DebugLog(DF_MOD, DS_DEBUG, "  enter match [%s]\n", match->regex_str);

      ret = regexec(&match->regex, buf, 255, regmatch, 0);
      if (ret) {
        char err_buf[64];
        regerror(ret, &match->regex, err_buf, sizeof (err_buf));
        DebugLog(DF_MOD, DS_DEBUG, "regexec() error (%s)\n", err_buf);
        continue ;
      }

      DebugLog(DF_MOD, DS_DEBUG, "regexec() MATCH\n");

      memset(vmod->field_values, 0, vmod->fields * sizeof (ovm_var_t *));

      STAILQ_FOREACH(field, &match->field_list, fields) {
        char buff[4096];
        size_t res_sz;
        ovm_var_t *res;
/*         event_t *new_event; */

        res_sz = regmatch[ field->substring ].rm_eo
               - regmatch[ field->substring ].rm_so;
        if (res_sz >= sizeof (buff))
          res_sz = sizeof (buff) - 1;
        memcpy(buff,
               &txt_line[ regmatch[ field->substring ].rm_so ],
               res_sz);
        buff[ res_sz ] = '\0';
        DebugLog(DF_MOD, DS_DEBUG,
                 "field '%s' %i (%i): \"%s\"\n",
                 field->name, field->substring, field->field_id, buff);

        switch (field->type) {

        case T_VSTR:
          res = ovm_vstr_new();
          VSTR(res) = &txt_line[ regmatch[ field->substring ].rm_so ];
          VSTRLEN(res) = res_sz;
          break;

        case T_INT:
          res = ovm_int_new();
          INT(res) = atoi(buff);
          break;

        case T_IPV4:
          res = ovm_ipv4_new();
          if ( inet_aton(buff, &IPV4(res)) == 0) {
            DebugLog(DF_MOD, DS_ERROR,
                     "Error in IPV4 convertion of (%s)\n",
                     buff);
            return (1);
          }
          break;

        case T_FLOAT:
          res = ovm_float_new();
          FLOAT(res) = atof(buff);
          break;

        default:
          DebugLog(DF_MOD, DS_ERROR, "Unknown field type\n", field->type);
          return (1);
          break;
        }

        vmod->field_values[ field->field_id ] = res;

/*         new_event = Xzmalloc(sizeof (event_t)); */
/* /\*         new_event->field_id = res; *\/ */
/*         new_event->value = res; */
/*         new_event->next = event; */
/*         event = new_event; */
      }

      add_fields_to_event(ctx,
                          &ctx->mods[vmod->mod_id],
                          &event,
                          vmod->field_values,
                          vmod->fields);

      post_event(ctx, &ctx->mods[vmod->mod_id], event);

      return (0);
    }
  }

  DebugLog(DF_MOD, DS_DEBUG, "No match\n");

  return (1); /* 1  E_NOMATCH */
}


static void *
generic_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  mod_generic_cfg_t *mod_cfg;

  DebugLog(DF_MOD, DS_DEBUG, "load() generic@%p\n", (void *) &mod_generic);

  mod_cfg = Xzmalloc(sizeof (mod_generic_cfg_t));
  gen_cfg_g = mod_cfg;

  mod_cfg->mod_hash = new_strhash(257);
  STAILQ_INIT(&mod_cfg->vmod_globlist);

  return (mod_cfg);
}


static void
generic_postconfig(orchids_t *ctx, mod_entry_t *mod)
{
  generic_hook_t *hook;
  generic_vmod_t *vmod;
  generic_field_t *field;
  int i;
  char field_name[128]; /* XXX: correct this ! */

  /* register hooks (as generic modules)
   * then, the registered callback will post event as the virtual mod matching the field */

  STAILQ_FOREACH(vmod, &gen_cfg_g->vmod_globlist, globvmods) {
    DebugLog(DF_MOD, DS_DEBUG, "*** adding mod [%s]\n", vmod->name);

    vmod->field_array = Xmalloc(vmod->fields * sizeof (field_t));
    vmod->field_values = Xzmalloc(vmod->fields * sizeof (ovm_var_t *));

    /* build field array */
    i = 0;
    STAILQ_FOREACH(field, &vmod->field_globlist, globfields) {
      snprintf(field_name, sizeof (field_name),
               "%s.%s",
               vmod->name, field->name);
      DebugLog(DF_MOD, DS_DEBUG,
               "  *** adding field %i [%s]\n",
               i, field_name);
      vmod->field_array[i].name = strdup(field_name);
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
                    vmod->fields);
  }

  /* register vmod hook stubs */
  for (i = 0, hook = gen_cfg_g->hook_array;
       (i < gen_cfg_g->used_hook); hook++, i++) {
    DebugLog(DF_MOD, DS_DEBUG, "Registering hook: module=%s condition=%s\n", 
             hook->module, hook->condition);
    register_conditional_dissector(ctx, mod, hook->module, hook->condition,
                                   strlen(hook->condition),
                                   generic_dissect, hook);
  }

  return ;
}


static void
add_field(orchids_t *ctx,
          generic_vmod_t *v,
          generic_match_t *m,
          config_directive_t *field_dir)
{
  generic_field_t *f, *f2;
  char buf[256]; /* <- argh ! */
  char comment_buf[64]; /* <- argh ! */
  /* add a field match here */

  f = Xzmalloc(sizeof (generic_field_t));

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
  sscanf(field_dir->args,
         "%64s %i %64[^\n]",
         buf, &f->substring, comment_buf);
  f->name = strdup(buf);
  if (comment_buf[0] != '\0')
    f->description = strdup(comment_buf);

  STAILQ_INSERT_TAIL(&m->field_list, f, fields);
  m->fields++;

  if ((f2 = strhash_get(v->field_hash, f->name)) == NULL) {
    DebugLog(DF_MOD, DS_DEBUG,
             "    Adding field [%s] into hash and globlist\n",
             f->name, f->substring);
    strhash_add(v->field_hash, f, f->name);
    f->field_id = v->fields++;
    STAILQ_INSERT_TAIL(&v->field_globlist, f, globfields);
  } else {
    f->field_id = f2->field_id;
  }

  DebugLog(DF_MOD, DS_DEBUG,
           "    Adding field [%s] %i\n",
           f->name, f->substring);
}


static void
add_fmatch(orchids_t *ctx, generic_vmod_t *v, config_directive_t *fmatch_dir)
{
  generic_match_t *m;
  config_directive_t *field_dir;
  int ret;

  m = Xzmalloc(sizeof (generic_match_t));
  STAILQ_INIT(&m->field_list);
  m->regex_str = strdup(fmatch_dir->args + 1);
  m->regex_str[ strlen(m->regex_str) - 2] = '\0';

  ret = regcomp(&m->regex, m->regex_str, REG_EXTENDED);
  if (ret) {
    char err_buf[64];

    DebugLog(DF_MOD, DS_FATAL,
             "regex compilation error (%s)\n",
             m->regex_str);
    regerror(ret, &m->regex, err_buf, sizeof (err_buf));
    exit(EXIT_FAILURE);
  }

  DebugLog(DF_MOD, DS_DEBUG, "    Adding field match [%s]\n", m->regex_str);

  for (field_dir = fmatch_dir->first_child;
       field_dir;
       field_dir = field_dir->next) {
    add_field(ctx, v, m, field_dir);
  }
  STAILQ_INSERT_TAIL(&v->match_list, m, matches);
}


static void
add_vmod(orchids_t *ctx, generic_hook_t *h, config_directive_t *vmod_dir)
{
  generic_vmod_t *vmod;
  config_directive_t *fmatch_dir;
  char *mod_name;
  /* add a virtual module */

  mod_name = strdup(vmod_dir->args);
  mod_name[ strlen(mod_name) - 1 ] = '\0';

  vmod = strhash_get(gen_cfg_g->mod_hash, mod_name);

  if (vmod == NULL) {
    vmod = Xzmalloc(sizeof (generic_vmod_t));
    STAILQ_INIT(&vmod->match_list);
    STAILQ_INIT(&vmod->field_globlist);
    vmod->name = mod_name;
    vmod->mod_entry.magic = MOD_MAGIC;
    vmod->mod_entry.version = ORCHIDS_VERSION;
    vmod->mod_entry.license = mod_generic.license;
    vmod->mod_entry.name = vmod->name;
    vmod->field_hash = new_strhash(257);

    DebugLog(DF_MOD, DS_DEBUG, "  Adding virtual module [%s]\n", vmod->name);

    if (find_module(ctx, vmod->name) != NULL) {
      DebugLog(DF_MOD, DS_DEBUG,
               "warning! module [%s] already loaded...\n",
               vmod->name);
      exit(EXIT_FAILURE);
    }

    strhash_add(gen_cfg_g->mod_hash, vmod, vmod->name);
    gen_cfg_g->mods++;
    STAILQ_INSERT_TAIL(&gen_cfg_g->vmod_globlist, vmod, globvmods);
  }
  else {
    DebugLog(DF_MOD, DS_DEBUG, "  Reuse virtual module [%s]\n", vmod->name);
  }

  STAILQ_INSERT_TAIL(&h->vmod_list, vmod, vmods);

  for (fmatch_dir = vmod_dir->first_child;
       fmatch_dir;
       fmatch_dir = fmatch_dir->next) {
    if (strcmp(fmatch_dir->directive, "<fieldmatch")) {
      fprintf(stderr,
              "bad directive ('%s' instead of '<fieldmatch')\n",
              fmatch_dir->directive);
      exit(EXIT_FAILURE);
    }
    add_fmatch(ctx, vmod, fmatch_dir);
  }
  DebugLog(DF_MOD, DS_DEBUG, "end\n");
}


static void
add_hook(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  generic_hook_t *h;
  config_directive_t *vmod_dir;
  char mod_buf[64], mod_cond[256];
  size_t new_array_size;

  new_array_size = (gen_cfg_g->used_hook + 1) * sizeof (generic_hook_t);
  gen_cfg_g->hook_array = Xrealloc(gen_cfg_g->hook_array, new_array_size);

  DebugLog(DF_MOD, DS_DEBUG,
           "Adding hook #%i [%s]\n",
           gen_cfg_g->used_hook, dir->args);

  h = &gen_cfg_g->hook_array[ gen_cfg_g->used_hook++ ];
  STAILQ_INIT(&h->vmod_list);
  sscanf(dir->args, "%s \"%[^\"]\"", mod_buf, mod_cond);
  h->module = strdup(mod_buf);
  h->condition = strdup(mod_cond);

  for (vmod_dir = dir->first_child; vmod_dir; vmod_dir = vmod_dir->next) {
    if (strcmp(vmod_dir->directive, "<vmod")) {
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
  { "<hook", add_hook, "Begin a (conditionnal or unconditional) hook section" },
  { NULL, NULL, NULL }
};

input_module_t mod_generic = {
  MOD_MAGIC,
  ORCHIDS_VERSION,
  "generic",
  "CeCILL2",
  NULL,
  generic_config_commands,
  generic_preconfig,
  generic_postconfig,
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
