/**
 ** @file objpool.c
 ** Object pools.
 ** 
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 ** 
 ** @version 0.1.0
 ** @ingroup util
 ** 
 ** @date  Started on: Wed Apr 14 17:10:14 2004
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>

#include "objpool.h"

#define POOLENTRY_INUSE 0x01

/* 
 * declare a pool in a container structure
 */
#define OBJPOOL(name, type_t)                                                 \
struct objpool_##name##_s {                                                   \
  size_t alloc;                                                               \
  size_t lastid;                                                              \
  size_t elmts;                                                               \
  size_t grow;                                                                \
  size_t shrink;                                                              \
  type_t *objs;                                                               \
}


#define OBJPOOL_IS_INUSE(elm) ((elm)->poolflags & POOLENTRY_INUSE)

#define OBJPOOL_KILLOBJ(obj)  ((obj)->poolflags &= ~(POOLENTRY_INUSE))

/*
 * declare pool element infos in an element structure
 */
/* #define OBJPOOL_ENTRY(name)        \ */
/* struct objpoolentry_##name##_s {   \ */
/*   unsigned long flags;             \ */
/* } */
#define OBJPOOL_ENTRY  unsigned long poolflags

#define OBJPOOL_REMOVE(pool, obj)                                             \
do {                                                                          \
  if (OBJPOOL_IS_INUSE(obj)) {                                                \
    OBJPOOL_KILLOBJ(obj);                                                     \
    (pool)->elmts--;                                                          \
  }                                                                           \
} while (0)

/*
 * get a new object ptr
 * cptr: container pointer
 * pname: pool name
 */
#define GET_POOL_OBJ(pool, destptr)                                           \
do {                                                                          \
 size_t newid;                                                                \
 if ((pool)->elmts >= (pool)->alloc - 1) {                                    \
   (pool)->alloc += (pool)->grow;                                             \
   (pool)->objs = realloc( (pool)->objs, (pool)->alloc * POOLELMT_SIZEOF(pool)); \
 }                                                                            \
                                                                              \
 for (newid = (pool)->lastid;                                                 \
     ( OBJPOOL_IS_INUSE(&(pool)->objs[ newid ]) ) &&                    \
       newid != (pool)->lastid - 1; ) {                                       \
    newid++;                                                                  \
    if (newid == (pool)->alloc)                                               \
      newid = 0;                                                              \
  }                                                                           \
  (pool)->lastid = newid;                                                     \
  (destptr) = &((pool)->objs[ newid ]);                                       \
  (destptr)->poolflags |= POOLENTRY_INUSE;                                    \
  (pool)->elmts++;                                                            \
} while (0)

#define GET_OBJECT_ID(pool, objptr)     ( (objptr) - (pool)->objs )

#define OBJPOOL_INIT(pool, initsz)                                            \
do {                                                                          \
 (pool)->alloc = initsz;                                                      \
 (pool)->grow = initsz;                                                       \
 (pool)->elmts = 0;                                                           \
 (pool)->lastid = 0;                                                          \
 (pool)->objs = calloc((initsz), POOLELMT_SIZEOF(pool) );                     \
} while (0)

#define OBJPOOL_GET_BYID(pool, id) (&((pool)->objs[ (id) ]))

#define POOLELMT_SIZEOF(pool)   (sizeof (*((pool)->objs)))

#define POOL_ELEMENTS(pool)          ((pool)->elmts)

typedef struct myelmt_s myelmt_t;
struct myelmt_s {
  OBJPOOL_ENTRY;
  int myint;
  double blah;
};

typedef struct mycontainer_s mycontainer_t;
struct mycontainer_s {
  OBJPOOL(mypool, myelmt_t) mypool;
  int myint;
  char *mystr;
};

myelmt_t *
objpool_get(mycontainer_t *ctx)
{
  myelmt_t *ptr;

  GET_POOL_OBJ(&ctx->mypool, ptr);

  return (ptr);
}

void
objpool_init(mycontainer_t *ctx, size_t s)
{
  OBJPOOL_INIT(&ctx->mypool, s);
}

int
main(int argc, char *argv[])
{
  int i, j;
  mycontainer_t *ctx;
  myelmt_t *e;
  int rnd;

  ctx = malloc(sizeof (mycontainer_t));

  OBJPOOL_INIT(&ctx->mypool, 131072);

  ctx->mypool.grow = 131072;

/*   printf("sizeof (mycontainer_t) = %i\n", sizeof (mycontainer_t)); */
/*   printf("sizeof (pool elmt) = %i\n", POOLELMT_SIZEOF(ctx, mypool)); */

  rnd = (rand() % 10) + 4;
  for (i = 0; i < (1 << 20); i++) {
    GET_POOL_OBJ(&ctx->mypool, e);
/*     printf("# got objectid %i @ %p (%i elmts %i alloc)\n", GET_OBJECT_ID(ctx, mypool, e), (void *) e, POOL_ELEMENTS(ctx, mypool), ctx->mypool.alloc); */
/*     printf("%i %i %i %i\n", i, GET_OBJECT_ID(&ctx->mypool, e), POOL_ELEMENTS(&ctx->mypool), ctx->mypool.alloc); */
    if (!(i % rnd)) {
/*       printf("#  destroy objid %i @ %p\n", GET_OBJECT_ID(ctx, mypool, &ctx->mypool.pool[ i / rnd ]), (void *) &ctx->mypool.pool[ i / rnd ]); */
      OBJPOOL_REMOVE(&ctx->mypool, &ctx->mypool.objs[ i / rnd ] );
      rnd = (rand() % 10) + 4;
    }

    if (i == 1 << 19) {
      for (j = 0; j < 1 << 18; j++) {
        OBJPOOL_REMOVE(&ctx->mypool, &ctx->mypool.objs[ j ] );
      }
    }

  }

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
