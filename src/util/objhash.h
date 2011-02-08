/**
 ** @file objhash.h
 ** Header for mini object hash library.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup util
 **
 ** @date  Started on: Mon Jan 20 16:41:49 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef OBJHASH_H
#define OBJHASH_H

#define DEFAULT_OBJHASH_FUNCTION objhash_pjw
#define DEFAULT_OBJCMP_FUNCTION objhash_cmp

typedef struct objhash_elmt_s objhash_elmt_t;
struct objhash_elmt_s
{
  void *key;
  void *data;
  objhash_elmt_t *next;
};

typedef unsigned long (*objhashfunc_t)(void *key);
typedef int (*objhashcmp_t)(void *obj1, void *obj2);

typedef struct objhash_s objhash_t;
struct objhash_s
{
  objhash_elmt_t **htable;
  size_t size;
  int elmts;
  objhashfunc_t hash;
  objhashcmp_t cmp;
};

objhash_t *new_objhash(size_t hsize);
void objhash_resize(objhash_t *hash, size_t newsize);
void objhash_add(objhash_t *hash, void *data, void *key);
void *objhash_get(objhash_t *hash, void *key);
void *objhash_del(objhash_t *hash, void *key);
int objhash_walk(objhash_t *hash, int (func)(void *elmt, void *data), void *data);
int objhash_collide_count(objhash_t *hash);

unsigned long objhash_pjw(void *key);
int objhash_cmp(void *obj1, void *obj2);

#endif /* OBJHASH_H */



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
