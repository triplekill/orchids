/**
 ** @file inthash.h
 ** Header for mini hash library, with unsigned long indices.
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

#ifndef INTHASH_H
#define INTHASH_H

typedef struct inthash_elmt_s inthash_elmt_t;
struct inthash_elmt_s
{
  unsigned long key;
  void *data;
  inthash_elmt_t *next;
};

typedef struct inthash_s inthash_t;
struct inthash_s
{
  inthash_elmt_t **htable;
  size_t size;
  size_t elmts;
};

inthash_t *new_inthash(gc_t *gc_ctx, size_t hsize);
void clear_inthash(inthash_t *hash, void (*elmt_free)(void *e));
void free_inthash(inthash_t *hash, void (*elmt_free)(void *e));
void inthash_add(gc_t *gc_ctx, inthash_t *hash, void *data, unsigned long key);
void *inthash_get(inthash_t *hash, unsigned long key);

#endif /* HASH_H */



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
