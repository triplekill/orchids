/**
 ** @file mod_mgr.h
 ** Public definitions for mod_mgr.c.
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

#ifndef MOD_MGR_H
#define MOD_MGR_H

#include "orchids.h"

/**
 ** Add an input module.
 **
 ** @param ctx  Orchids application context.
 ** @param mod  The information structure of the module to register.
 ** @param dlhandle  The handle returned by dlopen() or NULL if not dynamic.
 **
 ** @return mod_id if module was successfully added, or -1 if an error occurred.
 **/
int
add_module(orchids_t *ctx, input_module_t *mod, void *dlhandle);


/**
 ** Load and register a Dynamic Shared Object (DSO) module.
 **
 ** @param ctx   Orchids application context.
 ** @param name  The module name to load.
 **
 ** @return A pointer to the information structure of the module, or
 **   NULL if an error occurs.
 **/
input_module_t *
load_add_shared_module(orchids_t *ctx, const char *name);


/**
 ** Load, register and add a Dynamic Shared Object (DSO) module.
 **
 ** @param ctx   Orchids application context.
 ** @param name  The module name to load.
 **
 ** @return A pointer to the information structure of the module, or
 **   NULL if an error occurs.
 **/
/* input_module_t * */
/* load_add_module(orchids_t *ctx, const char *name); */


/**
 ** Remove a module.
 **
 ** @param ctx  Orchids application context.
 ** @param name The module name to remove.
 **
 ** @return 0 if module was successfully removed, or -1 if an error occurred.
 **/
int
remove_module(orchids_t *ctx, char *name);


/**
 ** Find a module given its name.
 **
 ** @param ctx   Orchids context.
 ** @param name  Module name to find.
 **
 ** @return      A pointer to a input_module_s structure, or NULL if the module
 **              was not found.
 **/
input_module_t *
find_module(orchids_t *ctx, const char *name);


/**
 ** Find a module id given its name.
 **
 ** @param ctx   Orchids context.
 ** @param name  Module name to find.
 **
 ** @return      The identifier number.
 **/
int
find_module_id(orchids_t *ctx, const char *name);


/**
 ** Find a module entry given its name.
 **
 ** @param ctx   Orchids context.
 ** @param name  Module name to find.
 **
 ** @return      The module entry.
 **/
mod_entry_t *
find_module_entry(orchids_t *ctx, const char *name);


/**
 ** Try to call a module function, if the module is loaded and if the
 ** function is found.
 **
 ** @param ctx         Orchids context.
 ** @param modname     Module name to find.
 ** @param funcname    Function name to find.
 ** @param funcparams  Arbitrary parameters passed to the called function.
 **
 ** @return      An error code.
 **/
int
call_mod_func(orchids_t *ctx,
              const char *modname,
              const char *funcname,
              void *funcparams);


/**
 ** Display loaded modules.
 ** Available informations are :
 ** the module identifier (id),
 ** memory address,
 ** number of posted events (successfully dissected message)
 ** and the module name.
 **
 ** @param ctx  A pointer to the Orchids application context.
 ** @param fp   Output stream.
 **/
void
fprintf_loaded_modules(orchids_t *ctx, FILE *fp);


/**
 ** Look for a directive handler, given its name.
 ** 
 ** @param ctx  Orchids application context.
 ** @param mod  Input module information structure.
 ** @param dir  The directive name to resolve.
 **
 ** @return A function pointer of the resolved directive handler.
 **/
dir_handler_t
dir_handler_lookup(orchids_t *ctx, input_module_t *mod, char *dir);


#endif /*MOD_MGR_H */

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
