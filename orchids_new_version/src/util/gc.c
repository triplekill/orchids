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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
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
  if (p->class!=NULL)
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
		  gc_touch (gc_ctx, sdata->data[i]);
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
	      if (p->class!=NULL) /* This test is in principle useless:
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
	  if (p->class!=NULL)
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
  if (p->class==NULL) /* no subfield: stop */
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
