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

#ifndef ORCHIDS_GC
#define ORCHIDS_GC

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
  /* Reserved flags */
#define GC_FLAGS_VIRTUAL (0x01 << 5)
#define GC_FLAGS_RESERVED1 (0x01 << 6)
#define GC_FLAGS_RESERVED2 (0x01 << 7)
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
  gc_header_t *data[1]; /* actually data[nmax], for some value
			 nmax >= n that we don't care to store. */
} gc_stack_data;

/* In the following macros, we keep an identifier '__gc' that nobody
   else should use.
   No bounds are checked here: be warned.
*/
#define GC_START_PROGRESSIVE(gc_ctx, nmax)				\
  {									\
  struct __gc { struct gc_stack_data *next; int n; gc_header_t *data[nmax]; } __gc; \
  __gc.next = (gc_ctx)->stack_data;					\
  __gc.n = 0;								\
  (gc_ctx)->stack_data = (gc_stack_data *)&__gc
#define GC_END_PROGRESSIVE(gc_ctx)			\
  (gc_ctx)->stack_data = (gc_ctx)->stack_data->next; }
#define GC_PROTECT_PROGRESSIVE(gc_ctx, value) \
  do { GC_TOUCH(gc_ctx,value);		      \
    __gc.data[__gc.n++] = value; } while (0)
#define GC_PROTECT_PROGRESSIVE_NOTOUCH(value) \
   __gc.data[__gc.n++] = value
#define GC_N __gc.n

#define GC_START(gc_ctx, nmax) \
  { \
  struct __gc { struct gc_stack_data *next; int n; gc_header_t *data[nmax]; } __gc = { (gc_ctx)->stack_data, nmax, }; /* Note: data[] will be zeroed out */ \
  (gc_ctx)->stack_data = (gc_stack_data *)&__gc
#define GC_END(gc_ctx)					\
  (gc_ctx)->stack_data = (gc_ctx)->stack_data->next; }

#define GC_UPDATE(gc_ctx, i, new_value) \
  GC_TOUCH(gc_ctx, __gc.data[i] = (gc_header_t *)(new_value))
#define GC_UPDATE_NOTOUCH(i, new_value) \
  __gc.data[i] = new_value
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
#endif /* ORCHIDS_GC */