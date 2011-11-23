/**
 ** @file objhash.c
 ** Object hash utility functions.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup util
 **
 ** @date  Started on: Mon Jan 20 16:41:14 2003
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

#include "safelib.h"

#include "objhash.h"

/*
 * Default behaviour for hashing strings.
 */
objhash_t *
new_objhash(size_t hsize)
{
  objhash_t *h;

  h = Xmalloc(sizeof (objhash_t));
  h->size = hsize;
  h->elmts = 0;
  h->hash = DEFAULT_OBJHASH_FUNCTION;
  h->cmp = DEFAULT_OBJCMP_FUNCTION;
  h->htable = Xzmalloc(hsize * sizeof (objhash_elmt_t *));

  return (h);
}

void
objhash_resize(objhash_t *hash, size_t newsize)
{
  objhash_elmt_t **old_htable;
  int i;

  /* XXX: Not tested !!! */
  printf("Not tested !...\n");

  old_htable = hash->htable;
  hash->htable = Xzmalloc(newsize * sizeof (objhash_elmt_t *));

  for (i = 0; i < hash->size; ++i)
    {
      objhash_elmt_t *tmp;

      for (tmp = old_htable[i]; tmp; tmp = tmp->next)
        {
          unsigned long hcode;

          hcode = hash->hash(tmp->key) % newsize;
          tmp->next = hash->htable[hcode];
          hash->htable[hcode] = tmp;
        }
    }

  Xfree(old_htable);
  hash->size = newsize;
}

void
objhash_add(objhash_t *hash, void *data, void *key)
{
  objhash_elmt_t *elmt;
  unsigned long hcode;

  elmt = Xmalloc(sizeof (objhash_elmt_t));
  elmt->key = key;
  elmt->data = data;

  hcode = hash->hash(key) % hash->size;
  elmt->next = hash->htable[hcode];
  hash->htable[hcode] = elmt;
  hash->elmts++;
}

void *
objhash_del(objhash_t *hash, void *key)
{
  objhash_elmt_t **head;
  objhash_elmt_t  *elmt;
  objhash_elmt_t  *prev;
  void            *data;

  prev = NULL;
  head = &hash->htable[hash->hash(key) % hash->size];
  for (elmt = *head; elmt; elmt = elmt->next) {
    if (!hash->cmp(key, elmt->key)) {
      data = elmt->data;
      if (prev)
        prev->next = elmt->next;
      else
        *head = elmt->next;
      Xfree(elmt);

      return (data);
    }
    prev = elmt;
  }

  return (NULL);
}


void *
objhash_get(objhash_t *hash, void *key)
{
  objhash_elmt_t *elmt;

  elmt = hash->htable[hash->hash(key) % hash->size];
  for (; elmt; elmt = elmt->next) {
      if (!hash->cmp(key, elmt->key))
        return (elmt->data);
    }

  return (NULL);
}

int
objhash_walk(objhash_t *hash, int (func)(void *elmt, void *data), void *data)
{
  int   i;

  for (i = 0; i < hash->size; i++)
    {
      objhash_elmt_t *tmp;
      int status;

      for (tmp = hash->htable[i]; tmp; tmp = tmp->next)
        {
          if ((status = func(tmp->data, data)) != 0)
            return (status);
        }
    }
  return (0);
}


/*
** fast hash function samples
*/

unsigned long
objhash_pjw(void *key)
{
  unsigned long h;
  unsigned long g;

  h = 0;
  while (*(char *)key) {
    h = (h << 4) + *(char *)key++;
    if ((g = h & 0xF0000000U) != 0) {
      h = h ^ (g >> 24);
      h = h ^ g;
    }
  }

  return (h);
}

int
objhash_cmp(void *obj1, void *obj2)
{
  return ( strcmp( (char *)obj1, (char *)obj2 ) );
}


int
objhash_collide_count(objhash_t *hash)
{
  int   count;
  int   i;

  count = 0;
  for (i = 0; i < hash->size; i++)
    {
      objhash_elmt_t *tmp;

      for (tmp = hash->htable[i]; tmp; tmp = tmp->next)
        if (tmp->next)
          count++;
    }

  return (count);
}

#if 0
int
print_elmt(void *elmt, void *dummy)
{
  printf("%i\n", (int)elmt);

  return (0);
}

void
objhash_test(void)
{
  objhash_t *h;
  char buf[1024];
  int i;

  h = new_objhash(2053);
  h->hash = hash_pow;
  for (i = 0; i < 1000; i++)
    {
      sprintf(buf, "%i", i * 65599);
      objhash_add(h, (void *)(i * 65599), buf, strlen(buf));
    }
  objhash_walk(h, print_elmt, NULL);

  printf("collides : %i\n", objhash_collide_count(h));
}

void
fprintf_objhash_info(FILE *fp, objhash_t *h)
{
  fprintf(fp, "htable = %p\n", h->htable);
  fprintf(fp, "size = %i\n", h->size);
  fprintf(fp, "elmts = %i\n", h->elmts);
  fprintf(fp, "hfunc = %p\n", h->hash);
}

#endif /* DEBUG */



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
