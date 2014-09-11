/**
 ** @file stack.c
 ** Stack functions.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup util
 **
 ** @date  Started on: Mon Jan 20 16:49:53 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#include "orchids.h"

#include "stack.h"

static void stack_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  lifostack_t *stk = (lifostack_t *)p;
  int i;

  for (i=0; i<stk->n; i++)
    GC_TOUCH (gc_ctx, stk->data[i]);
}

static void stack_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  gc_base_free (((lifostack_t *)p)->data);
}

static int stack_traverse (gc_traverse_ctx_t *gtc,
			   gc_header_t *p, void *data)
{
  lifostack_t *stk = (lifostack_t *)p;
  int err = 0;
  int i;

  for (i=0; i<stk->n; i++)
    {
      err = (*gtc->do_subfield) (gtc, (gc_header_t *)stk->data[i], data);
      if (err)
	return err;
    }
  return err;
}
gc_class_t stack_class = {
  GC_ID('s','t','c','k'),
  stack_mark_subfields,
  stack_finalize,
  stack_traverse
};
  
lifostack_t *new_stack(gc_t *gc_ctx, int nmax)
{
  lifostack_t *s;

  GC_START(gc_ctx, 1);
  s = gc_alloc(gc_ctx, sizeof (lifostack_t), &stack_class);
  GC_UPDATE(gc_ctx, 0, s);
  s->n = 0;
  s->nmax = nmax;
  s->data = gc_base_malloc(gc_ctx, nmax * sizeof(gc_header_t *));
  GC_END(gc_ctx);
  return s;
}

void stack_push(gc_t *gc_ctx, lifostack_t *stack, gc_header_t *p)
{
  if (stack->n >= stack->nmax)
    {
      int new_nmax;

      GC_START(gc_ctx, 1);
      GC_UPDATE(gc_ctx, 0, p);
      new_nmax = 2*stack->nmax;
      if (new_nmax<=0) /* overflow */
	{
	  new_nmax = INT_MAX;
	  if (new_nmax==stack->nmax)
	    {
	      fprintf(stderr, "stack_push: stack overflow.\n");
	      exit(EXIT_FAILURE);
	    }
	}
      stack->data = gc_base_realloc(gc_ctx, (void *)stack->data, new_nmax);
      stack->nmax = new_nmax;
      GC_END(gc_ctx);
    }
  GC_TOUCH (gc_ctx, stack->data[stack->n++] = p);
}

gc_header_t *stack_pop(gc_t *gc_ctx, lifostack_t *stack)
{
  if (stack->n <= 0)
    {
      fprintf (stderr, "stack_pop: stack underflow.\n");
      exit(EXIT_FAILURE);
    }
  return stack->data[--stack->n];
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
