/**
 ** @file engine.c
 ** Analysis engine.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup engine
 **
 ** @date  Started on: Fri Feb 21 16:18:12 2003
 ** @date Last update: Tue Nov 29 11:14:53 2005
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

/* WARNING -- Field list in event_t, and field ids in int array must
   be sorted in decreasing order */

/* static int simul_and_eval_state_instance(orchids_t *ctx, */
/*                                          state_instance_t *state, */
/*                                          event_t *event); */
/* static rule_instance_t *try_to_launch_rule(orchids_t *ctx, */
/*                                            rule_t *rule, event_t *event); */
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

/* need to sort threads by rule instance */
static void
mark_dead_rule(orchids_t *ctx, rule_instance_t *rule)
{
  wait_thread_t *t;
  /* rule_instance_t *r; */

  /* mark threads */
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
    if ((t->state_instance->rule_instance == rule)) {
      DebugLog(DF_ENG, DS_TRACE, "Marking thread %p as KILLED (current_tail)\n", t);
      KILL_THREAD(ctx, t);
    }
  }

  for (t = ctx->new_qh; t; t = t->next) {
    if ((t->state_instance->rule_instance == rule) && !THREAD_IS_KILLED(t)) {
#ifndef ORCHIDS_DEMO
      printf("forgotten thread ? (new)\n");
#endif /* ORCHIDS_DEMO */
    }
  }

  /* mark rule */
/*   for (r = ctx->first_rule_instance; r && r != rule; r = r->next) */
/*     ; */
  rule->flags |= THREAD_KILLED;
}



static void
rip_dead_rule(orchids_t *ctx, rule_instance_t *rule)
{
  rule_instance_t *r;
  rule_instance_t *next_rule;
  rule_instance_t *prev_rule;

  prev_rule = NULL;
  for (r = ctx->first_rule_instance; r; r = next_rule) {
    next_rule = r->next;
    if (r == rule) {
/*      if ( NO_MORE_THREAD(r->threads) ) { */
      DebugLog(DF_ENG, DS_TRACE, "ripping killed rule %p\n", r);
/*       if (r->threads > 0) */
/*         DebugLog(DF_ENG, DS_ERROR, "ouch, r->thread > 0 (%u)\n", r->threads); */
      if (prev_rule) {
        prev_rule->next = r->next;
      } else {
        ctx->first_rule_instance = r->next;
      }
      free_rule_instance(ctx, r);
      return ;
    } else {
      prev_rule = r;
    }
  }
}


/**
 * Return TRUE if the synchronization variable environement
 * is completely defined.
 * @param ctx  Orchids context.
 * @param state  Current state instance.
 **/
int
sync_var_env_is_defined(orchids_t *ctx, state_instance_t *state)
{
  int i;
  int sync_var_sz;
  int sync_var;

  sync_var_sz = state->rule_instance->rule->sync_vars_sz;

  for (i = 0; i < sync_var_sz; i++) {
    sync_var = state->rule_instance->rule->sync_vars[i];
    if (   state->current_env[sync_var] == NULL
        && state->inherit_env[sync_var] == NULL)
      return (0);
  }

  return (1);
}


/**
 * Simulate a state, execute byte code, and create new treads.
 * (Jean's algo  -> q-add primitive).
 * @param ctx Orchids context.
 * @param state The state instance to simulate.
 * @param event A reference to the current event.
 * @param only_once THREAD_ONLYONCE flag (for rule initialisation).
 * @return Returns the number of created thread
 **/
static int
simulate_state_and_create_threads(orchids_t        *ctx,
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

  if (state->state->action)
    ovm_exec(ctx, state, state->state->action);

  trans_nb = state->state->trans_nb;
  if (trans_nb == 0) {
    DebugLog(DF_ENG, DS_INFO,
             "Terminal state reached for rule instance (%p)\n",
             state->rule_instance);
    mark_dead_rule(ctx, state->rule_instance);
    return (-1);
  }

  /* XXX test sync vars here, if sync, return */
  if (state->rule_instance->rule->sync_lock) {
    DebugLog(DF_ENG, DS_INFO, "Checking synchronization variables\n");
    if ( sync_var_env_is_defined(ctx, state) ) {
      DebugLog(DF_ENG, DS_INFO, "Sync env is fully defined.\n");
      sync_lock = objhash_get(state->rule_instance->rule->sync_lock, state);
      if (sync_lock) {
        DebugLog(DF_ENG, DS_INFO, "LOCK FOUND !\n");
        if (sync_lock->rule_instance != state->rule_instance) {
          DebugLog(DF_ENG, DS_INFO, "Pruning.\n");
          return (-1);
        }
        DebugLog(DF_ENG, DS_INFO, "In lock owner.\n");
      }
      else {
        /* create lock */
        if ( !only_once ) {
          DebugLog(DF_ENG, DS_INFO, "No lock found.  Creating...\n");
          objhash_add(state->rule_instance->rule->sync_lock, state, state);
          lock_elmt = Xmalloc(sizeof (sync_lock_list_t));
          lock_elmt->state = state;
          lock_elmt->next = state->rule_instance->sync_lock_list;
          state->rule_instance->sync_lock_list = lock_elmt;
        }
        else {
          DebugLog(DF_ENG, DS_INFO, "No lock found.  Don't create in ONLY_ONCE\n");
        }
      }
    }
    else {
      DebugLog(DF_ENG, DS_INFO, "Sync env is NOT fully defined.\n");
    }
  }

  for (t = 0; t < trans_nb; t++) {
    /* if we have an e-trans, pass-it */
    if (state->state->trans[t].required_fields_nb == 0) {
      vmret = 0;
      if (state->state->trans[t].eval_code)
        vmret = ovm_exec(ctx, state, state->state->trans[t].eval_code);
      if (vmret == 0) {
        DPRINTF( ("e-trans passed (to %s)\n", state->state->trans[t].dest->name) );
        new_state = create_state_instance(ctx, state->state->trans[t].dest, state);

        /* link new_state instance in the tree */
        new_state->next_sibling = state->first_child;
        state->first_child = new_state;
        new_state->parent = state;

        /* update state instance list of the current rule instance */
        new_state->retrig_next = state->rule_instance->state_list;
        state->rule_instance->state_list = new_state;

        /* and recursively call simul() */
        simul_ret = simulate_state_and_create_threads(ctx, new_state, event, only_once);
        if (simul_ret < 0)
          return (-created_threads + simul_ret);
        created_threads += simul_ret;
      }
    } else { /* we have a blocking trans, so create a new thread if needed */
      thread = Xzmalloc(sizeof (wait_thread_t));
      thread->trans = &state->state->trans[t];
      thread->state_instance = state;
      thread->flags |= only_once;
      state->rule_instance->threads++;
      ctx->threads++;
      created_threads++;

      DebugLog(DF_ENG, DS_DEBUG,
               "-- Create thread %p on state %s trans %i\n",
               thread, state->state->name, t);

      if (only_once == 0) {
      /* add it to the thread-list of the current state instance */
      thread->next_in_state_instance = state->thread_list;
      state->thread_list = thread;
      }

      /* compute timeout date */
      thread->timeout = time(NULL) + DEFAULT_TIMEOUT;

      /* add thread into the 'new thread' queue */
      if (ctx->new_qt) { /* if queue isn't empty, append to the tail */
        ctx->new_qt->next = thread;
        ctx->new_qt = thread;
      } else { /* else create head */
        ctx->new_qh = thread;
        ctx->new_qt = thread;
      }
    }
  }

  return (created_threads);
}

/**
 * Create initial threads of each rules, put the COMMIT flags
 * and merge to the current wait queue.
 * (Jean's algo -> q-init).
 * @param ctx Orchids context.
 * @param event A reference to the current event.
 **/
static void
create_rule_initial_threads(orchids_t *ctx,
                            active_event_t *event /* XXX: NOT USED */)
{
  rule_t *r;
  state_instance_t *init;
  rule_instance_t *new_rule;
  int ret;

  for (r = ctx->rule_compiler->first_rule; r; r = r->next) {
    init = create_init_state_instance(ctx, r);

    new_rule = Xzmalloc(sizeof (rule_instance_t));
    new_rule->rule = r;
    new_rule->first_state = init;
    new_rule->state_instances = 1;
    /* new_rule->max_depth = 0; */
    /* new_rule->threads = 0; */
    /* new_rule->state_list = NULL; */
    /* new_rule->retrig_list = NULL; */
    init->rule_instance = new_rule; /* move in create_init_inst() ? */
    /* link rule */
/*     ctx->state_instances++; */

    ret = simulate_state_and_create_threads(ctx, init, event, THREAD_ONLYONCE);
    /* XXX BUG HERE: if init doesn't create thread, empty rule will be linked */

    if (ret <= 0) {
      DebugLog(DF_ENG, DS_DEBUG, "No initial threads for rule %s\n",
               r->name);
      free_rule_instance(ctx, new_rule);
    }
    else {
      new_rule->threads = ret;
      new_rule->next = ctx->first_rule_instance;
      ctx->first_rule_instance = new_rule;

      if (ctx->new_qt)
        ctx->new_qt->flags |= THREAD_BUMP;
    }
  }

  /* prepare the current retrig queue by merging new and retirg queues
     (q_cur = q_new @ q_retrig) */
  if (ctx->new_qh) {
    ctx->new_qt->next = ctx->retrig_qh;
    ctx->cur_retrig_qh = ctx->new_qh;
    ctx->cur_retrig_qt = ctx->retrig_qt ? ctx->retrig_qt : ctx->new_qt;
    ctx->new_qh = NULL;
    ctx->new_qt = NULL;
  } else {
    ctx->cur_retrig_qh = ctx->retrig_qh;
    ctx->cur_retrig_qt = ctx->retrig_qt;
  }
  ctx->retrig_qh = NULL;
  ctx->retrig_qt = NULL;
}

static void
unlink_thread_in_state_instance_list(wait_thread_t *thread)
{
  wait_thread_t *t;
  wait_thread_t *prev_thread;
  wait_thread_t *next_thread;

  DebugLog(DF_ENG, DS_INFO, "unlink_thread_in_state_instance_list(%p)\n",
           thread);

  prev_thread = NULL;
  for (t = thread->state_instance->thread_list; t; t = next_thread) {
    next_thread = t->next_in_state_instance;
    if (t == thread) {
      if (prev_thread) {
        prev_thread->next_in_state_instance = t->next_in_state_instance;
      } else {
        thread->state_instance->thread_list = t->next_in_state_instance;
      }
      return ;
    } else {
      prev_thread = t;
    }
  }
}

static int
backtrack_is_not_needed(orchids_t *ctx, wait_thread_t *thread)
{
#if 1
  /* if destination state is fully blocking AND doesn't bind field value to a free variable,
   * this is THE shortest run.  Next threads will be redundant. */
  if (thread->trans->dest->trans_nb > 0 &&
      !(thread->trans->dest->flags & BYTECODE_HAVE_PUSHFIELD))
    return (TRUE);
  /* XXX: Destination MUST NOT be a cut-point. */
#endif

  return (FALSE);
}

/**
 * Analysis engine entry point.
 * (Jean's algo -> one-evt and evt-loop)
 * @param ctx orchids context.
 * @param event A reference to the event to be injected in the analysis engine.
 * @callgraph
 **/
void
inject_event(orchids_t *ctx, event_t *event)
{
  int i;
  event_t *e;
  wait_thread_t *t, *next_thread;
  int vmret;
  active_event_t *active_event;
  int sret = 0;
  int ret = 0;
  int passed_threads = 0;
  time_t cur_time;

  cur_time = time(NULL);

  DebugLog(DF_ENG, DS_INFO, "inject_event() (one-evt)\n");

/*   fprintf_safelib_memstat(stderr); */

  ctx->events++;

  /* prepare an active event record */
  active_event = Xzmalloc(sizeof (active_event_t));
  active_event->event = event;
  ctx->active_event_cur = active_event;

  /* UDP event feedback monitoring */
  if (ctx->evt_fb_fp) {
    fprintf_event(ctx->evt_fb_fp, ctx, event);
    fflush(ctx->evt_fb_fp);
  }

#if ORCHIDS_DEBUG
  fprintf_event(stderr, ctx, event);
#endif /* ORCHIDS_DEBUG */

  /* -0- resolve field value XXX: should be done with field checks */
  DebugLog(DF_ENG, DS_DEBUG, "STEP 0 - resolve field attribute values\n");
  for (i = 0; i < ctx->num_fields; i++) /* clean field val refs */
    ctx->global_fields[i].val = NULL;
  for (e = event; e; e = e->next)
    ctx->global_fields[ e->field_id ].val = e->value;

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
           "STEP 2 - evaluate all thread in retrig queue (evt-loop)\n");
  for (t = ctx->cur_retrig_qh; t; t = next_thread) {
    next_thread = t->next;
    ctx->current_tail = t;

    /* check timeout date */
    if ( !(t->flags & THREAD_ONLYONCE) && (t->timeout <= cur_time) ) {
      DebugLog(DF_ENG, DS_DEBUG, "thread %p timed-out ! (killing)\n", t);
      t->flags |= THREAD_KILLED;
    }

    /* thread ripper (and rule instance if apply) */
    if ( THREAD_IS_KILLED(t) ) {
      gettimeofday(&ctx->last_ruleinst_act, NULL);
      DebugLog(DF_ENG, DS_DEBUG, "Rip and overide killed thread (%p)\n", t);
      t->state_instance->rule_instance->threads--;
      ctx->threads--;
      if ( NO_MORE_THREAD(t->state_instance->rule_instance) ) {
        /* uptade rule instance list links (before removing) */
        t->state_instance->rule_instance->flags |= THREAD_KILLED;
        rip_dead_rule(ctx, t->state_instance->rule_instance);
      } else {
        unlink_thread_in_state_instance_list(t);
      }
      if (t == ctx->cur_retrig_qh) {
        ctx->cur_retrig_qh = next_thread;
      }
      if (ctx->cur_retrig_qt)
        ctx->cur_retrig_qt->next = NULL;
      Xfree(t);
      continue ;
    }

    DebugLog(DF_ENG, DS_DEBUG,
             "  proceeding thread %p (r=%s s=%s d=%s)\n",
             (void *)t,
             t->state_instance->rule_instance->rule->name,
             t->state_instance->state->name,
             t->trans->dest->name);

    vmret = ovm_exec(ctx, t->state_instance, t->trans->eval_code);
    if (vmret == 0) {
      state_instance_t *new_state;

      DebugLog(DF_ENG, DS_DEBUG,
               "TRANSITION MATCH ! (destination '%s')\n",
               t->trans->dest->name);
      t->pass++;
      passed_threads++;

      gettimeofday(&ctx->last_ruleinst_act, NULL);
      t->state_instance->rule_instance->new_last_act = ctx->last_ruleinst_act;

      if ( THREAD_IS_ONLYONCE(t) ) {
        DebugLog(DF_ENG, DS_DEBUG,
                 "Initial thread passed. Create rule instance.\n");
        t->state_instance->rule_instance->creation_date = time(NULL);
	t->state_instance->rule_instance->new_creation_date = ctx->last_ruleinst_act;
        t->state_instance->rule_instance->rule->instances++;
        t->state_instance->rule_instance->flags |= RULE_INUSE;
        ctx->rule_instances++;
      }

      /* create a new state instance */
      new_state = create_state_instance(ctx, t->trans->dest, t->state_instance);

      /* link new_state instance in the tree */
      new_state->next_sibling = t->state_instance->first_child;
      t->state_instance->first_child = new_state;
      new_state->parent = t->state_instance;
      new_state->event = active_event;
      active_event->refs++; /* update event references count */

      /* update state instance list of the current rule instance */
      new_state->retrig_next = t->state_instance->rule_instance->state_list;
      t->state_instance->rule_instance->state_list = new_state;

      /* and recursively call simul() */
      sret = simulate_state_and_create_threads(ctx, new_state, active_event, 0);
      if (sret < 0)
	ret -= sret;
      else
	ret += sret;

      /* static analysis flags test here (RETRIGGER) */
      if ( backtrack_is_not_needed(ctx, t) ) {
        DebugLog(DF_ENG, DS_DEBUG, "Backtrack not needed\n");
        KILL_THREAD(ctx, t);
      }
    }

    if ( THREAD_IS_ONLYONCE(t)) {
      KILL_THREAD(ctx, t);
      /* initial thread ripper: can't be free()d at the next loop
       * because initial environment can make reference to the current
       * event.  In the case of the event didn't match any transition,
       * it will be free()d _before_ the initial environment, so we'll
       * lose the reference.  */

      DebugLog(DF_ENG, DS_DEBUG, "Rip INITIAL thread (%p)\n", t);

      if (t->flags & THREAD_BUMP) {
        DebugLog(DF_ENG, DS_DEBUG,
                 "initial thread bump (commit q'_{new} = q_{new} @ q_{retrig})\n");
        if (ctx->retrig_qt)
          ctx->retrig_qt->flags |= THREAD_BUMP;
        if (ctx->new_qh) {
          ctx->new_qt->flags |= THREAD_BUMP;
          ctx->new_qt->next = ctx->retrig_qh;
          if (ctx->retrig_qt)
            ctx->new_qt = ctx->retrig_qt;
          ctx->retrig_qh = NULL;
          ctx->retrig_qt = NULL;
        } else { /* q_new is empty */
          ctx->new_qh = ctx->retrig_qh;
          ctx->new_qt = ctx->retrig_qt;
          ctx->retrig_qh = NULL;
          ctx->retrig_qt = NULL;
        }
      }

      t->state_instance->rule_instance->threads--;
      ctx->threads--;
      if ( NO_MORE_THREAD(t->state_instance->rule_instance) ) {
        /* uptade rule instance list links (before removing) */
        t->state_instance->rule_instance->flags |= THREAD_KILLED;
        rip_dead_rule(ctx, t->state_instance->rule_instance);
      } else {
        unlink_thread_in_state_instance_list(t);
      }
      if (t == ctx->cur_retrig_qh) {
        ctx->cur_retrig_qh = next_thread;
      }
      if (ctx->cur_retrig_qt)
        ctx->cur_retrig_qt->next = NULL;
      Xfree(t);
      continue;
    }

    DebugLog(DF_ENG, DS_DEBUG, "RETRIG q'_{retrig} = q_{retrig} + t\n");
    if (ctx->retrig_qh) {
      ctx->retrig_qt->next = t;
      ctx->retrig_qt = t;
    } else {
      ctx->retrig_qh = t;
      ctx->retrig_qt = t;
    }
    t->next = NULL;

#if 0
    fprintf(stdout, "thread loop new queue\n");
    fprintf_thread_queue(stdout, ctx, ctx->new_qh);

    fprintf(stdout, "thread loop  next retrig queue\n");
    fprintf_thread_queue(stdout, ctx, ctx->retrig_qh);
#endif

    /* if we need to commit new threads... merge two queues */
    if (t->flags & THREAD_BUMP) {
      DebugLog(DF_ENG, DS_DEBUG,
               "thread bump (commit q'_{new} = q_{new} @ q_{retrig})\n");
      if (ctx->retrig_qt)
        ctx->retrig_qt->flags |= THREAD_BUMP;
      if (ctx->new_qh) {
        ctx->new_qt->flags |= THREAD_BUMP;
        ctx->new_qt->next = ctx->retrig_qh;
        if (ctx->retrig_qt)
          ctx->new_qt = ctx->retrig_qt;
        ctx->retrig_qh = NULL;
        ctx->retrig_qt = NULL;
      } else { /* q_new is empty */
        ctx->new_qh = ctx->retrig_qh;
        ctx->new_qt = ctx->retrig_qt;
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

  /* free unreferenced event here (the the current event didn't passed any
     transition, Xfree() it) */
/*   if (passed_threads == 0) { */
  if (active_event->refs == 0) {
    DebugLog(DF_ENG, DS_DEBUG, "free unreferenced event (%p/%p)\n", active_event, active_event->event);
    free_event(active_event->event);
    Xfree(active_event);
  } else {
    gettimeofday(&ctx->last_evt_act, NULL);
    DebugLog(DF_ENG, DS_DEBUG,
             "Keep new event: active_event->refs = %i (linking) %i\n",
             active_event->refs, passed_threads);
    if (ctx->active_event_head == NULL) {
      ctx->active_event_head = active_event;
      ctx->active_event_tail = active_event;
    } else {
      active_event->prev = ctx->active_event_tail;
      ctx->active_event_tail->next = active_event;
      ctx->active_event_tail = active_event;
    }
/*     active_event->next = ctx->active_event_tail; */
/*     ctx->active_event_tail = active_event; */
    ctx->active_events++;
  }

  DebugLog(DF_ENG, DS_TRACE,
           "simulate_state_and_create_threads() = %i\n", ret);

#ifdef DMALLOC
    dmalloc_log_changed(dmalloc_orchids, 1, 1, 1);
#endif

}


/**
 * Create an instance of a state and inherits environment from parent.
 * This function doesn't link new state instance and its parent.
 *
 * @param ctx    The Orchids context.
 * @param state  The state definition to instantiate.
 * @param parent The parent state instance, in the path tree.
 * @return The new state instance.
 **/
/* Environments managment really needs _HEAVY_ optimizations */
static state_instance_t *
create_state_instance(orchids_t *ctx, state_t *state, const state_instance_t *parent)
{
  state_instance_t *new_state;
  int env_sz;
  int i;

  /* alloc and init state instance */
  new_state = Xzmalloc(sizeof (state_instance_t));
  new_state->state = state;
  new_state->rule_instance = parent->rule_instance;
  new_state->depth = parent->depth + 1;

  /* build inherited environment */
  if (state->rule->dynamic_env_sz > 0) {
    env_sz = state->rule->dynamic_env_sz * sizeof (ovm_var_t *);
    new_state->inherit_env = Xmalloc(env_sz); /* will be entirely overwritten */
    new_state->current_env = Xzmalloc(env_sz); /* initialise env to 0 */
  }
  /* IDEA of optimization :
  ** if there is no action bytecode in this state, shadow-env can be
  ** a reference to parent envs (PBs for desallocating) */

  /* shadow_env construction
     -- XXX: optimize this, remove pointer resolution !!! */
  for (i = 0; i < state->rule->dynamic_env_sz; ++i) {
    if (parent->current_env[i])
      new_state->inherit_env[i] = parent->current_env[i];
    else
      new_state->inherit_env[i] = parent->inherit_env[i];
  }

  ctx->state_instances++;

  return (new_state);
}

/**
 * Create an instance of the initial state of a rule.
 * 'init' state is a special case (environment aren't inherited).
 *
 * @param ctx  Orchids context.
 * @param rule The rule definition to instantiate its 'init' state.
 * @return The new 'init' state instance.
 **/
static state_instance_t *
create_init_state_instance(orchids_t *ctx, const rule_t *rule)
{
  state_t *state;
  state_instance_t *new_state;
  int env_sz;

  state = &rule->state[0];
  /* alloc and init state instance */
  new_state = Xzmalloc(sizeof (state_instance_t));
  new_state->state = state;

  /* build inherited environment */
  if (state->rule->dynamic_env_sz > 0) {
    env_sz = state->rule->dynamic_env_sz * sizeof (ovm_var_t *);
    new_state->current_env = Xzmalloc(env_sz);
    new_state->inherit_env = Xzmalloc(env_sz);
  }

  ctx->state_instances++;

  return (new_state);
}


/**
 * Free an entire rule instance (state instances and environments)
 * and update global statistics (active states and rules).
 *
 * @param ctx Orchids context.
 * @param rule_instance Rule instance to destroy.
 **/
static void
free_rule_instance(orchids_t *ctx, rule_instance_t *rule_instance)
{
  state_instance_t *si;
  state_instance_t *next_si;
  ovm_var_t **cur_env;
  int dyn_env_sz;
  int i;
  sync_lock_list_t *lock_elmt;
  sync_lock_list_t *lock_next;

  DebugLog(DF_ENG, DS_DEBUG, "free_rule_instance(%p)\n", rule_instance);

  /* remove synchronization locks, if any */
  for (lock_elmt = rule_instance->sync_lock_list;
       lock_elmt;
       lock_elmt = lock_next) {
    lock_next = lock_elmt->next;
    DebugLog(DF_ENG, DS_ERROR, "free_rule_instance(%p): removing lock %p\n",
             rule_instance, lock_elmt->state);
    si = objhash_del(rule_instance->rule->sync_lock, lock_elmt->state);
    if (si == NULL) {
      DebugLog(DF_ENG, DS_ERROR, "free_rule_instance(%p): lock not found\n",
               rule_instance);
    }
    Xfree(lock_elmt);
  }

  /* free init state (MEMLEAK WAS HERE ;) */
  if (rule_instance->first_state->inherit_env)
    Xfree(rule_instance->first_state->inherit_env);

  dyn_env_sz = rule_instance->rule->dynamic_env_sz;
  cur_env = rule_instance->first_state->current_env;
  for (i = 0; i < dyn_env_sz; ++i)
    if (cur_env[i] && CAN_FREE_VAR(cur_env[i]) ) {
      Xfree(cur_env[i]);
    }

  if (rule_instance->first_state->current_env)
    Xfree(rule_instance->first_state->current_env);
  Xfree(rule_instance->first_state);

  ctx->state_instances--;

  si = rule_instance->state_list;
/*   printf("si=%p\n", si); */
  while (si) {
    next_si = si->retrig_next;
    if (si->inherit_env) /* can be null for init state (XXX MEMLEAK HERE) */
      Xfree(si->inherit_env);

    /* free all 'current dynamic environment' variables */
    dyn_env_sz = rule_instance->rule->dynamic_env_sz;
    if (dyn_env_sz > 0) { /* Bug here: try to free() un-malloc()ed memory ! */
      for (i = 0, cur_env = si->current_env; i < dyn_env_sz; ++i)
        if (cur_env[i] && CAN_FREE_VAR(cur_env[i]) ) { /* Double free was here ! ;-( Free only TYPE_TEMP */
          Xfree(cur_env[i]);
        }
      Xfree(si->current_env);
    }

    /* update event reference count */
    if (si->event) {
      si->event->refs--;

    /* unreferenced active events are freed in inject_event() */
    if (si->event->refs <= 0 && si->event != ctx->active_event_cur) {
      gettimeofday(&ctx->last_evt_act, NULL);
      if (si->event->refs < 0) {
        printf("refs < 0 ?!\n");
      }
      DebugLog(DF_ENG, DS_DEBUG, "event %p ref=0\n", si->event);
      free_event(si->event->event);
      si->event->event = NULL;
      ctx->active_events--;
      /* unlink */
      if (!si->event->prev) { /* si l'element est le premier */

        if (!si->event->next) { /* si l'element est le dernier */
          /* alors c'est le dernier de la liste */
          ctx->active_event_head = NULL;
          ctx->active_event_tail = NULL;
        } else {
          /* alors c'est le premier de la liste, avec des elements deriere */
          ctx->active_event_head = si->event->next;
          ctx->active_event_head->prev = NULL;
        }
/*         si->event->next->prev = si->event->prev; */

      } else { /* si l'element c'est pas le permier */

        if (!si->event->next) { /* si l'element est le dernier */
          /* donc l'element est le dernier, avec d'autre elements avant */
          si->event->prev->next = NULL;
          ctx->active_event_tail = si->event->prev;
        } else { /* sinon, l'element n'est pas le dernier */
          /* donc l'element est en plein milieu de la liste */
          si->event->prev->next = si->event->next;
          si->event->next->prev = si->event->prev;
        }
/*         ctx->active_event_tail = si->event->prev; */
      }


/*         si->event->prev->next = si->event->next; */
/*       } else { /\* sinon l'element n'est le premier *\/ */
/*         ctx->active_event_head = si->event->next; */
/*       } */
      Xfree(si->event);
    }
    }
    Xfree(si);
    si = next_si;
    ctx->state_instances--;
  }

  /* update statistics */
  if (rule_instance->creation_date) {
    rule_instance->rule->instances--;
    ctx->rule_instances--;
  }

  Xfree(rule_instance);
}

/**
 * Display all active rule instances. Displayed informations are :
 * rule instance identifier (rid), 
 * rule name, 
 * number of actives states, 
 * max depth of the path tree (dpt) 
 * and the creation date.
 *
 * @param fp Output stream.
 * @param ctx Orchids context.
 **/
void
fprintf_rule_instances(FILE *fp, const orchids_t *ctx)
{
  char asc_time[32];
  rule_instance_t *r;
  int i;

  if (ctx->first_rule_instance == NULL)
    {
      fprintf(fp, "no rule instance.\n");
      return ;
    }

  fprintf(fp, "-------------------------------[ rule instances ]------------------------------\n");
  fprintf(fp, " rid | rule name   | state |thrds| dpt | creation date\n");
  fprintf(fp, "-----+-------------+-------+-----+-----+---------------------------------------\n");
  for (r = ctx->first_rule_instance, i = 0; r; r = r->next, ++i)
    {
      if (r->creation_date > 0) {
        strftime(asc_time, 32, "%a %b %d %H:%M:%S %Y",
                 localtime(&r->creation_date));
      }
      else {
        strcpy(asc_time, "initial instance");
      }
      fprintf(fp, " %3i |%12.12s |%6i |%4i | %3i | %li (%.32s)\n",
              i, r->rule->name, r->state_instances, r->threads,
              r->max_depth, r->creation_date, asc_time);
    }
  fprintf(fp, "-----+-------------+-------+-----+-----+---------------------------------------\n");
}

/*
 * Display a thread queue.
 **/
void
fprintf_thread_queue(FILE *fp, orchids_t *ctx, wait_thread_t *thread)
{
  unsigned int i;
  unsigned int k;

  fprintf(fp,
          "----------------------------[ "
          "thread wait queue"
          " ]----------------------------\n");
  fprintf(fp,
          " thid "
          "|   rule name  "
          "|      state names     "
          "| rid | sid->did |tran| cnt | bmp\n");
  fprintf(fp,
          "------"
          "+--------------"
          "+----------------------"
          "+-----+----------+----+-----+----\n");

  k = 0;
  for (i = 0; thread; thread = thread->next, i++) {
    if (!(thread->flags & THREAD_KILLED)) {
      fprintf(fp, "%5u | %12.12s | %8.8s -> %-8.8s | %3i | %2i -> %-2i | %2i | %3i |%s\n",
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
    else {
      k++;
      fprintf(fp, "%5u*| %12.12s | %8.8s -> %-8.8s | %3i | %2i -> %-2i | %2i | %3i |%s\n",
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
  fprintf(fp, "------+--------------+----------------------+-----+----------+----+-----+----\n");
}

/**
 * Display all active events
 **/
void
fprintf_active_events(FILE *fp, orchids_t *ctx)
{
  unsigned int i;
  active_event_t *e;

  if (ctx->active_event_head == NULL) {
    fprintf(fp, "no active event.\n");
    return ;
  }

  fprintf(fp, "--------------[ active events ]-------------\n");

  for (i = 0, e = ctx->active_event_head; e; e = e->next, i++) {
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
