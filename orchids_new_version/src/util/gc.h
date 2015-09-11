/**
 ** @file gc.h
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

#ifndef ORCHIDS_GC
#define ORCHIDS_GC

#include <stdint.h>

typedef struct gc_s gc_t;
typedef struct gc_header_s gc_header_t;
typedef struct gc_traverse_ctx_s gc_traverse_ctx_t;

typedef int (*subfield_action) (gc_traverse_ctx_t *gc_traverse_ctx,
				gc_header_t *subfield,
				void *data);

struct gc_traverse_ctx_s {
  gc_t *gc_ctx;
  subfield_action do_subfield;
  int traverse_action;
#define TRAVERSE_GC_CHECK 1
#define TRAVERSE_MARSHALL 2 /* reserved */
};

typedef struct save_ctx_s save_ctx_t;
struct save_ctx_s {
  gc_t *gc_ctx;
  FILE *f;
  unsigned long fuzz;
};

struct strhash_s; /* in strhash.h */
struct hash_s; /* in hash.h */
struct rule_compiler_s; /* in orchids.h */
struct field_record_table_s; /* in orchids.h */
struct issdl_function_s; /* in orchids.h */

typedef struct restore_ctx_s restore_ctx_t;
typedef int (*readc_f) (void *data);
struct restore_ctx_s {
  gc_t *gc_ctx;
  size_t version;
  FILE *f;
  struct strhash_s *externs; /* from orchids_context_t->xclasses */
  struct rule_compiler_s *rule_compiler; /* from orchids_context_t->rule_compiler */
  struct issdl_function_s *new_vm_func_tbl; /* from orchids_context_t->vm_func_tbl */
  int32_t new_vm_func_tbl_sz; /* from orchids_context_t->vm_func_tbl_sz */
  struct issdl_function_s *vm_func_tbl; /* not from orchids_context_t->vm_func_tbl: this one is meant to be restored first */
  int32_t vm_func_tbl_sz; /* not from orchids_context_t->vm_func_tbl_sz: this one is meant to be restored first */
  struct field_record_table_s *global_fields; /* not from orchids_context_t->global_fields: this one is meant to be restored first */
  struct hash_s *shared_hash; /* filled in while restoring */
  int32_t errs;
#define RESTORE_UNKNOWN_FIELD_NAME 0x1
#define RESTORE_UNKNOWN_PRIMITIVE 0x2
};

typedef struct gc_class_s gc_class_t;
struct gc_class_s
{
  long ident;
#define GC_ID(a,b,c,d) (((a)<<24) | ((b)<<16) | ((c)<<8) | (d))
  void (*mark_subfields) (gc_t *gc_ctx, gc_header_t *p);
  /* mark_subfields(): call gc_touch(gc_ctx,q) for each garbage-collectable
     subfield q of p */
  void (*finalize) (gc_t *gc_ctx, gc_header_t *p);
  /* finalize(): free all auxiliary data (e.g., close files,
     free non-garbage-collectable data).  Don't free garbage-collectable
     data, or p itself. */
  /* The above are special cases (done for efficiency) of the following
     higher-order function.  This is meant to apply do_subfield
     to each garbage-collectable subfield, do_aux to each auxiliary
     field, passing data to each.  The returned ints are meant to
     be error codes.  Any non-zero error code interrupts the interation.
  */
  int (*traverse) (gc_traverse_ctx_t *gc_traverse_ctx,
		   gc_header_t *p,
		   void *data);
  /* Save structure (to file, typically): */
  int (*save) (save_ctx_t *sctx, gc_header_t *p);
  /* Restore a structure (from file, typically);
     if error, then returns NULL and sets errno: */
  gc_header_t *(*restore) (restore_ctx_t *rctx);
};

struct gc_header_s
{
  unsigned char type; /* natural number coding the kind of object
			 we have here.  Actual numbers will be
			 defined in lang.h */
  unsigned char flags;
  /* Flags used by the garbage collector itself. */
#define GC_FLAGS_GREY_MASK (0x01)
# define GC_FLAGS_GREY GC_FLAGS_GREY_MASK
#define GC_FLAGS_ODD_COLOR_MASK (0x01 << 1)
# define GC_FLAGS_ODD_WHITE (0)
# define GC_FLAGS_ODD_BLACK GC_FLAGS_ODD_COLOR_MASK
#define GC_FLAGS_EVEN_COLOR_MASK (0x01 << 2)
# define GC_FLAGS_EVEN_WHITE (0)
# define GC_FLAGS_EVEN_BLACK GC_FLAGS_EVEN_COLOR_MASK
  /* Flags used by gc_check() */
#define GC_FLAGS_CHECKED (0x01 << 3)
#define GC_FLAGS_IN_GREY_LIST (0x01 << 4)
  /* Flags used by estimate_sharing() */
#define GC_FLAGS_EST_SEEN (0x01 << 5)
#define GC_FLAGS_EST_SHARED (0x01 << 6)
  /* Reserved flags */
#define GC_FLAGS_RESERVED (0x01 << 7)
  unsigned char magic[2]; /* padding; also for debugging */
  struct gc_class_s *class; /* NULL if no embedded pointer to
			       garbage-collected data */
  struct gc_header_s *next; /* sequence of allocated gc_headers;
			     used to sweep through them. */
  struct gc_header_s *nextgrey; /* used to implement next item in
				 grey list. */
};

typedef struct gc_rootzone {
  struct gc_rootzone *next;
  gc_header_t **root;
  /* Root pointers are meant to be fixed.
     E.g., not a stack, whose depth varies.
     Elements stored in a stack should be gc_touch()ed when inserted
     instead. */
} gc_rootzone;

typedef struct gc_stack_data {
  struct gc_stack_data *next;
  int n; /* number of elements currently in 'data' table */
  void *data[1]; /* actually gc_header_t *data[nmax], for some value
	            nmax >= n that we don't care to store.
                    The type is void *, because this is the only pointer
                    type in C that does not obey the strict aliasing rule,
                    and we need each data[i] to be of a type different
                    from gc_header_t * (still, a pointer to something
                    that contains a gc_header_t in first position) */
} gc_stack_data;

/* In the following macros, we keep an identifier '__gc' that nobody
   else should use.
   No bounds are checked here: be warned.
*/
#define GC_START_PROGRESSIVE(gc_ctx, nmax)				\
  {									\
  struct __gc { struct gc_stack_data *next; int n; void *data[nmax]; } __gc; \
  __gc.next = (gc_ctx)->stack_data;					\
  __gc.n = 0;								\
  (gc_ctx)->stack_data = (gc_stack_data *)&__gc
#define GC_END_PROGRESSIVE(gc_ctx)			\
  (gc_ctx)->stack_data = (gc_ctx)->stack_data->next; }
#define GC_PROTECT_PROGRESSIVE(gc_ctx, value) \
  do { GC_TOUCH(gc_ctx,value);		      \
    __gc.data[__gc.n++] = (char *)value; } while (0)
#define GC_PROTECT_PROGRESSIVE_NOTOUCH(value) \
   __gc.data[__gc.n++] = value
#define GC_N __gc.n

#define GC_START(gc_ctx, nmax) \
  { \
  struct __gc { struct gc_stack_data *next; int n; void *data[nmax]; } __gc = { (gc_ctx)->stack_data, nmax, }; /* Note: data[] will be zeroed out */ \
  (gc_ctx)->stack_data = (gc_stack_data *)&__gc
#define GC_END(gc_ctx)					\
  (gc_ctx)->stack_data = (gc_ctx)->stack_data->next; }

#define GC_UPDATE(gc_ctx, i, new_value) \
  GC_TOUCH(gc_ctx, __gc.data[i] = (void *)(new_value))
#define GC_UPDATE_NOTOUCH(i, new_value) \
  __gc.data[i] = (void *)new_value
#define GC_LOOKUP(i) __gc.data[i]
#define GC_DATA() __gc.data

#define GC_INC_UNIT (256)
/* With this value, int's should be at least 2 byte long,
   otherwise we won't be able to specify any interesting
   value for gc_mark_mod, gc_sweep_mod. */

struct gc_s
{
  // General parameters:
  int gc_mark_mod; /* controls how many objects
		      (gc_header_t) are marked at each
		      call to gc_mark(): on average,
		      gc_mark_mod / GC_INC_UNIT per call.
		      Ratio should be at least 1. */
  int gc_sweep_mod; /* same, for sweeping. */
  gc_rootzone *roots; /* list of root gc_headers, linked by 'next'. */
  gc_stack_data *stack_data; /* list of temporary roots, located on
				the C stack. */
  // Data that is meant to evolve through time:
  gc_header_t *grey; /* list of grey items, linked by 'nextgrey';
		      they should all have the GC_FLAGS_GREY bit set
		      (and only them) */
  gc_header_t *alloced; /* list of all allocated items, linked by 'next';
			 serves in the sweep phase */
  gc_header_t **sweep_current;
  int generation; /* GC generation, i.e., number of times we
		     have started a new mark phase. */
  int mark_state; /* GC marking state */
#define GC_MARK_STATE_INIT 0
#define GC_MARK_STATE_RUNNING 1
#define GC_MARK_STATE_END 2
  int gc_mark_cnt;
  int gc_sweep_cnt;
};

gc_t *gc_init (void);
void gc_add_root (gc_t *gc_ctx, gc_header_t **root);

void *gc_base_malloc (gc_t *gc_ctx, size_t n);
/* gc_base_malloc(): as for malloc(), except calls
   gc before malloc(), and calls gc() more insistently in
   case memory is lacking.
   gc_base_malloc() is meant to allocate non-garbage-collectable
   objects.  For garbage-collectable objects, use gc_alloc().
*/
void *gc_base_realloc (gc_t *gc_ctx, void *p, size_t n);
/* gc_base_realloc(): as for realloc(), except calls
   gc before realloc(), and calls gc() more insistently in
   case memory is lacking.
   gc_base_realloc() is meant to reallocate non-garbage-collectable
   objects.  Don't use on garbage-collectable objects.
*/
char *gc_strdup (gc_t *gc_ctx, char *s);
/* As strdup(), except calling gc_base_malloc() instead of malloc(). */
void gc_base_free (void *p);
/* gc_base_free(): to use instead of free() for gc_base_malloc() data.
 */

void *gc_alloc (gc_t *gc_ctx, size_t n, gc_class_t *class);
/* gc_alloc(): allocated a garbage-collectable object.
   Guaranteed to start with a gc_header_t.
 */

void gc_touch (gc_t *gc_ctx, gc_header_t *p);
#define GC_TOUCH(gc_ctx,p) gc_touch(gc_ctx,(gc_header_t *)(p))

int gc_mark (gc_t *gc_ctx); /* return #objects marked black;
			       may be 0 because we do not have any
			       grey object left, or because we have
			       only touched the roots. */
int gc_sweep (gc_t *gc_ctx); /* return #objects swept; should be
				zero only if no object remains in grey
				list. */

void gc (gc_t *gc_ctx); /* calls gc_mark(), gc_sweep() so as to do
			   a bit of marking, a bit of sweeping.
			   Changes generation if both are done.
			*/
void gc_full (gc_t *gc_ctx); /* Does a complete round of garbage
				collection, and changes generation.
				You might need to call gc_full()
				twice to reclaim all useless memory.
			     */

void gc_check (gc_t *gc_ctx); /* Looks for inconsistencies in memory.
				 Works similarly to fsck on Unix
				 systems.  Used to detect memory
				 and garbage-collector related bugs. */

void estimate_sharing (gc_t *gc_ctx, gc_header_t *p);
/* Sets GC_FLAGS_EST_* flags of all objects reachable from p.
   - GC_FLAGS_EST_SEEN means that object is reachable
   - GC_FLAGS_EST_SHARED means that object is reachable from at least
   two different objects.
   Intended use is:
   - start from a time where no object has any of the GC_FLAGS_EST_* set
   - assume you want to know which objects are (reachable, shared) from
   a group of objects p1, p2, ..., pn (i.e., from any of them)
   - call estimate_sharing() on each of p1, p2, ..., pn successively
   - if you need to know whether p is (reachable, shared) from p1, p2, ..., pn,
   just read its p->type field, anded with GC_FLAGS_EST_SEEN, resp.,
   GC_FLAGS_EST_SHARED;
   - in the last item, you may decide to reset the GC_FLAGS_EST_SHARED
   flag, or the GC_FLAGS_EST_SEEN flag, by hand (e.g., as in save_gc_struct()),
   but do not reset both, as it would break the next step;
   - once your are done,
   restore memory to its prior state (with no GC_FLAGS_EST_* flag set),
   by calling reset_sharing() on each of p1, p2, ..., pn in turn.
 */
void reset_sharing (gc_t *gc_ctx, gc_header_t *p);

int save_size_t (save_ctx_t *sctx, size_t sz);
int restore_size_t (restore_ctx_t *rctx, size_t *szp);
int save_int (save_ctx_t *sctx, int sz);
int restore_int (restore_ctx_t *rctx, int *szp);
int save_uint (save_ctx_t *sctx, unsigned int sz);
int restore_uint (restore_ctx_t *rctx, unsigned int *szp);
int save_int32 (save_ctx_t *sctx, int32_t sz);
int restore_int32 (restore_ctx_t *rctx, int32_t *szp);
int save_uint32 (save_ctx_t *sctx, uint32_t sz);
int restore_uint32 (restore_ctx_t *rctx, uint32_t *szp);
int save_long (save_ctx_t *sctx, long sz);
int restore_long (restore_ctx_t *rctx, long *szp);
int save_ulong (save_ctx_t *sctx, unsigned long sz);
int restore_ulong (restore_ctx_t *rctx, unsigned long *szp);
int save_ctime (save_ctx_t *sctx, time_t sz);
int restore_ctime (restore_ctx_t *rctx, time_t *szp);
int save_double (save_ctx_t *sctx, double x);
int restore_double (restore_ctx_t *rctx, double *xp);
int save_string (save_ctx_t *sctx, char *s);
int restore_string (restore_ctx_t *rctx, char **sp);

#endif /* ORCHIDS_GC */

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
