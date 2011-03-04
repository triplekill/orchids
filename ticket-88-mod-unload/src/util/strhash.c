/**
 ** @file strhash.c
 ** string hash utility functions
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 1.2
 ** @ingroup util
 **
 ** @date  Started on: Mon Mar 31 12:28:19 2003
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

  for (i = 0; i < hash->size; i++) {
    tmp = hash->htable[i];
    while (tmp) {
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

  for (i = 0; i < hash->size; i++) {
    tmp = hash->htable[i];
    while (tmp) {
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

  old_htable = hash->htable;
  hash->htable = Xzmalloc(newsize * sizeof (strhash_elmt_t *));

  for (i = 0; i < hash->size; ++i) {
    strhash_elmt_t *tmp;

    for (tmp = old_htable[i]; tmp; tmp = tmp->next) {
      strhcode_t hcode;

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
  strhcode_t hcode;

  elmt = Xmalloc(sizeof (strhash_elmt_t));
  elmt->key = key;
  elmt->data = data;

  hcode = hash->hash(key) % hash->size;
  elmt->next = hash->htable[hcode];
  hash->htable[hcode] = elmt;
  hash->elmts++;
}


void *
strhash_check_and_add(strhash_t *hash, void *data, char *key)
{
  strhash_elmt_t *elmt;
  strhcode_t hcode;

  elmt = hash->htable[hash->hash(key) % hash->size];
  for (; elmt; elmt = elmt->next) {
    if (!strcmp(key, elmt->key))
      return (elmt->data);
  }

  elmt = Xmalloc(sizeof (strhash_elmt_t));
  elmt->key = key;
  elmt->data = data;

  hcode = hash->hash(key) % hash->size;
  elmt->next = hash->htable[hcode];
  hash->htable[hcode] = elmt;
  hash->elmts++;

  return (NULL);
}


void *
strhash_update(strhash_t *hash, void *new_data, char *key)
{
  strhash_elmt_t *elmt;
  void *old_data;

  elmt = hash->htable[hash->hash(key) % hash->size];
  for (; elmt; elmt = elmt->next) {
    if (!strcmp(key, elmt->key)) {
      old_data = elmt->data;
      elmt->data = new_data;

      return (old_data);
    }
  }

  return (NULL);
}


void *
strhash_update_or_add(strhash_t *hash, void *new_data, char *key)
{
  strhash_elmt_t *elmt;
  void *old_data;
  strhcode_t hcode;

  elmt = hash->htable[hash->hash(key) % hash->size];
  for (; elmt; elmt = elmt->next) {
    if (!strcmp(key, elmt->key)) {
      old_data = elmt->data;
      elmt->data = new_data;

      return (old_data);
    }
  }

  elmt = Xmalloc(sizeof (strhash_elmt_t));
  elmt->key = key;
  elmt->data = new_data;

  hcode = hash->hash(key) % hash->size;
  elmt->next = hash->htable[hcode];
  hash->htable[hcode] = elmt;
  hash->elmts++;

  return (NULL);
}


void *
strhash_del(strhash_t *hash, char *key)
{
  strhash_elmt_t **head;
  strhash_elmt_t  *elmt;
  strhash_elmt_t  *prev;
  void            *data;

  prev = NULL;
  head = &hash->htable[hash->hash(key) % hash->size];
  for (elmt = *head; elmt; elmt = elmt->next) {
    if (!strcmp(key, elmt->key)) {
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
strhash_get(strhash_t *hash, char *key)
{
  strhash_elmt_t *elmt;

  elmt = hash->htable[hash->hash(key) % hash->size];
  for ( ; elmt; elmt = elmt->next) {
    if (!strcmp(key, elmt->key))
      return (elmt->data);
  }

  return (NULL);
}


strhash_t *
strhash_clone(strhash_t *hash, void *(clone)(void *elmt))
{
  strhash_t *h;
  strhash_elmt_t *tmp;
  int i;
  int hsize;
  void *data;

  if (clone == NULL)
    return (NULL);

  hsize = hash->size;
  h = new_strhash(hsize);
  for (i = 0; i < hsize; i++) {
    for (tmp = hash->htable[i]; tmp; tmp = tmp->next) {
      data = clone(tmp->data);
      strhash_add(h, data, tmp->key);
    }
  }

  return (h);
}


int
strhash_walk(strhash_t *hash, int (func)(void *elmt, void *data), void *data)
{
  int i;

  for (i = 0; i < hash->size; i++) {
    strhash_elmt_t *tmp;
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
strhcode_t
strhash_pjw(char *key)
{
  strhcode_t h;
  strhcode_t g;

  h = 0;
  while (*key) {
    h = (h << 4) + *key++;
    if ((g = h & 0xF0000000U) != 0) {
      h = h ^ (g >> 28);
      h = h ^ g;
    }
  }

  return (h);
}


/* Peter J. Weinberger with typo on 24 in the dragon book */
strhcode_t
strhash_pjw_typo(char *key)
{
  strhcode_t h;
  strhcode_t g;

  h = 0;
  while (*key) {
    h = (h << 4) + *key++;
    if ((g = h & 0xF0000000U) != 0) {
      h = h ^ (g >> 24);
      h = h ^ g;
    }
  }

  return (h);
}

strhcode_t
strhash_pow(char *key)
{
  strhcode_t h;

  h = 0;
  while (*key) {
    h += (*key) * (*key);
    key++;
  }

  return (h);
}

strhcode_t
strhash_x65599(char *key)
{
  strhcode_t h;

  h = 0;
  while (*key)
    h += *key++ * 65599;

  return (h);
}

strhcode_t
strhash_x65599_opt(char *key)
{
  strhcode_t h;

  h = 0;
  while (*key)
    h = *key++ + (h << 6) + (h << 16) - h;

  return (h);
}

strhcode_t
strhash_quad(char *key)
{
  int quads;
  int i;
  strhcode_t h;
  unsigned long *val;

  val = (unsigned long *) key;
  quads = strlen(key) / sizeof (unsigned long);
  h = 0;
  for (i = 0; i < quads; i++)
    h += *val++;

  return (h);
}

/* Robert Sedgewick, in book "Algorithms in C" */
strhcode_t
strhash_rs(char *key)
{
  strhcode_t h;
  strhcode_t a;

  h = 0;
  a = 63689;  
  while (*key) {
    h = h * a + (*key++);
    a *= 378551;
  }

  return (h);
}

strhcode_t
strhash_ptr(char *key)
{
  return ((strhcode_t)key);
}

/* Jean Goubault-Larrecq reversed bit pointer trick */
strhcode_t
strhash_jglbr(char *key)
{
  int b;
  strhcode_t i;
  strhcode_t j;

  i = (strhcode_t) key;
  j = 0;
  for (b = 0;
       b < ((sizeof (strhcode_t) * 8) - 1);
       b++, i >>= 1, j <<= 1)
    j |= i & 0x1;

  return (j);
}

/* GNU libc ELF hash function
 in glibc (2.3.6) sources, sysdeps/generic/dl-hash.h */
strhcode_t
strhash_elf(char *key)
{
  strhcode_t h;
  strhcode_t g;

  h = 0;
  while (*key) {
    h = (h << 4) + *key++;
    if ( (g = h & 0xF0000000U) != 0) {
      h ^= (g >> 24);
      h &= ~g;
    }
  }

  return (h);
}

/* Hash function from gcc cpp 2.95 in gcc/cpphash.h */
strhcode_t
strhash_gcc295_cpp(char *key)
{
  strhcode_t h;

  h = 0;
  while (*key)
    h = (h << 2) + *key++;

  return (h);
}

/* by Brian Kernighan and Dennis Ritchie's book
   "The C Programming Language" */
strhcode_t
strhash_bkdr(char *key)
{
  strhcode_t h;

  h = 0;
  while (*key)
    h = (h * 131313) + *key++; /* 31 131 1313 13131 131313 etc.. */

  return (h);
}

/* by Daniel Bernstein, in comp.lang.c */
strhcode_t
strhash_djb(char *key)
{
  strhcode_t h;

  h = 5381;
  while (*key)
    h = ((h << 5) + h) + (*key++);  /* h = h * 33 + (*key++); */

  return (h);
}

/* by Donald Knuth, in the Art of Computer Programming Vol. 3.  Sect 6.4 */
strhcode_t
strhash_dk(char *key)
{
  strhcode_t h;

  h = strlen(key);
  while (*key)
    h = ((h << 5) ^ (h >> 27)) ^ (*key++);

  return (h);
}


/* from Arash Partow */
strhcode_t
strhash_ap(char *key)
{
  strhcode_t h;
  int i;

  h = 0;
  i = 0;
  while (*key)
    h ^= ((i++ & 1) == 0) ? (  (h <<  7) ^ (*key++) ^ (h >> 3)) :
                            (~((h << 11) ^ (*key++) ^ (h >> 5)));

  return (h);
}

/* by Paul Hsieh, SuperFastHash function */
strhcode_t
strhash_sfh(char *key)
{
  strhcode_t h;
  strhcode_t t;
  int rem;
  size_t keylen;

  keylen = strlen(key);
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

/* by M.V. Ramakrishna and Justin Zobel,
   from paper "Performance in Practice of String Hashing Functions" */
strhcode_t
strhash_rz(char *key)
{
  strhcode_t h;

  h = 0;
  while(*key)
    h ^= ((h << 5) + (*key++) + (h >> 2));

  return (h);
}

/* by Fowler / Noll / Vo initial historic version */
strhcode_t
strhash_fnv0(char *key)
{
  strhcode_t h;

  h = 0;
  while (*key) {
    h *= 0x01000193;
    h ^= *key++;
  }

  return (h);
}

/* by Fowler / Noll / Vo faster on some processors */
strhcode_t
strhash_fnv0_opt(char *key)
{
  strhcode_t h;

  h = 0;
  while (*key) {
    h += (h << 1) + (h << 4) + (h << 7) + (h << 8) + (h << 24);
    h ^= *key++;
  }

  return (h);
}

/* by Fowler / Noll / Vo updated version with a new initial value */
strhcode_t
strhash_fnv1(char *key)
{
  strhcode_t h;

  h = 0x811c9dc5;
  while (*key) {
    h *= 0x01000193;
    h ^= *key++;
  }

  return (h);
}

/* Fowler / Noll / Vo version 1 faster on some processors */
strhcode_t
strhash_fnv1_opt(char *key)
{
  strhcode_t h;

  h = 0x811c9dc5;
  while (*key) {
    h += (h << 1) + (h << 4) + (h << 7) + (h << 8) + (h << 24);
    h ^= *key++;
  }

  return (h);
}

/* by Fowler / Noll / Vo version 1a */
strhcode_t
strhash_fnv1a(char *key)
{
  strhcode_t h;

  h = 0x811c9dc5;
  while (*key) {
    h ^= *key++;
    h *= 0x01000193;
  }

  return (h);
}

/* Fowler / Noll / Vo version 1a faster on some processors */
strhcode_t
strhash_fnv1a_opt(char *key)
{
  strhcode_t h;

  h = 0x811c9dc5;
  while (*key) {
    h ^= *key++;
    h += (h << 1) + (h << 4) + (h << 7) + (h << 8) + (h << 24);
  }

  return (h);
}

/* LCG hash, by Donald Knuth, LCG 2^32 */
strhcode_t
strhash_lcg32dk(char *key)
{
  strhcode_t h;
 
  h = 0;
  while (*key)
    h = (h * 1664525) + (*key++) + 1013904223;

  return (h);
}

/* rotating hash. GCC 4 should produce bit rotation instructions ;) */
strhcode_t
strhash_rot13(char *key)
{
  strhcode_t h;

  h = 0;
  while (*key) {
    h += (*key++);
    h -= (h << 13) | (h >> 19);
  }

  return (h);
}

/* One at time hash, by Bob Jenkins */
strhcode_t
strhash_oat(char *key)
{
  strhcode_t h;

  h = 0;
  while (*key) {
    h += *key++;
    h += (h << 10);
    h ^= (h >> 6);
  }
  h += (h << 3);
  h ^= (h >> 11);
  h += (h << 15);
  
  return (h);
}

/* ETH Zuerich hash function */
strhcode_t
strhash_ethz(char *key)
{
  strhcode_t h;

  h = 0;
  while (*key)
    h = (*key++) * (h % 257) + 1;

  return (h);
}


/* default hash function from Kazlib */
strhcode_t
strhash_kazlib(char *key)
{
  static unsigned long randbox[] = {
    0x49848f1bU, 0xe6255dbaU, 0x36da5bdcU, 0x47bf94e9U,
    0x8cbcce22U, 0x559fc06aU, 0xd268f536U, 0xe10af79aU,
    0xc1af4d69U, 0x1d2917b5U, 0xec4c304dU, 0x9ee5016cU,
    0x69232f74U, 0xfead7bb3U, 0xe9089ab6U, 0xf012f6aeU,
  };
  const unsigned char *str;
  strhcode_t h;

  h = 0;
  str = (unsigned char *) key;
  while ( *str ) {
    h ^= randbox[(*str + h) & 0xf];
    h = (h << 1) | (h >> 31);
    h &= 0xffffffffU;
    h ^= randbox[((*str++ >> 4) + h) & 0xf];
    h = (h << 2) | (h >> 30);
    h &= 0xffffffffU;
  }

  return (h);
}


int
strhash_collide_count(strhash_t *hash)
{
  int count;
  int i;

  count = 0;
  for (i = 0; i < hash->size; i++) {
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
  for (i = 0; i < 1000; i++) {
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
