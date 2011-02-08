/**
 ** @file evt_mgr.h
 ** Public definitions for evt_mgr.c
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 1.0
 ** @ingroup core
 **
 ** @date  Started on: Wed Jan 22 16:32:26 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef EVT_MGR_H
#define EVT_MGR_H

#include "orchids.h"


/**
 ** Runtime main loop.
 **
 ** @param ctx Orchids context.
 **/
void
event_dispatcher_main_loop(orchids_t *ctx);


/**
 ** Register a real-time action.  This scheduled action will be
 ** multiplexed to the real-time event flow.
 **
 ** @param ctx Orchids context.
 ** @param e   Real-time action to register.
 **/
void
register_rtaction(orchids_t *ctx, rtaction_t *e);


/**
 ** Helper function to register a real-time callback rtaction_cb_t.
 ** This function just create a rtaction_t object from giver
 ** parameters, then call register_rtaction().
 ** @param ctx    A pointer to the Orchids application context.
 ** @param cb     A function pointer to the function callback.
 ** @param data   Abritrary data which will be passed to the callback at
 **               the execution time.
 ** @param delay  The delay (from now) when the callback will be executed.
 ** @return A pointer to the created and register real-time action rtaction_t.
 **/
rtaction_t *
register_rtcallback(orchids_t *ctx, rtaction_cb_t cb, void *data, time_t delay);


#endif /* EVT_MGR_H */

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
