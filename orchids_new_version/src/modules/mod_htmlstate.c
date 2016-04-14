/**
 ** @file mod_htmlstate.c
 ** Generate HTML pages of the internal state of Orchids.
 ** Used to test/debug/orchids internals at runtime.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 ** @author Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
 **
 ** @version 0.2
 ** @ingroup modules
 **
 ** @date  Started on: Mon Jan 27 17:32:49 2003
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
#include "orchids_api.h"
#include "html_output.h"

#include "mod_htmlstate.h"

input_module_t mod_htmlstate;

static void *htmlstate_preconfig(orchids_t *ctx, mod_entry_t *mod);

static void htmlstate_postconfig(orchids_t *ctx, mod_entry_t *mod);

static void enable_cache(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir);

static void *htmlstate_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  html_output_cfg_t *mod_cfg;

  DebugLog(DF_MOD, DS_INFO,
           "loading htmlstate module @ %p\n", (void *) &mod_htmlstate);

  mod_cfg = gc_base_malloc(ctx->gc_ctx, sizeof (html_output_cfg_t));
  /* default initialisation here */
  mod_cfg->enable_cache = 0;
  mod_cfg->fork = 0;

  mod_cfg->auto_refresh_delay = 0;
  mod_cfg->page_generation_delay = 0;
  mod_cfg->cache_cleanup_delay = 0;;
  mod_cfg->rule_media = 0; /* flags for graphs */
  mod_cfg->rule_limit = 0;
  mod_cfg->rule_state_limit = 0;
  mod_cfg->rule_instance_limit = 0;
  mod_cfg->rule_instance_state_limit = 0;
  mod_cfg->thread_limit = 0;
  mod_cfg->event_limit = 0;

  mod_cfg->html_output_dir = NULL;

  mod_cfg->report_prefix = "report-";
  mod_cfg->report_ext = ".html";

  html_output_preconfig(ctx);

  return (mod_cfg);
}

static void
htmlstate_postconfig(orchids_t *ctx, mod_entry_t *mod)
{
  html_output_cfg_t *mod_cfg = mod->config;
//  html_output_postconfig(ctx, (html_output_cfg_t *)mod->config);

  /* register cache cleanup periodic job */
  register_rtcallback(ctx, rtaction_html_cache_cleanup, NULL, mod_cfg,
		      mod_cfg->auto_refresh_delay, 0);

  /* register regeneration job */
  register_rtcallback(ctx, rtaction_html_regeneration, NULL, mod_cfg,
		      mod_cfg->cache_cleanup_delay, 0);

  /* register report output*/
  register_report_output(ctx, mod, generate_html_report_cb, mod_cfg);
}

static void enable_cache(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  html_output_cfg_t *cfg;

  cfg = (html_output_cfg_t *)mod->config;
  cfg->enable_cache = 1;
}

static void set_HTML_output_dir(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  html_output_cfg_t *cfg = (html_output_cfg_t *)mod->config;
  struct stat s;
  char *path;

  path = adjust_path (ctx, dir->args);
  if (stat(path, &s) < 0)
    {
      fprintf (stderr, "HTMLOutputDir: cannot find '%s'.\n", path);
      fflush (stderr);
      exit(EXIT_FAILURE);
    }
  if (!(s.st_mode & S_IFDIR))
    {
      fprintf (stderr, "HTMLOutputDir: '%s' is not a directory.\n", path);
      fflush (stderr);
      exit(EXIT_FAILURE);
    }
  cfg->html_output_dir = path;
}

static void set_HTML_auto_refresh_delay(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  long value;
  
  html_output_cfg_t *cfg = (html_output_cfg_t *)mod->config;
  value = strtol(dir->args, (char **)NULL, 10);
  if (value < 1)
    value = 1;
  cfg->auto_refresh_delay = value;
}

static void set_HTML_cache_cleanup_delay(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  long value;
  
  html_output_cfg_t *cfg = (html_output_cfg_t *)mod->config;
  value = strtol(dir->args, (char **)NULL, 10);
  if (value < 1)
    value = 1;
  cfg->cache_cleanup_delay = value;
}


static mod_cfg_cmd_t htmlstate_dir[] =
{
  { "EnableCache", enable_cache, "Enable file cache" },
  { "HTMLOutputDir", set_HTML_output_dir, "set HTML output directory" },
  { "HTMLAutoRefreshDelay", set_HTML_auto_refresh_delay, "set delay before next HTML refresh" },
  { "HTMLCacheCleanupDelay", set_HTML_cache_cleanup_delay, "set delay between HTML cache cleanups" },
  { NULL, NULL }
};

input_module_t mod_htmlstate = {
  MOD_MAGIC,
  ORCHIDS_VERSION,
  0,			    /* flags */
  "htmlstate",
  "CeCILL2",
  NULL,
  htmlstate_dir,
  htmlstate_preconfig,
  htmlstate_postconfig,
  NULL, /* postcompil */
  NULL, /* predissect */
  NULL, /* dissect */
  NULL, /* dissect type */
  NULL, /* save */
  NULL, /* restore */
};


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
