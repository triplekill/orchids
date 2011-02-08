/**
 ** @file queue.h
 ** queue lib.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup util
 **
 ** @date  Started on: Tue Mar  4 12:25:38 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef QUEUE_H
#define QUEUE_H

typedef struct queue_element_s queue_element_t;
struct queue_element_s
{
  queue_element_t *next;
  queue_element_t *prev;
  void *data;
};

typedef struct queue_s queue_t;
struct queue_s
{
  int elements;
  queue_element_t *head;
  queue_element_t *tail;
};

queue_t *
queue_new(void);

void
queue_add(queue_t *q, void *data);

void *
queue_get(queue_t *q);

void
queue_move(queue_t *q_dest, queue_t *q_src);

void
queue_append(queue_t *q_dest, queue_t *q_src);

void
queue_merge(queue_t *q_dest, queue_t *q_1, queue_t *q_2);

#endif /* QUEUE_H */



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
