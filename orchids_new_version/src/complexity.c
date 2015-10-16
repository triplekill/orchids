/**
 ** @file complexity.c
 ** Automatic complexity evaluator, used by rule_compiler.c
 **
 ** @author Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
 **
 ** @version 1.0
 ** @ingroup compiler
 **
 ** @date  Started on: Jeu 15 oct 2015 08:50:00 UTC
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <limits.h>

#include "gc.h"
#include "complexity.h"

complexity_graph_t *new_complexity_graph (gc_t *gc_ctx)
{
  complexity_graph_t *g;
  vertex_info_t *nodes;
#define NMAX_INITIAL 100
  
  nodes = gc_base_malloc (gc_ctx, NMAX_INITIAL*sizeof(vertex_info_t));
  g = gc_base_malloc (gc_ctx, sizeof(complexity_graph_t));
  g->n = 0;
  g->nmax = NMAX_INITIAL;
  g->first_bad = ULONG_MAX;
  g->nodes = nodes;
  return g;
}

void free_complexity_graph (complexity_graph_t *g)
{
  vertex_t i, n;

  n = g->n;
  for (i=0; i<n; i++)
    gc_base_free (g->nodes[i].edges);
  gc_base_free (g->nodes);
  gc_base_free (g);
}

vertex_t cg_new_node (gc_t *gc_ctx, complexity_graph_t *g, void *data, char type, char label)
{
  vertex_t n;
#define NMAX_INCREMENT 100
#define EDGE_NMAX_INITIAL 10
  vertex_info_t *vi;
  
  n = g->n;
  if (n>=g->nmax)
    {
      g->nmax += NMAX_INCREMENT;
      g->nodes = gc_base_realloc (gc_ctx, g->nodes, g->nmax*sizeof(vertex_info_t));
    }
  vi = &g->nodes[n];
  g->n = n+1;
  vi->type = type;
  vi->label = label;
  vi->on_stack = 0;
  vi->data = data;
  vi->index = ULONG_MAX; /* undefined yet */
  vi->low = ULONG_MAX; /* undefined yet */
  vi->next_on_stack = ULONG_MAX; /* empty list */
  vi->scc_root = ULONG_MAX; /* undefined yet */
  vi->degree = ULONG_MAX;
  vi->n = 0;
  vi->nmax = EDGE_NMAX_INITIAL;
  vi->edges = gc_base_malloc (gc_ctx, EDGE_NMAX_INITIAL*sizeof(edge_info_t));
  return n;
}

size_t cg_new_edge (gc_t *gc_ctx, complexity_graph_t *g, vertex_t from, vertex_t to,
		    unsigned long weight)
{
  vertex_info_t *vi;
  size_t n;
#define EDGE_NMAX_INCREMENT 10
  edge_info_t *ei;

  vi = &g->nodes[from];
  n = vi->n;
  if (n>=vi->nmax)
    {
      vi->nmax += EDGE_NMAX_INCREMENT;
      vi->edges = gc_base_realloc (gc_ctx, vi->edges, vi->nmax*sizeof(edge_info_t));
    }
  vi->n = n+1;
  ei = &vi->edges[n];
  ei->target = to;
  ei->weight = weight;
  return n;
}

vertex_t cg_new_max_node_va (gc_t *gc_ctx, complexity_graph_t *g, void *data,
			     char label, va_list ap)
{
  vertex_t v;
  vertex_t succ;

  v = cg_new_node (gc_ctx, g, data, VI_TYPE_MAX, label);
  while ((succ = va_arg(ap,vertex_t)) != CG_NULL)
    (void) cg_new_edge (gc_ctx, g, v, succ, 1L);
  return v;
}

vertex_t cg_new_max_node (gc_t *gc_ctx, complexity_graph_t *g, void *data, char label, ...)
{
  va_list ap;
  vertex_t v;

  va_start (ap, label);
  v = cg_new_max_node_va (gc_ctx, g, data, label, ap);
  va_end (ap);
  return v;
}

vertex_t cg_new_plus_node_va (gc_t *gc_ctx, complexity_graph_t *g, void *data,
			      char label, va_list ap)
{
  vertex_t v;
  vertex_t succ;
  unsigned long weight;

  v = cg_new_node (gc_ctx, g, data, VI_TYPE_PLUS, label);
  while ((succ = va_arg(ap,vertex_t)) != CG_NULL)
    {
      weight = va_arg(ap, unsigned long);
      if (weight==0)
	continue;
      (void) cg_new_edge (gc_ctx, g, v, succ, weight);
    }
  return v;
}

vertex_t cg_new_plus_node (gc_t *gc_ctx, complexity_graph_t *g, void *data, char label, ...)
{
  va_list ap;
  vertex_t v;

  va_start (ap, label);
  v = cg_new_plus_node_va (gc_ctx, g, data, label, ap);
  va_end (ap);
  return v;
}

static int cg_bad_vertex (complexity_graph_t *g, vertex_t v)
{
  vertex_info_t *vi;
  vertex_t scc_root;
  unsigned long weight;
  size_t i, n;

  vi = &g->nodes[v];
  if (vi->type!=VI_TYPE_PLUS)
    return 0; /* VI_TYPE_MAX nodes are never bad */
  scc_root = vi->scc_root;
  weight = 0;
  n = vi->n;
  for (i=0; i<n; i++)
    {
      if (vi->edges[i].weight>=2)
	return 1;
      weight += vi->edges[i].weight;
      if (weight>=2)
	return 1;
    }
  return 0;
}

static int cg_trivial_scc (complexity_graph_t *g, vertex_t v)
{
  vertex_info_t *vi;
  size_t i, n;
  edge_info_t *ei;
  
  v = g->nodes[v].scc_root;
  vi = &g->nodes[v];
  if (vi->next_on_stack!=ULONG_MAX) /* if there is another node in the same scc,
				       then it is non-trivial */
    return 0;
  n = vi->n;
  for (i=0; i<n; i++)
    {
      ei = &vi->edges[i];
      if (ei->target==v) /* and if v has a self-loop, then it is non-trivial */
	return 0;
    }
  return 1;
}

static void cg_compute_sccs_from_root_rec (complexity_graph_t *g, vertex_t root,
					   unsigned long *indexp, unsigned long *stackp)
{
  vertex_info_t *vi;
  vertex_t succ, w;
  vertex_info_t *succ_vi, *wi;
  size_t i, n;
  edge_info_t *ei;
  unsigned long dv;
  
  vi = &g->nodes[root];
  vi->low = vi->index = (*indexp)++;
  /* Now push root onto stack. */
  vi->next_on_stack = *stackp;
  vi->on_stack = 1;
  *stackp = root;
  /* For each successor 'succ': */
  n = vi->n;
  for (i=0; i<n; i++)
    {
      ei = &vi->edges[i];
      succ = ei->target;
      succ_vi = &g->nodes[succ];
      if (succ_vi->index==ULONG_MAX) /* if succ's index is undefined, then
					      recurse on it */
	{
	  cg_compute_sccs_from_root_rec (g, succ, indexp, stackp);
	  if (succ_vi->low < vi->low)
	    vi->low = succ_vi->low;
	}
      else if (succ_vi->on_stack) /* otherwise, and if succ is on the stack,
				     then it is in the current scc */
	{
	  if (succ_vi->index < vi->low)
	    vi->low = succ_vi->index;
	}
    }
  if (vi->low == vi->index) /* then vi is the root node of the current scc:
			       pop the stack until we have popped 'root' (i.e., vi),
			       and put all the elements in the scc.
			       Since we reuse 'next_on_stack' to store the scc,
			       we have almost nothing to do.  We just sever the list
			       at 'root' (vi->next_on_stack=ULONG_MAX), set back the
			       stack to the next vertex (the old value of vi->next_on_stack),
			       and we iterate the whole list starting at the old *stackp
			       (in succ) to reset the on_stack flags. */
    {
      succ = root = *stackp;
      *stackp = vi->next_on_stack;
      vi->next_on_stack = ULONG_MAX;
      while (succ!=ULONG_MAX) /* Now sweep through the scc and reset the on_stack flags.
				 We take the opportunity to set the 'root' field of
				 every node in the scc to 'root' (the first element of
				 the scc, seen as a list).
			       */
	{
	  succ_vi = &g->nodes[succ];
	  succ_vi->on_stack = 0;
	  succ_vi->scc_root = root;
	  succ = succ_vi->next_on_stack;
	}
      /* It is a good idea now to compute complexities. */
      vi = &g->nodes[root];
      vi->degree = 0L;
      if (cg_trivial_scc (g, root))
	for (w = root; w!=ULONG_MAX; w = wi->next_on_stack)
	  {
	    wi = &g->nodes[w];
	    n = wi->n;
	    for (i=0; i<n; i++)
	      {
		ei = &wi->edges[i];
		succ = ei->target;
		succ_vi = &g->nodes[succ];
		dv = g->nodes[succ_vi->scc_root].degree; /* edge w -> succ must go out of
							    the scc, since it is trivial */
		if (dv > vi->degree)
		  vi->degree = dv;
	      }
	  }
      else /* the scc is non-trivial */
	for (w = root; w!=ULONG_MAX; w = wi->next_on_stack)
	  {
	    wi = &g->nodes[w];
	    if (cg_bad_vertex (g, w))
	      {
		vi->degree = ULONG_MAX; /* exponential behavior */
		if (g->first_bad==ULONG_MAX)
		  g->first_bad = w;
		break;
	      }
	    n = wi->n;
	    for (i=0; i<n; i++)
	      {
		ei = &wi->edges[i];
		succ = ei->target;
		succ_vi = &g->nodes[succ];
		if (succ_vi->scc_root==root) /* edge leads into the same scc: ignore */
		  continue;
		dv = g->nodes[succ_vi->scc_root].degree; /* edge w -> succ goes out of the scc */
		/* We fetch the degree (already computed) of succ; this is stored in the
		   scc_root's degree field */
		if (wi->type==VI_TYPE_PLUS) /* this is an expensive edge: add 1 to dv */
		  dv++;
		/* Finally, take the max with previous degree value */
		if (dv > vi->degree)
		  vi->degree = dv;
	      }
	  }
    }
}

void cg_compute_sccs_from_root (complexity_graph_t *g, vertex_t root, unsigned long *indexp)
{
  unsigned long stack = ULONG_MAX;
  
  if (g->nodes[root].index==ULONG_MAX) /* if index is still undefined */
    cg_compute_sccs_from_root_rec (g, root, indexp, &stack);
}

void cg_compute_sccs (complexity_graph_t *g)
{
  vertex_t i, n;
  unsigned long index = 0L;

  n = g->n;
  for (i=0; i<n; i++)
    cg_compute_sccs_from_root (g, i, &index);
}

unsigned long cg_degree (complexity_graph_t *g, vertex_t v)
{
  vertex_info_t *vi = &g->nodes[v];

  return g->nodes[vi->scc_root].degree;
}

/*
** Copyright (c) 2015 by Jean GOUBAULT-LARRECQ, Laboratoire Spécification
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
