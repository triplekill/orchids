/**
 ** @file prover.c
 ** Data structures for prover.c
 **
 ** @author Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
 **
 ** @version 1.0
 ** @ingroup compiler
 **
 ** @date  Started on: Sam 15 nov 2014 16:42:47 UTC
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef PROVER_H
#define PROVER_H

#include "orchids.h"

struct node_expr_s;

/* We implement a simple tableaux prover, for a quantifier-free
   logic with three truth values (false, true, I don't know).
   Atomic formulas are built from == and <=, where <= is a partial ordering.
   Expressions are side-effect free.
   There is a huge room for improvement in terms of efficiency,
   and in terms of precision (more axioms should be included, constants
   should be evaluated at parse time).
*/

#define F_FALSE 0x0
#define F_TRUE 0x1
#define F_DONT_KNOW 0x2
#define F_EQUAL 0x3
#define F_REGEXP_MATCH 0x4
#define F_LEQ 0x5
#define F_LT 0x6
#define F_ATOM 0x7
#define F_AND 0x8
#define F_OR 0x9
#define F_NOT 0xa

typedef struct formula_s formula_t;
struct formula_s {
  gc_header_t gc;
  int f_op;
  union {
    struct { struct node_expr_s *l, *r; } leq; /* F_EQUAL, F_REGEXP_MATCH, F_LEQ, F_LT */
    struct node_expr_s *atom; /* F_ATOM */
    struct { struct formula_s *l, *r; } bin; /* F_AND, F_OR */
    struct { struct formula_s *l; } mon; /* F_NOT: only applied to leq or atom
					    (in negation normal form) */
  } what;
};

typedef struct branch_s branch_t;
struct branch_s {
  gc_header_t gc;
  struct branch_s *next;
  struct formula_s *f; /* must be an equality or an inequality */
};

formula_t *make_formula (gc_t *gc_ctx, struct node_expr_s *e, int negate);

int refute (gc_t *gc_ctx, branch_t *pos, branch_t *neg, branch_t *unsorted);
#define refute_simple(gc_ctx,formulae) refute(gc_ctx,NULL,NULL,formulae)

int node_expr_incompatible (gc_t *gc_ctx, size_t nexprs, struct node_expr_s **exprs);

#endif

/*
** Copyright (c) 2014-2015 by Jean GOUBAULT-LARRECQ, Laboratoire Spécification
** et Vérification (LSV), CNRS UMR 8643 & ENS Cachan.
**
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
