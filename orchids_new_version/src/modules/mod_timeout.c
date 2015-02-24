/**
 ** @file mod_timeout.c
 ** A module for implementing active real-time timeouts.
 ** 
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 ** 
 ** @version 0.1.0
 ** 
 ** @date  Started on: Fri Jun 15 10:53:53 2007
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "orchids.h"
#include "ovm.h"
#include "evt_mgr.h"
#include "orchids_api.h"

#include "mod_timeout.h"


input_module_t mod_timeout;

/* XXX: small hack to retrieve the mod entry
   in language function and/or rtcallback */
static mod_entry_t *mod_entry_g = NULL;


static field_t timeout_fields[] = {
  { "timeout.date",        &t_timeval, MONO_MONO,  "Date when the timeout triggered"   },
  { "timeout.regdate",     &t_timeval, MONO_MONO,  "Registration date of the timeout"   },
  { "timeout.name",        &t_str, MONO_UNKNOWN,   "Name of the timeout"   },
#ifdef OBSOLETE
  { "timeout.rule",        &t_str, MONO_UNKNOWN,   "Name of the registration rule" },
  { "timeout.state",       &t_str, MONO_UNKNOWN,   "Name of the registration state" },
#endif
};


static int timeout_rtcallback(orchids_t *ctx, heap_entry_t *he)
{
  field_table_t *fields;
  ovm_var_t *val;
  gc_t *gc_ctx = ctx->gc_ctx;

  fields = (field_table_t *)he->gc_data;
  GC_START(gc_ctx, 1); /* To store the event list.
			  No need to protect fields, which our caller
			  protected already. */
  /* set the current date */
  val = ovm_timeval_new (gc_ctx);
  TIMEVAL(val) = ctx->cur_loop_time;
  GC_TOUCH (gc_ctx, fields->field_values[F_DATE] = val);

  add_fields_to_event(ctx, mod_entry_g, (event_t **)GC_LOOKUP(0),
		      fields->field_values, TIMEOUT_FIELDS);
  post_event (ctx, mod_entry_g, (event_t *)GC_LOOKUP(0));

  gc_base_free (he);
  GC_END(gc_ctx);
  return 0;
}


static void issdl_timeout(orchids_t *ctx, state_instance_t *state)
{
  gc_t *gc_ctx = ctx->gc_ctx;
  ovm_var_t *toname;
  ovm_var_t *todelay;
  ovm_var_t *val;
#ifdef OBSOLETE
  rule_t *rule;
  char *str;
#endif
  field_table_t *fields;

  toname = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  if (toname==NULL || (TYPE(toname) != T_STR && TYPE(toname) != T_VSTR))
    {
      DebugLog(DF_ENG, DS_ERROR, "parameter type error\n");
      STACK_DROP(ctx->ovm_stack, 2);
      PUSH_RETURN_FALSE(ctx);
      return;
    }

  todelay = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  if (todelay==NULL || TYPE(todelay) != T_INT)
    {
      DebugLog(DF_ENG, DS_ERROR, "parameter type error\n");
      STACK_DROP(ctx->ovm_stack, 2);
      PUSH_RETURN_FALSE(ctx);
      return;
    }

  fields = new_field_table (gc_ctx, TIMEOUT_FIELDS);
  GC_START(gc_ctx, 1);
  GC_UPDATE(gc_ctx, 0, fields);

  /* clone all data, in case of the rule which registered the timeout
     was killed before the timeout */
  val = ovm_timeval_new(gc_ctx);
  TIMEVAL(val) = ctx->cur_loop_time;
  GC_TOUCH (gc_ctx, fields->field_values[F_REGDATE] = val);

  GC_TOUCH (gc_ctx, fields->field_values[F_NAME] = toname);

#ifdef OBSOLETE
  rule = state->rule_instance->rule;
  val = ovm_vstr_new (gc_ctx, (ovm_var_t *)rule);
  str = rule->name;
  VSTR(val) = str;
  VSTRLEN(val) = strlen(str);
  GC_TOUCH (gc_ctx, fields->field_values[F_RULE] = val);

  val = ovm_vstr_new (gc_ctx, (ovm_var_t *)rule);
  /* We are creating a vstring on state->state->name.  The delegate
     really should be rule, which holds the rule->state[] of all its states */
  str = state->state->name;
  VSTR(val) = str;
  VSTRLEN(val) = strlen(str);
  GC_TOUCH (gc_ctx, fields->field_values[F_STATE] = val);
#endif

  register_rtcallback(ctx, timeout_rtcallback,
		      (gc_header_t *)fields, NULL, INT(todelay) );
  GC_END(gc_ctx);
  STACK_DROP(ctx->ovm_stack, 2);
  PUSH_RETURN_TRUE(ctx);
}

static const type_t *timeout_sig[] = { &t_int, &t_str, &t_int };
static const type_t **timeout_sigs[] = { timeout_sig, NULL };

static void * timeout_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  DebugLog(DF_MOD, DS_DEBUG, "load() timeout@%p\n", (void *) &mod_timeout);

  register_fields(ctx, mod, timeout_fields, TIMEOUT_FIELDS);

  register_lang_function(ctx,
                         issdl_timeout,
                         "timeout",
			 2, timeout_sigs,
			 m_unknown_2,
                         "Register a real-time timeout.");

  mod_entry_g = mod;

  return (NULL);
}


input_module_t mod_timeout = {
  MOD_MAGIC,
  ORCHIDS_VERSION,
  0,			    /* flags */
  "timeout",
  "CeCILL2",
  NULL,
  NULL,
  timeout_preconfig,
  NULL,
  NULL,
  NULL,
  NULL
};


/*
** Copyright (c) 2002-2007 by Julien OLIVAIN, Laboratoire Spécification
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
