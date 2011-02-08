/**
 ** @file engine.h
 ** Public definitions for engine.c.
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

#ifndef ENGINE_H
#define ENGINE_H

#include "orchids.h"


/**
 ** Inject an event into the analysis engine.
 ** This is the entry point of the correlator.
 ** This function correspond to the judgments one-evt and evt-loop in the
 ** Jean's paper ("Un Algorithme pour l'Analyse de Logs").
 **
 ** @param ctx    Orchids application context.
 ** @param event  The event to inject.
 **/
void
inject_event(orchids_t *ctx, event_t *event);


/**
 ** Display all active rule instances on a stream.
 ** Displayed informations are :
 ** rule instance identifier (rid), 
 ** rule name, 
 ** number of actives states, 
 ** max depth of the path tree (dpt) 
 ** and the creation date.
 **
 ** @param fp Output stream.
 ** @param ctx Orchids context.
 **/
void
fprintf_rule_instances(FILE *fp, const orchids_t *ctx);


/**
 ** Display a waiting thread queue.
 ** Displayed information are:
 **   the thread identifier (thid),
 **   the rule name,
 **   the state name,
 **   the rule identifier (rid),
 **   the source state and the destination state identifiers (sid->did),
 **   the transition identifier,
 **   the transition counter value,
 **   the bump flag.
 **
 ** @param fp      The output stream.
 ** @param ctx     Orchids application context.
 ** @param thread  The waiting thread queue to display.
 **/
void
fprintf_thread_queue(FILE *fp, orchids_t *ctx, wait_thread_t *thread);


/**
 ** Display the list of active events.
 ** An event is active when it has passed a transition, at least.
 ** An active event record (active_event_s) may have passed more than
 ** one transition, so we count the number of references. When a rule instance
 ** is free'd, the reference counter is updated, and if it null, we can
 ** destroy the saved event too.
 **
 ** @param fp   The output stream.
 ** @param ctx  Orchids application context.
 **/
void
fprintf_active_events(FILE *fp, orchids_t *ctx);




#endif /* ENGINE_H */

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
