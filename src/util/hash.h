/**
 ** @file hash.h
 ** Header for mini hash library.
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

#ifndef HASH_H
#define HASH_H

#define DEFAULT_HASH_FUNCTION hash_djb

/* key are arbitraty binary data (unsigned char) */
typedef unsigned char hkey_t;

typedef unsigned long hcode_t;

typedef struct hash_elmt_s hash_elmt_t;
struct hash_elmt_s
{
  hkey_t *key;
  size_t keylen;
  void *data;
  hash_elmt_t *next;
};

typedef hcode_t (*hashfunc_t)(hkey_t *key, size_t keylen);

typedef struct hash_s hash_t;
struct hash_s
{
  hash_elmt_t **htable;
  size_t size;
  int elmts;
  hashfunc_t hash;
};

typedef int (*hash_walk_func_t)(void *elmt, void *data);

hash_t *new_hash(size_t hsize);
void hash_resize(hash_t *hash, size_t newsize);
void clear_hash(hash_t *hash, void (*elmt_free)(void *e));
void free_hash(hash_t *hash, void (*elmt_free)(void *e));
void hash_add(hash_t *hash, void *data, void *key, size_t keylen);
void *hash_check_and_add(hash_t *hash, void *data, void *key, size_t keylen);
void *hash_del(hash_t *hash, void *key, size_t keylen);
void *hash_get(hash_t *hash, void *key, size_t keylen);
void *hash_update(hash_t *hash, void *new_data, void *key, size_t keylen);
void *hash_update_or_add(hash_t *hash, void *new_data, void *key, size_t keylen);
hash_t *hash_clone(hash_t *hash, void *(clone)(void *elmt));
int hash_walk(hash_t *hash, hash_walk_func_t func, void *data);
int hash_collide_count(hash_t *hash);

hcode_t hash_pjw(hkey_t *key, size_t keylen);
hcode_t hash_pjw_typo(hkey_t *key, size_t keylen);
hcode_t hash_pow(hkey_t *key, size_t keylen);
hcode_t hash_x65599(hkey_t *key, size_t keylen);
hcode_t hash_x65599_opt(hkey_t *key, size_t keylen);
hcode_t hash_quad(hkey_t *key, size_t keylen);
hcode_t hash_rs(hkey_t *key, size_t keylen);
hcode_t hash_ptr(hkey_t *key, size_t keylen);
hcode_t hash_jglbr(hkey_t *key, size_t keylen);
hcode_t hash_elf(hkey_t *key, size_t keylen);
hcode_t hash_gcc295_cpp(hkey_t *key, size_t keylen);
hcode_t hash_bkdr(hkey_t *key, size_t keylen);
hcode_t hash_djb(hkey_t *key, size_t keylen);
hcode_t hash_ap(hkey_t *key, size_t keylen);
hcode_t hash_dk(hkey_t *key, size_t keylen);
hcode_t hash_sfh(hkey_t *key, size_t keylen);
hcode_t hash_jr(hkey_t *key, size_t keylen);
hcode_t hash_fnv0(hkey_t *key, size_t keylen);
hcode_t hash_fnv0_opt(hkey_t *key, size_t keylen);
hcode_t hash_fnv1(hkey_t *key, size_t keylen);
hcode_t hash_fnv1_opt(hkey_t *key, size_t keylen);
hcode_t hash_fnv1a(hkey_t *key, size_t keylen);
hcode_t hash_fnv1a_opt(hkey_t *key, size_t keylen);
hcode_t hash_lcg32dk(hkey_t *key, size_t keylen);
hcode_t hash_rot13(hkey_t *key, size_t keylen);
hcode_t hash_hsh1113_8bits(hkey_t *key, size_t keylen);
hcode_t hash_hsh1113_32bits(hkey_t *key, size_t keylen);
hcode_t hash_oat(hkey_t *key, size_t keylen);
hcode_t hash_ethz(hkey_t *key, size_t keylen);
hcode_t hash_kazlib(hkey_t *key, size_t keylen);





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
