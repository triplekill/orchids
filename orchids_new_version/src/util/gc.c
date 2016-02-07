/**
 ** @file gc.c
 ** Garbage collection
 **
 ** @author Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup engine
 **
 ** @date  Started on: Jeu 26 jui 2014 15:22:49 CEST
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include "gc.h"

#include "safelib.h"

/* Magic numbers for all gc_alloc()ed objects */
#define MAGIC0 0x9a
#define MAGIC1 0xdd
/* Magic numbers for objects that once were gc_alloc()ed but are now free */
#define MAGIC_FREE0 0xfb
#define MAGIC_FREE1 0x33

gc_t *gc_init (void)
{
  gc_t *gc_ctx;

  gc_ctx = Xmalloc(sizeof(gc_t));
  gc_ctx->gc_mark_mod = 768; /* by default, 3 objects marked per
			     call to gc_mark () */
  gc_ctx->gc_mark_cnt = 0;
  gc_ctx->gc_sweep_mod = 640; /* by default, 2.5 objects swept per
			      call to gc_sweep () */
  gc_ctx->gc_sweep_cnt = 0;
  gc_ctx->roots = NULL;
  gc_ctx->stack_data = NULL;
  gc_ctx->grey = NULL;
  gc_ctx->alloced = NULL;
  gc_ctx->sweep_current = &gc_ctx->alloced;
  gc_ctx->generation = 0;
  gc_ctx->mark_state = GC_MARK_STATE_INIT;
  return gc_ctx;
}

void gc_add_root (gc_t *gc_ctx, gc_header_t **root)
{
  gc_rootzone *rz;

  GC_TOUCH(gc_ctx, *root);
  rz = Xmalloc (sizeof(gc_rootzone));
  rz->next = gc_ctx->roots;
  rz->root = root;
  gc_ctx->roots = rz;
}

#define GC_BLACK_OLDp(gc_ctx,p) (((gc_ctx)->generation & 1)?((p)->flags & GC_FLAGS_ODD_COLOR_MASK):((p)->flags & GC_FLAGS_EVEN_COLOR_MASK))
#define GC_SET_BLACK_OLD(gc_ctx,p) (p)->flags |= (((gc_ctx)->generation & 1)?GC_FLAGS_ODD_COLOR_MASK:GC_FLAGS_EVEN_COLOR_MASK)
#define GC_CLR_BLACK_OLD(gc_ctx,p) (p)->flags &= (((gc_ctx)->generation & 1)?(~GC_FLAGS_ODD_COLOR_MASK):(~GC_FLAGS_EVEN_COLOR_MASK))

#define GC_BLACK_NEWp(gc_ctx,p) (((gc_ctx)->generation & 1)?((p)->flags & GC_FLAGS_EVEN_COLOR_MASK):((p)->flags & GC_FLAGS_ODD_COLOR_MASK))
#define GC_SET_BLACK_NEW(gc_ctx,p) (p)->flags |= (((gc_ctx)->generation & 1)?GC_FLAGS_EVEN_COLOR_MASK:GC_FLAGS_ODD_COLOR_MASK)
#define GC_CLR_BLACK_NEW(gc_ctx,p) (p)->flags &= (((gc_ctx)->generation & 1)?(~GC_FLAGS_EVEN_COLOR_MASK):(~GC_FLAGS_ODD_COLOR_MASK))

#define GC_GREY_OR_BLACK_NEWp(gc_ctx,p) (((gc_ctx)->generation & 1)?((p)->flags & (GC_FLAGS_GREY_MASK | GC_FLAGS_EVEN_COLOR_MASK)):((p)->flags & (GC_FLAGS_GREY_MASK | GC_FLAGS_ODD_COLOR_MASK)))



void gc_touch (gc_t *gc_ctx, gc_header_t *p)
{
  if (p==NULL)
    return;
  if (GC_GREY_OR_BLACK_NEWp(gc_ctx,p))
    return;
  if (p->class->mark_subfields!=NULL)
    {
      p->flags |= GC_FLAGS_GREY; /* set grey flag, */
      p->nextgrey = gc_ctx->grey; /* and insert into grey list */
      gc_ctx->grey = p;
    }
  else /* otherwise, if p cannot contain any pointer to
	  any garbage-collected data, we make p black directly,
	  avoiding some overhead... unless p is grey (which should
	  in fact not happen). */
    {
      GC_SET_BLACK_NEW(gc_ctx,p);
    }
}

static void gc_advance (gc_t *gc_ctx)
{
  gc_ctx->generation++;
  gc_ctx->sweep_current = &gc_ctx->alloced;
  gc_ctx->mark_state = GC_MARK_STATE_INIT;
  gc_ctx->gc_mark_cnt = 0;
  gc_ctx->gc_sweep_cnt = 0;
}

int gc_mark (gc_t *gc_ctx)
{
  int nmarked = 0;

  while (1)
    {
      switch (gc_ctx->mark_state)
	{
	case GC_MARK_STATE_INIT:
	  /* mark all roots */
	  {
	    gc_rootzone *root;
	    gc_stack_data *sdata;
	    int i, n;

	    for (root = gc_ctx->roots; root!=NULL; root = root->next)
	      gc_touch (gc_ctx, *root->root);
	    for (sdata = gc_ctx->stack_data; sdata!=NULL;
		 sdata = sdata->next)
	      {
		n = sdata->n;
		for (i=0; i<n; i++)
		  gc_touch (gc_ctx, (gc_header_t *)sdata->data[i]);
	      }
	    gc_ctx->mark_state = GC_MARK_STATE_RUNNING;
	    break;
	  }
	case GC_MARK_STATE_RUNNING:
	  if (gc_ctx->grey==NULL) /* no grey object to mark: end */
	    {
	      gc_ctx->gc_mark_cnt = 0;
	      gc_ctx->mark_state = GC_MARK_STATE_END;
	      return nmarked;
	    }
	  else
	    {
	      gc_header_t *p;

	      p = gc_ctx->grey;
	      gc_ctx->grey = p->nextgrey;
	      GC_SET_BLACK_NEW(gc_ctx,p);
	      p->flags &= ~GC_FLAGS_GREY_MASK;
	      if (p->class->mark_subfields!=NULL)
		/* This test is in principle useless:
		   only objects p with non-null class
		   can ever be put into the grey list,
		   if you obey the API. */
		(*p->class->mark_subfields) (gc_ctx, p);
	      nmarked++;
	    }
	  break;
	default: /* case GC_MARK_STATE_END: */
	  if (gc_ctx->grey==NULL)
	    return nmarked; /* really done */
	  gc_ctx->mark_state = GC_MARK_STATE_RUNNING;
	  break;
	}
      gc_ctx->gc_mark_cnt += GC_INC_UNIT;
      if (gc_ctx->gc_mark_cnt >= gc_ctx->gc_mark_mod)
	{
	  gc_ctx->gc_mark_cnt -= gc_ctx->gc_mark_mod;
	  break;
	}
    }
  return nmarked;
}

int gc_sweep (gc_t *gc_ctx)
{
  gc_header_t **pp, *p;
  int nswept = 0;

  pp = gc_ctx->sweep_current;
  while (1)
    {
      p = *pp;
      if (p==NULL)
	{
	  gc_ctx->gc_sweep_cnt = 0;
	  break;
	}
      if (!GC_BLACK_OLDp(gc_ctx,p))
	/* not black in old generation, hence
	   white (there is no grey color for
	   old generation): free the object */
	{
	  *pp = p->next;
	  if (p->class->finalize!=NULL)
	    (*p->class->finalize) (gc_ctx, p);
	  p->magic[0] = MAGIC_FREE0;
	  p->magic[1] = MAGIC_FREE1;
	  free (p);
	}
      else
	{ /* Object was black in previous generation.
	     Make it white at previous generation, so that
	     when we gc_advance(), it becomes white in what
	     will be the current generation then. */
	  GC_CLR_BLACK_OLD(gc_ctx,p);
	  pp = &p->next;
	}
      nswept++;
      gc_ctx->gc_sweep_cnt += GC_INC_UNIT;
      if (gc_ctx->gc_sweep_cnt >= gc_ctx->gc_sweep_mod)
	{
	  gc_ctx->gc_sweep_cnt -= gc_ctx->gc_sweep_mod;
	  break;
	}
    }
  gc_ctx->sweep_current = pp;
  return nswept;
}

void gc (gc_t *gc_ctx)
{
  (void) gc_mark (gc_ctx);
  (void) gc_sweep (gc_ctx);
  if (gc_ctx->mark_state==GC_MARK_STATE_END && *gc_ctx->sweep_current==NULL)
    gc_advance (gc_ctx);
}

void gc_full (gc_t *gc_ctx)
{
  int save_gc_mark_mod = gc_ctx->gc_mark_mod;
  int save_gc_sweep_mod = gc_ctx->gc_sweep_mod;

  gc_ctx->gc_mark_mod = INT_MAX;
  gc_ctx->gc_sweep_mod = INT_MAX;

  /* Finish the current mark phase: */
  do {
    (void) gc_mark (gc_ctx);
  } while (gc_ctx->mark_state!=GC_MARK_STATE_END);
  /* Sweep and reclaim some memory.  The main purpose here
     is to be able to gc_advance().
  */
  do {
    (void) gc_sweep (gc_ctx);
  } while (*gc_ctx->sweep_current!=NULL);
  gc_advance (gc_ctx);
  /* Now do a second sweep and really reclaim some memory.
   */
  do {
    (void) gc_sweep (gc_ctx);
  } while (*gc_ctx->sweep_current!=NULL);
  /* We cannot gc_advance(), as there might be plenty of
     data to gc_mark() left. */

  gc_ctx->gc_mark_mod = save_gc_mark_mod;
  gc_ctx->gc_sweep_mod = save_gc_sweep_mod;
}

static char gc_base_malloc_0;

void *gc_base_malloc (gc_t *gc_ctx, size_t n)
{
  void *p;

  gc (gc_ctx);
  if (n==0)
    return (void *)&gc_base_malloc_0;
  p = malloc(n);
  if (p==NULL)
    {
      gc_full (gc_ctx);
      p = malloc(n);
      if (p==NULL)
	{
	  gc_full (gc_ctx);
	  p = Xmalloc (n);
	}
    }
  return p;
}

void *gc_base_realloc (gc_t *gc_ctx, void *p0, size_t n)
{
  void *p;

  if (p0==NULL)
    return gc_base_malloc (gc_ctx, n);
  gc (gc_ctx);
  if (n==0)
    {
      if (p0!=(void *)&gc_base_malloc_0)
	free (p0);
      return (void *)&gc_base_malloc_0;
    }
  if (p0==(void *)&gc_base_malloc_0)
    return gc_base_malloc (gc_ctx, n);
  p = realloc(p0, n);
  if (p==NULL)
    {
      gc_full (gc_ctx);
      p = realloc(p0, n);
      if (p==NULL)
	{
	  gc_full (gc_ctx);
	  p = Xrealloc (p0, n);
	}
    }
  return p;
}

char *gc_strdup(gc_t *gc_ctx, char *s)
{
  if (s==NULL)
    return NULL;
  {
    char *copy;
    size_t len = strlen(s)+1;

    copy = gc_base_malloc (gc_ctx, len);
    memcpy (copy, s, len);
    return copy;
  }
}

void gc_base_free (void *p)
{
  if (p==(void *)&gc_base_malloc_0)
    return;
  Xfree (p);
}

void *gc_alloc (gc_t *gc_ctx, size_t n, gc_class_t *class)
{
  gc_header_t *p;

  p = gc_base_malloc (gc_ctx, n);
  p->class = class;
  /* Install object at beginning of gc_ctx->alloced list
     We do as though we had already gc_sweep()ed through it,
     so p should be white in the old generation.
     It should be white in the new generation, too: it will
     be the responsibility of the caller to touch it if necessary.
   */
  p->flags = GC_FLAGS_ODD_WHITE | GC_FLAGS_EVEN_WHITE;
  p->magic[0] = MAGIC0;
  p->magic[1] = MAGIC1;
  /* Since object has (virtually) been gc_sweep()ed already,
     we must move the sweep_current pointer one step beyond
     in case it is at beginning of alloced list. */
  if (gc_ctx->sweep_current == &gc_ctx->alloced)
    gc_ctx->sweep_current = &p->next;
  p->next = gc_ctx->alloced;
  gc_ctx->alloced = p;
  p->nextgrey = NULL; /* not grey */
  return p;
}

/* gc_check(): an fsck-style memory consistency verifier */

#define GC_CHECK_MASK(gc_ctx) (((gc_ctx)->generation & 1)?(GC_FLAGS_GREY_MASK | GC_FLAGS_EVEN_COLOR_MASK):(GC_FLAGS_GREY_MASK | GC_FLAGS_ODD_COLOR_MASK))

static void gc_check_magic(gc_header_t *p)
{
  if (p->magic[0]!=MAGIC0 || p->magic[1]!=MAGIC1)
    {
      fprintf (stderr, "gc_check: garbled memory, fake object or magic[] array clobbered.\n");
      if (p->magic[0]==MAGIC_FREE0 || p->magic[1]==MAGIC_FREE1)
	fprintf (stderr, "          magic numbers indicate object has probably recently been freed (not protected against gc?).\n");
      abort ();
    }
}

static int gc_check_grey (gc_traverse_ctx_t *gtc, gc_header_t *p,
			  void *data)
/* Assume we traverse the memory graph.
   We want to make sure that it is not the case that
   we come from a black node in the NEW generation (parent_new_color),
   and p is white in the NEW generation.
   Then recurse.
*/
{
  int parent_new_color;
  gc_t *gc_ctx = gtc->gc_ctx;

  parent_new_color = *(int *)data;
  if (p==NULL) /* no descendant */
    return 0;
  gc_check_magic (p);
  if (parent_new_color & GC_FLAGS_GREY_MASK) /* parent is grey: no problem */
    ;
  else if (parent_new_color && !GC_GREY_OR_BLACK_NEWp(gc_ctx,p))
    { /* parent_new_color is non-zero, and not grey, so it is black;
	 and p is white: we have found a memory inconsistency. */
      fprintf (stderr, "gc_check: black object pointing to white, directly.\n");
      abort (); /* so as to fall into debugger */
    }
  if (p->class->traverse==NULL) /* no subfield: stop */
    return 0;
  if (p->flags & GC_FLAGS_CHECKED) /* already marked p's subfields */
    return 0;
  p->flags |= GC_FLAGS_CHECKED;
  parent_new_color = p->flags & GC_CHECK_MASK(gc_ctx);
  return (*p->class->traverse) (gtc, p, (void *)&parent_new_color);
}

void gc_check (gc_t *gc_ctx)
{
  gc_rootzone *root;
  gc_header_t *p;
  gc_traverse_ctx_t gtc;
  gc_stack_data *sd;
  int color;
  int current_seen;

  color = 0;
  gtc.gc_ctx = gc_ctx;
  gtc.do_subfield = gc_check_grey;
  gtc.traverse_action = TRAVERSE_GC_CHECK;
  for (root = gc_ctx->roots; root!=NULL; root = root->next)
    {
      /* Check that:
	 - In the NEW generation, no black object points directly
	 to a white object (it must go through a grey object first)
	 In the meantime, actually simulate a mark phase of a
	 classical mark-and-sweep garbage collector, using
	 the GC_FLAGS_CHECKED bit to mark.
       */
      (void) gc_check_grey (&gtc, *root->root, &color);
      /* 0 is for initial parent color white */
    }
  for (sd = gc_ctx->stack_data; sd!=NULL; sd = sd->next)
    {
      int i, n;

      for (n=sd->n, i=0; i<n; i++)
        (void) gc_check_grey (&gtc, (gc_header_t *)sd->data[i], &color);
    }
  /* Now check that:
     - every object in the grey list is indeed grey.
     We shall check the converse implication below, using
     the GC_FLAGS_IN_GREY_LIST flag.
   */
  for (p = gc_ctx->grey; p!=NULL; p = p->nextgrey)
    {
      if (!p->flags & GC_FLAGS_GREY_MASK)
	{
	  fprintf (stderr, "gc_check: object in grey list, without the grey bit.\n");
	  abort (); /* so as to fall into debugger */
	}
      p->flags |= GC_FLAGS_IN_GREY_LIST;
    }
  current_seen = (gc_ctx->sweep_current == &gc_ctx->alloced);
  /* current_seen will be false while p below is to the left
     of gc_ctx->sweep_current [exclusive],
     and true if to the right [inclusive].
  */
  for (p = gc_ctx->alloced; p!=NULL; p = p->next)
    {
      if (p->flags & GC_FLAGS_GREY_MASK)
	{ /* Check that:
	     - every grey object is in the grey list.
	     We have checked the converse implication above.
	  */
	  if (!(p->flags & GC_FLAGS_IN_GREY_LIST))
	    {
	      fprintf (stderr, "gc_check: object with the grey bit set, yet not in the grey list.\n");
	      abort (); /* so as to fall into debugger */
	    }
	  p->flags &= ~GC_FLAGS_IN_GREY_LIST;
	}
      if (current_seen)
	{
	  if (p->flags & GC_FLAGS_CHECKED)
	    {
	      /* p has the GC_FLAGS_CHECKED bit, and is therefore
		 reachable.
		 Check that:
		 - it is black in the OLD generation.  Recall that
		 we have not yet swept through it (i.e., current_seen
		 is true).
	      */
	      if (!GC_BLACK_OLDp(gc_ctx,p))
		{
		  fprintf (stderr, "gc_check: reachable object that is white in the old generation and to the right of current_sweep, hence will be freed.\n");
		  abort (); /* so as to fall into debugger */
		}
	    }
	}
      else
	{
	  if (GC_BLACK_OLDp(gc_ctx,p))
	    {
	      fprintf (stderr, "gc_check: object to the left of current_sweep but still black in the old generation (gc bug).\n");
	      abort (); /* so as to fall into debugger */
	    }
	}
      p->flags &= ~GC_FLAGS_CHECKED;
      if (gc_ctx->sweep_current == &p->next)
	current_seen |= 1;
    }
}

static int gc_estimate_sharing (gc_traverse_ctx_t *gtc, gc_header_t *p,
				void *data)
{
  if (p==NULL)
    return 0;
  if (p->flags & GC_FLAGS_EST_SEEN)
    {
      p->flags |= GC_FLAGS_EST_SHARED;
      return 0;
    }
  p->flags |= GC_FLAGS_EST_SEEN;
  if (p->class->traverse==NULL)
    return 0;
  return (*p->class->traverse) (gtc, p, NULL);
}

void estimate_sharing (gc_t *gc_ctx, gc_header_t *p)
{
  gc_traverse_ctx_t gtc;
  
  gtc.gc_ctx = gc_ctx;
  gtc.do_subfield = gc_estimate_sharing;
  gtc.traverse_action = TRAVERSE_MARSHALL;
  (void) gc_estimate_sharing (&gtc, p, NULL);
}

static int gc_reset_sharing (gc_traverse_ctx_t *gtc, gc_header_t *p,
				      void *data)
{
  if (p==NULL)
    return 0;
  if ((p->flags & (GC_FLAGS_EST_SEEN | GC_FLAGS_EST_SHARED))==0)
    return 0;
  p->flags &= ~(GC_FLAGS_EST_SEEN | GC_FLAGS_EST_SHARED);
  if (p->class->traverse==NULL)
    return 0;
  return (*p->class->traverse) (gtc, p, NULL);
}

void reset_sharing (gc_t *gc_ctx, gc_header_t *p)
{
  gc_traverse_ctx_t gtc;
  
  gtc.gc_ctx = gc_ctx;
  gtc.do_subfield = gc_reset_sharing;
  gtc.traverse_action = TRAVERSE_MARSHALL;
  (void) gc_reset_sharing (&gtc, p, NULL);
}

int save_size_t (save_ctx_t *sctx, size_t sz)
{
  int i, c;
  FILE *f = sctx->f;

  for (i=0; i<sizeof(size_t); i++)
    {
      c = sz & 0xff;
      if (putc_unlocked (c, f) < 0) return errno;
      sz >>= 8;
    }
  return 0;
}

int restore_size_t (restore_ctx_t *rctx, size_t *szp)
{
  FILE *f = rctx->f;
  int i, c;
  size_t sz;

  for (i=0, sz=0; i<sizeof(size_t); i++)
    {
      c = getc_unlocked (f);
      if (c==EOF)
	return c;
      sz >>= 8;
      sz |= (((size_t)c) << 8*(sizeof(size_t)-1));
    }
  *szp = sz;
  return 0;
}

int save_int (save_ctx_t *sctx, int sz)
{
  int i, c;
  FILE *f = sctx->f;

  for (i=0; i<sizeof(int); i++)
    {
      c = sz & 0xff;
      if (putc_unlocked (c, f) < 0) return errno;
      sz >>= 8;
    }
  return 0;
}

int restore_int (restore_ctx_t *rctx, int *szp)
{
  FILE *f = rctx->f;
  int i, c;
  unsigned int sz; /* must be unsigned, see comment in restore_int32() */

  for (i=0, sz=0; i<sizeof(int); i++)
    {
      c = getc_unlocked (f);
      if (c==EOF)
	return c;
      sz >>= 8;
      sz |= (((unsigned int)c) << 8*(sizeof(int)-1));
    }
  *szp = (int)sz;
  return 0;
}

int save_uint (save_ctx_t *sctx, unsigned int sz)
{
  int i, c;
  FILE *f = sctx->f;

  for (i=0; i<sizeof(unsigned int); i++)
    {
      c = sz & 0xff;
      if (putc_unlocked (c, f) < 0) return errno;
      sz >>= 8;
    }
  return 0;
}

int restore_uint (restore_ctx_t *rctx, unsigned int *szp)
{
  FILE *f = rctx->f;
  int i, c;
  unsigned int sz;

  for (i=0, sz=0; i<sizeof(unsigned int); i++)
    {
      c = getc_unlocked (f);
      if (c==EOF)
	return c;
      sz >>= 8;
      sz |= (((unsigned int)c) << 8*(sizeof(unsigned int)-1));
    }
  *szp = sz;
  return 0;
}

int save_int32 (save_ctx_t *sctx, int32_t sz)
{
  int i, c;
  FILE *f = sctx->f;

  for (i=0; i<sizeof(int32_t); i++)
    {
      c = sz & 0xff;
      if (putc_unlocked (c, f) < 0) return errno;
      sz >>= 8;
    }
  return 0;
}

int restore_int32 (restore_ctx_t *rctx, int32_t *szp)
{
  FILE *f = rctx->f;
  int i, c;
  uint32_t sz; /* must be unsigned; otherwise +132 would get restored as -124 */

  for (i=0, sz=0; i<sizeof(int32_t); i++)
    {
      c = getc_unlocked (f);
      if (c==EOF)
	return c;
      sz >>= 8;
      sz |= (((uint32_t)c) << 8*(sizeof(uint32_t)-1));
    }
  *szp = (int32_t)sz;
  return 0;
}

int save_uint32 (save_ctx_t *sctx, uint32_t sz)
{
  int i, c;
  FILE *f = sctx->f;

  for (i=0; i<sizeof(uint32_t); i++)
    {
      c = sz & 0xff;
      if (putc_unlocked (c, f) < 0) return errno;
      sz >>= 8;
    }
  return 0;
}

int restore_uint32 (restore_ctx_t *rctx, uint32_t *szp)
{
  FILE *f = rctx->f;
  int i, c;
  uint32_t sz;

  for (i=0, sz=0; i<sizeof(int32_t); i++)
    {
      c = getc_unlocked (f);
      if (c==EOF)
	return c;
      sz >>= 8;
      sz |= (((uint32_t)c) << 8*(sizeof(uint32_t)-1));
    }
  *szp = sz;
  return 0;
}

int save_long (save_ctx_t *sctx, long sz)
{
  int i, c;
  FILE *f = sctx->f;

  for (i=0; i<sizeof(long); i++)
    {
      c = sz & 0xff;
      if (putc_unlocked (c, f) < 0) return errno;
      sz >>= 8;
    }
  return 0;
}

int restore_long (restore_ctx_t *rctx, long *szp)
{
  FILE *f = rctx->f;
  int i, c;
  unsigned long sz; /* must be unsigned, see comment in restore_int32() */

  for (i=0, sz=0; i<sizeof(long); i++)
    {
      c = getc_unlocked (f);
      if (c==EOF)
	return c;
      sz >>= 8;
      sz |= (((unsigned long)c) << 8*(sizeof(long)-1));
    }
  *szp = (long)sz;
  return 0;
}

int save_ulong (save_ctx_t *sctx, unsigned long sz)
{
  int i, c;
  FILE *f = sctx->f;

  for (i=0; i<sizeof(unsigned long); i++)
    {
      c = sz & 0xff;
      if (putc_unlocked (c, f) < 0) return errno;
      sz >>= 8;
    }
  return 0;
}

int restore_ulong (restore_ctx_t *rctx, unsigned long *szp)
{
  FILE *f = rctx->f;
  int i, c;
  unsigned long sz;

  for (i=0, sz=0; i<sizeof(unsigned long); i++)
    {
      c = getc_unlocked (f);
      if (c==EOF)
	return c;
      sz >>= 8;
      sz |= (((unsigned long)c) << 8*(sizeof(unsigned long)-1));
    }
  *szp = sz;
  return 0;
}

int save_ctime (save_ctx_t *sctx, time_t sz)
{
  int i, c;
  FILE *f = sctx->f;

  for (i=0; i<sizeof(time_t); i++)
    {
      c = sz & 0xff;
      if (putc_unlocked (c, f) < 0) return errno;
      sz >>= 8;
    }
  return 0;
}

int restore_ctime (restore_ctx_t *rctx, time_t *szp)
{
  FILE *f = rctx->f;
  int i, c;
  unsigned long sz; /* must be unsigned, see comment in restore_int32() */
  /* the following code assumes that sizeof(time_t) <= sizeof(unsigned long) */

  for (i=0, sz=0; i<sizeof(time_t); i++)
    {
      c = getc_unlocked (f);
      if (c==EOF)
	return c;
      sz >>= 8;
      sz |= (((unsigned long)c) << 8*(sizeof(time_t)-1));
    }
  *szp = (time_t)sz;
  return 0;
}

int save_double (save_ctx_t *sctx, double x)
{
  int i, c;
  FILE *f = sctx->f;

  for (i=0; i<sizeof(double); i++)
    {
      c = ((char *)&x)[i];
      if (putc_unlocked (c, f) < 0) return errno;
    }
  return 0;
}

int restore_double (restore_ctx_t *rctx, double *xp)
{
  FILE *f = rctx->f;
  int i, c;

  *xp = 0.0;
  for (i=0; i<sizeof(double); i++)
    {
      c = getc_unlocked (f);
      if (c==EOF)
	return c;
      ((char *)xp)[i] = c;
    }
  return 0;
}

int save_string (save_ctx_t *sctx, char *s)
{
  FILE *f = sctx->f;
  int c, err;
  size_t j, m;

  if (s==NULL)
    return save_size_t (sctx, (size_t)-1L);
  m = strlen (s);
  err = save_size_t (sctx, m);
  if (err) return err;
  for (j=0; j<m; j++)
    {
      c = s[j];
      if (putc_unlocked (c, f) < 0) return errno;
    }
  return 0;
}

int restore_string (restore_ctx_t *rctx, char **sp)
{
  gc_t *gc_ctx = rctx->gc_ctx;
  FILE *f = rctx->f;
  size_t j, m;
  char *s;
  int c, err;

  *sp = NULL;
  err = restore_size_t (rctx, &m);
  if (err) return err;
  if (m==(size_t)-1L)
    return 0;
  s = gc_base_malloc (gc_ctx, m+1);
  for (j=0; j<m; j++)
    {
      c = getc_unlocked (f);
      if (c==EOF) return c;
      s[j] = c;
    }
  s[m] = '\0';
  *sp = s;
  return 0;
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
