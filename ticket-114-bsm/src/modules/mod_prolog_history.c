/**
 ** @file mod_prolog_history.c
 ** Update the prolog database from external databases
 ** according to events time field **
 ** @author Baptiste Gourdin <gourdin@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 ** @date  Started on: Mon Mar 14 16:01:18 CET 2011

 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */


#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_SWIPROLOG

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <time.h>

#include "orchids.h"
#include "orchids_api.h"
#include "file_cache.h"
#include "mod_mgr.h"
#include "mod_prolog.h"
#include "mod_prolog_history.h"

input_module_t mod_prolog_history;

static int
prolog_history_hook(orchids_t *ctx, mod_entry_t *mod, void *data, event_t *event)
{
  prolog_history_cfg_t *cfg;
  int		i;
  event_t	*e;
  char		buff[256];

  cfg = (prolog_history_cfg_t *)mod->config;

  for (i = 0; i < cfg->time_fields_nb; i++)
    for (e = event; e; e = e->next)
      if (cfg->time_fields_ids[i] == e->field_id)
      {
	DebugLog(DF_MOD, DS_DEBUG, "Followed time field %i (%s). "
		 "Current Db (%s).\n",
		 cfg->time_fields_ids[i], ctime(&(CTIME(e->value))),
		 ctime(&(SLIST_FIRST(&cfg->history)->time)));

	if (CTIME(e->value) > SLIST_FIRST(&cfg->history)->time)
	{
	  snprintf(buff, 256, "flushall, getall('%s').",
		   SLIST_FIRST(&cfg->history)->odbc_DSN);
	  int ret = pl_execute(buff);
	  DebugLog(DF_MOD, DS_DEBUG, "Update database to odbc %s.\n",
		   (SLIST_FIRST(&cfg->history)->odbc_DSN), ret);
	  if (ret)
	  {
	    SLIST_REMOVE_HEAD(&cfg->history, next);
	    gettimeofday(&cfg->prolog_cfg->last_db_update, NULL);
	  }
	}
	return 0;
      }

  return 0;
}

static void
add_odbc_history(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  prolog_history_cfg_t		*cfg;
  prolog_history_t	*history = NULL;
  char			*odbc;
  char			*times;
  struct tm		tm;
  time_t		timet;

  cfg = (prolog_history_cfg_t *)mod->config;

  memset(&tm, 0, sizeof(struct tm));

  DebugLog(DF_MOD, DS_INFO, "Add prolog odbc %s\n", dir->args);

  odbc = strchr(dir->args, ' ');

  times = strchr(dir->args, '/');
  *times = 0;
  tm.tm_mon = atoi(dir->args);

  dir->args = times + 1;
  times = strchr(dir->args, '/');
  *times = 0;
  tm.tm_mday = atoi(dir->args);

  dir->args = times + 1;
  tm.tm_year = atoi(dir->args) - 1900;

  timet = mktime(&tm);

  history = Xzmalloc(sizeof(prolog_history_t));

  history->time = timet;

  // XXX Need to strdup ?
  history->odbc_DSN = strdup(odbc + 1);

  SLIST_INSERT_HEAD(&cfg->history, history, next);
  DebugLog(DF_MOD, DS_INFO, "prolog %d\n", history->time);
}

static void
add_time_field(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  prolog_history_cfg_t		*cfg;


  cfg = (prolog_history_cfg_t *)mod->config;

  DebugLog(DF_MOD, DS_INFO, "Prolog history : Add time field %s\n",
	   dir->args);

  if (cfg->time_fields_nb >= 256)
  {
    DebugLog(DF_MOD, DS_ERROR, "Prolog history : cannot register more than 256 time fields\n");
    return;
  }
  cfg->time_fields_name[cfg->time_fields_nb++] = dir->args;
}

static void
build_time_fields_ids(orchids_t		*ctx,
		      prolog_history_cfg_t	*cfg)
{
  int	i = 0;
  int	k;
  int	nb = 0;
  int	found = 0;

  for (i = 0; i < cfg->time_fields_nb; i++)
  {
    found = 0;
    for (k = 0; k < ctx->num_fields; k++)
      if (!strcmp(cfg->time_fields_name[i], ctx->global_fields[k].name))
      {
	cfg->time_fields_ids[nb++] = ctx->global_fields[k].id;
	found = 1;
	break;
      }
    if (!found)
      DebugLog(DF_MOD, DS_ERROR, "Unknown field %s\n",
	       cfg->time_fields_name[i]);
  }
  cfg->time_fields_nb = nb;
}


static void *
prolog_history_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  prolog_history_cfg_t *cfg;
  mod_entry_t	*mod_prolog;

  DebugLog(DF_MOD, DS_INFO, "load() prolog_history@%p\n", &mod_prolog_history);

  if ((mod_prolog = find_module_entry(ctx, "prolog")) == NULL)
  {
    DebugLog(DF_MOD, DS_ERROR, "prolog module not found\n");
    return NULL;
  }

  cfg = Xzmalloc(sizeof (prolog_history_cfg_t));
  SLIST_INIT(&cfg->history);
  cfg->prolog_cfg = mod_prolog->config;

  return (cfg);
}

static void
prolog_history_postconfig(orchids_t *ctx, mod_entry_t *mod)
{
   prolog_history_cfg_t *cfg;

   cfg = (prolog_history_cfg_t *)mod->config;

   build_time_fields_ids(ctx, cfg);

   if (cfg->time_fields_nb)
     register_pre_inject_hook(ctx, mod, prolog_history_hook, NULL);
}

static mod_cfg_cmd_t prolog_history_config_commands[] =
{
  { "History", add_odbc_history, "Retreive the data from an external database" },
  { "FollowTimeField" , add_time_field, "Update prolog Database following this field value"},
  { NULL, NULL, NULL }
};

static char *prolog_history_deps[] = {
  "prolog",
  NULL
};

input_module_t mod_prolog_history = {
  MOD_MAGIC,		            /* Magic number */
  ORCHIDS_VERSION,		    /* Module version */
  "prolog_history",                  /* module name */
  "CeCILL2",                        /* module license */
  prolog_history_deps,              /* module dependencies */
  prolog_history_config_commands,   /* module configuration commands,
                                       for core config parser */
  prolog_history_preconfig,         /* called just after module registration */
  prolog_history_postconfig,        /* called after all mods preconfig,
                                       and after all module configuration*/
  NULL
};

#endif /* HAVE_SWIPROLOG */


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
