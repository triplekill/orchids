/**
 ** @file strhash.h
 ** Header for string hash functions.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup util
 **
 ** @date  Started on: Mon Mar 31 12:26:30 2003
 ** @date Last update: Tue Nov 29 11:38:20 2005
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef STRHASH_H
#define STRHASH_H

#define DEFAULT_STRHASH_FUNCTION strhash_pjw

typedef struct strhash_elmt_s strhash_elmt_t;
struct strhash_elmt_s
{
  char *key;
  void *data;
  strhash_elmt_t *next;
};

typedef unsigned long (*strhashfunc_t)(char *key);

typedef struct strhash_s strhash_t;
struct strhash_s
{
  strhash_elmt_t **htable;
  size_t size;
  int elmts;
  strhashfunc_t hash;
};

strhash_t *new_strhash(size_t hsize);
void free_strhash(strhash_t *hash, void (*elmt_free)(void *e));
void clear_strhash(strhash_t *hash, void (*elmt_free)(void *e));
void *strhash_to_array(strhash_t *hash);
void strhash_resize(strhash_t *hash, size_t newsize);
void strhash_add(strhash_t *hash, void *data, char *key);
void *strhash_get(strhash_t *hash, char *key);
int strhash_walk(strhash_t *hash, int (func)(void *elmt, void *data), void *data);
int strhash_collide_count(strhash_t *hash);

unsigned long strhash_pjw(char *key);
unsigned long strhash_pow(char *key);
unsigned long strhash_x65599(char *key);
unsigned long strhash_quad(char *key);

#endif /* HASH_H */



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
