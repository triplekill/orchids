/**
 ** @file engine.c
 ** Analysis engine.
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

/**
 ** @defgroup engine Orchids Analysis Engine
 **/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#include "orchids.h"
#include "lang.h"
#include "orchids_api.h"

#include "engine.h"
#include "engine_priv.h"

/* WARNING -- Field list in event_t, and field IDs in int array must
   be sorted in decreasing order */


/* XXX: need to sort threads by rule instance, for fast marking */
static void mark_dead_rule(orchids_t *ctx, rule_instance_t *rule)
{
  wait_thread_t *t;

  for (t = ctx->cur_retrig_qh; t; t = t->next) {
    if (t->state_instance->rule_instance == rule) {
      DebugLog(DF_ENG, DS_TRACE, "Marking thread %p as KILLED (cur)\n", t);
      KILL_THREAD(ctx, t);
    }
  }

  for (t = ctx->retrig_qh; t; t = t->next) {
    if ((t->state_instance->rule_instance == rule) && !THREAD_IS_KILLED(t)) {
      DebugLog(DF_ENG, DS_TRACE, "Marking thread %p as KILLED (retrig)\n", t);
      KILL_THREAD(ctx, t);
    }
  }

  for (t = ctx->current_tail; t; t = t->next) {
    if (t->state_instance->rule_instance == rule) {
      DebugLog(DF_ENG, DS_TRACE,
               "Marking thread %p as KILLED (current_tail)\n", t);
      KILL_THREAD(ctx, t);
    }
  }

  for (t = ctx->new_qh; t; t = t->next) {
    if ((t->state_instance->rule_instance == rule) && !THREAD_IS_KILLED(t)) {
      DebugLog(DF_ENG, DS_TRACE, "Forgotten thread ? (in new queue)\n");
    }
  }

  rule->flags |= THREAD_KILLED;
}



static void reap_dead_rule(orchids_t *ctx, rule_instance_t *rule)
{
  rule_instance_t *r;
  rule_instance_t *next_rule;
  rule_instance_t *prev_rule;

  prev_rule = NULL;
  for (r = ctx->first_rule_instance; r; r = next_rule) {
    next_rule = r->next;
    if (r == rule) {
      DebugLog(DF_ENG, DS_TRACE, "reaping killed rule %p\n", r);
      if (prev_rule!=NULL)
        GC_TOUCH (ctx->gc_ctx, prev_rule->next = next_rule);
      else
        GC_TOUCH (ctx->gc_ctx, ctx->first_rule_instance = next_rule);
      cleanup_rule_instance(ctx, r);
      return ;
    } else {
      prev_rule = r;
    }
  }
}


static int sync_var_env_is_defined(orchids_t *ctx, state_instance_t *state)
{
  int i;
  int sync_var_sz;
  int sync_var;

  sync_var_sz = state->rule_instance->rule->sync_vars_sz;

  for (i = 0; i < sync_var_sz; i++)
    {
      sync_var = state->rule_instance->rule->sync_vars[i];
      if (ovm_read_value (state->env, sync_var)==NULL)
	return 0;
    }
  return 1;
}


static void sync_lock_list_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  sync_lock_list_t *sll = (sync_lock_list_t *)p;

  GC_TOUCH (gc_ctx, sll->next);
  GC_TOUCH (gc_ctx, sll->state);
}

static void sync_lock_list_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  return;
}

static int sync_lock_list_traverse (gc_traverse_ctx_t *gtc, gc_header_t *p,
				    void *data)
{
  sync_lock_list_t *sll = (sync_lock_list_t *)p;
  int err = 0;

  err = (*gtc->do_subfield) (gtc, (gc_header_t *)sll->next, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *)sll->state, data);
  return err;
}

static gc_class_t sync_lock_list_class = {
  GC_ID('l','o','k','l'),
  sync_lock_list_mark_subfields,
  sync_lock_list_finalize,
  sync_lock_list_traverse
};


static void wait_thread_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  wait_thread_t *wt = (wait_thread_t *)p;

  GC_TOUCH (gc_ctx, wt->next);
  GC_TOUCH (gc_ctx, wt->state_instance);
  GC_TOUCH (gc_ctx, wt->next_in_state_instance);
}

static void wait_thread_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  return;
}

static int wait_thread_traverse (gc_traverse_ctx_t *gtc, gc_header_t *p,
				 void *data)
{
  wait_thread_t *wt = (wait_thread_t *)p;
  int err = 0;

  err = (*gtc->do_subfield) (gtc, (gc_header_t *)wt->next, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *)wt->state_instance, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *)wt->next_in_state_instance, data);
  return err;
}

static gc_class_t wait_thread_class = {
  GC_ID('t','h','r','d'),
  wait_thread_mark_subfields,
  wait_thread_finalize,
  wait_thread_traverse
};


static int simulate_state_and_create_threads(orchids_t        *ctx,
					     state_instance_t *state,
					     active_event_t   *event, /* XXX: NOT USED */
					     int               only_once)
{
  wait_thread_t *thread;
  state_instance_t *new_state;
  int t;
  int trans_nb;
  int vmret;
  int created_threads = 0;
  int simul_ret;
  state_instance_t *sync_lock;
  sync_lock_list_t *lock_elmt;
  gc_t *gc_ctx = ctx->gc_ctx;

  if (state->state->action!=NULL)
    ovm_exec_stmt(ctx, state, state->state->action);

  trans_nb = state->state->trans_nb;
  if (trans_nb == 0)
    {
      DebugLog(DF_ENG, DS_INFO,
             "Terminal state reached for rule instance (%p)\n",
	       state->rule_instance);
      mark_dead_rule(ctx, state->rule_instance);
      return -1;
    }

  /* test sync vars here, if sync, return */
  if (state->rule_instance->rule->sync_lock!=NULL)
    {
      DebugLog(DF_ENG, DS_INFO, "Checking synchronization variables\n");
      if ( sync_var_env_is_defined(ctx, state) )
	{
	  DebugLog(DF_ENG, DS_INFO, "Sync env is fully defined.\n");
	  sync_lock = objhash_get(state->rule_instance->rule->sync_lock, state);
	  if (sync_lock!=NULL)
	    {
	      DebugLog(DF_ENG, DS_INFO, "LOCK FOUND !\n");
	      if (sync_lock->rule_instance != state->rule_instance) {
		DebugLog(DF_ENG, DS_INFO, "Pruning.\n");
		return -1;
	      }
	      DebugLog(DF_ENG, DS_INFO, "In lock owner.\n");
	    }
	  else
	    {
	      /* create lock */
	      if ( !only_once ) {
		DebugLog(DF_ENG, DS_INFO, "No lock found.  Creating...\n");
		objhash_add(gc_ctx, state->rule_instance->rule->sync_lock,
			    state, state);
		lock_elmt = gc_alloc (gc_ctx, sizeof (sync_lock_list_t),
				      &sync_lock_list_class);
		lock_elmt->gc.type = T_NULL;
		GC_TOUCH (gc_ctx, lock_elmt->state = state);
		GC_TOUCH (gc_ctx, lock_elmt->next =
			  state->rule_instance->sync_lock_list);
		GC_TOUCH (gc_ctx, state->rule_instance->sync_lock_list =
			  lock_elmt);
        }
        else {
          DebugLog(DF_ENG, DS_INFO,
                   "No lock found.  Don't create in ONLY_ONCE\n");
        }
      }
    }
    else {
      DebugLog(DF_ENG, DS_INFO, "Sync env is NOT fully defined.\n");
    }
  }

  for (t = 0; t < trans_nb; t++) {
    /* if we have an e-transition, pass it */
    if (state->state->trans[t].required_fields_nb == 0) {
      vmret = 0;
      if (state->state->trans[t].eval_code!=NULL)
        vmret = ovm_exec_expr(ctx, state, state->state->trans[t].eval_code);
      if (vmret == 0) {
        DPRINTF( ("e-trans passed (to %s)\n",
                  state->state->trans[t].dest->name) );
        new_state = create_state_instance(ctx,
                                          state->state->trans[t].dest,
                                          state);

        /* link new_state instance in the tree */
        GC_TOUCH (gc_ctx, new_state->next_sibling = state->first_child);
        GC_TOUCH (gc_ctx, state->first_child = new_state);
        GC_TOUCH (gc_ctx, new_state->parent = state);

        /* update state instance list of the current rule instance */
        GC_TOUCH (gc_ctx, new_state->retrig_next =
		  state->rule_instance->state_list);
        GC_TOUCH (gc_ctx, state->rule_instance->state_list = new_state);

        /* and recursively call the simulation function */
        simul_ret = simulate_state_and_create_threads(ctx,
                                                      new_state,
                                                      event,
                                                      only_once);
        if (simul_ret < 0)
          return (-created_threads + simul_ret);
        created_threads += simul_ret;
      }
    } else { /* we have a blocking trans, so create a new thread if needed */
      thread = gc_alloc (gc_ctx, sizeof (wait_thread_t),
			 &wait_thread_class);
      thread->gc.type = T_NULL;
      thread->next = NULL;
      thread->trans = &state->state->trans[t];
      GC_TOUCH (gc_ctx, thread->state_instance = state);
      thread->next_in_state_instance = NULL;
      thread->flags = only_once;
      thread->pass = 0;
      state->rule_instance->threads++;
      ctx->threads++;
      created_threads++;

      DebugLog(DF_ENG, DS_DEBUG,
               "-- Create thread %p on state %s trans %i\n",
               thread, state->state->name, t);

      if (only_once == 0) {
        /* add it to the thread-list of the current state instance */
        GC_TOUCH (gc_ctx, thread->next_in_state_instance =
		  state->thread_list);
        GC_TOUCH (gc_ctx, state->thread_list = thread);
      }

#ifdef TIMEOUT_OBSOLETE
      /* compute the timeout date */
      thread->timeout = time(NULL) + DEFAULT_TIMEOUT;
#endif

      /* add thread into the 'new thread' queue */
      if (ctx->new_qt) { /* if queue isn't empty, append to the tail */
        GC_TOUCH (gc_ctx, ctx->new_qt->next = thread);
        GC_TOUCH (gc_ctx, ctx->new_qt = thread);
      } else { /* else create head */
        GC_TOUCH (gc_ctx, ctx->new_qh = thread);
        GC_TOUCH (gc_ctx, ctx->new_qt = thread);
      }
    }
  }
  return created_threads;
}


static void rule_instance_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  rule_instance_t *ri = (rule_instance_t *)p;

  GC_TOUCH (gc_ctx, ri->rule);
  GC_TOUCH (gc_ctx, ri->first_state);
  GC_TOUCH (gc_ctx, ri->next);
  GC_TOUCH (gc_ctx, ri->queue_head);
  GC_TOUCH (gc_ctx, ri->queue_tail);
  GC_TOUCH (gc_ctx, ri->state_list);
  GC_TOUCH (gc_ctx, ri->sync_lock_list);
}

static void rule_instance_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  return;
}

static int rule_instance_traverse (gc_traverse_ctx_t *gtc, gc_header_t *p,
				   void *data)
{
  rule_instance_t *ri = (rule_instance_t *)p;
  int err = 0;

  err = (*gtc->do_subfield) (gtc, (gc_header_t *)ri->rule, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *)ri->first_state, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *)ri->next, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *)ri->queue_head, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *)ri->queue_tail, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *)ri->state_list, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *)ri->sync_lock_list, data);
  return err;
}

static gc_class_t rule_instance_class = {
  GC_ID('r','u','l','i'),
  rule_instance_mark_subfields,
  rule_instance_finalize,
  rule_instance_traverse
};


static void create_rule_initial_threads(orchids_t *ctx,
					active_event_t *event /* XXX: NOT USED */)
{
  rule_t *r;
  state_instance_t *init;
  rule_instance_t *new_rule;
  int ret;
  gc_t *gc_ctx = ctx->gc_ctx;

  GC_START (gc_ctx, 1);
  for (r = ctx->rule_compiler->first_rule; r; r = r->next) {
    init = create_init_state_instance(ctx, r);
    GC_UPDATE (gc_ctx, 0, init);

    new_rule = gc_alloc(gc_ctx, sizeof (rule_instance_t),
			&rule_instance_class);
    new_rule->gc.type = T_NULL;
    GC_TOUCH (gc_ctx, new_rule->rule = r);
    GC_TOUCH (gc_ctx, new_rule->first_state = init);
    new_rule->next = NULL;
    new_rule->state_instances = 1;
    new_rule->creation_date = 0;
    new_rule->new_creation_date.tv_sec = 0;
    new_rule->new_creation_date.tv_usec = 0;
    new_rule->new_last_act.tv_sec = 0;
    new_rule->new_last_act.tv_usec = 0;
    new_rule->queue_head = NULL;
    new_rule->queue_tail = NULL;
    new_rule->state_list = NULL;
    new_rule->max_depth = 0;
    new_rule->threads = 0;
    new_rule->flags = 0;
    new_rule->sync_lock_list = NULL;
    //GC_UPDATE (gc_ctx, 0, new_rule); /* useless, because of the following line */
    GC_TOUCH (gc_ctx, init->rule_instance = new_rule); /* move in create_init_inst() ? */
    /* link rule */
/*     ctx->state_instances++;
*/

    ret = simulate_state_and_create_threads(ctx, init, event, THREAD_ONLYONCE);

    if (ret <= 0) {
      DebugLog(DF_ENG, DS_DEBUG, "No initial threads for rule %s\n",
               r->name);
      cleanup_rule_instance(ctx, new_rule);
    }
    else {
      new_rule->threads = ret;
      GC_TOUCH (gc_ctx, new_rule->next = ctx->first_rule_instance);
      GC_TOUCH (gc_ctx, ctx->first_rule_instance = new_rule);

      if (ctx->new_qt!=NULL)
        ctx->new_qt->flags |= THREAD_BUMP;
    }
  }

  /* Prepare the current retrig queue by merging new and retrig queues
     (q_cur = q_new @ q_retrig) */
  if (ctx->new_qh!=NULL)
    {
      GC_TOUCH (gc_ctx, ctx->new_qt->next = ctx->retrig_qh);
      GC_TOUCH (gc_ctx, ctx->cur_retrig_qh = ctx->new_qh);
      GC_TOUCH (gc_ctx, ctx->cur_retrig_qt =
		(ctx->retrig_qt!=NULL) ? ctx->retrig_qt : ctx->new_qt);
      ctx->new_qh = NULL;
      ctx->new_qt = NULL;
    }
  else
    {
      GC_TOUCH (gc_ctx, ctx->cur_retrig_qh = ctx->retrig_qh);
      GC_TOUCH (gc_ctx, ctx->cur_retrig_qt = ctx->retrig_qt);
    }
  ctx->retrig_qh = NULL;
  ctx->retrig_qt = NULL;
  GC_END(gc_ctx);
}

static void unlink_thread_in_state_instance_list(gc_t *gc_ctx,
						 wait_thread_t *thread)
{
  wait_thread_t *t;
  wait_thread_t *prev_thread;
  wait_thread_t *next_thread;

  DebugLog(DF_ENG, DS_INFO, "unlink_thread_in_state_instance_list(%p)\n",
           thread);

  prev_thread = NULL;
  for (t = thread->state_instance->thread_list; t!=NULL; t = next_thread)
    {
      next_thread = t->next_in_state_instance;
      if (t == thread)
	{
	  if (prev_thread!=NULL)
	    {
	      GC_TOUCH (gc_ctx, prev_thread->next_in_state_instance =
			t->next_in_state_instance);
	    }
	  else
	    {
	      GC_TOUCH (gc_ctx, thread->state_instance->thread_list =
			t->next_in_state_instance);
	    }
	  return;
	}
      else
	{
	  prev_thread = t;
	}
    }
}

static int backtrack_is_not_needed(orchids_t *ctx, wait_thread_t *thread)
{
#if 1
  /* if destination state is fully blocking AND doesn't bind field value
   * to a free variable, this is THE shortest run.
   * Next threads will be redundant. */
  if (thread->trans->dest->trans_nb > 0 &&
      !(thread->trans->dest->flags & BYTECODE_HAS_PUSHFIELD))
    return (TRUE);
  /* XXX: Destination MUST NOT be a cut-point.
     Destination cuts should be resolved at compilation-time
     instead of runtime ? */
#endif

  return (FALSE);
}

static void active_event_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  active_event_t *ae = (active_event_t *)p;

  GC_TOUCH (gc_ctx, ae->event);
  GC_TOUCH (gc_ctx, ae->next);
  GC_TOUCH (gc_ctx, ae->prev);
}

static void active_event_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  return;
}

static int active_event_traverse (gc_traverse_ctx_t *gtc, gc_header_t *p,
				  void *data)
{
  active_event_t *ae = (active_event_t *)p;
  int err = 0;

  err = (*gtc->do_subfield) (gtc, (gc_header_t *)ae->event, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *)ae->next, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *)ae->prev, data);
  return err;
}

static gc_class_t active_event_class = {
  GC_ID('a','e','v','t'),
  active_event_mark_subfields,
  active_event_finalize,
  active_event_traverse
};

void inject_event(orchids_t *ctx, event_t *event)
{
  event_t *e;
  wait_thread_t *t, *next_thread;
  int vmret;
  active_event_t *active_event;
  int sret = 0;
  int ret = 0;
  int passed_threads = 0;
#ifdef TIMEOUT_OBSOLETE
  time_t cur_time;
#endif
  gc_t *gc_ctx = ctx->gc_ctx;

#ifdef TIMEOUT_OBSOLETE
  cur_time = time(NULL);
#endif

  DebugLog(DF_ENG, DS_INFO, "inject_event() (one-evt)\n");

/*   fprintf_safelib_memstat(stderr); */

  ctx->events++;

  /* prepare an active event record */
  active_event = gc_alloc(gc_ctx, sizeof (active_event_t),
			  &active_event_class);
  active_event->gc.type = T_NULL;
  GC_TOUCH (gc_ctx, active_event->event = event);
  active_event->next = NULL;
  active_event->prev = NULL;
  active_event->refs = 0;
  GC_TOUCH (gc_ctx, ctx->active_event_cur = active_event);

  execute_pre_inject_hooks(ctx, active_event->event);

  /* UDP event feedback monitoring */
  if (ctx->evt_fb_fp!=NULL)
    {
      fprintf_event(ctx->evt_fb_fp, ctx, event);
      fflush(ctx->evt_fb_fp);
    }

#if ORCHIDS_DEBUG
  fprintf_event(stderr, ctx, event);
#endif /* ORCHIDS_DEBUG */

  /* -0- resolve field value XXX: should be done with field checks */
  DebugLog(DF_ENG, DS_DEBUG, "STEP 0 - resolve field attribute values\n");
  /* jgl: the following resetting loop is too slow when
     ctx->global_fields->num_fields
     becomes large.  Instead, we will maintain the invariant that
     ctx->global_fields->fields[i].val==NULL between any two calls to inject_event()
  */
//  for (i = 0; i < ctx->global_fields->num_fields; i++) /* clean field val refs */
//    ctx->global_fields->fields[i].val = NULL;

#if ORCHIDS_DEBUG
  for (i=0; i<ctx->global_fields->num_fields; i++)
    if (ctx->global_fields->fields[i].val!=NULL)
      abort();
#endif
  for (e = event; e!=NULL; e = e->next)
    GC_TOUCH (gc_ctx,
	      ctx->global_fields->fields[ e->field_id ].val = e->value);

#if 0
    fprintf(stdout, "begin new queue\n");
    fprintf_thread_queue(stdout, ctx, ctx->new_qh);

    fprintf(stdout, "begin next retrig queue\n");
    fprintf_thread_queue(stdout, ctx, ctx->retrig_qh);
#endif

  /* q-init */
  DebugLog(DF_ENG, DS_DEBUG, "STEP 1 - create initial threads (q-init)\n");
  create_rule_initial_threads(ctx, active_event);

#if ORCHIDS_DEBUG
  fprintf(stderr, "current retrig queue\n");
  fprintf_thread_queue(stderr, ctx, ctx->cur_retrig_qh);
#endif /* ORCHIDS_DEBUG */

  /* evt-loop */
  DebugLog(DF_ENG, DS_DEBUG,
           "STEP 2 - evaluate all threads in retrig queue (evt-loop)\n");
  for (t = ctx->cur_retrig_qh; t!=NULL; t = next_thread) {
    next_thread = t->next;
    GC_TOUCH (gc_ctx, ctx->current_tail = t);

#ifdef TIMEOUT_OBSOLETE
    /* Check timeout date */
    if ( !(t->flags & THREAD_ONLYONCE) && (t->timeout <= cur_time) )
      {
	DebugLog(DF_ENG, DS_DEBUG, "thread %p timed out ! (killing)\n", t);
	t->flags |= THREAD_KILLED;
      }
#endif

    /* Killed thread reaper (and rule instance if apply) */
    if ( THREAD_IS_KILLED(t) )
      {
	ctx->last_ruleinst_act = ctx->cur_loop_time;
	DebugLog(DF_ENG, DS_DEBUG, "Reap and override killed thread (%p)\n", t);
	t->state_instance->rule_instance->threads--;
	ctx->threads--;
	if ( NO_MORE_THREAD(t->state_instance->rule_instance) )
	  {
	    /* Update rule instance list links (before removing) */
	    t->state_instance->rule_instance->flags |= THREAD_KILLED;
	    reap_dead_rule(ctx, t->state_instance->rule_instance);
	  }
	else
	  {
	    unlink_thread_in_state_instance_list(gc_ctx, t);
	  }
	if (t == ctx->cur_retrig_qh)
	  {
	    GC_TOUCH (gc_ctx, ctx->cur_retrig_qh = next_thread);
	  }
	if (ctx->cur_retrig_qt!=NULL)
	  ctx->cur_retrig_qt->next = NULL;
	ctx->current_tail = NULL;
	continue;
      }

    DebugLog(DF_ENG, DS_DEBUG,
             "  processing thread %p (r=%s s=%s d=%s)\n",
             (void *)t,
             t->state_instance->rule_instance->rule->name,
             t->state_instance->state->name,
             t->trans->dest->name);

    vmret = ovm_exec_expr(ctx, t->state_instance, t->trans->eval_code);
    if (vmret == 0)
      {
	state_instance_t *new_state;

	DebugLog(DF_ENG, DS_DEBUG,
		 "TRANSITION MATCH ! (destination '%s')\n",
		 t->trans->dest->name);
	t->pass++;
	passed_threads++;

	ctx->last_ruleinst_act = ctx->cur_loop_time;
	t->state_instance->rule_instance->new_last_act = ctx->last_ruleinst_act;

	if ( THREAD_IS_ONLYONCE(t) )
	  {
	    DebugLog(DF_ENG, DS_DEBUG,
		     "Initial thread passed. Create rule instance.\n");
	    t->state_instance->rule_instance->creation_date = time(NULL);
	    t->state_instance->rule_instance->new_creation_date =
	      ctx->last_ruleinst_act;
	    t->state_instance->rule_instance->rule->instances++;
	    t->state_instance->rule_instance->flags |= RULE_INUSE;
	    ctx->rule_instances++;
	  }

	/* Create a new state instance */
	new_state = create_state_instance(ctx,
					  t->trans->dest, t->state_instance);

	/* Link the new_state instance into the tree */
	GC_TOUCH (gc_ctx, new_state->next_sibling =
		  t->state_instance->first_child);
	GC_TOUCH (gc_ctx, t->state_instance->first_child = new_state);
	GC_TOUCH (gc_ctx, new_state->parent = t->state_instance);
	GC_TOUCH (gc_ctx, new_state->event = active_event);
	active_event->refs++;

	/* Update state instance list of the current rule instance */
	GC_TOUCH (gc_ctx, new_state->retrig_next =
		  t->state_instance->rule_instance->state_list);
	GC_TOUCH (gc_ctx, t->state_instance->rule_instance->state_list =
		  new_state);

	/* And recursively call simulation function */
	sret = simulate_state_and_create_threads(ctx,
						 new_state, active_event, 0);
	if (sret < 0)
	  ret -= sret;
	else
	  ret += sret;

	/* Static analysis flags test here (RETRIGGER) */
	if ( backtrack_is_not_needed(ctx, t) ) {
	  DebugLog(DF_ENG, DS_DEBUG, "Backtrack not needed\n");
	  KILL_THREAD(ctx, t);
	}
      }

    if ( THREAD_IS_ONLYONCE(t))
      {
	KILL_THREAD(ctx, t);
	/* Initial thread reaper: an initial thread can't be free()d at
	 * the next loop because initial environment can make references
	 * to the current event.  In case the event didn't match
	 * any transition, it will be free()d _before_ the initial
	 * environment, so we'll lose the reference.  */

	DebugLog(DF_ENG, DS_DEBUG, "Reap INITIAL thread (%p)\n", t);

	if (t->flags & THREAD_BUMP)
	  {
	    DebugLog(DF_ENG, DS_DEBUG,
		     "initial thread bump "
		     "(commit q'_{new} = q_{new} @ q_{retrig})\n");
	    if (ctx->retrig_qt)
	      ctx->retrig_qt->flags |= THREAD_BUMP;
	    if (ctx->new_qh!=NULL)
	      {
		ctx->new_qt->flags |= THREAD_BUMP;
		GC_TOUCH (gc_ctx, ctx->new_qt->next = ctx->retrig_qh);
		if (ctx->retrig_qt!=NULL)
		  GC_TOUCH (gc_ctx, ctx->new_qt = ctx->retrig_qt);
		ctx->retrig_qh = NULL;
		ctx->retrig_qt = NULL;
	      }
	    else
	      { /* q_new is empty */
		GC_TOUCH (gc_ctx, ctx->new_qh = ctx->retrig_qh);
		GC_TOUCH (gc_ctx, ctx->new_qt = ctx->retrig_qt);
		ctx->retrig_qh = NULL;
		ctx->retrig_qt = NULL;
	      }
	  }

	t->state_instance->rule_instance->threads--;
	ctx->threads--;
	if ( NO_MORE_THREAD(t->state_instance->rule_instance) )
	  {
	    /* Update rule instance list links (before removing) */
	    t->state_instance->rule_instance->flags |= THREAD_KILLED;
	    reap_dead_rule(ctx, t->state_instance->rule_instance);
	  }
	else
	  {
	    unlink_thread_in_state_instance_list(gc_ctx, t);
	  }
	if (t == ctx->cur_retrig_qh)
	  {
	    GC_TOUCH (gc_ctx, ctx->cur_retrig_qh = next_thread);
	  }
	if (ctx->cur_retrig_qt!=NULL)
	  ctx->cur_retrig_qt->next = NULL;
	ctx->current_tail = NULL;
	continue;
      }

    DebugLog(DF_ENG, DS_DEBUG, "RETRIG q'_{retrig} = q_{retrig} + t\n");
    if (ctx->retrig_qh!=NULL)
      {
	GC_TOUCH (gc_ctx, ctx->retrig_qt->next = t);
	GC_TOUCH (gc_ctx, ctx->retrig_qt = t);
      }
    else
      {
	GC_TOUCH (gc_ctx, ctx->retrig_qh = t);
	GC_TOUCH (gc_ctx, ctx->retrig_qt = t);
      }
    t->next = NULL;

#if 0
    fprintf(stdout, "thread loop new queue\n");
    fprintf_thread_queue(stdout, ctx, ctx->new_qh);

    fprintf(stdout, "thread loop  next retrig queue\n");
    fprintf_thread_queue(stdout, ctx, ctx->retrig_qh);
#endif

    /* If we need to commit new threads, then merge two queues */
    if (t->flags & THREAD_BUMP)
      {
	DebugLog(DF_ENG, DS_DEBUG,
		 "thread bump (commit q'_{new} = q_{new} @ q_{retrig})\n");
	if (ctx->retrig_qt!=NULL)
	  ctx->retrig_qt->flags |= THREAD_BUMP;
	if (ctx->new_qh!=NULL)
	  {
	    ctx->new_qt->flags |= THREAD_BUMP;
	    GC_TOUCH (gc_ctx, ctx->new_qt->next = ctx->retrig_qh);
	    if (ctx->retrig_qt!=NULL)
	      GC_TOUCH (gc_ctx, ctx->new_qt = ctx->retrig_qt);
	    ctx->retrig_qh = NULL;
	    ctx->retrig_qt = NULL;
	  }
	else
	  { /* q_new is empty */
	    GC_TOUCH (gc_ctx, ctx->new_qh = ctx->retrig_qh);
	    GC_TOUCH (gc_ctx, ctx->new_qt = ctx->retrig_qt);
	    ctx->retrig_qh = NULL;
	    ctx->retrig_qt = NULL;
	  }
      }

#if 0
    fprintf(stdout, "end new queue\n");
    fprintf_thread_queue(stdout, ctx, ctx->new_qh);

    fprintf(stdout, "end next retrig queue\n");
    fprintf_thread_queue(stdout, ctx, ctx->retrig_qh);
#endif

  }

  execute_post_inject_hooks(ctx, active_event->event);

  for (e = event; e!=NULL; e = e->next)
    ctx->global_fields->fields[ e->field_id ].val = NULL;

  if (active_event->refs != 0) /* If event still alive: */
    {
      ctx->last_evt_act = ctx->cur_loop_time;
      DebugLog(DF_ENG, DS_DEBUG,
	       "Keep new event: active_event->refs = %i (linking) %i\n",
	       active_event->refs, passed_threads);
      if (ctx->active_event_head == NULL)
	{
	  GC_TOUCH (gc_ctx, ctx->active_event_head = active_event);
	  GC_TOUCH (gc_ctx, ctx->active_event_tail = active_event);
	}
      else
	{
	  GC_TOUCH (gc_ctx, active_event->prev = ctx->active_event_tail);
	  GC_TOUCH (gc_ctx, ctx->active_event_tail->next = active_event);
	  GC_TOUCH (gc_ctx, ctx->active_event_tail = active_event);
	}
      ctx->active_events++;
    }

  DebugLog(DF_ENG, DS_TRACE,
           "simulate_state_and_create_threads() = %i\n", ret);

#ifdef DMALLOC
  dmalloc_log_changed(dmalloc_orchids, 1, 1, 1);
#endif
}

static void state_instance_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  state_instance_t *si = (state_instance_t *)p;

  GC_TOUCH (gc_ctx, si->first_child);
  GC_TOUCH (gc_ctx, si->next_sibling);
  GC_TOUCH (gc_ctx, si->parent);
  GC_TOUCH (gc_ctx, si->event);
  GC_TOUCH (gc_ctx, si->rule_instance);
  GC_TOUCH (gc_ctx, si->env);
  GC_TOUCH (gc_ctx, si->retrig_next);
  GC_TOUCH (gc_ctx, si->thread_list);
  GC_TOUCH (gc_ctx, si->next_report_elmt);
}

static void state_instance_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  state_instance_t *si = (state_instance_t *)p;

  remove_thread_local_entries (si);
}

static int state_instance_traverse (gc_traverse_ctx_t *gtc,
				    gc_header_t *p,
				    void *data)
{
  state_instance_t *si = (state_instance_t *)p;
  int err = 0;

  err = (*gtc->do_subfield) (gtc, (gc_header_t *) si->first_child, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *) si->next_sibling, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *) si->parent, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *) si->event, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *) si->rule_instance, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *) si->env, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *) si->retrig_next, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *) si->thread_list, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *) si->next_report_elmt, data);
  return err;
}

static gc_class_t state_instance_class = {
  GC_ID('s','t','t','i'),
  state_instance_mark_subfields,
  state_instance_finalize,
  state_instance_traverse
};

static state_instance_t *create_state_instance(orchids_t *ctx,
					       state_t *state,
					       const state_instance_t *parent)
{
  state_instance_t *new_state;

  /* Allocate and init state instance */
  new_state = gc_alloc (ctx->gc_ctx, sizeof (state_instance_t),
			&state_instance_class);
  new_state->gc.type = T_STATE_INSTANCE;
  new_state->state = state; /* no GC_TOUCH() here! state is not gc-able */
  GC_TOUCH (ctx->gc_ctx, new_state->rule_instance = parent->rule_instance);
  new_state->first_child = NULL;
  new_state->next_sibling = NULL;
  new_state->parent = NULL;
  new_state->event_level = 0;
  new_state->flags = 0;
  new_state->event = NULL;
  GC_TOUCH (ctx->gc_ctx, new_state->env = parent->env);
  new_state->thread_locals = NULL;
  new_state->retrig_next = NULL;
  ctx->state_instances++;
  return new_state;
}


static state_instance_t *create_init_state_instance(orchids_t *ctx, const rule_t *rule)
{
  state_t *state;
  state_instance_t *new_state;

  state = &rule->state[0];
  new_state = gc_alloc (ctx->gc_ctx, sizeof (state_instance_t),
			&state_instance_class);
  new_state->gc.type = T_STATE_INSTANCE;
  new_state->state = state; /* no GC_TOUCH() here! state is not gc-able */
  new_state->rule_instance = NULL;
  new_state->first_child = NULL;
  new_state->next_sibling = NULL;
  new_state->parent = NULL;
  new_state->event_level = 0;
  new_state->flags = 0;
  new_state->event = NULL;
  new_state->env = NULL;
  new_state->thread_locals = NULL;
  new_state->retrig_next = NULL;
  new_state->thread_list = NULL;
  new_state->next_report_elmt = NULL;
  ctx->state_instances++;
  return new_state;
}


static void cleanup_rule_instance(orchids_t *ctx, rule_instance_t *rule_instance)
{
  state_instance_t *si;
  state_instance_t *next_si;
  sync_lock_list_t *lock_elmt;
  sync_lock_list_t *lock_next;

  DebugLog(DF_ENG, DS_DEBUG, "cleanup_rule_instance(%p)\n", rule_instance);

  /* Remove synchronization locks, if any exists */
  for (lock_elmt = rule_instance->sync_lock_list;
       lock_elmt!=NULL;
       lock_elmt = lock_next)
    {
      lock_next = lock_elmt->next;
      DebugLog(DF_ENG, DS_DEBUG, "cleanup_rule_instance(%p): removing lock %p\n",
	       rule_instance, lock_elmt->state);
      si = objhash_del(rule_instance->rule->sync_lock, lock_elmt->state);
      if (si == NULL)
	{
	  DebugLog(DF_ENG, DS_ERROR,
		   "cleanup_rule_instance(%p): lock not found\n",
		   rule_instance);
	}
  }
  ctx->state_instances--;

  si = rule_instance->state_list;
  while (si) {
    next_si = si->retrig_next;

    /* Update event reference count */
    if (si->event) {
      si->event->refs--;

      /* The current active event is freed in inject_event() if unreferenced.
       * There is the special case of rules that terminate after exactly one
       * event: the event matches a transition, is referenced, the rule reaches
       * instantaneously a final state, then terminate.  Here, we have to
       * only free events other than the current one (i.e. past events). */
      if (si->event->refs <= 0 && si->event != ctx->active_event_cur) {
        ctx->last_evt_act = ctx->cur_loop_time;
        DebugLog(DF_ENG, DS_DEBUG, "event %p ref=0\n", si->event);
        si->event->event = NULL;
        ctx->active_events--;
        /* unlink */
        if (!si->event->prev) { /* If we are in the first event reference */
          if (!si->event->next) { /* If it is also the last (it is alone) */
            ctx->active_event_head = NULL;
            ctx->active_event_tail = NULL;
          }
          else {
            /* Else it is the first, and there is other references after it */
            GC_TOUCH (ctx->gc_ctx, ctx->active_event_head = si->event->next);
            ctx->active_event_head->prev = NULL;
          }
          /* si->event->next->prev = si->event->prev; */
        }
        else { /* Else the event reference is not the first */

          if (!si->event->next) { /* If it is exactly the last one */
            /* Then update the previous element */
            si->event->prev->next = NULL;
            GC_TOUCH (ctx->gc_ctx, ctx->active_event_tail = si->event->prev);
          }
          else { /* Else the event reference is not the last
                    we are in the middle of the list */
            GC_TOUCH (ctx->gc_ctx, si->event->prev->next = si->event->next);
            GC_TOUCH (ctx->gc_ctx, si->event->next->prev = si->event->prev);
          }
          /* ctx->active_event_tail = si->event->prev; */
        }
      }
    }
    si = next_si;
    ctx->state_instances--;
  }

  if (rule_instance->creation_date) {
    rule_instance->rule->instances--;
    ctx->rule_instances--;
  }
}


void fprintf_rule_instances(FILE *fp, const orchids_t *ctx)
{
  char asc_time[32];
  rule_instance_t *r;
  int i;

  if (ctx->first_rule_instance == NULL) {
    fprintf(fp, "no rule instance.\n");
    return;
  }

  fprintf(fp,
          "-------------------------------[ "
          "rule instances"
          " ]------------------------------\n");
  fprintf(fp, " rid | rule name   | state |thrds| dpt | creation date\n");
  fprintf(fp,
          "-----+-------------+-------+-----+"
          "-----+---------------------------------------\n");
  for (r = ctx->first_rule_instance, i = 0; r; r = r->next, ++i) {
    if (r->creation_date > 0) {
      strftime(asc_time, 32, "%a %b %d %H:%M:%S %Y",
               localtime(&r->creation_date));
    }
    else {
      strcpy(asc_time, "initial instance");
    }
    fprintf(fp, " %3i |%12.12s |%6lu |%4i | %3i | %li (%.32s)\n",
            i, r->rule->name, r->state_instances, r->threads,
            r->max_depth, r->creation_date, asc_time);
  }
  fprintf(fp,
          "-----+-------------+-------+-----+"
          "-----+---------------------------------------\n");
}


void fprintf_thread_queue(FILE *fp, orchids_t *ctx, wait_thread_t *thread)
{
  unsigned int i;
  unsigned int k;

  fprintf(fp,
          "----------------------------[ "
          "thread wait queue"
          " ]----------------------------\n");
  fprintf(fp,
          " thid |"
          "   rule name  |"
          "      state names     |"
          " rid | sid->did |tran| cnt | bmp\n");
  fprintf(fp,
          "------+"
          "--------------+"
          "----------------------+"
          "-----+----------+----+-----+----\n");

  k = 0;
  for (i = 0; thread!=NULL; thread = thread->next, i++)
    {
      if (!(thread->flags & THREAD_KILLED))
	{
	  fprintf(fp,
		  "%5u | "
		  "%12.12s | "
		  "%8.8s -> %-8.8s | "
		  "%3i | "
		  "%2i -> %-2i | "
		  "%2i | "
		  "%3i | "
		  "%s\n",
		  i,
		  thread->state_instance->state->rule->name,
		  thread->state_instance->state->name,
		  thread->trans->dest->name,
		  thread->state_instance->state->rule->id,
		  thread->state_instance->state->id,
		  thread->trans->dest->id,
		  thread->trans->id,
		  thread->pass,
		  (thread->flags & THREAD_BUMP) ? "_." : "  ");
	}
      else
	{
	  k++;
	  fprintf(fp,
		  "%5u*| "
		  "%12.12s | "
		  "%8.8s -> %-8.8s | "
		  "%3i | "
		  "%2i -> %-2i | "
		  "%2i | "
		  "%3i |"
		  "%s\n",
		  i,
		  thread->state_instance->state->rule->name,
		  thread->state_instance->state->name,
		  thread->trans->dest->name,
		  thread->state_instance->state->rule->id,
		  thread->state_instance->state->id,
		  thread->trans->dest->id,
		  thread->trans->id,
		  thread->pass,
		  (thread->flags & THREAD_BUMP) ? "_." : "  "); 
	}
    }
  fprintf(fp, "(*) %u killed threads\n", k);
  fprintf(fp,
          "------+"
          "--------------+"
          "----------------------+"
          "-----+----------+----+-----+----\n");
}

void fprintf_active_events(FILE *fp, orchids_t *ctx)
{
  unsigned int i;
  active_event_t *e;

  if (ctx->active_event_head == NULL)
    {
      fprintf(fp, "no active event.\n");
      return;
    }

  fprintf(fp, "--------------[ active events ]-------------\n");

  for (i = 0, e = ctx->active_event_head; e!=NULL; e = e->next, i++)
    {
      fprintf(fp,
	      "\n"
	      "=============================[ "
	      "evt %5u (%i refs)"
	      " ]============================\n\n", i, e->refs);
      fprintf_event(fp, ctx, e->event);
    }
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
