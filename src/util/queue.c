/**
 ** @file queue.c
 ** queue library.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup util
 **
 ** @date  Started on: Tue Mar  4 16:25:29 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "orchids.h"
#include "queue.h"

queue_t *
queue_new(void)
{
  queue_t *q;

  q = Xzmalloc(sizeof (queue_t));

  return (q);
}

void
queue_add(queue_t *q, void *data)
{
  queue_element_t *e;

  e = Xzmalloc(sizeof (queue_element_t));
  e->data = data;

  if (q->elements == 0)
    {
      q->head = e;
      q->tail = e;
    }
  else
    {
      q->tail->next = e;
      q->tail = e;
    }

  q->elements++;
}

void *
queue_get(queue_t *q)
{
  queue_element_t *e;
  void *data;

  if (q->elements == 0)
    return (NULL);

  e = q->head;
  q->elements--;
  q->head = e->next;
  data = e->data;
  Xfree(e);

  return (data);
}

void
queue_move(queue_t *q_dest, queue_t *q_src)
{
  if (q_dest->elements > 0)
    {
      DPRINTF( ("error: destination queue MUST be empty (to avoid me leaks)\n") );
      return ;
    }

  q_dest->elements = q_src->elements;
  q_dest->head = q_src->head;
  q_dest->tail = q_src->tail;
  q_src->elements = 0;
}

void
queue_append(queue_t *q_dest, queue_t *q_src)
{
  if (q_dest->elements > 0 && q_src->elements > 0) /* most frequent case */
    {
      q_dest->tail->next = q_src->head;
      q_dest->tail = q_src->tail;
      q_dest->elements += q_src->elements;
      q_src->elements = 0;
    }
  else if (q_dest->elements == 0) /* q_dest is empty */
    {
      q_dest->head = q_src->head;
      q_dest->tail = q_src->tail;
      q_dest->elements = q_src->elements;
      q_src->elements = 0;
    }
  /* if (q_1->elements == 0) */ /* q_src is empty, nothing to do. */
  /* the last case is when q_1 AND q_2 are empty, nothing to do too. */
}

void
queue_merge(queue_t *q_dest, queue_t *q_1, queue_t *q_2)
{
  if (q_dest->elements > 0)
    {
      DPRINTF( ("error: destination queue MUST be empty (to avoid me leaks)\n") );
      return ;
    }

  q_dest->elements = q_1->elements + q_2->elements;

  if (q_1->elements > 0 && q_2->elements > 0) /* most frequent case */
    {
      q_1->tail->next = q_2->head;
      q_dest->head = q_1->head;
      q_dest->tail = q_2->tail;
      q_1->elements = 0;
      q_2->elements = 0;
    }
  else if (q_2->elements == 0) /* q_2 is empty */
    {
      q_dest->head = q_1->head;
      q_dest->tail = q_1->tail;
      q_1->elements = 0;
    }
  else /* if (q_1->elements == 0) */ /* q_1 is empty */
    {
      q_dest->head = q_2->head;
      q_dest->tail = q_2->tail;
      q_2->elements = 0;
    }
  /* the last case is when q_1 AND q_2 are empty */
}


#if 0
void
queue_test(void)
{
  int i;
  queue_t *q, *q1, *q2;
  void *d;

  q = queue_new();
  q1 = queue_new();
  q2 = queue_new();
  DPRINTF( ("-0- new\n") );

  /* -1- fill queue */
  for (i = 0; i < 10000; ++i)
    {
      d = queue_get(q);
      DPRINTF( ("-1- dequeued val: %i\n", (int)d) );
      queue_add(q, (void *)123456789);
      queue_add(q, (void *)i);
      queue_add(q1, (void *) (i*2));
      queue_add(q2, (void *) (i*3));
    }

  /* -2- empty queue */
  while (q->elements > 0)
    {
      d = queue_get(q);
      DPRINTF( ("-2- dequeued val: %i\n", (int)d) );
    }

  DPRINTF( ("e = %i\n", q->elements) );

  queue_merge(q, q1, q2);

  while (q->elements > 0)
    {
      d = queue_get(q);
      DPRINTF( ("-3- dequeued val: %i\n", (int)d) );
    }


  exit(EXIT_SUCCESS);
}
#endif 


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
