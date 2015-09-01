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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>

#include "orchids.h"
#include "gc.h"
#include "lang.h"
#include "hash.h"
#include "db.h"

static db_map *empty_db;
static int db_nfields (db_map *m);

/* A total ordering on objects stored in databases:
   this is total because of typing.  Notably we never
   store (partially ordered) databases into databases.
   It is important that the order is total.  We crucially
   rely on it (implicitly) everywhere in this file.  */
#define OBJCMP(s1,s2) issdl_cmp_2(s1,s2)

static int issdl_cmp_2 (ovm_var_t *var1, ovm_var_t *var2)
{ /* like issdl_cmp(), except NULL is considered smaller
     than everything else */
  if (var1==NULL)
    {
      if (var2==NULL)
	return CMP_EQ;
      return CMP_LT;
    }
  if (var2==NULL)
    return CMP_GT;
  return issdl_cmp (var1, var2, CMP_LEQ_MASK | CMP_GEQ_MASK);
}

unsigned long issdl_hash (ovm_var_t *var)
{
  hkey_t *s;
  size_t len;

  s = issdl_get_data (var);
  len = issdl_get_data_len (var);
  return hash_djb (s, len);
}

static int db_tuple_cmp (int nfields, ovm_var_t **t1, ovm_var_t **t2)
{
  int i, res;

  for (i=0; i<nfields; i++)
    {
      res = OBJCMP(t1[i], t2[i]);
      if (CMP_EQUAL(res))
	continue;
      return res;
    }
  return CMP_EQ;
}

void db_small_table_check (int nfields, db_small_table *t)
{
  db_small_table *tt;
  int cmp;

  if (t==NULL)
    {
      fprintf (stderr, "db_small_table is empty: 0x%p.\n", t);
      fflush (stderr);
      abort ();
    }
  if (t->nfields!=nfields)
    {
      fprintf (stderr, "db_small_table: has %d fields, should have %d.\n", t->nfields, nfields);
      fflush (stderr);
      abort ();
    }
  for (tt=t; tt=t->next, tt!=NULL; t=tt)
    {
      if (tt->nfields!=nfields)
	{
	  fprintf (stderr, "db_small_table: has %d fields, should have %d.\n",
		   tt->nfields, nfields);
	  fflush (stderr);
	  abort ();
	}
      cmp = db_tuple_cmp (nfields, t->tuple, tt->tuple);
      if (CMP_LESS(cmp))
	continue;
      if (CMP_EQUAL(cmp))
	fprintf (stderr, "db_small_table: duplicate entry: 0x%p.\n", t);
      else
	fprintf (stderr, "db_small_table: ordered the wrong way: 0x%p.\n", t);
      fflush (stderr);
      abort ();
    }
}

void db_map_check_small (int nfields, db_map *m)
/* m is a tuple */
{
  int i;
  db_small_table *t;
  unsigned long hash;

  db_small_table_check (nfields, m->what.tuples.table);
  for (t=m->what.tuples.table; t!=NULL; t=t->next)
    for (i=0; i<nfields; i++)
      {
	hash = issdl_hash (t->tuple[i]);
	if (hash!=m->what.tuples.hash[i])
	  {
	    fprintf (stderr, "db_map_check_small: wrong hash[%d]: 0x%p.\n", i, t);
	    fflush (stderr);
	    abort ();
	  }
      }
}

void db_map_check_large (int nfields, db_map *m,
			 unsigned long hash[],
			 int field, unsigned long mask)
{
  int newfield;
  unsigned long newmask;
  int i;

  if (m==NULL)
    return;
  switch (m->gc.type)
    {
    case T_DB_EMPTY:
      return;
    case T_DB_MAP:
      {
	newfield = field+1;
	newmask = mask;
	if (newfield>=nfields)
	  {
	    newfield = 0;
	    newmask <<= 1;
	  }
	hash[field] &= ~mask;
	db_map_check_large (nfields, m->what.branch.left, hash, newfield, newmask);
	hash[field] |= mask;
	db_map_check_large (nfields, m->what.branch.right, hash, newfield, newmask);
	i = 0;
	if (m->what.branch.left->gc.type!=T_DB_EMPTY)
	  {
	    if (m->what.branch.left->gc.type==T_DB_MAP)
	      i += 2;
	    else
	      i++;
	  }
	if (m->what.branch.right->gc.type!=T_DB_EMPTY)
	  {
	    if (m->what.branch.right->gc.type==T_DB_MAP)
	      i += 2;
	    else
	      i++;
	  }
	if (i<=1)
	  {
	    fprintf (stderr,
		     "db_map_check_large: branch on too few tuples: 0x%p.\n", m);
	    fflush (stderr);
	    abort ();
	  }
      }
    case T_DB_SINGLETON:
      {
	newmask = (mask-1) | mask;
	for (i=0; i<field; i++)
	  {
	    if ((m->what.tuples.hash[i] & newmask) != (hash[i] & newmask))
	      {
		fprintf (stderr,
			 "db_map_check_large: hash[%d] on wrong branch: 0x%p.\n", i, m);
		fflush (stderr);
		abort ();
	      }	  
	  }
	newmask = mask-1;
	for (; i<nfields; i++)
	  {
	    if ((m->what.tuples.hash[i] & newmask) != (hash[i] & newmask))
	      {
		fprintf (stderr,
			 "db_map_check_large: hash[%d] on wrong branch: 0x%p.\n", i, m);
		fflush (stderr);
		abort ();
	      }	  
	  }
	db_map_check_small (nfields, m);
      }
    default:
      fprintf (stderr, "db_map_check_large: wrong gc.type %d.\n", m->gc.type);
      fflush (stderr);
      abort ();
    }
}

void db_map_check (gc_t *gc_ctx, int nfields, db_map *m)
{
  unsigned long *hash;

  hash = gc_base_malloc (gc_ctx, nfields*sizeof(unsigned long));
  db_map_check_large (nfields, m, hash, 0, 1L);
  gc_base_free (hash);
}

static void db_small_table_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  int i, n;
  db_small_table *t = (db_small_table *)p;

  for (n=t->nfields, i=0; i<n; i++)
    GC_TOUCH (gc_ctx, t->tuple[i]);
  GC_TOUCH (gc_ctx, t->next);
}

static int db_small_table_traverse (gc_traverse_ctx_t *gtc, gc_header_t *p,
				    void *data)
{
  int i, n;
  db_small_table *t = (db_small_table *)p;
  int err = 0;

  for (n=t->nfields, i=0; i<n; i++)
    {
      err = (*gtc->do_subfield) (gtc, (gc_header_t *)t->tuple[i], data);
      if (err)
	return err;
    }
  err = (*gtc->do_subfield) (gtc, (gc_header_t *)t->next, data);
  return err;
}

static int db_small_table_save (save_ctx_t *sctx, gc_header_t *p)
{
  int i, n, err;
  db_small_table *t = (db_small_table *)p;

  n = t->nfields;
  err = save_int (sctx, n);
  if (err) return err;
  for (i=0; i<n; i++)
    {
      err = save_gc_struct (sctx, (gc_header_t *)t->tuple[i]);
      if (err) return err;
    }
  err = save_gc_struct (sctx, (gc_header_t *)t->next);
  return err;
}

gc_class_t db_small_table_class;

static gc_header_t *db_small_table_restore (restore_ctx_t *rctx)
{
  int i, n, err;
  gc_t *gc_ctx = rctx->gc_ctx;
  gc_header_t *p;
  db_small_table *t;

  GC_START (gc_ctx, 1);
  err = restore_int (rctx, &n);
  if (err) goto errlab;
  t = gc_alloc (gc_ctx, DBST_SIZE(n), &db_small_table_class);
  t->gc.type = T_DB_SMALL_TABLE;
  t->nfields = n;
  t->next = NULL;
  for (i=0; i<n; i++)
    t->tuple[i] = NULL;
  GC_UPDATE (gc_ctx, 0, t);
  for (i=0; i<n; i++)
    {
      p = restore_gc_struct (rctx);
      if (p==NULL && errno!=0)
	goto err;
      GC_TOUCH (gc_ctx, t->tuple[i] = (OBJTYPE)p);
    }
  p = restore_gc_struct (rctx);
  if (p==NULL && errno!=0)
    goto err;
  if (p!=NULL && TYPE(p)!=T_DB_SMALL_TABLE)
    { errno = -3; goto errlab; }
  if (p!=NULL && ((db_small_table *)p)->nfields!=n)
    { errno = -4; goto errlab; }
  GC_TOUCH (gc_ctx, t->next = (db_small_table *)p);
  GC_END (gc_ctx);
  goto normal;
 errlab:
  errno = err;
 err:
  t = NULL;
 normal:
  return (gc_header_t *)t;
}

gc_class_t db_small_table_class = {
  GC_ID('d','b','s','t'),
  db_small_table_mark_subfields,
  NULL,
  db_small_table_traverse,
  db_small_table_save,
  db_small_table_restore
};

static void db_tuples_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  db_map *m = (db_map *)p;

  GC_TOUCH (gc_ctx, m->what.tuples.table);
}

static int db_tuples_traverse (gc_traverse_ctx_t *gtc, gc_header_t *p,
			       void *data)
{
  db_map *m = (db_map *)p;
  int err = 0;

  err = (*gtc->do_subfield) (gtc, (gc_header_t *)m->what.tuples.table, data);
  return err;
}

static int db_tuples_save (save_ctx_t *sctx, gc_header_t *p)
{
  db_map *m = (db_map *)p;
  // int i, nfields;
  int err;

  /* No, don't save hash table: this will be reconstructed
     at restoration time */
  /*
  nfields = m->what.tuples.table->nfields;
  for (i=0; i<nfields; i++)
    {
      err = save_ulong (sctx, m->what.tuples.hash[i]);
      if (err) return err;
    }
  */
  err = save_gc_struct (sctx, (gc_header_t *)m->what.tuples.table);
  return err;
}

gc_class_t db_tuples_class;

static gc_header_t *db_tuples_restore (restore_ctx_t *rctx)
{
  gc_t *gc_ctx = rctx->gc_ctx;
  db_small_table *t, *next;
  db_map *m, *singleton;
  int i, nfields;

  GC_START (gc_ctx, 1);
  t = (db_small_table *)restore_gc_struct (rctx);
  if (t==NULL)
    m = empty_db;
  else if (TYPE(t)!=T_DB_SMALL_TABLE)
    {
      errno = -3;
      m = NULL;
    }
  else
    {
      nfields = t->nfields;
      m = empty_db;
      while (t!=NULL)
	{
	  next = t->next;
	  t->next = NULL;
	  singleton = gc_alloc (gc_ctx, DB_TUPLE_SIZE(nfields), &db_tuples_class);
	  singleton->gc.type = T_DB_SINGLETON;
	  GC_TOUCH (gc_ctx, singleton->what.tuples.table = t);
	  GC_UPDATE (gc_ctx, 0, singleton);
	  for (i=0; i<nfields; i++) /* recompute hashes */
	    singleton->what.tuples.hash[i] = issdl_hash (t->tuple[i]);
	  m = db_union (gc_ctx, nfields, m, singleton);
	  GC_UPDATE (gc_ctx, 0, m);
	  t = next;
	}
    }
  GC_END (gc_ctx);
  return (gc_header_t *)m;
}

gc_class_t db_tuples_class = {
  GC_ID('d','b','t','p'),
  db_tuples_mark_subfields,
  NULL,
  db_tuples_traverse,
  db_tuples_save,
  db_tuples_restore
};

static void db_branch_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  db_map *m = (db_map *)p;

  GC_TOUCH (gc_ctx, m->what.branch.left);
  GC_TOUCH (gc_ctx, m->what.branch.right);
}

static int db_branch_traverse (gc_traverse_ctx_t *gtc, gc_header_t *p,
			       void *data)
{
  db_map *m = (db_map *)p;
  int err = 0;

  err = (*gtc->do_subfield) (gtc, (gc_header_t *)m->what.branch.left, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *)m->what.branch.right, data);
  return err;
}

static int db_branch_save (save_ctx_t *sctx, gc_header_t *p)
{
  db_map *m = (db_map *)p;
  int err;

  err = save_gc_struct (sctx, (gc_header_t *)m->what.branch.left);
  if (err) return err;
  err = save_gc_struct (sctx, (gc_header_t *)m->what.branch.right);
  return err;
}

gc_class_t db_branch_class;

static gc_header_t *db_branch_restore (restore_ctx_t *rctx)
{
  gc_t *gc_ctx = rctx->gc_ctx;
  int nfields1, nfields2;
  db_map *m1, *m2, *m;

  GC_START (gc_ctx, 2);
  m1 = (db_map *)restore_gc_struct (rctx);
  if (m1==NULL || (TYPE(m1)!=T_DB_EMPTY && TYPE(m1)!=T_DB_MAP &&
		   TYPE(m1)!=T_DB_SINGLETON))
    { errno = -3; m = NULL; goto end; }
  GC_UPDATE (gc_ctx, 0, m1);
  nfields1 = db_nfields (m1);
  m2 = (db_map *)restore_gc_struct (rctx);
  if (m2==NULL || (TYPE(m2)!=T_DB_EMPTY && TYPE(m2)!=T_DB_MAP &&
		   TYPE(m2)!=T_DB_SINGLETON))
    { errno = -3; m = NULL; goto end; }
  GC_UPDATE (gc_ctx, 1, m2);
  nfields2 = db_nfields (m2);
  if (nfields1!=nfields2)
    { errno = -4; m = NULL; goto end; }
  m = db_union (gc_ctx, nfields1, m1, m2);
 end:
  GC_END (gc_ctx);
  return (gc_header_t *)m;
}

gc_class_t db_branch_class = {
  GC_ID('d','b','b','r'),
  db_branch_mark_subfields,
  NULL,
  db_branch_traverse,
  db_branch_save,
  db_branch_restore
};

static db_small_table *db_small_table_union (gc_t *gc_ctx, int nfields,
					     db_small_table *t1, db_small_table *t2)
/* merges the two lists of tuples (of nfields elements) in
   a sorted list without duplicates */
{
  int cmp;
  db_small_table *res, *next;
  int i;

  if (t1==NULL)
    return t2;
  if (t2==NULL)
    return t1;
  GC_START(gc_ctx, 1);
  cmp = db_tuple_cmp (nfields, t1->tuple, t2->tuple);
  if (CMP_LESS(cmp))
    {
      next = db_small_table_union (gc_ctx, nfields, t1->next, t2);
      GC_UPDATE(gc_ctx,0,next);
      res = gc_alloc (gc_ctx, DBST_SIZE(nfields), &db_small_table_class);
      res->gc.type = T_DB_SMALL_TABLE;
      res->nfields = nfields;
      GC_TOUCH (gc_ctx, res->next = next);
      for (i=0; i<nfields; i++)
	GC_TOUCH (gc_ctx, res->tuple[i] = t1->tuple[i]);
    }
  else if (CMP_GREATER(cmp))
    {
      next = db_small_table_union (gc_ctx, nfields, t1, t2->next);
      GC_UPDATE(gc_ctx,0,next);
      res = gc_alloc (gc_ctx, DBST_SIZE(nfields), &db_small_table_class);
      res->gc.type = T_DB_SMALL_TABLE;
      res->nfields = nfields;
      GC_TOUCH (gc_ctx, res->next = next);
      for (i=0; i<nfields; i++)
	GC_TOUCH (gc_ctx, res->tuple[i] = t2->tuple[i]);
    }
  else /* equal */
    {
      next = db_small_table_union (gc_ctx, nfields, t1->next, t2->next);
      GC_UPDATE(gc_ctx,0,next);
      res = gc_alloc (gc_ctx, DBST_SIZE(nfields), &db_small_table_class);
      res->gc.type = T_DB_SMALL_TABLE;
      res->nfields = nfields;
      GC_TOUCH (gc_ctx, res->next = next);
      for (i=0; i<nfields; i++)
	GC_TOUCH (gc_ctx, res->tuple[i] = t2->tuple[i]);
    }
  GC_END(gc_ctx);
  return res;
}

static db_map *db_union_small_small_diff_hash (gc_t *gc_ctx, int nfields,
					       db_map *m1, db_map *m2,
					       int field, unsigned long mask)
/* m1 and m2 are tuples, and the hashes of m1 and m2 differ */
{
  unsigned long lbit, rbit;
  db_map *son, *m;

  lbit = m1->what.tuples.hash[field] & mask;
  rbit = m2->what.tuples.hash[field] & mask;
  if (lbit==rbit)
    {
      if (++field >= nfields)
	{
	  field = 0;
	  mask <<= 1;
	}
      GC_START(gc_ctx,1);
      son = db_union_small_small_diff_hash (gc_ctx, nfields, m1, m2, field, mask);
      GC_UPDATE(gc_ctx,0,son);
      m = gc_alloc (gc_ctx, DB_BRANCH_SIZE, &db_branch_class);
      m->gc.type = T_DB_MAP;
      if (rbit)
	{
	  GC_TOUCH (gc_ctx, m->what.branch.left = empty_db);
	  GC_TOUCH (gc_ctx, m->what.branch.right = son);
	}
      else
	{
	  GC_TOUCH (gc_ctx, m->what.branch.left = son);
	  GC_TOUCH (gc_ctx, m->what.branch.right = empty_db);
	}
      GC_END(gc_ctx);
    }
  else
    {
      m = gc_alloc (gc_ctx, DB_BRANCH_SIZE, &db_branch_class);
      m->gc.type = T_DB_MAP;
      if (rbit)
	{
	  GC_TOUCH (gc_ctx, m->what.branch.left = m1);
	  GC_TOUCH (gc_ctx, m->what.branch.right = m2);
	}
      else
	{
	  GC_TOUCH (gc_ctx, m->what.branch.left = m2);
	  GC_TOUCH (gc_ctx, m->what.branch.right = m1);
	}
    }
  return m;
}

static db_map *db_union_small_small (gc_t *gc_ctx, int nfields, db_map *m1, db_map *m2,
				     int field, unsigned long mask)
/* m1 and m2 are tuples */
{
  db_map *m;
  db_small_table *t;
  int i;

  for (i=0; i<nfields; i++)
    if (m1->what.tuples.hash[i]!=m2->what.tuples.hash[i])
      return db_union_small_small_diff_hash (gc_ctx, nfields, m1, m2, field, mask);
  t = db_small_table_union (gc_ctx, nfields, m1->what.tuples.table,
			    m2->what.tuples.table);
  GC_START(gc_ctx,1);
  GC_UPDATE(gc_ctx,0,t);
  m = gc_alloc (gc_ctx, DB_TUPLE_SIZE(nfields), &db_tuples_class);
  m->gc.type = T_DB_SINGLETON;
  GC_TOUCH(gc_ctx,m->what.tuples.table = t);
  GC_END(gc_ctx);
  for (i=0; i<nfields; i++)
    m->what.tuples.hash[i] = m1->what.tuples.hash[i];
  return m;
}

static db_map *db_union_small (gc_t *gc_ctx, int nfields, db_map *small, db_map *large,
			       int field, unsigned long mask)
/* small is a tuple, large is anything */
{
  unsigned long bit;
  db_map *res, *son;

  switch (large->gc.type)
    {
    case T_DB_EMPTY:
      return small;
    case T_DB_SINGLETON:
      return db_union_small_small (gc_ctx, nfields, small, large, field, mask);
    case T_DB_MAP:
      /* now large is a branch */
      GC_START(gc_ctx,1);
      bit = small->what.tuples.hash[field] & mask;
      if (++field>=nfields)
	{
	  field = 0;
	  mask <<= 1;
	}
      if (bit)
	{
	  son = db_union_small (gc_ctx, nfields, small, large->what.branch.right, field, mask);
	  GC_UPDATE(gc_ctx,0,son);
	  res = gc_alloc (gc_ctx, DB_BRANCH_SIZE, &db_branch_class);
	  res->gc.type = T_DB_MAP;
	  GC_TOUCH (gc_ctx, res->what.branch.left = large->what.branch.left);
	  GC_TOUCH (gc_ctx, res->what.branch.right = son);
	}
      else
	{
	  son = db_union_small (gc_ctx, nfields, small, large->what.branch.left, field, mask);
	  GC_UPDATE(gc_ctx,0,son);
	  res = gc_alloc (gc_ctx, DB_BRANCH_SIZE, &db_branch_class);
	  res->gc.type = T_DB_MAP;
	  GC_TOUCH (gc_ctx, res->what.branch.left = son);
	  GC_TOUCH (gc_ctx, res->what.branch.right = large->what.branch.right);
	}
      GC_END(gc_ctx);
      return res;
    default:
      DebugLog(DF_OVM, DS_DEBUG, "Type error in db_union_small()\n");
      return NULL;
    }
}

static db_map *db_union_large (gc_t *gc_ctx, int nfields, db_map *m1, db_map *m2,
			       int field, unsigned long mask)
{
  db_map *res, *left, *right;

  if (m1->gc.type==T_DB_EMPTY)
    return m2;
  if (m2->gc.type==T_DB_EMPTY)
    return m1;
  if (m1->gc.type==T_DB_MAP)
    {
      if (m2->gc.type==T_DB_MAP)
	{
	  GC_START(gc_ctx,2);
	  if (++field >= nfields)
	    {
	      field = 0;
	      mask <<= 1;
	    }
	  left = db_union_large (gc_ctx, nfields, m1->what.branch.left, m2->what.branch.left,
				 field, mask);
	  GC_UPDATE(gc_ctx,0,left);
	  right = db_union_large (gc_ctx, nfields, m1->what.branch.right, m2->what.branch.right,
				  field, mask);
	  GC_UPDATE(gc_ctx,1,right);
	  res = gc_alloc (gc_ctx, DB_BRANCH_SIZE, &db_branch_class);
	  res->gc.type = T_DB_MAP;
	  GC_TOUCH (gc_ctx, res->what.branch.left = left);
	  GC_TOUCH (gc_ctx, res->what.branch.right = right);
	  GC_END(gc_ctx);
	}
      else
	{
	  res = db_union_small (gc_ctx, nfields, m2, m1, field, mask);
	}
    }
  else
    {
      res = db_union_small (gc_ctx, nfields, m1, m2, field, mask);
    }
  return res;
}

static int db_nfields (db_map *m)
{
  while (1)
    {
      if (m->gc.type==T_DB_MAP)
	{
	  if (m->what.branch.left==NULL)
	    m = m->what.branch.right;
	  else m = m->what.branch.left;
	}
      else return m->what.tuples.table->nfields;
    }
}

db_map *db_union (gc_t *gc_ctx, int nfields, db_map *m1, db_map *m2)
{
  return db_union_large (gc_ctx, nfields, m1, m2, 0, 1L);
}

db_map *db_union_lazy (gc_t *gc_ctx, db_map *m1, db_map *m2)
{
  int nfields;

  if (m1->gc.type==T_DB_EMPTY)
    return m2;
  nfields = db_nfields (m1);
  return db_union (gc_ctx, nfields, m1, m2);
}

static db_small_table *db_proj_small_table (gc_t *gc_ctx,
					    struct db_proj_spec *spec,
					    db_small_table *t,
					    int stride)
/* project the subtable of t of elements at positions 0, stride, 2.stride, etc. */
{
  db_small_table *subt, *t1, *t2;
  db_small_table *res;
  int i, nsubfields;
  unsigned long hd;

  if (t==NULL)
    return t;
  nsubfields = spec->nsubfields;
  for (i=0, subt=t; i<stride && subt!=NULL; i++, subt=subt->next)
      ;
  if (subt==NULL) /* only one element in subtable: create it and return it */
    {
      res = gc_alloc (gc_ctx, DBST_SIZE(nsubfields), &db_small_table_class);
      res->gc.type = T_DB_SMALL_TABLE;
      res->nfields = nsubfields;
      GC_TOUCH (gc_ctx, res->next = NULL);
      for (i=0; i<nsubfields; i++)
	{
	  hd = spec->head[i];
	  if (DB_IS_CST(hd))
	    GC_TOUCH (gc_ctx, res->tuple[i] = spec->constants[DB_CST(hd)]);
	  else GC_TOUCH (gc_ctx, res->tuple[i] = t->tuple[DB_VAR(hd)]);
	}
    }
  else
    {
      stride <<= 1;
      GC_START(gc_ctx,2);
      t1 = db_proj_small_table (gc_ctx, spec, t, stride);
      GC_UPDATE(gc_ctx,0,t1);
      t2 = db_proj_small_table (gc_ctx, spec, subt, stride);
      GC_UPDATE(gc_ctx,1,t2);
      res = db_small_table_union (gc_ctx, nsubfields, t1, t2);
      GC_END(gc_ctx);
    }
  return res;
}

static db_map *db_proj_small (gc_t *gc_ctx,
			      struct db_proj_spec *spec,
			      db_map *m)
/* m is a list of tuples */
{
  db_small_table *rest;
  db_map *res;
  int i, nsubfields;
  unsigned long hd;

  GC_START(gc_ctx,1);
  rest = db_proj_small_table (gc_ctx, spec, m->what.tuples.table, 1);
  GC_UPDATE(gc_ctx,0,rest);
  nsubfields = spec->nsubfields;
  res = gc_alloc (gc_ctx, DB_TUPLE_SIZE(nsubfields), &db_tuples_class);
  res->gc.type = T_DB_SINGLETON;
  GC_TOUCH(gc_ctx,res->what.tuples.table = rest);
  for (i=0; i<nsubfields; i++)
    {
      hd = spec->head[i];
      if (DB_IS_CST(hd))
	res->what.tuples.hash[i] = spec->hash[DB_CST(hd)];
      else res->what.tuples.hash[i] = m->what.tuples.hash[DB_VAR(hd)];
    }
  GC_END(gc_ctx);
  return res;
}

db_map *db_proj (gc_t *gc_ctx,
		 struct db_proj_spec *spec,
		 db_map *m)
{
  db_map *res, *left, *right;

  switch (m->gc.type)
    {
    case T_DB_EMPTY:
      res = empty_db;
      break;
    case T_DB_MAP:
      GC_START(gc_ctx,2);
      left = db_proj (gc_ctx, spec, m->what.branch.left);
      GC_UPDATE(gc_ctx,0,left);
      right = db_proj (gc_ctx, spec, m->what.branch.right);
      GC_UPDATE(gc_ctx,1,right);
      res = db_union (gc_ctx, spec->nsubfields, left, right);
      GC_END(gc_ctx);
      break;
    case T_DB_SINGLETON:
      res = db_proj_small (gc_ctx, spec, m);
      break;
    default:
      DebugLog(DF_OVM, DS_DEBUG, "Type error in db_proj()\n");
      return NULL;
    }
  return res;
}


static db_small_table *db_filter_small_table (gc_t *gc_ctx,
					      struct db_filter_spec *spec,
					      db_small_table *t)
{
  db_small_table *rest;
  db_small_table *last;
  db_small_table *newt;
  int i, j, nfields;
  unsigned long hd;
  int ok;

  nfields = spec->nfields;
  rest = NULL;
  last = NULL;
  GC_START(gc_ctx,1);
  for (; t!=NULL; t=t->next)
    {
      ok = 1;
      for (i=0; i<nfields; i++)
	{
	  hd = spec->vals[i];
	  if (hd==DB_VAR_NONE)
	    continue;
	  if (DB_IS_CST(hd))
	    {
	      if (OBJCMP(t->tuple[i],spec->constants[DB_CST(hd)]) != 0)
	      {
		ok = 0;
		i = nfields; /* to exit the loop */
	      }
	    }
	  else
	    {
	      if (OBJCMP(t->tuple[i],t->tuple[DB_VAR(hd)]) != 0)
		{
		  ok = 0;
		  i = nfields; /* to exit the loop */
		}
	    }
	}
      if (ok && spec->test!=NULL)
	{
	  ok = (*spec->test) (gc_ctx, nfields, t->tuple, spec->data);
	}
      if (ok)
	{ /* copy the tuple */
	  newt = gc_alloc (gc_ctx, DBST_SIZE(spec->nfields_res), &db_small_table_class);
	  newt->gc.type = T_DB_SMALL_TABLE;
	  newt->nfields = spec->nfields_res;
	  GC_TOUCH (gc_ctx, newt->next = NULL);
	  for (i=0, j=0; i<nfields; i++)
	    {
	      hd = spec->vals[i];
	      if (hd==DB_VAR_NONE)
		GC_TOUCH (gc_ctx, newt->tuple[j++] = t->tuple[i]);
	      /* Keep the tuples in the same ordering: a miracle occurs,
		 they will be ordered the same way */
	      if (last==NULL)
		{
		  rest = newt;
		  GC_UPDATE(gc_ctx,0,rest);
		}
	      else
		{
		  GC_TOUCH(gc_ctx, last->next = newt);
		}
	      last = newt;
	    }
	}
    }
  GC_END(gc_ctx);
  return rest;
}

static db_map *db_filter_small (gc_t *gc_ctx, struct db_filter_spec *spec,
				db_map *m)
/* m is a list of tuples */
{
  db_small_table *rest;
  db_map *res;
  int i, j;

  GC_START(gc_ctx,1);
  rest = db_filter_small_table (gc_ctx, spec, m->what.tuples.table);
  if (rest==NULL)
    res = empty_db;
  else
    {
      GC_UPDATE(gc_ctx,0,rest);
      res = gc_alloc (gc_ctx, DB_TUPLE_SIZE(spec->nfields_res),
		      &db_tuples_class);
      res->gc.type = T_DB_SINGLETON;
      GC_TOUCH (gc_ctx, res->what.tuples.table = rest);
      for (i=0, j=0; i<spec->nfields; i++)
	if (spec->vals[i]==DB_VAR_NONE)
	  res->what.tuples.hash[j++] = m->what.tuples.hash[i];
    }
  GC_END(gc_ctx);
  return res;
}

static db_map *db_filter_large (gc_t *gc_ctx,
				struct db_filter_spec *spec,
				db_map *m,
				char *dirs, /* 8*sizeof(unsigned long) arrays of
					       nfields booleans */
				int field, unsigned long mask)
{
  db_map *res, *left, *right;
  int newfield;
  unsigned long newmask;
  char *newdirs;
  unsigned long hd;

  switch (m->gc.type)
    {
    case T_DB_EMPTY:
      res = empty_db;
      break;
    case T_DB_SINGLETON:
      res = db_filter_small (gc_ctx, spec, m);
      break;
    case T_DB_MAP:
      hd = spec->vals[field];
      if (hd==DB_VAR_NONE)
	{ /* any value of this field is OK */
	  newfield = field+1;
	  newmask = mask;
	  newdirs = dirs;
	  if (newfield >= spec->nfields)
	    {
	      newfield = 0;
	      newmask <<= 1;
	      newdirs += 8*sizeof(unsigned long);
	    }
	  GC_START(gc_ctx,2);
	  dirs[field] = 0;
	  left = db_filter_large (gc_ctx, spec, m->what.branch.left,
				  newdirs, newfield, newmask);
	  GC_UPDATE(gc_ctx,0,left);
	  dirs[field] = 1;
	  right = db_filter_large (gc_ctx, spec, m->what.branch.right,
				   newdirs, newfield, newmask);
	  GC_UPDATE(gc_ctx,1,right);
	  res = db_union (gc_ctx, spec->nfields_res, left, right);
	  GC_END(gc_ctx);
	}
      else if (DB_IS_CST(hd))
	{
	  newfield = field+1;
	  newmask = mask;
	  newdirs = dirs;
	  if (newfield >= spec->nfields)
	    {
	      newfield = 0;
	      newmask <<= 1;
	      newdirs += 8*sizeof(unsigned long);
	    }
	  if (spec->hash[DB_CST(hd)] & mask)
	    {
	      dirs[field] = 1;
	      res = db_filter_large (gc_ctx, spec, m->what.branch.right,
				     newdirs, newfield, newmask);
	    }
	  else
	    {
	      dirs[field] = 0;
	      res = db_filter_large (gc_ctx, spec, m->what.branch.left,
				     newdirs, newfield, newmask);
	    }
	}
      else /* variable comparison */
	{
	  newfield = field+1;
	  newmask = mask;
	  newdirs = dirs;
	  if (newfield >= spec->nfields)
	    {
	      newfield = 0;
	      newmask <<= 1;
	      newdirs += 8*sizeof(unsigned long);
	    }
	  if (dirs[DB_VAR(hd)])
	    {
	      dirs[field] = 1;
	      res = db_filter_large (gc_ctx, spec, m->what.branch.right,
				     newdirs, newfield, newmask);
	    }
	  else
	    {
	      dirs[field] = 0;
	      res = db_filter_large (gc_ctx, spec, m->what.branch.left,
				     newdirs, newfield, newmask);
	    }
	}
      break;
    default:
      DebugLog(DF_OVM, DS_DEBUG, "Type error in db_filter_large()\n");
      res = NULL;
      break;
    }
  return res;
}

db_map *db_filter (gc_t *gc_ctx, struct db_filter_spec *spec, db_map *m)
{
  char *dirs;
  db_map *res;

  dirs = gc_base_malloc (gc_ctx, (8*sizeof(unsigned long)) * spec->nfields);
  res = db_filter_large (gc_ctx, spec, m, dirs, 0, 1L);
  gc_base_free (dirs);
  return res;
}

#ifdef UNUSED
static int db_is_in_small_table (int nfields, ovm_var_t **tup, db_small_table *t)
{
  int cmp;

  for (; t!=NULL; t=t->next)
    {
      cmp = db_tuple_cmp (nfields, tup, t->tuple);
      if (CMP_LESS(cmp))
	return 0;
      if (CMP_EQUAL(cmp))
	return 1;
    }
  return 0;
}
#endif

static db_small_table *db_diff_small_table (gc_t *gc_ctx, int nfields,
					    db_small_table *t1,
					    db_small_table *t2)
{
  int cmp;
  db_small_table *res, *next;
  int i;

  if (t1==NULL)
    return NULL;
  if (t2==NULL)
    return t1;
  cmp = db_tuple_cmp (nfields, t1->tuple, t2->tuple);
  if (CMP_LESS(cmp))
    {
      GC_START(gc_ctx,1);
      next = db_diff_small_table (gc_ctx, nfields, t1->next, t2);
      GC_UPDATE(gc_ctx,0,next);
      res = gc_alloc (gc_ctx, DBST_SIZE(nfields), &db_small_table_class);
      res->gc.type = T_DB_SMALL_TABLE;
      res->nfields = nfields;
      GC_TOUCH (gc_ctx, res->next = next);
      for (i=0; i<nfields; i++)
	GC_TOUCH (gc_ctx, res->tuple[i] = t1->tuple[i]);
      GC_END(gc_ctx);
    }
  else if (CMP_GREATER(cmp))
    {
      res = db_diff_small_table (gc_ctx, nfields, t1, t2->next);
    }
  else
    {
      res = db_diff_small_table (gc_ctx, nfields, t1->next, t2->next);
    }
  return res;
}

static db_map *db_diff_small_small (gc_t *gc_ctx, int nfields, db_map *m1, db_map *m2,
				    int field, unsigned long mask)
{
  db_map *m;
  db_small_table *t;
  int i;

  for (i=0; i<nfields; i++)
    if (m1->what.tuples.hash[i]!=m2->what.tuples.hash[i])
      return m1;
  t = db_diff_small_table (gc_ctx, nfields, m1->what.tuples.table, m2->what.tuples.table);
  if (t==NULL)
    m = empty_db;
  else
    {
      GC_START(gc_ctx,1);
      GC_UPDATE(gc_ctx,0,t);
      m = gc_alloc (gc_ctx, DB_TUPLE_SIZE(nfields), &db_tuples_class);
      m->gc.type = T_DB_SINGLETON;
      GC_TOUCH (gc_ctx, m->what.tuples.table = t);
      for (i=0; i<nfields; i++)
	m->what.tuples.hash[i] = m1->what.tuples.hash[i];
      GC_END(gc_ctx);
    }
  return m;
}

static db_map *db_diff_small_large (gc_t *gc_ctx, int nfields, db_map *small, db_map *large,
				    int field, unsigned long mask)
{
  unsigned long bit;

  while (1)
    switch (large->gc.type)
      {
      case T_DB_EMPTY:
	return small;
      case T_DB_SINGLETON:
	return db_diff_small_small (gc_ctx, nfields, small, large, field, mask);
      case T_DB_MAP:
	bit = small->what.tuples.hash[field] & mask;
	if (++field>=nfields)
	  {
	    field = 0;
	    mask <<= 1;
	  }
	if (bit)
	  large = large->what.branch.right;
	else
	  large = large->what.branch.left;
	break;
      default:
	DebugLog(DF_OVM, DS_DEBUG, "Type error in db_diff_small_large()\n");
	return NULL;
      }
}

static db_map *db_diff_large_small (gc_t *gc_ctx, int nfields, db_map *large, db_map *small,
				    int field, unsigned long mask)
{
  unsigned long bit;
  db_map *res, *left, *right;

  switch (large->gc.type)
    {
    case T_DB_EMPTY:
      return empty_db;
    case T_DB_SINGLETON:
      return db_diff_small_small (gc_ctx, nfields, large, small, field, mask);
    case T_DB_MAP:
      GC_START(gc_ctx,1);
      bit = small->what.tuples.hash[field] & mask;
      if (++field>=nfields)
	{
	  field = 0;
	  mask <<= 1;
	}
      if (bit)
	{
	  right = db_diff_large_small (gc_ctx, nfields, large->what.branch.right, small,
				       field, mask);
	  GC_UPDATE(gc_ctx,0,right);
	  left = large->what.branch.left;
	}
      else
	{
	  left = db_diff_large_small (gc_ctx, nfields, large->what.branch.left, small,
				      field, mask);
	  GC_UPDATE(gc_ctx,0,left);
	  right = large->what.branch.right;
	}
      res = NULL;
      if (left->gc.type==T_DB_EMPTY)
	{
	  if (right->gc.type!=T_DB_MAP)
	    res = right;
	}
      else if (right->gc.type==T_DB_EMPTY && left->gc.type==T_DB_SINGLETON)
	res = left;
      if (res==NULL)
	{
	  res = gc_alloc (gc_ctx, DB_BRANCH_SIZE, &db_branch_class);
	  res->gc.type = T_DB_MAP;
	  GC_TOUCH (gc_ctx, res->what.branch.left = left);
	  GC_TOUCH (gc_ctx, res->what.branch.right = right);
	}
      GC_END(gc_ctx);
      return res;
    default:
      DebugLog(DF_OVM, DS_DEBUG, "Type error in db_diff_small_large()\n");
      return NULL;
    }
}

static db_map *db_diff_large (gc_t *gc_ctx, int nfields, db_map *m1, db_map *m2,
			      int field, unsigned long mask)
{
  db_map *left, *right, *res;

  if (m1->gc.type==T_DB_EMPTY)
    return m1; /* empty */
  if (m2->gc.type==T_DB_EMPTY)
    return m1;
  if (m1->gc.type==T_DB_MAP)
    {
      if (m2->gc.type==T_DB_MAP)
	{
	  GC_START(gc_ctx,2);
	  if (++field >= nfields)
	    {
	      field = 0;
	      mask <<= 1;
	    }
	  left = db_diff_large (gc_ctx, nfields, m1->what.branch.left, m2->what.branch.left,
				field, mask);
	  GC_UPDATE(gc_ctx,0,left);
	  right = db_diff_large (gc_ctx, nfields, m1->what.branch.right, m2->what.branch.right,
				 field, mask);
	  GC_UPDATE(gc_ctx,1,right);
	  res = NULL;
	  if (left->gc.type==T_DB_EMPTY)
	    {
	      if (right->gc.type!=T_DB_MAP)
		res = right;
	    }
	  else if (right->gc.type==T_DB_EMPTY && left->gc.type==T_DB_SINGLETON)
	    res = left;
	  if (res==NULL)
	    {
	      res = gc_alloc (gc_ctx, DB_BRANCH_SIZE, &db_branch_class);
	      res->gc.type = T_DB_MAP;
	      GC_TOUCH (gc_ctx, res->what.branch.left = left);
	      GC_TOUCH (gc_ctx, res->what.branch.right = right);
	    }
	  GC_END(gc_ctx);
	}
      else
	{
	  res = db_diff_large_small (gc_ctx, nfields, m1, m2, field, mask);
	}
    }
  else
    {
      res = db_diff_small_large (gc_ctx, nfields, m1, m2, field, mask);
    }
  return res;
}

db_map *db_diff (gc_t *gc_ctx, int nfields, db_map *m1, db_map *m2)
{
  return db_diff_large (gc_ctx, nfields, m1, m2, 0, 1L);
}

db_map *db_diff_lazy (gc_t *gc_ctx, db_map *m1, db_map *m2)
{
  int nfields;

  if (m1->gc.type==T_DB_EMPTY)
    return m1;
  nfields = db_nfields (m1);
  return db_diff (gc_ctx, nfields, m1, m2);
}

static int db_included_small_table (int nfields,
				    db_small_table *t1,
				    db_small_table *t2)
{
  int cmp;

  while (1)
    {
      if (t1==NULL)
	return 1;
      if (t2==NULL)
	return 0;
      cmp = db_tuple_cmp (nfields, t1->tuple, t2->tuple);
      if (CMP_LESS(cmp))
	return 0;
      if (CMP_EQUAL(cmp))
	t1 = t1->next;
      t2 = t2->next;
    }
}

static int db_included_small_small (int nfields, db_map *m1, db_map *m2,
				    int field, unsigned long mask)
{
  int i;

  for (i=0; i<nfields; i++)
    if (m1->what.tuples.hash[i]!=m2->what.tuples.hash[i])
      return 0;
  return db_included_small_table (nfields, m1->what.tuples.table, m2->what.tuples.table);
}

static int db_included_small_large (int nfields, db_map *small, db_map *large,
				    int field, unsigned long mask)
{
  unsigned long bit;

  while (1)
    switch (large->gc.type)
      {
      case T_DB_EMPTY:
	return 0;
      case T_DB_SINGLETON:
	return db_included_small_small (nfields, small, large, field, mask);
      case T_DB_MAP:
	bit = small->what.tuples.hash[field] & mask;
	if (++field>=nfields)
	  {
	    field = 0;
	    mask <<= 1;
	  }
	if (bit)
	  large = large->what.branch.right;
	else
	  large = large->what.branch.left;
	break;
      default:
	DebugLog(DF_OVM, DS_DEBUG, "Type error in db_included_small_large()\n");
	return 0;
      }
}

static int db_included_large (int nfields, db_map *m1, db_map *m2,
			      int field, unsigned long mask)
{
  if (m1->gc.type==T_DB_EMPTY)
    return 1;
  if (m2->gc.type==T_DB_EMPTY)
    return 0;
  if (m1->gc.type==T_DB_MAP)
    {
      if (m2->gc.type!=T_DB_MAP)
	return 0;
      if (++field >= nfields)
	{
	  field = 0;
	  mask <<= 1;
	}
      return db_included_large (nfields, m1->what.branch.left, m2->what.branch.left,
				field, mask) &&
	db_included_large (nfields, m1->what.branch.right, m2->what.branch.right,
			   field, mask);
    }
  else
    {
      return db_included_small_large (nfields, m1, m2, field, mask);
    }
}

int db_included (int nfields, db_map *m1, db_map *m2)
{
  return db_included_large (nfields, m1, m2, 0, 1L);
}

int db_included_lazy (db_map *m1, db_map *m2)
{
  int nfields;

  if (m1->gc.type==T_DB_EMPTY)
    return 1;
  nfields = db_nfields (m1);
  return db_included (nfields, m1, m2);
}



struct db_internal_join_spec {
  int nfields; /* equal to nfields1 + number of entries in vals[] that are equal
		  to -1 */
  int nfields1, nfields2;
  db_var_or_cst *vals; /* vals[nfields2], where vals[j]
			  is either !=DB_VAR_NONE, in which case we take only tuples where
			  field i in t1==field j in t2;
			  or DB_VAR_NONE, in which case, variable j is not compared to
			  any other variable
		       */
  db_var_or_cst *invvals; /* invvals[nfields1], inverse of vals[] */

};

static db_small_table *db_join_small_table (gc_t *gc_ctx,
					    db_small_table *t1, db_small_table *t2,
					    struct db_internal_join_spec *spec)
{
  db_small_table *tt1, *tt2;
  db_small_table *rest;
  db_small_table *last;
  db_small_table *newt;
  int i, j;
  int nfields, nfields1, nfields2;
  int ok;
  db_var_or_cst *vals;

  if (spec->nfields2==0)
    return t1;
  if (spec->nfields1==0)
    return t2;
  vals = spec->vals;
  nfields = spec->nfields;
  nfields1 = spec->nfields;
  nfields2 = spec->nfields2;
  rest = NULL;
  last = NULL;
  GC_START(gc_ctx,1);
  for (tt1=t1; tt1!=NULL; tt1=tt1->next)
    for (tt2=t2; tt2!=NULL; tt2=tt2->next)
      {
	ok = 1;
	for (j=0; j<nfields2; j++)
	  if (vals[j]==DB_VAR_NONE)
	    continue;
	  else if (OBJCMP(tt1->tuple[DB_VAR(vals[j])], tt2->tuple[j]) != 0)
	    {
	      ok = 0;
	      break;
	    }
	if (!ok)
	  break;
	newt = gc_alloc (gc_ctx, DBST_SIZE(nfields), &db_small_table_class);
	newt->gc.type = T_DB_SMALL_TABLE;
	newt->nfields = spec->nfields;
	GC_TOUCH (gc_ctx, newt->next = NULL);
	for (i=0; i<nfields1; i++)
	  GC_TOUCH (gc_ctx, newt->tuple[i] = tt1->tuple[i]);
	for (j=0; j<nfields2; j++)
	  if (vals[j]==DB_VAR_NONE)
	    {
	      GC_TOUCH (gc_ctx, newt->tuple[i] = tt2->tuple[j]);
	      i++;
	    }
	/* Then append at the end.  A miracle occurs: since t1 and
	   t2 are sorted lexicographically, traversing them the way
	   we do, the list rest will also be sorted lexicographically. */
	if (last==NULL)
	  {
	    rest = newt;
	    GC_UPDATE(gc_ctx,0,rest);
	  }
	else
	  {
	    GC_TOUCH(gc_ctx, last->next = newt);
	  }
	last = newt;
      }
  GC_END(gc_ctx);
  return rest;
}

static db_map *db_join_small_small (gc_t *gc_ctx,
				    db_map *m1, db_map *m2,
				    struct db_internal_join_spec *spec)
/* m1, m2 are just lists of tuples */
{
  db_small_table *rest;
  db_map *res;
  int i, j;
  db_var_or_cst *vals;
  int nfields1, nfields2;

  GC_START(gc_ctx,1);
  rest = db_join_small_table (gc_ctx, m1->what.tuples.table,
			      m2->what.tuples.table, spec);
  if (rest==NULL)
    res = empty_db;
  else
    {
      GC_UPDATE(gc_ctx,0,rest);
      res = gc_alloc (gc_ctx, DB_TUPLE_SIZE(spec->nfields), &db_tuples_class);
      res->gc.type = T_DB_SINGLETON;
      GC_TOUCH (gc_ctx, res->what.tuples.table = rest);
      nfields1 = spec->nfields1;
      for (i=0; i<nfields1; i++)
	res->what.tuples.hash[i] = m1->what.tuples.hash[i];
      vals = spec->vals;
      nfields2 = spec->nfields2;
      for (j=0; j<nfields2; j++)
	if (vals[j]==DB_VAR_NONE)
	  {
	    res->what.tuples.hash[i] = m2->what.tuples.hash[j];
	    i++;
	  }
    }
  GC_END(gc_ctx);
  return res;
}

static db_map *db_join_small_large (gc_t *gc_ctx,
				    db_map *m1, db_map *m2,
				    struct db_internal_join_spec *spec,
				    int field2, unsigned long mask)
/* m1 is a list of tuples */
{
  db_map *res, *left, *right;
  db_var_or_cst i;

  switch (m2->gc.type)
    {
    case T_DB_EMPTY:
      res = m2; /* empty */
      break;
    case T_DB_SINGLETON:
      res = db_join_small_small (gc_ctx, m1, m2, spec);
      break;
    case T_DB_MAP:
      i = spec->vals[field2];
      if (i==DB_VAR_NONE)
	{
	  if (++field2 >= spec->nfields2)
	    {
	      field2 = 0;
	      mask <<= 1;
	    }
	  GC_START(gc_ctx,2);
	  left = db_join_small_large (gc_ctx, m1,
				      m2->what.branch.left, spec, field2, mask);
	  GC_UPDATE(gc_ctx,0,left);
	  right = db_join_small_large (gc_ctx, m1,
				       m2->what.branch.right, spec, field2, mask);
	  GC_UPDATE(gc_ctx,1,right);
	  res = db_union (gc_ctx, spec->nfields, left, right);
	  GC_END(gc_ctx);
	}
      else /* here we know that field2 should be taken equal to i,
	      in particular for their values bitwise-anded with mask */
	{
	  if (m1->what.tuples.hash[DB_VAR(i)] & mask)
	    {
	      if (++field2 >= spec->nfields2)
		{
		  field2 = 0;
		  mask <<= 1;
		}
	      res = db_join_small_large (gc_ctx, m1,
					 m2->what.branch.right, spec,
					 field2, mask);
	    }
	  else
	    {
	      if (++field2 >= spec->nfields2)
		{
		  field2 = 0;
		  mask <<= 1;
		}
	      res = db_join_small_large (gc_ctx, m1,
					 m2->what.branch.left, spec,
					 field2, mask);
	    }
	}
      break;
    default:
      DebugLog(DF_OVM, DS_DEBUG, "Type error in db_join_small_large()\n");
      res = NULL;
      break;
    }
  return res;
}

static db_map *db_join_large_small (gc_t *gc_ctx,
				    db_map *m1, db_map *m2,
				    struct db_internal_join_spec *spec,
				    int field1, unsigned long mask)
/* m2 is a list of tuples */
{
  db_map *res, *left, *right;
  db_var_or_cst j;

  switch (m1->gc.type)
    {
    case T_DB_EMPTY:
      res = m1; /* empty */
      break;
    case T_DB_SINGLETON:
      res = db_join_small_small (gc_ctx, m1, m2, spec);
      break;
    case T_DB_MAP:
      j = spec->invvals[field1];
      if (j==DB_VAR_NONE)
	{
	  if (++field1 >= spec->nfields1)
	    {
	      field1 = 0;
	      mask <<= 1;
	    }
	  GC_START(gc_ctx,2);
	  left = db_join_large_small (gc_ctx, m1->what.branch.left,
				      m2, spec, field1, mask);
	  GC_UPDATE(gc_ctx,0,left);
	  right = db_join_large_small (gc_ctx, m1->what.branch.right,
				       m2, spec, field1, mask);
	  GC_UPDATE(gc_ctx,1,right);
	  res = db_union (gc_ctx, spec->nfields, left, right);
	  GC_END(gc_ctx);
	}
      else /* here we know that field1 should be taken equal to j,
	      in particular for their values bitwise-anded with mask */
	{
	  if (m2->what.tuples.hash[DB_VAR(j)] & mask)
	    {
	      if (++field1 >= spec->nfields1)
		{
		  field1 = 0;
		  mask <<= 1;
		}
	      res = db_join_large_small (gc_ctx, m1->what.branch.right,
					 m2, spec,
					 field1, mask);
	    }
	  else
	    {
	      if (++field1 >= spec->nfields1)
		{
		  field1 = 0;
		  mask <<= 1;
		}
	      res = db_join_large_small (gc_ctx, m1->what.branch.left,
					 m2, spec,
					 field1, mask);
	    }
	}
      break;
    default:
      DebugLog(DF_OVM, DS_DEBUG, "Type error in db_join_large_small()\n");
      res = NULL;
      break;
    }
  return res;
}

static db_map *db_join_large_1 (gc_t *gc_ctx,
				db_map *m1, db_map *m2,
				struct db_internal_join_spec *spec,
				char *dirs, /* 8*sizeof(unsigned long) arrays of
					       nfields1 booleans */
				int field1, unsigned long mask);

static db_map *db_join_large_1_from_2 (gc_t *gc_ctx,
				       db_map *m1, db_map *m2,
				       struct db_internal_join_spec *spec,
				       char *dirs, /* 8*sizeof(unsigned long) arrays of
						      nfields1 booleans */
				       unsigned long mask)
{
  switch (m2->gc.type)
    {
    case T_DB_EMPTY:
      return m2; /* empty */
    case T_DB_SINGLETON:
      return db_join_large_small (gc_ctx, m1, m2, spec, 0, mask << 1);
    case T_DB_MAP:
      return db_join_large_1 (gc_ctx, m1, m2, spec, dirs, 0, mask << 1);
    default:
      DebugLog(DF_OVM, DS_DEBUG, "Type error in db_join_large_1_from_2()\n");
      return NULL;
    }
}

static db_map *db_join_large_2 (gc_t *gc_ctx,
				db_map *m1, db_map *m2,
				struct db_internal_join_spec *spec,
				char *dirs, /* 8*sizeof(unsigned long) arrays of
					       nfields1 booleans */
				int field2, unsigned long mask)
{
  db_map *left, *right, *res;
  db_var_or_cst i;

  switch (m2->gc.type)
    {
    case T_DB_EMPTY:
      res = m2; /* empty */
      break;
    case T_DB_SINGLETON:
      res = db_join_large_small (gc_ctx, m1, m2, spec, 0, mask << 1);
      break;
    case T_DB_MAP:
      i = spec->vals[field2];
      if (i==DB_VAR_NONE) /* any value of this field is OK */
	{
	  GC_START(gc_ctx,2);
	  if (++field2 >= spec->nfields2)
	    {
	      dirs += spec->nfields1;
	      left = db_join_large_1_from_2 (gc_ctx, m1,
					     m2->what.branch.left, spec, dirs, mask);
	      GC_UPDATE(gc_ctx,0,left);
	      right = db_join_large_1_from_2 (gc_ctx, m1,
					      m2->what.branch.right, spec, dirs, mask);
	      GC_UPDATE(gc_ctx,1,right);
	    }
	  else
	    {
	      left = db_join_large_2 (gc_ctx, m1,
				      m2->what.branch.left, spec, dirs, field2, mask);
	      GC_UPDATE(gc_ctx,0,left);
	      right = db_join_large_2 (gc_ctx, m1,
				       m2->what.branch.right, spec, dirs, field2, mask);
	      GC_UPDATE(gc_ctx,1,right);
	    }
	  res = db_union (gc_ctx, spec->nfields, left, right);
	  GC_END(gc_ctx);
	}
      else if (dirs[DB_VAR(i)]) /* select right son */
	{
	  if (++field2 >= spec->nfields2)
	    {
	      dirs += spec->nfields1;
	      res = db_join_large_1_from_2 (gc_ctx, m1,
					    m2->what.branch.right, spec, dirs, mask);
	    }
	  else
	    res = db_join_large_2 (gc_ctx, m1,
				   m2->what.branch.right, spec, dirs, field2, mask);
	}
      else /* select left son */
	{
	  if (++field2 >= spec->nfields2)
	    {
	      dirs += spec->nfields1;
	      res = db_join_large_1_from_2 (gc_ctx, m1,
					    m2->what.branch.left, spec, dirs, mask);
	    }
	  else
	    res = db_join_large_2 (gc_ctx, m1,
				   m2->what.branch.left, spec, dirs, field2, mask);
	}
      break;
    default:
      DebugLog(DF_OVM, DS_DEBUG, "Type error in db_join_large_2()\n");
      res = NULL;
      break;
    }
  return res;
}

static db_map *db_join_large_2_from_1 (gc_t *gc_ctx,
				       db_map *m1, db_map *m2,
				       struct db_internal_join_spec *spec,
				       char *dirs, /* 8*sizeof(unsigned long) arrays of
						      nfields1 booleans */
				       unsigned long mask)
{
  switch (m1->gc.type)
    {
    case T_DB_EMPTY:
      return m1; /* empty */
    case T_DB_SINGLETON:
      return db_join_small_large (gc_ctx, m1, m2, spec, 0, mask);
    case T_DB_MAP:
      return db_join_large_2 (gc_ctx, m1, m2, spec, dirs, 0, mask);
    default:
      DebugLog(DF_OVM, DS_DEBUG, "Type error in db_join_large_2_from_1()\n");
      return NULL;
    }
}

static db_map *db_join_large_1 (gc_t *gc_ctx,
				db_map *m1, db_map *m2,
				struct db_internal_join_spec *spec,
				char *dirs, /* 8*sizeof(unsigned long) arrays of
					       nfields1 booleans */
				int field1, unsigned long mask)
{
  db_map *left, *right, *res;

  switch (m1->gc.type)
    {
    case T_DB_EMPTY:
      res = m1; /* empty */
      break;
    case T_DB_SINGLETON:
      res = db_join_small_large (gc_ctx, m1, m2, spec, 0, mask);
      break;
    case T_DB_MAP:
      GC_START(gc_ctx,2);
      if (++field1 >= spec->nfields1)
	{
	  /*field1 = 0;*/
	  dirs[field1-1] = 0;
	  left = db_join_large_2_from_1 (gc_ctx, m1->what.branch.left,
					 m2, spec, dirs, mask);
	  GC_UPDATE(gc_ctx,0,left);
	  dirs[field1-1] = 1;
	  right = db_join_large_2_from_1 (gc_ctx, m1->what.branch.right,
					  m2, spec, dirs, mask);
	  GC_UPDATE(gc_ctx,1,right);
	}
      else
	{
	  dirs[field1-1] = 0;
	  left = db_join_large_1 (gc_ctx, m1->what.branch.left,
				  m2, spec, dirs, field1, mask);
	  GC_UPDATE(gc_ctx,0,left);
	  dirs[field1-1] = 1;
	  right = db_join_large_1 (gc_ctx, m1->what.branch.right,
				   m2, spec, dirs, field1, mask);
	  GC_UPDATE(gc_ctx,1,right);
	}
      res = db_union (gc_ctx, spec->nfields, left, right);
      GC_END(gc_ctx);
      break;
    default:
      DebugLog(DF_OVM, DS_DEBUG, "Type error in db_join_large_1()\n");
      res = NULL;
      break;
    }
  return res;
}

db_map *db_join (gc_t *gc_ctx, struct db_join_spec *spec,
		 db_map *m1, db_map *m2)
{
  struct db_internal_join_spec ispec;
  char *dirs;
  db_map *res;
  int i;
  int nfields1, nfields2;

  nfields1 = spec->nfields1;
  ispec.invvals = gc_base_malloc (gc_ctx, nfields1 * sizeof(db_var_or_cst));
  for (i=0; i<nfields1; i++)
    ispec.invvals[i] = DB_VAR_NONE;
  ispec.nfields = ispec.nfields1 = nfields1;
  nfields2 = spec->nfields2;
  for (i=0; i<nfields2; i++)
    if (spec->vals[i]==DB_VAR_NONE)
      ispec.nfields++;
    else ispec.invvals[spec->vals[i]] = i;
  ispec.nfields2 = nfields2;
  ispec.vals = spec->vals;
  dirs = gc_base_malloc (gc_ctx, (8*sizeof(unsigned long)) * nfields1);
  res = db_join_large_1 (gc_ctx, m1, m2, &ispec, dirs, 0, 1L);
  gc_base_free (dirs);
  gc_base_free (ispec.invvals);
  return res;
}

int db_sweep (gc_t *gc_ctx, int nfields, db_map *m,
	      int (*do_sweep) (gc_t *gc_ctx, int nfields, db_map *singleton, void *data),
	      void *data)
{
  int res;

  switch (m->gc.type)
    {
    case T_DB_EMPTY:
      return 0;
    case T_DB_MAP:
      res = db_sweep (gc_ctx, nfields, m->what.branch.left, do_sweep, data);
      if (res)
	return res;
      return db_sweep (gc_ctx, nfields, m->what.branch.right, do_sweep, data);
    case T_DB_SINGLETON:
      return (*do_sweep) (gc_ctx, nfields, m, data);
    default:
      DebugLog(DF_OVM, DS_DEBUG, "Type error in db_sweep()\n");
      return 0;
    }
}

#ifdef OBSOLETE
int print_sweeper (gc_t *gc_ctx, int nfields, db_map *m, void *data)
{
  int i;
  db_small_table *t;
  FILE *f = data;

  t = m->what.tuples.table;
  for (i=0; i<nfields; i++)
    {
      fprintf (f, "\t%s", t->tuple[i]);
    }
  fputc ('\n', f);
  return 0;
}
#endif

db_map *db_collect (gc_t *gc_ctx, int nfields_res, int nfields, db_map *m,
		    db_map *(*do_collect) (gc_t *gc_ctx, int nfields_res, int nfields,
					   db_map *singleton, void *data),
		    void *data)
{
  db_map *res, *left, *right;

  switch (m->gc.type)
    {
    case T_DB_EMPTY:
      res = m; /* empty */
      break;
    case T_DB_MAP:
      GC_START(gc_ctx,2);
      left = db_collect (gc_ctx, nfields_res, nfields, m->what.branch.left,
			 do_collect, data);
      if (left!=NULL)
	{
	  GC_UPDATE(gc_ctx,0,left);
	  right = db_collect (gc_ctx, nfields_res, nfields, m->what.branch.right,
			      do_collect, data);
	  if (right!=NULL)
	    {
	      GC_UPDATE(gc_ctx,1,right);
	      res = db_union (gc_ctx, nfields_res, left, right);
	    }
	  else res = left; /* and break there */
	}
      else res = NULL;
      GC_END(gc_ctx);
      break;
    case T_DB_SINGLETON:
      res = (*do_collect) (gc_ctx, nfields_res, nfields, m, data);
      break;
    default:
      DebugLog(DF_OVM, DS_DEBUG, "Type error in db_collect()\n");
      res = NULL;
      break;
    }
  return res;
}

db_map *db_collect_lazy (gc_t *gc_ctx, int nfields_res, db_map *m,
			 db_map *(*do_collect) (gc_t *gc_ctx,
						int nfields_res,
						int nfields,
						db_map *singleton,
						void *data),
			 void *data)
{
  int nfields;

  if (m->gc.type==T_DB_EMPTY)
    return m;
  nfields = db_nfields (m);
  return db_collect (gc_ctx, nfields_res, nfields, m,
		     do_collect, data);
}


#ifdef OBSOLETE
struct db_fold_spec {
  void *(*do_zero) (gc_t *gc_ctx, void *data);
  void *(*do_one) (gc_t *gc_ctx, int nfields, db_map *singleton, void *data);
  void *(*do_two) (gc_t *gc_ctx, void *obj1, void *obj2, void *data);
  void *data;
};

void *db_fold (gc_t *gc_ctx, int nfields, db_map *m,
	       struct db_fold_spec *spec)
{
  switch (m->gc.type)
    {
    case T_DB_EMPTY:
      return (*spec->do_zero) (gc_ctx, spec->data);
    case T_DB_MAP:
      return (*spec->do_two) (gc_ctx,
			      db_fold (gc_ctx, nfields, m->what.branch.left, spec),
			      db_fold (gc_ctx, nfields, m->what.branch.right, spec),
			      spec->data);
    case T_DB_SINGLETON:
      return (*spec->do_one) (gc_ctx, nfields, m, spec->data);
    default:
      DebugLog(DF_OVM, DS_DEBUG, "Type error in db_fold()\n");
      return NULL;
    }
}
#endif

void *db_transitive_closure (gc_t *gc_ctx, int nfields, int nio,
			     unsigned long *inp_fields, unsigned long *out_fields,
			     db_map *m)
{
  char *outs;
  int i, j, nfields_join;
  db_map *join, *proj, *res_at_most_n, *res_exactly_n, *diff;
  struct db_join_spec jspec;
  struct db_proj_spec pspec;

  jspec.vals = gc_base_malloc (gc_ctx, nfields*sizeof(db_var_or_cst));
  for (i=0; i<nfields; i++)
    jspec.vals[i] = DB_MAKE_VAR(i);
  for (i=0; i<nio; i++)
    {
      jspec.vals[inp_fields[i]] = DB_MAKE_VAR(out_fields[i]);
      jspec.vals[out_fields[i]] = DB_VAR_NONE;
    }
  jspec.nfields1 = jspec.nfields2 = nfields;
  /* nfields fields, among with nio inputs, nio outputs;
     the join will contain (nfields-nio-nio) non-input-output fields,
     plus nio input fields,
     plus nio output fields,
     plus nio common, intermediate fields */
  nfields_join = nfields + nio;
  pspec.head = gc_base_malloc (gc_ctx, nfields*sizeof(db_var_or_cst));
  /* project out the nio intermediate fields; they are at positions
     given by out_fields in the left argument to the join */
  outs = gc_base_malloc (gc_ctx, nfields_join);
  for (i=0; i<nfields_join; i++)
    outs[i] = 0;
  for (i=0; i<nio; i++)
    outs[out_fields[i]] = 1;
  for (i=0, j=0; i<nfields_join; i++)
    {
      if (outs[i]==0)
	{
	  pspec.head[j] = DB_MAKE_VAR(i);
	  j++;
	}
    }
  gc_base_free (outs);
  pspec.constants = NULL;
  pspec.hash = NULL;
  res_at_most_n = m;
  res_exactly_n = m;
  GC_START(gc_ctx,3);
  while (1)
    {
      /* 0: res_at_most_n (or NULL, initially)
	 1: res_exactly_n (or NULL, initially)
      */
      join = db_join (gc_ctx, &jspec, m, res_exactly_n);
      GC_UPDATE(gc_ctx,1,join); /* overwrites res_exactly_n at slot 1 */
      proj = db_proj (gc_ctx, &pspec, join); /* res_exactly_{n+1} */
      GC_UPDATE(gc_ctx,1,proj); /* will be stored into res_exactly_n below;
				   overwrites join, which we do not need any longer anyway */
      diff = db_diff (gc_ctx, nfields, proj, res_at_most_n);
      GC_UPDATE(gc_ctx,2,diff);
      if (diff->gc.type==T_DB_EMPTY) /* fixpoint reached! */
	break;
      res_at_most_n = db_union (gc_ctx, nfields, res_at_most_n, diff);
      GC_UPDATE(gc_ctx,0,res_at_most_n);
      GC_UPDATE(gc_ctx,2,NULL); /* save space used by diff */
      res_exactly_n = proj;
    }
  GC_END(gc_ctx);
  gc_base_free (pspec.head);
  gc_base_free (jspec.vals);
  return res_at_most_n;
}

db_map *db_empty (gc_t *gc_ctx)
{
  return empty_db;
}

db_map *db_singleton (gc_t *gc_ctx, ovm_var_t **tuple, int nfields)
{
  db_small_table *t;
  db_map *m;
  int i;

  GC_START(gc_ctx, 1);
  t = gc_alloc (gc_ctx, DBST_SIZE(nfields), &db_small_table_class);
  t->gc.type = T_DB_SMALL_TABLE;
  t->nfields = nfields;
  t->next = NULL;
  for (i=0; i<nfields; i++)
    GC_TOUCH (gc_ctx, t->tuple[i] = tuple[i]);
  GC_UPDATE(gc_ctx,0,t);
  m = gc_alloc (gc_ctx, DB_TUPLE_SIZE(nfields), &db_tuples_class);
  m->gc.type = T_DB_SINGLETON;
  GC_TOUCH (gc_ctx, m->what.tuples.table = t);
  for (i=0; i<nfields; i++)
    m->what.tuples.hash[i] = issdl_hash (tuple[i]);
  GC_END(gc_ctx);
  return m;
}

static int empty_db_save (save_ctx_t *sctx, gc_header_t *p)
{
  return 0;
}

gc_class_t empty_db_class;

static gc_header_t *empty_db_restore (restore_ctx_t *rctx)
{
  return (gc_header_t *)empty_db;
}

gc_class_t empty_db_class = {
  GC_ID('d','b','0','0'),
  NULL,
  NULL,
  NULL,
  empty_db_save,
  empty_db_restore
};

void db_init (gc_t *gc_ctx)
{
  GC_TOUCH(gc_ctx, empty_db = gc_alloc(gc_ctx, sizeof(gc_header_t), &empty_db_class));
  empty_db->gc.type = T_DB_EMPTY;
  gc_add_root(gc_ctx, (gc_header_t **)&empty_db);
}

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
