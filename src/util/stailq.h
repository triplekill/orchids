/**
 ** @file stailq.h
 ** Macros for singly linked queues.
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


#ifndef STAILQ_H
#define STAILQ_H

#include "offsetof.h"

/* list macros. inspired from FreeBSD <sys/queue.h> */


/*
 * Singly-linked Tail queue declarations.
 */
#define STAILQ_HEAD(name, type_t)                                             \
struct stailq_##name##_s {                                                    \
  type_t  *stqh_first;                                                        \
  type_t **stqh_last;                                                         \
}

#define STAILQ_HEAD_INITIALIZER(head)                                         \
  { NULL, &(head).stqh_first }

#define STAILQ_ENTRY(type_t)                                                  \
struct {                                                                      \
  type_t *stqe_next;                                                          \
}

/*
 * Singly-linked Tail queue functions.
 */
#define STAILQ_CONCAT(head1, head2)                                           \
do {                                                                          \
  if (!STAILQ_IS_EMPTY((head2))) {                                            \
    *(head1)->stqh_last = (head2)->stqh_first;                                \
    (head1)->stqh_last = (head2)->stqh_last;                                  \
    STAILQ_INIT((head2));                                                     \
  }                                                                           \
} while (0)

#define STAILQ_IS_EMPTY(head)  ((head)->stqh_first == NULL)

#define STAILQ_FIRST(head)  ((head)->stqh_first)

#define STAILQ_FOREACH(var, head, field)                                      \
  for((var) = STAILQ_FIRST((head));                                           \
      (var);                                                               \
      (var) = STAILQ_NEXT((var), field))


#define STAILQ_FOREACH_SAFE(var, head, field, tvar)                           \
  for ((var) = STAILQ_FIRST((head));                                          \
       (var) && ((tvar) = STAILQ_NEXT((var), field), 1);                      \
       (var) = (tvar))

#define STAILQ_INIT(head)                                                     \
do {                                                                          \
  STAILQ_FIRST((head)) = NULL;                                                \
  (head)->stqh_last = &STAILQ_FIRST((head));                                  \
} while (0)

#define STAILQ_INSERT_AFTER(head, tqelm, elm, field)                          \
do {                                                                          \
  if ((STAILQ_NEXT((elm), field) = STAILQ_NEXT((tqelm), field)) == NULL)      \
      (head)->stqh_last = &STAILQ_NEXT((elm), field);                         \
  STAILQ_NEXT((tqelm), field) = (elm);                                        \
} while (0)

#define STAILQ_INSERT_HEAD(head, elm, field)                                  \
do {                                                                          \
  if ((STAILQ_NEXT((elm), field) = STAILQ_FIRST((head))) == NULL)             \
      (head)->stqh_last = &STAILQ_NEXT((elm), field);                         \
  STAILQ_FIRST((head)) = (elm);                                               \
} while (0)

#define STAILQ_INSERT_TAIL(head, elm, field)                                  \
do {                                                                          \
  STAILQ_NEXT((elm), field) = NULL;                                           \
  *(head)->stqh_last = (elm);                                                 \
  (head)->stqh_last = &STAILQ_NEXT((elm), field);                             \
} while (0)

#define STAILQ_LAST(head, type_t, field)                                      \
(STAILQ_IS_EMPTY((head)) ?                                                    \
  NULL :                                                                      \
  ((type_t *) ((char *)((head)->stqh_last) - __OFFSETOF(type_t, field))))

#define STAILQ_NEXT(elm, field)   ((elm)->field.stqe_next)

#define STAILQ_REMOVE(head, elm, type, field)                                 \
do {                                                                          \
  if (STAILQ_FIRST((head)) == (elm)) {                                        \
    STAILQ_REMOVE_HEAD((head), field);                                        \
  } else {                                                                    \
    type_t *curelm = STAILQ_FIRST((head));                                    \
    while (STAILQ_NEXT(curelm, field) != (elm))                               \
      curelm = STAILQ_NEXT(curelm, field);                                    \
    if ((STAILQ_NEXT(curelm, field) =                                         \
            STAILQ_NEXT(STAILQ_NEXT(curelm, field), field)) == NULL)          \
    (head)->stqh_last = &STAILQ_NEXT((curelm), field);                        \
  }                                                                           \
} while (0)

#define STAILQ_REMOVE_HEAD(head, field)                                       \
do {                                                                          \
  if ((STAILQ_FIRST((head)) =                                                 \
           STAILQ_NEXT(STAILQ_FIRST((head)), field)) == NULL)                 \
    (head)->stqh_last = &STAILQ_FIRST((head));                                \
} while (0)

#define STAILQ_REMOVE_HEAD_UNTIL(head, elm, field)                            \
do {                                                                          \
  if ((STAILQ_FIRST((head)) = STAILQ_NEXT((elm), field)) == NULL)             \
    (head)->stqh_last = &STAILQ_FIRST((head));                                \
} while (0)

#define STAILQ_REVERSE(head, type_t, field)                                   \
do {                                                                          \
  type_t *curelm;                                                             \
  type_t *nextelm;                                                            \
  type_t *tailelm;                                                            \
                                                                              \
  tailelm = NULL;                                                             \
  (head)->stqh_last = &STAILQ_FIRST((head));                                  \
  STAILQ_FOREACH_SAFE(curelm, head, field, nextelm) {                         \
    STAILQ_NEXT(curelm, field) = tailelm;                                     \
    tailelm = curelm;                                                         \
  }                                                                           \
  STAILQ_FIRST((head)) = tailelm;                                             \
} while (0)


#endif /* STAILQ_H */

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
