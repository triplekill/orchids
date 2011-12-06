/**
 ** @file ringqueue.c
 ** Fast ring buffer queue library.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup util
 **
 ** @date  Started on: Tue Mar  4 16:22:21 2003
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
#include "ringqueue.h"

ringqueue_t *
ringqueue_new(size_t size, size_t grow)
{
  ringqueue_t *q;

  q = Xmalloc(sizeof (ringqueue_t));
  q->enqueue = -1;
  q->dequeue = -1;
  q->size = size;
  q->grow = grow;
  q->data = Xmalloc(size * sizeof (void *));

  return (q);
}

void
ringqueue_shrink(ringqueue_t *q)
{
  int cur_size;

  /* XXX: ToDo compute # of present elements */

  /* if queue is empty, we cannot shkink */
  if (q->dequeue == -1)
    return ;

  cur_size = 0;
  q->size = cur_size - (cur_size % q->grow) + q->grow;
}

void
ringqueue_add(ringqueue_t *q, void *data)
{
  /* head and tail collide, need to grow data buffer */
  if ((q->enqueue + 1 % q->size ) == q->dequeue)
    {
      size_t old_size;
      void **new_data;

      old_size = q->size;
      q->size += q->grow;
      new_data = Xmalloc(q->size * sizeof (void *));
      memcpy(new_data, &q->data[ q->dequeue ], (old_size - q->dequeue) * sizeof (void *));
      memcpy(&new_data[ old_size - q->dequeue ], q->data, (q->enqueue + 1) * sizeof (void *));
      Xfree(q->data);
      q->data = new_data;
      q->enqueue = old_size;
      q->dequeue = 0;
    }
  else
    q->enqueue = ++q->enqueue % q->size;

  if (q->dequeue == -1)
    q->dequeue = q->enqueue;

  q->data[ q->enqueue ] = data;
}

void *
ringqueue_get(ringqueue_t *q)
{
  void *data;

  if (q->dequeue == -1)
    {
      DPRINTF( ("nothing to dequeue\n") );
      return (NULL);
    }

  data = q->data[ q->dequeue ];
  if (q->dequeue == q->enqueue)
    q->dequeue = -1;
  else
    q->dequeue = q->dequeue++ % q->size;

  return (data);
}

#if 0
void
ringqueue_test(void)
{
  int i;
  ringqueue_t *q;
  void *d;

  q = new_queue(1000, 500);

  /* -1- fill queue */
  for (i = 0; i < 10000; ++i)
    {
      d = dequeue(q);
      DPRINTF( ("-1- dequeued val: %i\n", d) );
      enqueue(q, (void *)123456789);
      enqueue(q, (void *)i);
    }

  /* -2- empty queue */
  while (d)
    {
      d = dequeue(q);
      DPRINTF( ("-2- dequeued val: %i\n", d) );
    }
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
