/**
 ** @file mod_htmlstate.c
 ** Generate HTML pages of the internal state of Orchids.
 ** Used to test/debug/orchids internals at runtime.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 ** @date  Started on: Mon Jan 27 17:32:49 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef MOD_HTMLSTATE_H
#define MOD_HTMLSTATE_H

#include "orchids.h"

#define HTMLSTATE_DEFAULT_REFRESH_DELAY 10

typedef struct htmlstatecfg_s htmlstatecfg_t;
struct htmlstatecfg_s
{
  int enable_cache;
  int fork;

  int auto_refresh_delay;
  int page_generation_delay;
  int cache_cleanup_delay;
  int rule_media; /* flags for graphs */
  int rule_limit;
  int rule_state_limit;
  int rule_instance_limit;
  int rule_instence_state_limit;
  int thread_limit;
  int event_limit;

  char*	HTML_output_dir;
};

static void *
htmlstate_preconfig(orchids_t *ctx, mod_entry_t *mod);

static void
htmlstate_postconfig(orchids_t *ctx, mod_entry_t *mod);

static void
enable_cache(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir);

static void
add_cache_params(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir);

int
mod_htmlstate_do_update(orchids_t *ctx, mod_entry_t *mod, void *params);



#endif /* MOD_HTMLSTATE_H */
