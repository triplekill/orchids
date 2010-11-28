/**
 ** @file orchids_api.h
 ** Public definitions for orchids_api.c.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 1.0
 ** @ingroup core
 **
 ** @date  Started on: Wed Jan 22 16:31:59 2003
 ** @date Last update: Fri Aug  3 12:28:50 2007
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef ORCHIDS_API_H
#define ORCHIDS_API_H

#include <sys/types.h>

#include "orchids.h"

int
lock_reg(int fd, int cmd, int type, off_t offset, int whence, off_t len);


pid_t
lock_test(int fd, int type, off_t offset, int whence, off_t len);


#define Read_lock(fd, offset, whence, len) \
          lock_reg(fd, F_SETLK, F_RDLCK, offset, whence, len)

#define Readw_lock(fd, offset, whence, len) \
          lock_reg(fd, F_SETLKW, F_RDLCK, offset, whence, len)

#define Write_lock(fd, offset, whence, len) \
          lock_reg(fd, F_SETLK, F_WRLCK, offset, whence, len)

#define Writew_lock(fd, offset, whence, len) \
          lock_reg(fd, F_SETLKW, F_WRLCK, offset, whence, len)

#define Un_lock(fd, offset, whence, len) \
          lock_reg(fd, F_SETLK, F_UNLCK, offset, whence, len)


void
orchids_lock(const char *lockfile);


/**
 ** Create a new Orchids application context, and initialize it with
 ** default values.
 **
 ** @return A new allocated application context structure.
 **/
orchids_t *
new_orchids_context(void);


/**
 ** Add a new polled input callback.
 **
 ** @param ctx Orchids application context.
 ** @param cb A Polled input callback.
 ** @param data Data to pass to the callback. (some configuration for exemple).
 **/
void
add_polled_input_callback(orchids_t *ctx,
                          mod_entry_t *mod,
                          poll_callback_t cb,
                          void *data);


/**
 ** Add a new realtime input descriptor.
 ** New data on the descriptor will be monitored with select() syscall.
 **
 ** @param ctx  Orchids application context.
 ** @param cb   A realtime input callback.
 ** @param fd   The file descriptor associated to the realtime input.
 ** @param data Data to pass realtime input callback. (module config fer ex.).
 **/
void
add_input_descriptor(orchids_t *ctx,
                     mod_entry_t *mod,
                     realtime_callback_t cb,
                     int fd,
                     void *data);


/** 
 ** Remove a realtime input descriptor.
 **
 ** @param ctx Orchids application context.
 ** @param fd  Descriptor to remove.
 **/
void
del_input_descriptor(orchids_t *ctx, int fd);


/**
 ** Register an unconditional disector.
 **
 ** @param ctx Orchids application context.
 ** @param parent_modname The module to hook.
 ** @param dissect The dissector to register.
 ** @param data  Data passed to the dissector.
 **/
void
register_dissector(orchids_t *ctx,
                   mod_entry_t *mod,
                   char *parent_modname,
                   dissect_t dissect,
                   void *data);


/**
 ** Register a conditionnal dissector.
 **
 ** @param ctx             Orchids application context.
 ** @param parent_modname  The module to hook.
 ** @param key             The key value to trig the dissector.
 ** @param keylen          The legth of the key.
 ** @param dissect         The sub-dissector to register.
 ** @param data            Data passed to the dissector.
 **/
void
register_conditional_dissector(orchids_t *ctx,
                               mod_entry_t *mod,
                               char *parent_modname,
                               void *key,
                               size_t keylen,
                               dissect_t dissect,
                               void *data);


/**
 ** Register a new callback in the post-inject-event hook.
 **/
void
register_post_inject_hook(orchids_t *ctx,
                          mod_entry_t *mod,
                          hook_cb_t cb,
                          void *data);


/**
 ** Execute all registered callbacks in the post-inject-event hook.
 **/
void
execute_post_inject_hooks(orchids_t *ctx, event_t *event);


/**
 ** Register module supported fields into the analysis engine.
 ** This function should be called during the module
 ** pre-configuration callback.
 **
 ** @param ctx        Orchids application context.
 ** @param mod        The configured module entry.
 ** @param field_tab  Field array.
 ** @param sz         Field size.
 **/
void
register_fields(orchids_t *ctx,
                mod_entry_t *mod,
                field_t *field_tab,
                size_t sz);


/**
 ** Free fields.
 **
 ** @param tbl_event Event property table.
 ** @param s         Event proterty table size.
 **/
void
free_fields(ovm_var_t **tbl_event, size_t s);


/**
 ** Append an event with a given property table filled by a dissection module.
 **
 ** @param ctx        Orchids application context.
 ** @param mod        The module entry which want this event construction.
 **   This is used by the rule compiler which proceed to a statical analysis 
 **   of rules, and mark as `disabled' field that are unused in rules.
 **   This can speed-up dissection, when a module can skip a sub-dissection.
 ** @param event      The destination event to append.
 ** @param tbl_event  The property table.
 ** @param sz         The property table size.
 **/
void
add_fields_to_event(orchids_t *ctx,
                    mod_entry_t *mod,
                    event_t **event,
                    ovm_var_t **tbl_event,
                    size_t sz);


/**
 ** Print an event on a strio stream.
 **
 ** @param fp     The stdio stream to print on.
 ** @param ctx    Orchids application context.
 ** @param event  The event to print.
 **/
void
fprintf_event(FILE *fp, const orchids_t *ctx, const event_t *event);


/**
 ** Event destructor.
 ** Free all allocated resources of an event.
 **
 ** @param event The event to destroy.
 **/
void
free_event(event_t *event);


/**
 ** Post an event.
 ** If a module have registered a sub-disector, the function will call it.
 ** If no more module can apply, the event will be injectect into the
 ** analysis engine with inject_event().
 **
 ** @param ctx            Orchids application context.
 ** @param sender         The entry of the sender module.
 ** @param event          The event to post.
 **/
void
post_event(orchids_t *ctx, mod_entry_t *sender, event_t *event);


/**
 ** Print the Orchids application statistics to a stdio stream.
 ** Displayed info are: real uptime, user cpu time, system cpu time
 ** total cpu time, average load, pre-configuration time, rule compilation time
 ** post-configuration time, current used config file, the list of loaded
 ** modules, the current poll period, the number of registered fields,
 ** the number of injected events, the number of active events, the current
 ** number of rule instances and the current total number of state instances.
 ** Additionnaly, on a linux operating system, this function displays some
 ** specifics informations (Memory usage, attached CPU, etc...).
 **
 ** @param fp  The stdio stream to print on.
 ** @param ctx The Orchids applecation context to print.
 **/
void
fprintf_orchids_stats(FILE *fp, const orchids_t *ctx);


/**
 ** Display an hexadecimal dump on a stream.
 **
 ** @param fp    A stream.
 ** @param data  Data to display.
 ** @param n     Data size.
 **/
void
fprintf_hexdump(FILE *fp, const void *data, size_t n);


/**
 ** Display all registered fields on a stream.
 **
 ** @param fp   The output stream.
 ** @param ctx  Orchids application context.
 **/
void
fprintf_fields(FILE *fp, const orchids_t *ctx);


/**
 ** Display a state environment on a stream.
 ** This function display the resulting environment given the current
 ** (state_instance_s::current_env) and the
 ** inherited (state_instance_s::inherit_env) ones.
 **
 ** @param fp    The output stream.
 ** @param state The state instance which contains the environment to print.
 **/
void
fprintf_state_env(FILE *fp, const state_instance_t *state);


#endif /* ORCHIDS_API_H */

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