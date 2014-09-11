/**
 ** @file stack.h
 ** Header for stack functions.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup util
 **
 ** @date  Started on: Mon Jan 20 16:52:03 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef STACK_H
#define STACK_H

typedef struct lifostack_s lifostack_t;
struct lifostack_s
{
  /*
  ** pos: position in stack. points on top stack elements
  */
  gc_header_t gc;
  int n; /* current number of elements in data[] */
  int nmax; /* max number of elements in data[]; should be >0. */
  gc_header_t **data; /* pointer to array[nmax] */
};

/* macro version of stack_push and stack_pop -- really faster */

#define STACK_POP(gc_ctx,stack) \
  (((stack)->n > 0) ? \
   (stack)->data[--(stack)->n] :		\
   stack_pop (gc_ctx,stack))
#define STACK_ELT(stack,i) (stack)->data[(stack)->n-(i)]
#define STACK_DROP(stack,k)		 \
  (stack)->n -= (k)
#define STACK_DEPTH(stack) (stack)->n
#define STACK_PUSH_NOTOUCH(gc_ctx, stack, value) \
  do {									\
  if ((stack)->n < (stack)->nmax) \
    { (stack)->data[(stack)->n++] = value; } \
  else stack_push(gc_ctx, stack, value); \
  } while (0)
#define STACK_PUSH(gc_ctx, stack, value) \
  do {									\
  if ((stack)->n < (stack)->nmax) \
    { GC_TOUCH(gc_ctx, value); (stack)->data[(stack)->n++] = value; } \
  else stack_push(gc_ctx, stack, value); \
  } while (0)

lifostack_t *new_stack(gc_t *gc_ctx, int nmax);
void stack_push(gc_t *gc_ctx, lifostack_t *stack, gc_header_t *p);
gc_header_t *stack_pop(gc_t *gc_ctx, lifostack_t *stack);

#endif /* STACK_H */



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
