/**
 ** @file inthash.h
 ** mini hash library, with unsigned long indices.
 **
 ** @author Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
 **
 ** @version 1.0
 ** @ingroup util
 **
 ** @date  Started on: Lun  8 fév 2016 14:50:44 UTC
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "gc.h"
#include "inthash.h"


inthash_t *new_inthash(gc_t *gc_ctx, size_t hsize)
{
  inthash_t *h;
  inthash_elmt_t **he;
  size_t i;

  h = gc_base_malloc(gc_ctx, sizeof (inthash_t));
  h->size = hsize;
  h->elmts = 0;
  h->htable = he = gc_base_malloc(gc_ctx, hsize * sizeof (inthash_elmt_t *));
  for (i=0; i<hsize; i++)
    he[i] = NULL;
  return h;
}


void clear_inthash(inthash_t *hash, void (*elmt_free)(void *e))
{
  size_t i, sz;
  inthash_elmt_t **he, *tmp;
  inthash_elmt_t *tmp_next;

  sz = hash->size;
  he = hash->htable;
  for (i = 0; i < sz; i++) {
    tmp = he[i];
    while (tmp!=NULL)
      {
	tmp_next = tmp->next;
	if (elmt_free!=NULL)
	  (*elmt_free) (tmp->data);
	gc_base_free (tmp);
	tmp = tmp_next;
      }
    he[i] = NULL;
  }
  hash->elmts = 0;
}

void free_inthash(inthash_t *hash, void (*elmt_free)(void *e))
{
  clear_inthash (hash, elmt_free);
  gc_base_free(hash->htable);
  gc_base_free(hash);
}

void inthash_add(gc_t *gc_ctx, inthash_t *hash,
		 void *data, unsigned long key)
{
  inthash_elmt_t *elmt;
  unsigned long hcode;

  elmt = gc_base_malloc(gc_ctx, sizeof (inthash_elmt_t));
  elmt->key = key;
  elmt->data = data;
  hcode = key % hash->size;
  elmt->next = hash->htable[hcode];
  hash->htable[hcode] = elmt;
  hash->elmts++;
}

void *inthash_get(inthash_t *hash, unsigned long key)
{
  inthash_elmt_t *elmt;

  elmt = hash->htable[key % hash->size];
  for (; elmt!=NULL; elmt = elmt->next)
    {
      if (key==elmt->key)
	return elmt->data;
    }
  return NULL;
}


/*
** Copyright (c) 2002-2005 by Julien OLIVAIN, Laboratoire Spécification
** et Vérification (LSV), CNRS UMR 8643 & ENS Cachan.
** Copyright (c) 2016 by Jean GOUBAULT-LARRECQ, Laboratoire Spécification
** et Vérification (LSV), CNRS UMR 8643 & ENS Cachan.
**
** Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
** Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
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
