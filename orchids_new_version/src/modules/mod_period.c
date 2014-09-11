/**
 ** @file mod_period.c
 ** Module for frequencies and phase analysis.
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
#include <string.h>

#include "orchids.h"

#include "file_cache.h"
#include "period.h"

#include "mod_period.h"

input_module_t mod_period;

static int
qsort_strcmp(const void *a, const void *b)
{
  return ( strcmp(*(char **)a, *(char **)b) );
}

static int
period_htmloutput(orchids_t *ctx, mod_entry_t *mod, FILE *menufp, html_output_cfg_t *htmlcfg)
{
  FILE *fp;
  int i;
  strhash_elmt_t *helmt;
  size_t ctx_array_sz;
  char **ctx_array;

  fprintf(menufp,
	  "<a href=\"orchids-period.html\" "
          "target=\"main\">Periods</a><br/>\n");

  fp = create_html_file(htmlcfg, "orchids-period.html", NO_CACHE);
  fprintf_html_header(fp, "Orchids frequencies / phases tables");

  fprintf(fp, "<center><h1>Orchids frequencies / phases tables</h1></center>\n");

  ctx_array = NULL;
  ctx_array_sz = 0;
  for (i = 0; i < ctx->temporal->size; i++) {
    for (helmt = ctx->temporal->htable[i]; helmt; helmt = helmt->next) {
      ctx_array_sz++;
      ctx_array = Xrealloc(ctx_array, ctx_array_sz * sizeof (char *));
      ctx_array[ ctx_array_sz - 1 ] = helmt->key;
/*       period_output_gnuplot(); */
    }
  }
  qsort(ctx_array, ctx_array_sz, sizeof (char *), qsort_strcmp);

  fprintf(fp, "%zd context%s<br/><br/><br/>\n",
          ctx_array_sz, ctx_array_sz > 1 ? "s" : "");

  for (i = 0; i < ctx_array_sz; i++)
    fprintf(fp, "%i: %s<br/>\n", i, ctx_array[i]);

  if (ctx_array_sz > 0)
    Xfree(ctx_array);

  fprintf_html_trailer(fp);
  Xfclose(fp);

  return (0);
}


static void
issdl_temporal(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *str;
  void *temp_ctx;
  char *key;

  /* XXX: if str is temp, clone ! */
  str = stack_pop(ctx->ovm_stack);
  if (TYPE(str) != T_STR) {
    DebugLog(DF_ENG, DS_ERROR, "parameter type error\n");
    ISSDL_RETURN_PARAM_ERROR(ctx, state);
    return ;
  }

  DebugLog(DF_ENG, DS_INFO, "Updating temporal information for container %s\n", STR(str));

  key = ovm_strdup(str);

  temp_ctx = strhash_get(ctx->temporal, key);

  if (temp_ctx == NULL) {
    DebugLog(DF_ENG, DS_INFO, "New container %s\n", key);
    /* create container ctx */
    /* add to hash */
    strhash_add(ctx->temporal, (void *)1, key);
  }
  else {
    Xfree(key);
  }

  /* update temporal info */
  ISSDL_RETURN_TRUE(ctx, state);
}

static void *
period_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  period_config_t *cfg;

  DebugLog(DF_MOD, DS_INFO, "load() period@%p\n", &mod_period);

  /* allocate some memory for module configuration
  ** and initialize default configuration. */
  cfg = Xzmalloc(sizeof (period_config_t));
  cfg->contexts = new_strhash(65537);

  register_lang_function(ctx, issdl_temporal,
                         "temporal", 1, "update a temporal context");

  html_output_add_menu_entry(ctx, mod, period_htmloutput);

  /* return config structure, for module manager */
  return (cfg);
}

static void
period_postconfig(orchids_t *ctx, mod_entry_t *mod)
{
  /* Do all thing needed _AFTER_ module configuration.
  ** (register configurable callbacks for examples) */
}

static void
period_postcompil(orchids_t *ctx, mod_entry_t *mod)
{
  /* Do all thing needed _AFTER_ rule compilation. */
}

#if 0
static void
set_some_option(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  int someoption;

  someoption = atoi(dir->args);
  DebugLog(DF_MOD, DS_INFO, "setting some_option to %i\n", someoption);

/*   ((period_config_t *)mod->config)->some_option = someoption; */
}
#endif

static mod_cfg_cmd_t period_config_commands[] =
{
  { NULL, NULL, NULL }
};

input_module_t mod_period = {
  MOD_MAGIC,                /* Magic number */
  ORCHIDS_VERSION,          /* Module version */
  "period",                 /* module name */
  "CeCILL2",                /* module license */
  NULL               ,      /* module dependencies */
  period_config_commands,   /* module configuration commands,
                               for core config parser */
  period_preconfig,         /* called just after module registration */
  period_postconfig,        /* called after all mods preconfig,
                               and after all module configuration*/
  period_postcompil
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
