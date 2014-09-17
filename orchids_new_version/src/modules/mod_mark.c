/**
 ** @file mod_mark.c
 ** Add the ability to add marks and do red cuts from these marks.
 **
 ** @author Baptiste GOURDIN <baptiste.gourdin@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 **
 ** @date  Started on: Wed Nov 16 10:17:21 CET 2011
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */


#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "ovm.h"

#include <stdlib.h>
#include <stdio.h>
//#include <avl.h>


#include "mod_mark.h"

input_module_t mod_mark;

static void issdl_mark(orchids_t *ctx, state_instance_t *state)
{
  PUSH_VALUE(ctx, state);
}


static void do_recursive_cut(state_instance_t *state)
{
  wait_thread_t *t;

  state->flags |= SF_PRUNED;

  /* Cut current state instance threads */
  for (t = state->thread_list; t; t = t->next_in_state_instance)
    {
      if ( !(t->flags & THREAD_ONLYONCE) )
	{
	  DebugLog(DF_ENG, DS_TRACE,
		   "Marking thread %p as KILLED (cut)\n", t);
	  t->flags |= THREAD_KILLED;
	}
    }

  /* Then call recursively */
  if (state->first_child)
    do_recursive_cut(state->first_child);

  if (state->next_sibling)
    do_recursive_cut(state->next_sibling);
}

static void issdl_mark_cut(orchids_t *ctx, state_instance_t *state)
{
  state_instance_t *mark;

  mark = (state_instance_t *)STACK_ELT(ctx->ovm_stack, 1);
  if (mark==NULL || TYPE(mark)!=T_STATE_INSTANCE)
    {
      DebugLog(DF_MOD, DS_ERROR, "parameter type error\n");
      STACK_DROP(ctx->ovm_stack, 1);
      PUSH_RETURN_FALSE(ctx);
      return;
    }
  do_recursive_cut(mark);
  STACK_DROP(ctx->ovm_stack, 1);
  PUSH_RETURN_TRUE(ctx);
}

static void *mark_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  DebugLog(DF_MOD, DS_INFO, "load() mark@%p\n", &mod_mark);

  register_lang_function(ctx,
                         issdl_mark,
                         "mark", 0,
                         "Set a mark and return an id");

  register_lang_function(ctx,
                         issdl_mark_cut,
                         "mark_cut", 1,
                         "Red cut from the state where the id was created");
  return NULL;
}

input_module_t mod_mark = {
  MOD_MAGIC,                /* Magic number */
  ORCHIDS_VERSION,          /* Module version */
  "mark",                   /* module name */
  "CeCILL2",                /* module license */
  NULL               ,      /* module dependencies */
  NULL,			    /* module configuration commands,
                               for core config parser */
  mark_preconfig,           /* called just after module registration */
  NULL,		            /* called after all mods preconfig,
                               and after all module configuration*/
  NULL
};


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