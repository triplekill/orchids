/**
 ** @file evt_mgr.c
 ** Runtime main loop: dispatch events and real-time actions.
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#include "orchids.h"

#include "evt_mgr.h"
#include "evt_mgr_priv.h"

#ifdef DMALLOC
unsigned long dmalloc_orchids;
#endif


rtaction_t *
register_rtcallback(orchids_t *ctx, rtaction_cb_t cb, void *data, time_t delay)
{
  rtaction_t *event;

  event = Xzmalloc(sizeof (rtaction_t));
  event->cb = cb;
  event->data = data;
  event->date = ctx->cur_loop_time;
  event->date.tv_sec += delay;

  register_rtaction(ctx, event);

  return (event);
}


void
register_rtaction(orchids_t *ctx, rtaction_t *e)
{
  rtaction_t *event;

  if ( DLIST_IS_EMPTY(&(ctx->rtactionlist)) ) {
    DLIST_INSERT_HEAD(&(ctx->rtactionlist), e, evtlist);
    return ;
  }

  DLIST_FOREACH(event, &ctx->rtactionlist, evtlist) {
    if (timercmp(&e->date, &event->date, < )
        || DLIST_NEXT(event, evtlist) == NULL) {
      break ;
    }
  }

  if (   (DLIST_NEXT(event, evtlist) == NULL) 
      && (timercmp(&e->date, &event->date, > ))) {
    DLIST_INSERT_AFTER(event, e, evtlist);
  }
  else {
    DLIST_INSERT_BEFORE(event, e, evtlist);
  }
}


static rtaction_t *
get_next_rtaction(orchids_t *ctx)
{
  rtaction_t *e;

  if (DLIST_IS_EMPTY(&ctx->rtactionlist)) {
    fprintf(stderr, "error: event queue is empty !\n");
    return (NULL);
  }

  e = DLIST_FIRST(&ctx->rtactionlist);
  DLIST_REMOVE(e, evtlist);

  return (e);
}

#if 0
static int
rt_event_poll(orchids_t *ctx, rtaction_t *e)
{
  DebugLog(DF_CORE, DS_TRACE,
           "Entering real time event... polling data...\n");

  e->date.tv_sec += ctx->poll_period.tv_sec;

  register_rtaction(ctx, e);

  return (0);
}
#endif


void
event_dispatcher_main_loop(orchids_t *ctx)
{
  fd_set rfds;
  time_t curr_time;
  struct timeval cur_time;
  struct timeval wait_time;
  struct timeval *wait_time_ptr;
  int retval;
  realtime_input_t *rti;
  rtaction_t *e;
  realtime_input_t *next;

  if ((ctx->poll_handler_list == NULL) && (ctx->realtime_handler_list == NULL))
    {
      DebugLog(DF_CORE, DS_FATAL,
               "Nothing to do... No modules have requested input...\n");
      exit(EXIT_FAILURE);
    }

  curr_time = time(NULL);
  ctx->last_poll = curr_time;
  wait_time_ptr = &wait_time;

  DebugLog(DF_CORE, DS_NOTICE,
           "*** Setup done. Entering in event dispatcher main loop ***\n");

#ifdef ENABLE_ACTMON
  printf("[pid %i] Running...\n ", getpid());
#endif

#ifdef DMALLOC
  dmalloc_orchids = dmalloc_mark();
#endif

  e = get_next_rtaction(ctx);

  for (;;) {
    gettimeofday(&cur_time, NULL);
    ctx->cur_loop_time = cur_time;

    /* Consume past event, if any */
    while ( e && timercmp( &e->date, &cur_time, <= )) {
      if (e->cb)
        e->cb(ctx, e);
      e = get_next_rtaction(ctx);
    }

    if (e) {
      Timer_Sub( &wait_time, &e->date, &cur_time );
      wait_time_ptr = &wait_time;
    }
    else {
      wait_time_ptr = NULL;
    }

    Monitor_Activity();
    memcpy(&rfds, &ctx->fds, sizeof(fd_set));

    DebugLog(DF_CORE, DS_DEBUG,
             "*** waiting for real-time event (or timeout) *** wait=%li.%06li\n",
             wait_time_ptr ? wait_time_ptr->tv_sec : 0,
             wait_time_ptr ? wait_time_ptr->tv_usec : 0);

    retval = Xselect(ctx->maxfd + 1, &rfds, NULL, NULL, wait_time_ptr);

    if (retval) {
      DebugLog(DF_CORE, DS_INFO, "New real-time input data.... (%i)\n", retval);
      for (rti = ctx->realtime_handler_list; rti && retval; ) {
        next = rti->next;
        if (FD_ISSET(rti->fd, &rfds)) {
          retval--;

	  int n = (rti->cb)(ctx, &ctx->mods[rti->mod_id], rti->fd, rti->data);
	  if (n>0) // then callback asked to be removed from select()ed fds
	    { // this is typical of disconnections.  Reconnections should
	      // be rescheduled using register_rtaction()
	      FD_CLR(rti->fd, &ctx->fds);
	    }
        }
        rti = next;
      }
    }
    else {
      DebugLog(DF_CORE, DS_DEBUG, "Timeout... Calling real-time callback...\n");
      gettimeofday(&cur_time, NULL);
      ctx->cur_loop_time = cur_time; // added, JGL Jun 03, 2012

      /* Force the callback execution here.
       * We assume the timeout was correct.
       * This corrects small imprecisions we may have here. */
      do {
        if (e->cb)
          e->cb(ctx, e);
        e = get_next_rtaction(ctx);
      } while ( e && timercmp( &e->date, &cur_time, <= ));
      /* Here, the do {} while construct is for handling the special case
       * when action execution is longer than the delay
       * to the next action. */
    }
  }
}


#ifdef ENABLE_ACTMON

static void
monitor_activity(void)
{
  static int position = 0;

  switch (position) {

  case 0:
    printf("\r - ");
    break ;
    
  case 1:
    printf("\r \\ ");
    break ;

  case 2:
    printf("\r | ");
    break ;

  case 3:
    printf("\r / ");
    break ;

  }

  fflush(stdout);

  position = (position + 1) % 4;
}

#endif /* ENABLE_ACTMON */


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
