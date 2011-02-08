/**
 ** @file mod_mgr.c
 ** Manages orchids modules and plug-ins.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 1.0
 ** @ingroup core
 **
 ** @date  Started on: Fri Jan 17 16:57:51 2003
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
#include <dlfcn.h>
#include <limits.h>

#include "mod_mgr.h"

#include "orchids.h"


input_module_t *
load_add_shared_module(orchids_t *ctx, const char *name)
{
  char mod_fname[PATH_MAX];
  void *mod_handle;
  input_module_t *mod;

  snprintf(mod_fname, sizeof (mod_fname),
           "%s/mod_%s.so", ctx->modules_dir, name);
  mod_handle = dlopen(mod_fname, RTLD_NOW | RTLD_GLOBAL);
  if (mod_handle == NULL) {
    DebugLog(DF_CORE, DS_FATAL, "error: dlopen(%s): %s\n", mod_fname, dlerror());
    return (NULL);
  }

  snprintf(mod_fname, sizeof (mod_fname), "mod_%s", name);
  mod = dlsym(mod_handle, mod_fname);
  if (mod == NULL) {
    DebugLog(DF_CORE, DS_FATAL, "error: dlsym(%s): %s\n", mod_fname, dlerror());
    return (NULL);
  }

  DebugLog(DF_CORE, DS_NOTICE, "module [%s] successfully loaded: symbol mod_%s found at %x\n", name, name, mod);

  add_module(ctx, mod, mod_handle);

  return (mod);
}

int
add_module(orchids_t *ctx, input_module_t *mod, void *dlhandle)
{
  int mod_id;
  void *mod_cfg;

  if (ctx->loaded_modules >= MAX_MODULES) {
    fprintf(stderr, "module limit reached.\n");
    exit(EXIT_FAILURE);
  }

  /* 0 - check module info */
  if ((mod->magic != MOD_MAGIC) ||
      (mod->version != ORCHIDS_VERSION) ||
      (mod->name == NULL))
    {
      DPRINTF( ("Unable to load module...\n") );
      return (-1);
    }

  /* 1 - Check if already registered */
  if (find_module(ctx, mod->name) != NULL) {
    DPRINTF( ("warning! module [%s] already loaded...\n", mod->name) );
    return (-1);
  }

  /* 2 - Check dependencies */
  if (mod->dependencies) {
    char **d;

    for (d = mod->dependencies; *d; d++)
      if (find_module(ctx, *d) == NULL) {
        DPRINTF( ("failed module dependencie '%s' for module '%s'\n",
                  *d, mod->name) );
        exit(EXIT_FAILURE); /* XXX: Keep This ??? */
        return (-1);
      }
  }

  /* 3 - Add to core list (and hashlist ?) */
  mod_id = ctx->loaded_modules;
  ctx->mods[mod_id].mod_id = mod_id;
  ctx->mods[mod_id].mod = mod;
  ctx->mods[mod_id].dlhandle = dlhandle;
  /* dyn realloc mod array ? remove mod limit ? */

  /* 4 - Proceed to initialisation */
  if (mod->pre_config) {
    mod_cfg = mod->pre_config(ctx, &ctx->mods[mod_id]);
    ctx->mods[mod_id].config = mod_cfg;
  }
  ctx->loaded_modules++;

  return (mod_id);
}

dir_handler_t
dir_handler_lookup(orchids_t *ctx, input_module_t *mod, char *dir)
{
  mod_cfg_cmd_t *c;
  int i;

  c = mod->cfg_cmds;
  if (c == NULL) {
    DPRINTF( ("Module %s have no directive table.\n", mod->name) );
    return (NULL);
  }

  i = 0;
  while (c[i].name && strcmp(c[i].name, dir))
    i++;

  return (c[i].cmd);
}

int
remove_module(orchids_t *ctx, char *name)
{
  /* XXX * ToDo */
  return (0);
}

input_module_t *
find_module(orchids_t *ctx, const char *name)
{
  mod_entry_t *m;
  int nm;
  int i;

  for (m = ctx->mods, nm = ctx->loaded_modules, i = 0;
       i < nm;
       m++, i++) {
    if (strcmp(m->mod->name, name) == 0)
      return (m->mod);
  }

  return (NULL);
}

int
find_module_id(orchids_t *ctx, const char *name)
{
  mod_entry_t *m;
  int nm;
  int i;

  for (m = ctx->mods, nm = ctx->loaded_modules, i = 0;
       i < nm;
       m++, i++) {
    if (strcmp(m->mod->name, name) == 0)
      return (i);
  }

  return (-1);
}


mod_entry_t *
find_module_entry(orchids_t *ctx, const char *name)
{
  mod_entry_t *m;
  int nm;
  int i;

  for (m = ctx->mods, nm = ctx->loaded_modules, i = 0;
       i < nm;
       m++, i++) {
    if (strcmp(m->mod->name, name) == 0)
      return (m);
  }

  return (NULL);
}


int
call_mod_func(orchids_t *ctx,
              const char *modname,
              const char *funcname,
              void *funcparams)
{
  char full_func_name[1024];
  mod_entry_t *m;
  mod_func_t   f;
  int ret;

  m = find_module_entry(ctx, modname);
  if (m == NULL) {
    return (-1); /* module not loaded */
  }

  snprintf(full_func_name, sizeof (full_func_name),
           "mod_%s_%s",
           modname, funcname);
  f = dlsym(m->dlhandle, full_func_name);
  if (f == NULL) {
    DebugLog(DF_CORE, DS_FATAL, "error: dlsym(%s): %s\n",
             full_func_name, dlerror());
    return (-2); /* module loaded, but function not found */
  }

  ret = f(ctx, m, funcparams);

  return (ret);
}


void
fprintf_loaded_modules(orchids_t *ctx, FILE *fp)
{
  int m;

  fprintf(fp, "-------[ loaded modules ]-------\n");
  fprintf(fp, " id |  address  |   posts    |  name\n");
  fprintf(fp, "----+-----------+------------+-------------\n");
  for (m = 0; m < ctx->loaded_modules; m++)
    fprintf(fp, " %2d | %p | %10li | %.32s\n",
            m, (void *) ctx->mods[m].mod, ctx->mods[m].posts,
	    ctx->mods[m].mod->name);
  fprintf(fp, "----+-----------+------------+-------------\n");
}



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
