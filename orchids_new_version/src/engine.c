/**
 ** @file engine.c
 ** Analysis engine.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 ** @author Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
 **
 ** @version 2.0
 ** @ingroup engine
 **
 ** @date  Started on: Jeu  2 jul 2015 10:57:40 UTC
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
#include <limits.h>

#include "orchids.h"
#include "lang.h"
#include "orchids_api.h"

#include "engine.h"

static void thread_group_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  thread_group_t *pid = (thread_group_t *)p;

  GC_TOUCH (gc_ctx, pid->rule);
}

static void thread_group_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  //thread_group_t *pid = (thread_group_t *)p;

  return;
}

static int thread_group_traverse (gc_traverse_ctx_t *gtc,
				  gc_header_t *p,
				  void *data)
{
  thread_group_t *pid = (thread_group_t *)p;
  int err = 0;

  err = (*gtc->do_subfield) (gtc, (gc_header_t *) pid->rule, data);
  return err;
}

static gc_class_t thread_group_class = {
  GC_ID('p','i','d',' '),
  thread_group_mark_subfields,
  thread_group_finalize,
  thread_group_traverse
};

static void state_instance_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  state_instance_t *si = (state_instance_t *)p;

  GC_TOUCH (gc_ctx, si->pid);
  GC_TOUCH (gc_ctx, si->env);
}

static void state_instance_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  //state_instance_t *si = (state_instance_t *)p;

  /*remove_thread_local_entries (si);*/
}

static int state_instance_traverse (gc_traverse_ctx_t *gtc,
				    gc_header_t *p,
				    void *data)
{
  state_instance_t *si = (state_instance_t *)p;
  int err = 0;

  err = (*gtc->do_subfield) (gtc, (gc_header_t *) si->pid, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *) si->env, data);
  return err;
}

static gc_class_t state_instance_class = {
  GC_ID('s','t','t','i'),
  state_instance_mark_subfields,
  state_instance_finalize,
  state_instance_traverse
};

static void thread_queue_elt_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  thread_queue_elt_t *qe = (thread_queue_elt_t *)p;

  GC_TOUCH (gc_ctx, qe->thread);
  GC_TOUCH (gc_ctx, qe->next);
}

static void thread_queue_elt_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  return;
}

static int thread_queue_elt_traverse (gc_traverse_ctx_t *gtc,
				      gc_header_t *p,
				      void *data)
{
  thread_queue_elt_t *qe = (thread_queue_elt_t *)p;
  int err = 0;

  err = (*gtc->do_subfield) (gtc, (gc_header_t *) qe->thread, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *) qe->next, data);
  return err;
}

static gc_class_t thread_queue_elt_class = {
  GC_ID('t','h','q','e'),
  thread_queue_elt_mark_subfields,
  thread_queue_elt_finalize,
  thread_queue_elt_traverse
};

static void thread_queue_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  thread_queue_t *tq = (thread_queue_t *)p;

  GC_TOUCH (gc_ctx, tq->first);
  GC_TOUCH (gc_ctx, tq->last);
}

static void thread_queue_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  return;
}

static int thread_queue_traverse (gc_traverse_ctx_t *gtc,
				  gc_header_t *p,
				  void *data)
{
  thread_queue_t *tq = (thread_queue_t *)p;
  int err = 0;

  err = (*gtc->do_subfield) (gtc, (gc_header_t *) tq->first, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *) tq->last, data);
  return err;
}

static gc_class_t thread_queue_class = {
  GC_ID('t','h','q','.'),
  thread_queue_mark_subfields,
  thread_queue_finalize,
  thread_queue_traverse
};

static thread_queue_t *new_thread_queue (gc_t *gc_ctx)
{
  thread_queue_t *tq;

  tq = gc_alloc (gc_ctx, sizeof (thread_queue_t), &thread_queue_class);
  tq->gc.type = T_NULL;
  tq->nelts = 0;
  tq->first = NULL;
  tq->last = NULL;
  return tq;
}

#define THREAD_QUEUE_IS_EMPTY(tq) ((tq)->first==NULL)

static state_instance_t *thread_dequeue (gc_t *gc_ctx, thread_queue_t *tq)
{ /* tq should not be empty here,
     in the sense that THREAD_QUEUE_IS_EMPTY(tq) should be false */
  thread_queue_elt_t *qe, *last;
  state_instance_t *si;

  qe = tq->first;
  si = qe->thread;
  last = tq->last;
  if (qe==last) /* only one element left: erase queue */
    {
      tq->nelts = 0;
      tq->first = NULL; /* no need to GC_TOUCH() NULL */
      tq->last = NULL; /* no need to GC_TOUCH() NULL */
    }
  else
    {
      GC_TOUCH (gc_ctx, tq->first = qe->next);
      if (si!=NULL)
	{
	  si->pid->rule->instances--;
	  tq->nelts--;
	}
    }
  return si;
}

static void thread_enqueue (gc_t *gc_ctx, thread_queue_t *tq, state_instance_t *si)
{ /* add si to the end of queue tq,
     unless si==NULL (BUMP) and last element of tq is already BUMP */
  thread_queue_elt_t *qe;
  thread_queue_elt_t *last;

  last = tq->last;
  if (si==NULL && last!=NULL && last->thread==NULL)
    return; /* si==BUMP, there is a last element, and its thread is BUMP */
  qe = gc_alloc (gc_ctx, sizeof(thread_queue_elt_t), &thread_queue_elt_class);
  qe->gc.type = T_NULL;
  qe->next = NULL; /* no need to GC_TOUCH() NULL */
  GC_TOUCH (gc_ctx, qe->thread = si);
  if (si!=NULL)
    {
      tq->nelts++;
      si->pid->rule->instances++;
    }
  if (THREAD_QUEUE_IS_EMPTY(tq))
    {
      GC_TOUCH (gc_ctx, tq->first = qe);
    }
  else
    {
      GC_TOUCH (gc_ctx, last->next = qe);
    }
  GC_TOUCH (gc_ctx, tq->last = qe);
}

static void thread_enqueue_all (gc_t *gc_ctx, thread_queue_t *tq, thread_queue_t *all)
{ /* concatenate the 'all' queue to the end of tq;
     'all' is destroyed after that;
     if last element of tq is BUMP and first element of 'all' is BUMP, remove one of
     them */
  thread_queue_elt_t *last;

  last = tq->last;
  if (last!=NULL && last->thread==NULL &&
      !THREAD_QUEUE_IS_EMPTY(all) && all->first->thread==NULL)
    (void) thread_dequeue (gc_ctx, all); /* remove first BUMP of 'all' */
  if (THREAD_QUEUE_IS_EMPTY(all))
    return; /* nothing to do */
  if (THREAD_QUEUE_IS_EMPTY(tq))
    {
      tq->nelts = all->nelts;
      GC_TOUCH (gc_ctx, tq->first = all->first);
    }
  else
    {
      tq->nelts += all->nelts;
      GC_TOUCH (gc_ctx, tq->last->next = all->first);
    }
  GC_TOUCH (gc_ctx, tq->last = all->last);
  all->nelts = 0;
  all->first = NULL; /* no need to GC_TOUCH() NULL */
  all->last = NULL; /* no need to GC_TOUCH() NULL */
}

static int sync_var_env_is_defined(state_instance_t *state)
{ /*!!! should be optimized, by maintaining a count of assigned variables, say */
  size_t i;
  size_t sync_vars_sz;
  int32_t sync_var;
  int32_t *vars;

  sync_vars_sz = state->pid->rule->sync_vars_sz;
  vars = state->pid->rule->sync_vars;

  for (i = 0; i < sync_vars_sz; i++)
    {
      sync_var = vars[i];
      if (ovm_read_value (state->env, sync_var)==NULL)
	return 0;
    }
  return 1;
}

static ovm_var_t *copy_high_handles (gc_t *gc_ctx, ovm_var_t *env, uint16_t nhandles)
{
  uint16_t k;
  ovm_var_t *val;

  GC_START (gc_ctx, 1);
  for (k=MAX_HANDLES; k<nhandles; k++)
    {
      val = ovm_read_value (env, HANDLE_FAKE_VAR(k));
      GC_UPDATE(gc_ctx, 0, val);
      val = issdl_clone (gc_ctx, val);
      GC_UPDATE(gc_ctx, 0, val);
      env = ovm_write_value (gc_ctx, env, HANDLE_FAKE_VAR(k), val);
      GC_UPDATE(gc_ctx, 0, val);
    }
  GC_END(gc_ctx);
  return env;
}

static state_instance_t *create_state_instance (orchids_t *ctx,
						thread_group_t *pid,
						state_t *state,
						transition_t *trans,
						ovm_var_t *env,
						uint16_t nhandles)
{
  state_instance_t *newsi;
  gc_t *gc_ctx = ctx->gc_ctx;

  /* First, we make fresh copies of all handles >=MAX_HANDLES.
     This is inefficient, but is not meant to happen: it is
     expected that we would have very few handles at the same
     time, and all will have handles<MAX_HANDLES.  The latter
     will be copied lazily, by handle_get_wr(), not here. */
  if (nhandles>MAX_HANDLES)
    env = copy_high_handles (gc_ctx, env, nhandles);
  /* Allocate and init state instance */
  newsi = gc_alloc (ctx->gc_ctx, sizeof (state_instance_t),
			&state_instance_class);
  newsi->gc.type = T_STATE_INSTANCE;
  GC_TOUCH (gc_ctx, newsi->pid = pid);
  pid->nsi++;
  newsi->q = state; /* no GC_TOUCH() here! state is not gc-able */
  newsi->t = trans; /* no GC_TOUCH() here! trans is not gc-able */
  GC_TOUCH (ctx->gc_ctx, newsi->env = env);
  newsi->nhandles = nhandles;
  newsi->owned_handles = 0; /* new threads own none of the handles:
			       if they wish to modify the value of the
			       handle, they will have to copy it first. */
  return newsi;
}

static void cleanup_state_instance (state_instance_t *si)
/* Cleanup state_instance before we throw it away. */
{ /* if state_instance was synchronized, then remove its entry from
     the table of synchronized state_instances. */
  struct thread_group_s *pid;

  pid = si->pid;
  --pid->nsi;
  if (pid->nsi==0 && pid->rule->sync_lock!=NULL && sync_var_env_is_defined (si))
    {
      (void) objhash_del (pid->rule->sync_lock, si);
    }
}

static void detach_state_instance (gc_t *gc_ctx, state_instance_t *si)
{
  thread_group_t *pid, *newpid;

  pid = si->pid;
  cleanup_state_instance (si);
  newpid = gc_alloc (gc_ctx, sizeof(thread_group_t), &thread_group_class);
  newpid->gc.type = T_NULL;
  GC_TOUCH (gc_ctx, newpid->rule = pid->rule);
  newpid->nsi = 1;
  newpid->flags = 0;
  GC_TOUCH (gc_ctx, si->pid = newpid);
}

static void enter_state_and_follow_epsilon_transitions (orchids_t *ctx,
							state_instance_t *si,
							thread_queue_t *tq,
							int only_once)
{
  size_t i, trans_nb;
  state_instance_t *newsi;
  transition_t *trans;
  ovm_var_t *oldenv;
  objhash_t *lock_table;
  struct thread_group_s *sync_lock;
  state_t *q;
  gc_t *gc_ctx = ctx->gc_ctx;

  GC_START (gc_ctx, 1);
  q = si->q;
  if (q->flags & STATE_COMMIT)
    {
      if (si->pid->nsi!=1)
	{ /* optimization: if only one state instance has this pid (namely, ourselves: si),
	     then no need to kill the other state instances with the same pid
	     and to detach. */
	  si->pid->flags |= THREAD_KILL;
	  detach_state_instance (gc_ctx, si);
	}
    }
  if (q->action!=NULL)
    {
      oldenv = si->env;
      ovm_exec_stmt (ctx, si, q->action);
      /* This is the unique place where synchronization may take place, when
	 executing code inside states.  Indeed, synchronization can only
	 happen when variables change.  The call to ovm_exec_expr() done
	 when evaluating 'expect' clauses cannot change any variable. */
      lock_table = si->pid->rule->sync_lock;
      if (si->env!=oldenv /* Also, we need to check for synchronization
			     only if the environment actually changed, */
	  && lock_table!=NULL /* and if the rule is actually synchronized, */
	  && sync_var_env_is_defined (si) /* and if all the synchronization variables
					     are defined. */
	  )
	{
	  sync_lock = objhash_get (lock_table, si);
	  if (sync_lock!=NULL)
	    { /* some pid sync_lock (with the same rule) already holds the lock;
		 then we redirect our pid to be sync_lock;
		 this is rule (7), p.22 in the spec */
	      /* As an exception, it may be that sync_lock is a group
		 of threads that have been killed (typically because we
		 just entered a STATE_COMMIT state), in which case we do
		 not join that group, instead we declare the synchronization
		 group to be our own group---with should not be killed. */
	      if (sync_lock->flags & THREAD_KILL)
		{ /* declare si->pid to be the new synchronization group */
		  objhash_del (lock_table, si);
		  GC_TOUCH (gc_ctx, si->pid);
		  objhash_add (gc_ctx, lock_table, si, si->pid);
		}
	      else
		{ /* join the synchronization group */
		  sync_lock->nsi += si->pid->nsi;
		  GC_TOUCH (gc_ctx, si->pid = sync_lock);
		  /* there is no 'anchored' rule in Orchids (the boolean represented
		     by the lightning-shaped down arrow in the spec is always false),
		     so we never kill a thread directly because of synchronization. */
		}
	    }
	  else
	    { /* create a lock. */
	      GC_TOUCH (gc_ctx, si->pid);
	      objhash_add (gc_ctx, lock_table, si, si->pid);
	    }
	}
    }
  trans_nb = q->trans_nb;
  if (q->flags & EPSILON_STATE) /* follow the first epsilon transition that matches */
    for (i=0, trans=q->trans; i<trans_nb; i++, trans++)
      {
	if (trans->eval_code==NULL ||
	    ovm_exec_expr (ctx, si, trans->eval_code)==0)
	  {
	    si->q = trans->dest;
	    enter_state_and_follow_epsilon_transitions (ctx, si, tq, only_once);
	    break;
	  }
      }
  else if (only_once)
    for (i=0, trans=q->trans; i<trans_nb; i++, trans++)
      { /* ordinary transition ('expect (<cond>) goto <state>'),
	   and flag only_once is set: create temporary fresh state_instance and try to match
	   transition; then cleanup the fresh state_instance, since it is not enqueued */
	if (trans->eval_code==NULL ||
	    ovm_exec_expr (ctx, si, trans->eval_code)==0)
	  {
	    newsi = create_state_instance (ctx, si->pid, trans->dest, trans, si->env, si->nhandles);
	    GC_UPDATE (gc_ctx, 0, newsi);
	    /* now we call ourselves back with only_once false */
	    enter_state_and_follow_epsilon_transitions (ctx, newsi, tq, 0);
	    cleanup_state_instance (newsi);
	  }
      }
  else
    for (i=0, trans=q->trans; i<trans_nb; i++, trans++)
      { /* ordinary transition ('expect (<cond>) goto <state>')
	   then we place ourselves on the queue */
	newsi = create_state_instance (ctx, si->pid, q, trans, si->env, si->nhandles);
	GC_UPDATE (gc_ctx, 0, newsi);
	thread_enqueue (gc_ctx, tq, newsi);
      }
  GC_END (gc_ctx);
}

static int simulate_state_and_create_threads (orchids_t *ctx,
					      state_instance_t *si,
					      thread_queue_t *tq)
{
  transition_t *t; /* t should not be NULL, and should not be an epsilon transition */
  int vmret;

  t = si->t;
  vmret = 1;
  if (t->eval_code!=NULL)
    vmret = ovm_exec_trans_cond (ctx, si, t->eval_code);
  if (vmret > 0)
    { /* OK: pass transition */
      si->q = t->dest;
      enter_state_and_follow_epsilon_transitions (ctx, si, tq, 0);
    }
  return vmret;
}

static void create_rule_initial_threads (orchids_t *ctx, thread_queue_t *tq)
{
  rule_t *r;
  thread_group_t *pid;
  state_instance_t *init;
  state_t *q;
  gc_t *gc_ctx = ctx->gc_ctx;

  GC_START (gc_ctx, 1);
  for (r = ctx->rule_compiler->first_rule; r!=NULL; r = r->next)
    {
      if (r->flags & RULE_INITIAL_ALREADY_LAUNCHED)
	continue; /* The 'anchored' rules of the spec, that is, the rules
		     that should start to be monitored at the very beginning,
		     but should never be relaunched later on. */
      pid = gc_alloc (gc_ctx, sizeof(thread_group_t), &thread_group_class);
      pid->gc.type = T_NULL;
      GC_TOUCH (gc_ctx, pid->rule = r);
      pid->nsi = 0;
      pid->flags = 0;
      GC_UPDATE (gc_ctx, 0, pid);
      q = &r->state[0];
      if (q->flags & STATE_COMMIT) /* 'anchored' rule */
	r->flags |= RULE_INITIAL_ALREADY_LAUNCHED;
      init = create_state_instance (ctx, pid, q, NULL, NULL, 0);
      /* create state instance, in state q, waiting for no transition yet (NULL),
	 and with empty environment (NULL) */
      GC_UPDATE (gc_ctx, 0, init);
      enter_state_and_follow_epsilon_transitions (ctx, init, tq, 1);
      /* since init is meant to be launched only once, we do not enqueue it,
	 and clean it up. */
      cleanup_state_instance (init);
    }
  GC_END (gc_ctx);
}

static void engine_activity_monitor (orchids_t *ctx, state_instance_t *si)
{
  thread_group_t *pid;
  unsigned long code;

  if (si==NULL)
    fputc ('|', stderr);
  else
    {
      pid = si->pid;
      code = (unsigned long)pid;
      code >>= 4;
      code %= 26;
      fputc ('A'+code, stderr);
    }
}

void inject_event(orchids_t *ctx, event_t *event)
{
  event_t *e;
  field_record_t *fields;
  gc_t *gc_ctx = ctx->gc_ctx;
  thread_queue_t *new_queue, *unsorted, *next, *old_queue;
  state_instance_t *si;
  int vmret;

  GC_TOUCH (gc_ctx, ctx->current_event = event);
  ctx->events++;
  execute_pre_inject_hooks (ctx, event);
  /* Between two calls to inject_event(),
     ctx->global_fields->fields[i].val==NULL
     for every i.
     We just store each field of the event into the fields[] array
     and we shall set the latter entries to NULL on exit.
  */
  fields = ctx->global_fields->fields;
  for (e = event; e!=NULL; e = e->next)
    GC_TOUCH (gc_ctx, fields[e->field_id].val = e->value);

  GC_START (gc_ctx, 4);
  new_queue = new_thread_queue (gc_ctx);
  GC_UPDATE (gc_ctx, 0, new_queue);
  unsorted = new_thread_queue (gc_ctx);
  GC_UPDATE (gc_ctx, 1, unsorted);
  next = new_thread_queue (gc_ctx);
  GC_UPDATE (gc_ctx, 2, next);

#define BUMP() do {						   \
    thread_enqueue_all (gc_ctx, new_queue, unsorted);		   \
    thread_enqueue (gc_ctx, new_queue, NULL); /* enqueue 'BUMP' */ \
    thread_enqueue_all (gc_ctx, new_queue, next);		   \
    thread_enqueue (gc_ctx, new_queue, NULL); /* enqueue 'BUMP' */ \
  } while(0)

  old_queue = ctx->thread_queue;
  if (old_queue!=NULL)
    {
      while (!THREAD_QUEUE_IS_EMPTY(old_queue))
	{
	  si = thread_dequeue (gc_ctx, old_queue);
	  GC_UPDATE (gc_ctx, 3, si);
	  if (ctx->verbose>=2)
	    engine_activity_monitor (ctx, si);
	  if (si==NULL) /* This is a BUMP */
	    BUMP();
	  else if (si->pid->flags & THREAD_KILL)
	    { /* kill thread */
	      cleanup_state_instance (si);
	    }
	  else
	    {
	      vmret = simulate_state_and_create_threads (ctx, si, unsorted);
	      /* If si->t->flags does not have the TRANS_NO_WAIT flag set (normal case):
		 - if vmret=1, we succeeded, and we must wait (i.e., reenqueue si)
		 - if vmret=0, we failed, and we must wait (reenqueue si)
		 - if vmret=-1, we failed and will fail forever (don't reenqueue; hence cleanup si)
		 If si->t->flags has the TRANS_NO_WAIT flag set:
		 - if vmret=1, we succeeded, hence we don't have to wait (cleanup si)
		 - if vmret=0, we failed, and we must wait (reenqueue si)
		 - if vmret=-1, we failed and will fail forever (cleanup si).
		 In other words, we reenqueue if vmret=0, or if vmret=1 and the TRANS_NO_WAIT
		 flag is not set.  Equivalently, if:
		 - the TRANS_NO_WAIT flags is set and vmret==0
		 - or it is not set and vmret>=0
	      */
	      if ((si->t->flags & TRANS_NO_WAIT)?(vmret==0):(vmret>=0))
		thread_enqueue (gc_ctx, next, si);
	      else cleanup_state_instance (si); /* We succeeded: no need to wait. */
	    }
	}
    }
  BUMP();
  create_rule_initial_threads (ctx, new_queue);
  BUMP();

  GC_TOUCH (gc_ctx, ctx->thread_queue = new_queue);
  GC_END (gc_ctx);

  execute_post_inject_hooks(ctx, event);
  /* Set back the entries of the fields[] array to NULL. */
  for (e = event; e!=NULL; e = e->next)
    fields[e->field_id].val = NULL;
  if (ctx->verbose>=2)
    fputc ('\n', stderr);
}

uint16_t create_fresh_handle (gc_t *gc_ctx, state_instance_t *si, ovm_var_t *val)
{
  uint16_t k, n;
  unsigned long kvar;
  ovm_var_t *storedval;
  ovm_var_t *env, *newenv;

  n = si->nhandles;
  if (n>MAX_HANDLES)
    n = MAX_HANDLES;
  env = si->env;
  for (k=0; k<n; k++)
    {
      kvar = HANDLE_FAKE_VAR(k);
      storedval = ovm_read_value (env, kvar);
      if (storedval==NULL)
	{ /* found a slot */
	  si->owned_handles |= (1 << k);
	  newenv = ovm_write_value (gc_ctx, env, kvar, val);
	  GC_TOUCH (gc_ctx, si->env = newenv);
	  return k;
	}
    }
  if (n<MAX_HANDLES)
    si->owned_handles |= (1 << n);
  si->nhandles = n+1;
  kvar = HANDLE_FAKE_VAR(n);
  newenv = ovm_write_value (gc_ctx, env, kvar, val);
  GC_TOUCH (gc_ctx, si->env = newenv);
  return n;
}

int release_handle (gc_t *gc_ctx, state_instance_t *si, uint16_t k)
{
  uint16_t n;
  ovm_var_t *env;

  if (k<MAX_HANDLES)
    si->owned_handles ^= ~(1 << k); /* we no longer own the handle */
  n = si->nhandles;
  if (k==n-1)
    si->nhandles--;
  env = ovm_release_value (gc_ctx, si->env, HANDLE_FAKE_VAR(k));
  GC_TOUCH (gc_ctx, si->env = env);
  return 0;
}

ovm_var_t *handle_get_rd (gc_t *gc_ctx, state_instance_t *si, uint16_t k)
{
  return ovm_read_value (si->env, HANDLE_FAKE_VAR(k));
}

ovm_var_t *handle_get_wr (gc_t *gc_ctx, state_instance_t *si, uint16_t k)
{
  uint16_t n;
  ovm_var_t *val;
  ovm_var_t *env;

  if (k>=MAX_HANDLES) /* value has already been copied, so return it right away */
    return ovm_read_value (si->env, HANDLE_FAKE_VAR(k));
  n = si->nhandles;
  if (k>=n)
    return NULL;
  val = ovm_read_value (si->env, HANDLE_FAKE_VAR(k));
  if (val==NULL)
    return NULL;
  if (si->owned_handles & (1 << k)) /* I own it: fine */
    return val;
  /* Otherwise, make a copy: */
  GC_START (gc_ctx, 1);
  val = issdl_clone (gc_ctx, val);
  GC_UPDATE (gc_ctx, 0, val);
  env = ovm_write_value (gc_ctx, si->env, HANDLE_FAKE_VAR(k), val);
  GC_TOUCH (gc_ctx, si->env = env);
  GC_END(gc_ctx);
  return val;
}

/*
** Copyright (c) 2002-2005 by Julien OLIVAIN, Laboratoire Spécification
** et Vérification (LSV), CNRS UMR 8643 & ENS Cachan.
** Copyright (c) 2013-2015 by Jean GOUBAULT-LARRECQ, Laboratoire Spécification
** et Vérification (LSV), CNRS UMR 8643 & ENS Cachan.
**
** Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
** Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
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
