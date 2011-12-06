/**
 ** @file hash.c
 ** Hash utility functions.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 1.2
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
clear_hash(hash_t *hash, void (*elmt_free)(void *e))
{
  int i;
  hash_elmt_t *tmp;
  hash_elmt_t *tmp_next;

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
hash_to_array(hash_t *hash)
{
  void **array;
  hash_elmt_t *helmt;
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
free_hash(hash_t *hash, void (*elmt_free)(void *e))
{
  int i;
  hash_elmt_t *tmp;
  hash_elmt_t *tmp_next;

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
hash_resize(hash_t *hash, size_t newsize)
{
  hash_elmt_t **old_htable;
  int i;

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
hash_check_and_add(hash_t *hash, void *data, void *key, size_t keylen)
{
  hash_elmt_t *elmt;
  hcode_t hcode;

  elmt = hash->htable[hash->hash((hkey_t *)key, keylen) % hash->size];
  for (; elmt; elmt = elmt->next) {
    if (keylen == elmt->keylen && !memcmp(key, elmt->key, keylen))
      return (elmt->data);
  }

  elmt = Xmalloc(sizeof (hash_elmt_t));
  elmt->key = (hkey_t *) key;
  elmt->keylen = keylen;
  elmt->data = data;

  hcode = hash->hash((hkey_t *) key, keylen) % hash->size;
  elmt->next = hash->htable[hcode];
  hash->htable[hcode] = elmt;
  hash->elmts++;

  return (NULL);
}


void *
hash_update(hash_t *hash, void *new_data, void *key, size_t keylen)
{
  hash_elmt_t *elmt;
  void *old_data;

  elmt = hash->htable[hash->hash(key, keylen) % hash->size];
  for (; elmt; elmt = elmt->next) {
    if (keylen == elmt->keylen && !memcmp(key, elmt->key, keylen)) {
      old_data = elmt->data;
      elmt->data = new_data;

      return (old_data);
    }
  }

  return (NULL);
}


void *
hash_update_or_add(hash_t *hash, void *new_data, void *key, size_t keylen)
{
  hash_elmt_t *elmt;
  void *old_data;
  hcode_t hcode;

  elmt = hash->htable[hash->hash(key, keylen) % hash->size];
  for (; elmt; elmt = elmt->next) {
    if (keylen == elmt->keylen && !memcmp(key, elmt->key, keylen)) {
      old_data = elmt->data;
      elmt->data = new_data;

      return (old_data);
    }
  }

  elmt = Xmalloc(sizeof (hash_elmt_t));
  elmt->key = key;
  elmt->keylen = keylen;
  elmt->data = new_data;

  hcode = hash->hash(key, keylen) % hash->size;
  elmt->next = hash->htable[hcode];
  hash->htable[hcode] = elmt;
  hash->elmts++;

  return (NULL);
}


void *
hash_del(hash_t *hash, void *key, size_t keylen)
{
  hash_elmt_t **head;
  hash_elmt_t  *elmt;
  hash_elmt_t  *prev;
  void         *data;

  prev = NULL;
  head = &hash->htable[hash->hash(key, keylen) % hash->size];
  for (elmt = *head; elmt; elmt = elmt->next) {
    if (keylen == elmt->keylen && !memcmp(key, elmt->key, keylen)) {
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
hash_get(hash_t *hash, void *key, size_t keylen)
{
  hash_elmt_t *elmt;

  elmt = hash->htable[hash->hash((hkey_t *)key, keylen) % hash->size];
  for (; elmt; elmt = elmt->next) {
    if (keylen == elmt->keylen && !memcmp(key, elmt->key, keylen))
      return (elmt->data);
  }

  return (NULL);
}


hash_t *
hash_clone(hash_t *hash, void *(clone)(void *elmt))
{
  hash_t *h;
  hash_elmt_t *tmp;
  int i;
  int hsize;
  void *data;
 
  if (clone == NULL)
    return (NULL);

  hsize = hash->size;
  h = new_hash(hsize);
  for (i = 0; i < hsize; i++) {
    for (tmp = hash->htable[i]; tmp; tmp = tmp->next) {
      data = clone(tmp->data);
      hash_add(h, data, tmp->key, tmp->keylen);
    }
  }

  return (h);
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
    h += (*key++) * 65599;

  return (h);
}


/* SDBM hash function */
hcode_t
hash_x65599_opt(hkey_t *key, size_t keylen)
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
  for ( ; keylen > 0; keylen--) {
    h = (h << 4) + *key++;
    if ( (g = h & 0xF0000000U) != 0) {
      h ^= (g >> 24);
      h &= ~g;
    }
  }
  return (h);
}


/* Hash function from gcc cpp 2.95 in gcc/cpphash.h */
hcode_t
hash_gcc295_cpp(hkey_t *key, size_t keylen)
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


/* by M.V. Ramakrishna and Justin Zobel,
   from paper "Performance in Practice of String Hashing Functions" */
hcode_t
hash_rz(hkey_t *key, size_t keylen)
{
  hcode_t h;

  h = 0;
  for( ; keylen > 0; keylen--)
    h ^= ((h << 5) + (*key++) + (h >> 2));

  return (h);
}


/* by Fowler / Noll / Vo initial historic version */
hcode_t
hash_fnv0(hkey_t *key, size_t keylen)
{
  hcode_t h;

  h = 0;
  for ( ; keylen > 0; keylen--) {
    h *= 0x01000193;
    h ^= *key++;
  }

  return (h);
}


/* by Fowler / Noll / Vo faster on some processors */
hcode_t
hash_fnv0_opt(hkey_t *key, size_t keylen)
{
  hcode_t h;

  h = 0;
  for ( ; keylen > 0; keylen--) {
    h += (h << 1) + (h << 4) + (h << 7) + (h << 8) + (h << 24);
    h ^= *key++;
  }

  return (h);
}


/* by Fowler / Noll / Vo updated version with a new initial value */
hcode_t
hash_fnv1(hkey_t *key, size_t keylen)
{
  hcode_t h;

  h = 0x811c9dc5;
  for ( ; keylen > 0; keylen--) {
    h *= 0x01000193;
    h ^= *key++;
  }

  return (h);
}


/* Fowler / Noll / Vo version 1 faster on some processors */
hcode_t
hash_fnv1_opt(hkey_t *key, size_t keylen)
{
  hcode_t h;

  h = 0x811c9dc5;
  for ( ; keylen > 0; keylen--) {
    h += (h << 1) + (h << 4) + (h << 7) + (h << 8) + (h << 24);
    h ^= *key++;
  }

  return (h);
}


/* by Fowler / Noll / Vo version 1a */
hcode_t
hash_fnv1a(hkey_t *key, size_t keylen)
{
  hcode_t h;

  h = 0x811c9dc5;
  for ( ; keylen > 0; keylen--) {
    h ^= *key++;
    h *= 0x01000193;
  }

  return (h);
}


/* Fowler / Noll / Vo version 1a faster on some processors */
hcode_t
hash_fnv1a_opt(hkey_t *key, size_t keylen)
{
  hcode_t h;

  h = 0x811c9dc5;
  for ( ; keylen > 0; keylen--) {
    h ^= *key++;
    h += (h << 1) + (h << 4) + (h << 7) + (h << 8) + (h << 24);
  }

  return (h);
}


/* LCG hash, by Donald Knuth, LCG 2^32 */
hcode_t
hash_lcg32dk(hkey_t *key, size_t keylen)
{
  hcode_t h;
 
  h = 0;
  for ( ; keylen > 0; keylen--)
    h = (h * 1664525) + (*key++) + 1013904223;

  return (h);
}


/* rotating hash. GCC 4 should produce bit rotation instructions ;) */
hcode_t
hash_rot13(hkey_t *key, size_t keylen)
{
  hcode_t h;

  h = 0;
  for ( ; keylen > 0; keylen--) {
    h += (*key++);
    h -= (h << 13) | (h >> 19);
  }

  return (h);
}


hcode_t
hash_hsh1113_8bits(hkey_t *key, size_t keylen)
{
  hcode_t state;
  hcode_t h;
  int rotval;
  int precision;

  state = 0x40490FDB; /* or  0xC0C0C0C0, 0x03030303, 1 */
  h = 0;
  for ( ; keylen > 0; keylen--) {
    h ^= *key++;
    for (precision = 0; precision < 7; precision++) {
      state = (state << 11) | (state >> 21);
      h = (h << 13) | (h >> 19);
      h ^= state;
      rotval = state & 0x1F; /* mod 32 */
      h = (h << rotval) | (h >> (32 - rotval));
      rotval = h & 0x1F; /* mod 32 */
      state = (state << rotval) | (state >> (32 - rotval));
    }
  }

  return (h);
}


hcode_t
hash_hsh1113_32bits(hkey_t *key, size_t keylen)
{
  hcode_t state;
  hcode_t h;
  int rotval;
  int precision;
  int rem;

  rem = keylen & 0x3; /* mod 4 */
  state = 0x40490FDB; /* or  0xC0C0C0C0, 0x03030303, 1 */
  h = 0;
  for ( ; keylen > sizeof (unsigned long); keylen -= sizeof (unsigned long)) {
    h ^= *(unsigned long *)key;
    key += sizeof (unsigned long);
    for (precision = 0; precision < 31; precision++) {
      state = (state << 11) | (state >> 21);
      h = (h << 13) | (h >> 19);
      h ^= state;
      rotval = state & 0x1F; /* mod 32 */
      h = (h << rotval) | (h >> (32 - rotval));
      rotval = h & 0x1F; /* mod 32 */
      state = (state << rotval) | (state >> (32 - rotval));
    }
  }

  /* remaining data */
  switch (rem) {
  case 3:
    h ^= ((key[0]) | (key[1] << 8) | (key[2] << 16));
    break ;
  case 2:
    h ^= ((key[0]) | (key[1] << 8));
    break ;
  case 1:
    h ^= key[0];
    break ;
  case 0:
    return (h);
  }

  for (precision = 0; precision < 31; precision++) {
    state = (state << 11) | (state >> 21);
    h = (h << 13) | (h >> 19);
    h ^= state;
    rotval = state & 0x1F; /* mod 32 */
    h = (h << rotval) | (h >> (32 - rotval));
    rotval = h & 0x1F; /* mod 32 */
    state = (state << rotval) | (state >> (32 - rotval));
  }

  return (h);
}


/* One at time hash, by Bob Jenkins */
hcode_t
hash_oat(hkey_t *key, size_t keylen)
{
  hcode_t h;

  h = 0;
  for ( ; keylen > 0; keylen--) {
    h += *key++;
    h += (h << 10);
    h ^= (h >> 6);
  }
  h += (h << 3);
  h ^= (h >> 11);
  h += (h << 15);
  
  return (h);
}


/* ETH Zurich hash function */
hcode_t
hash_ethz(hkey_t *key, size_t keylen)
{
  hcode_t h;

  h = 0;
  for ( ; keylen > 0 ; keylen++)
    h = (*key++) * (h % 257) + 1;

  return (h);
}


/* default hash function from Kazlib */
hcode_t
hash_kazlib(hkey_t *key, size_t keylen)
{
  static unsigned long randbox[] = {
    0x49848f1bU, 0xe6255dbaU, 0x36da5bdcU, 0x47bf94e9U,
    0x8cbcce22U, 0x559fc06aU, 0xd268f536U, 0xe10af79aU,
    0xc1af4d69U, 0x1d2917b5U, 0xec4c304dU, 0x9ee5016cU,
    0x69232f74U, 0xfead7bb3U, 0xe9089ab6U, 0xf012f6aeU,
  };
  hcode_t h;

  h = 0;
  for ( ; keylen > 0 ; keylen-- ) {
    h ^= randbox[(*key + h) & 0xf];
    h = (h << 1) | (h >> 31);
    h &= 0xffffffffU;
    h ^= randbox[((*key++ >> 4) + h) & 0xf];
    h = (h << 2) | (h >> 30);
    h &= 0xffffffffU;
  }

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
