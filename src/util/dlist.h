/**
 ** @file dlist.h
 ** Macros for doubly linked lists.
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


#ifndef DLIST_H
#define DLIST_H

#include "offsetof.h"

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

#define DLIST_REVERSE(head, type_t, field)                                    \
do {                                                                          \
  type_t *curelm;                                                             \
  type_t *nextelm;                                                            \
  type_t *tailelm;                                                            \
                                                                              \
  tailelm = NULL;                                                             \
  DLIST_FOREACH_SAFE(curelm, head, field, nextelm) {                          \
    if (tailelm == NULL)                                                      \
      (curelm)->field.le_prev = &DLIST_FIRST((head));                         \
    else                                                                      \
      (curelm)->field.le_prev = &DLIST_NEXT((tailelm), field);                \
    DLIST_NEXT(curelm, field) = tailelm;                                      \
    tailelm = curelm;                                                         \
  }                                                                           \
  DLIST_FIRST((head)) = tailelm;                                              \
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
