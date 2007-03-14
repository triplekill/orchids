/**
 ** @file strhash.c
 ** string hash utility functions
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup util
 **
 ** @date  Started on: Mon Mar 31 12:28:19 2003
 ** @date Last update: Tue Nov 29 11:38:11 2005
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

#include "strhash.h"

strhash_t *
new_strhash(size_t hsize)
{
  strhash_t *h;

  h = Xmalloc(sizeof (strhash_t));
  h->size = hsize;
  h->elmts = 0;
  h->hash = DEFAULT_STRHASH_FUNCTION;
  h->htable = Xzmalloc(hsize * sizeof (strhash_elmt_t *));

  return (h);
}

void
clear_strhash(strhash_t *hash, void (*elmt_free)(void *e))
{
  int i;
  strhash_elmt_t *tmp;
  strhash_elmt_t *tmp_next;

  for (i = 0; i < hash->size; i++)
    {
      tmp = hash->htable[i];
      while (tmp)
        {
          tmp_next = tmp->next;
          if (elmt_free)
            elmt_free(tmp->data);
          Xfree(tmp);
          tmp = tmp_next;
        }
    }
  hash->elmts = 0;
  memset(hash->htable, 0, hash->size * sizeof (void *));
}

void *
strhash_to_array(strhash_t *hash)
{
  void **array;
/*  void **array_idx; */
/*  strhash_elmt_t **hhead; */
  strhash_elmt_t *helmt;
  int elmts;
  int i;
  int hsize;
  int j;

  hsize = hash->size;
  elmts = hash->elmts;
  array = Xmalloc(elmts * sizeof (void *));
  j = 0;
  for (i = 0; i < hsize; i++) {
    for (helmt = hash->htable[i]; helmt; helmt = helmt->next) {
      array[j] = helmt->data;
      j++;
    }
  }

  return (array);
}

void
free_strhash(strhash_t *hash, void (*elmt_free)(void *e))
{
  int i;
  strhash_elmt_t *tmp;
  strhash_elmt_t *tmp_next;

  for (i = 0; i < hash->size; i++)
    {
      tmp = hash->htable[i];
      while (tmp)
        {
          tmp_next = tmp->next;
          if (elmt_free)
            elmt_free(tmp->data);
          Xfree(tmp);
          tmp = tmp_next;
        }
    }

  Xfree(hash->htable);
  Xfree(hash);
}

void
strhash_resize(strhash_t *hash, size_t newsize)
{
  strhash_elmt_t **old_htable;
  int i;

  /* XXX: Not tested !!! */
  printf("Not tested !...\n");

  old_htable = hash->htable;
  hash->htable = Xzmalloc(newsize * sizeof (strhash_elmt_t *));

  for (i = 0; i < hash->size; ++i)
    {
      strhash_elmt_t *tmp;

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
strhash_add(strhash_t *hash, void *data, char *key)
{
  strhash_elmt_t *elmt;
  unsigned long hcode;

  elmt = Xmalloc(sizeof (strhash_elmt_t));
  elmt->key = key;
  elmt->data = data;

  hcode = hash->hash(key) % hash->size;
  elmt->next = hash->htable[hcode];
  hash->htable[hcode] = elmt;
  hash->elmts++;
}

void *
strhash_get(strhash_t *hash, char *key)
{
  strhash_elmt_t *elmt;

  elmt = hash->htable[hash->hash(key) % hash->size];
  for (; elmt; elmt = elmt->next)
    {
      if (!strcmp(key, elmt->key))
        return (elmt->data);
    }

  return (NULL);
}

int
strhash_walk(strhash_t *hash, int (func)(void *elmt, void *data), void *data)
{
  int i;

  for (i = 0; i < hash->size; i++)
    {
      strhash_elmt_t *tmp;
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
strhash_pjw(char *key)
{
  unsigned long h;
  unsigned long g;

  h = 0;
  while (*key)
    {
      h = (h << 4) + *key++;
      if ((g = h & 0xF0000000U) != 0)
        {
          h = h ^ (g >> 24);
          h = h ^ g;
        }
    }

  return (h);
}

unsigned long
strhash_pow(char *key)
{
  unsigned long hcode;

  hcode = 0;
  while (*key)
    {
      hcode += (*key) * (*key);
      key++;
    }

  return (hcode);
}

unsigned long
strhash_x65599(char *key)
{
  unsigned long hcode;

  hcode = 0;
  while (*key)
    hcode += *key++ * 65599;

  return (hcode);
}

unsigned long
strhash_quad(char *key)
{
  int quads;
  int i;
  unsigned long h;
  unsigned long *val;

  val = (unsigned long *) key;
  quads = strlen(key) / sizeof (unsigned long);
  h = 0;
  for (i = 0; i < quads; i++)
    h += *val++;

  return (h);
}

int
strhash_collide_count(strhash_t *hash)
{
  int count;
  int i;

  count = 0;
  for (i = 0; i < hash->size; i++)
    {
      strhash_elmt_t *tmp;

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
hash_test(void)
{
  strhash_t *h;
  char buf[1024];
  int i;

  h = new_hash(2053);
  h->hash = hash_pow;
  for (i = 0; i < 1000; i++)
    {
      sprintf(buf, "%i", i * 65599);
      hash_add(h, (void *)(i * 65599), buf, strlen(buf));
    }
  hash_walk(h, print_elmt, NULL);

  printf("collides : %i\n", hash_collide_count(h));
}

void
fprintf_hash_info(FILE *fp, strhash_t *h)
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
