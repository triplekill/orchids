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


static void heap_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  heap_t *h = (heap_t *)p;

  if (h->entry!=NULL)
    GC_TOUCH (gc_ctx, h->entry->gc_data);
  GC_TOUCH (gc_ctx, h->left);
  GC_TOUCH (gc_ctx, h->right);
}

static void heap_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  heap_t *h = (heap_t *)p;

  if (h->entry!=NULL)
    gc_base_free (h->entry);
}

static int heap_traverse (gc_traverse_ctx_t *gtc, gc_header_t *p,
			  void *data)
{
  heap_t *h = (heap_t *)p;
  int err = 0;

  if (h->entry!=NULL)
    {
      err = (*gtc->do_subfield) (gtc, (gc_header_t *)h->entry->gc_data, data);
      if (err)
	return err;
    }
  err = (*gtc->do_subfield) (gtc, (gc_header_t *)h->left, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *)h->right, data);
  return err;
}

static gc_class_t heap_class = {
  GC_ID('h','e','a','p'),
  heap_mark_subfields,
  heap_finalize,
  heap_traverse
};

/* Priority queues, implemented as skew heaps */


#ifdef DEBUG_RTACTION
void rtaction_print (heap_t *rt, int lmargin);
#endif

#define he_before(he1,he2) (timercmp(&(he1)->date, &(he2)->date, < ) || \
			    (timercmp(&(he1)->date, &(he2)->date, ==) && (he1)->pri > (he2)->pri))

void register_rtaction (orchids_t *ctx, heap_entry_t *he)
{
  gc_t *gc_ctx = ctx->gc_ctx;
  heap_t **rtp;
  heap_t *rt;
  heap_t *left;
  struct heap_entry_s *he2;

#ifdef DEBUG_RTACTION
  fprintf (stderr, "* register_rtaction: he->data=0x%p\n", he->data);
  fflush (stderr);
#endif
  GC_START(gc_ctx, 1);
  GC_UPDATE(gc_ctx, 0, he->gc_data);
  rtp = &ctx->rtactionlist;
  while ((rt = *rtp) != NULL)
    {
      /* Swap left and right.
       In principle we should GC_TOUCH() after each of the following
      two assignments, but this is not needed.  The only purpose of
      GC_TOUCH() is to make sure no black object points to a white
      object.  But if rt is black then neither rt->left nor rt->right
      will be white before the swap, hence also, trivially, after
      the swap. */
      left = rt->left;
      rt->left = rt->right;
      rt->right = left;

      if (he_before(he, rt->entry))
	{ /* Store he into root, then insert the old
	     value of rt->entry into rt->left subheap */
	  he2 = rt->entry;
	  rt->entry = he;
	  GC_TOUCH (gc_ctx, he->gc_data);
	  he = he2;
	  GC_UPDATE(gc_ctx, 0, he->gc_data); // we don't want it to be freed
	  rtp = &rt->left;
	}
      else
	{ /* Else insert he into rt->left subheap */
	  rtp = &rt->left;
	}
    }
  rt = gc_alloc (gc_ctx, sizeof(heap_t), &heap_class);
  rt->gc.type = T_HEAP;
  rt->entry = he; // no need to GC_TOUCH he->gc_data since rt is white and will only become grey below, not black
  rt->left = NULL;
  rt->right = NULL;
  GC_TOUCH (gc_ctx, *rtp = rt);
  GC_END(gc_ctx);
#ifdef DEBUG_RTACTION
  fprintf (stderr, "* at end of register_rtaction:\n");
  rtaction_print (ctx->rtactionlist, 0);
  fflush (stderr);
#endif
}


heap_entry_t *register_rtcallback(orchids_t *ctx,
				  rtaction_cb_t cb,
				  gc_header_t *gc_data,
				  void *data,
				  time_t delay,
				  int pri)
{
  heap_entry_t *he;

  he = gc_base_malloc (ctx->gc_ctx, sizeof (heap_entry_t));
  he->cb = cb;
  he->gc_data = gc_data;
  he->data = data;
  he->date = ctx->cur_loop_time;
  he->date.tv_sec += delay;
  he->pri = pri;
  register_rtaction(ctx, he);
  return he;
}

static void heap_merge (gc_t *gc_ctx, heap_t *h1, heap_t *h2,
			heap_t **res)
{
  heap_entry_t *he1, *he2;
  heap_t *left;

  while (1)
    {
      if (h2==NULL)
	{
	  GC_TOUCH (gc_ctx, *res = h1);
	  return;
	}
      if (h1==NULL)
	{
	  GC_TOUCH (gc_ctx, *res = h2);
	  return;
	}
      he1 = h1->entry;
      he2 = h2->entry;
      if (he_before (he1, he2))
	{
	  /* We shall return h1, which has the least date. */
	  GC_TOUCH (gc_ctx, *res = h1);
	  /* First exchange the left and right parts of h1.
	   No need to GC_TOUCH() here, see 'Swap left and right' comment
	   in register_rtaction(). */
	  left = h1->right;
	  h1->right = h1->left;
	  h1->left = left;
	  /* Next, merge the old right part (left now) with h2 */
	  res = &h1->left;
	  h1 = left;
	  /* h2 unchanged; then loop */
	}
      else
	{ /* In this other case, we shall instead return h2. */
	  GC_TOUCH (gc_ctx, *res = h2);
	  /* First (again) exchange its left and right parts.
	   No need to GC_TOUCH() here, see 'Swap left and right' comment
	   in register_rtaction(). */
	  left = h2->right;
	  h2->right = h2->left;
	  h2->left = left;
	  /* Next merge the old right part (left now) with h1...
	     swapping the roles of h1 and h2 in the process. */
	  res = &h2->left;
	  h2 = h1;
	  h1 = left;
	}
    }
}

#if 0
  /* This is the recursive version: */
static heap_t *heap_merge (gc_t *gc_ctx, heap_t *h1, heap_t *h2)
{
  heap_entry_t *he1, *he2;
  heap_t *left;

  if (h2==NULL)
    return h1;
  if (h1==NULL)
    return h2;
  he1 = h1->entry;
  he2 = h2->entry;
  if (he_before (he1, he2))
    {
      /* We shall return he1, which has the least date.
	 First exchange its left and right parts.
      */
      left = h1->right;
      GC_TOUCH (gc_ctx, h1->right = h1->left);
      GC_TOUCH (gc_ctx, h1->left = left);
      /* Next, merge the old right part (left now) with h2 */
      GC_TOUCH (gc_ctx, h1->left = heap_merge (gc_ctx, left, h2));
      return h1;
    }
  else
    { /* In this other case, we shall instead return he2.
	 First (again) exchange its left and right parts.
      */
      left = h2->right;
      GC_TOUCH (gc_ctx, h2->right = h2->left);
      GC_TOUCH (gc_ctx, h2->left = left);
      /* Next merge the old right part (left now) with h1 */
      GC_TOUCH (gc_ctx, h2->left = heap_merge (gc_ctx, left, h1));
      return h2;
    }
}
#endif


static heap_entry_t *get_next_rtaction(orchids_t *ctx)
{
  gc_t *gc_ctx = ctx->gc_ctx;
  heap_t *h;
  heap_entry_t *he;

  h = ctx->rtactionlist;
  if (h==NULL)
    {
      DebugLog(DF_EVT, DS_DEBUG, "Empty rtactionlist\n");
      return NULL;
    }
  heap_merge (gc_ctx, h->left, h->right, &ctx->rtactionlist);
  he = h->entry;
  h->entry = NULL;
  return he;
}


void event_dispatcher_main_loop(orchids_t *ctx)
{
  fd_set rfds;
  time_t curr_time;
  struct timeval cur_time;
  struct timeval wait_time;
  struct timeval *wait_time_ptr;
  int retval;
  realtime_input_t *rti;
  heap_entry_t *he;
  realtime_input_t *next;

  if (ctx->realtime_handler_list == NULL)
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

  if (ctx->actmon)
    {
      printf("[pid %i] Running...\n ", getpid());
      fflush (stdout);
    }

#ifdef DMALLOC
  dmalloc_orchids = dmalloc_mark();
#endif

  he = get_next_rtaction(ctx);
#ifdef DEBUG_RTACTION
  fprintf (stderr, "* after get_next_rtaction (1): he->data=0x%p\n", he->data);
  rtaction_print (ctx->rtactionlist, 0);
  fflush (stderr);
#endif

  for (;;) {
    gc (ctx->gc_ctx);
    gettimeofday(&cur_time, NULL);
    ctx->cur_loop_time = cur_time;

    /* Consume past event, if any */
    while (he!=NULL && timercmp (&he->date, &cur_time, <=))
      {
	if (he->cb!=NULL)
	  {
	    GC_START(ctx->gc_ctx, 1);
	    GC_UPDATE(ctx->gc_ctx, 0, he->gc_data);
	    (*he->cb) (ctx, he);
	    GC_END(ctx->gc_ctx);
	  }
	else gc_base_free (he);
#ifdef DEBUG_RTACTION
	fprintf (stderr, "* before get_next_rtaction (2):\n");
	rtaction_print (ctx->rtactionlist, 0);
	fflush (stderr);
#endif
	he = get_next_rtaction(ctx);
#ifdef DEBUG_RTACTION
	fprintf (stderr, "* after get_next_rtaction (2): he->data=0x%p\n", he->data);
	rtaction_print (ctx->rtactionlist, 0);
	fflush (stderr);
#endif
      }

    if (he!=NULL)
      {
	Timer_Sub (&wait_time, &he->date, &cur_time);
	wait_time_ptr = &wait_time;
      }
    else
      {
	wait_time_ptr = NULL;
      }

    if (ctx->actmon && ctx->verbose<2)
      monitor_activity();
    memcpy(&rfds, &ctx->fds, sizeof(fd_set));

    DebugLog(DF_CORE, DS_DEBUG,
             "*** waiting for real-time event (or timeout) *** wait=%li.%06li\n",
             wait_time_ptr ? wait_time_ptr->tv_sec : 0,
             wait_time_ptr ? wait_time_ptr->tv_usec : 0);

    retval = Xselect(ctx->maxfd + 1, &rfds, NULL, NULL, wait_time_ptr);

    if (retval!=0)
      {
	DebugLog(DF_CORE, DS_INFO, "New real-time input data.... (%i)\n",
		 retval);
	for (rti = ctx->realtime_handler_list; rti!=NULL && retval!=0; )
	  {
	    next = rti->next;
	    if (FD_ISSET(rti->fd, &rfds))
	      {
		retval--;

		int n = (*rti->cb) (ctx, &ctx->mods[rti->mod_id],
				    rti->fd, rti->data);
		if (n>0) // then callback asked to be removed from select()ed fds
		  { // this is typical of disconnections.  Reconnections should
		    // be rescheduled using register_rtaction()
		    FD_CLR(rti->fd, &ctx->fds);
		  }
	      }
	    rti = next;
	  }
      }
    else
      {
	DebugLog(DF_CORE, DS_DEBUG, "Timeout... Calling real-time callback...\n");
	gettimeofday(&cur_time, NULL);
	ctx->cur_loop_time = cur_time; // added, JGL Jun 03, 2012

	/* Force the callback execution here.
	 * We assume the timeout was correct.
	 * This corrects small imprecisions we may have here. */
	do {
	  if (he->cb!=NULL)
	    {
	      GC_START(ctx->gc_ctx, 1);
	      GC_UPDATE(ctx->gc_ctx, 0, he->gc_data);
	      (*he->cb) (ctx, he);
	      GC_END(ctx->gc_ctx);
	    }
	  else gc_base_free (he);
	  he = get_next_rtaction(ctx);
#ifdef DEBUG_RTACTION
	  fprintf (stderr, "* after get_next_rtaction (3): he->data=0x%p\n", he->data);
	  rtaction_print (ctx->rtactionlist, 0);
	  fflush (stderr);
#endif
	} while (he!=NULL && timercmp (&he->date, &cur_time, <=));
	/* Here, the do {} while construct is for handling the special case
	 * when action execution is longer than the delay
	 * to the next action. */
      }
  }
}


static void monitor_activity(void)
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
