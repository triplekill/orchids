/**
 ** @file hash.c
 ** Hash utility functions.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup util
 **
 ** @date  Started on: Mon Jan 20 16:41:14 2003
 ** @date Last update: Fri Mar 23 12:24:03 2007
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

#include "hash.h"

hash_t *
new_hash(size_t hsize)
{
  hash_t *h;

  h = Xmalloc(sizeof (hash_t));
  h->size = hsize;
  h->elmts = 0;
  h->hash = DEFAULT_HASH_FUNCTION;
  h->htable = Xzmalloc(hsize * sizeof (hash_elmt_t *));

  return (h);
}

void
hash_resize(hash_t *hash, size_t newsize)
{
  hash_elmt_t **old_htable;
  int i;

  /* XXX: Not tested !!! */
  printf("Not tested !...\n");

  old_htable = hash->htable;
  hash->htable = Xzmalloc(newsize * sizeof (hash_elmt_t *));

  for (i = 0; i < hash->size; ++i)
    {
      hash_elmt_t *tmp;

      for (tmp = old_htable[i]; tmp; tmp = tmp->next)
        {
          unsigned long hcode;

          hcode = hash->hash(tmp->key, tmp->keylen) % newsize;
          tmp->next = hash->htable[hcode];
          hash->htable[hcode] = tmp;
        }
    }

  Xfree(old_htable);
  hash->size = newsize;
}

void
hash_add(hash_t *hash, void *data, void *key, size_t keylen)
{
  hash_elmt_t *elmt;
  unsigned long hcode;

  elmt = Xmalloc(sizeof (hash_elmt_t));
  elmt->key = key;
  elmt->keylen = keylen;
  elmt->data = data;

  hcode = hash->hash(key, keylen) % hash->size;
  elmt->next = hash->htable[hcode];
  hash->htable[hcode] = elmt;
  hash->elmts++;
}

void *
hash_get(hash_t *hash, void *key, size_t keylen)
{
  hash_elmt_t *elmt;
  int len;

  elmt = hash->htable[hash->hash(key, keylen) % hash->size];
  for (; elmt; elmt = elmt->next)
    {
      len = keylen < elmt->keylen ? keylen : elmt->keylen;
      if (!memcmp(key, elmt->key, len))
        return (elmt->data);
    }

  return (NULL);
}

int
hash_walk(hash_t *hash, int (func)(void *elmt, void *data), void *data)
{
  int   i;

  for (i = 0; i < hash->size; i++)
    {
      hash_elmt_t *tmp;
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

/* Peter J. Weinberger */
unsigned long
hash_pjw(void *key, size_t keylen)
{
  unsigned long h;
  unsigned long g;
  char *p;

  h = 0;
  p = (char *) key;
  for (; keylen > 0; keylen--)
    {
      h = (h << 4) + *p++;
      if ((g = h & 0xF0000000U) != 0)
        {
          h = h ^ (g >> 28);
          h = h ^ g;
        }
    }

  return (h);
}

unsigned long
hash_pjw_typo(void *key, size_t keylen)
{
  unsigned long h;
  unsigned long g;
  char *p;

  h = 0;
  p = (char *) key;
  for (; keylen > 0; keylen--)
    {
      h = (h << 4) + *p++;
      if ((g = h & 0xF0000000U) != 0)
        {
          h = h ^ (g >> 24);
          h = h ^ g;
        }
    }

  return (h);
}

unsigned long
hash_pow(void *key, size_t keylen)
{
  unsigned long hcode;
  char *k;

  hcode = 0;
  k = (char *) key;
  for (; keylen > 0; keylen--)
    {
      hcode += *k * *k;
      k++;
    }

  return (hcode);
}

/* SDBM hash function */
unsigned long
hash_x65599(void *key, size_t keylen)
{
  unsigned long hcode;
  char *k;

  hcode = 0;
  k = (char *) key;
  for (; keylen > 0; keylen--, k++)
    hcode += *k * 65599;

  return (hcode);
}

/* SDBM hash function, faster */
unsigned long
hash_x65599_fast(void *key, size_t keylen)
{
  unsigned long hcode;
  char *k;

  hcode = 0;
  k = (char *) key;
  for (; keylen > 0; keylen--, k++)
    hcode = *k + (hcode << 6) + (hcode << 16) - hcode;

  return (hcode);
}

unsigned long
hash_quad(void *key, size_t keylen)
{
  int quads;
  int i;
  unsigned long h;
  unsigned long *val;

  val = (unsigned long *) key;
  quads = keylen / sizeof (unsigned long);
  h = 0;
  for (i = 0; i < quads; i++)
    h += *val++;

  return (h);
}

/* Robert Sedgwicks, in book "Algorithms in C" */
unsigned long
hash_rs(void *key, size_t keylen)
{
  unsigned long hcode = 0;
  unsigned long a;
  unsigned char *k;

  a = 63689;  
  for(k = (unsigned char *)key; keylen > 0; k++, keylen--)
    {
      hcode = hcode * a + (*k);
      a *= 378551;
    }

  return (hcode);
}

int
hash_collide_count(hash_t *hash)
{
  int   count;
  int   i;

  count = 0;
  for (i = 0; i < hash->size; i++)
    {
      hash_elmt_t *tmp;

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
  hash_t *h;
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
fprintf_hash_info(FILE *fp, hash_t *h)
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
