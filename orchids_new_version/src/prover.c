/**
 ** @file prover.c
 ** Prover for quantifier-free formulae, used by rule_compiler.c
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "rule_compiler.h"
#include "issdl.tab.h"
#include "ovm.h"
#include "prover.h"

static void formula_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  formula_t *n = (formula_t *)p;

  switch (n->f_op)
    {
    case F_EQUAL: case F_LEQ:
      GC_TOUCH (gc_ctx, n->what.leq.l);
      GC_TOUCH (gc_ctx, n->what.leq.r);
      break;
    case F_ATOM:
      GC_TOUCH (gc_ctx, n->what.atom);
      break;
    case F_AND: case F_OR:
      GC_TOUCH (gc_ctx, n->what.bin.l);
      GC_TOUCH (gc_ctx, n->what.bin.r);
      break;
    case F_NOT:
      GC_TOUCH (gc_ctx, n->what.mon.l);
      break;
    }
}

static int formula_traverse (gc_traverse_ctx_t *gtc, gc_header_t *p,
				    void *data)
{
  formula_t *n = (formula_t *)p;
  int err;

  err = 0;
  switch (n->f_op)
    {
    case F_EQUAL: case F_LEQ:
      err = (*gtc->do_subfield) (gtc, (gc_header_t *)n->what.leq.l, data);
      if (err)
	return err;
      err = (*gtc->do_subfield) (gtc, (gc_header_t *)n->what.leq.r, data);
      break;
    case F_ATOM:
      err = (*gtc->do_subfield) (gtc, (gc_header_t *)n->what.atom, data);
      break;
    case F_AND: case F_OR:
      err = (*gtc->do_subfield) (gtc, (gc_header_t *)n->what.bin.l, data);
      if (err)
	return err;
      err = (*gtc->do_subfield) (gtc, (gc_header_t *)n->what.bin.r, data);
      break;
    case F_NOT:
      err = (*gtc->do_subfield) (gtc, (gc_header_t *)n->what.mon.l, data);
      break;
    }
  return err;
}

static gc_class_t formula_class = {
  GC_ID('f','o','r','m'),
  formula_mark_subfields,
  NULL,
  formula_traverse
};

static void branch_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  branch_t *b = (branch_t *)p;

  GC_TOUCH (gc_ctx, b->next);
  GC_TOUCH (gc_ctx, b->f);
}

static int branch_traverse (gc_traverse_ctx_t *gtc, gc_header_t *p,
			    void *data)
{
  branch_t *b = (branch_t *)p;
  int err;

  err = (*gtc->do_subfield) (gtc, (gc_header_t *)b->next, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *)b->f, data);
  return err;
}

static gc_class_t branch_class = {
  GC_ID('b','r','c','h'),
  branch_mark_subfields,
  NULL,
  branch_traverse
};

formula_t *make_formula (gc_t *gc_ctx, struct node_expr_s *e, int negate)
{
  formula_t *f, *l, *r;
  
  GC_START (gc_ctx, 2);
  if (e==NULL)
    {
    dont_know:
      f = gc_alloc (gc_ctx, sizeof(formula_t), &formula_class);
      f->gc.type = T_NULL;
      f->f_op = F_DONT_KNOW;
    }
  else switch (e->type)
    {
    case NODE_COND:
      switch (BIN_OP(e))
	{
	case ANDAND:
	  l = make_formula (gc_ctx, BIN_LVAL(e), negate);
	  GC_UPDATE (gc_ctx, 0, l);
	  r = make_formula (gc_ctx, BIN_RVAL(e), negate);
	  GC_UPDATE (gc_ctx, 0, r);
	  f = gc_alloc (gc_ctx, sizeof(formula_t), &formula_class);
	  f->gc.type = T_NULL;
	  f->f_op = negate?F_OR:F_AND;
	  GC_TOUCH (gc_ctx, f->what.bin.l = l);
	  GC_TOUCH (gc_ctx, f->what.bin.r = r);
	  break;
	case OROR:
	  l = make_formula (gc_ctx, BIN_LVAL(e), negate);
	  GC_UPDATE (gc_ctx, 0, l);
	  r = make_formula (gc_ctx, BIN_RVAL(e), negate);
	  GC_UPDATE (gc_ctx, 0, r);
	  f = gc_alloc (gc_ctx, sizeof(formula_t), &formula_class);
	  f->gc.type = T_NULL;
	  f->f_op = negate?F_AND:F_OR;
	  GC_TOUCH (gc_ctx, f->what.bin.l = l);
	  GC_TOUCH (gc_ctx, f->what.bin.r = r);
	  break;
	case BANG: /* negated argument is BIN_RVAL */
	  f = make_formula (gc_ctx, BIN_RVAL(e), !negate);
	  break;
	case OP_CNEQ:
	  negate = !negate;
	  /*fallthrough*/
	case OP_CEQ:
	  f = gc_alloc (gc_ctx, sizeof(formula_t), &formula_class);
	  f->gc.type = T_NULL;
	  f->f_op = F_EQUAL;
	  GC_TOUCH (gc_ctx, f->what.leq.l = BIN_LVAL(e));
	  GC_TOUCH (gc_ctx, f->what.leq.r = BIN_RVAL(e));
	  goto neg_atom;
	case OP_CNRM:
	  negate = !negate;
	  /*fallthrough*/
	case OP_CRM:
	  f = gc_alloc (gc_ctx, sizeof(formula_t), &formula_class);
	  f->gc.type = T_NULL;
	  f->f_op = F_REGEXP_MATCH;
	  GC_TOUCH (gc_ctx, f->what.leq.l = BIN_LVAL(e));
	  GC_TOUCH (gc_ctx, f->what.leq.r = BIN_RVAL(e));
	  goto neg_atom;
	case OP_CLE:
	  f = gc_alloc (gc_ctx, sizeof(formula_t), &formula_class);
	  f->gc.type = T_NULL;
	  f->f_op = F_LEQ;
	  GC_TOUCH (gc_ctx, f->what.leq.l = BIN_LVAL(e));
	  GC_TOUCH (gc_ctx, f->what.leq.r = BIN_RVAL(e));
	  goto neg_atom;
	case OP_CGE:
	  f = gc_alloc (gc_ctx, sizeof(formula_t), &formula_class);
	  f->gc.type = T_NULL;
	  f->f_op = F_LEQ;
	  GC_TOUCH (gc_ctx, f->what.leq.l = BIN_RVAL(e));
	  GC_TOUCH (gc_ctx, f->what.leq.r = BIN_LVAL(e));
	  goto neg_atom;
	case OP_CLT:
	  f = gc_alloc (gc_ctx, sizeof(formula_t), &formula_class);
	  f->gc.type = T_NULL;
	  f->f_op = F_LT;
	  GC_TOUCH (gc_ctx, f->what.leq.l = BIN_LVAL(e));
	  GC_TOUCH (gc_ctx, f->what.leq.r = BIN_RVAL(e));
	  goto neg_atom;
	case OP_CGT:
	  f = gc_alloc (gc_ctx, sizeof(formula_t), &formula_class);
	  f->gc.type = T_NULL;
	  f->f_op = F_LT;
	  GC_TOUCH (gc_ctx, f->what.leq.l = BIN_RVAL(e));
	  GC_TOUCH (gc_ctx, f->what.leq.r = BIN_LVAL(e));
	  goto neg_atom;
	default: /* there should be no other case */
	  goto dont_know;
	}
      break;
    default:
      f = gc_alloc (gc_ctx, sizeof(formula_t), &formula_class);
      f->gc.type = T_NULL;
      f->f_op = F_ATOM;
      GC_TOUCH (gc_ctx, f->what.atom = e);
    neg_atom:
      if (negate)
	{
	  l = f;
	  GC_UPDATE (gc_ctx, 0, l);
	  f = gc_alloc (gc_ctx, sizeof(formula_t), &formula_class);
	  f->gc.type = T_NULL;
	  f->f_op = F_NOT;
	  GC_TOUCH (gc_ctx, f->what.mon.l = l);
	}
      break;
    }
  GC_END (gc_ctx);
  return f;
}

static branch_t *b_cons (gc_t *gc_ctx, formula_t *f, branch_t *b)
{
  branch_t *bb;

  bb = gc_alloc (gc_ctx, sizeof(branch_t), &branch_class);
  bb->gc.type = T_NULL;
  GC_TOUCH (gc_ctx, bb->next = b);
  GC_TOUCH (gc_ctx, bb->f = f);
  return bb;
}

static int contradict_p (gc_t *gc_ctx, formula_t *f, branch_t *pos)
/* Does (not f) contradict the conjunction of formulae in pos?
 f is F_EQUAL, F_REGEXP_MATCH, F_LEQ, F_LT, or F_ATOM */
{
  branch_t *b;
  formula_t *ff;

  switch (f->f_op)
    {
    case F_ATOM:
      for (b=pos; b!=NULL; b=b->next)
	{
	  ff = b->f;
	  if (ff->f_op==F_ATOM && node_expr_equal (f->what.atom, ff->what.atom))
	    return 1;
	}
      break;
    case F_REGEXP_MATCH:
    case F_EQUAL:
    case F_LT:
      for (b=pos; b!=NULL; b=b->next)
	{
	  ff = b->f;
	  if (ff->f_op==f->f_op &&
	      node_expr_equal (f->what.leq.l, ff->what.leq.l) &&
	      node_expr_equal (f->what.leq.r, ff->what.leq.r))
	    return 1;
	}
      break;
    case F_LEQ:
      for (b=pos; b!=NULL; b=b->next)
	{
	  ff = b->f;
	  if ((ff->f_op==F_LEQ || ff->f_op==F_LT || ff->f_op==F_EQUAL) &&
	      node_expr_equal (f->what.leq.l, ff->what.leq.l) &&
	      node_expr_equal (f->what.leq.r, ff->what.leq.r))
	    return 1;
	}
      break;
    }
  return 0;
}

static int contradict_n (gc_t *gc_ctx, formula_t *f, branch_t *neg)
/* Does f contradict the conjunction of the negations of the formulae in neg?
   I.e., does f imply the disjunction of the formulae in neg?
   f is F_EQUAL, F_REGEXP_MATCH, F_LEQ, F_LT, or F_ATOM */
{
  branch_t *b;
  formula_t *ff;

  switch (f->f_op)
    {
    case F_ATOM:
      for (b=neg; b!=NULL; b=b->next)
	{
	  ff = b->f;
	  if (ff->f_op==F_ATOM && node_expr_equal (f->what.atom, ff->what.atom))
	    return 1;
	}
      break;
    case F_REGEXP_MATCH:
    case F_LEQ:
      for (b=neg; b!=NULL; b=b->next)
	{
	  ff = b->f;
	  if (ff->f_op==f->f_op &&
	      node_expr_equal (f->what.leq.l, ff->what.leq.l) &&
	      node_expr_equal (f->what.leq.r, ff->what.leq.r))
	    return 1;
	}
      break;
    case F_EQUAL:
    case F_LT:
      for (b=neg; b!=NULL; b=b->next)
	{
	  ff = b->f;
	  if ((ff->f_op==f->f_op || ff->f_op==F_EQUAL) &&
	      node_expr_equal (f->what.leq.l, ff->what.leq.l) &&
	      node_expr_equal (f->what.leq.r, ff->what.leq.r))
	    return 1;
	}
      break;
    }
  return 0;
}

int refute (gc_t *gc_ctx, branch_t *pos, branch_t *neg, branch_t *unsorted)
{
  formula_t *f;
  int ret;

  GC_START (gc_ctx, 1);
 again:
  if (unsorted==NULL)
    ret = 0; /* no contradiction */
  else
    {
      f = unsorted->f;
      switch (f->f_op)
	{
	case F_FALSE:
	  ret = 1; /* always contradictory */
	  break;
	case F_TRUE:
	  unsorted = unsorted->next;
	  goto again;
	case F_DONT_KNOW: /* we are optimistic: we shall never find a contradiction with
			     F_DONT_KNOW, so do as for F_TRUE */
	  unsorted = unsorted->next;
	  goto again;
	case F_AND:
	  unsorted = b_cons (gc_ctx, f->what.bin.r, unsorted->next);
	  GC_UPDATE (gc_ctx, 0, unsorted);
	  unsorted = b_cons (gc_ctx, f->what.bin.l, unsorted);
	  ret = refute (gc_ctx, pos, neg, unsorted);
	  break;
	case F_OR:
	  unsorted = b_cons (gc_ctx, f->what.bin.l, unsorted->next);
	  GC_UPDATE (gc_ctx, 0, unsorted);
	  ret = refute (gc_ctx, pos, neg, unsorted);
	  if (ret==0)
	    break;
	  unsorted = b_cons (gc_ctx, f->what.bin.r, unsorted->next);
	  GC_UPDATE (gc_ctx, 0, unsorted);
	  ret = refute (gc_ctx, pos, neg, unsorted);
	  break;
	case F_NOT:
	  if (contradict_p (gc_ctx, f->what.mon.l, pos))
	    ret = 1;
	  else
	    {
	      neg = b_cons (gc_ctx, f->what.mon.l, neg);
	      GC_UPDATE (gc_ctx, 0, neg);
	      ret = refute (gc_ctx, pos, neg, unsorted->next);
	    }
	  break;
	default:
	  if (contradict_n (gc_ctx, f, neg))
	    ret = 1;
	  else
	    {
	      pos = b_cons (gc_ctx, f, pos);
	      GC_UPDATE (gc_ctx, 0, pos);
	      ret = refute (gc_ctx, pos, neg, unsorted->next);
	    }
	  break;
	}
    }
  GC_END (gc_ctx);
  return ret;
}

int node_expr_incompatible (gc_t *gc_ctx, size_t nexprs, struct node_expr_s **exprs)
{
  int ret;
  formula_t *f;
  branch_t *b;
  size_t i;
  
  GC_START (gc_ctx, 1);
  b = NULL;
  for (i=0; i<nexprs; i++)
    {
      f = make_formula (gc_ctx, exprs[i], 0);
      GC_UPDATE (gc_ctx, 0, f);
      b = b_cons (gc_ctx, f, b);
      GC_UPDATE (gc_ctx, 0, b);
    }
  ret = refute_simple (gc_ctx, b);
  GC_END (gc_ctx);
  return ret;
}

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
