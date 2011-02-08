/**
 ** @file slist.h
 ** Macros for singly linked lists.
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


#ifndef SLIST_H
#define SLIST_H

#include "offsetof.h"

/* list macros. inspired from FreeBSD <sys/queue.h> */


/*
 * Singly-linked List declarations.
 */
#define SLIST_HEAD(name, type_t)                                              \
struct slist_##name##_s {                                                     \
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
do {                                                                          \
  SLIST_FIRST((head)) = SLIST_NEXT(SLIST_FIRST((head)), field);               \
} while (0)

#define SLIST_REVERSE(head, type_t, field)                                    \
do {                                                                          \
  type_t *curelm;                                                             \
  type_t *nextelm;                                                            \
  type_t *tailelm;                                                            \
                                                                              \
  tailelm = NULL;                                                             \
  SLIST_FOREACH_SAFE(curelm, head, field, nextelm) {                          \
    SLIST_NEXT(curelm, field) = tailelm;                                      \
    tailelm = curelm;                                                         \
  }                                                                           \
  SLIST_FIRST((head)) = tailelm;                                              \
} while (0)


#endif /* SLIST_H */

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
