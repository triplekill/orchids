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
 ** @date Last update: Fri Mar 23 17:33:07 2007
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

  for (i = 0; i < hash->size; ++i) {
    hash_elmt_t *tmp;

    for (tmp = old_htable[i]; tmp; tmp = tmp->next) {
      hcode_t hcode;

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
  hcode_t hcode;

  elmt = Xmalloc(sizeof (hash_elmt_t));
  elmt->key = (hkey_t *) key;
  elmt->keylen = keylen;
  elmt->data = data;

  hcode = hash->hash((hkey_t *) key, keylen) % hash->size;
  elmt->next = hash->htable[hcode];
  hash->htable[hcode] = elmt;
  hash->elmts++;
}

void *
hash_get(hash_t *hash, void *key, size_t keylen)
{
  hash_elmt_t *elmt;
  int len;

  elmt = hash->htable[hash->hash((hkey_t *)key, keylen) % hash->size];
  for (; elmt; elmt = elmt->next) {
    len = keylen < elmt->keylen ? keylen : elmt->keylen;
    if (!memcmp(key, elmt->key, len))
      return (elmt->data);
  }

  return (NULL);
}

int
hash_walk(hash_t *hash, hash_walk_func_t func, void *data)
{
  int   i;

  for (i = 0; i < hash->size; i++) {
    hash_elmt_t *tmp;
    int status;

    for (tmp = hash->htable[i]; tmp; tmp = tmp->next) {
      if ((status = func(tmp->data, data)) != 0)
        return (status);
    }
  }
  return (0);
}


/*
** fast hash function samples
*/


/* Peter J. Weinberger with the 24 corrected to 28 in the dragon book */
hcode_t
hash_pjw(hkey_t *key, size_t keylen)
{
  hcode_t h;
  hcode_t g;

  h = 0;
  for (; keylen > 0; keylen--) {
    h = (h << 4) + *key++;
    if ((g = h & 0xF0000000U) != 0) {
      h = h ^ (g >> 28);
      h = h ^ g;
    }
  }

  return (h);
}


/* Peter J. Weinberger with typo on 24 in the dragon book */
hcode_t
hash_pjw_typo(hkey_t *key, size_t keylen)
{
  hcode_t h;
  hcode_t g;

  h = 0;
  for (; keylen > 0; keylen--) {
    h = (h << 4) + *key++;
    if ((g = h & 0xF0000000U) != 0) {
      h = h ^ (g >> 24);
      h = h ^ g;
    }
  }

  return (h);
}


hcode_t
hash_pow(hkey_t *key, size_t keylen)
{
  hcode_t hcode;
  int k;

  hcode = 0;
  for (; keylen > 0; keylen--) {
    k = *key++;
    hcode += k * k;
  }

  return (hcode);
}


/* SDBM hash function */
hcode_t
hash_x65599(hkey_t *key, size_t keylen)
{
  hcode_t h;

  h = 0;
  for (; keylen > 0; keylen--)
    h = (*key++) + (h << 6) + (h << 16) - h; /* h += (*key++) * 65599; */

  return (h);
}


hcode_t
hash_quad(hkey_t *key, size_t keylen)
{
  int quads;
  int i;
  hcode_t h;
  hcode_t *val;

  val = (hcode_t *) key;
  quads = keylen / sizeof (hcode_t);
  h = 0;
  for (i = 0; i < quads; i++)
    h += *val++;

  return (h);
}


/* Robert Sedgewick, in book "Algorithms in C" */
hcode_t
hash_rs(hkey_t *key, size_t keylen)
{
  hcode_t hcode;
  hcode_t a;

  hcode = 0;
  a = 63689;  
  for ( ; keylen > 0; keylen--) {
    hcode = hcode * a + (*key++);
    a *= 378551;
  }

  return (hcode);
}


/* Ugly hack.  But usefull on huge keys. */
hcode_t
hash_ptr(hkey_t *key, size_t keylen)
{
  return ((hcode_t)key);
}


/* Jean Goubault-Larrecq reversed bit pointer trick */
hcode_t
hash_jglbr(hkey_t *key, size_t keylen)
{
  int b;
  hcode_t i;
  hcode_t j;

  i = (hcode_t) key;
  j = 0;
  for (b = 0;
       b < ((sizeof (hcode_t) * 8) - 1);
       b++, i >>= 1, j <<= 1)
    j |= i & 0x1;

  return (j);
}


/* GNU libc ELF hash function
 in glibc (2.3.6) sources, sysdeps/generic/dl-hash.h */
hcode_t
hash_elf(hkey_t *key, size_t keylen)
{
  hcode_t h;
  hcode_t g;

  h = 0;
  for ( ; keylen > 0; key++, keylen--) {
    h = (h << 4) + *key;
    if ( (g = h & 0xF0000000U) != 0) {
      h ^= (g >> 24);
      h &= ~g;
    }
  }
  return (h);
}


/* Hash function from gcc cpp 2.95 in gcc/cpphash.h */
hcode_t
hash_old_cpp(hkey_t *key, size_t keylen)
{
  hcode_t h;

  h = 0;
  for ( ; keylen > 0; keylen--)
    h = (h << 2) + *key++;

  return (h);
}


/* by Brian Kernighan and Dennis Ritchie's book
   "The C Programming Language" */
hcode_t
hash_bkdr(hkey_t *key, size_t keylen)
{
  hcode_t h;

  h = 0;
  for ( ; keylen > 0; keylen--)
    h = (h * 131313) + *key++; /* 31 131 1313 13131 131313 etc.. */

  return (h);
}


/* by Daniel Bernstein, in comp.lang.c */
hcode_t
hash_djb(hkey_t *key, size_t keylen)
{
  hcode_t h;

  h = 5381;
  for ( ; keylen > 0; keylen--)
    h = ((h << 5) + h) + (*key++);  /* h = h * 33 + (*key++); */

  return (h);
}


/* by Donald Knuth, in the Art of Computer Programming Vol. 3.  Sect 6.4 */
hcode_t
hash_dk(hkey_t *key, size_t keylen)
{
  hcode_t h;

  h = keylen; 
  for ( ; keylen > 0; keylen--)
    h = ((h << 5) ^ (h >> 27)) ^ (*key++);

  return (h);
}


/* from Arash Partow */
hcode_t
hash_ap(hkey_t *key, size_t keylen)
{
  hcode_t h;

  h = 0;
  for( ; keylen > 0; keylen--)
    h ^= ((keylen & 1) == 0) ? (  (h <<  7) ^ (*key++) ^ (h >> 3)) :
                               (~((h << 11) ^ (*key++) ^ (h >> 5)));

  return (h);
}


/* by Paul Hsieh, SuperFastHash function */
hcode_t
hash_sfh(hkey_t *key, size_t keylen)
{
  hcode_t h;
  hcode_t t;
  int rem;

  h = keylen;
  rem = keylen & 3;
  keylen >>= 2;

  for ( ; keylen > 0; keylen--) {
    h += *(unsigned short *)(key);
    t = ( (*(unsigned short *)(key+2)) << 11) ^ h;
    h = (h << 16) ^ t;
    key += 2 * sizeof (unsigned short);
    h += h >> 11;
  }

  switch (rem) {

  case 3:
    h += *(unsigned short *)(key);
    h ^= h << 16;
    h ^= key[sizeof (unsigned short)] << 18;
    h += h >> 11;
    break;

  case 2:
    h += *(unsigned short *)(key);
    h ^= h << 11;
    h += h >> 17;
    break;

  case 1:
    h += *key;
    h ^= h << 10;
    h += h >> 1;
  }

  h ^= h << 3;
  h += h >> 5;
  h ^= h << 4;
  h += h >> 17;
  h ^= h << 25;
  h += h >> 6;

  return (h);
}




int
hash_collide_count(hash_t *hash)
{
  int   count;
  int   i;

  count = 0;
  for (i = 0; i < hash->size; i++) {
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
  for (i = 0; i < 1000; i++) {
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
