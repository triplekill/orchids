/**
 ** @file evt_mgr_priv.h
 ** Private definitions for evt_mgr.c
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

#ifndef EVT_MGR_PRIV_H
#define EVT_MGR_PRIV_H

#include "orchids.h"

/**
 ** @def Monitor_Activity
 **   A macro which wrap the monitor_activity() function if this
 **   feature is enabled.  Otherwise, it does nothing.
 **/

#ifdef ENABLE_ACTMON
/**
 ** An activity monitor for seeing Orchids moving.  This function is
 ** only for demonstration purposes.
 **/
static void monitor_activity(void);
# define Monitor_Activity() monitor_activity()
#else
# define Monitor_Activity()
#endif /* ACTMON */


/**
 ** Get the next real-time action heap_entry_t from the action queue.
 ** @param ctx  A pointer to the Orchids application context.
 ** @return The next real-time action in the action queue.
 **/
static heap_entry_t *get_next_rtaction(orchids_t *ctx);


#else
#warning "Private file should not be included multiple times."
#endif /* EVT_MGR_PRIV_H */

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