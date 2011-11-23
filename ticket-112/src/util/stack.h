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
  int pos;
  size_t size;
  size_t grow;
  void **data;
};

/* macro version of stack_push and stack_pop -- really faster */

#define STACK_pop(stack) \
  ((stack)->pos < 0 ? NULL : (stack)->data[ stack->pos--] )

#define STACK_push(stack, value)                                             \
do {                                                                         \
  if ((stack)->pos == (stack)->size - 1) {                                   \
    (stack)->size += (stack)->grow;                                          \
    (stack)->data = xrealloc((stack)->data, (stack)->size*(sizeof(void *))); \
  }                                                                          \
  stack->data[ ++stack->pos ] = value;                                       \
} while (0)

lifostack_t *new_stack(size_t initial_size, size_t grow_size);
void stack_push(lifostack_t *stack, void *data);
void *stack_pop(lifostack_t *stack);
void stack_resize(lifostack_t *stack, size_t newsize);
void stack_shrink(lifostack_t *stack);
void stack_free(lifostack_t *stack);

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
