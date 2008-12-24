/**
 ** @file engine_priv.h
 ** Private definitions for engine.c.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 1.0
 ** @ingroup engine
 **
 ** @date  Started on: Fri Feb 21 16:18:12 2003
 ** @date Last update: Fri Jul 27 15:27:54 2007
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef ENGINE_PRIV_H
#define ENGINE_PRIV_H

#include "orchids.h"

static void
free_rule_instance(orchids_t *ctx,
                   rule_instance_t *rule_instance);

static state_instance_t *
create_state_instance(orchids_t *ctx,
                      state_t *state,
                      const state_instance_t *parent);

static state_instance_t *
create_init_state_instance(orchids_t *ctx,
                           const rule_t *rule);

static void
mark_dead_rule(orchids_t *ctx,
               rule_instance_t *rule);

static void
rip_dead_rule(orchids_t *ctx, rule_instance_t *rule);

static int
sync_var_env_is_defined(orchids_t *ctx, state_instance_t *state);

static int
simulate_state_and_create_threads(orchids_t        *ctx,
                                  state_instance_t *state,
                                  active_event_t   *event,
                                  int               only_once);

static void
create_rule_initial_threads(orchids_t *ctx,
                            active_event_t *event /* XXX: NOT USED */);

static void
unlink_thread_in_state_instance_list(wait_thread_t *thread);

static int
backtrack_is_not_needed(orchids_t *ctx, wait_thread_t *thread);

#endif /* ENGINE_PRIV_H */

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
