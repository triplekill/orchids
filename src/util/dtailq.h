/**
 ** @file dtailq.h
 ** Macros for doubly linked queues.
 ** 
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 ** 
 ** @version 0.1.0
 ** 
 ** @date  Started on: Mon Jan 13 10:09:19 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */


#ifndef DTAILQ_H
#define DTAILQ_H

#include "offsetof.h"

/* list macros. inspired from FreeBSD <sys/queue.h> */


/*
 * Tail queue declarations.
 */
#define DTAILQ_HEAD(name, type_t)                                             \
struct dtailq_##name##_s {                                                    \
  type_t  *tqh_first;                                                         \
  type_t **tqh_last;                                                          \
}

#define DTAILQ_HEAD_INITIALIZER(head)                                         \
  { NULL, &(head).tqh_first }

#define DTAILQ_ENTRY(type_t)                                                  \
struct {                                                                      \
  type_t  *tqe_next;                                                          \
  type_t **tqe_prev;                                                          \
}

/*
 * Tail queue functions.
 */
#define DTAILQ_CONCAT(head1, head2, field)                                    \
do {                                                                          \
  if (!DTAILQ_IS_EMPTY(head2)) {                                              \
    *(head1)->tqh_last = (head2)->tqh_first;                                  \
    (head2)->tqh_first->field.tqe_prev = (head1)->tqh_last;                   \
    (head1)->tqh_last = (head2)->tqh_last;                                    \
    DTAILQ_INIT((head2));                                                     \
  }                                                                           \
} while (0)

#define DTAILQ_IS_EMPTY(head) ((head)->tqh_first == NULL)

#define DTAILQ_FIRST(head) ((head)->tqh_first)

#define DTAILQ_FOREACH(var, head, field)                                      \
  for ((var) = DTAILQ_FIRST((head));                                          \
       (var);                                                                 \
       (var) = DTAILQ_NEXT((var), field))

#define DTAILQ_FOREACH_SAFE(var, head, field, tvar)                           \
  for ((var) = DTAILQ_FIRST((head));                                          \
       (var) && ((tvar) = DTAILQ_NEXT((var), field), 1);                      \
       (var) = (tvar))

#define DTAILQ_FOREACH_REVERSE(var, head, headname, field)                    \
  for ((var) = DTAILQ_LAST((head), headname);                                 \
       (var);                                                                 \
       (var) = DTAILQ_PREV((var), headname, field))

#define DTAILQ_FOREACH_REVERSE_SAFE(var, head, headname, field, tvar)         \
  for ((var) = DTAILQ_LAST((head), headname);                                 \
       (var) && ((tvar) = DTAILQ_PREV((var), headname, field), 1);            \
       (var) = (tvar))

#define DTAILQ_INIT(head)                                                     \
do {                                                                          \
  DTAILQ_FIRST((head)) = NULL;                                                \
  (head)->tqh_last = &DTAILQ_FIRST((head));                                   \
} while (0)

#define DTAILQ_INSERT_AFTER(head, listelm, elm, field)                        \
do {                                                                          \
  if ((DTAILQ_NEXT((elm), field) = DTAILQ_NEXT((listelm), field)) != NULL)    \
    DTAILQ_NEXT((elm), field)->field.tqe_prev = &DTAILQ_NEXT((elm), field);   \
  else {                                                                      \
    (head)->tqh_last = &DTAILQ_NEXT((elm), field);                            \
  }                                                                           \
  DTAILQ_NEXT((listelm), field) = (elm);                                      \
  (elm)->field.tqe_prev = &TAILQ_NEXT((listelm), field);                      \
} while (0)

#define DTAILQ_INSERT_BEFORE(listelm, elm, field)                             \
do {                                                                          \
  (elm)->field.tqe_prev = (listelm)->field.tqe_prev;                          \
  DTAILQ_NEXT((elm), field) = (listelm);                                      \
  *(listelm)->field.tqe_prev = (elm);                                         \
  (listelm)->field.tqe_prev = &DTAILQ_NEXT((elm), field);                     \
} while (0)

#define DTAILQ_INSERT_HEAD(head, elm, field)                                  \
do {                                                                          \
  if ((DTAILQ_NEXT((elm), field) = DTAILQ_FIRST((head))) != NULL)             \
    DTAILQ_FIRST((head))->field.tqe_prev = &DTAILQ_NEXT((elm), field);        \
  else                                                                        \
    (head)->tqh_last = &DTAILQ_NEXT((elm), field);                            \
  DTAILQ_FIRST((head)) = (elm);                                               \
  (elm)->field.tqe_prev = &DTAILQ_FIRST((head));                              \
} while (0)

#define DTAILQ_INSERT_TAIL(head, elm, field)                                  \
do {                                                                          \
  DTAILQ_NEXT((elm), field) = NULL;                                           \
  (elm)->field.tqe_prev = (head)->tqh_last;                                   \
  *(head)->tqh_last = (elm);                                                  \
  (head)->tqh_last = &DTAILQ_NEXT((elm), field);                              \
} while (0)

#define DTAILQ_LAST(head, headname)                                           \
  (*(((struct dtailq_##headname##_s *)((head)->tqh_last))->tqh_last))

#define DTAILQ_NEXT(elm, field) ((elm)->field.tqe_next)

#define DTAILQ_PREV(elm, hname, field)                                        \
  (*(((struct dtailq_##hname##_s *)((elm)->field.tqe_prev))->tqh_last))

#define DTAILQ_REMOVE(head, elm, field)                                       \
do {                                                                          \
  if ((DTAILQ_NEXT((elm), field)) != NULL)                                    \
    DTAILQ_NEXT((elm), field)->field.tqe_prev = (elm)->field.tqe_prev;        \
  else {                                                                      \
    (head)->tqh_last = (elm)->field.tqe_prev;                                 \
  }                                                                           \
  *(elm)->field.tqe_prev = DTAILQ_NEXT((elm), field);                         \
} while (0)

#define DTAILQ_REVERSE(head, type_t, field)                                   \
do {                                                                          \
  type_t *curelm;                                                             \
  type_t *nextelm;                                                            \
  type_t *tailelm;                                                            \
                                                                              \
  tailelm = NULL;                                                             \
  (head)->tqh_last = &DTAILQ_FIRST((head));                                   \
  DTAILQ_FOREACH_SAFE(curelm, head, field, nextelm) {                         \
    if (tailelm == NULL)                                                      \
      (curelm)->field.tqe_prev = &DTAILQ_FIRST((head));                       \
    else                                                                      \
      (curelm)->field.tqe_prev = &DTAILQ_NEXT((tailelm), field);              \
    DTAILQ_NEXT(curelm, field) = tailelm;                                     \
    tailelm = curelm;                                                         \
  }                                                                           \
  DTAILQ_FIRST((head)) = tailelm;                                             \
} while (0)


#endif /* DTAILQ_H */

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
