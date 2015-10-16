/**
 ** @file complexity.h
 ** Data structures for complexity.c
 **
 ** @author Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
 **
 ** @version 1.0
 ** @ingroup compiler
 **
 ** @date  Started on: Started on: Jeu 15 oct 2015 08:50:00 UTC
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef COMPLEXITY
#define COMPLEXITY

/* Undirected graphs.
   Vertices are represented as unsigned longs.
 */
typedef unsigned long vertex_t;

typedef struct complexity_graph_s complexity_graph_t;

typedef struct vertex_info_s vertex_info_t;
typedef struct edge_info_s edge_info_t;

struct vertex_info_s {
#define VI_TYPE_MAX 1
#define VI_TYPE_PLUS 2  
  char type; /* VI_TYPE_* */
  char label; /* +0 or +1 */
  char on_stack; /* used internally to Tarjan's scc algorithm */
  void *data; /* pointer to a node_expr_t or other */
  unsigned long index, low; /* for the computation of sccs */
  vertex_t next_on_stack; /* used internally to Tarjan's scc algorithm;
			     in case the current vertex is on the stack (on_stack!=0),
			     gives next vertex on the stack, or ULONG_MAX if end of
			     stack; once Tarjan's algorithm is over,
			     this will contain a list of vertices in the same scc---
			     the complete list of all of them if we are the root
			     node of the scc (index==low).
			  */
  vertex_t scc_root; /* root of this node's scc; obtained by Tarjan's algorithm */
  unsigned long degree; /* if this node is a polynomial, it will be a Theta(n^degree);
			   degree is ULONG_MAX if node is exponential (or a polynomial
			   with a degree >=ULONG_MAX) */
  /* degree is only computed for the root nodes of sccs,
     i.e., for nodes v such that v==g->nodes[v].scc_root
  */
  size_t n; /* number of successors */
  size_t nmax; /* number of allocated successors until now */
  edge_info_t *edges; /* table of nsuccs successors */
};

struct edge_info_s {
  vertex_t target;
  unsigned long weight; /* the edge's label */
};

struct complexity_graph_s {
  vertex_t n; /* number of nodes */
  vertex_t nmax; /* number of allocated nodes until now */
  vertex_t first_bad; /* will be filled in by sg_compute_sccs(), and will contain either
			 ULONG_MAX or the first found bad vertex in a non-trivial scc
			 (causing exponential behavior). */
  vertex_info_t *nodes; /* table of n vertices */
};

#define CG_TYPE(g,v) (g)->nodes[v].type
#define CG_LABEL(g,v) (g)->nodes[v].label
#define CG_DEGREE(g,v) cg_degree(g,v)
#define CG_DATA(g,v) (g)->nodes[v].data
#define CG_SCC_ROOT(g,v) (g)->nodes[v].scc_root
#define CG_NEXT(g,v) (g)->nodes[v].next_on_stack

complexity_graph_t *new_complexity_graph (gc_t *gc_ctx);
void free_complexity_graph (complexity_graph_t *g);
vertex_t cg_new_node (gc_t *gc_ctx, complexity_graph_t *g, void *data, char type, char label);
size_t cg_new_edge (gc_t *gc_ctx, complexity_graph_t *g, vertex_t from, vertex_t to,
		    unsigned long weight);

#define CG_NULL ULONG_MAX
/* Declare a new node u with u_{n+a} = max (v_n, w_n, ...)
   a is given in argument label
   the list v, w, ..., is a CG_NULL terminated list of vertex_t (given as ap, or hidden in '...')
   data is some data associated with u
*/
vertex_t cg_new_max_node_va (gc_t *gc_ctx, complexity_graph_t *g, void *data,
			     char label, va_list ap);
vertex_t cg_new_max_node (gc_t *gc_ctx, complexity_graph_t *g, void *data, char label, ...);

/* Declare a new node u with u_{n+a} = c v_n + d w_n + ...
   a is given in argument label
   the list v, c, w, d, ..., is a CG_NULL terminated list (given as ap, or hidden in '...')
   that alternates between v:vertex_t, c:unsigned long, w:vertex_t, d:unsigned long, etc.
   caution: c is given after v, not before.
   finally, data is some data associated with u.
*/
vertex_t cg_new_plus_node_va (gc_t *gc_ctx, complexity_graph_t *g, void *data,
			      char label, va_list ap);
vertex_t cg_new_plus_node (gc_t *gc_ctx, complexity_graph_t *g, void *data, char label, ...);

/* Main entry point: cg_compute_sccs(g) computes the sccs by Tarjan's algorithm.
   It also fills in all the remaining information:
   - the scc_root nodes, which tell you what vertex is the root of the current vertex's scc
   - the next_on_stack nodes which link together all the vertices of the scc, starting
   from its scc_root
   - the degree node (beware, only filled in for scc_root nodes: use cg_degree() to
   access the degree of general vertices)
*/
void cg_compute_sccs_from_root (complexity_graph_t *g, vertex_t root, unsigned long *indexp);
void cg_compute_sccs (complexity_graph_t *g);
unsigned long cg_degree (complexity_graph_t *g, vertex_t v);

#endif /* COMPLEXITY */

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
