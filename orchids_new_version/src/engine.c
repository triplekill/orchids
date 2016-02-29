/**
 ** @file engine.c
 ** Analysis engine.
 **
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
#include <errno.h>

#include "orchids.h"
#include "lang.h"
#include "orchids_api.h"

#include "engine.h"

/* In lang.c: */
extern void fprintf_env (FILE *fp, const orchids_t *ctx, rule_t *rule, ovm_var_t *env,
			 ovm_var_t *oldenv, char *header, char *trailer);

static void thread_group_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  thread_group_t *pid = (thread_group_t *)p;

  GC_TOUCH (gc_ctx, pid->rule);
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

static int thread_group_save (save_ctx_t *sctx, gc_header_t *p)
{
  thread_group_t *pid = (thread_group_t *)p;
  int err;

  err = save_gc_struct (sctx, (gc_header_t *)pid->rule);
  if (err) return err;
  err = save_size_t (sctx, pid->nsi);
  if (err) return err;
  err = save_uint32 (sctx, pid->flags.i);
  return err;
}

gc_class_t thread_group_class;

static gc_header_t *thread_group_restore (restore_ctx_t *rctx)
{
  gc_t *gc_ctx = rctx->gc_ctx;
  rule_t *rule;
  thread_group_t *pid;
  size_t nsi;
  uint32_t flags;
  int err;

  GC_START (gc_ctx, 1);
  pid = NULL;
  rule = (rule_t *)restore_gc_struct (rctx);
  if (rule==NULL && errno!=0)
    goto end;
  GC_UPDATE (gc_ctx, 0, rule);
  err = restore_size_t (rctx, &nsi);
  if (err) { errno = err; goto end; }
  err = restore_uint32 (rctx, &flags);
  if (err) { errno = err; goto end; }
  pid = gc_alloc (gc_ctx, sizeof(thread_group_t), &thread_group_class);
  pid->gc.type = T_THREAD_GROUP;
  GC_TOUCH (gc_ctx, pid->rule = rule);
  pid->nsi = nsi;
  pid->flags.i = flags;
 end:
  GC_END (gc_ctx);
  return (gc_header_t *)pid;
}

gc_class_t thread_group_class = {
  GC_ID('p','i','d',' '),
  thread_group_mark_subfields,
  NULL,
  thread_group_traverse,
  thread_group_save,
  thread_group_restore
};

static void state_instance_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  state_instance_t *si = (state_instance_t *)p;

  GC_TOUCH (gc_ctx, si->pid);
  GC_TOUCH (gc_ctx, si->env);
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

static int state_instance_save (save_ctx_t *sctx, gc_header_t *p)
{
  state_instance_t *si = (state_instance_t *)p;
  long stateno, transno;
  uint32_t nhandles;
  int err;

  err = save_gc_struct (sctx, (gc_header_t *)si->pid);
  if (err) return err;
  stateno = si->q - si->pid->rule->state;
  err = save_long (sctx, stateno);
  if (err) return err;
  if (si->t==NULL)
    transno = -1;
  else
    transno = si->t - si->q->trans;
  err = save_long (sctx, transno);
  if (err) return err;
  err = save_gc_struct (sctx, (gc_header_t *)si->env);
  if (err) return err;
  err = save_uint32 (sctx, si->owned_handles); /* requires handle_bitmask_t==uint32_t */
  if (err) return err;
  nhandles = si->nhandles; /* convert from uint16_t to uint32_t,
			      to avoid creating a save_uint16_t() function */
  err = save_uint32 (sctx, nhandles);
  if (err) return err;
  err = save_uint32 (sctx, (uint32_t)si->age);
  return err;
}

gc_class_t state_instance_class;

gc_header_t *state_instance_restore (restore_ctx_t *rctx)
{
  gc_t *gc_ctx = rctx->gc_ctx;
  thread_group_t *pid;
  state_instance_t *si;
  long stateno, transno;
  state_t *q;
  transition_t *t;
  ovm_var_t *env;
  uint32_t owned_handles, nhandles, age;
  int err;

  GC_START (gc_ctx, 2);
  si = NULL;
  pid = (thread_group_t *)restore_gc_struct (rctx);
  if (pid==NULL && errno!=0)
    goto end;
  if (pid==NULL || TYPE(pid)!=T_THREAD_GROUP)
    { errno = -2; goto end; }
  GC_UPDATE (gc_ctx, 0, pid);
  err = restore_long (rctx, &stateno);
  if (err) { errno = err; goto end; }
  if (stateno<0 || stateno>=pid->rule->state_nb)
    { errno = -3; goto end; }
  q = &pid->rule->state[stateno];
  err = restore_long (rctx, &transno);
  if (err) { errno = err; goto end; }
  if (transno==-1)
    t = NULL;
  else if (transno<0 || transno>=q->trans_nb)
    { errno = -3; goto end; }
  else t = &q->trans[transno];
  env = (ovm_var_t *)restore_gc_struct (rctx);
  if (errno) goto end;
  if (env!=NULL && TYPE(env)!=T_BIND && TYPE(env)!=T_SPLIT)
    { errno = -2; goto end; }
  GC_UPDATE (gc_ctx, 1, env);
  err = restore_uint32 (rctx, &owned_handles);
  if (err) { errno = err; goto end; }
  err = restore_uint32 (rctx, &nhandles);
  if (err) { errno = err; goto end; }
  err = restore_uint32 (rctx, &age);
  if (err) { errno = err; goto end; }
  si = gc_alloc (gc_ctx, sizeof(state_instance_t), &state_instance_class);
  si->gc.type = T_STATE_INSTANCE;
  GC_TOUCH (gc_ctx, si->pid = pid);
  si->q = q;
  si->t = t;
  GC_TOUCH (gc_ctx, si->env = env);
  si->owned_handles = owned_handles;
  si->nhandles = nhandles;
  si->age = age;
  si->dummy = 0;
 end:
  GC_END (gc_ctx);
  return (gc_header_t *)si;
}

gc_class_t state_instance_class = {
  GC_ID('s','t','t','i'),
  state_instance_mark_subfields,
  NULL,
  state_instance_traverse,
  state_instance_save,
  state_instance_restore
};

static void thread_queue_elt_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  thread_queue_elt_t *qe = (thread_queue_elt_t *)p;

  GC_TOUCH (gc_ctx, qe->thread);
  GC_TOUCH (gc_ctx, qe->next);
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

static int thread_queue_elt_save (save_ctx_t *sctx, gc_header_t *p)
{
  thread_queue_elt_t *qe = (thread_queue_elt_t *)p;
  int err;

  err = save_gc_struct (sctx, (gc_header_t *)qe->thread);
  if (err) return err;
  err = save_gc_struct (sctx, (gc_header_t *)qe->next);
  return err;
}

gc_class_t thread_queue_elt_class;

static void cleanup_state_instance (state_instance_t *si)
/* Cleanup state_instance before we throw it away. */
{ /* if state_instance was synchronized, then remove its entry from
     the table of synchronized state_instances. */
  struct thread_group_s *pid;

  pid = si->pid;
  if (SI_AGE(si)==THREAD_AGE(pid)) /* si is live */
    {
      --pid->nsi; /* number of live state_instances decreases */
      if (pid->nsi==0 && (THREAD_FLAGS(pid) & THREAD_LOCK))
	{
	  (void) objhash_del (pid->rule->sync_lock, si);
	  THREAD_FLAGS(pid) &= ~THREAD_LOCK;
	}
    }
}

static gc_header_t *thread_queue_elt_restore (restore_ctx_t *rctx)
{
  gc_t *gc_ctx = rctx->gc_ctx;
  thread_queue_elt_t *qe, *next;
  state_instance_t *si;

  GC_START (gc_ctx, 2);
  qe = NULL;
  si = (state_instance_t *)restore_gc_struct (rctx);
  if (si==NULL && errno!=0)
    goto end;
  if (si!=NULL && TYPE(si)!=T_STATE_INSTANCE)
    { errno = -2; goto end; }
  if (si!=NULL &&
      (si->pid->rule->flags &
       (RULE_RESTORE_UNKNOWN_FIELD_NAME | RULE_RESTORE_UNKNOWN_PRIMITIVE))
      !=0)
    { /* Kill those threads (and rules) that cannot be run under the current
	 configuration */
      cleanup_state_instance (si);
      rctx->errs = 0;
    }
  GC_UPDATE (gc_ctx, 0, si);
  next = (thread_queue_elt_t *)restore_gc_struct (rctx);
  if (next==NULL && errno!=0)
    goto end;
  if (next!=NULL && TYPE(next)!=T_THREAD_QUEUE_ELT)
    { errno = -2; goto end; }
  GC_UPDATE (gc_ctx, 1, next);
  qe = gc_alloc (gc_ctx, sizeof(thread_queue_elt_t), &thread_queue_elt_class);
  qe->gc.type = T_THREAD_QUEUE_ELT;
  GC_TOUCH (gc_ctx, qe->next = next);
  GC_TOUCH (gc_ctx, qe->thread = si);
 end:
  GC_END (gc_ctx);
  return (gc_header_t *)qe;
}

gc_class_t thread_queue_elt_class = {
  GC_ID('t','h','q','e'),
  thread_queue_elt_mark_subfields,
  NULL,
  thread_queue_elt_traverse,
  thread_queue_elt_save,
  thread_queue_elt_restore
};

static void thread_queue_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  thread_queue_t *tq = (thread_queue_t *)p;

  GC_TOUCH (gc_ctx, tq->first);
  GC_TOUCH (gc_ctx, tq->last);
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

static int thread_queue_save (save_ctx_t *sctx, gc_header_t *p)
{
  thread_queue_t *tq = (thread_queue_t *)p;
  int err;

  err = save_gc_struct (sctx, (gc_header_t *)tq->first);
  /* don't save nelts or last, which will be recomputed
     at restoration time */
  return err;
}

gc_class_t thread_queue_class;

static gc_header_t *thread_queue_restore (restore_ctx_t *rctx)
{
  gc_t *gc_ctx = rctx->gc_ctx;
  thread_queue_t *tq;
  thread_queue_elt_t *qe, *last, *next;
  size_t n;

  GC_START (gc_ctx, 1);
  tq = NULL;
  qe = (thread_queue_elt_t *)restore_gc_struct (rctx);
  if (qe==NULL && errno!=0)
    goto end;
  if (qe!=NULL && TYPE(qe)!=T_THREAD_QUEUE_ELT)
    { errno = -2; goto end; }
  GC_UPDATE (gc_ctx, 0, qe);
  if (qe==NULL)
    {
      last = NULL;
      n = 0;
    }
  else for (n=1, last=qe; (next = last->next)!=NULL; last=next);
  tq = gc_alloc (gc_ctx, sizeof(thread_queue_t), &thread_queue_class);
  tq->gc.type = T_THREAD_QUEUE;
  GC_TOUCH (gc_ctx, tq->first = qe);
  GC_TOUCH (gc_ctx, tq->last = last);
  tq->nelts = n;
 end:
  GC_END (gc_ctx);
  return (gc_header_t *)tq;
}

gc_class_t thread_queue_class = {
  GC_ID('t','h','q','.'),
  thread_queue_mark_subfields,
  NULL,
  thread_queue_traverse,
  thread_queue_save,
  thread_queue_restore
};

static thread_queue_t *new_thread_queue (gc_t *gc_ctx)
{
  thread_queue_t *tq;

  tq = gc_alloc (gc_ctx, sizeof (thread_queue_t), &thread_queue_class);
  tq->gc.type = T_THREAD_QUEUE;
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
  qe->gc.type = T_THREAD_QUEUE_ELT;
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

struct rule_size_s {
  size_t occupied;
  unsigned long occurs;
  unsigned long allowed;
  rule_t *rule;
};

void emergency_drop_threads (orchids_t *ctx)
{
  gc_t *gc_ctx = ctx->gc_ctx;
  unsigned long needed;
  thread_queue_t *queue;
  thread_queue_elt_t *qe, *last;
  state_instance_t *si;
  thread_group_t *pid;
  rule_t *rule;
  struct rule_size_s *rule_sizes, *rs;
  size_t total_rule_sizes;
  int i, n;
  int drop;

#define SI_SIZE_ESTIMATE (sizeof(thread_queue_elt_t) + sizeof(state_instance_t))
  
  needed = GC_CRITICAL (gc_ctx);
  if (needed==0)
    return;
  DebugLog (DF_ENG, DS_CRIT,
	    "Emergency: need %lu bytes (taken from rainy day fund for now), try to free threads (%lu bytes each).\n",
	    SI_SIZE_ESTIMATE);

  n = ctx->rule_compiler->rules;
  rule_sizes = gc_base_malloc (gc_ctx, n * sizeof(struct rule_size_s));
  for (i=0; i<n; i++)
    {
      rs = &rule_sizes[i];
      rs->occupied = 0;
      rs->occurs = 0;
      rs->allowed = ULONG_MAX;
      rs->rule = NULL;
    }
  queue = ctx->thread_queue;
  /* Now estimate size taken by each rule */
  for (qe = queue->first; qe!=NULL; qe=qe->next)
    {
      si = qe->thread;
      if (si==NULL)
	continue;
      pid = si->pid;
      rule = pid->rule;
      if (rule->complexity_degree==0)
	continue; /* do not count rules with complexity degree 0: they are benign */
      i = rule->id;
      if (i>=0 && i<n)
	{
	  rs = &rule_sizes[i];
	  rs->occupied += SI_SIZE_ESTIMATE;
	  rs->rule = rule;
	}
      /* This is conservative (too low): ignores sizes of environments (si->env),
	 of thread_groups (si->pid), and also of overhead data structures used by
	 the malloc()/free() algorithm itself */
    }
  total_rule_sizes = 0;
  for (i=0; i<n; i++)
    total_rule_sizes += rule_sizes[i].occupied;
  /* We need to free roughly needed/total_rule_sizes threads (this is an upper bound).
     We divide that evenly among all rules with complexity degree != 0.
   */
  if (total_rule_sizes==0) /* If cannot free anything: quit (we are apparently running with
			      incredibly little memory)
			   */
    goto end;
  for (i=0; i<n; i++)
    {
      rs = &rule_sizes[i];
      /* p = rs->occupied/total_rule_sizes is the relative weight of this rule;
	 it should free p*needed bytes, hence it can keep rs->occupied - p*needed
	 = rs->occupied*(total_rule_sizes-needed)/total_rule_sizes bytes.
       */
      rs->allowed = rs->occupied*(total_rule_sizes-needed)/total_rule_sizes;
      if (rs->allowed < 128*SI_SIZE_ESTIMATE) /* keep at least 128 entries min
						 (128 is pretty random).
						 This may counter the purpose of
						 the whole rainy day fund scheme,
						 but 128 is not much, and I would
						 like to avoid rs->allowed to drop to
						 too small a value because of the underestimation of
						 rule_sizes[i].occupied (see above).
					       */
	rs->allowed = 128*SI_SIZE_ESTIMATE;
      if (rs->occupied > rs->allowed)
	{
	  DebugLog(DF_ENG, DS_CRIT, "Emergency: will drop %lu threads running rule %s, %lu remaining.\n",
		   (rs->occupied - rs->allowed)/SI_SIZE_ESTIMATE, rs->rule->name,
		   rs->allowed/SI_SIZE_ESTIMATE);
	}
    }
  last = NULL;
  for (qe = queue->first; qe!=NULL; qe=qe->next)
    {
      si = qe->thread;
      if (si==NULL)
	{
	  if (last!=NULL && last->thread==NULL)
	    { /* If both last and si are BUMPs, remove the second one. */
	      GC_TOUCH (gc_ctx, last->next = qe->next);
	      // queue->nelts--;  No, only non-BUMP threads are counted
	    }
	  else
	    last = qe; /* keep BUMP threads, unless previous one
			  was already a BUMP */
	  continue;
	}
      pid = si->pid;
      rule = pid->rule;
      i = rule->id;
      drop = 0;
      if (i>=0 && i<n)
	{
	  rs = &rule_sizes[i];
	  rs->occurs += SI_SIZE_ESTIMATE;
	  if (rs->occurs>=rs->allowed)
	    drop = 1;
	}
      if (drop)
	{
	  if (last==NULL)
	    {
	      GC_TOUCH (gc_ctx, queue->first = qe->next);
	    }
	  else
	    {
	      GC_TOUCH (gc_ctx, last->next = qe->next);
	    }
	  rule->instances--;
	  queue->nelts--;
	  cleanup_state_instance (si);
	}
      else last = qe;
    }
  GC_TOUCH (gc_ctx, queue->last = last);
  gc_full (gc_ctx);
  gc_full (gc_ctx);
  gc_recuperate (gc_ctx);
 end:
  gc_base_free (rule_sizes);
}

#ifdef OBSOLETE
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
#endif


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
						uint16_t nhandles,
						unsigned char age)
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
  SI_AGE(newsi) = age;
  return newsi;
}

static void enter_state_and_follow_epsilon_transitions (orchids_t *ctx,
							state_instance_t *si,
							thread_queue_t *tq,
							int only_once)
/* Here si->q is the (new) state we just moved to;
   si->t is temporarily meaningless;
   as an invariant, we shall need to thread_enqueue(si) or to cleanup_instance(si),
   and this not only for the given si, but also for every si we create. */
{
  size_t i, trans_nb;
  state_instance_t *newsi;
  transition_t *trans;
  objhash_t *lock_table;
  struct thread_group_s *sync_lock;
  state_t *q;
  gc_t *gc_ctx = ctx->gc_ctx;

  GC_START (gc_ctx, 1);
 again:
  q = si->q;
  if (q->flags & STATE_COMMIT)
    {
      if (si->pid->nsi==1)
	; /* Optimization: if only one live state instance has this pid (namely, ourselves: si),
	     then no need to kill the other state instances with the same pid
	     and to detach.
	  */
      else /*  Otherwise, we kill all threads in our group except ourselves.
	    */
	{
	  THREAD_AGE(si->pid) = ++SI_AGE(si);
	  /* This is done by incrementing si's age, and setting its pid's
	     age to the same thing: so only si is now live with that pid.
	  */
	  si->pid->nsi = 1;
	}
    }
  if (q->action!=NULL)
    {
      ovm_exec_stmt (ctx, si, q->action);
      /* This is the unique place where synchronization may take place, when
	 executing code inside states.  Indeed, synchronization can only
	 happen when variables change.  The call to ovm_exec_expr() done
	 when evaluating 'expect' clauses cannot change any variable. */
      if (THREAD_FLAGS(si->pid) & THREAD_LOCK)
	; /* already locked */
      else if (q->flags & STATE_SYNCVARS_ALL_SYNC) /* we are in a state where synchronization
						      can occur */
	{
	  /* Before Feb 19, 2016, we checked whether the environment actually changed
	     (si->env!=oldenv), but that is not needed: by definition of AN_SYNCVARS_ALL_SYNC,
	     we have just entered the (rather, a) first state where all the synchronization
	     variables are defined.
	     We were also checking whether all the synchronization variables
	     are defined (sync_var_env_is_defined (si)), but that is useless as well.
	     Finally, we were also checking whether the rule is actually synchronized
	     (lock_table!=NULL), but again this will always be the case.
	  */
	  lock_table = si->pid->rule->sync_lock;
	  sync_lock = objhash_get (lock_table, si);
	  if (sync_lock!=NULL)
	    { /* Somebody already holds the lock. */
	      /* We should be in a commit state, as checked by check_sync_same_commit()
		 in rule_compiler.c; we don't both to check this here.
		 Also, all synchronization states are commit states, too:
		 then kill ourselves: some other thread is already doing the job. */
		  cleanup_state_instance (si);
		  goto end; /* and that's all */
	    }
	  else
	    { /* create a lock. */
	      THREAD_FLAGS(si->pid) |= THREAD_LOCK;
	      // GC_TOUCH (gc_ctx, si->pid); /* useless: objhash_add() will do it */
	      objhash_add (gc_ctx, lock_table, si->pid, si);
	    }
	}
    }
  trans_nb = q->trans_nb;
  if (q->flags & STATE_EPSILON) /* follow the first epsilon transition that matches;
				 there must be at least one, as guaranteed by syntax
				 (and in state_restore()).
				*/
    for (i=0, trans=q->trans; i<trans_nb; i++, trans++)
      {
	if (trans->eval_code==NULL ||
	    ovm_exec_trans_cond (ctx, si, trans->eval_code)>0)
	  {
	    si->q = trans->dest;
	    si->t = trans;
	    goto again;
	    /* The above eliminates a terminal recursive call:
	       enter_state_and_follow_epsilon_transitions (ctx, si, tq, only_once);
	       break;
	    */
	  }
      }
  else if (only_once)
    switch (trans_nb)
      {
      case 1: /* optimization again; the general case is 'default:' */
	trans = q->trans;
	if (trans->eval_code==NULL ||
	    ovm_exec_trans_cond (ctx, si, trans->eval_code)>0)
	  {
	    si->q = trans->dest;
	    si->t = trans;
	    only_once = 0;
	    goto again;
	  }
	/*fallthrough*/
      case 0: cleanup_state_instance (si); break; /* optimization */
      default:
	for (i=0, trans=q->trans; i<trans_nb; i++, trans++)
	  { /* ordinary transition ('expect (<cond>) goto <state>'),
	       and flag only_once is set: try to match transition; if
	       there is a match, create a fresh state_instance newsi (i.e., fork
	       a new thread);
	       finally cleanup_state_instance(si) since si is not reenqueued. */
	    if (trans->eval_code==NULL ||
		ovm_exec_trans_cond (ctx, si, trans->eval_code)>0)
	      {
		newsi = create_state_instance (ctx, si->pid, trans->dest, trans,
					       si->env, si->nhandles, SI_AGE(si));
		GC_UPDATE (gc_ctx, 0, newsi);
		/* now we call ourselves back with only_once false */
		enter_state_and_follow_epsilon_transitions (ctx, newsi, tq, 0);
	      }
	  }
	cleanup_state_instance (si);
	break;
      }
  else
    {
      for (i=0, trans=q->trans; i<trans_nb; i++, trans++)
	{ /* ordinary transition ('expect (<cond>) goto <state>'):
	     fork a new thread on the queue, for each outgoing transition;
	     then cleanup_state_instance(si) since si is not reenqueued. */
	  newsi = create_state_instance (ctx, si->pid, q, trans, si->env, si->nhandles, SI_AGE(si));
	  GC_UPDATE (gc_ctx, 0, newsi);
	  thread_enqueue (gc_ctx, tq, newsi);
	}
      cleanup_state_instance (si);
    }
 end:
  GC_END (gc_ctx);
}

static void enter_state_and_follow_epsilon_transitions_then_reenqueue (orchids_t *ctx,
								       state_instance_t *si,
								       thread_queue_t *tq,
								       int only_once)
{
  gc_t *gc_ctx = ctx->gc_ctx;
  state_instance_t *newsi;

  GC_START (gc_ctx, 1);
  newsi = create_state_instance (ctx, si->pid, si->t->dest /* go to successor state */,
				 si->t /* temporarily wrong */,
				 si->env, si->nhandles, SI_AGE(si));
  GC_UPDATE (gc_ctx, 0, newsi);
  enter_state_and_follow_epsilon_transitions (ctx, newsi, tq, 0);
  thread_enqueue (gc_ctx, tq, si); /* and enqueue old si, which remains there waiting */
  GC_END (gc_ctx);
}

#define SSCT_DECL							\
  transition_t *t; /* t should not be NULL, and should not be an epsilon transition */ \
  int vmret								\
  	      /* Computing reenqueue:
		 If si->t->flags does not have the TRANS_NO_WAIT flag set (normal case):
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
#define SSCT_TRANS_COND					\
  t = si->t;							\
  vmret = 1;							\
  if (t->eval_code!=NULL)					\
    vmret = ovm_exec_trans_cond (ctx, si, t->eval_code)
  /*reenqueue = (si->t->flags & TRANS_NO_WAIT)?(vmret==0):(vmret>=0)*/

#define SSCT_GO	do {	 /* OK (vmret=1): pass transition */			\
    if ((si->t->flags & TRANS_NO_WAIT) == 0)				\
      enter_state_and_follow_epsilon_transitions_then_reenqueue (ctx, si, tq, 0); \
    else {								\
      si->q = t->dest; /* move to successor directly */			\
      enter_state_and_follow_epsilon_transitions (ctx, si, tq, 0);	\
    }									\
  } while (0)

#define SSCT_FAIL do { /* not OK: wait or cleanup, depending on reenqueue */ \
    if (vmret==0)							\
      thread_enqueue (ctx->gc_ctx, tq, si);				\
    else cleanup_state_instance (si);					\
  } while (0)

static int simulate_state_and_create_threads (orchids_t *ctx,
					      state_instance_t *si,
					      thread_queue_t *tq)
{
  SSCT_DECL;

  SSCT_TRANS_COND;
  if (vmret > 0)
    SSCT_GO;
  else
    SSCT_FAIL;
  return vmret;
}

static char pid_codes[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz123456789";

static char pid_code (thread_group_t *pid)
{
  unsigned long code;

  code = (unsigned long)pid;
  code %= 61;
  return pid_codes[code];
}

static int simulate_state_and_create_threads_verbose (orchids_t *ctx,
						      state_instance_t *si,
						      thread_queue_t *tq)
{
  SSCT_DECL;
  thread_group_t *oldpid;

  GC_START (ctx->gc_ctx, 1);
  SSCT_TRANS_COND;
  if (vmret > 0)
    { /* OK: pass transition */
      fprintf (stderr, "* %s", si->q->rule->name);
      oldpid = si->pid;
      if (ctx->verbose>=3)
	fprintf (stderr, "[grp=%c]", pid_code (oldpid));
      fprintf (stderr, ": %s -#%d->", si->q->name, t->id);
      GC_UPDATE (ctx->gc_ctx, 0, si->env);
      
      SSCT_GO;
      
      if (si->q!=t->dest)
	fprintf (stderr, " %s ~>", t->dest->name);
      fprintf (stderr, " %s", si->q->name);
      if (ctx->verbose>=3 && si->pid!=oldpid)
	fprintf (stderr, " [grp:=%c]\n", pid_code(si->pid));
      else fprintf (stderr, "\n");
      fprintf_env (stderr, ctx, si->q->rule, si->env, (ovm_var_t *)GC_LOOKUP(0),
		   "  - ", "\n");
      fflush (stderr);
     }
  else
    SSCT_FAIL;
  GC_END (ctx->gc_ctx);
  return vmret;
}

#define SIMULATE_STATE_AND_CREATE_THREADS(ctx,si,tq) (((ctx)->verbose>=2)? \
						      simulate_state_and_create_threads_verbose(ctx,si,tq): \
						      simulate_state_and_create_threads(ctx,si,tq))

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
      pid->gc.type = T_THREAD_GROUP;
      GC_TOUCH (gc_ctx, pid->rule = r);
      pid->nsi = 0;
      pid->flags.i = 0;
      GC_UPDATE (gc_ctx, 0, pid);
      q = &r->state[0];
      if (q->flags & STATE_COMMIT) /* 'anchored' rule */
	r->flags |= RULE_INITIAL_ALREADY_LAUNCHED;
      init = create_state_instance (ctx, pid, q, NULL, NULL, 0, 0);
      /* create state instance, in state q, waiting for no transition yet (NULL),
	 and with empty environment (NULL) */
      GC_UPDATE (gc_ctx, 0, init);
      enter_state_and_follow_epsilon_transitions (ctx, init, tq, 1);
    }
  GC_END (gc_ctx);
}

#define MONITOR_MAXBUF 80

struct engine_actmon_s {
  char buf[MONITOR_MAXBUF];
  char *cur;
  unsigned long excess;
};

static void engine_activity_monitor (orchids_t *ctx, state_instance_t *si, struct engine_actmon_s *monp)
{
  if (monp->cur >= monp->buf+MONITOR_MAXBUF-1)
    {
      monp->excess++;
      return; /* overflow */
    }
  if (si==NULL)
    *monp->cur++ = '|';
  else
    *monp->cur++ = pid_code (si->pid);
}

static void collect_actmon (orchids_t *ctx, struct engine_actmon_s *monp)
{
  thread_queue_elt_t *qe = ctx->thread_queue->first;

  for (; qe!=NULL; qe=qe->next)
    engine_activity_monitor (ctx, qe->thread, monp);
}

static void fprintf_actmon (FILE *f, struct engine_actmon_s *monp)
{
  unsigned long exc, bound;
  unsigned long digits;
  
  if (monp->cur >= monp->buf+MONITOR_MAXBUF-1)
    {
      exc = monp->excess+4; /* 3 characters for [+ and ], plus at least one for a digit */
      for (digits=1, bound=9; exc>bound; digits++)
	bound = 10*bound + 9*digits-1; /* = 10^digits - digits:
					digits: 1  2   3    4 ...
					bound:  9 98 997 9996 ...
				       */
      /* If exc<=9, digits:=1
	 If 10<=exc<=98, digits:=2
	 If 99<=exc<=997, digits:=3
	 etc.
	 Now print [+n] where n = excess+digits+3
	 Examples (with MONITOR_MAXBUF==80; length of line should be at most 79):
	 - if excess==1 (80 characters total), exc==5, digits=1, print [+5] at position 75
	 - if excess==2 (81 characters total), exc==6, digits=1, print [+6] at position 75
	 - if excess==5 (84 characters total), exc==9, digits==1, print [+9] at position 75
	 - if excess==6 (85 characters total), exc==10, digits==2, print [+11] at position 74
	 - if excess==94 (173 characters total), exc==98, digits==2, print [+99] at position 74
	 - if excess==95 (174 characters total), exc==99, digits==3, print [+101] at position 73
	 - etc.
      */
      if (digits<=MONITOR_MAXBUF-4)
	{
	  sprintf (&monp->buf[MONITOR_MAXBUF-4-digits], "[+%lu]", exc+digits-1);
	}
      else /* even [+n] message is too long: hope MONITOR_MAXBUF>=6 and print [...] */
	{
	  strcpy (&monp->buf[MONITOR_MAXBUF-6], "[...]");
	}
      //monp->buf[MONITOR_MAXBUF-1] = '\0';
      fprintf (f, "%s\n", monp->buf);
    }
  else
    {
      *monp->cur = '\0';
      fprintf (f, "%s\n", monp->buf);
    }
  fflush (f);
}

void inject_event(orchids_t *ctx, event_t *event)
{
  event_t *e;
  field_record_t *fields;
  gc_t *gc_ctx = ctx->gc_ctx;
  thread_queue_t *new_queue, *unsorted, *next, *old_queue;
  state_instance_t *si;
  struct engine_actmon_s actmon;
  thread_group_t *pid;

  GC_TOUCH (gc_ctx, ctx->current_event = event);
  ctx->events++;
  actmon.cur = actmon.buf;
  actmon.excess = 0;
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
	  if (si==NULL) /* This is a BUMP */
	    {
	      BUMP();
	      continue;
	    }
	  pid = si->pid;
	  if (SI_AGE(si)!=THREAD_AGE(pid))
	    continue; /* This si is killed.  No need to cleanup_instance(): this would not do
			 anything anyway. */
	  (void) SIMULATE_STATE_AND_CREATE_THREADS (ctx, si, unsorted);
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
  if (ctx->verbose>=3)
    {
      collect_actmon (ctx, &actmon);
      fprintf_actmon (stderr, &actmon);
    }
  if ((GC_CRITICAL(gc_ctx)<<2) > gc_ctx->gc_rainy_day_goal)
    emergency_drop_threads (ctx);
    /* Drop threads to get back some memory, in critically low memory situations.
       We will try to get back at least needed = GC_CRITICAL(gc_ctx) bytes.
       We only try to do so provided needed<<2 > rainy_day_goal,
       i.e., provided we needed at least 1/4 of the rainy_day_goal.
       E.g., if rainy_day_goal = 16Mb, we shall start to drop threads only when
       we have consumed at least 4Mb of the rainy day fund (and 12Mb are still remaining).
       The constant 1/4 is essentially random.
    */
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
