/**
 ** @file db.c
 ** In-memory databases
 **
 ** @author Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
 **
 ** @version 1.0
 ** @ingroup engine
 **
 ** @date  Started on: Mon Feb  3 18:11:19 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef DB
#define DB

#define OBJTYPE ovm_var_t *

typedef struct db_small_table db_small_table;
struct db_small_table { /* lexicographically ordered list of tuples */
  gc_header_t gc;
  db_small_table *next;
  int nfields;
  OBJTYPE tuple[1]; /* tuple[nfields] */
};
#define DBST_SIZE(nfields) offsetof(db_small_table,tuple[nfields])

/*typedef struct db_map db_map;*/

struct db_branch {
  db_map *left;
  db_map *right;
  int dummy; /* never actually there---only used in DB_BRANCH_SIZE below */
};
#define DB_BRANCH_SIZE offsetof(db_map,what.branch.dummy)

struct db_tuples {
  db_small_table *table;
  unsigned long hash [1]; /* a list of nfields hash codes;
			     all tuples in table have the same */
};
#define DB_TUPLE_SIZE(nfields) offsetof(db_map,what.tuples.hash[nfields])

struct db_map {
  gc_header_t gc;
  /*unsigned int flags;*/
  union {
    struct db_tuples tuples;
    struct db_branch branch;
  } what;
};

/* The following is meant to denote a variable (field number)
   or a constant.
   The type db_var_or_cst is meant to be compatible with bytecode_t(==unsigned long).
   Variable i is encoded as DB_VAR_MASK|i.
   Constants j (with j & DB_VAR_MASK==0) are to be found in a constant table at offset j.
   Additionally, there is a fake variable DB_VAR_NONE.
*/
typedef unsigned long db_var_or_cst;
#define DB_VAR_MASK ((unsigned long)(1L << (8*sizeof(unsigned long)-1)))
#define DB_VAR_NONE DB_VAR(DB_VAR_MASK-1L)
#define DB_IS_CST(i) (((i) & DB_VAR_MASK)==0)
#define DB_VAR(i) ((i) & ~DB_VAR_MASK)
#define DB_CST(i) (i)
#define DB_MAKE_VAR(i) (((db_var_or_cst)(i)) | DB_VAR_MASK)
#define DB_MAKE_CST(i) ((db_var_or_cst)(i))

struct db_proj_spec { /* Projecting onto tuples of nsubfields fields
			 - head[nsubfields]: lists whether each field should be
			 a variable or a constant, and what number it has
			 (DB_VAR_NONE cannot be used here)
			 - constants[]: array containing the constants referenced in head[]
			 - hash[]: their hash codes
		       */
  int nsubfields;
  db_var_or_cst *head;
  ovm_var_t **constants;
  unsigned long *hash;
};

struct db_filter_spec { /* Filtering those tuples satisfying a condition
			   The tuples have nfields fields.
			   - vals[nfields]: either DB_VAR_NONE (no condition),
			   a constant (which the field must match literally),
			   or a variable of strictly lower number (which should
			   be compared for equality).
			   - constants[]: array containing the constants referenced in vals[]
			   - hash[]: their hash codes
			 */
  int nfields_res; /* number of fields in the produced tuples, should be equal
		      to the number of DB_VAR_NONE entries in vals[] */
  int nfields; /* number of fields in the input tuples */
  db_var_or_cst *vals;
  ovm_var_t **constants;
  unsigned long *hash;
  int (*test) (gc_t *gc_ctx, int nfields, OBJTYPE *t, void *data);
  void *data;
};

struct db_join_spec { /* Joining two db_maps, of nfields1 and nfields2 fields each.
			 - vals[nfields2]: either DB_VAR_NONE (no joining on this field),
			 or a variable (field number) between 0 and nfields1-1 in
			 the first table; cannot be a constant.
		       */
  int nfields1, nfields2;
  db_var_or_cst *vals;
};



#endif /*DB*/

/*
** Copyright (c) 2015 by Jean GOUBAULT-LARRECQ, Laboratoire Spécification
** et Vérification (LSV), CNRS UMR 8643 & ENS Cachan.
**
** Jean GOUBAULT-LARRECQ <goubaut@lsv.ens-cachan.fr>
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
