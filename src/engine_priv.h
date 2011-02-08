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
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef ENGINE_PRIV_H
#define ENGINE_PRIV_H

#include "orchids.h"

/**
 * Free an entire rule instance (state instances and environments)
 * and update global statistics (active states and rules).
 *
 * @param ctx Orchids context.
 * @param rule_instance Rule instance to destroy.
 **/
static void
free_rule_instance(orchids_t *ctx,
                   rule_instance_t *rule_instance);


/**
 * Create an instance of a state and inherits environment from parent.
 * This function doesn't link new state instance and its parent.
 *
 * @param ctx    The Orchids context.
 * @param state  The state definition to instantiate.
 * @param parent The parent state instance, in the path tree.
 * @return The new state instance.
 **/
static state_instance_t *
create_state_instance(orchids_t *ctx,
                      state_t *state,
                      const state_instance_t *parent);


/**
 * Create an instance of the initial state of a rule.
 * 'init' state is a special case (environment are not inherited).
 *
 * @param ctx  Orchids context.
 * @param rule The rule definition to instantiate its 'init' state.
 * @return The new 'init' state instance.
 **/
static state_instance_t *
create_init_state_instance(orchids_t *ctx,
                           const rule_t *rule);

/**
 * Flag all thread of a rule instance as killed.  This function only
 * set a 'kill-flag' on threads.  It does not free anything.
 * @param ctx  A pointer to the Orchids application context.
 * @param rule A pointer to the rule instance to kill.
 **/
static void
mark_dead_rule(orchids_t *ctx,
               rule_instance_t *rule);


/**
 * Dead rule reaper: free all memory of a rule instance marked
 * as dead.  This function search for a rule, and update the linked list.
 * @param ctx  A pointer to the Orchids application context.
 * @param rule A pointer to the dead rule instance to reap.
 **/
static void
reap_dead_rule(orchids_t *ctx, rule_instance_t *rule);


/**
 * Return TRUE if the synchronization variable environment
 * is completely defined.
 * @param ctx  Orchids context.
 * @param state  The state instance to test.
 **/
static int
sync_var_env_is_defined(orchids_t *ctx, state_instance_t *state);


/**
 * Simulate a state, execute the byte code of actions, and create new treads.
 * (This correspond to the q-add judgment of Jean's algorithm).
 * @param ctx Orchids context.
 * @param state The state instance to simulate.
 * @param event A reference to the current event.
 * @param only_once THREAD_ONLYONCE flag (for rule initialisation).
 * @return Returns the number of created thread
 **/
static int
simulate_state_and_create_threads(orchids_t        *ctx,
                                  state_instance_t *state,
                                  active_event_t   *event,
                                  int               only_once);


/**
 * Create initial threads of each rules, put the COMMIT flags
 * and merge to the current wait queue.
 * This function correspond to the q-init judgement of Jean's algorithm).
 * @param ctx Orchids context.
 * @param event A reference to the current event.
 **/
static void
create_rule_initial_threads(orchids_t *ctx,
                            active_event_t *event /* XXX: NOT USED */);


/**
 * This function unlink a thread from the list of its state instance.
 * This naive implementation have to walk the list to find
 * its previous element.  A better solution should be to use
 * a double linked list (or a tail queue depending of the type
 * of access).
 * @param thread  The thread to unlink.
 **/
static void
unlink_thread_in_state_instance_list(wait_thread_t *thread);

/**
 * This function tells if this thread will need to be re-checked in
 * the future, with the present knowledge (this is a green cut).  In
 * case we are (for some reasons) sure that we have a shortest-run,
 * then this thread will not need to be rechecked in the future.  We
 * can add any (lightweight if possible) analysis.  This function
 * should be a trade-off between the time we spend to perform the
 * analysis, and the time we waste be searching in useless threads.
 *
 *   The current analysis is (somewhat) simple: if the destination
 * state is (1) fully-blocking (i.e. with no epsilon-transition), and
 * (2) the destination state action code does not bind a field value
 * to a variable, and (3) the destination state is not a cut point, then
 * the thread is the shortest run.
 *
 *   The condition (1) ensure that the execution can't continue in
 * other states (so we can only analyze the destination state.  An
 * alternative could be to recursively analyze states that are
 * reachable from the destination state of the current thread). By
 * ensuring that the execution will not go further, we are sure that
 * any field of the current event will not be bound to a variable.
 * 
 *   The condition (2) ensure that if no field is bound to a variable,
 * no future test involving a variable with a value derived from this
 * event and other event will be done.
 *
 *  The condition (3) ensure the if the destination is not a cut
 * point, this thread wont need to be tested in the future.  This is
 * because a cut is somewhat like a cancellation (or a commitment,
 * depending how we use it) of the search in the path tree.  When a
 * cut is done (killing all child-threads of a state instance), we
 * will need a new instance, with new events.
 *
 * @param ctx     The Orchids application context.
 * @param thread  The thread to be analyzed.
 * @return A boolean telling if a backtracking will be needed
 *           in the future or not.
 **/
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
