/**
 ** @file dlist.h
 ** Macros for lists and queues.
 ** 
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 ** 
 ** @version 0.1.0
 ** 
 ** @date  Started on: Mon Jan 13 10:09:19 2003
 ** @date Last update: Tue Nov 29 11:24:47 2005
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */


#ifndef DLIST_H
#define DLIST_H

#define __OFFSETOF(type, field)  ((size_t)(&((type *)0)->field))


/* list macros. inspired from FreeBSD <sys/queue.h> */


#define DLIST_HEAD(name, type_t)                                              \
struct dlist_##name##_s {                                                     \
  type_t *lh_first;                                                           \
}

#define DLIST_HEAD_INITIALIZER(head)                                          \
  { NULL }

#define DLIST_ENTRY(type_t)                                                   \
struct {                                                                      \
  type_t *le_next;                                                            \
  type_t **le_prev;                                                           \
}



#define DLIST_IS_EMPTY(head)  ((head)->lh_first == NULL)

#define DLIST_FIRST(head)     ((head)->lh_first)

#define DLIST_FOREACH(var, head, field)                                       \
  for ((var) = DLIST_FIRST((head));                                           \
       (var);                                                                 \
       (var) = DLIST_NEXT((var), field))

#define DLIST_FOREACH_SAFE(var, head, field, tvar)                            \
  for ((var) = DLIST_FIRST((head));                                           \
       (var) && ((tvar) = DLIST_NEXT((var), field), 1);                       \
       (var) = (tvar))

#define DLIST_INIT(head)                                                      \
do {                                                                          \
  DLIST_FIRST((head)) = NULL;                                                 \
} while (0)

#define DLIST_INSERT_AFTER(listelm, elm, field)                               \
do {                                                                          \
  if ((DLIST_NEXT((elm), field) = DLIST_NEXT((listelm), field)) != NULL)      \
    DLIST_NEXT((listelm), field)->field.le_prev = &DLIST_NEXT((elm), field);  \
  DLIST_NEXT((listelm), field) = (elm);                                       \
  (elm)->field.le_prev = &DLIST_NEXT((listelm), field);                       \
} while (0)

#define DLIST_INSERT_BEFORE(listelm, elm, field)                              \
do {                                                                          \
  (elm)->field.le_prev = (listelm)->field.le_prev;                            \
  DLIST_NEXT((elm), field) = (listelm);                                       \
  *(listelm)->field.le_prev = (elm);                                          \
  (listelm)->field.le_prev = &DLIST_NEXT((elm), field);                       \
} while (0)

#define DLIST_INSERT_HEAD(head, elm, field)                                   \
do {                                                                          \
  if ((DLIST_NEXT((elm), field) = DLIST_FIRST((head))) != NULL)               \
    DLIST_FIRST((head))->field.le_prev = &DLIST_NEXT((elm), field);           \
  DLIST_FIRST((head)) = (elm);                                                \
  (elm)->field.le_prev = &DLIST_FIRST((head));                                \
} while (0)

#define DLIST_NEXT(elm, field)   ((elm)->field.le_next)

#define DLIST_REMOVE(elm, field)                                              \
do {                                                                          \
  if (DLIST_NEXT((elm), field) != NULL)                                       \
    DLIST_NEXT((elm), field)->field.le_prev = (elm)->field.le_prev;           \
  *(elm)->field.le_prev = DLIST_NEXT((elm), field);                           \
} while (0)


















/*
 * Singly-linked List declarations.
 */
#define SLIST_HEAD(name, type_t)                                              \
struct slist_##name##_t {                                                     \
  type_t *slh_first;                                                          \
}

#define SLIST_HEAD_INITIALIZER(head)                                          \
  { NULL }

#define SLIST_ENTRY(type_t)                                                   \
struct {                                                                      \
  type_t *sle_next;                                                           \
}

/*
 * Singly-linked List functions.
 */
#define SLIST_IS_EMPTY(head)       ((head)->slh_first == NULL)

#define SLIST_FIRST(head)       ((head)->slh_first)

#define SLIST_FOREACH(var, head, field)                                       \
  for ((var) = SLIST_FIRST((head));                                           \
       (var);                                                                 \
       (var) = SLIST_NEXT((var), field))

#define SLIST_FOREACH_SAFE(var, head, field, tvar)                            \
  for ((var) = SLIST_FIRST((head));                  \
       (var) && ((tvar) = SLIST_NEXT((var), field), 1);                       \
       (var) = (tvar))

#define SLIST_FOREACH_PREVPTR(var, varp, head, field)                         \
  for ((varp) = &SLIST_FIRST((head));                                         \
       ((var) = *(varp)) != NULL;                                             \
        (varp) = &SLIST_NEXT((var), field))

#define SLIST_INIT(head)                                                      \
do {                                                                          \
  SLIST_FIRST((head)) = NULL;                                                 \
} while (0)

#define SLIST_INSERT_AFTER(slistelm, elm, field)                              \
do {                                                                          \
  SLIST_NEXT((elm), field) = SLIST_NEXT((slistelm), field);                   \
  SLIST_NEXT((slistelm), field) = (elm);                                      \
} while (0)

#define SLIST_INSERT_HEAD(head, elm, field)                                   \
do {                                                                          \
  SLIST_NEXT((elm), field) = SLIST_FIRST((head));                             \
  SLIST_FIRST((head)) = (elm);                                                \
} while (0)

#define SLIST_NEXT(elm, field) ((elm)->field.sle_next)

#define SLIST_REMOVE(head, elm, type_t, field)                                \
do {                                                                          \
  if (SLIST_FIRST((head)) == (elm)) {                                         \
    SLIST_REMOVE_HEAD((head), field);                                         \
  } else {                                                                    \
    type_t *curelm = SLIST_FIRST((head));                                     \
    while (SLIST_NEXT(curelm, field) != (elm))                                \
      curelm = SLIST_NEXT(curelm, field);                                     \
    SLIST_NEXT(curelm, field) = SLIST_NEXT(SLIST_NEXT(curelm, field), field); \
  }                                                                           \
} while (0)

#define SLIST_REMOVE_HEAD(head, field)                                        \
do {                                                  \
  SLIST_FIRST((head)) = SLIST_NEXT(SLIST_FIRST((head)), field);               \
} while (0)






















/*
 * Singly-linked Tail queue declarations.
 */
#define STAILQ_HEAD(name, type_t)                                             \
struct stailq_##name##_t {            \
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
  if ((DTAILQ_NEXT((elm), field) = TAILQ_FIRST((head))) != NULL)              \
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
  (*(((struct dtailq_##headname##_t *)((head)->tqh_last))->tqh_last))

#define DTAILQ_NEXT(elm, field) ((elm)->field.tqe_next)

#define DTAILQ_PREV(elm, hname, field)                                        \
  (*(((struct dtailq_##hname##_t *)((elm)->field.tqe_prev))->tqh_last))

#define DTAILQ_REMOVE(head, elm, field)                                       \
do {                                                                          \
  if ((DTAILQ_NEXT((elm), field)) != NULL)                                    \
    DTAILQ_NEXT((elm), field)->field.tqe_prev = (elm)->field.tqe_prev;        \
  else {                                                                      \
    (head)->tqh_last = (elm)->field.tqe_prev;                                 \
  }                                                                           \
  *(elm)->field.tqe_prev = DTAILQ_NEXT((elm), field);                         \
} while (0)


#endif /* DLIST_H */

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
