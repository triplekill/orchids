/**
 ** @file dlist.c
 ** 
 ** 
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 ** 
 ** @version 0.1.0
 ** @ingroup util
 ** 
 ** @date  Started on: Mon Apr 19 10:16:40 2004
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "dlist.h"


/* #include "bsdqueue.h" */


#if 0

#define DLIST_HEAD(name, type_t)                                              \
struct dlist_##name##_s {                                                     \
  type_t  *lh_first;                                                          \
}

#define DLIST_ENTRY(type_t)                                                   \
struct dlist_entry_##name##_s {                                               \
  type_t  *next;                                                              \
  type_t **prev;                                                              \
}

#define DLIST_NEXT(listname, elmt) \
  ((elmt)->listname.next)

#define DLIST_PREV(listname, elmt) \
  ((elmt)->listname.prev)

#define DLIST_HEAD(ctx, dlist) \
  ((ctx)->dlist.head)

#define DLIST_TAIL(ctx, dlist) \
  ( (ctx)->dlist.tail )

#define DLIST_COUNT(ctx, dlist) \
  ( (ctx)->dlist.count )



#define DLIST_FOR_EACH(pos, ctx, dlist) \
  for (pos = DLIST_HEAD(ctx, dlist) ; pos; pos = DLIST_NEXT(dlist, pos))

#define DLIST_FOR_EACH_REV(pos, ctx, dlist) \
  for (pos = DLIST_TAIL(ctx, dlist) ; pos; pos = DLIST_PREV(dlist, pos))



#define DLIST_IS_EMPTY(ctx, dlist) \
  ( (DLIST_HEAD(ctx, dlist) == NULL) && (DLIST_TAIL(ctx, dlist) == NULL) )



#define DLIST_INS_HEAD(ctx, dlist, elmt)                                      \
do {                                                                          \
  if ((ctx)->dlist.head) {                                                    \
    (ctx)->dlist.head->dlist.prev = (elmt);                                   \
    (elmt)->dlist.next = (ctx)->dlist.head;                                   \
    (elmt)->dlist.prev = NULL;                                                \
    (ctx)->dlist.head = (elmt);                                               \
  } else {                                                                    \
    (elmt)->dlist.next = NULL;                                                \
    (elmt)->dlist.prev = NULL;                                                \
    (ctx)->dlist.head = (elmt);                                               \
    (ctx)->dlist.tail = (elmt);                                               \
  }                                                                           \
  DLIST_COUNT(ctx, dlist)++;                                                  \
} while (0)

#define DLIST_INS_TAIL(ctx, dlist, elmt)     \
do {                                         \
  if ((ctx)->dlist.tail) {                   \
    (ctx)->dlist.tail->dlist.next = (elmt);  \
    (elmt)->dlist.prev = (ctx)->dlist.tail;  \
    (elmt)->dlist.next = NULL;               \
    (ctx)->dlist.tail = (elmt);              \
  } else {                                   \
    (elmt)->dlist.next = NULL;               \
    (elmt)->dlist.prev = NULL;               \
    (ctx)->dlist.head = (ctx)->dlist.tail = (elmt); \
  } \
  DLIST_COUNT(ctx, dlist)++; \
} while (0)

#define DLIST_INS_NEXT(ctx, dlist, pos, newelmt)                              \
do {                                                                          \
  DLIST_PREV(dlist, newelmt) = (pos);                                         \
  DLIST_NEXT(dlist, newelmt) = DLIST_NEXT(dlist, pos);                        \
  if ( DLIST_NEXT(dlist, pos) ) {                                             \
    DLIST_PREV(dlist, DLIST_NEXT(dlist, pos)) = (newelmt);                    \
  } else {                                                                    \
    DLIST_TAIL(ctx, dlist) = (newelmt);                                       \
  }                                                                           \
  DLIST_NEXT(dlist, pos) = (newelmt);                                         \
  DLIST_COUNT(ctx, dlist)++;                                                  \
} while (0)

#define DLIST_INS_PREV(ctx, dlist, pos, newelmt)                              \
do {                                                                          \
  DLIST_NEXT(dlist, newelmt) = (pos);                                         \
  DLIST_PREV(dlist, newelmt) = DLIST_PREV(dlist, pos);                        \
  if ( DLIST_PREV(dlist, pos) ) {                                             \
    DLIST_NEXT(dlist, DLIST_PREV(dlist, pos)) = (newelmt);                    \
  } else {                                                                    \
    DLIST_HEAD(ctx, dlist) = (newelmt);                                       \
  }                                                                           \
  DLIST_PREV(dlist, pos) = (newelmt);                                         \
  DLIST_COUNT(ctx, dlist)++;                                                  \
} while (0)



#define DLIST_UNLINK(ctx, dlist, elmt) \
do {                                                                          \
                                                                              \
  if ( DLIST_PREV(dlist, elmt) ) {                                            \
    DLIST_NEXT(dlist, DLIST_PREV(dlist, elmt)) = DLIST_NEXT(dlist, elmt);     \
  } else {                                                                    \
    DLIST_HEAD(ctx, dlist) = DLIST_NEXT(dlist, elmt);                         \
  }                                                                           \
                                                                              \
  if ( DLIST_NEXT(dlist, elmt) ) {                                            \
    DLIST_PREV(dlist, DLIST_NEXT(dlist, elmt)) = DLIST_PREV(dlist, elmt);     \
  } else {                                                                    \
    DLIST_TAIL(ctx, dlist) = DLIST_PREV(dlist, elmt);                         \
  }                                                                           \
                                                                              \
  DLIST_COUNT(ctx, dlist)--; \
} while (0)



#define DLIST_INIT(ctx, dlist)                                                \
do {                                                                          \
  DLIST_HEAD(ctx, dlist) = NULL;                                              \
  DLIST_TAIL(ctx, dlist) = NULL;                                              \
  DLIST_COUNT(ctx, dlist) = 0;                                                \
} while (0)


#endif /* 0 */





#if 0

typedef struct myelmt_s myelmt_t;
struct myelmt_s
{
  int blah;
  double bloh;
  DECLARE_DLIST_ELMT(taillist, myelmt_t);
  DECLARE_DLIST_ELMT(headlist, myelmt_t);
  char *bluh;
};

typedef struct myctx_s myctx_t;
struct myctx_s
{
  int titi;
  char *toto;
  DECLARE_DLIST(headlist, myelmt_t);
  DECLARE_DLIST(taillist, myelmt_t);
};

#endif

double
prob_rand(void)
{
  return ((rand()) / ((double)RAND_MAX));
}

int
rand_range(int min, int max)
{
  return ( (int) (prob_rand() * (double)(max - min) + (double)min) );
}

#define ACTION_INSHEAD 0
#define ACTION_UNLINK1 1
#define ACTION_INSTAIL 2
#define ACTION_UNLINK2 3
#define ACTION_INSNEXT 4
#define ACTION_UNLINK3 5
#define ACTION_INSPREV 6
#define ACTION_UNLINK4 7
#define ACTION_SHOWLIST 8
#define ACTION_UNLINK5 9

#if 0
int
main(int argc, char *argv[])
{
  myelmt_t *newelmt;
  myelmt_t *elmt;
  myctx_t *ctx;
  int i;
  int action;
  int j;
  int r;

  ctx = malloc(sizeof (myctx_t));

  DLIST_INIT(ctx, headlist);
  DLIST_INIT(ctx, taillist);

  for ( i = 0; i < 50000000 ; i++) {
    action = rand_range(0, 9);
    printf("action: %i\n", action);
    switch (action) {
    case ACTION_INSHEAD:
      elmt = malloc(sizeof (myelmt_t));
      elmt->blah = i;
      DLIST_INS_HEAD(ctx, headlist, elmt);
      break;

    case ACTION_INSTAIL:
      elmt = malloc(sizeof (myelmt_t));
      elmt->blah = i;
      DLIST_INS_TAIL(ctx, headlist, elmt);
      break;

    case ACTION_INSNEXT:
      printf("* insert after a random element.\n");
      newelmt = malloc(sizeof (myelmt_t));
      newelmt->blah = i;
      if (DLIST_IS_EMPTY(ctx, headlist)) {
        printf("    list is empty\n");
        DLIST_INS_HEAD(ctx, headlist, newelmt);
        continue ;
      }
      r = rand_range(0, DLIST_COUNT(ctx, headlist));
      for (j = 0, elmt = DLIST_HEAD(ctx, headlist) ;
           j < r;
           j++, elmt = DLIST_NEXT(headlist, elmt))
        ;
      DLIST_INS_NEXT(ctx, headlist, elmt, newelmt);
      break;

    case ACTION_INSPREV:
      printf("* insert before a random element.\n");
      newelmt = malloc(sizeof (myelmt_t));
      newelmt->blah = i;
      if (DLIST_IS_EMPTY(ctx, headlist)) {
        printf("    list is empty\n");
        DLIST_INS_HEAD(ctx, headlist, newelmt);
        continue ;
      }
      r = rand_range(0, DLIST_COUNT(ctx, headlist));
      for (j = 0, elmt = DLIST_HEAD(ctx, headlist) ;
           j < r;
           j++, elmt = DLIST_NEXT(headlist, elmt))
        ;
      DLIST_INS_PREV(ctx, headlist, elmt, newelmt);
      break;

    case ACTION_UNLINK1:
    case ACTION_UNLINK2:
    case ACTION_UNLINK3:
    case ACTION_UNLINK4:
    case ACTION_UNLINK5:
      printf("* unlink a random element.\n");
      if (DLIST_IS_EMPTY(ctx, headlist)) {
        printf("    list is empty\n");
        continue ;
      }
      r = rand_range(0, DLIST_COUNT(ctx, headlist));
      for (j = 0, elmt = DLIST_HEAD(ctx, headlist) ;
           j < r;
           j++, elmt = DLIST_NEXT(headlist, elmt))
        ;
      DLIST_UNLINK(ctx, headlist, elmt);
      free(elmt);
      break;

    case ACTION_SHOWLIST:
      printf("* show list content: (%i elements)\n", DLIST_COUNT(ctx, headlist));
      if ( DLIST_IS_EMPTY(ctx, headlist) ) {
        printf("    list is empty\n");
      } else {
        j = 0;
        DLIST_FOR_EACH(elmt, ctx, headlist) {
          printf("  elmt %i: val: %i\n", j, elmt->blah);
          j++;
        }
      }
      break;
    }
  }

  return (0);
}
#endif









typedef struct mybsdelmt_s mybsdelmt_t;
struct mybsdelmt_s
{
  int blah;
  double bloh;
  DLIST_ENTRY(mybsdelmt_t) entries;
  STAILQ_ENTRY(mybsdelmt_t) stq;
  char *bluh;
};


typedef struct mybsdctx_s mybsdctx_t;
struct mybsdctx_s
{
  int titi;
  char *toto;
  DLIST_HEAD(slisthead, mybsdelmt_t) head;
  STAILQ_HEAD(stq, mybsdelmt_t) stqh;
};

void
dlist_insert_head(mybsdctx_t *ctx, mybsdelmt_t *elmt)
{
  DLIST_INSERT_HEAD(&(ctx->head), elmt, entries);
}

int
main(int argc, char *argv[])
{
  mybsdelmt_t *newelmt;
  mybsdelmt_t *elmt;
  mybsdctx_t *ctx;
  int i;
  int action;
  int j;
  int r;

  ctx = malloc(sizeof (mybsdctx_t));

  DLIST_INIT(&(ctx->head));
  STAILQ_INIT(&(ctx->stqh));

  for (i = 0; i < 10000000; i++) {
    elmt = malloc(sizeof (mybsdelmt_t));
    elmt->blah = i;
    DLIST_INSERT_HEAD(&(ctx->head), elmt, entries);
    STAILQ_INSERT_TAIL(&(ctx->stqh), elmt, stq);
  }

/*   DLIST_FOREACH(elmt, &(ctx->head), entries) { */
/*     printf("i: %i\n", elmt->blah); */
/*   } */

/*   STAILQ_FOREACH(elmt, &(ctx->stqh), stq) { */
/*     printf("i: %i\n", elmt->blah); */
/*   } */


  return (0);
}


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
