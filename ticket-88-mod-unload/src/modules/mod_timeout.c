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

#include "evt_mgr.h"
#include "orchids_api.h"

#include "mod_timeout.h"


input_module_t mod_timeout;

/* XXX: small hack to retrieve the mod entry
   in language function and/or rtcallback */
static mod_entry_t *mod_entry_g = NULL;


static field_t timeout_fields[] = {
  { "timeout.date",        T_TIMEVAL,  "Date when the timeout triggered"   },
  { "timeout.regdate",     T_TIMEVAL,  "Registration date of the timeout"   },
  { "timeout.name",        T_VSTR,     "Name of the timeout"   },
  { "timeout.rule",        T_VSTR,     "Name of the registration rule" },
  { "timeout.rule_inst",   T_INT,      "Instance ID of the registration rule instance" },
  { "timeout.state",       T_VSTR,     "Name of the registration state" },
  { "timeout.state_inst",  T_INT,      "Instance ID of the registration state instance" },
};


static int
timeout_rtcallback(orchids_t *ctx, rtaction_t *e)
{
  ovm_var_t **attr;
  event_t *event;

  attr = (ovm_var_t **)e->data;

  /* set the current date */
  attr[F_DATE] = ovm_timeval_new();
  TIMEVAL(attr[F_DATE]) = ctx->cur_loop_time;

  event = NULL;
  add_fields_to_event(ctx, mod_entry_g, &event, attr, TIMEOUT_FIELDS);

  Xfree(attr);

  post_event(ctx, mod_entry_g, event);

  return (0);
}


static void
issdl_timeout(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *toname;
  ovm_var_t *todelay;
  ovm_var_t **attr;
  char *str;

  toname = stack_pop(ctx->ovm_stack);
  if (TYPE(toname) != T_STR) {
    DebugLog(DF_ENG, DS_ERROR, "parameter type error\n");
    return ;
  }

  todelay = stack_pop(ctx->ovm_stack);
  if (TYPE(todelay) != T_INT) {
    DebugLog(DF_ENG, DS_ERROR, "parameter type error\n");
    return ;
  }

  attr = Xzmalloc( sizeof (ovm_var_t) * TIMEOUT_FIELDS);

  /* clone all data, in case of the rule which registered the timeout
     was killed before the timeout */
  attr[F_REGDATE] = ovm_timeval_new();
  TIMEVAL(attr[F_REGDATE]) = ctx->cur_loop_time;

  attr[F_NAME] = ovm_str_new( STRLEN(toname) );
  memcpy(STR(attr[F_NAME]), STR(toname), STRLEN(toname));

  attr[F_RULE] = ovm_vstr_new();
  str = state->rule_instance->rule->name;
  VSTR(attr[F_RULE]) = str;
  VSTRLEN(attr[F_RULE]) = strlen(str);

  attr[F_STATE] = ovm_vstr_new();
  str = state->state->name;
  VSTR(attr[F_STATE]) = str;
  VSTRLEN(attr[F_STATE]) = strlen(str);

  register_rtcallback(ctx, timeout_rtcallback, attr, INT(todelay) );

  /* XXX: FREE IF NEEDED parameters */
}


static void *
timeout_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  DebugLog(DF_MOD, DS_DEBUG, "load() timeout@%p\n", (void *) &mod_timeout);

  register_fields(ctx, mod, timeout_fields, TIMEOUT_FIELDS);

  register_lang_function(ctx,
                         issdl_timeout,
                         "timeout", 2,
                         "Register a real-time timeout.");

  mod_entry_g = mod;

  return (NULL);
}


input_module_t mod_timeout = {
  MOD_MAGIC,
  ORCHIDS_VERSION,
  "timeout",
  "CeCILL2",
  NULL,
  NULL,
  timeout_preconfig,
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
