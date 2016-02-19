/**
 ** @file rule_compiler.c
 ** High-level functions for the yaccer for abstract syntax tree building
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 ** @author Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
 **
 ** @version 0.2
 ** @ingroup compiler
 **
 ** @date  Started on: Sat Feb 22 17:57:07 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#define _XOPEN_SOURCE 600 /* (ugly) for strptime(), included from <time.h>
			     on Linux/glibc2 only if this is defined */

#ifndef PATH_MAX
#define PATH_MAX 8192
/* PATH_MAX is undefined on systems without a limit of filename length,
   such as GNU/Hurd.  Also, defining _XOPEN_SOURCE on Linux will make
   PATH_MAX undefined.
*/
#endif

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>

/* for inet_addr() */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>

#include "orchids.h"
#include "lang.h"
#include "rule_compiler.h"
#include "issdl.tab.h"
#include "ovm.h"
#include "db.h"
#include "complexity.h"

static unsigned long bigprime =
  (sizeof(unsigned long)>=8)?5174544934365344191L: /* 63 bit prime */
  ((sizeof(unsigned long)>=4)?2031676799L: /* 31 bit prime */
   22391L /* 15 bit prime */);

#define STATICS_SZ 16
#define DYNVARNAME_SZ 16

/* Lex scanner globals */
/*
extern FILE *issdlin;
extern int issdllineno_g;
extern char *issdlcurrentfile_g;
*/

/* XXX defined by lex: should be moved into some header file. */
int issdllex_destroy (void * yyscanner); /* really int issdllex_destroy (yyscan_t yyscanner ); */
int issdllex_init (void ** scanner); /* really int issdllex_init (yyscan_t* scanner); */


/* XXX: static function prototype definition:
 * (should be moved in a private header) */

static void compile_and_add_rulefile(orchids_t *ctx, char *rulefile);

static void compile_state_ast(rule_compiler_t *ctx,
			      rule_t *rule,
			      state_t *state,
			      node_state_t *node_state);

static void compile_actions_ast(rule_compiler_t *ctx,
				rule_t *rule,
				state_t *state,
				node_expr_t *actionlist);

static void compile_transitions_ast(rule_compiler_t *ctx,
				    rule_t *rule,
				    state_t *state,
				    node_expr_t *translist);

static void compile_actions_bytecode(node_expr_t *actionlist, bytecode_buffer_t *code);

static bytecode_t *compile_trans_bytecode(rule_compiler_t  *ctx,
					  node_expr_t	*expr,
					  transition_t	*trans);

static void compile_bytecode_stmt(node_expr_t *expr,
				  bytecode_buffer_t *code);

static void compile_bytecode_expr(node_expr_t *expr,
				  bytecode_buffer_t *code);

/* flags passed to compile_bytecode_cond():
   COND_THEN_IMMEDIATE says that we plan label_then to be the label
   right after the compiled bytecode for 'code';
   COND_ELSE_IMMEDIATE is similar, for label_else.
   It is always safe to pass a zero value of flags.
   If you pass one of these flags, you should see
   a SET_LABEL (..., label_then) (resp., label_end)
   after the call to compile_bytecode_cond().
*/
#define COND_THEN_IMMEDIATE 0x1
#define COND_ELSE_IMMEDIATE 0x2
static void compile_bytecode_cond(node_expr_t *expr,
				  bytecode_buffer_t *code,
				  int	label_then,
				  int	label_else,
				  int flags);


static void statics_add(rule_compiler_t *ctx, ovm_var_t *data);

static void build_fields_hash(orchids_t *ctx);

static void build_functions_hash(orchids_t *ctx);

static void fprintf_term_expr(FILE *fp, node_expr_t *expr);

/* Table of compute_stype() functions */

static void compute_stype_ifstmt (rule_compiler_t *ctx, node_expr_t *myself);
static void compute_stype_string_split (rule_compiler_t *ctx, node_expr_t *myself);
static void compute_stype_binop (rule_compiler_t *ctx, node_expr_t *myself);
static void compute_stype_monop (rule_compiler_t *ctx, node_expr_t *myself);
static void compute_stype_return (rule_compiler_t *ctx, node_expr_t *myself);
static void compute_stype_break (rule_compiler_t *ctx, node_expr_t *myself);
static void compute_stype_action (rule_compiler_t *ctx, node_expr_t *myself);
static void compute_stype_cond (rule_compiler_t *ctx, node_expr_t *myself);
static void compute_stype_assoc (rule_compiler_t *ctx, node_expr_t *myself);
static void compute_stype_call (rule_compiler_t *ctx, node_expr_t *myself);
static void compute_stype_fieldname (rule_compiler_t *ctx, node_expr_t *myself);
static void compute_stype_dummy (rule_compiler_t *ctx, node_expr_t *myself);
static void compute_stype_integer (rule_compiler_t *ctx, node_expr_t *myself);
static void compute_stype_double (rule_compiler_t *ctx, node_expr_t *myself);
static void compute_stype_string (rule_compiler_t *ctx, node_expr_t *myself);
static void compute_stype_db_empty (rule_compiler_t *ctx, node_expr_t *myself);
static void compute_stype_event (rule_compiler_t *ctx, node_expr_t *myself);
static void compute_stype_db_pattern (rule_compiler_t *ctx, node_expr_t *myself);
static void compute_stype_db_singleton (rule_compiler_t *ctx, node_expr_t *myself);
static void compute_stype_db_collect (rule_compiler_t *ctx, node_expr_t *myself);
static void compute_stype_ipv4 (rule_compiler_t *ctx, node_expr_t *myself);
static void compute_stype_ipv6 (rule_compiler_t *ctx, node_expr_t *myself);
static void compute_stype_ctime (rule_compiler_t *ctx, node_expr_t *myself);
static void compute_stype_timeval (rule_compiler_t *ctx, node_expr_t *myself);
static void compute_stype_regex (rule_compiler_t *ctx, node_expr_t *myself);

static void (*compute_stype_fun[]) (rule_compiler_t *ctx, node_expr_t *myself) = {
#define CS_NULL 0
  NULL,
#define CS_IFSTMT 1
  compute_stype_ifstmt,
#define CS_STRING_SPLIT 2
  compute_stype_string_split,
#define CS_BINOP 3
  compute_stype_binop,
#define CS_MONOP 4
  compute_stype_monop,
#define CS_RETURN 5
  compute_stype_return,
#define CS_BREAK 6
  compute_stype_break,
#define CS_ACTION 7
  compute_stype_action,
#define CS_COND 8
  compute_stype_cond,
#define CS_ASSOC 9
  compute_stype_assoc,
#define CS_CALL 10
  compute_stype_call,
#define CS_FIELDNAME 11
  compute_stype_fieldname,
#define CS_DUMMY 12
  compute_stype_dummy,
#define CS_INTEGER 13
  compute_stype_integer,
#define CS_DOUBLE 14
  compute_stype_double,
#define CS_STRING 15
  compute_stype_string,
#define CS_DB_EMPTY 16
  compute_stype_db_empty,
#define CS_EVENT 17
  compute_stype_event,
#define CS_DB_PATTERN 18
  compute_stype_db_pattern,
#define CS_DB_SINGLETON 19
  compute_stype_db_singleton,
#define CS_DB_COLLECT 20
  compute_stype_db_collect,
#define CS_IPV4 21
  compute_stype_ipv4,
#define CS_IPV6 22
  compute_stype_ipv6,
#define CS_CTIME 23
  compute_stype_ctime,
#define CS_TIMEVAL 24
  compute_stype_timeval,
#define CS_REGEX 25
  compute_stype_regex,
};

#define CS_NUM 26

static int is_logical_variable(node_expr_t *e)
{
  if (e==NULL)
    return 0;
  if (e->type!=NODE_VARIABLE)
    return 0;
  if (SYM_NAME(e)[0]=='$')
    return 0;
  return 1;
}

static type_t *stype_new (gc_t *gc_ctx, char *type_name, unsigned char tag)
{
  type_t *type;
  size_t len;

  len = strlen(type_name);
  type = gc_base_malloc (gc_ctx, sizeof(type_t) + len+1);
  strcpy (((char *)type)+sizeof(type_t), type_name);
  type->name = ((char *)type)+sizeof(type_t);
  type->tag = tag;
  return type;
}

type_t *stype_from_string (gc_t *gc_ctx, char *type_name, int forcenew,
			   unsigned char tag)
{
  static strhash_t *type_hash = NULL; // XXX global variable here (remove?)
  type_t *type;

  if (type_hash==NULL)
    {
      type_hash = new_strhash(gc_ctx, 31);
      strhash_add(gc_ctx, type_hash, &t_int, t_int.name);
      strhash_add(gc_ctx, type_hash, &t_uint, t_uint.name);
      strhash_add(gc_ctx, type_hash, &t_float, t_float.name);
      strhash_add(gc_ctx, type_hash, &t_bstr, t_bstr.name);
      strhash_add(gc_ctx, type_hash, &t_str, t_str.name);
      strhash_add(gc_ctx, type_hash, &t_ctime, t_ctime.name);
      strhash_add(gc_ctx, type_hash, &t_timeval, t_timeval.name);
      strhash_add(gc_ctx, type_hash, &t_ipv4, t_ipv4.name);
      strhash_add(gc_ctx, type_hash, &t_ipv6, t_ipv6.name);
      strhash_add(gc_ctx, type_hash, &t_regex, t_regex.name);
      strhash_add(gc_ctx, type_hash, &t_snmpoid, t_snmpoid.name);
      strhash_add(gc_ctx, type_hash, &t_event, t_event.name);
      strhash_add(gc_ctx, type_hash, &t_mark, t_mark.name);
    }
  type = strhash_get(type_hash, type_name);
  if (type==NULL && forcenew)
    {
      type = stype_new (gc_ctx, type_name, tag);
      strhash_add(gc_ctx, type_hash, type, type->name);
    }
  return type;
}

static int rule_compiler_mark_hash_walk_func (char *key, void *elt, void *data)
{
  GC_TOUCH ((gc_t *)data, elt);
  return 0;
}

static void rule_compiler_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  rule_compiler_t *ctx = (rule_compiler_t *)p;

  {
    size_t i, n;

    for (i=0, n=ctx->nprotected; i<n; i++)
      GC_TOUCH (gc_ctx, ctx->protected[i]);
  }
  if (ctx->statics!=NULL)
    {
      int32_t i, n;
      
      for (i=0, n=ctx->statics_nb; i<n; i++)
	GC_TOUCH (gc_ctx, ctx->statics[i]);
    }
  /*  Don't do anything for functions_hash: this is a hash table mapping
      static field names to static data.
  (void) strhash_walk (ctx->functions_hash,
		    rule_compiler_mark_hash_walk_func, (void *)gc_ctx);
  */
  /*  Don't do anything for fields_hash: this is a hash table mapping
      static field names to static data.  It also contains values,
      but they are in the ctx->global_fields value, which is a root
      of the garbage collector.
  (void) strhash_walk (ctx->fields_hash,
		    rule_compiler_mark_hash_walk_func, (void *)gc_ctx);
  */
  (void) strhash_walk (ctx->rulenames_hash,
		       rule_compiler_mark_hash_walk_func, (void *)gc_ctx);
  (void) strhash_walk (ctx->statenames_hash,
		       rule_compiler_mark_hash_walk_func, (void *)gc_ctx);
  (void) strhash_walk (ctx->rule_env,
		       rule_compiler_mark_hash_walk_func, (void *)gc_ctx);
  GC_TOUCH (gc_ctx, ctx->first_rule);
  GC_TOUCH (gc_ctx, ctx->last_rule);
  GC_TOUCH (gc_ctx, ctx->returns);
  GC_TOUCH (gc_ctx, ctx->type_stack);
}

static void rule_compiler_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  rule_compiler_t *ctx = (rule_compiler_t *)p;

  gc_base_free (ctx->protected);
  issdllex_destroy (ctx->scanner);
  if (ctx->issdlin!=NULL)
    {
#ifdef ENABLE_PREPROC
      int ret;

      ret = Xpclose(ctx->issdlin);
      if (ret > 0) {
	DebugLog(DF_OLC, DS_ERROR, "error: preprocessor returned %i.\n", ret);
	exit(EXIT_FAILURE);
      }
#else
      Xfclose(ctx->issdlin);
#endif /* ENABLE_PREPROC */
    }
  if (ctx->issdlcurrentfile!=NULL)
    gc_base_free (ctx->issdlcurrentfile);
  /* !!! should probably free ctx->issdlfilestack, too */

  free_strhash (ctx->functions_hash, NULL);
  free_strhash (ctx->fields_hash, NULL);
  gc_free_strhash (ctx->rulenames_hash, NULL);
  gc_free_strhash (ctx->statenames_hash, NULL);
  gc_free_strhash (ctx->rule_env, NULL);

  if (ctx->statics!=NULL)
    gc_base_free (ctx->statics);
  if (ctx->dyn_var_name!=NULL)
    {
      int32_t i, n;

      for (i=0, n=ctx->dyn_var_name_nb; i<n; i++)
	gc_base_free (ctx->dyn_var_name[i]);
      gc_base_free (ctx->dyn_var_name);
    }
}

struct rct_data {
  gc_traverse_ctx_t *gtc;
  void *data;
};

static int rule_compiler_traverse_hash_walk_func (char *key, void *elt,
						  void *data)
{
  struct rct_data *walk_data = (struct rct_data *)data;

  return (*walk_data->gtc->do_subfield) (walk_data->gtc, (gc_header_t *)elt,
					 walk_data->data);
}

static int rule_compiler_traverse (gc_traverse_ctx_t *gtc, gc_header_t *p,
				   void *data)
{
  rule_compiler_t *ctx = (rule_compiler_t *)p;
  int err = 0;
  subfield_action do_subfield = gtc->do_subfield;
  struct rct_data walk_data = { gtc, data };

  {
    size_t i, n;

    for (i=0, n=ctx->nprotected; i<n; i++)
      {
	err = (*do_subfield) (gtc, ctx->protected[i], data);
	if (err)
	  return err;
      }
  }
  if (ctx->statics!=NULL)
    {
      int32_t i, n;

      for (i=0, n=ctx->statics_nb; i<n; i++)
	{
	  err = (*do_subfield) (gtc, (gc_header_t *)ctx->statics[i], data);
	  if (err)
	    return err;
	}
    }
  /*  Don't do anything for functions_hash: this is a hash table mapping
      static field names to static data.
  if (err = strhash_walk (ctx->functions_hash,
		       rule_compiler_traverse_hash_walk_func,
		       (void *)&walk_data))
    return err;
  */
  /*  Don't do anything for fields_hash: this is a hash table mapping
      static field names to static data.  See discussion in
      rule_compiler_mark_subfields().
  if (err = strhash_walk (ctx->fields_hash,
		       rule_compiler_traverse_hash_walk_func,
		       (void *)&walk_data))
    return err;
  */
  err = strhash_walk (ctx->rulenames_hash,
		      rule_compiler_traverse_hash_walk_func,
		      (void *)&walk_data);
  if (err)
    return err;
  err = strhash_walk (ctx->statenames_hash,
		      rule_compiler_traverse_hash_walk_func,
		      (void *)&walk_data);
  if (err)
    return err;
  err = strhash_walk (ctx->rule_env,
		      rule_compiler_traverse_hash_walk_func,
		      (void *)&walk_data);
  if (err)
    return err;
  err = (*do_subfield) (gtc, (gc_header_t *)ctx->first_rule, data);
  if (err)
    return err;
  err = (*do_subfield) (gtc, (gc_header_t *)ctx->last_rule, data);
  if (err)
    return err;
  err = (*do_subfield) (gtc, (gc_header_t *)ctx->returns, data);
  if (err)
    return err;
  err = (*do_subfield) (gtc, (gc_header_t *)ctx->type_stack, data);
  if (err)
    return err;
  return err;
}

static gc_class_t rule_compiler_class = {
  GC_ID('c','o','m','p'),
  rule_compiler_mark_subfields,
  rule_compiler_finalize,
  rule_compiler_traverse
};

rule_compiler_t *new_rule_compiler_ctx(gc_t *gc_ctx)
{
  rule_compiler_t *ctx;
  strhash_t *functions_hash, *fields_hash, *rulenames_hash,
    *statenames_hash, *rule_env;
  gc_header_t **protected;
#define NPROTECTED 256

  functions_hash  = new_strhash(gc_ctx, 1021);
  fields_hash     = new_strhash(gc_ctx, 2503);
  rulenames_hash  = new_strhash(gc_ctx, 251);
  statenames_hash = new_strhash(gc_ctx, 251);
  rule_env        = new_strhash(gc_ctx, 251);
  protected = gc_base_malloc (gc_ctx, NPROTECTED * sizeof(gc_header_t *));

  GC_START (gc_ctx, 1);
  ctx = gc_alloc (gc_ctx, sizeof (rule_compiler_t), &rule_compiler_class);
  ctx->gc.type = T_RULE_COMPILER;
  ctx->gc_ctx = gc_ctx;
  ctx->nprotected = 0;
  ctx->maxprotected = NPROTECTED;
  ctx->protected = protected;
  ctx->currfile = NULL;
  ctx->issdllineno = 1;
  ctx->issdlcurrentfile = NULL;
  ctx->issdlin = NULL;
  SLIST_INIT(&ctx->issdlfilestack);
  ctx->rules = 0;
  ctx->functions_hash  = functions_hash;
  ctx->fields_hash     = fields_hash;
  ctx->rulenames_hash  = rulenames_hash;
  ctx->statenames_hash = statenames_hash;
  ctx->rule_env        = rule_env;
  ctx->statics = NULL; /* temporary */
  ctx->dyn_var_name = NULL; /* temporary */
  ctx->first_rule = NULL;
  ctx->last_rule = NULL;
  ctx->returns = NULL;
  ctx->type_stack = NULL;
  ctx->nerrors = 0;
  ctx->verbose = 0;
  GC_UPDATE(gc_ctx, 0, ctx);

  issdllex_init (&ctx->scanner);
  ctx->statics = gc_base_malloc (gc_ctx, STATICS_SZ * sizeof (ovm_var_t *));
  ctx->statics_nb = 0;
  ctx->statics_sz = STATICS_SZ;

  ctx->dyn_var_name = gc_base_malloc (gc_ctx, DYNVARNAME_SZ * sizeof (char *));
  ctx->dyn_var_name_nb = 0;
  ctx->dyn_var_name_sz = DYNVARNAME_SZ;
  GC_END(gc_ctx);
  return ctx;
}

void compile_gc_protect_drop (rule_compiler_t *ctx, gc_header_t *p,
			      size_t oldprotected)
{
  size_t n;

  GC_START(ctx->gc_ctx, 1);
  GC_UPDATE(ctx->gc_ctx, 0, p);
  n = oldprotected; /* should be <= ctx->nprotected */
  if (n >= ctx->maxprotected)
    {
      ctx->maxprotected += 256;
      ctx->protected = gc_base_realloc (ctx->gc_ctx,
					(void *)ctx->protected,
					ctx->maxprotected);
    }
  GC_TOUCH(ctx->gc_ctx, ctx->protected[n] = p);
  ctx->nprotected = n+1;
  GC_END(ctx->gc_ctx);
}

void compile_gc_protect (rule_compiler_t *ctx, gc_header_t *p)
{
  compile_gc_protect_drop (ctx, p, ctx->nprotected);
}

static void node_state_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  node_state_t *n = (node_state_t *)p;

  GC_TOUCH (gc_ctx, n->actionlist);
  GC_TOUCH (gc_ctx, n->translist);
  GC_TOUCH (gc_ctx, n->mustset);
}

static void node_state_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  node_state_t *state = (node_state_t *)p;

  if (state->file!=NULL)
    gc_base_free (state->file);
  if (state->name!=NULL)
    gc_base_free (state->name);
}

static int node_state_traverse (gc_traverse_ctx_t *gtc, gc_header_t *p,
				    void *data)
{
  node_state_t *n = (node_state_t *)p;
  int err;

  err = (*gtc->do_subfield) (gtc, (gc_header_t *)n->actionlist, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *)n->translist, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *)n->mustset, data);
  return err;
}

static int node_state_save (save_ctx_t *sctx, gc_header_t *p)
{
  node_state_t *n = (node_state_t *)p;
  int err;

  err = save_int (sctx, n->state_id);
  if (err) return err;
  err = save_string (sctx, n->file);
  if (err) return err;
  err = save_int (sctx, n->line);
  if (err) return err;
  err = save_string (sctx, n->name);
  if (err) return err;
  err = save_gc_struct (sctx, (gc_header_t *)n->actionlist);
  if (err) return err;
  err = save_gc_struct (sctx, (gc_header_t *)n->translist);
  if (err) return err;
  err = save_ulong (sctx, n->flags);
  if (err) return err;
  err = save_ulong (sctx, n->an_flags);
  if (err) return err;
  err = save_gc_struct (sctx, (gc_header_t *)n->mustset);
  return err;
}

gc_class_t node_state_class;

static gc_header_t *node_state_restore (restore_ctx_t *rctx)
{
  gc_t *gc_ctx = rctx->gc_ctx;
  node_state_t *n;
  node_expr_t *actionlist, *translist;
  varset_t *mustset;
  int id, line;
  char *file = NULL;
  char *name = NULL;
  unsigned long flags, an_flags;
  int err;

  GC_START (gc_ctx, 23);
  n = NULL;
  err = restore_int (rctx, &id);
  if (err) { errlab: errno = err; goto end; }
  err = restore_string (rctx, &file);
  if (err) goto errlab;
  err = restore_int (rctx, &line);
  if (err) { end_freefile: if (file!=NULL) gc_base_free (file); goto errlab; }
  err = restore_string (rctx, &name);
  if (err) goto end_freefile;
  actionlist = (node_expr_t *)restore_gc_struct (rctx);
  if (actionlist==NULL && errno!=0)
    { err_freefilename:
      if (name!=NULL) gc_base_free (name);
      if (file!=NULL) gc_base_free (file); goto end; }
  GC_UPDATE (gc_ctx, 0, actionlist);
  translist = (node_expr_t *)restore_gc_struct (rctx);
  if (translist==NULL && errno!=0)
    goto err_freefilename;
  GC_UPDATE (gc_ctx, 1, translist);
  err = restore_ulong (rctx, &flags);
  if (err) { errno = err; goto err_freefilename; }
  err = restore_ulong (rctx, &an_flags);
  if (err) { errno = err; goto err_freefilename; }
  mustset = (varset_t *)restore_gc_struct (rctx);
  if (mustset==NULL && errno!=0)
    goto err_freefilename;
  if (mustset!=NULL && TYPE(mustset)!=T_VARSET)
    { errno = -2; goto err_freefilename; }
  n = gc_alloc (gc_ctx, sizeof(node_state_t), &node_state_class);
  n->gc.type = T_NODE_STATE;
  n->state_id = id;
  n->file = file;
  n->line = line;
  n->name = name;
  GC_TOUCH (gc_ctx, n->actionlist = actionlist);
  GC_TOUCH (gc_ctx, n->translist = translist);
  n->flags = flags;
  n->an_flags = an_flags;
  GC_TOUCH (gc_ctx, n->mustset = mustset);
 end:
  GC_END (gc_ctx);
  return (gc_header_t *)n;
}

gc_class_t node_state_class = {
  GC_ID('n','s','t','e'),
  node_state_mark_subfields,
  node_state_finalize,
  node_state_traverse,
  node_state_save,
  node_state_restore
};

static void node_trans_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  node_trans_t *n = (node_trans_t *)p;

  GC_TOUCH (gc_ctx, n->cond);
  GC_TOUCH (gc_ctx, n->sub_state_dest);
}

static void node_trans_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  char *file = ((node_trans_t *)p)->file;
  char *name = ((node_trans_t *)p)->dest;

  if (file!=NULL)
    gc_base_free (file);
  if (name!=NULL)
    gc_base_free (name);
}

static int node_trans_traverse (gc_traverse_ctx_t *gtc, gc_header_t *p,
				void *data)
{
  node_trans_t *n = (node_trans_t *)p;
  int err;

  err = (*gtc->do_subfield) (gtc, (gc_header_t *)n->cond, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *)n->sub_state_dest, data);
  return err;
}

static int node_trans_save (save_ctx_t *sctx, gc_header_t *p)
{
  node_trans_t *n = (node_trans_t *)p;
  int err;

  err = save_int (sctx, n->type);
  if (err) return err;
  err = save_gc_struct (sctx, (gc_header_t *)n->cond);
  if (err) return err;
  err = save_string (sctx, n->dest);
  if (err) return err;
  err = save_string (sctx, n->file);
  if (err) return err;
  err = save_uint (sctx, n->lineno);
  if (err) return err;
  err = save_gc_struct (sctx, (gc_header_t *)n->sub_state_dest);
  if (err) return err;
  err = save_ulong (sctx, n->an_flags);
  return err;
}

gc_class_t node_trans_class;

static gc_header_t *node_trans_restore (restore_ctx_t *rctx)
{
  gc_t *gc_ctx = rctx->gc_ctx;
  int type;
  node_expr_t *cond;
  node_state_t *sub_state_dest;
  char *dest, *file;
  unsigned int lineno;
  unsigned long an_flags;
  node_trans_t *n;
  int err;

  GC_START (gc_ctx, 2);
  n = NULL;
  err = restore_int (rctx, &type);
  if (err) { errno = err; goto end; }
  cond = (node_expr_t *)restore_gc_struct (rctx);
  if (cond==NULL && errno!=0)
    goto end;
  GC_UPDATE (gc_ctx, 0, cond);
  err = restore_string (rctx, &dest);
  if (err) { errno = err; goto end; }
  err = restore_string (rctx, &file);
  if (err) { err_freedest: errno = err;
    if (dest!=NULL) gc_base_free (dest); goto end; }
  err = restore_uint (rctx, &lineno);
  if (err) { err_freedestfile:
    if (file!=NULL) gc_base_free (file); goto err_freedest; }
  sub_state_dest = (node_state_t *)restore_gc_struct (rctx);
  if (sub_state_dest==NULL && errno!=0)
    { if (file!=NULL) gc_base_free (file);
      if (dest!=NULL) gc_base_free (dest); goto end; }
  if (sub_state_dest!=NULL && TYPE(sub_state_dest)!=T_NODE_STATE)
    { err = -2; goto err_freedestfile; }
  GC_UPDATE (gc_ctx, 1, sub_state_dest);
  err = restore_ulong (rctx, &an_flags);
  if (err) goto err_freedestfile;
  n = gc_alloc (gc_ctx, sizeof(node_trans_t), &node_trans_class);
  n->gc.type = T_NODE_TRANS;
  n->type = type;
  GC_TOUCH (gc_ctx, n->cond = cond);
  n->dest = dest;
  n->file = file;
  n->lineno = lineno;
  GC_TOUCH (gc_ctx, n->sub_state_dest = sub_state_dest);
  n->an_flags = an_flags;
 end:
  GC_END (gc_ctx);
  return (gc_header_t *)n;
}

gc_class_t node_trans_class = {
  GC_ID('n','t','r','n'),
  node_trans_mark_subfields,
  node_trans_finalize,
  node_trans_traverse,
  node_trans_save,
  node_trans_restore
};

static void node_rule_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  node_rule_t *n = (node_rule_t *)p;

  GC_TOUCH (gc_ctx, n->init);
  GC_TOUCH (gc_ctx, n->statelist);
}

static void node_rule_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  node_rule_t *n = (node_rule_t *)p;
  node_syncvarlist_t *sv;

  if (n->real_file!=NULL)
    gc_base_free (n->real_file);
  if (n->filename!=NULL)
    gc_base_free (n->filename);
  if (n->name!=NULL)
    gc_base_free (n->name);
  sv = n->sync_vars;
  if (sv!=NULL)
    {
      size_t i, n;

      for (i=0, n=sv->vars_nb; i<n; i++)
	if (sv->vars[i]!=NULL)
	  gc_base_free (sv->vars[i]);
      if (sv->vars!=NULL)
	gc_base_free (sv->vars);
      gc_base_free (sv);
  }
}

static int node_rule_traverse (gc_traverse_ctx_t *gtc, gc_header_t *p,
				    void *data)
{
  node_rule_t *n = (node_rule_t *)p;
  int err;

  err = (*gtc->do_subfield) (gtc, (gc_header_t *)n->init, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *)n->statelist, data);
  return err;
}

static int node_rule_save (save_ctx_t *sctx, gc_header_t *p)
{
  node_rule_t *n = (node_rule_t *)p;
  node_syncvarlist_t *sv;
  size_t i, nvars;
  int err;

  err = save_string (sctx, n->real_file);
  if (err) return err;
  err = save_string (sctx, n->filename);
  if (err) return err;
  err = save_int (sctx, n->line);
  if (err) return err;
  err = save_string (sctx, n->name);
  if (err) return err;
  err = save_gc_struct (sctx, (gc_header_t *)n->init);
  if (err) return err;
  err = save_gc_struct (sctx, (gc_header_t *)n->statelist);
  if (err) return err;
  sv = n->sync_vars;
  if (sv==NULL)
    err = save_size_t (sctx, 0);
  else
    {
      nvars = sv->vars_nb;
      err = save_size_t (sctx, nvars);
      if (err) return err;
      for (i=0; i<nvars; i++)
	{
	  err = save_string (sctx, sv->vars[i]);
	  if (err) return err;
	}
      err = 0;
    }
  err = save_ulong (sctx, n->complexity_degree);
  return err;
}

gc_class_t node_rule_class;

static gc_header_t *node_rule_restore (restore_ctx_t *rctx)
{
  gc_t *gc_ctx = rctx->gc_ctx;
  node_rule_t *n;
  char *real_file, *filename;
  int line;
  char *name;
  node_state_t *init;
  node_expr_t *statelist;
  size_t i, nvars;
  char **vars;
  node_syncvarlist_t *sv;
  unsigned long degree;
  int err;

  GC_START (gc_ctx, 2);
  n = NULL;
  err = restore_string (rctx, &real_file);
  if (err) { errno = err; goto end; }
  err = restore_string (rctx, &filename);
  if (err) { errno = err;
  err_free_real_file:
    if (real_file!=NULL) gc_base_free (real_file); goto end; }
  err = restore_int (rctx, &line);
  if (err)
    { err_freefile: errno = err;
    end_freefile: if (filename!=NULL) gc_base_free (filename); goto err_free_real_file; }
  err = restore_string (rctx, &name);
  if (err) goto err_freefile;
  if (name==NULL) { errno = -2; goto end_freefile; }
  init = (node_state_t *)restore_gc_struct (rctx);
  if (init==NULL && errno!=0)
    { end_freefilename: if (name!=NULL) gc_base_free (name); goto end_freefile; }
  if (init==NULL || TYPE(init)!=T_NODE_STATE)
    { errno = -2; goto end_freefilename; }
  GC_UPDATE (gc_ctx, 0, init);
  statelist = (node_expr_t *)restore_gc_struct (rctx);
  if (statelist==NULL && errno!=0) goto end_freefilename;
  GC_UPDATE (gc_ctx, 1, statelist);
  err = restore_size_t (rctx, &nvars);
  i=0;
  vars = NULL;
  if (nvars==0)
    sv = NULL;
  else
    {
      vars = gc_base_malloc (gc_ctx, nvars*sizeof(char *));
      for (; i<nvars; i++)
	{
	  err = restore_string (rctx, &vars[i]);
	  if (err)
	    {
	    end_freevars:
	      while (i!=0) { --i; gc_base_free (vars[i]); }
	      if (vars!=NULL) gc_base_free (vars);
	      errno = err;
	      goto end_freefilename;
	    }
	  if (vars[i]==NULL) { err = -2; goto end_freevars; }
	}
      sv = gc_base_malloc (gc_ctx, sizeof(node_syncvarlist_t));
      sv->vars_nb = nvars;
      sv->size = nvars;
      sv->grow = 8;
      sv->vars = vars;
    }
  err = restore_ulong (rctx, &degree);
  if (err) goto end_freevars;
  n = gc_alloc (gc_ctx, sizeof(node_rule_t), &node_rule_class);
  n->gc.type = T_NODE_RULE;
  n->line = line;
  n->real_file = real_file;
  n->filename = filename;
  n->name = name;
  GC_TOUCH (gc_ctx, n->init = init);
  GC_TOUCH (gc_ctx, n->statelist = statelist);
  n->sync_vars = sv;
  n->complexity_degree = degree;
 end:
  GC_END (gc_ctx);
  return (gc_header_t *)n;
}

gc_class_t node_rule_class = {
  GC_ID('n','r','u','l'),
  node_rule_mark_subfields,
  node_rule_finalize,
  node_rule_traverse,
  node_rule_save,
  node_rule_restore
};

static void node_expr_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  node_expr_t *n = (node_expr_t *)p;

  GC_TOUCH (gc_ctx, n->file);
  GC_TOUCH (gc_ctx, n->parents);
}

static int node_expr_traverse (gc_traverse_ctx_t *gtc, gc_header_t *p,
			       void *data)
{
  node_expr_t *n = (node_expr_t *)p;
  int err;

  err = (*gtc->do_subfield) (gtc, (gc_header_t *)n->file, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *)n->parents, data);
  if (err)
    return err;
  return 0;
}

static int node_expr_save (save_ctx_t *sctx, gc_header_t *p)
{
  node_expr_t *n = (node_expr_t *)p;
  FILE *f = sctx->f;
  type_t *stype;
  int err;

  err = save_gc_struct (sctx, (gc_header_t *)n->file);
  if (err) return err;
  err = save_uint (sctx, n->lineno);
  if (err) return err;
  err = save_int (sctx, n->type);
  if (err) return err;
  err = save_ulong (sctx, n->hash);
  if (err) return err;
  stype = n->stype;
  if (stype==NULL)
    {
      if (putc_unlocked (T_NOTHING, f) < 0) { return errno; }
      err = save_string (sctx, NULL);
    }
  else
    {
      if (putc_unlocked ((int)stype->tag, f) < 0) { return errno; }
      err = save_string (sctx, stype->name);
    }
  if (err) return err;
  err = save_int (sctx, n->compute_stype);
  if (err) return err;
  err = save_gc_struct (sctx, (gc_header_t *)n->parents);
  return err;
}

static int node_expr_restore (restore_ctx_t *rctx, node_expr_t *n)
{
  gc_t *gc_ctx = rctx->gc_ctx;
  FILE *f = rctx->f;
  unsigned char tag;
  char *name;
  int c, err;

  GC_START (gc_ctx, 1);
  GC_TOUCH (gc_ctx, n->file = (ovm_var_t *)restore_gc_struct (rctx));
  if (n->file==NULL && errno!=0)
    { err = errno; goto end; }
  GC_UPDATE (gc_ctx, 0, n->file);
  err = restore_uint (rctx, &n->lineno);
  if (err) goto end;
  err = restore_int (rctx, &n->type);
  if (err) goto end;
  err = restore_ulong (rctx, &n->hash);
  if (err) goto end;
  c = getc_unlocked (f);
  if (c==EOF) goto end;
  tag = (unsigned char)c;
  err = restore_string (rctx, &name);
  if (err) goto end;
  if (tag==T_NOTHING)
    {
      if (name!=NULL)
	gc_base_free (name);
      n->stype = NULL;
    }
  else
    {
      if (name==NULL) { err = -2; goto end; }
      n->stype = stype_from_string (gc_ctx, name, 1, tag);
      gc_base_free (name);
    }
  err = restore_int (rctx, &n->compute_stype);
  if (err) goto end;
  if (n->compute_stype<0 || n->compute_stype>=CS_NUM)
    { err = -2; goto end; }
  GC_TOUCH (gc_ctx, n->parents = (node_expr_t *)restore_gc_struct (rctx));
 end:
  GC_END (gc_ctx);
  return err;
}

#define NODE_EXPR_RESTORE(err,rctx,ne) do {				\
  (ne)->file = NULL; (ne)->parents = NULL;				\
  err = node_expr_restore (rctx, ne);					\
  if (err==0)								\
    {									\
      GC_UPDATE (gc_ctx, 0, (ne)->file);				\
      GC_UPDATE (gc_ctx, 1, (ne)->parents);				\
    }									\
  } while (0)

#define NODE_EXPR_COPY(n,ne) do {					\
    (n)->type = (ne)->type;						\
    GC_TOUCH (gc_ctx, (n)->file = (ne)->file);				\
    (n)->lineno = (ne)->lineno;						\
    (n)->hash = (ne)->hash;						\
    (n)->stype = (ne)->stype;						\
    (n)->compute_stype = (ne)->compute_stype;				\
    GC_TOUCH (gc_ctx, (n)->parents = (ne)->parents);			\
  }									\
  while (0)

static void node_expr_term_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  node_expr_term_t *n = (node_expr_term_t *)p;

  node_expr_mark_subfields (gc_ctx, p);
  GC_TOUCH (gc_ctx, n->data);
}

static int node_expr_term_traverse (gc_traverse_ctx_t *gtc, gc_header_t *p,
				    void *data)
{
  node_expr_term_t *n = (node_expr_term_t *)p;
  int err;

  err = node_expr_traverse (gtc, p, data);
  if (err)
    return err;
  return (*gtc->do_subfield) (gtc, (gc_header_t *)n->data, data);
}

static int node_expr_term_save (save_ctx_t *sctx, gc_header_t *p)
{
  node_expr_term_t *n = (node_expr_term_t *)p;
  int err;

  err = node_expr_save (sctx, p);
  if (err) return err;
  err = save_int (sctx, n->res_id);
  if (err) return err;
  err = save_gc_struct (sctx, (gc_header_t *)n->data);
  return err;
}

gc_class_t node_expr_term_class;

static gc_header_t *node_expr_term_restore (restore_ctx_t *rctx)
{
  gc_t *gc_ctx = rctx->gc_ctx;
  node_expr_t ne;
  node_expr_term_t *n;
  int id;
  ovm_var_t *data;
  int err;

  GC_START (gc_ctx, 3);
  n = NULL;
  NODE_EXPR_RESTORE(err,rctx,&ne);
  if (err) { errno = err; goto end; }
  err = restore_int (rctx, &id);
  if (err) { errno = err; goto end; }
  data = (ovm_var_t *)restore_gc_struct (rctx);
  if (data==NULL && errno!=0) goto end;
  GC_UPDATE (gc_ctx, 2, data);
  n = gc_alloc (gc_ctx, sizeof(node_expr_term_t), &node_expr_term_class);
  n->gc.type = T_NODE_CONST;
  NODE_EXPR_COPY(n,&ne);
  n->res_id = id;
  GC_TOUCH (gc_ctx, n->data = data);
 end:
  GC_END (gc_ctx);
  return (gc_header_t *)n;
}

gc_class_t node_expr_term_class = {
  GC_ID('n','t','r','m'),
  node_expr_term_mark_subfields,
  NULL,
  node_expr_term_traverse,
  node_expr_term_save,
  node_expr_term_restore
};


static void node_expr_sym_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  node_expr_symbol_t *sym = (node_expr_symbol_t *)p;
  
  node_expr_mark_subfields (gc_ctx, p);
  GC_TOUCH (gc_ctx, SYM_DEF(sym));
  return;
}

static void node_expr_sym_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  if (SYM_NAME(p)!=NULL)
    gc_base_free (SYM_NAME(p));
}

static int node_expr_sym_traverse (gc_traverse_ctx_t *gtc, gc_header_t *p,
				   void *data)
{
  node_expr_symbol_t *sym = (node_expr_symbol_t *)p;
  int err;

  err = node_expr_traverse (gtc, p, data);
  if (err)
    return err;
  return (*gtc->do_subfield) (gtc, (gc_header_t *)SYM_DEF(sym), data);
}

static int node_expr_sym_save (save_ctx_t *sctx, gc_header_t *p)
{
  node_expr_symbol_t *n = (node_expr_symbol_t *)p;
  int err;

  err = node_expr_save (sctx, p);
  if (err) return err;
  err = save_int (sctx, n->res_id);
  if (err) return err;
  err = save_string (sctx, n->name);
  if (err) return err;
  err = save_gc_struct (sctx, (gc_header_t *)n->def);
  if (err) return err;
  err = save_int (sctx, (int)n->mono);
  return err;
}

static gc_header_t *node_expr_sym_restore (restore_ctx_t *rctx, gc_class_t *class,
					   unsigned char type)
{
  gc_t *gc_ctx = rctx->gc_ctx;
  node_expr_t ne;
  node_expr_symbol_t *n;
  int id;
  char *name;
  node_expr_t *def;
  int mono;
  int err;

  GC_START (gc_ctx, 3);
  n = NULL;
  NODE_EXPR_RESTORE(err,rctx,&ne);
  if (err) { errno = err; goto end; }
  err = restore_int (rctx, &id);
  if (err) { errno = err; goto end; }
  err = restore_string (rctx, &name);
  if (err) { errno = err; goto end; }
  if (name==NULL) { errno = -2; goto end; }
  def = (node_expr_t *)restore_gc_struct (rctx);
  if (def==NULL && errno!=0) { err_freename: gc_base_free(name); goto end; }
  err = restore_int (rctx, &mono);
  if (err) goto err_freename;
  n = gc_alloc (gc_ctx, sizeof(node_expr_symbol_t), class);
  n->gc.type = type;
  NODE_EXPR_COPY(n,&ne);
  n->res_id = id;
  n->name = name;
  GC_TOUCH (gc_ctx, n->def = def);
  n->mono = (monotony)mono;
 end:
  GC_END (gc_ctx);
  return (gc_header_t *)n;
}

gc_class_t node_expr_var_class, node_expr_field_class;

static gc_header_t *node_expr_var_restore (restore_ctx_t *rctx)
{
  return node_expr_sym_restore (rctx, &node_expr_var_class, T_NODE_VARIABLE);
}

static gc_header_t *node_expr_field_restore (restore_ctx_t *rctx)
{
  return node_expr_sym_restore (rctx, &node_expr_field_class, T_NODE_FIELD);
}

gc_class_t node_expr_var_class = {
  GC_ID('n','v','a','r'),
  node_expr_sym_mark_subfields,
  node_expr_sym_finalize,
  node_expr_sym_traverse,
  node_expr_sym_save,
  node_expr_var_restore
};

gc_class_t node_expr_field_class = {
  GC_ID('n','f','l','d'),
  node_expr_sym_mark_subfields,
  node_expr_sym_finalize,
  node_expr_sym_traverse,
  node_expr_sym_save,
  node_expr_field_restore
};

static void node_expr_bin_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  node_expr_bin_t *n = (node_expr_bin_t *)p;

  node_expr_mark_subfields (gc_ctx, p);
  GC_TOUCH (gc_ctx, n->lval);
  GC_TOUCH (gc_ctx, n->rval);
}

static int node_expr_bin_traverse (gc_traverse_ctx_t *gtc, gc_header_t *p,
				   void *data)
{
  node_expr_bin_t *n = (node_expr_bin_t *)p;
  int err;

  err = node_expr_traverse (gtc, p, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *)n->lval, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *)n->rval, data);
  return err;
}

static int node_expr_bin_save (save_ctx_t *sctx, gc_header_t *p)
{
  node_expr_bin_t *n = (node_expr_bin_t *)p;
  int err;

  err = node_expr_save (sctx, p);
  if (err) return err;
  err = save_int (sctx, n->op);
  if (err) return err;
  err = save_gc_struct (sctx, (gc_header_t *)n->lval);
  if (err) return err;
  err = save_gc_struct (sctx, (gc_header_t *)n->rval);
  return err;
}

gc_class_t node_expr_bin_class;

static gc_header_t *node_expr_bin_restore (restore_ctx_t *rctx)
{
  gc_t *gc_ctx = rctx->gc_ctx;
  node_expr_t ne;
  node_expr_bin_t *n;
  int op;
  node_expr_t *lval, *rval;
  int err;

  GC_START (gc_ctx, 4);
  n = NULL;
  NODE_EXPR_RESTORE(err,rctx,&ne);
  if (err) { errno = err; goto end; }
  err = restore_int (rctx, &op);
  if (err) { errno = err; goto end; }
  lval = (node_expr_t *)restore_gc_struct (rctx);
  if (lval==NULL && errno!=0) goto end;
  GC_UPDATE (gc_ctx, 2, lval);
  rval = (node_expr_t *)restore_gc_struct (rctx);
  if (rval==NULL && errno!=0) goto end;
  GC_UPDATE (gc_ctx, 3, rval);
  n = gc_alloc (gc_ctx, sizeof(node_expr_bin_t), &node_expr_bin_class);
  n->gc.type = T_NODE_BINOP;
  NODE_EXPR_COPY(n,&ne);
  n->op = op;
  GC_TOUCH (gc_ctx, n->lval = lval);
  GC_TOUCH (gc_ctx, n->rval = rval);
 end:
  GC_END (gc_ctx);
  return (gc_header_t *)n;
}

gc_class_t node_expr_bin_class = {
  GC_ID('n','b','i','n'),
  node_expr_bin_mark_subfields,
  NULL,
  node_expr_bin_traverse,
  node_expr_bin_save,
  node_expr_bin_restore
};

static void node_expr_mon_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  node_expr_mon_t *n = (node_expr_mon_t *)p;

  node_expr_mark_subfields (gc_ctx, p);
  GC_TOUCH (gc_ctx, n->val);
}

static int node_expr_mon_traverse (gc_traverse_ctx_t *gtc, gc_header_t *p,
				   void *data)
{
  node_expr_mon_t *n = (node_expr_mon_t *)p;
  int err;

  err = node_expr_traverse (gtc, p, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *)n->val, data);
  return err;
}

static int node_expr_mon_save (save_ctx_t *sctx, gc_header_t *p)
{
  node_expr_mon_t *n = (node_expr_mon_t *)p;
  int err;

  err = node_expr_save (sctx, p);
  if (err) return err;
  err = save_int (sctx, n->op);
  if (err) return err;
  err = save_gc_struct (sctx, (gc_header_t *)n->val);
  return err;
}

gc_class_t node_expr_mon_class;

static gc_header_t *node_expr_mon_restore (restore_ctx_t *rctx)
{
  gc_t *gc_ctx = rctx->gc_ctx;
  node_expr_t ne;
  node_expr_mon_t *n;
  int op;
  node_expr_t *val;
  int err;

  GC_START (gc_ctx, 3);
  n = NULL;
  NODE_EXPR_RESTORE(err,rctx,&ne);
  if (err) { errno = err; goto end; }
  err = restore_int (rctx, &op);
  if (err) { errno = err; goto end; }
  val = (node_expr_t *)restore_gc_struct (rctx);
  if (val==NULL && errno!=0) goto end;
  GC_UPDATE (gc_ctx, 2, val);
  n = gc_alloc (gc_ctx, sizeof(node_expr_mon_t), &node_expr_mon_class);
  n->gc.type = T_NODE_MONOP;
  NODE_EXPR_COPY(n,&ne);
  n->op = op;
  GC_TOUCH (gc_ctx, n->val = val);
 end:
  GC_END (gc_ctx);
  return (gc_header_t *)n;
}

gc_class_t node_expr_mon_class = {
  GC_ID('n','m','o','n'),
  node_expr_mon_mark_subfields,
  NULL,
  node_expr_mon_traverse,
  node_expr_mon_save,
  node_expr_mon_restore
};

static void node_expr_call_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  node_expr_call_t *n = (node_expr_call_t *)p;

  node_expr_mark_subfields (gc_ctx, p);
  GC_TOUCH (gc_ctx, n->paramlist);
}

static void node_expr_call_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  if (CALL_SYM(p)!=NULL)
    gc_base_free (CALL_SYM(p));
}

static int node_expr_call_traverse (gc_traverse_ctx_t *gtc, gc_header_t *p,
				    void *data)
{
  node_expr_call_t *n = (node_expr_call_t *)p;
  int err;

  err = node_expr_traverse (gtc, p, data);
  if (err)
    return err;
  return (*gtc->do_subfield) (gtc, (gc_header_t *)n->paramlist, data);
}

static int node_expr_call_save (save_ctx_t *sctx, gc_header_t *p)
{
  node_expr_call_t *n = (node_expr_call_t *)p;
  int err;

  err = node_expr_save (sctx, p);
  if (err) return err;
  err = save_string (sctx, n->symbol);
  if (err) return err;
  err = save_gc_struct (sctx, (gc_header_t *)n->paramlist);
  return err;
}

gc_class_t node_expr_call_class;

static gc_header_t *node_expr_call_restore (restore_ctx_t *rctx)
{
  gc_t *gc_ctx = rctx->gc_ctx;
  node_expr_t ne;
  node_expr_call_t *n;
  char *name;
  issdl_function_t *func;
  node_expr_t *params;
  int err;

  GC_START (gc_ctx, 3);
  n = NULL;
  NODE_EXPR_RESTORE(err,rctx,&ne);
  if (err) { errno = err; goto end; }
  err = restore_string (rctx, &name);
  if (err) { errno = err; goto end; }
  if (name==NULL) { errno = -2; goto end; }
  func = strhash_get(rctx->rule_compiler->functions_hash, name);
  if (func==NULL) { gc_base_free (name); errno = -8; goto end; }
  params = (node_expr_t *)restore_gc_struct (rctx);
  if (params==NULL && errno!=0) { gc_base_free (name); goto end; }
  GC_UPDATE (gc_ctx, 2, params);
  n = gc_alloc (gc_ctx, sizeof(node_expr_call_t), &node_expr_call_class);
  n->gc.type = T_NODE_CALL;
  NODE_EXPR_COPY(n,&ne);
  n->symbol = name;
  n->f = func;
  GC_TOUCH (gc_ctx, n->paramlist = params);
 end:
  GC_END (gc_ctx);
  return (gc_header_t *)n;
}

gc_class_t node_expr_call_class = {
  GC_ID('n','c','l','l'),
  node_expr_call_mark_subfields,
  node_expr_call_finalize,
  node_expr_call_traverse,
  node_expr_call_save,
  node_expr_call_restore
};

static void node_expr_ifstmt_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  node_expr_if_t *n = (node_expr_if_t *)p;

  node_expr_mark_subfields (gc_ctx, p);
  GC_TOUCH (gc_ctx, n->cond);
  GC_TOUCH (gc_ctx, n->then);
  GC_TOUCH (gc_ctx, n->els);
}

static int node_expr_ifstmt_traverse (gc_traverse_ctx_t *gtc,
				      gc_header_t *p,
				      void *data)
{
  node_expr_if_t *n = (node_expr_if_t *)p;
  int err;

  err = node_expr_traverse (gtc, p, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *)n->cond, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *)n->then, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *)n->els, data);
  return err;
}

static int node_expr_ifstmt_save (save_ctx_t *sctx, gc_header_t *p)
{
  node_expr_if_t *n = (node_expr_if_t *)p;
  int err;

  err = node_expr_save (sctx, p);
  if (err) return err;
  err = save_gc_struct (sctx, (gc_header_t *)n->cond);
  if (err) return err;
  err = save_gc_struct (sctx, (gc_header_t *)n->then);
  if (err) return err;
  err = save_gc_struct (sctx, (gc_header_t *)n->els);
  return err;
}

gc_class_t node_expr_ifstmt_class;

static gc_header_t *node_expr_ifstmt_restore (restore_ctx_t *rctx)
{
  gc_t *gc_ctx = rctx->gc_ctx;
  node_expr_t ne;
  node_expr_if_t *n;
  node_expr_t *cond, *then, *els;
  int err;

  GC_START (gc_ctx, 5);
  n = NULL;
  NODE_EXPR_RESTORE(err,rctx,&ne);
  if (err) { errno = err; goto end; }
  cond = (node_expr_t *)restore_gc_struct (rctx);
  if (cond==NULL && errno!=0) goto end;
  if (cond==NULL) { errno = -2; goto end; }
  GC_UPDATE (gc_ctx, 2, cond);
  then = (node_expr_t *)restore_gc_struct (rctx);
  if (then==NULL && errno!=0) goto end;
  GC_UPDATE (gc_ctx, 3, then);
  els = (node_expr_t *)restore_gc_struct (rctx);
  if (els==NULL && errno!=0) goto end;
  GC_UPDATE (gc_ctx, 4, els);
  n = gc_alloc (gc_ctx, sizeof(node_expr_if_t), &node_expr_ifstmt_class);
  n->gc.type = T_NODE_IFSTMT;
  NODE_EXPR_COPY(n,&ne);
  GC_TOUCH (gc_ctx, n->cond = cond);
  GC_TOUCH (gc_ctx, n->then = then);
  GC_TOUCH (gc_ctx, n->els = els);
 end:
  GC_END (gc_ctx);
  return (gc_header_t *)n;
}

gc_class_t node_expr_ifstmt_class = {
  GC_ID('n','i','f','s'),
  node_expr_ifstmt_mark_subfields,
  NULL,
  node_expr_ifstmt_traverse,
  node_expr_ifstmt_save,
  node_expr_ifstmt_restore
};

static void node_expr_regsplit_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  node_expr_regsplit_t *rs = (node_expr_regsplit_t *)p;
  size_t i, nvars;
  node_varlist_t *dv;

  node_expr_mark_subfields (gc_ctx, p);
  GC_TOUCH (gc_ctx, rs->string);
  GC_TOUCH (gc_ctx, rs->split_pat);
  dv = rs->dest_vars;
  if (dv!=NULL)
    for (i=0, nvars=dv->vars_nb; i<nvars; i++)
      GC_TOUCH (gc_ctx, dv->vars[i]);
}

static void node_expr_regsplit_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  node_expr_regsplit_t *rs = (node_expr_regsplit_t *)p;
  node_varlist_t *dv = rs->dest_vars;

  //node_expr_finalize (gc_ctx, p);
  if (dv!=NULL)
    {
      if (dv->vars!=NULL)
	gc_base_free (dv->vars);
      gc_base_free (dv);
    }
}

static int node_expr_regsplit_traverse (gc_traverse_ctx_t *gtc,
					gc_header_t *p, void *data)
{
  node_expr_regsplit_t *rs = (node_expr_regsplit_t *)p;
  subfield_action do_subfield = gtc->do_subfield;
  size_t i, nvars;
  node_varlist_t *dv;
  int err;

  err = node_expr_traverse (gtc, p, data);
  if (err)
    return err;
  err = (*do_subfield) (gtc, (gc_header_t *)rs->string, data);
  if (err)
    return err;
  err = (*do_subfield) (gtc, (gc_header_t *)rs->split_pat, data);
  if (err)
    return err;
  dv = rs->dest_vars;
  if (dv!=NULL)
    for (i=0, nvars=dv->vars_nb; i<nvars; i++)
      {
	err = (*do_subfield) (gtc, (gc_header_t *)dv->vars[i], data);
	if (err)
	  return err;
      }
  return err;
}

static int node_expr_regsplit_save (save_ctx_t *sctx, gc_header_t *p)
{
  node_expr_regsplit_t *n = (node_expr_regsplit_t *)p;
  size_t i, nvars;
  node_varlist_t *dv;
  int err;

  err = node_expr_save (sctx, p);
  if (err) return err;
  err = save_gc_struct (sctx, (gc_header_t *)n->string);
  if (err) return err;
  err = save_gc_struct (sctx, (gc_header_t *)n->split_pat);
  if (err) return err;
  dv = n->dest_vars;
  if (dv==NULL)
    err = save_size_t (sctx, 0);
  else
    {
      nvars = dv->vars_nb;
      err = save_size_t (sctx, nvars);
      if (err) return err;
      for (i=0; i<nvars; i++)
	{
	  err = save_gc_struct (sctx, (gc_header_t *)dv->vars[i]);
	  if (err) return err;
	}
    }
  return err;
}

gc_class_t node_expr_regsplit_class;

static gc_header_t *node_expr_regsplit_restore (restore_ctx_t *rctx)
{
  gc_t *gc_ctx = rctx->gc_ctx;
  node_expr_t ne;
  node_expr_regsplit_t *n;
  node_expr_t *string, *split_pat;
  size_t i, nvars;
  node_varlist_t *dv;
  node_expr_t **vars, *e;
  int err;

  GC_START (gc_ctx, 4);
  n = NULL;
  NODE_EXPR_RESTORE(err,rctx,&ne);
  if (err) { errno = err; goto end; }
  string = (node_expr_t *)restore_gc_struct (rctx);
  if (string==NULL && errno!=0) goto end;
  if (string==NULL) { errno = -2; goto end; }
  GC_UPDATE (gc_ctx, 2, string);
  split_pat = (node_expr_t *)restore_gc_struct (rctx);
  if (split_pat==NULL && errno!=0) goto end;
  if (split_pat==NULL) { errno = -2; goto end; }
  GC_UPDATE (gc_ctx, 3, split_pat);
  n = gc_alloc (gc_ctx, sizeof(node_expr_regsplit_t), &node_expr_regsplit_class);
  n->gc.type = T_NODE_REGSPLIT;
  NODE_EXPR_COPY(n,&ne);
  GC_TOUCH (gc_ctx, n->string = string);
  GC_TOUCH (gc_ctx, n->split_pat = split_pat);
  n->dest_vars = NULL;
  GC_UPDATE (gc_ctx, 2, n);
  err = restore_size_t (rctx, &nvars);
  if (err) { errno = err; n = NULL; goto end; }
  if (nvars!=0)
    {
      dv = gc_base_malloc (gc_ctx, sizeof(node_varlist_t));
      dv->vars_nb = 0;
      dv->size = nvars;
      dv->grow = 8; /* whatever */
      dv->vars = vars = gc_base_malloc (gc_ctx, nvars*sizeof(node_expr_t *));
      n->dest_vars = dv;
      for (i=0; i<nvars; )
	{
	  e = (node_expr_t *)restore_gc_struct (rctx);
	  if (e==NULL && errno!=0) { n = NULL; goto end; }
	  if (e==NULL) { errno = -2; n = NULL; goto end; }
	  GC_TOUCH (gc_ctx, vars[i] = e);
	  dv->vars_nb = ++i;
	}
    }
 end:
  GC_END (gc_ctx);
  return (gc_header_t *)n;
}

gc_class_t node_expr_regsplit_class = {
  GC_ID('r','g','s','p'),
  node_expr_regsplit_mark_subfields,
  node_expr_regsplit_finalize,
  node_expr_regsplit_traverse,
  node_expr_regsplit_save,
  node_expr_regsplit_restore
};

/**
 * Add a new entry in the static data environment.
 * @param ctx Rule compiler context.
 * @param data The new 'static' value.
 **/
static void statics_add(rule_compiler_t *ctx, ovm_var_t *data)
{
  if (ctx->statics_nb == ctx->statics_sz)
    {
      ctx->statics_sz += STATICS_SZ;
      ctx->statics = gc_base_realloc (ctx->gc_ctx,
				      ctx->statics,
				      ctx->statics_sz * sizeof (ovm_var_t *));
    }
  GC_TOUCH (ctx->gc_ctx, ctx->statics[ctx->statics_nb++] = data);
}

/**
 * Add a new entry in the dynamic environment variable name table.
 * @param ctx Rule compiler context.
 * @param var_name The new variable name.
 **/
static void dynamic_add(rule_compiler_t *ctx, char *var_name)
{
  if (ctx->dyn_var_name_nb == ctx->dyn_var_name_sz) {
    ctx->dyn_var_name_sz += STATICS_SZ;
    ctx->dyn_var_name = gc_base_realloc (ctx->gc_ctx,
					 ctx->dyn_var_name,
					 ctx->dyn_var_name_sz * sizeof (char *));
  }
  ctx->dyn_var_name[ctx->dyn_var_name_nb++] = gc_strdup (ctx->gc_ctx, var_name);
}


void compile_rules(orchids_t *ctx)
{
  rulefile_t *rulefile;

  DebugLog(DF_OLC, DS_NOTICE, "*** beginning rule compilation ***\n");

  ctx->rule_compiler->verbose = ctx->verbose;
  build_fields_hash(ctx);
  /* XXX functions hash construction moved to register_lang_function() */
  build_functions_hash(ctx);

  for (rulefile = ctx->rulefile_list; rulefile; rulefile = rulefile->next)
    compile_and_add_rulefile(ctx, rulefile->name);

  gettimeofday(&ctx->compil_time, NULL);

/*   DebugLog(DF_OLC, DS_DEBUG, "Pre-compute reachable init states\n"); */
  /* precomp_rule_init_threads(ctx); */
}


#ifdef ENABLE_PREPROC
static char *
get_preproc_cmd(orchids_t *ctx, const char *filename)
{
  preproc_cmd_t *c;
  size_t filename_len;
  size_t suffix_len;

  filename_len = strlen(filename);

  SLIST_FOREACH(c, &ctx->preproclist, preproclist) {
    suffix_len = strlen(c->suffix);
    if (suffix_len > filename_len)
      continue ;

    if (!strcmp(c->suffix, filename + filename_len - suffix_len))
      return (c->cmd);
  }

  return (ctx->default_preproc_cmd);
}
#endif /* ENABLE_PREPROC */

static size_t list_len (node_expr_t *l)
{
  size_t n;

  for (n=0; l!=NULL; l=BIN_RVAL(l))
    n++;
  return n;
}


static node_expr_t *pop_type_node (gc_t *gc_ctx, struct type_heap_s **rtp, type_t **tp);
static int stype_below (type_t *t, type_t *model);
static type_t *stype_join (type_t *t1, type_t *t2);

static void type_solve (rule_compiler_t *ctx)
{
  node_expr_t *e, *l, *parent;
  type_t *t, *newt;
  int cs;

  GC_START(ctx->gc_ctx, 2);
  /* Compute types by solving a fixed point system */
  while (ctx->type_stack!=NULL)
    {
      e = pop_type_node (ctx->gc_ctx, &ctx->type_stack, &t);
      GC_UPDATE(ctx->gc_ctx, 0, e);
      if (stype_below (t, e->stype))
	continue;
      if (e->stype==&t_any)
	continue; /* no need to propagate a type error */
      /* t should not be equal to &t_any (bad taste) */
      newt = stype_join (t, e->stype);
      if (newt==&t_any && t!=&t_any)
	{
	  if (e->file!=NULL)
	    fprintf (stderr, "%s:", STR(e->file));
	  /* printing STR(e->file) is legal, since it
	     was created NUL-terminated, on purpose */
	  fprintf (stderr, "%u: type error: expresssion cannot have both types %s and %s.\n",
		   e->lineno, e->stype->name, t->name);
	  ctx->nerrors++;
	  continue; /* no need to propagate a type error */
	}
      if (e->stype!=NULL && strcmp(e->stype->name, newt->name)==0)
	continue; /* type did not change: continue */
      e->stype = newt; /* type changed: update e's type, and propagate: */
      l = e->parents;
      GC_UPDATE(ctx->gc_ctx, 1, l);
      //GC_TOUCH(ctx->gc_ctx, e->parents = NULL); /* remove cycles */
      for (; l!=NULL; l = BIN_RVAL(l))
	{
	  parent = BIN_LVAL(l);
	  /*if (--parent->npending_argtypes==0)*/
	  cs = parent->compute_stype;
	  if (cs<0 || cs>=CS_NUM || compute_stype_fun[cs]==NULL)
	    {
	      DebugLog (DF_OLC, DS_ERROR, "unknown compute_stype number %d.\n", cs);
	    }
	  else
	    (*compute_stype_fun[cs]) (ctx, parent);
	}
    }
  GC_END(ctx->gc_ctx);
}

static void type_explore_expr (rule_compiler_t *ctx,
			       node_expr_t *expr)
{
  node_expr_t *l;

  if (expr==NULL)
    return;
  switch (expr->type)
    {
    case NODE_FIELD:
    case NODE_VARIABLE:
    case NODE_CONST:
    case NODE_BREAK:
      break;
    case NODE_ASSOC:
      type_explore_expr (ctx, BIN_RVAL(expr));
      break;
    case NODE_BINOP:
    case NODE_COND:
      type_explore_expr (ctx, BIN_LVAL(expr));
      type_explore_expr (ctx, BIN_RVAL(expr));
      break;
    case NODE_MONOP:
    case NODE_RETURN:
      type_explore_expr (ctx, MON_VAL(expr));
      break;
    case NODE_EVENT:
      type_explore_expr (ctx, BIN_LVAL(expr));
      for (l=BIN_RVAL(expr); l!=NULL; l=BIN_RVAL(l))
	type_explore_expr (ctx, BIN_RVAL(BIN_LVAL(l)));
      break;
    case NODE_DB_PATTERN: 
      type_explore_expr (ctx, BIN_LVAL(expr));
      for (l=BIN_RVAL(expr); l!=NULL; l=BIN_RVAL(l))
	if (!is_logical_variable(BIN_LVAL(l)))
	  type_explore_expr (ctx, BIN_LVAL(l));
      break;
    case NODE_DB_COLLECT:
      /* (NODE_DB_COLLECT returns (actions . collect) pat1 ... patn) */
      /* explore patterns first: */
      for (l=BIN_RVAL(BIN_RVAL(expr)); l!=NULL; l=BIN_RVAL(l))
	type_explore_expr (ctx, BIN_LVAL(l));
      /* explore actions second: */
      for (l=BIN_LVAL(BIN_LVAL(BIN_RVAL(expr))); l!=NULL; l=BIN_RVAL(l))
	type_explore_expr (ctx, BIN_LVAL(l));
      /* then explore collect: */
      type_explore_expr (ctx, BIN_RVAL(BIN_LVAL(BIN_RVAL(expr))));
      /* we don't need to explore returns, which is just a list
	 of 'return' and 'break' statements found inside actions */
      break;
    case NODE_DB_SINGLETON:
      for (l=MON_VAL(expr); l!=NULL; l=BIN_RVAL(l))
	type_explore_expr (ctx, BIN_LVAL(l));
      break;
    case NODE_CALL:
      for (l=CALL_PARAMS(expr); l!=NULL; l=BIN_RVAL(l))
	type_explore_expr (ctx, BIN_LVAL(l));
      break;
    case NODE_IFSTMT:
      type_explore_expr (ctx, IF_COND(expr));
      type_explore_expr (ctx, IF_THEN(expr));
      type_explore_expr (ctx, IF_ELSE(expr));
      break;
    case NODE_CONS:
      type_explore_expr (ctx, BIN_LVAL(expr));
      type_explore_expr (ctx, BIN_RVAL(expr));
      break;
    case NODE_REGSPLIT:
      type_explore_expr (ctx, REGSPLIT_STRING(expr));
      break;
    case NODE_UNKNOWN:
    default:
      DebugLog(DF_OLC, DS_ERROR, "type_explore_expr on type %i.\n",
	       expr->type);
      exit(EXIT_FAILURE);
      break;
    }
  {
    int cs = expr->compute_stype;
    
    if (cs<0 || cs>=CS_NUM || compute_stype_fun[cs]==NULL)
      {
	DebugLog (DF_OLC, DS_ERROR, "unknown compute_stype number %d.\n", cs);
      }
    else
      (*compute_stype_fun[cs]) (ctx, expr);
  }
}

#define type_explore_stmt(ctx,expr) type_explore_expr(ctx,expr)

static void type_explore_actions (rule_compiler_t *ctx,
				  node_expr_t *actionlist)
{
  node_expr_t *l;

  for (l=actionlist; l!=NULL; l=BIN_RVAL(l))
    type_explore_stmt (ctx, BIN_LVAL(l));
}

static node_state_t *trans_dest_state (rule_compiler_t *ctx, node_trans_t *trans)
{
  node_state_t *s;

  s = trans->sub_state_dest;
  if (s==NULL)
    {
      s = strhash_get(ctx->statenames_hash, trans->dest);
      if (s==NULL && !(trans->an_flags & AN_NONEXISTENT_TARGET_ALREADY_SAID))
	{
	  trans->an_flags |= AN_NONEXISTENT_TARGET_ALREADY_SAID;
	  if (trans->file!=NULL)
	    fprintf (stderr, "%s:", trans->file);
	  fprintf (stderr, "%u: error: unresolved state name '%s'.\n",
		   trans->lineno, trans->dest);
	  ctx->nerrors++;
	}
    }
  return s;
}

static void type_explore_state (rule_compiler_t *ctx,
				node_state_t *state);

static void type_explore_transition (rule_compiler_t *ctx,
				     node_trans_t *trans)
{
  node_state_t *s;

  if (trans->cond!=NULL)
    type_explore_expr (ctx, trans->cond);
  s = trans_dest_state (ctx, trans);
  if (s!=NULL)
    type_explore_state (ctx, s);
}

static void type_explore_transitions (rule_compiler_t *ctx,
				      node_expr_t *translist)
{
  node_expr_t *l;

  for (l=translist; l!=NULL; l=BIN_RVAL(l))
    type_explore_transition (ctx, (node_trans_t *)BIN_LVAL(l));
}

static void type_explore_state (rule_compiler_t *ctx,
				node_state_t *state)
{
  if (state->an_flags & AN_EXPLORED)
    return; /* already explored */
  state->an_flags |= AN_EXPLORED;
  type_explore_actions (ctx, state->actionlist);
  type_explore_transitions (ctx, state->translist);
}

static void type_explore_rule (rule_compiler_t *ctx,
			       node_rule_t *node_rule)
{
  node_expr_t *states;

  type_explore_state (ctx, node_rule->init);
  for (states=node_rule->statelist; states!=NULL; states=BIN_RVAL(states))
    {
      type_explore_state (ctx, (node_state_t *)BIN_LVAL(states));
    }
  /* we don't check the type of synchronization variables here;
     this will be done in get_and_check_syncvar(), in a later phase */
}

static void type_check (rule_compiler_t *ctx, node_rule_t *node_rule)
{
  type_explore_rule (ctx, node_rule);
  type_solve (ctx);
}

int node_expr_equal (node_expr_t *e1, node_expr_t *e2)
{
  if (e1==NULL)
    {
      if (e2==NULL)
	return 1;
      else return 0;
    }
  else if (e2==NULL)
    return 0;
  if (e1->hash!=e2->hash)
    return 0;
  if (e1->type!=e2->type)
    return 0;
  switch (e1->type)
    {
    case NODE_FIELD:
    case NODE_VARIABLE:
      return SYM_RES_ID(e1)==SYM_RES_ID(e2);
    case NODE_CONST:
      return issdl_cmp (TERM_DATA(e1), TERM_DATA(e2), CMP_LEQ_MASK | CMP_GEQ_MASK)==
	(CMP_LEQ_MASK | CMP_GEQ_MASK);
    case NODE_BINOP:
    case NODE_COND:
    case NODE_CONS:
    case NODE_EVENT: /* compare events literally (not even modulo reordering of fields) */
    case NODE_DB_PATTERN: /* compare patterns literally (not even modulo alpha-renaming) */
    case NODE_DB_COLLECT:
      return node_expr_equal (BIN_LVAL(e1), BIN_LVAL(e2))
	&& node_expr_equal (BIN_RVAL(e1), BIN_RVAL(e2));
    case NODE_MONOP:
    case NODE_DB_SINGLETON:
      return node_expr_equal (MON_VAL(e1), MON_VAL(e2));
    case NODE_CALL:
      return CALL_RES_ID(e1)==CALL_RES_ID(e2)
	&& node_expr_equal (CALL_PARAMS(e1), CALL_PARAMS(e2));
    case NODE_IFSTMT:
      return node_expr_equal (IF_COND(e1), IF_COND(e2))
	&& node_expr_equal (IF_THEN(e1), IF_THEN(e2))
	&& node_expr_equal (IF_ELSE(e1), IF_ELSE(e2));
    case NODE_REGSPLIT: /* should not happen */
      if (!node_expr_equal (REGSPLIT_STRING(e1), REGSPLIT_STRING(e2)))
	return 0;
      if (!node_expr_equal (REGSPLIT_PAT(e1), REGSPLIT_PAT(e2)))
	return 0;
      if (REGSPLIT_DEST_VARS(e1)->vars_nb!=REGSPLIT_DEST_VARS(e2)->vars_nb)
	return 0;
      {
	size_t i;
	
	for (i=0; i<REGSPLIT_DEST_VARS(e1)->vars_nb; i++)
	  if (!node_expr_equal (REGSPLIT_DEST_VARS(e1)->vars[i], REGSPLIT_DEST_VARS(e2)->vars[i]))
	    return 0;
      }
      return 1;
    case NODE_ASSOC: /* should not happen */
      return node_expr_equal (BIN_LVAL(e1), BIN_LVAL(e2))
	&& node_expr_equal (BIN_RVAL(e1), BIN_RVAL(e2));
    case NODE_BREAK: /* should not happen */
      return 1;
    case NODE_RETURN: /* should not happen */
      return node_expr_equal (MON_VAL(e1), MON_VAL(e2));
    case NODE_UNKNOWN:
    default:
      return 0;
    }
}

/* Sets of variable numbers are implemented as sorted lists
   of integers.  This is not optimal, but is easy to implement
   and fares OK on small numbers of variables.
*/

struct varset_s {
  gc_header_t gc;
  int n;
  struct varset_s *next;
};

static void varset_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  varset_t *vs = (varset_t *)p;

  GC_TOUCH (gc_ctx, vs->next);
}

static int varset_traverse (gc_traverse_ctx_t *gtc, gc_header_t *p,
			    void *data)
{
  varset_t *vs = (varset_t *)p;

  return (*gtc->do_subfield) (gtc, (gc_header_t *)vs->next, data);
}

static int varset_save (save_ctx_t *sctx, gc_header_t *p)
{
  varset_t *vs = (varset_t *)p;
  varset_t *v;
  int err;
  size_t n;

  for (v=vs, n=0; v!=NULL; v=v->next)
    n++;
  err = save_size_t (sctx, n);
  for (v=vs; v!=NULL; v=v->next)
    {
      err = save_int (sctx, v->n);
      if (err) return err;
    }
  return 0;
}

gc_class_t varset_class;

static gc_header_t *varset_restore (restore_ctx_t *rctx)
{
  gc_t *gc_ctx = rctx->gc_ctx;
  varset_t *vs, *one;
  size_t i, n;
  int var;
  int err;

  GC_START (gc_ctx, 2);
  vs = NULL;
  err = restore_size_t (rctx, &n);
  if (err) { errno = err; goto end; }
  for (i=0; i<n; i++)
    {
      err = restore_int (rctx, &var);
      if (err) { errno = err; goto end; }
      one = vs_one_element (gc_ctx, var);
      GC_UPDATE (gc_ctx, 1, one);
      vs = vs_union (gc_ctx, vs, one);
      GC_UPDATE (gc_ctx, 0, vs);
    }
 end:
  GC_END (gc_ctx);
  return (gc_header_t *)vs;
}

gc_class_t varset_class = {
  GC_ID('v','s','e','t'),
  varset_mark_subfields,
  NULL,
  varset_traverse,
  varset_save,
  varset_restore
};

varset_t *vs_one_element (gc_t *gc_ctx, int n)
{
  varset_t *vs;

  //gc_check(gc_ctx);
  vs = gc_alloc (gc_ctx, sizeof(varset_t), &varset_class);
  vs->gc.type = T_VARSET;
  vs->n = n;
  vs->next = NULL;
  //gc_check(gc_ctx);
  return vs;
}

varset_t *vs_union (gc_t *gc_ctx, varset_t *vs1, varset_t *vs2)
{
  varset_t *vs;

  if (vs1==NULL)
    return vs2;
  if (vs2==NULL)
    return vs1;
  GC_START(gc_ctx, 1);
  if (vs1->n < vs2->n)
    {
      vs = vs_union (gc_ctx, vs1->next, vs2);
      GC_UPDATE (gc_ctx, 0, vs);
      vs = gc_alloc (gc_ctx, sizeof(varset_t), &varset_class);
      vs->gc.type = T_VARSET;
      vs->n = vs1->n;
      vs->next = (varset_t *)GC_LOOKUP(0);
    }
  else if (vs1->n > vs2->n)
    {
      vs = vs_union (gc_ctx, vs1, vs2->next);
      GC_UPDATE (gc_ctx, 0, vs);
      vs = gc_alloc (gc_ctx, sizeof(varset_t), &varset_class);
      vs->gc.type = T_VARSET;
      vs->n = vs2->n;
      vs->next = (varset_t *)GC_LOOKUP(0);
    }
  else
    {
      vs = vs_union (gc_ctx, vs1->next, vs2->next);
      GC_UPDATE (gc_ctx, 0, vs);
      vs = gc_alloc (gc_ctx, sizeof(varset_t), &varset_class);
      vs->gc.type = T_VARSET;
      vs->n = vs1->n;
      vs->next = (varset_t *)GC_LOOKUP(0);
    }
  GC_END(gc_ctx);
  return vs;
}

int vs_in (int varid, varset_t *vs)
{
  for (; vs!=NULL; vs=vs->next)
    if (vs->n == varid)
      return 1;
    else if (vs->n > varid)
      break;
  return 0;
}

varset_t *vs_inter (gc_t *gc_ctx, varset_t *vs1, varset_t *vs2)
{
  varset_t *vs;

 again:
  if (vs1==NULL)
    return NULL;
  if (vs2==NULL)
    return NULL;
  if (vs1->n < vs2->n)
    {
      vs1 = vs1->next;
      goto again;
    }
  if (vs1->n > vs2->n)
    {
      vs2 = vs2->next;
      goto again;
    }
  vs = vs_inter (gc_ctx, vs1->next, vs2->next);
  GC_START(gc_ctx, 1);
  GC_UPDATE (gc_ctx, 0, vs);
  vs = gc_alloc (gc_ctx, sizeof(varset_t), &varset_class);
  vs->gc.type = T_VARSET;
  vs->n = vs1->n;
  vs->next = (varset_t *)GC_LOOKUP(0);
  GC_END(gc_ctx);
  return vs;
}

varset_t *vs_diff (gc_t *gc_ctx, varset_t *vs1, varset_t *vs2)
{
  varset_t *vs;

 again:
  if (vs1==NULL)
    return NULL;
  if (vs2==NULL)
    return vs1;
  if (vs1->n > vs2->n)
    {
      vs2 = vs2->next;
      goto again;
    }
  if (vs1->n < vs2->n)
    {
      vs = vs_diff (gc_ctx, vs1->next, vs2);
      GC_START(gc_ctx, 1);
      GC_UPDATE (gc_ctx, 0, vs);
      vs = gc_alloc (gc_ctx, sizeof(varset_t), &varset_class);
      vs->gc.type = T_VARSET;
      vs->n = vs1->n;
      vs->next = (varset_t *)GC_LOOKUP(0);
      GC_END(gc_ctx);
    }
  else
    {
      vs1 = vs1->next;
      vs2 = vs2->next;
      goto again;
    }
  return vs;
}

int vs_subset (gc_t *gc_ctx, varset_t *vs1, varset_t *vs2)
{
 again:
  if (vs1==NULL)
    return 1;
 again2:
  if (vs2==NULL)
    return 0;
  if (vs1->n > vs2->n)
    {
      vs2 = vs2->next;
      goto again2;
    }
  if (vs1->n < vs2->n)
    return 0;
  vs1 = vs1->next;
  vs2 = vs2->next;
  goto again;
}

int vs_sweep (varset_t *vs, int (*p) (int var, void *data), void *data)
{
  while (vs!=NULL)
    {
      if ((*p) (vs->n, data))
	return 1;
      vs = vs->next;
    }
  return 0;
}

static varset_t *varlist_varset (gc_t *gc_ctx, size_t start, size_t end,
				 node_expr_t **vars)
{ /* convert a list to a set of variables; this is just merge sort */
  size_t mid;
  varset_t *vs, *vs1, *vs2;

  if (end<=start)
    return VS_EMPTY;
  if (end==start+1)
    return vs_one_element (gc_ctx, SYM_RES_ID(vars[start]));
  mid = (start+end) >> 1;
  GC_START(gc_ctx, 2);
  vs1 = varlist_varset (gc_ctx, start, mid, vars);
  GC_UPDATE(gc_ctx, 0, vs1);
  vs2 = varlist_varset (gc_ctx, mid, end, vars);
  GC_UPDATE(gc_ctx, 1, vs2);
  vs = vs_union (gc_ctx, vs1, vs2);
  GC_END(gc_ctx);
  return vs;
}

varset_t *vs_bound_vars_tuple (gc_t *gc_ctx, node_expr_t *tuple)
{
  node_expr_t *l;
  varset_t *vs;
  size_t nargs;
  node_expr_t **vars;

  nargs = 0;
  //  gc_check (gc_ctx);
  for (l=tuple; l!=NULL; l=BIN_RVAL(l))
    nargs++;
  vars = gc_base_malloc (gc_ctx, nargs*sizeof(node_expr_t *));
  nargs = 0;
  for (l=tuple; l!=NULL; l=BIN_RVAL(l))
    {
      if (is_logical_variable(BIN_LVAL(l)))
	vars[nargs++] = BIN_LVAL(l);
    }
  vs = varlist_varset (gc_ctx, 0, nargs, vars);
  gc_base_free (vars);
  return vs;
}

varset_t *vs_bound_vars_db_pattern (gc_t *gc_ctx, node_expr_t *patterns)
{
  varset_t *vs, *tuplevs;

  GC_START(gc_ctx, 1);
  vs = VS_EMPTY;
  for (; patterns!=NULL; patterns = BIN_RVAL(patterns))
    {
      tuplevs = vs_bound_vars_tuple (gc_ctx, BIN_RVAL(BIN_LVAL(patterns)));
      vs = vs_union (gc_ctx, vs, tuplevs);
      GC_UPDATE (gc_ctx, 0, vs);
    }
  GC_END(gc_ctx);
  return vs;
}

node_expr_vars_t node_expr_vars (gc_t *gc_ctx, node_expr_t *e)
{
  node_expr_vars_t res, newres;

  res.mayread = VS_EMPTY;
  res.mustset = VS_EMPTY;
  GC_START(gc_ctx, 4);
 again:
  //gc_check(gc_ctx);
  if (e==NULL)
    ;
  else switch (e->type)
    {
    case NODE_FIELD:
    case NODE_CONST:
    case NODE_BREAK:
      break;
    case NODE_VARIABLE:
      res.mayread = vs_one_element (gc_ctx, SYM_RES_ID(e));
      break;
    case NODE_CALL:
      /* For NODE_CALL, NODE_BINOP, and general sequence evaluation, we have:
	 mayread(e1; e2) = mayread(e1) U (mayread(e2) - mustset(e1))
	 mustset(e1; e2) = mustset(e1) U mustset(e2)
      */
      {
	node_expr_t *l;

	GC_UPDATE(gc_ctx, 0, res.mayread);
	GC_UPDATE(gc_ctx, 1, res.mustset);
	for (l=CALL_PARAMS(e); l!=NULL; l=BIN_RVAL(l))
	  {
	    /* Given that *mayread==mayread(e1) and *mustset=mustset(e1),
	       where e1 is sequence of all previous parameters,
	       and letting e2 be the current parameter BIN_LVAL(l),
	    */
	    newres = node_expr_vars (gc_ctx, BIN_LVAL(l)); /* e2 */
	    GC_UPDATE(gc_ctx, 2, newres.mayread);
	    GC_UPDATE(gc_ctx, 3, newres.mustset);
	    newres.mayread = vs_diff (gc_ctx, newres.mayread, res.mustset);
	    GC_UPDATE(gc_ctx, 2, newres.mayread);
	    /* now mayread(e2) - mustset(e1) */
	    res.mayread = vs_union (gc_ctx, res.mayread, newres.mayread);
	    GC_UPDATE(gc_ctx, 0, res.mayread);
	    /* now res.mayread == mayread(e1) U (mayread(e2) - mustset(e1)) */
	    res.mustset = vs_union (gc_ctx, res.mustset, newres.mustset);
	    GC_UPDATE(gc_ctx, 1, res.mustset);
	    /* and *mustset == mustset(e1) U mustset(e2) */
	  }
	break;
      }
    case NODE_RETURN:
      res = node_expr_vars (gc_ctx, MON_VAL(e));
      break;
    case NODE_BINOP:
    case NODE_CONS:
    case NODE_EVENT:
      /* e1 == BIN_LVAL(e), e2 == BIN_RVAL(e) */
      res = node_expr_vars (gc_ctx, BIN_LVAL(e));
      GC_UPDATE(gc_ctx, 0, res.mayread);
      GC_UPDATE(gc_ctx, 1, res.mustset);
      newres = node_expr_vars (gc_ctx, BIN_RVAL(e));
      GC_UPDATE(gc_ctx, 2, newres.mayread);
      GC_UPDATE(gc_ctx, 3, newres.mustset);
      newres.mayread = vs_diff (gc_ctx, newres.mayread, res.mustset);
      GC_UPDATE(gc_ctx, 2, newres.mayread);
      /* now mayread(e2) - mustset(e1) */
      res.mayread = vs_union (gc_ctx, res.mayread, newres.mayread);
      GC_UPDATE(gc_ctx, 0, res.mayread);
      /* now res.mayread == mayread(e1) U (mayread(e2) - mustset(e1)) */
      res.mustset = vs_union (gc_ctx, res.mustset, newres.mustset);
      /* and *mustset == mustset(e1) U mustset(e2) */
      break;
    case NODE_DB_PATTERN:
      /* e is db_pattern(db, pat1, ..., patn)
	 mayread(e) = mayread(db) U union {mayread(pati) | pati not a logical variable}
	 mustset(e) = mustset(db) U union {mustset(pati) | pati not a logical variable}
	              U {pati | pati a logical variable}
       */
      res = node_expr_vars (gc_ctx, BIN_LVAL(e));
      GC_UPDATE(gc_ctx, 0, res.mayread);
      GC_UPDATE(gc_ctx, 1, res.mustset);
      {
	node_expr_t *l;

	for (l=BIN_RVAL(e); l!=NULL; l=BIN_RVAL(l))
	  {
	    if (is_logical_variable(BIN_LVAL(l)))
	      {
		newres.mustset = vs_one_element (gc_ctx, SYM_RES_ID(BIN_LVAL(l)));
		GC_UPDATE(gc_ctx, 3, newres.mustset);
		res.mustset = vs_union (gc_ctx, res.mustset, newres.mustset);
		GC_UPDATE(gc_ctx, 1, res.mustset);
	      }
	    else
	      {
		newres = node_expr_vars (gc_ctx, BIN_LVAL(l));
		GC_UPDATE(gc_ctx, 2, newres.mayread);
		GC_UPDATE(gc_ctx, 3, newres.mustset);
		res.mayread = vs_union (gc_ctx, res.mayread, newres.mayread);
		GC_UPDATE(gc_ctx, 0, res.mayread);
		res.mustset = vs_union (gc_ctx, res.mustset, newres.mustset);
		GC_UPDATE(gc_ctx, 1, res.mustset);
	      }
	  }
      }
      break;
    case NODE_DB_COLLECT:
      /* e is db_collect(returns, (actions . expr), pat1, pat2, ..., patn)
	 mayread(e) = (mayread(actions.expr) - quantified_vars(pat1, ..., patn))
	              U union {mayread (pati) | i=1...n}
	 mustset(e) = union {mustset (pati) | i=1...n}
	 Note that mustset(e) does not contain mustset(actions.expr),
	 since the comprehension may be over an empty domain, in which case
	 actions.expr will not be run.
       */
      {
	node_expr_t *l;

	e = BIN_RVAL(e); /* skip 'returns' part */
	res = node_expr_vars (gc_ctx, BIN_LVAL(e));
	GC_UPDATE(gc_ctx, 0, res.mayread);
	/* and forget about newres.mustset */
	res.mustset = VS_EMPTY;
	newres.mayread = vs_bound_vars_db_pattern (gc_ctx, BIN_RVAL(e));
	GC_UPDATE(gc_ctx, 2, newres.mayread);
	res.mayread = vs_diff (gc_ctx, res.mayread, newres.mayread);
	GC_UPDATE(gc_ctx, 0, res.mayread);
	for (l=BIN_RVAL(e); l!=NULL; l=BIN_RVAL(l))
	  {
	    newres = node_expr_vars (gc_ctx, BIN_LVAL(l));
	    GC_UPDATE(gc_ctx, 2, newres.mayread);
	    GC_UPDATE(gc_ctx, 3, newres.mustset);
	    res.mayread = vs_union (gc_ctx, res.mayread, newres.mayread);
	    GC_UPDATE(gc_ctx, 0, res.mayread);
	    res.mustset = vs_union (gc_ctx, res.mustset, newres.mustset);
	    GC_UPDATE(gc_ctx, 1, res.mustset);
	  }
      }
      break;
    case NODE_DB_SINGLETON:
      /* e is db_singleton(e1, ..., em)
	 mayread(e) = union {mayread (ei) | i=1...m}
	 mustset(e) = union {mustset (ei) | i=1...m}
       */
      {
	node_expr_t *l;

	for (l=MON_VAL(e); l!=NULL; l=BIN_RVAL(l))
	  {
	    newres = node_expr_vars (gc_ctx, BIN_LVAL(l));
	    GC_UPDATE(gc_ctx, 2, newres.mayread);
	    GC_UPDATE(gc_ctx, 3, newres.mustset);
	    res.mayread = vs_union (gc_ctx, res.mayread, newres.mayread);
	    GC_UPDATE(gc_ctx, 0, res.mayread);
	    res.mustset = vs_union (gc_ctx, res.mustset, newres.mustset);
	    GC_UPDATE(gc_ctx, 1, res.mustset);
	  }
      }
      break;
    case NODE_COND:
      /* Here there is a difference: in e1 && e2, or e1 || e2 for example,
	 e2 may fail to be executed.  So
	 mayread(e1 cond e2) = mayread(e1) U (mayread(e2) - mustset(e1))
	 but mustset(e1; e2) = mustset(e1) only
      */
      /* e1 == BIN_LVAL(e), e2 == BIN_RVAL(e) */
      res = node_expr_vars (gc_ctx, BIN_LVAL(e));
      GC_UPDATE(gc_ctx, 0, res.mayread);
      GC_UPDATE(gc_ctx, 1, res.mustset);
      newres = node_expr_vars (gc_ctx, BIN_RVAL(e));
      GC_UPDATE(gc_ctx, 2, newres.mayread);
      GC_UPDATE(gc_ctx, 3, newres.mustset);
      newres.mayread = vs_diff (gc_ctx, newres.mayread, res.mustset);
      GC_UPDATE(gc_ctx, 2, newres.mayread);
      /* now mayread(e2) - mustset(e1) */
      res.mayread = vs_union (gc_ctx, res.mayread, newres.mayread);
      /* now res.mayread == mayread(e1) U (mayread(e2) - mustset(e1)) */
      break;
    case NODE_MONOP:
      e = MON_VAL(e);
      goto again;
    case NODE_REGSPLIT:
      /* split "regex" /x1, ..., xn/ e1  (former syntax: /e1/"regex"/x1/x2/.../xn)
	 where e1 == REGSPLIT_STRING(e),
	 "regex" == REGSPLIT_PAT(e),
	 [x1, x2, ..., xn] == REGSPLIT_DEST_VARS(e)
	 Its mayread is mayread(e1),
	 its mustset is mustset(e1) union {x1, x2, ..., xn}.
      */
      res = node_expr_vars (gc_ctx, REGSPLIT_STRING(e));
      GC_UPDATE(gc_ctx, 0, res.mayread);
      GC_UPDATE(gc_ctx, 1, res.mustset);
      if (REGSPLIT_DEST_VARS(e)!=NULL)
	{
	  newres.mustset = varlist_varset(gc_ctx, 0,
					  REGSPLIT_DEST_VARS(e)->vars_nb,
					  REGSPLIT_DEST_VARS(e)->vars);
	  GC_UPDATE(gc_ctx, 2, newres.mustset);
	  res.mustset = vs_union (gc_ctx, res.mustset, newres.mustset);
	}
      break;
    case NODE_IFSTMT:
      /*
	e == if (e1) e2 else e3
	mayread(e) = mayread(e1) union (mayread(e2) - mustset(e1))
	                         union (mayread(e3) - mustset(e1))
                   = mayread(e1) union ((mayread(e2) union mayread(e3)
                                        - mustset(e1))
	mustset(e) = mustset(e1) union (mustset(e2) inter mustset(e3))
       */
      res = node_expr_vars (gc_ctx, IF_THEN(e));
      GC_UPDATE(gc_ctx, 0, res.mayread);
      GC_UPDATE(gc_ctx, 1, res.mustset);
      newres = node_expr_vars (gc_ctx, IF_ELSE(e));
      GC_UPDATE(gc_ctx, 2, newres.mayread);
      GC_UPDATE(gc_ctx, 3, newres.mustset);
      newres.mayread = vs_union (gc_ctx, res.mayread, newres.mayread);
      GC_UPDATE(gc_ctx, 2, newres.mayread);
      /* now newres.mayread == mayread(e2) union mayread(e3) */
      newres.mustset = vs_inter (gc_ctx, res.mustset, newres.mustset);
      GC_UPDATE(gc_ctx, 3, newres.mustset);
      /* now newres.mustset == mustset(e2) inter mustset(e3) */
      res = node_expr_vars (gc_ctx, IF_COND(e));
      GC_UPDATE(gc_ctx, 0, res.mayread);
      GC_UPDATE(gc_ctx, 1, res.mustset);
      newres.mayread = vs_diff (gc_ctx, newres.mayread, res.mustset);
      GC_UPDATE(gc_ctx, 2, newres.mayread);
      /* now newres.mayread ==
	 (mayread(e2) union mayread(e3))
	 - mustset(e1) */
      res.mayread = vs_union (gc_ctx, res.mayread, newres.mayread);
      GC_UPDATE(gc_ctx, 0, res.mayread);
      /* now res.mayread ==
	 mayread(e1) union
	 ((mayread(e2) union mayread(e3))
	 - mustset(e1)) */
      res.mustset = vs_union (gc_ctx, res.mustset, newres.mustset);
      /* and res.mustset == mustset(e1) union
	 (mustset(e2) inter mustset(e3)) */
      break;
    case NODE_ASSOC:
      /* e is x = e2
	 mayread(e) = mayread(e2)
	 mustset(e) = mustset(e2) U {x}
	 where x == BIN_LVAL(e), e2 == BIN_RVAL(e)
       */
      res = node_expr_vars (gc_ctx, BIN_RVAL(e));
      GC_UPDATE(gc_ctx, 0, res.mayread);
      GC_UPDATE(gc_ctx, 1, res.mustset);
      newres.mustset = vs_one_element (gc_ctx, SYM_RES_ID(BIN_LVAL(e)));
      GC_UPDATE(gc_ctx, 2, newres.mustset);
      res.mustset = vs_union (gc_ctx, res.mustset, newres.mustset);
      break;
    default:
      DebugLog(DF_OLC, DS_FATAL,
	       "unrecognized node type %d\n", e->type);
      exit(EXIT_FAILURE);
      break;
    }
  GC_END(gc_ctx);
  return res;
}

typedef struct an_worklist_s {
  struct an_worklist_s *next;
  node_state_t *state;
} an_worklist_t;

static node_state_t *find_state_by_name (node_rule_t *rule, char *name)
{
  node_expr_t *l;
  node_state_t *state;

  state = (node_state_t *)rule->init;
  if (state->name!=NULL && strcmp(state->name, name)==0)
    return state;
  for (l=rule->statelist; l!=NULL; l=BIN_RVAL(l))
    {
      state = (node_state_t *)BIN_LVAL(l);
      if (state->name!=NULL && strcmp(state->name, name)==0)
	return state;
    }
  return NULL;
}

static void check_state_mustset (rule_compiler_t *ctx, node_rule_t *rule,
				 node_state_t *state)
{
  gc_t *gc_ctx = ctx->gc_ctx;
  varset_t *vs;
  node_expr_t *l;
  node_trans_t *trans;
  node_expr_vars_t action_res, cond_res;
  char *file;
  unsigned int line;

  GC_START(gc_ctx, 4);
  action_res = node_expr_vars (gc_ctx, state->actionlist);
  GC_UPDATE(gc_ctx, 0, action_res.mayread);
  GC_UPDATE(gc_ctx, 1, action_res.mustset);
  vs = vs_diff (gc_ctx, action_res.mayread, state->mustset);
  file = rule->filename;
  line = rule->line;
  if (state->actionlist!=NULL)
    {
      file = STR(BIN_LVAL(state->actionlist)->file);
      /* STR() is NUL-terminated by construction */
      line = BIN_LVAL(state->actionlist)->lineno;
    }
  for (; vs!=VS_EMPTY; vs = vs->next)
    {
      if (file!=NULL)
	fprintf (stderr, "%s:", file);
      fprintf (stderr, "%u: error: variable %s may be used uninitialized in action list",
	       line, ctx->dyn_var_name[vs->n]);
      if (state->name!=NULL)
	fprintf (stderr, " of state '%s'", state->name);
      fprintf (stderr, ".\n");
      ctx->nerrors++;
    }
  action_res.mustset = vs_union (gc_ctx, state->mustset, action_res.mustset);
  GC_UPDATE(gc_ctx, 1, action_res.mustset);
  /* now == state->mustset union mustset(actionlist) */
  for (l=state->translist; l!=NULL; l=BIN_RVAL(l))
    {
      trans = (node_trans_t *)BIN_LVAL(l);
      if (trans==NULL)
	continue;
      cond_res = node_expr_vars (gc_ctx, trans->cond);
      GC_UPDATE(gc_ctx, 2, cond_res.mayread);
      GC_UPDATE(gc_ctx, 3, cond_res.mustset);
      vs = vs_diff (gc_ctx, cond_res.mayread, action_res.mustset);
      file = trans->file;
      line = trans->lineno;
      for (; vs!=VS_EMPTY; vs = vs->next)
	{
	  if (file!=NULL)
	    fprintf (stderr, "%s:", file);
	  fprintf (stderr, "%u: error: variable %s may be used uninitialized in expect clause.\n",
		   line, ctx->dyn_var_name[vs->n]);
	  ctx->nerrors++;
	}
      if (trans->sub_state_dest!=NULL)
	check_state_mustset (ctx, rule, trans->sub_state_dest);
    }
  GC_END(gc_ctx);
}

static void check_rule_state_mustset (rule_compiler_t *ctx, node_rule_t *rule,
				      node_state_t *state)
{
  char *file;
  unsigned int line;

  file = state->file;
  line = (unsigned int)state->line;
  if ((state->an_flags & AN_MUSTSET_REACHABLE)==0)
    {
      if (file!=NULL)
	fprintf (stderr, "%s:", file);
      if (state->name!=NULL)
	fprintf (stderr, "%u: warning: state %s is unreachable, hence useless.\n",
		 line, state->name);
      else fprintf (stderr, "%u: warning: inner state is unreachable (I am confused).\n", line);
      /* state->name cannot be NULL, normally, since if an inner state is unreachable,
	 then its encasing state would be, too. */
    }
  else check_state_mustset (ctx, rule, state);
}

static void check_rule_mustset (rule_compiler_t *ctx, node_rule_t *rule)
{
  node_expr_t *l;
  node_state_t *state;

  state = (node_state_t *)rule->init;
  check_rule_state_mustset (ctx, rule, state);
  for (l=rule->statelist; l!=NULL; l=BIN_RVAL(l))
    {
      state = (node_state_t *)BIN_LVAL(l);
      check_rule_state_mustset (ctx, rule, state);
    }
}

static void check_var_usage (rule_compiler_t *ctx, node_rule_t *rule)
{
  gc_t *gc_ctx = ctx->gc_ctx;
  an_worklist_t *work, *next;
  node_state_t *state, *target;
  node_expr_t *l;
  node_trans_t *trans;
  node_expr_vars_t action_res, cond_res;

  GC_START(gc_ctx, 4);
  work = gc_base_malloc (ctx->gc_ctx, sizeof(an_worklist_t));
  work->next = NULL;
  work->state = rule->init;
  rule->init->an_flags |= AN_MUSTSET_REACHABLE; /* and its mustset
						   is already set to
						   VS_EMPTY, fine */
  /* AN_MUSTSET_REACHABLE really means that the state's mustset
     field is (some upper approximation) of the set of variables
     that are guaranteed to be set at the beginning of the set.
     If AN_MUSTSET_REACHABLE is not set, then this set must be
     conceived as the set of all variables.
  */
  while (work!=NULL)
    {
      next = work->next;
      state = work->state;
      gc_base_free (work);
      work = next;
      action_res = node_expr_vars (gc_ctx, state->actionlist);
      GC_UPDATE(gc_ctx, 0, action_res.mayread);
      GC_UPDATE(gc_ctx, 1, action_res.mustset);
      action_res.mustset = vs_union (gc_ctx, state->mustset, action_res.mustset);
      GC_UPDATE(gc_ctx, 1, action_res.mustset);
      /* now equal to mustset after execution of the actionlist */
      for (l=state->translist; l!=NULL; l=BIN_RVAL(l))
	{
	  trans = (node_trans_t *)BIN_LVAL(l);
	  cond_res = node_expr_vars (gc_ctx, trans->cond);
	  GC_UPDATE(gc_ctx, 2, cond_res.mayread);
	  GC_UPDATE(gc_ctx, 3, cond_res.mustset);
	  cond_res.mustset = vs_union (gc_ctx, action_res.mustset,
				       cond_res.mustset);
	  GC_UPDATE (gc_ctx, 3, cond_res.mustset);
	  /* now equal to the mustset after this transition */
	  target = trans->sub_state_dest;
	  if (target==NULL)
	    {
	      target = find_state_by_name (rule, trans->dest);
	      if (target==NULL &&
		  (trans->an_flags & AN_NONEXISTENT_TARGET_ALREADY_SAID)==0)
		{
		  trans->an_flags |= AN_NONEXISTENT_TARGET_ALREADY_SAID;
		  if (trans->file!=NULL)
		    fprintf (stderr, "%s:", trans->file);
		  fprintf (stderr, "%u: error: nonexistent target state '%s'.\n",
			   trans->lineno, trans->dest);
		  ctx->nerrors++;
		}
	    }
	  if (target->an_flags & AN_MUSTSET_REACHABLE)
	    { /* if target already reached by the analyzer,
		 take intersection of its mustset with the mustset
		 obtained after the transition, in GC_LOOKUP(3).
		 We shall reinstall target into the worklist
		 iff target->mustset changes.  It does not change
		 iff target->mustset is already included in GC_LOOKUP(3);
		 we signal this by setting target to NULL, when this
		 happens.
	      */
	      if (vs_subset (gc_ctx, target->mustset, cond_res.mustset))
		target = NULL;
	      else
		{
		  GC_TOUCH (gc_ctx, target->mustset =
			    vs_inter (gc_ctx, target->mustset,
				      cond_res.mustset));
		}
	    }
	  else
	    {
	      target->an_flags |= AN_MUSTSET_REACHABLE;
	      GC_TOUCH (gc_ctx, target->mustset = cond_res.mustset);
	    }
	  /* Now add target to the worklist */
	  if (target!=NULL)
	    {
	      next = work;
	      work = gc_base_malloc (gc_ctx, sizeof(an_worklist_t));
	      work->next = next;
	      work->state = target;
	    }
	  /* And continue */
	}
    }
  GC_END(gc_ctx);
  check_rule_mustset (ctx, rule);
}

varset_t *node_expr_vars_mayset (gc_t *gc_ctx, node_expr_t *e)
{
  varset_t *vs, *newvs;

  vs = VS_EMPTY;
  GC_START(gc_ctx, 2);
 again:
  if (e==NULL)
    ;
  else switch (e->type)
    {
    case NODE_FIELD:
    case NODE_CONST:
    case NODE_BREAK:
    case NODE_VARIABLE:
      break;
    case NODE_CALL:
      /* For NODE_CALL, NODE_BINOP, and general sequence evaluation, we have:
	 mayset(e1; e2) = mayset(e1) U mayset(e2)
      */
      {
	node_expr_t *l;

	for (l=CALL_PARAMS(e); l!=NULL; l=BIN_RVAL(l))
	  {
	    newvs = node_expr_vars_mayset (gc_ctx, BIN_LVAL(l));
	    GC_UPDATE(gc_ctx, 1, newvs);
	    vs = vs_union (gc_ctx, vs, newvs);
	    GC_UPDATE (gc_ctx, 0, vs);
	  }
	break;
      }
    case NODE_RETURN:
      vs = node_expr_vars_mayset (gc_ctx, MON_VAL(e));
      break;
    case NODE_BINOP:
    case NODE_CONS:
    case NODE_EVENT:
    case NODE_COND:
      vs = node_expr_vars_mayset (gc_ctx, BIN_LVAL(e));
      GC_UPDATE(gc_ctx, 0, vs);
      newvs = node_expr_vars_mayset (gc_ctx, BIN_RVAL(e));
      GC_UPDATE(gc_ctx, 1, newvs);
      vs = vs_union (gc_ctx, vs, newvs);
      break;
    case NODE_DB_PATTERN:
      /* e is db_pattern(db, pat1, ..., patn)
	 mayset(e) = mayset(db) U union {mayset(pati) | pati not a logical variable}
	             U {pati | pati a logical variable}
       */
      vs = node_expr_vars_mayset (gc_ctx, BIN_LVAL(e));
      GC_UPDATE(gc_ctx, 0, vs);
      {
	node_expr_t *l;

	for (l=BIN_RVAL(e); l!=NULL; l=BIN_RVAL(l))
	  {
	    if (is_logical_variable(BIN_LVAL(l)))
	      {
		newvs = vs_one_element (gc_ctx, SYM_RES_ID(BIN_LVAL(l)));
		GC_UPDATE(gc_ctx, 1, newvs);
		vs = vs_union (gc_ctx, vs, newvs);
		GC_UPDATE(gc_ctx, 0, vs);
	      }
	    else
	      {
		newvs = node_expr_vars_mayset (gc_ctx, BIN_LVAL(l));
		GC_UPDATE(gc_ctx, 1, newvs);
		vs = vs_union (gc_ctx, vs, newvs);
		GC_UPDATE(gc_ctx, 0, vs);
	      }
	  }
      }
      break;
    case NODE_DB_COLLECT:
      /* e is db_collect(returns, (actions . expr), pat1, pat2, ..., patn)
	 mayset(e) = mayset(actions.expr) U union {mayset (pati) | i=1...n}
       */
      {
	node_expr_t *l;

	e = BIN_RVAL(e); /* skip 'returns' part */
	vs = node_expr_vars_mayset (gc_ctx, BIN_LVAL(e));
	GC_UPDATE(gc_ctx, 0, vs);
	for (l=BIN_RVAL(e); l!=NULL; l=BIN_RVAL(l))
	  {
	    newvs = node_expr_vars_mayset (gc_ctx, BIN_LVAL(l));
	    GC_UPDATE(gc_ctx, 1, newvs);
	    vs = vs_union (gc_ctx, vs, newvs);
	    GC_UPDATE(gc_ctx, 0, vs);
	  }
      }
      break;
    case NODE_DB_SINGLETON:
      /* e is db_singleton(e1, ..., em)
	 mayset(e) = union {mayset (ei) | i=1...m}
       */
      {
	node_expr_t *l;

	for (l=MON_VAL(e); l!=NULL; l=BIN_RVAL(l))
	  {
	    newvs = node_expr_vars_mayset (gc_ctx, BIN_LVAL(l));
	    GC_UPDATE(gc_ctx, 1, newvs);
	    vs = vs_union (gc_ctx, vs, newvs);
	    GC_UPDATE(gc_ctx, 0, vs);
	  }
      }
      break;
    case NODE_MONOP:
      e = MON_VAL(e);
      goto again;
    case NODE_REGSPLIT:
      /* split "regex" /x1, ..., xn/ e1  (former syntax: /e1/"regex"/x1/x2/.../xn)
	 where e1 == REGSPLIT_STRING(e),
	 "regex" == REGSPLIT_PAT(e),
	 [x1, x2, ..., xn] == REGSPLIT_DEST_VARS(e)
	 Its mayset is mayset(e1) union {x1, x2, ..., xn}.
      */
      vs = node_expr_vars_mayset (gc_ctx, REGSPLIT_STRING(e));
      GC_UPDATE(gc_ctx, 0, vs);
      if (REGSPLIT_DEST_VARS(e)!=NULL)
	{
	  newvs = varlist_varset(gc_ctx, 0,
				 REGSPLIT_DEST_VARS(e)->vars_nb,
				 REGSPLIT_DEST_VARS(e)->vars);
	  GC_UPDATE(gc_ctx, 1, newvs);
	  vs = vs_union (gc_ctx, vs, newvs);
	}
      break;
    case NODE_IFSTMT:
      /*
	e == if (e1) e2 else e3
	mayset(e) = mayset(e1) U mayset(e2) U mayset(e3)
       */
      vs = node_expr_vars_mayset (gc_ctx, IF_THEN(e));
      GC_UPDATE(gc_ctx, 0, vs);
      newvs = node_expr_vars_mayset (gc_ctx, IF_ELSE(e));
      GC_UPDATE(gc_ctx, 1, newvs);
      vs = vs_union (gc_ctx, vs, newvs);
      GC_UPDATE(gc_ctx, 0, vs);
      newvs = node_expr_vars_mayset (gc_ctx, IF_COND(e));
      GC_UPDATE(gc_ctx, 1, newvs);
      vs = vs_union (gc_ctx, vs, newvs);
      break;
    case NODE_ASSOC:
      /* e is x = e2
	 mayset(e) = mayset(e2) U {x}
	 where x == BIN_LVAL(e), e2 == BIN_RVAL(e)
       */
      vs = node_expr_vars_mayset (gc_ctx, BIN_RVAL(e));
      GC_UPDATE(gc_ctx, 0, vs);
      newvs = vs_one_element (gc_ctx, SYM_RES_ID(BIN_LVAL(e)));
      GC_UPDATE(gc_ctx, 1, newvs);
      vs = vs_union (gc_ctx, vs, newvs);
      break;
    default:
      DebugLog(DF_OLC, DS_FATAL,
	       "unrecognized node type %d\n", e->type);
      exit(EXIT_FAILURE);
      break;
    }
  GC_END(gc_ctx);
  return vs;
}

static varset_t *single_state_sync_vars_set (rule_compiler_t *ctx, node_state_t *state, varset_t *syncvars)
{ /* For each state, let A be the set of sync vars that it may set,
     B be the set of sync vars that it must set: we first check that
     A==B (there should be no ambiguity whether this state will assign
     to each synchronization variable or not; e.g., 'if (...) $x=1;'
     is forbidden if $x is synchronized, since there will be cases
     where it will be assigned, and cases where it will not).
   */
  gc_t *gc_ctx = ctx->gc_ctx;
  varset_t *mayset, *vs;
  node_expr_vars_t action_res;

  GC_START (gc_ctx, 2);
  action_res = node_expr_vars (gc_ctx, state->actionlist);
  GC_UPDATE (gc_ctx, 0, action_res.mustset);
  mayset = node_expr_vars_mayset (gc_ctx, state->actionlist);
  GC_UPDATE (gc_ctx, 1, mayset);
  mayset = vs_inter (gc_ctx, mayset, syncvars);
  GC_UPDATE (gc_ctx, 1, mayset);
  vs = vs_diff (gc_ctx, mayset, action_res.mustset); /* set of synchronized variables
							that may be assigned to,
							but we are not sure they are. */
  for (; vs!=VS_EMPTY; vs=vs->next)
    {
      if (state->file!=NULL)
	fprintf (stderr, "%s:", state->file);
      fprintf (stderr, "%u: error: synchronized variable %s may be set, may also be left untouched",
	       state->line, ctx->dyn_var_name[vs->n]);
      if (state->name!=NULL)
	fprintf (stderr, ", in state '%s'", state->name);
      fprintf (stderr, " - this is forbidden.\n");
      ctx->nerrors++;
    }
  GC_END (gc_ctx);
  return mayset;
}

struct node_state_array {
  size_t n;
  size_t size;
  node_state_t **state; /* state[size], actually */
};

static void add_node_state (gc_t *gc_ctx, struct node_state_array *nsa, node_state_t *state)
{
  if (nsa->n >= nsa->size)
    nsa->state = gc_base_realloc (gc_ctx, nsa->state, nsa->size = nsa->n+16);
  nsa->state[nsa->n++] = state;
}

static void state_sync_vars_set (rule_compiler_t *ctx, node_rule_t *rule,
				 node_state_t *state,
				 varset_t *syncvars,
				 varset_t *already_set,
				 node_state_t *pred,
				 int all_sync,
				 struct node_state_array *nsa)
/* Check that synchronization variables are rigid, i.e.: once they are set,
   they cannot be changed.  Also do the checks in single_state_sync_vars_set().
   Return number of states reachable from state where all synchronized variables
   will have been set.
*/
{
  /* We assume that check_var_usage() has already been called, so state->mustset
     contains the set of variables that are set on entry to this state for sure.
  */
  gc_t *gc_ctx = ctx->gc_ctx;
  varset_t *vs, *conflict;
  node_expr_t *l;
  node_trans_t *trans;
  node_state_t *target;

  GC_START (gc_ctx, 2);
  /* already_set is a set of synchronized variables that may be set on entry to
     this state: we must first check that they are all set *for sure*.
  */
  vs = vs_diff (gc_ctx, already_set, state->mustset);
  /* No need to test (state->an_flags & AN_MUSTSET_REACHABLE): state is always reachable here. */
  GC_UPDATE (gc_ctx, 0, vs);
  for (; vs!=VS_EMPTY; vs=vs->next)
    {
      /* in that case, pred cannot be NULL */
      if (state->file!=NULL)
	fprintf (stderr, "%s:", state->file);
      fprintf (stderr, "%u: error: on arriving ", state->line);
      if (state->name!=NULL)
	fprintf (stderr, "at state '%s', ", state->name);
      else fprintf (stderr, "here, ");
      fprintf (stderr, "synchronized variable %s may have been set already",
	       ctx->dyn_var_name[vs->n]);
      if (pred!=NULL && pred->name!=NULL)
	fprintf (stderr, " (e.g., at state '%s'),", pred->name);
      fprintf (stderr, " or not");
#ifdef OBSOLETE
      if (pred!=NULL && pred->name!=NULL)
	fprintf (stderr, " (coming from another state)");
#endif
      fprintf (stderr, " - this is forbidden.\n");
      ctx->nerrors++;
    }
  if (state->an_flags & AN_SYNCVARS_DONE)
    goto end;
  vs = single_state_sync_vars_set (ctx, state, syncvars);
  GC_UPDATE (gc_ctx, 0, vs);
  /* Now check that no synchronization variable that is set in the current state
     was in already_set: we cannot reassign a synchronization variable (they are 'rigid').
  */
  conflict = vs_inter (gc_ctx, vs, already_set);
  GC_UPDATE (gc_ctx, 1, conflict);
  for (; conflict!=VS_EMPTY; conflict=conflict->next)
    {
      if (state->file!=NULL)
	fprintf (stderr, "%s:", state->file);
      fprintf (stderr, "%u: error: state ", state->line);
      if (state->name!=NULL)
	fprintf (stderr, "'%s' ", state->name);
      fprintf (stderr, "reassigns synchronized variable %s - this is forbidden.\n",
	       ctx->dyn_var_name[conflict->n]);
      ctx->nerrors++;
    }
  if (!all_sync)
    {
      already_set = vs_union (gc_ctx, already_set, vs);
      GC_UPDATE (gc_ctx, 1, already_set);
      if (vs_subset (gc_ctx, syncvars, already_set))
	{ /* This state is one state where the engine will have to check synchronization:
	     this is the (rather, a) first state where all synchronization variables are defined. */
	  state->flags |= STATE_SYNCVARS_ALL_SYNC;
	  all_sync = 1;
	  add_node_state (gc_ctx, nsa, state);
	  if (ctx->verbose>=2)
	    {
	      if (state->file!=NULL)
		fprintf (stderr, "%s:", state->file);
	      fprintf (stderr, "%u: info: synchronization will occur ", state->line);
	      if (state->name!=NULL)
		fprintf (stderr, "at state '%s'.\n", state->name);
	      else fprintf (stderr, "here.\n");
	    }
	  if ((state->flags & STATE_COMMIT)==0)
	    {
	      if (state->file!=NULL)
		fprintf (stderr, "%s:", state->file);
	      fprintf (stderr, "%u: error: cannot synchronize in a non-commit state:  will occur ", state->line);
	      if (state->name!=NULL)
		fprintf (stderr, "at state '%s'.\n", state->name);
	      else fprintf (stderr, "here.\n");
	    }
	}
    }
  state->an_flags |= AN_SYNCVARS_DONE;
  for (l=state->translist; l!=NULL; l=BIN_RVAL(l))
    {
      trans = (node_trans_t *)BIN_LVAL(l);
      /* In principle, transitions should not be able to set any variable,
	 as checked by check_expect_condition (), so there is no need
	 to update already_set for each transition */
      target = trans->sub_state_dest;
      if (target==NULL)
	{
	  target = find_state_by_name (rule, trans->dest);
	  if (target==NULL &&
	      (trans->an_flags & AN_NONEXISTENT_TARGET_ALREADY_SAID)==0)
	    {
	      trans->an_flags |= AN_NONEXISTENT_TARGET_ALREADY_SAID;
	      if (trans->file!=NULL)
		fprintf (stderr, "%s:", trans->file);
	      fprintf (stderr, "%u: error: nonexistent target state %s.\n",
		       trans->lineno, trans->dest);
	      ctx->nerrors++;
	    }
	}
      state_sync_vars_set (ctx, rule, target, syncvars, already_set, state, all_sync, nsa);
    }
 end:
  GC_END (gc_ctx);
}


static varset_t *rule_sync_vars (rule_compiler_t *ctx, node_rule_t *rule)
{
  node_syncvarlist_t *sync_vars;
  size_t i, n;
  node_expr_t *sync_var;
  node_expr_t **vars;
  varset_t *vs;

  sync_vars = rule->sync_vars;
  if (sync_vars==NULL)
    return VS_EMPTY;
  n = sync_vars->vars_nb;
  vars = gc_base_malloc (ctx->gc_ctx, n*sizeof(node_expr_t *));
  for (i=0; i<n; )
    {
      sync_var = strhash_get (ctx->rule_env, sync_vars->vars[i]);
      if (sync_var==NULL)
	{ /* ignore that (non-existent) synchronization variable;
	     an error message will be printed later, when we call get_and_check_syncvar(),
	     in compile_and_add_rule_ast(). */
	  continue;
	}
      vars[i++] = sync_var;
    }
  n = i;
  vs = varlist_varset (ctx->gc_ctx, 0, n, vars);
  gc_base_free (vars);
  return vs;
}

static void print_cg_data (FILE *f, gc_header_t *data, int vocab);

static void check_sync_same_commit (rule_compiler_t *ctx, node_rule_t *rule, struct node_state_array *nsa)
{ /* nsa is the list of states where synchronization will occur.
     They must all be commit states (with a '!'), or none should be. */
  size_t i, n;
  unsigned long flags;

  n=nsa->n;
  if (n==0)
    return;
  flags = nsa->state[0]->flags & STATE_COMMIT;
  for (i=1; i<n; i++)
    {
      if ((nsa->state[i]->flags & STATE_COMMIT) != flags)
	{
	  if (rule->filename!=NULL)
	    fprintf (stderr, "%s:", rule->filename);
	  fprintf (stderr, "%u: error: in rule %s, all states at which synchronization occurs should commit, or none should, but",
		   rule->line, rule->filename);
	  for (i=0; i<n; i++) /* find commit state; preferrably one with a printable name */
	    {
	      if ((nsa->state[i]->flags & STATE_COMMIT) && nsa->state[i]->name!=NULL)
		break;
	    }
	  if (i>=n)
	    for (i=0; i<n; i++)
	      {
		if (nsa->state[i]->flags & STATE_COMMIT)
		  break;
	      }
	  print_cg_data (stderr, (gc_header_t *)nsa->state[i], 1);
	  fprintf (stderr, " commits (has a '!'), while ");
	  for (i=0; i<n; i++) /* find non-commit state; preferrably one with a printable name */
	    {
	      if (((nsa->state[i]->flags & STATE_COMMIT)==0) && nsa->state[i]->name!=NULL)
		break;
	    }
	  if (i>=n)
	    for (i=0; i<n; i++)
	      {
		if ((nsa->state[i]->flags & STATE_COMMIT)==0)
		  break;
	      }
	  print_cg_data (stderr, (gc_header_t *)nsa->state[i], 1);
	  fprintf (stderr, " does not.\n");
	  ctx->nerrors++;
	  break;
	}
    }
}

static void check_sync_vars (rule_compiler_t *ctx, node_rule_t *rule)
{
  gc_t *gc_ctx = ctx->gc_ctx;
  varset_t *syncvars;
  struct node_state_array nsa = { 0, 0, NULL };

  GC_START (gc_ctx, 1);
  syncvars = rule_sync_vars (ctx, rule);
  if (syncvars!=VS_EMPTY)
    {
      GC_UPDATE (gc_ctx, 0, syncvars);
      state_sync_vars_set (ctx, rule, rule->init, syncvars, VS_EMPTY, NULL, 0, &nsa);
      if (nsa.n==0)
	{
	  if (rule->filename!=NULL)
	    fprintf (stderr, "%s:", rule->filename);
	  fprintf (stderr, "%u: warning: synchronization variables are never all set, rule '%s' will never synchronize.\n",
		   rule->line, rule->name);
	}
      check_sync_same_commit (ctx, rule, &nsa);
    }
  GC_END (gc_ctx);
}

static void mark_epsilon_reachable (rule_compiler_t *ctx, node_state_t *state)
{
  node_expr_t *l;
  node_trans_t *trans;
  node_state_t *s;

  if (state->an_flags & AN_EPSILON_DONE)
    return; /* already dealt with in a previous call to
	       mark_epsilon_reachable() */
  if (state->an_flags & AN_EPSILON_REACHABLE)
    { /* cycle */
      if (state->file!=NULL)
	fprintf (stderr, "%s:", state->file);
      fprintf (stderr, "%u: error: state ", state->line);
      if (state->name!=NULL)
	fprintf (stderr, "'%s' ", state->name);
      fprintf (stderr, "is part of a cycle of gotos.\n");
      ctx->nerrors++;
      return;
    }
  state->an_flags |= AN_EPSILON_REACHABLE;
  if (state->flags & STATE_EPSILON)
    for (l=state->translist; l!=NULL; l=BIN_RVAL(l))
      {
	trans = (node_trans_t *)BIN_LVAL(l);
	if (trans==NULL)
	  continue;
	s = trans_dest_state (ctx, trans);
	if (s!=NULL)
	  mark_epsilon_reachable (ctx, s);
      }
  state->an_flags &= ~AN_EPSILON_REACHABLE;
  state->an_flags |= AN_EPSILON_DONE;
}

static void check_epsilon_transitions (rule_compiler_t *ctx, node_rule_t *rule)
{ /* must be called after check_var_usage(), which sets the
     AN_MUSTSET_REACHABLE flags of each state */
  node_expr_t *l;
  node_state_t *state;

  state = (node_state_t *)rule->init;
  mark_epsilon_reachable (ctx, state);
  for (l=rule->statelist; l!=NULL; l=BIN_RVAL(l))
    {
      state = (node_state_t *)BIN_LVAL(l);
      if ((state->an_flags & AN_MUSTSET_REACHABLE)==0)
	continue; /* ignore unreachable states */
      mark_epsilon_reachable (ctx, state);
    }
}

static monotony compute_monotony_bin (rule_compiler_t *ctx,
				      int op,
				      monotony ma,
				      monotony mb)
{
  monotony res;

  res = MONO_UNKNOWN;
  switch (op)
    {
    case OP_ADD: /* + is meant to be monotonic, whatever the types
		    of the arguments */
      /* MONO+MONO=MONO, ANTI+ANTI=ANTI */
      if ((ma & MONO_MONO) && (mb & MONO_MONO))
	res |= MONO_MONO;
      if ((ma & MONO_ANTI) && (mb & MONO_ANTI))
	res |= MONO_ANTI;
      break;
    case OP_SUB:
      /* - is meant to be monotonic in its first argument and
	 antitonic in its second argument, whatever the types
	 of the arguments */
      /* MONO-ANTI=MONO, ANTI-MONO=ANTI */
      res = MONO_UNKNOWN;
      if ((ma & MONO_MONO) && (mb & MONO_ANTI))
	res |= MONO_MONO;
      if ((ma & MONO_ANTI) && (mb & MONO_MONO))
	res |= MONO_ANTI;
      break;
    case OP_MUL:
    case OP_DIV:
    case OP_MOD:
    case OP_AND: /* &, not && */
    case OP_OR: /* |, not || */
    case OP_XOR: /* ^ */
    case OP_ADD_EVENT:
      if (ma==MONO_CONST && mb==MONO_CONST)
	res = MONO_CONST;
      break;
    default:
      DebugLog(DF_OLC, DS_FATAL,
	       "unrecognized binop %d\n", op);
      exit(EXIT_FAILURE);
      break;
    }
  return res;
}

static monotony compute_monotony_mon (rule_compiler_t *ctx,
				      int op,
				      monotony ma,
				      type_t *ta)
{
  monotony res;

  res = MONO_UNKNOWN;
  switch (op)
    {
    case OP_OPP:
    case OP_NOT:
      if (ma & MONO_MONO)
	res |= MONO_ANTI;
      if (ma & MONO_ANTI)
	res |= MONO_MONO;
      break;
    case OP_PLUS:
    case OP_NOP:
      res = ma;
      break;
    default:
      DebugLog(DF_OLC, DS_FATAL,
	       "unrecognized monop %d\n", op);
      exit(EXIT_FAILURE);
      break;
    }
  return res;
}

static void set_monotony_unknown_mayset (rule_compiler_t *ctx,
					 node_expr_t *e,
					 monotony vars[])
{
  if (e==NULL)
    return;
  switch (e->type)
    {
    case NODE_FIELD:
    case NODE_CONST:
    case NODE_VARIABLE:
    case NODE_BREAK:
      break;
    case NODE_CALL:
      {
	node_expr_t *l;

	for (l=CALL_PARAMS(e); l!=NULL; l=BIN_RVAL(l))
	  set_monotony_unknown_mayset (ctx, BIN_LVAL(l), vars);
	break;
      }
    case NODE_BINOP:
    case NODE_EVENT:
    case NODE_CONS:
    case NODE_COND:
      set_monotony_unknown_mayset (ctx, BIN_LVAL(e), vars);
      set_monotony_unknown_mayset (ctx, BIN_RVAL(e), vars);
      break;
    case NODE_RETURN:
    case NODE_MONOP:
      set_monotony_unknown_mayset (ctx, MON_VAL(e), vars);
      break;
    case NODE_REGSPLIT:
      {
	size_t i, n;

	set_monotony_unknown_mayset (ctx, REGSPLIT_STRING(e), vars);
	if (REGSPLIT_DEST_VARS(e)!=NULL)
	  {
	    n = REGSPLIT_DEST_VARS(e)->vars_nb;
	    for (i=0; i<n; i++)
	      vars[SYM_RES_ID(REGSPLIT_DEST_VARS(e)->vars[i])] = MONO_UNKNOWN;
	  }
	break;
      }
    case NODE_DB_PATTERN:
      {
	node_expr_t *l;

	set_monotony_unknown_mayset (ctx, BIN_LVAL(e), vars);
	for (l=BIN_RVAL(e); l!=NULL; l=BIN_RVAL(l))
	  {
	    if (is_logical_variable (BIN_LVAL(l)))
	      vars[SYM_RES_ID(BIN_LVAL(l))] = MONO_UNKNOWN;
	    else set_monotony_unknown_mayset (ctx, BIN_LVAL(l), vars);
	  }
	break;
      }
    case NODE_DB_COLLECT:
      {
	e = BIN_RVAL(e); /* skip 'returns' part */
	set_monotony_unknown_mayset (ctx, BIN_RVAL(e), vars);
	set_monotony_unknown_mayset (ctx, BIN_LVAL(e), vars);
	break;
      }
    case NODE_DB_SINGLETON:
      {
	node_expr_t *l;

	for (l=MON_VAL(e); l!=NULL; l=BIN_RVAL(l))
	  set_monotony_unknown_mayset (ctx, BIN_LVAL(l), vars);
	break;
      }
    case NODE_IFSTMT:
      set_monotony_unknown_mayset (ctx, IF_COND(e), vars);
      set_monotony_unknown_mayset (ctx, IF_THEN(e), vars);
      set_monotony_unknown_mayset (ctx, IF_ELSE(e), vars);
      break;
    case NODE_ASSOC:
      set_monotony_unknown_mayset (ctx, BIN_RVAL(e), vars);
      vars[SYM_RES_ID(BIN_LVAL(e))] = MONO_UNKNOWN;
      break;
    default:
      DebugLog(DF_OLC, DS_FATAL,
	       "unrecognized node type %d\n", e->type);
      exit(EXIT_FAILURE);
      break;
    }
}
/*
static only_depends_on (gc_t *gc_ctx, node_expr_t *e, varset_t *vars)
{
  node_expr_t *l;

  if (e==NULL)
    return 1;
  switch (e->type)
    {
    case NODE_FIELD:
    case NODE_CONST:
      return 1;
    case NODE_VARIABLE:
      return !vs_in (SYM_RES_ID(e), vars);
    case NODE_CALL:
      for (l=CALL_PARAMS(e); l!=NULL; l=BIN_RVAL(l))
	if (!only_depends_on (gc_ctx, BIN_LVAL(l), vars))
	  return 0;
      return 1;
    case NODE_RETURN:
      return only_depends_on (gc_ctx, MON_VAL(e), vars);
    case NODE_BINOP:
    case NODE_EVENT:
    case NODE_CONS:
    case NODE_COND:
      return only_depends_on (gc_ctx, BIN_LVAL(e), vars) &&
	only_depends_on (gc_ctx, BIN_RVAL(e), vars);
    case NODE_MONOP:
      return only_depends_on (gc_ctx, MON_VAL(e), vars);
    case NODE_REGSPLIT:
      if (!only_depends_on (gc_ctx, REGSPLIT_STRING(e), vars))
	return 0;
      {
	varset_t 
      GC_START(gc_ctx, 1);

      GC_END(gc_ctx);

    }
}
*/

static monotony compute_monotony (rule_compiler_t *ctx, node_expr_t *e,
				  monotony vars[], int const_fields);
/* Imagine we evaluate e in two different variable environments env1 and env2.
   vars[i] says how variable #i evolves from env1 to env2:
   - value increases (>=) if MONO_MONO
   - value decreases (<=) if MONO_ANTI
   - value stays the same (==) if MONO_CONST
   - don't know if MONO_UNKNOWN.
   Then compute_monotony() computes how the value of expression e evolves:
   returns MONO_MONO if values increases, etc.
   This also updates vars[i] in case of side-effects on variables, and
   instructs us about the direction of variation of the values of variables
   *after* the execution of e.
   Finally, const_fields is true if the fields are assumed constant.
   If const_fields is false, we take them to vary according to their
   SYM_MONO field. */

static monotony compute_monotony_cond (rule_compiler_t *ctx,
				       int op,
				       node_expr_t *left,
				       node_expr_t *right,
				       monotony vars[],
				       int const_fields)
{
  monotony ma, mb, res;
  monotony *alt;
  size_t nvars, i;
  size_t varsz;

  switch (op)
    {
    case ANDAND:
    case OROR:
      ma = compute_monotony (ctx, left, vars, const_fields);
      nvars = ctx->rule_env->elmts;
      varsz = nvars*sizeof(monotony);
      alt = gc_base_malloc (ctx->gc_ctx, varsz);
      memcpy (alt, vars, varsz);
      mb = compute_monotony (ctx, right, alt, const_fields);
      /* The effect on vars[] is a bit complicated, and
	 is as in the NODE_IFSTMT case in compute_monotony() */
      if (ma==MONO_CONST)
	{
	  for (i=0; i<nvars; i++)
	    vars[i] &= alt[i];
	}
      else /* cannot conclude on any variable that may be set
	      by left or right. */
	{
	  set_monotony_unknown_mayset (ctx, left, vars);
	  set_monotony_unknown_mayset (ctx, right, vars);
	}
      gc_base_free (alt);
      /* Finally, ANDAND and OROR are monotonic, just like + */
      res = MONO_UNKNOWN;
      /* MONO+MONO=MONO, ANTI+ANTI=ANTI */
      if ((ma & MONO_MONO) && (mb & MONO_MONO))
	res |= MONO_MONO;
      if ((ma & MONO_ANTI) && (mb & MONO_ANTI))
	res |= MONO_ANTI;
      return res;
    case BANG:
      ma = compute_monotony (ctx, right, vars, const_fields);
      /* BANG is antitonic: */
      res = MONO_UNKNOWN;
      if (ma & MONO_MONO)
	res |= MONO_ANTI;
      if (ma & MONO_ANTI)
	res |= MONO_MONO;
      return res;
    case OP_CEQ:
    case OP_CNEQ:
    case OP_CRM:
    case OP_CNRM:
      ma = compute_monotony (ctx, left, vars, const_fields);
      mb = compute_monotony (ctx, right, vars, const_fields);
      if (ma==MONO_CONST && mb==MONO_CONST)
	return MONO_CONST;
      return MONO_UNKNOWN;
    case OP_CGT:
    case OP_CGE:
      ma = compute_monotony (ctx, left, vars, const_fields);
      mb = compute_monotony (ctx, right, vars, const_fields);
      /* >, >= are monotonic in its first argument, antitonic
	 in its second argument, just like OP_SUB */
      res = MONO_UNKNOWN;
      if ((ma & MONO_MONO) && (mb & MONO_ANTI))
	res |= MONO_MONO;
      if ((ma & MONO_ANTI) && (mb & MONO_MONO))
	res |= MONO_ANTI;
      return res;
    case OP_CLT:
    case OP_CLE:
      ma = compute_monotony (ctx, left, vars, const_fields);
      mb = compute_monotony (ctx, right, vars, const_fields);
      /* <, <= are antitonic in its first argument, monotonic
	 in its second argument, just like OP_SUB */
      res = MONO_UNKNOWN;
      if ((mb & MONO_MONO) && (ma & MONO_ANTI))
	res |= MONO_MONO;
      if ((mb & MONO_ANTI) && (ma & MONO_MONO))
	res |= MONO_ANTI;
      return res;
    default:
      DebugLog(DF_OLC, DS_FATAL,
	       "unrecognized cond op %d\n", op);
      exit(EXIT_FAILURE);
      break;
    }
}

static int do_set_mono_const (int var, void *data)
{
  ((monotony *)data)[var] = MONO_CONST;
  return 0;
}

static monotony compute_monotony (rule_compiler_t *ctx, node_expr_t *e,
				  monotony vars[], int const_fields)
{
  if (e==NULL)
    return MONO_CONST; /* null is constant */
  switch (e->type)
    {
    case NODE_FIELD:
      if (const_fields)
	return MONO_CONST;
      else
	return SYM_MONO(e);
    case NODE_CONST:
      return MONO_CONST;
    case NODE_VARIABLE:
      return vars[SYM_RES_ID(e)];
    case NODE_CALL:
      {
	monotony (*cm) (rule_compiler_t *ctx,
			node_expr_t *e,
			monotony args[]);
	size_t i, n;
	monotony *args;
	node_expr_t *l;
	monotony res;

	cm = CALL_COMPUTE_MONOTONY(e);
	n = list_len (CALL_PARAMS(e));
	args = gc_base_malloc (ctx->gc_ctx, n*sizeof(monotony));
	for (i=0, l=CALL_PARAMS(e); l!=NULL; i++, l=BIN_RVAL(l))
	  args[i] = compute_monotony (ctx, BIN_LVAL(l), vars, const_fields);
	if (cm==NULL)
	  res = MONO_UNKNOWN; /* we do this after we have computed
				 args[i] for every i, to give a chance
				 to compute_monotony() to update vars[]
				 for the given sequence of arguments. */
	else res = (*cm) (ctx, e, args) & MONOTONY_MASK; /* remove MONO_THRASH bit */
	for (i=0, l=CALL_PARAMS(e); l!=NULL; i++, l=BIN_RVAL(l))
	  if (args[i] & MONO_THRASH)
	    set_monotony_unknown_mayset (ctx, BIN_LVAL(l), vars);
	gc_base_free(args);
	return res;
      }
    case NODE_RETURN:
    case NODE_BREAK:
      /* return compute_monotony (ctx, MON_VAL(e), vars); */
      /* No: 'return' never returns, i.e., never proceeds to the next
	 instruction.  Hence the final value of compute_monotony()
	 in this case should be top: MONO_CONST, and all vars
	 set to MONO_CONST.  Same thing for 'break'. */
      {
	size_t nvars, i;

	nvars = ctx->rule_env->elmts;
	for (i=0; i<nvars; i++)
	  vars[i] = MONO_CONST;
	return MONO_CONST;
      }
    case NODE_BINOP:
      {
	monotony ma, mb;

	ma = compute_monotony (ctx, BIN_LVAL(e), vars, const_fields);
	mb = compute_monotony (ctx, BIN_RVAL(e), vars, const_fields);
	return compute_monotony_bin (ctx, BIN_OP(e), ma, mb);
      }
    case NODE_MONOP:
      {
	monotony ma;

	ma = compute_monotony (ctx, MON_VAL(e), vars, const_fields);
	return compute_monotony_mon (ctx, MON_OP(e),
				     ma, MON_VAL(e)->stype);
      }
    case NODE_COND:
      return compute_monotony_cond (ctx, BIN_OP(e),
				    BIN_LVAL(e), BIN_RVAL(e),
				    vars, const_fields);
    case NODE_EVENT:
    case NODE_CONS:
      {
	monotony ma, mb;

	ma = compute_monotony (ctx, BIN_LVAL(e), vars, const_fields);
	mb = compute_monotony (ctx, BIN_RVAL(e), vars, const_fields);
	if (ma==MONO_CONST && mb==MONO_CONST)
	  return MONO_CONST;
	return MONO_UNKNOWN;
      }
    case NODE_DB_PATTERN:
      {
	monotony m;
	node_expr_t *l;

	/* The ordering on databases is inclusion.
	   Here we check whether there is potential for the final database
	   comprehension to grow, judging from a single pattern.
	   In a pattern (pat1, ..., patn) in db,
	   the final database may grow iff db grows, and each pati
	   that is not a logical variable is constant.
	   Additionally, we set each pati that is a logical variable
	   to MONO_UNKNOWN.
	 */
	m = compute_monotony (ctx, BIN_LVAL(e), vars, const_fields);
	for (l=BIN_RVAL(e); l!=NULL; l=BIN_RVAL(l))
	  {
	    if (is_logical_variable (BIN_LVAL(l)))
	      vars[SYM_RES_ID(BIN_LVAL(l))] = MONO_UNKNOWN;
	    else if (compute_monotony (ctx, BIN_LVAL(l), vars, const_fields)!=MONO_CONST)
	      m = MONO_UNKNOWN;
	  }
	return m;
      }
    case NODE_DB_COLLECT:
      {
	monotony m;
	monotony *alt;
	size_t nvars;
	size_t varsz;
	varset_t *bound;
	node_expr_t *l;

	if (BIN_LVAL(e)!=NULL)
	  { /* If there are any 'return' statements,
	       we just decide that the whole db_collect
	       has unknown monotonicity.  This is brutal,
	       but at least it is correct.
	       In general, the only purpose of 'return' statements
	       here is to install exceptions to the case where
	       'collect' is computed, inside 'if' statements... in
	       which case the monotonicity will turn out to be
	       unknown anyway.
	    */
	    m = MONO_UNKNOWN;
	    return m;
	  }
	m = MONO_CONST;
	for (l=BIN_RVAL(BIN_RVAL(e)); l!=NULL; l=BIN_RVAL(l))
	  {
	    m &= compute_monotony (ctx, BIN_LVAL(l), vars, const_fields);
	  }
	if (m!=MONO_UNKNOWN)
	  { /* decide whether the tuple BIN_LVAL(e) is constant under the
	       assumption that all bound variables (in 'bound' below) are constant;
	       if so, keep m, otherwise set m to MONO_UNKNOWN.
	       We do so using an alternate monotony table alt, instead of vars.
	     */
	    nvars = ctx->rule_env->elmts;
	    varsz = nvars*sizeof(monotony);
	    alt = gc_base_malloc (ctx->gc_ctx, varsz);
	    memcpy (alt, vars, varsz);
	    bound = vs_bound_vars_db_pattern (ctx->gc_ctx, BIN_RVAL(BIN_RVAL(e)));
	    (void) vs_sweep (bound, do_set_mono_const, alt);
	    (void) compute_monotony (ctx, BIN_LVAL(BIN_LVAL(BIN_RVAL(e))), alt, const_fields);
	    if (compute_monotony (ctx, BIN_RVAL(BIN_LVAL(BIN_RVAL(e))), alt, const_fields)!=MONO_CONST)
	      m = MONO_UNKNOWN;
	    gc_base_free (alt);
	  }

	/* It might be that the actions.expr BIN_LVAL(e) is never evaluated;
	   this forces us to do as in the NODE_IFSTMT case, and
	   call set_monotony_unknown_mayset() to say that all assignments
	   inside actions.expr force the assigned-to variable to be
	   MONO_UNKNOWN */
	set_monotony_unknown_mayset (ctx, BIN_LVAL(e), vars);
	return m;
      }
    case NODE_DB_SINGLETON:
      { /* Recall db_maps are compared for inclusion;
	   the only case where a singleton can grow is when it is constant
	*/
	monotony m;
	node_expr_t *l;

	m = MONO_CONST;
	for (l=MON_VAL(e); l!=NULL; l=BIN_RVAL(l))
	  if (compute_monotony (ctx, BIN_LVAL(l), vars, const_fields)!=MONO_CONST)
	    {
	      m = MONO_UNKNOWN;
	      break;
	    }
	return m;
      }
    case NODE_REGSPLIT:
      {
	size_t i, n;

	if (REGSPLIT_DEST_VARS(e)!=NULL)
	  n = REGSPLIT_DEST_VARS(e)->vars_nb;
	else n = 0;
	if (compute_monotony (ctx, REGSPLIT_STRING(e), vars, const_fields)==MONO_CONST)
	  {
	    for (i=0; i<n; i++)
	      vars[SYM_RES_ID(REGSPLIT_DEST_VARS(e)->vars[i])] = MONO_CONST;
	    return MONO_CONST;
	  }
	else
	  {
	    for (i=0; i<n; i++)
	      vars[SYM_RES_ID(REGSPLIT_DEST_VARS(e)->vars[i])] = MONO_UNKNOWN;
	    return MONO_UNKNOWN;
	  }
      }
    case NODE_IFSTMT:
      {
	monotony mif, mthen, melse;
	monotony *vars_then, *vars_else;
	size_t nvars, i;
	size_t varsz;
	monotony res;

	mif = compute_monotony (ctx, IF_COND(e), vars, const_fields);
	nvars = ctx->rule_env->elmts;
	varsz = nvars*sizeof(monotony);
	vars_then = gc_base_malloc (ctx->gc_ctx, varsz);
	vars_else = gc_base_malloc (ctx->gc_ctx, varsz);
	memcpy (vars_then, vars, varsz);
	mthen = compute_monotony (ctx, IF_THEN(e), vars_then, const_fields);
	memcpy (vars_else, vars, varsz);
	melse = compute_monotony (ctx, IF_ELSE(e), vars_else, const_fields);
	if (mif==MONO_CONST)
	  {
	    for (i=0; i<nvars; i++)
	      vars[i] = vars_then[i] & vars_else[i];
	    res = mthen & melse;
	  }
	else /* cannot conclude, on result (res), and on
	      any variable that may be set by the then or else branch. */
	  {
	    set_monotony_unknown_mayset (ctx, IF_THEN(e), vars);
	    set_monotony_unknown_mayset (ctx, IF_ELSE(e), vars);
	    res = MONO_UNKNOWN;
	  }
	gc_base_free(vars_then);
	gc_base_free(vars_else);
	return res;
      }
    case NODE_ASSOC:
      return vars[SYM_RES_ID(BIN_LVAL(e))] =
	compute_monotony (ctx, BIN_RVAL(e), vars, const_fields);
    default:
      DebugLog(DF_OLC, DS_FATAL,
	       "unrecognized node type %d\n", e->type);
      exit(EXIT_FAILURE);
      break;
    }
}

static monotony compute_monotony_simple (rule_compiler_t *ctx, node_expr_t *e)
{
  monotony *vars, m;
  size_t i, n;

  n = ctx->dyn_var_name_nb;
  vars = gc_base_malloc (ctx->gc_ctx, n*sizeof(monotony));
  for (i=0; i<n; i++)
    vars[i] = MONO_CONST;
  m = compute_monotony (ctx, e, vars, 0);
  gc_base_free (vars);
  return m;
}

typedef struct sesf_s sesf_t;
struct sesf_s {
  node_state_t *state;
  size_t nvars;
  monotony vars[];
};

static hcode_t sesf_hash (hkey_t *key, size_t keylen)
{
  sesf_t *sesf = (sesf_t *)key;
  hcode_t hash;
  size_t i, n;

  hash = (unsigned long)sesf->state;
  n = sesf->nvars;
  hash += n;
  for (i=0; i<n; i++)
    hash += sesf->vars[i];
  return hash;
}

static int same_event_sequence_fits (rule_compiler_t *ctx, node_state_t *state,
				     monotony vars[], hash_t *memo, int notfirst)
{
  sesf_t *sesf;
  size_t i, n, sz;
  void *p;
  monotony *newvars, *condvars;
  size_t varsz;
  node_expr_t *l;
  monotony m;
  node_trans_t *t;
  node_state_t *next;
  int res;

  if (state->flags & STATE_COMMIT)
    return 1; /* acceptance (even though the sequence may go on) */
  n = ctx->dyn_var_name_nb;
  sz = offsetof(sesf_t, vars[n]);
  p = NULL;
  sesf = NULL;
  if (notfirst)
    {
      sesf = gc_base_malloc (ctx->gc_ctx, sz);
      sesf->state = state;
      sesf->nvars = n;
      for (i=0; i<n; i++)
	sesf->vars[i] = vars[i];
      p = hash_get (memo, sesf, sz);
    }
  res = 1;
  if (p!=NULL) /* cycle found */
    ;
  else
    {
      if (notfirst)
	hash_add (ctx->gc_ctx, memo, (void *)1, sesf, sz);
      varsz = n*sizeof(monotony);
      newvars = gc_base_malloc (ctx->gc_ctx, 2*varsz);
      memcpy (newvars, vars, varsz);
      condvars = newvars + n;
      for (l=state->actionlist; l!=NULL; l=BIN_RVAL(l))
	(void) compute_monotony (ctx, BIN_LVAL(l), newvars, notfirst);
      for (l=state->translist; l!=NULL; l=BIN_RVAL(l))
	{
	  t = (node_trans_t *)BIN_LVAL(l);
	  if (t->cond!=NULL)
	    {
	      memcpy (condvars, newvars, varsz);
	      m = compute_monotony (ctx, t->cond, condvars, 1);
	      /* we wish that if the condition holds (==1) in the future
		 then it holds in the past, namely that the condition
		 be antitone (or constant). */
	      if (!(m & MONO_ANTI))
		{
		  res = 0; /* no way */
		  break;
		}
	      next = trans_dest_state (ctx, t);
	      res = same_event_sequence_fits (ctx, next, condvars, memo, 1);
	      if (res==0)
		break;
	    }
	}
      gc_base_free (newvars);
      if (notfirst)
	(void) hash_del (memo, sesf, sz);
    }
  if (notfirst)
    gc_base_free (sesf);
  return res;
}

static int trans_no_wait_needed (rule_compiler_t *ctx, node_trans_t *trans)
/* Imagine we are able to take transition 'trans' at some event #m.
   By default, the Orchids engine will also created a thread T that waits
   for 'trans' to match at some later event #n.
   Imagine the latter will eventually succeed with a run n=n0 < n1 < n2 < ...
   We would like to know whether the run m < n1 < n2 < ... (i.e., with n
   replaced by m) would have succeeded too.  In that case, it is useless
   to create the thread T.
   The purpose of trans_no_wait_needed() is to detect that case.
   If so, we return true.
*/
{
  monotony *vars;
  size_t varsz, i, n;
  node_state_t *next;
  hash_t *memo;
  int res;

  res = 1;
  n = ctx->dyn_var_name_nb;
  varsz = n*sizeof(monotony);
  vars = gc_base_malloc (ctx->gc_ctx, varsz);
  for (i=0; i<n; i++)
    vars[i] = MONO_CONST; /* We start assuming that the engine is started
			     with the same values of the variables at #m and
			     at #n. */
  next = trans_dest_state (ctx, trans);
  memo = new_hash (ctx->gc_ctx, 1023); /* XXX hardcoded */
  memo->hash = sesf_hash;
  /* Then we check that the same sequence of events n1 < n2 < ...
     will fit the next state */
  res = same_event_sequence_fits (ctx, next, vars, memo, 0);
  free_hash (memo, NULL);
  gc_base_free (vars);
  return res;
}

monotony m_const (rule_compiler_t *ctx, node_expr_t *e, monotony m[])
{ /* function is constant, no side-effect */
  return MONO_CONST;
}

monotony m_const_thrash (rule_compiler_t *ctx, node_expr_t *e, monotony m[])
{ /* function is constant, but may have side-effects (printing, exiting, etc.) */
  return MONO_CONST | MONO_THRASH;
}

monotony m_unknown_1 (rule_compiler_t *ctx, node_expr_t *e, monotony m[])
{
  if (m[0]==MONO_CONST)
    return MONO_CONST;
  return MONO_UNKNOWN;
}

monotony m_unknown_1_thrash (rule_compiler_t *ctx, node_expr_t *e, monotony m[])
{
  if (m[0]==MONO_CONST)
    return MONO_CONST | MONO_THRASH;
  return MONO_UNKNOWN | MONO_THRASH;
}

monotony m_unknown_2 (rule_compiler_t *ctx, node_expr_t *e, monotony m[])
{
  if (m[0]==MONO_CONST && m[1]==MONO_CONST)
    return MONO_CONST;
  return MONO_UNKNOWN;
}

monotony m_unknown_2_thrash (rule_compiler_t *ctx, node_expr_t *e, monotony m[])
{
  if (m[0]==MONO_CONST && m[1]==MONO_CONST)
    return MONO_CONST | MONO_THRASH;
  return MONO_UNKNOWN | MONO_THRASH;
}

monotony m_random (rule_compiler_t *ctx, node_expr_t *e, monotony m[])
{
  return MONO_UNKNOWN;
}

monotony m_random_thrash (rule_compiler_t *ctx, node_expr_t *e, monotony m[])
{
  return MONO_UNKNOWN | MONO_THRASH;
}

static void mark_no_wait_transitions_state (rule_compiler_t *ctx, node_state_t *state)
{
  node_expr_t *l;
  node_trans_t *trans;
  node_state_t *s;

  if (state->flags & STATE_EPSILON) /* no transition out of an epsilon state has to wait */
    for (l=state->translist; l!=NULL; l=BIN_RVAL(l))
      {
	trans = (node_trans_t *)BIN_LVAL(l);
	if (trans==NULL)
	  continue;
	trans->an_flags |= AN_TRANS_NO_WAIT;
	s = trans->sub_state_dest;
	if (s!=NULL)
	  mark_no_wait_transitions_state (ctx, s);
      }
  else for (l=state->translist; l!=NULL; l=BIN_RVAL(l))
      {
	trans = (node_trans_t *)BIN_LVAL(l);
	if (trans==NULL)
	  continue;
	if (trans_no_wait_needed (ctx, trans))
	  trans->an_flags |= AN_TRANS_NO_WAIT;
	s = trans->sub_state_dest;
	if (s!=NULL)
	  mark_no_wait_transitions_state (ctx, s);
      }
}

static void mark_no_wait_transitions (rule_compiler_t *ctx, node_rule_t *rule)
{ /* must be called after check_var_usage(), which sets the
     AN_MUSTSET_REACHABLE flags of each state */
  node_expr_t *l;
  node_state_t *state;

  state = (node_state_t *)rule->init;
  mark_no_wait_transitions_state (ctx, state);
  for (l=rule->statelist; l!=NULL; l=BIN_RVAL(l))
    {
      state = (node_state_t *)BIN_LVAL(l);
      if ((state->an_flags & AN_MUSTSET_REACHABLE)==0)
	continue; /* ignore unreachable states */
      mark_no_wait_transitions_state (ctx, state);
    }
}

/* Complexity evaluator */

typedef struct complexity_ctx_s complexity_ctx_t;
struct complexity_ctx_s {
  gc_t *gc_ctx;
  hash_t *nodes;
  rule_compiler_t *ctx;
  complexity_graph_t *g;
  vertex_t one;
  vertex_t root;
};

static vertex_t get_state_node (complexity_ctx_t *cc, node_state_t *state)
{
  void *data;
  vertex_t v;

  data = hash_get (cc->nodes, &state, sizeof(node_state_t *));
  if (data!=NULL)
    return (vertex_t)data;
  if (state->flags & STATE_EPSILON)
    {
      v = cg_new_node (cc->gc_ctx, cc->g, state, VI_TYPE_MAX, 0);
      (void) cg_new_edge (cc->gc_ctx, cc->g, v, cc->one, 1);
      /* All state-related nodes for STATE_EPSILON nodes
	 are created of the form u_n = max (one_n, ...).
	 Note that label is 0, i.e.,
	 equation is u_n = max (...), not u_{n+1} = max (...):
	 a STATE_EPSILON node does not wait for an event to be read.
      */
    }
  else
    {
      v = cg_new_node (cc->gc_ctx, cc->g, state, VI_TYPE_PLUS, 0);
      /* All other nodes are created of the form
	 u_n = v_n + w_n + ... */
    }
  hash_add (cc->gc_ctx, cc->nodes, (void *)v, &state, sizeof(node_state_t *));
  return v;
}

static vertex_t get_commit_node (complexity_ctx_t *cc, node_state_t *state)
{
  vertex_t one;

  one = cg_new_node (cc->gc_ctx, cc->g, state, VI_TYPE_MAX, 1);
  (void) cg_new_edge (cc->gc_ctx, cc->g, one, one, 1);
  return one;
}

static vertex_t add_complexity_node (complexity_ctx_t *cc, node_state_t *state);

static vertex_t add_max_complexity_node (complexity_ctx_t *cc, node_state_t *state)
{
  gc_t *gc_ctx = cc->gc_ctx;
  complexity_graph_t *g = cc->g;
  vertex_t u, v;
  node_expr_t *l;
  node_trans_t *trans;
  node_state_t *succ;
  
  /* For a node u of the form 
     {
       [actions...]
       case (...) goto v;
       else case (...) goto w;
       else ...
     }
     the number of threads that can be created from this point when we have read n events
     is u_n, defined as u_n = max (one_n, v_n, w_n, ...)
     except that v_n is removed if v is marked as STATE_COMMIT.
     The edge from u to one is created automatically by get_state_node().
     (It is probably not needed in the implementation; it is needed for the
     soundness proof.)
   */
  u = get_state_node (cc, state);
  for (l = state->translist; l!=NULL; l=BIN_RVAL(l))
    {
      trans = (node_trans_t *)BIN_LVAL(l);
      if (trans==NULL)
	continue;
      succ = trans->sub_state_dest;
      if (succ==NULL)
	{
	  succ = trans_dest_state (cc->ctx, trans);
	  if (succ->flags & STATE_COMMIT)
	    continue; /* ignore those 'commit' states (written with a bang '!'
			 in the concrete syntax) */
	  v = get_state_node (cc, succ);
	}
      else
	v = add_complexity_node (cc, succ);
      (void) cg_new_edge (gc_ctx, g, u, v, 1);
    }
  return u;
}

static vertex_t add_plus_complexity_node (complexity_ctx_t *cc, node_state_t *state)
{
  gc_t *gc_ctx = cc->gc_ctx;
  complexity_graph_t *g = cc->g;
  vertex_t u, v, vprime;
  node_expr_t *l;
  node_trans_t *trans;
  node_state_t *succ;

  /* For a node u of the form
     {
       [actions...]
       expect (...) goto v;
       expect (...) goto w;
       ...
     }
     the number of threads that can be created from this point when we have read n events
     is u_n, defined as u_n = v'_n + w'_n + ...
     where v'_n is the number of threads that are created from 'expect (...) goto v'
     (and similarly for w, etc.), and is defined as
     v'_{n+1} = v'_n + v_n
     ... except:
     - if v is marked as STATE_COMMIT, then v'_n = one_n
     - else, and if the transition is marked as AN_TRANS_NO_WAIT, then v'_n = v_n

     If there is no transition out of this node, then we do not write the sum v'_n+w'_n+...
     at all, and u_n = one_n
   */
  u = get_state_node (cc, state);
  l = state->translist;
  if (l==NULL) /* if no transition out of this node, then u_n = one_n */
    {
      (void) cg_new_edge (gc_ctx, g, u, cc->one, 1);
    }
  else for (; l!=NULL; l=BIN_RVAL(l))
    {
      trans = (node_trans_t *)BIN_LVAL(l);
      if (trans==NULL)
	continue;
      succ = trans->sub_state_dest;
      if (succ==NULL)
	{
	  succ = trans_dest_state (cc->ctx, trans);
	  if (succ->flags & STATE_COMMIT)
	    vprime = get_commit_node (cc, succ);
	  /* We might just write cc->one here, instead of allocating a fresh
	     node that will have complexity just one; but cg_critical_path()
	     would then return one instead of vprime, and printing the critical
	     path would produce unreadable results. */
	  else
	    {
	      v = get_state_node (cc, succ);
	      if (trans->an_flags & AN_TRANS_NO_WAIT)
		vprime = v;
	      else
		{
		  vprime = cg_new_node (gc_ctx, g, trans, VI_TYPE_PLUS, 1);
		  (void) cg_new_edge (gc_ctx, g, vprime, vprime, 1);
		  (void) cg_new_edge (gc_ctx, g, vprime, v, 1);
		}
	    }
	}
      else
	{
	  /* included state (not a named state) */
	  /* Do as above, except succ cannot have the STATE_COMMIT flag,
	     we should not call get_state_node(),
	     and we have to call add_complexity_node() recursively. */
	  v = add_complexity_node (cc, succ);
	  if (trans->an_flags & AN_TRANS_NO_WAIT)
	    vprime = v;
	  else
	    {
	      vprime = cg_new_node (gc_ctx, g, trans, VI_TYPE_PLUS, 1);
	      (void) cg_new_edge (gc_ctx, g, vprime, vprime, 1);
	      (void) cg_new_edge (gc_ctx, g, vprime, v, 1);
	    }
	}
      (void) cg_new_edge (gc_ctx, g, u, vprime, 1);
    }
  return u;
}

static vertex_t add_complexity_node (complexity_ctx_t *cc, node_state_t *state)
{
  vertex_t u;
  
  if (state->flags & STATE_EPSILON)
    u = add_max_complexity_node (cc, state);
  else u = add_plus_complexity_node (cc, state);
  if (state->flags & STATE_COMMIT) /* root_n = max (v_n, w_n, ...)
				      where v, w, ..., are the STATE_COMMIT states */
    (void) cg_new_edge (cc->gc_ctx, cc->g, cc->root, u, 1);
  return u;
}

#ifdef OBSOLETE
static void print_cg_data_where (FILE *f, gc_header_t *data)
{
  char *file = NULL;
  int line = 0;
  
  if (data!=NULL)
    switch (TYPE(data))
      {
      case T_NODE_RULE:
	file = ((node_rule_t *)data)->filename; line = ((node_rule_t *)data)->line; break;
      case T_NODE_STATE:
	file =  ((node_state_t *)data)->file; line = ((node_state_t *)data)->line; break;
      case T_NODE_TRANS:
	file = ((node_trans_t *)data)->file; line = ((node_trans_t *)data)->lineno; break;
      }
  if (file!=NULL)
    fprintf (f, "%s:%u:", file, line);
  else fprintf (f, "%u:", line);
}
#endif

static void print_cg_data (FILE *f, gc_header_t *data, int vocab)
{
  node_state_t *state;
  node_trans_t *trans;
  char *name;
  static char *implicit_state_rendition[2] = { "", "state " };
  static char *implicit_waiting_trans_rendition[2] = { "waiting at ", "transition waiting at " };
  static char *implicit_non_waiting_trans_rendition[2] = { "going through ", "transition through " };
  
  if (data==NULL)
    fprintf (f, "(null)");
  else switch (TYPE(data))
	 {
	 case T_NODE_RULE:
	   fprintf (f, "start of rule %s", ((node_rule_t *)data)->name);
	   break;
	 case T_NODE_STATE:
	   state = (node_state_t *)data;
	   name = state->name;
	   if (name==NULL)
	     {
	       fprintf (f, "%s", implicit_state_rendition[vocab]);
	       if (state->file==NULL)
		 fprintf (f, "at line %u", state->line);
	       else fprintf (f, "at %s:%u", state->file, state->line);
	     }
	   else
	     fprintf (f, "'%s'", name);
	   break;
	 case T_NODE_TRANS:
	   trans = (node_trans_t *)data;
	   if ((trans->an_flags & AN_TRANS_NO_WAIT)==0)
	     {
	       fprintf (f, "%s", implicit_waiting_trans_rendition[vocab]);
	     }
	   else
	     {
	       fprintf (f, "%s", implicit_non_waiting_trans_rendition[vocab]);
	     }
	   if (trans->file==NULL)
	     fprintf (f, "line %u", trans->lineno);
	   else fprintf (f, "%s:%u", trans->file, trans->lineno);
	   break;
	 default:
	   fprintf (f, "(?)");
	   break;
	 }
}

static void print_scc (FILE *f, complexity_graph_t *g, vertex_t u, char *ifone, char *ifseveral)
{
  char *delim = "";
  
  u = CG_SCC_ROOT(g,u);
  if (CG_NEXT(g,u)==ULONG_MAX)
      fprintf (f, "%s", ifone);
  else fprintf (f, "%s", ifseveral);
  while (u!=ULONG_MAX)
    {
      fprintf (f, "%s", delim);
      print_cg_data (f, CG_DATA(g,u), 1);
      delim = ",\n       ";
      u = CG_NEXT(g,u);
    }
}

static void print_complexity_evaluation (gc_t *gc_ctx, FILE *f,
					 complexity_ctx_t *cc, node_rule_t *node_rule)
{
  complexity_graph_t *g = cc->g;
  vertex_t root = cc->root;
  unsigned long degree;
  vertex_t u;
  gc_header_t *data;
  critical_path_t *cp;
  size_t i, n;

  degree = CG_DEGREE(g,root);
  if (degree==0) /* rule will only create a constant number of threads: excellent. */
    return;
  if (node_rule->filename!=NULL)
    fprintf (f, "%s:", node_rule->filename);
  switch (degree)
    {
    case ULONG_MAX:
      fprintf (f, "%u: info: rule %s may exhibit exponential behavior.\n",
	       node_rule->line, node_rule->name);
      u = g->first_bad;
      if (u!=ULONG_MAX)
	{
	  data = CG_DATA(g,u);
	  fprintf (f, "  ");
	  /*
	    print_cg_data_where (f, data);
	    fprintf (f, " ");
	  */
	  print_cg_data (f, data, 1);
	  fprintf (f, " will fork at least two threads,\n    while looping ");
	  print_scc (f, g, u, "on ", "among:\n   ");
	  fprintf (f, ".\n");
	}
      break;
    case 1:
      fprintf (f, "%u: info: rule %s may have worst case linear behavior, i.e., O(#events).\n",
	       node_rule->line, node_rule->name);
      goto explain_poly;
    default:
      fprintf (f, "%u: info: rule %s may have worst case behavior O(#events^%lu).\n",
	       node_rule->line, node_rule->name, degree);
    explain_poly:
      cp = cg_critical_path (gc_ctx, g, root);
      if (cp!=NULL)
	{
	  for (n=cp->nedges, i=0; i<n; i++)
	    {
	      if (cp->edges[i].flags & CRIT_EXPENSIVE)
		{
		  fprintf (f, "  ");
		  u = cp->edges[i].source;
		  data = CG_DATA(g, u);
		  print_cg_data (f, data, 1);
		  fprintf (f, " will fork a new thread going to ");
		  data = CG_DATA(g,cp->edges[i].dest);
		  print_cg_data (f, data, 1);
		  fprintf (f, ",\n    while looping ");
		  print_scc (f, g, u, "on ", "among:\n   ");
		  fprintf (f, ".\n");
		}
	    }
	  gc_base_free (cp);
	}
      break;
    }
}

static void evaluate_complexity (rule_compiler_t *ctx, node_rule_t *node_rule)
{
  gc_t *gc_ctx = ctx->gc_ctx;
  node_expr_t *states;
  complexity_ctx_t cc;
  unsigned long index = 0L;
  vertex_t init;

  cc.gc_ctx = gc_ctx;
  cc.ctx = ctx;
  cc.nodes = new_hash (gc_ctx, 100);
  cc.g = new_complexity_graph (gc_ctx);
  cc.one = cg_new_node (gc_ctx, cc.g, node_rule, VI_TYPE_MAX, 1);
  (void) cg_new_edge (gc_ctx, cc.g, cc.one, cc.one, 1);
  /* sequence one_{n+1} = one_n (and implicitly, one_0=1):
     defines the constant "one". */
  cc.root = cg_new_node (gc_ctx, cc.g, NULL, VI_TYPE_MAX, 1);
  init = add_complexity_node (&cc, node_rule->init);
  (void) cg_new_edge (gc_ctx, cc.g, cc.root, init, 1);
  for (states=node_rule->statelist; states!=NULL; states=BIN_RVAL(states))
    (void) add_complexity_node (&cc, (node_state_t *)BIN_LVAL(states));
  cg_compute_sccs_from_root (cc.g, cc.root, &index);
  if (ctx->verbose>=1)
    print_complexity_evaluation (gc_ctx, stderr, &cc, node_rule);
  node_rule->complexity_degree = CG_DEGREE (cc.g, cc.root);
  free_complexity_graph (cc.g);
}


/**
 * Lex/yacc parser entry point.
 * @param ctx Orchids context.
 * @param rulefile File to parse.
 **/
static void compile_and_add_rulefile(orchids_t *ctx, char *rulefile)
{
  int ret;
  FILE *issdlin;
  ovm_var_t *currfile;
  size_t len;
#ifdef ENABLE_PREPROC
  const char *ppcmd;
  char cmd[4096];
#endif /* ENABLE_PREPROC */

  DebugLog(DF_OLC, DS_NOTICE, "Compiling rule file '%s'\n", rulefile);

  /* set some compiler context values */
  len = strlen(rulefile);
  currfile = ovm_str_new(ctx->gc_ctx, len+1);
  strcpy(STR(currfile), rulefile); /* add final NUL byte so as to
				      ease printing */
  STRLEN(currfile) = len;
  GC_TOUCH (ctx->gc_ctx, ctx->rule_compiler->currfile = currfile);

  ctx->rule_compiler->issdlcurrentfile = gc_strdup(ctx->gc_ctx, rulefile);
  ctx->rule_compiler->issdllineno = 1;
#ifdef ENABLE_PREPROC
  ppcmd = get_preproc_cmd(ctx, rulefile);
  DebugLog(DF_OLC, DS_NOTICE, "Using the preproc cmd '%s'.\n", ppcmd);
  snprintf(cmd, sizeof (cmd), "%s %s", ppcmd, rulefile);
  issdlin = Xpopen(cmd, "r");
#else
  issdlin = Xfopen(rulefile, "r");
#endif /* ENABLE_PREPROC */

  /* parse rule file */
  ctx->rule_compiler->issdlin = issdlin;
  issdllex_init (&ctx->rule_compiler->scanner);
  ret = issdlparse(ctx->rule_compiler);
  if (ret > 0) {
    fprintf (stderr, "%s: error: cannot compile rule file.\n", rulefile);
    fflush (stderr);
    exit(EXIT_FAILURE);
  }

#ifdef ENABLE_PREPROC
  ret = Xpclose(issdlin);
  if (ret > 0) {
    DebugLog(DF_OLC, DS_ERROR, "error: preprocessor returned %i.\n", ret);
    exit(EXIT_FAILURE);
  }
#else
  Xfclose(issdlin);
#endif /* ENABLE_PREPROC */
  ctx->rule_compiler->issdlin = NULL;

  gettimeofday(&ctx->last_rule_act, NULL);
}


/* Abstract syntax tree building functions (called by the yaccer issdl.y) */

node_state_t *build_state(rule_compiler_t *ctx,
			  node_expr_t *actions,
			  node_expr_t *transitions)
{
  node_state_t *s;

  s = gc_alloc (ctx->gc_ctx, sizeof(node_state_t),
		&node_state_class);
  s->gc.type = T_NODE_STATE;
  s->state_id = 0; /* not set: will be set by compile_and_add_rule_ast() */
  s->file = NULL; /* not set: will be set by set_state_label() */
  s->line = 0; /* not set: will be set by set_state_label() */
  s->name = NULL; /* not set: will be set by set_state_label() */
  s->flags = 0; /* not set: will be set by set_state_label() */
  s->an_flags = 0;
  GC_TOUCH (ctx->gc_ctx, s->actionlist = actions);
  GC_TOUCH (ctx->gc_ctx, s->translist = transitions);
  s->mustset = VS_EMPTY;
  return s;
}


node_state_t *set_state_label(rule_compiler_t *ctx,
			      node_state_t *state,
			      symbol_token_t *sym,
			      unsigned long flags)
{
  if (sym->real_file!=NULL)
    gc_base_free (sym->real_file);
  if (state == NULL)
    {
      if (sym->filename!=NULL)
	gc_base_free (sym->filename);
      gc_base_free (sym->name);
      return NULL;
    }
  if (sym->filename!=NULL)
    {
      if (state->file!=NULL)
	gc_base_free (state->file);
      state->file = sym->filename;
    }
  state->name = sym->name;
  state->line = sym->line;
  state->flags |= flags;
  /* add state name in current compiler context */
  if (strhash_get(ctx->statenames_hash, sym->name)!=NULL)
    {
      if (sym->filename!=NULL)
	fprintf (stderr, "%s:", sym->filename);
      fprintf (stderr, "%u: error: duplicate state name '%s'.\n",
	       sym->line, sym->name);
      ctx->nerrors++;
    }
  else
    gc_strhash_add(ctx->gc_ctx, ctx->statenames_hash,
		   (gc_header_t *)state, state->name);
  return state;
}

node_varlist_t *build_varlist(rule_compiler_t *ctx, node_expr_t *first_param)
{
  node_varlist_t *vl;

  vl = gc_base_malloc (ctx->gc_ctx, sizeof (node_varlist_t));
  vl->vars_nb = 1;
  vl->size = 8;
  vl->grow = 8;
  vl->vars = gc_base_malloc (ctx->gc_ctx, 8 * sizeof (node_expr_t *));
  vl->vars[0] = first_param;
  return vl;
}


void varlist_add(rule_compiler_t *ctx, node_varlist_t *list, node_expr_t *expr)
{
  if (list->vars_nb == list->size) {
    list->size += list->grow;
    list->vars = gc_base_realloc (ctx->gc_ctx, list->vars,
				  list->size * sizeof (node_expr_t *));
  }
  list->vars[ list->vars_nb++ ] = expr;
}


node_syncvarlist_t *build_syncvarlist(rule_compiler_t *ctx,
				      char *first_syncvar)
{
  node_syncvarlist_t *vl;

  vl = gc_base_malloc (ctx->gc_ctx, sizeof (node_syncvarlist_t));
  vl->vars_nb = 1;
  vl->size = 8;
  vl->grow = 8;
  vl->vars = gc_base_malloc (ctx->gc_ctx, 8 * sizeof (char *));
  vl->vars[0] = first_syncvar;
  return vl;
}


node_syncvarlist_t *syncvarlist_add(rule_compiler_t *ctx,
				    node_syncvarlist_t *list, char *syncvar)
{
  if (list->vars_nb == list->size) {
    list->size += list->grow;
    list->vars = gc_base_realloc (ctx->gc_ctx, list->vars,
				  list->size * sizeof (char *));
  }
  list->vars[list->vars_nb++] = syncvar;
  return list;
}

/*
node_expr_t *build_unconditional_transition(rule_compiler_t *ctx,
					    char *dest)
{
  node_trans_t *n;
  node_expr_t *nl;

  GC_START(ctx->gc_ctx, 1);
  n = build_direct_transition(NULL, dest);
  GC_UPDATE(ctx->gc_ctx, 0, n);
  nl = build_expr_cons (ctx, n, NULL);
  GC_END(ctx->gc_ctx);
  return nl;
}
*/

static void check_expect_condition(rule_compiler_t *ctx, node_expr_t *e)
{
  if (e==NULL)
    return;
  switch (e->type)
    {
    case NODE_FIELD:
    case NODE_CONST:
    case NODE_VARIABLE:
    case NODE_BREAK:
      break;
    case NODE_CALL:
      {
	monotony (*cm) (rule_compiler_t *ctx,
			node_expr_t *e,
			monotony args[]);
	size_t i, n;
	monotony *args;
	monotony res;
	node_expr_t *l;

	for (l=CALL_PARAMS(e), n=0; l!=NULL; l=BIN_RVAL(l), n++)
	  {
	    check_expect_condition (ctx, BIN_LVAL(l));
	  }
	cm = CALL_COMPUTE_MONOTONY(e);
	if (cm==NULL)
	  res = MONO_UNKNOWN;
	else
	  {
	    args = gc_base_malloc (ctx->gc_ctx, n*sizeof(monotony));
	    for (i=0; i<n; i++)
	      args[i] = MONO_CONST;
	    res = (*cm) (ctx, e, args);
	    for (i=0; i<n; i++)
	      if (args[i] & MONO_THRASH)
		{
		  res = MONO_UNKNOWN;
		  break;
		}
	    gc_base_free (args);
	  }
	if (res!=MONO_CONST)
	  {
	    if (e->file!=NULL)
	      fprintf (stderr, "%s:", STR(e->file));
	    /* printing STR(e->file) is legal, since it
	       was created NUL-terminated, on purpose */
	  fprintf (stderr, "%u: error: cannot call side-effecting function '%s' inside 'expect' clause.\n",
		   e->lineno, CALL_SYM(e));
	  ctx->nerrors++;
	  }
	break;
      }
    case NODE_BINOP:
    case NODE_CONS:
    case NODE_EVENT:
    case NODE_COND:
      check_expect_condition (ctx, BIN_LVAL(e));
      check_expect_condition (ctx, BIN_RVAL(e));
      break;
    case NODE_DB_PATTERN:
      check_expect_condition (ctx, BIN_LVAL(e));
      {
	node_expr_t *l;

	for (l=BIN_RVAL(e); l!=NULL; l=BIN_RVAL(l))
	  if (!is_logical_variable(BIN_LVAL(l)))
	    check_expect_condition (ctx, BIN_LVAL(l));
      }
      break;
    case NODE_DB_COLLECT:
      {
	e = BIN_RVAL(e); /* skip 'returns' part */
	check_expect_condition (ctx, BIN_LVAL(e));
	check_expect_condition (ctx, BIN_RVAL(e));
      }
      break;
    case NODE_DB_SINGLETON:
      {
	node_expr_t *l;

	for (l=MON_VAL(e); l!=NULL; l=BIN_RVAL(l))
	  check_expect_condition (ctx, BIN_LVAL(l));
      }
      break;
    case NODE_RETURN:
    case NODE_MONOP:
      check_expect_condition (ctx, MON_VAL(e));
      break;
    case NODE_REGSPLIT:
      check_expect_condition (ctx, REGSPLIT_STRING(e));
      if (REGSPLIT_DEST_VARS(e)!=NULL && REGSPLIT_DEST_VARS(e)->vars_nb!=0)
	{
	  if (e->file!=NULL)
	    fprintf (stderr, "%s:", STR(e->file));
	  /* printing STR(e->file) is legal, since it
	     was created NUL-terminated, on purpose */
	  fprintf (stderr, "%u: error: cannot assign to variable%s inside 'expect' clause.\n",
		   e->lineno, REGSPLIT_DEST_VARS(e)->vars_nb==1?"":"s");
	  ctx->nerrors++;
	}
      break;
    case NODE_IFSTMT:
      check_expect_condition (ctx, IF_COND(e));
      check_expect_condition (ctx, IF_THEN(e));
      check_expect_condition (ctx, IF_ELSE(e));
      break;
    case NODE_ASSOC:
      check_expect_condition (ctx, BIN_RVAL(e));
      if (e->file!=NULL)
	fprintf (stderr, "%s:", STR(e->file));
      /* printing STR(e->file) is legal, since it
	 was created NUL-terminated, on purpose */
      fprintf (stderr, "%u: error: cannot assign to variable inside 'expect' clause.\n",
	       e->lineno);
      ctx->nerrors++;
      break;
    default:
      DebugLog(DF_OLC, DS_FATAL,
	       "unrecognized node type %d\n", e->type);
      exit(EXIT_FAILURE);
      break;
    }
}

/* functions used to compute the hash code of every node_expr_t: */
static unsigned long h_mul (unsigned long x, unsigned long y)
{ /* multiplication mod bigprime;
     this is native, and essentially meant not to overflow
     sizeof(unsigned long) */
  unsigned long z;

  if (x==0 || y==0)
    return 0;
  if (y < x)
    {
      z = x;
      x = y;
      y = z;
    }
  z = h_mul (x, y>>1) << 1;
  if (z>=bigprime)
    z -= bigprime; /* reduce mod bigprime */
  if (y & 1)
    {
      z += x;
      if (z>=bigprime)
	z -= bigprime; /* reduce mod bigprime */
    }
  return z;
}

/*
static unsigned long h_exp (unsigned long x, unsigned long n)
{
  unsigned long res;

  if (x==0)
    return 0L;
  if (n==0)
    return 1L;
  res = h_exp (x, n>>1);
  res = h_mul (res, res);
  if (n & 1)
    res = h_mul (res, x);
  return res;
}
*/

static unsigned long h_pair (unsigned long m, unsigned long n)
{ /* There is a well-known bijection from N x N to N:
     <m, n> = (m+n)(m+n+1)/2 + m
     Here we compute <m, n> mod bigprime
  */
  unsigned long sum, sump1, prod, res;

  sum = m+n;
  if (sum>=bigprime)
    sum -= bigprime;
  sump1 = sum+1;
  if (sump1>=bigprime)
    sump1 -= bigprime;
  prod = h_mul (sum, sump1);
  /* prod should be even (mod bigprime)
     If it is even at all, compute prod/2
     Otherwise, compute (prod+bigprime)/2
     Note that <_, _> is a morphism mod bigprime, since
     bigprime is odd: this follows from the fact that
     k -> k(k+1)/2 is a morphism mod bigprime
     (if k=k' mod bigprime, say k' = k + a.bigprime,
     then k'(k'+1)/2
     = (k+a.bigprime)(k+a.bigprime+1)/2
     = k(k+a.bigprime+1)/2 + a.bigprime(k+a.bigprime+1)/2
     = k(k+1)/2 + ka.bigprime/2 + a.bigprime(k+a.bigprime+1)/2
     = k(k+1)/2 + [2ka+a^2.bigprime+a)].bigprime/2;
     then note that 2ka is even, and a^2.bigprime+a mod 2=a^2+a mod 2
     since bigprime is odd, and a^2+a mod 2 = 0 mod 2.)
   */
  if (prod & 1)
    res = (prod + bigprime) >> 1;
  else res = prod >> 1;
  res += m;
  if (res>=bigprime)
    res -= bigprime;
  return res;
}

#define H_NULL (bigprime-1)  /* NULL is encoded as (-1) mod bigprime */

static unsigned long h_cons (node_expr_t *e, unsigned long l)
{
  if (e==NULL)
    return h_pair (H_NULL, l);
  return h_pair (e->hash, l);
}

static void add_parent (rule_compiler_t *ctx, node_expr_t *node,
			node_expr_t *parent);

static void add_boolean_check (rule_compiler_t *ctx,
			       node_expr_t *expr)
{
  node_expr_if_t *bool_check;

  GC_START(ctx->gc_ctx, 1);
  bool_check = gc_alloc (ctx->gc_ctx, sizeof(node_expr_if_t),
			 &node_expr_ifstmt_class);
  bool_check->gc.type = T_NODE_IFSTMT;
  bool_check->type = NODE_IFSTMT;
  GC_TOUCH (ctx->gc_ctx, bool_check->file = expr->file);
  bool_check->lineno = expr->lineno;
  bool_check->hash = h_pair (NODE_IFSTMT,
			     h_cons (expr,
				     h_cons (NULL,
					     h_cons (NULL,
						     H_NULL))));
  /* hash = '(NODE_IFSTMT expr NULL NULL)' mod bigprime */
  bool_check->stype = NULL; /* not known yet */
  bool_check->compute_stype = CS_IFSTMT;
  bool_check->parents = NULL;
  GC_TOUCH (ctx->gc_ctx, bool_check->cond = expr);
  bool_check->then = NULL;
  bool_check->els = NULL;
  GC_UPDATE(ctx->gc_ctx, 0, bool_check);
  add_parent (ctx, expr, (node_expr_t *)bool_check);
  GC_END(ctx->gc_ctx);			 
}

node_trans_t *build_direct_transition(rule_compiler_t *ctx,
				      node_expr_t *cond,
				      symbol_token_t *sym)
{
  node_trans_t *trans;
  char *file;
  unsigned int line;

  if (sym->real_file!=NULL)
    gc_base_free (sym->real_file);
  if (cond!=NULL)
    {
      file = gc_strdup (ctx->gc_ctx, STR(cond->file));
      /* STR(cond->file) was NUL-terminated by construction */
      line = cond->lineno;
    }
  else
    {
      file = sym->filename;
      line = sym->line;
    }
  GC_START(ctx->gc_ctx,1);
  trans = gc_alloc (ctx->gc_ctx, sizeof(node_trans_t),
		    &node_trans_class);
  trans->gc.type = T_NODE_TRANS;
  trans->type = -1;
  GC_TOUCH (ctx->gc_ctx, trans->cond = cond);
  trans->file = file;
  trans->lineno = line;
  trans->dest = sym->name;
  trans->sub_state_dest = NULL; /* not set */
  trans->an_flags = 0;
  GC_UPDATE(ctx->gc_ctx, 0, trans);
  check_expect_condition (ctx, cond);
  if (cond!=NULL)
    add_boolean_check (ctx, cond);
  GC_END(ctx->gc_ctx);
  return trans;
}


node_trans_t *build_indirect_transition(rule_compiler_t *ctx,
					node_expr_t *cond,
					node_state_t *substate)
{
  node_trans_t *trans;
  char *file;
  unsigned int line;

  if (cond!=NULL)
    {
      file = gc_strdup (ctx->gc_ctx, STR(cond->file));
      /* STR(cond->file) was NUL-terminated by construction */
      line = cond->lineno;
    }
  else
    {
      file = NULL;
      line = -1;
    }
  trans = gc_alloc (ctx->gc_ctx, sizeof(node_trans_t),
		    &node_trans_class);
  trans->gc.type = T_NODE_TRANS;
  trans->type = -1;
  GC_TOUCH (ctx->gc_ctx, trans->cond = cond);
  check_expect_condition (ctx, cond);
  trans->dest = NULL; /* not set */
  trans->file = file;
  trans->lineno = line;
  GC_TOUCH (ctx->gc_ctx, trans->sub_state_dest = substate);
  trans->an_flags = 0;
  return trans;
}

node_rule_t *build_rule(rule_compiler_t *ctx,
			symbol_token_t   *sym,
			node_state_t     *init_state,
			node_expr_t *states,
			node_syncvarlist_t *sync_vars)
{
  node_rule_t *new_rule;

  new_rule = gc_alloc (ctx->gc_ctx, sizeof(node_rule_t),
		       &node_rule_class);
  new_rule->gc.type = T_NODE_RULE;
  new_rule->name = sym->name;
  new_rule->real_file = sym->real_file;
  new_rule->filename = sym->filename;
  new_rule->line = sym->line;
  GC_TOUCH (ctx->gc_ctx, new_rule->init = init_state);
  GC_TOUCH (ctx->gc_ctx, new_rule->statelist = states);
  new_rule->sync_vars = sync_vars;
  new_rule->complexity_degree = ULONG_MAX; /* by default, at worst exponential */
  return new_rule;
}

/*
** expression building functions...
*/

typedef struct type_heap_s type_heap_t;
struct type_heap_s {
  gc_header_t gc;
  node_expr_t *e;
  type_t *stype;
  type_heap_t *left;
  type_heap_t *right;
} ;

static void type_heap_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  type_heap_t *h = (type_heap_t *)p;

  GC_TOUCH (gc_ctx, h->e);
  GC_TOUCH (gc_ctx, h->left);
  GC_TOUCH (gc_ctx, h->right);
}

static int type_heap_traverse (gc_traverse_ctx_t *gtc, gc_header_t *p,
			       void *data)
{
  type_heap_t *h = (type_heap_t *)p;
  int err = 0;

  err = (*gtc->do_subfield) (gtc, (gc_header_t *)h->e, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *)h->left, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *)h->right, data);
  return err;
}

static size_t type_heap_count (type_heap_t *h)
{
  if (h==NULL)
    return 0;
  return 1 + type_heap_count (h->left) + type_heap_count (h->right);
}

static int type_heap_save_recursive (save_ctx_t *sctx, type_heap_t *h)
{
  type_t *stype;
  int err;
  
  if (h==NULL)
    return 0;
  err = save_gc_struct (sctx, (gc_header_t *)h->e);
  if (err) return err;
  stype = h->stype;
  if (stype==NULL)
    {
      if (putc_unlocked (T_NOTHING, sctx->f) < 0) { return errno; }
      err = save_string (sctx, NULL);
    }
  else
    {
      if (putc_unlocked ((int)stype->tag, sctx->f) < 0) { return errno; }
      err = save_string (sctx, stype->name);
    }
  if (err) return err;
  err = type_heap_save_recursive (sctx, h->left);
  if (err) return err;
  err = type_heap_save_recursive (sctx, h->right);
  return err;
}

static int type_heap_save (save_ctx_t *sctx, gc_header_t *p)
{
  type_heap_t *h = (type_heap_t *)p;
  size_t n;
  int err;

  n = type_heap_count (h);
  err = save_size_t (sctx, n);
  if (err) return err;
  err = type_heap_save_recursive (sctx, h);
  return err;
}

gc_class_t type_heap_class;

static void push_type_node (gc_t *gc_ctx, type_heap_t **rtp,
			    node_expr_t *e, type_t *t);

static gc_header_t *type_heap_restore (restore_ctx_t *rctx)
{
  gc_t *gc_ctx = rctx->gc_ctx;
  type_heap_t *h;
  node_expr_t *e;
  char *typename;
  type_t *stype;
  size_t i, n;
  unsigned char tag;
  int c;
  int err;

  GC_START (gc_ctx, 2);
  h = NULL;
  /* GC_UPDATE (gc_ctx, 1, NULL); */
  err = restore_size_t (rctx, &n);
  if (err) { errno = err; goto end; }
  for (i=0; i<n; i++)
    {
      e = (node_expr_t *)restore_gc_struct (rctx);
      GC_UPDATE (gc_ctx, 0, e);
      if (e==NULL && errno!=0) { h = NULL; goto end; }
      c = getc_unlocked (rctx->f);
      if (c==EOF) { h = NULL; goto end; }
      tag = (unsigned char)c;
      err = restore_string (rctx, &typename);
      if (err) { errno = err; h = NULL; goto end; }
      if (tag==T_NOTHING)
	{
	  if (typename!=NULL)
	    gc_base_free (typename);
	  stype = NULL;
	}
      else
	{
	  if (typename==NULL) { errno = -2; h = NULL; goto end; }
	  stype = stype_from_string (gc_ctx, typename, 1, tag);
	  gc_base_free (typename);
	}
      push_type_node (gc_ctx, (type_heap_t **)&GC_LOOKUP(1), e, stype);
    }
  h = GC_LOOKUP(1);
 end:
  GC_END (gc_ctx);
  return (gc_header_t *)h;
}

gc_class_t type_heap_class = {
  GC_ID('t','p','h','p'),
  type_heap_mark_subfields,
  NULL,
  type_heap_traverse,
  type_heap_save,
  type_heap_restore
};

/* Priority queues, implemented as skew heaps */

static int expr_node_cmp (node_expr_t *e1, node_expr_t *e2)
{
  int cmp;

  if (e1==NULL)
    {
      if (e2==NULL)
	return 0;
      return -1;
    }
  if (e2==NULL)
    return 1;
  if (e1->file!=NULL && e2->file==NULL)
    return -1; /* prefer to treat named files first */
  if (e1->file==NULL && e2->file!=NULL)
    return 1;
  if (e1->file!=NULL && e2->file!=NULL)
    {
      cmp = strcmp (STR(e1->file), STR(e2->file));
      /* STR() is legal, because since e1->file and e2->file
	 were created NUL-terminated, on purpose */
      if (cmp)
	return cmp;
    }
  if (e1->lineno < e2->lineno)
    return -1;
  if (e1->lineno > e2->lineno)
    return 1;
  return (e1 - e2);
}

static void push_type_node (gc_t *gc_ctx, type_heap_t **rtp,
			    node_expr_t *e, type_t *t)
{
  type_heap_t *rt;
  type_heap_t *left;
  node_expr_t *olde;
  type_t *oldt;
  int cmp;

  GC_START(gc_ctx, 1);
  while ((rt = *rtp) != NULL)
    {
      /* Swap left and right.
       In principle we should GC_TOUCH() after each of the following
      two assignments, but this is not needed.  The only purpose of
      GC_TOUCH() is to make sure no black object points to a white
      object.  But if rt is black then neither rt->left nor rt->right
      will be white before the swap, hence also, trivially, after
      the swap. */
      left = rt->left;
      rt->left = rt->right;
      rt->right = left;

      cmp = expr_node_cmp(e, rt->e);
      if (cmp < 0)
	{ /* Store e into root, then insert the old
	     value of rt->entry into rt->left subheap */
	  olde = rt->e;
	  oldt = rt->stype;
	  GC_UPDATE(gc_ctx, 0, olde);
	  GC_TOUCH (gc_ctx, rt->e = e);
	  rt->stype = t;
	  e = olde;
	  t = oldt;
	  rtp = &rt->left;
	}
      else
	{ /* Else insert he into rt->left subheap */
	  rtp = &rt->left;
	}
    }
  rt = gc_alloc (gc_ctx, sizeof(type_heap_t), &type_heap_class);
  rt->gc.type = T_TYPE_HEAP;
  GC_TOUCH (gc_ctx, rt->e = e);
  rt->stype = t;
  rt->left = NULL;
  rt->right = NULL;
  GC_TOUCH (gc_ctx, *rtp = rt);
  GC_END(gc_ctx);
}


static void type_heap_merge (gc_t *gc_ctx, type_heap_t *h1, type_heap_t *h2,
			     type_heap_t **res)
{
  type_heap_t *left;

  while (1)
    {
      if (h2==NULL)
	{
	  GC_TOUCH (gc_ctx, *res = h1);
	  return;
	}
      if (h1==NULL)
	{
	  GC_TOUCH (gc_ctx, *res = h2);
	  return;
	}
      if (expr_node_cmp (h1->e, h2->e) < 0)
	{
	  /* We shall return h1, which is first. */
	  GC_TOUCH (gc_ctx, *res = h1);
	  /* First exchange the left and right parts of h1.
	   No need to GC_TOUCH() here, see 'Swap left and right' comment
	   in push_type_node(). */
	  left = h1->right;
	  h1->right = h1->left;
	  h1->left = left;
	  /* Next, merge the old right part (left now) with h2 */
	  res = &h1->left;
	  h1 = left;
	  /* h2 unchanged; then loop */
	}
      else
	{ /* In this other case, we shall instead return h2. */
	  GC_TOUCH (gc_ctx, *res = h2);
	  /* First (again) exchange its left and right parts.
	   No need to GC_TOUCH() here, see 'Swap left and right' comment
	   in push_type_node(). */
	  left = h2->right;
	  h2->right = h2->left;
	  h2->left = left;
	  /* Next merge the old right part (left now) with h1...
	     swapping the roles of h1 and h2 in the process. */
	  res = &h2->left;
	  h2 = h1;
	  h1 = left;
	}
    }
}

static node_expr_t *pop_type_node (gc_t *gc_ctx, type_heap_t **rtp,
				   type_t **stypep)
{
  type_heap_t *rt = *rtp;
  node_expr_t *e;

  if (rt==NULL)
    return NULL;
  GC_START(gc_ctx, 1);
  e = rt->e;
  *stypep = rt->stype;
  GC_UPDATE(gc_ctx, 0, e);
  type_heap_merge (gc_ctx, rt->left, rt->right, rtp);
  GC_END(gc_ctx);
  return e;
}

static type_t *stype_join (type_t *t1, type_t *t2)
{
  /* NULL is bottom: */
  if (t1==NULL)
    return t2;
  if (t2==NULL)
    return t1;
  /* if t1==t2, this is their join: */
  if (strcmp(t1->name, t2->name)==0)
    return t1;
  /* "*" is top: */
  if (strcmp(t1->name, "*")==0)
    return &t_any;
  if (strcmp(t2->name, "*")==0)
    return &t_any;
  /* database types: db[*] <= db[type1, ..., typen]
  */
  if (strcmp(t1->name, "db[*]")==0 && memcmp(t2->name, "db[", 3)==0)
    return t2;
  if (strcmp(t2->name, "db[*]")==0 && memcmp(t1->name, "db[", 3)==0)
    return t1;
  return &t_any; /* error---equated with top, "*" */
}

static int stype_below (type_t *t, type_t *model)
{
  if (t==NULL) /* NULL is bottom */
    return 1;
  if (model==NULL)
    return 0;
  if (strcmp(model->name, "*")==0) /* "*" is top */
    return 1;
  if (strcmp(t->name, model->name)==0)
    return 1;
  /* database types: db[*] <= db[type1, ..., typen] */
  if (strcmp(t->name, "db[*]")==0 && memcmp(model->name, "db[", 3)==0)
    return 1;
  return 0;
}

static void set_type (rule_compiler_t *ctx, node_expr_t *myself,
		      type_t *stype)
{
  if (stype_below (stype, myself->stype))
    return;
  /* Now add myself to the stack of nodes whose type we now know */
  push_type_node (ctx->gc_ctx, &ctx->type_stack, myself, stype);
}

static void add_parent (rule_compiler_t *ctx, node_expr_t *node,
			node_expr_t *parent)
{
  node_expr_t *l;

  l = build_expr_cons (ctx, parent, node->parents);
  GC_TOUCH (ctx->gc_ctx, node->parents = l);
}

static void compute_stype_string_split (rule_compiler_t *ctx, node_expr_t *myself)
{
  node_expr_regsplit_t *n = (node_expr_regsplit_t *)myself;
  node_expr_t *str;
  node_varlist_t *vars;
  size_t i, j, nb;
  node_expr_t *var, *varj;
  type_t *vartype;

  set_type (ctx, myself, &t_void);
  str = n->string;
  if (!stype_below(str->stype, &t_str))
    {
      if (str->file!=NULL)
	fprintf (stderr, "%s:", STR(str->file));
      /* printing STR(str->file) is legal, since it
	 was created NUL-terminated, on purpose */
      fprintf (stderr, "%u: type error: expected str, got %s.\n",
	       str->lineno,
	       str->stype->name);
      ctx->nerrors++;
    }
  if (str->stype==NULL || str->stype==&t_any)
    return; /* stop here if we have not yet learned that str was of string "str",
	       or if a type error occurred in typing str */
  vars = n->dest_vars;
  if (vars!=NULL)
    {
      nb = vars->vars_nb;
      for (i=0; i<nb; i++)
	{
	  var = vars->vars[i];
	  for (j=0; j<i; j++)
	    {
	      varj = vars->vars[j];
	      if (var==varj) /* == is OK, since variables are hash-consed */
		{
		  if (varj->file!=NULL)
		    fprintf (stderr, "%s:", STR(varj->file));
		  /* printing STR(varj->file) is legal, since it
		     was created NUL-terminated, on purpose */
		  fprintf (stderr, "%u: error: duplicate variable %s in split regex.\n",
			   varj->lineno,
			   var->stype->name);
		  ctx->nerrors++;
		  break;
		}
	    }
	  if (j<i) /* duplicate variable */
	    continue;
	  vartype = var->stype;
	  if (vartype==NULL)
	    {
	      set_type (ctx, var, &t_str);
	      /* set type of variable to type of expression */
	    }
	  else /* variable already has a type */
	    if (!stype_below(vartype, &t_str)) /* and the type is not str */
	      {
		if (var->file!=NULL)
		  fprintf (stderr, "%s:", STR(var->file));
		/* printing STR(var->file) is legal, since it
		   was created NUL-terminated, on purpose */
		fprintf (stderr, "%u: type error:"
			 " split regex assigns value of type str"
			 " to variable %s of type %s.\n",
			 var->lineno,
			 ((node_expr_symbol_t *)var)->name,
			 vartype->name);
		ctx->nerrors++;
	      }
	}
    }
}

static unsigned long h_varlist (node_varlist_t *vl)
{
  size_t i, n;
  unsigned long res = H_NULL;

  if (vl!=NULL)
    {
      for (n=vl->vars_nb, i=0; i<n; i++)
	res = h_cons (vl->vars[i], res);
    }
  return res;
}

node_expr_t *build_string_split(rule_compiler_t *ctx,
				node_expr_t *source,
				node_expr_t *pattern,
				node_varlist_t *dest_list)
{
  node_expr_regsplit_t *n;
  size_t i, nvars;

  GC_START(ctx->gc_ctx, 1);
  n = gc_alloc (ctx->gc_ctx, sizeof (node_expr_regsplit_t),
		&node_expr_regsplit_class);
  n->gc.type = T_NODE_REGSPLIT;
  n->type = NODE_REGSPLIT;
  GC_TOUCH (ctx->gc_ctx, n->file = pattern->file);
  n->lineno = pattern->lineno;
  n->hash = h_pair (NODE_REGSPLIT,
		    h_cons (source,
			    h_cons (pattern,
				    h_varlist (dest_list))));
  /* hash = '(NODE_REGSPLIT source pattern x1 ... xn)' mod bigprime */
  n->stype = NULL; /* not known yet */
  n->compute_stype = CS_STRING_SPLIT;
  n->parents = NULL;
  GC_TOUCH (ctx->gc_ctx, n->string = source);
  GC_TOUCH (ctx->gc_ctx, n->split_pat = pattern);
  n->dest_vars = dest_list;
  if (dest_list!=NULL)
    for (i=0, nvars = dest_list->vars_nb; i<nvars; i++)
      GC_TOUCH (ctx->gc_ctx, dest_list->vars[i]);
  GC_UPDATE(ctx->gc_ctx, 0, n);
  REGEXNUM(TERM_DATA(pattern)) = (dest_list==NULL)?0:dest_list->vars_nb;
  if (REGEXNUM(TERM_DATA(pattern))!=REGEX(TERM_DATA(pattern)).re_nsub)
    {
      if (n->file!=NULL)
	fprintf (stderr, "%s:", STR(n->file));
      /* printing STR(n->file) is legal, since it
	 was created NUL-terminated, on purpose */
      fprintf (stderr, "%u: error: split regex produces %zd substrings, bound to %d variables.\n",
	       n->lineno,
	       REGEX(TERM_DATA(pattern)).re_nsub,
	       REGEXNUM(TERM_DATA(pattern)));
      ctx->nerrors++;
    }
  add_parent (ctx, source, (node_expr_t *)n);
  GC_END(ctx->gc_ctx);
  return (node_expr_t *)n;
}

static void compute_stype_binop (rule_compiler_t *ctx, node_expr_t *myself)
{
  node_expr_bin_t *n = (node_expr_bin_t *)myself;
  type_t *ltype, *rtype, *restype;
  char *op_name;

  ltype = BIN_LVAL(n)->stype;
  rtype = BIN_RVAL(n)->stype;
  if (ltype==NULL || rtype==NULL)
    return; /* not enough info yet */
  if (ltype==&t_any || rtype==&t_any)
    {
      set_type (ctx, myself, &t_any); /* propagate type error, but don't
					 bother user with spurious messages */
      return;
    }
  switch (BIN_OP(n))
    {
    case OP_ADD:
      if (strcmp(ltype->name, "ctime")==0 &&
	  strcmp(rtype->name, "timeval")==0)
	{
	  set_type (ctx, myself, &t_timeval);
	  return;
	}
      if (strcmp(ltype->name, "str")==0 &&
	  strcmp(rtype->name, "str")==0)
	{
	  set_type (ctx, myself, &t_str);
	  return;
	}
      /*FALLTHROUGH*/ /* all the other cases are as for subtraction */
    case OP_SUB:
      if (strcmp(ltype->name, "ctime")==0)
	{
	  if (strcmp(rtype->name, "int") &&
	      strcmp(rtype->name, "ctime"))
	    goto type_error;
	  set_type (ctx, myself, &t_ctime);
	  return;
	}
      if (strcmp(ltype->name, "timeval")==0)
	{
	  if (strcmp(rtype->name, "int") &&
	      strcmp(rtype->name, "ctime") &&
	      strcmp(rtype->name, "timeval"))
	    goto type_error;
	  set_type (ctx, myself, &t_timeval);
	  return;
	}
      if (memcmp(ltype->name, "db[", 3)==0)
	{
	  restype = stype_join (ltype, rtype);
	  if (restype==&t_any)
	    goto type_error;
	  return;
	}
      /*FALLTHROUGH*/ /* all the other cases as for *, / */
    case OP_MUL:
    case OP_DIV:
      restype = stype_join (ltype, rtype);
      if (restype==&t_any)
	goto type_error;
      if (strcmp(restype->name, "int") &&
	  strcmp(restype->name, "uint") &&
	  strcmp(restype->name, "float"))
	goto type_error;
      set_type (ctx, myself, restype);
      return;
    case OP_MOD:
      restype = stype_join (ltype, rtype);
      if (restype==&t_any)
	goto type_error;
      if (strcmp(restype->name, "int") &&
	  strcmp(restype->name, "uint"))
	goto type_error;
      set_type (ctx, myself, ltype);
      return;
    case OP_AND:
    case OP_OR:
    case OP_XOR:
      if (strcmp(ltype->name, "int")==0 ||
	  strcmp(ltype->name, "uint")==0 ||
	  strcmp(ltype->name, "ipv4")==0)
	{
	  if (strcmp(rtype->name, "int") &&
	      strcmp(rtype->name, "uint") &&
	      strcmp(rtype->name, "ipv4"))
	    goto type_error;
	  set_type (ctx, myself, ltype);
	  return;
	}
      restype = stype_join (ltype, rtype);
      if (stype_below(restype, &t_ipv6))
	{
	  set_type (ctx, myself, restype);
	  return;
	}
      goto type_error;
    case OP_ADD_EVENT:
      set_type (ctx, myself, &t_void); /* OP_ADD_EVENT, used as a binop
					  tag, does not actually
					  return an event per se; it only
					  serves to build a pair (field, val)
					  Do not confuse with event nodes.
				       */
      restype = stype_join (ltype, rtype);
      if (restype==&t_any)
	{
	  if (n->file!=NULL)
	    fprintf (stderr, "%s:", STR(n->file));
	  /* printing STR(n->file) is legal, since it
	     was created NUL-terminated, on purpose */
	  fprintf (stderr, "%u: type error: field .%s expects type %s, got %s.\n",
		   BIN_LVAL(n)->lineno,
		   ((node_expr_symbol_t *)BIN_LVAL(n))->name,
		   ltype->name,
		   rtype->name);
	  ctx->nerrors++;
	}
      return;
    default:
      DebugLog(DF_OLC, DS_ERROR, "unknown binop %d.\n", BIN_OP(n));
      exit(EXIT_FAILURE);
      return;
    }
 type_error:
  if (n->file!=NULL)
    fprintf (stderr, "%s:", STR(n->file));
  /* printing STR(n->file) is legal, since it
     was created NUL-terminated, on purpose */
  switch (BIN_OP(n))
    {
    case OP_ADD: op_name = "+"; break;
    case OP_SUB: op_name = "-"; break;
    case OP_MUL: op_name = "*"; break;
    case OP_DIV: op_name = "/"; break;
    case OP_MOD: op_name = "%"; break;
    case OP_AND: op_name = "&"; break;
    case OP_OR: op_name = "|"; break;
    case OP_XOR: op_name = "^"; break;
    default: op_name = ""; break;
    }
  fprintf (stderr, "%u: type error: %s%s%s.\n",
	   n->lineno,
	   ltype->name, op_name, rtype->name);
  ctx->nerrors++;
  set_type (ctx, myself, &t_any);
}

node_expr_t *build_expr_binop(rule_compiler_t *ctx, int op,
			      node_expr_t *left_node,
			      node_expr_t *right_node)
{
  node_expr_bin_t *n;

  GC_START(ctx->gc_ctx, 1);
  n = gc_alloc (ctx->gc_ctx, sizeof(node_expr_bin_t),
		&node_expr_bin_class);
  n->gc.type = T_NODE_BINOP;
  n->type = NODE_BINOP;
  GC_TOUCH (ctx->gc_ctx, n->file = left_node->file);
  n->lineno = left_node->lineno;
  n->hash = h_pair (NODE_BINOP,
		    h_pair (op,
			    h_cons (left_node,
				    h_cons (right_node,
					    H_NULL))));
  /* hash = '(NODE_BINOP op left_node right_node)' mod bigprime */
  n->stype = NULL; /* not known yet */
  n->compute_stype = CS_BINOP;
  n->parents = NULL;
  n->op = op;
  GC_TOUCH (ctx->gc_ctx, n->lval = left_node);
  GC_TOUCH (ctx->gc_ctx, n->rval = right_node);
  GC_UPDATE(ctx->gc_ctx, 0, n);
  add_parent (ctx, left_node, (node_expr_t *)n);
  add_parent (ctx, right_node, (node_expr_t *)n);
  GC_END(ctx->gc_ctx);
  return (node_expr_t *)n;
}

static void compute_stype_monop (rule_compiler_t *ctx, node_expr_t *myself)
{
  node_expr_mon_t *n = (node_expr_mon_t *)myself;
  type_t *atype;
  char *op_name;

  atype = MON_VAL(n)->stype;
  if (atype==NULL)
    {
      return; /* not enough info yet */
    }
  if (atype==&t_any)
    {
      set_type (ctx, myself, &t_any); /* propagate error, but do not emit
					 any spurious error message */
      return;
    }
  switch (MON_OP(n))
    {
    case OP_OPP:
    case OP_PLUS:
      if (strcmp(atype->name, "int")==0 ||
	  strcmp(atype->name, "uint")==0)
	{
	  set_type (ctx, myself, &t_int); // yes, converts uints to ints, in particular
	  return;
	}
      if (strcmp(atype->name, "float")==0)
	{
	  set_type (ctx, myself, &t_float);
	  return;
	}
      break;
    case OP_NOP:
      set_type (ctx, myself, atype);
      return;
    case OP_NOT:
      if (strcmp(atype->name, "int")==0 ||
	  strcmp(atype->name, "uint")==0 ||
	  strcmp(atype->name, "ipv4")==0 ||
	  strcmp(atype->name, "ipv6")==0)
	{
	  set_type (ctx, myself, atype);
	  return;
	}
      break;
    default:
      DebugLog(DF_OLC, DS_ERROR, "unknown monop %d.\n", BIN_OP(n));
      exit(EXIT_FAILURE);
      return;
    }
  /* If we arrive here, this is a type error */
  if (n->file!=NULL)
    fprintf (stderr, "%s:", STR(n->file));
  /* printing STR(n->file) is legal, since it
     was created NUL-terminated, on purpose */
  switch (MON_OP(n))
    {
    case OP_OPP: op_name = "-"; break;
    case OP_PLUS: op_name = "+"; break;
    case OP_NOT: op_name = "~"; break;
    default: op_name = ""; break;
    }
  fprintf (stderr, "%u: type error: %s%s.\n",
	   n->lineno,
	   op_name, atype->name);
  ctx->nerrors++;
  set_type (ctx, myself, &t_any);
}

node_expr_t *build_expr_monop(rule_compiler_t *ctx, int op,
			      node_expr_t *arg_node)
{
  node_expr_mon_t *n;

  GC_START(ctx->gc_ctx, 1);
  n = gc_alloc (ctx->gc_ctx, sizeof(node_expr_mon_t),
		&node_expr_mon_class);
  n->gc.type = T_NODE_MONOP;
  n->type = NODE_MONOP;
  GC_TOUCH (ctx->gc_ctx, n->file = arg_node->file);
  n->lineno = arg_node->lineno;
  n->hash = h_pair (NODE_MONOP,
		    h_pair (op,
			    h_cons (arg_node, H_NULL)));
  /* hash = '(NODE_MONOP op arg_node)' mod bigprime */
  n->stype = NULL; /* not known yet */
  n->compute_stype = CS_MONOP;
  n->parents = NULL;
  n->op = op;
  GC_TOUCH (ctx->gc_ctx, n->val = arg_node);
  GC_UPDATE(ctx->gc_ctx, 0, n);
  add_parent (ctx, arg_node, (node_expr_t *)n);
  GC_END(ctx->gc_ctx);
  return (node_expr_t *)n;
}

static void compute_stype_return (rule_compiler_t *ctx, node_expr_t *myself)
{
  /*node_expr_mon_t *n = (node_expr_mon_t *)myself;*/
  /*type_t *atype;*/

  /*atype = MON_VAL(n)->stype;*/
  set_type (ctx, myself, &t_void);
}

node_expr_t *build_expr_return(rule_compiler_t *ctx,
			       node_expr_t *arg_node)
{
  node_expr_mon_t *n;
  node_expr_t *l, *rest;

  GC_START(ctx->gc_ctx, 1);
  n = gc_alloc (ctx->gc_ctx, sizeof(node_expr_mon_t),
		&node_expr_mon_class);
  n->gc.type = T_NODE_RETURN;
  n->type = NODE_RETURN;
  GC_TOUCH (ctx->gc_ctx, n->file = arg_node->file);
  n->lineno = arg_node->lineno;
  n->hash = h_pair (NODE_RETURN,
		    h_cons (arg_node, H_NULL));
  /* hash = '(NODE_RETURN arg_node)' mod bigprime */
  n->stype = NULL; /* not known yet */
  n->compute_stype = CS_RETURN;
  n->parents = NULL;
  n->op = -1;
  GC_TOUCH (ctx->gc_ctx, n->val = arg_node);
  GC_UPDATE(ctx->gc_ctx, 0, n);
  add_parent (ctx, arg_node, (node_expr_t *)n);
  if (ctx->returns==NULL)
    {
      if (arg_node->file!=NULL)
	fprintf (stderr, "%s:", STR(arg_node->file));
      /* STR() is NUL-terminated by construction */
      fprintf (stderr, "%u: error: return not in scope of a database collect expression.\n",
	       arg_node->lineno);
      ctx->nerrors++;
    }
  else
    {
      rest = BIN_LVAL(ctx->returns);
      l = build_expr_cons (ctx, (node_expr_t *)n, rest);
      GC_TOUCH (ctx->gc_ctx, BIN_LVAL(ctx->returns) = l);
      l->hash = h_cons ((node_expr_t *)n, (rest==NULL)?H_NULL:rest->hash);
    }
  GC_END(ctx->gc_ctx);
  return (node_expr_t *)n;
}

static void compute_stype_break (rule_compiler_t *ctx, node_expr_t *myself)
{
  set_type (ctx, myself, &t_void);
}

node_expr_t *build_expr_break(rule_compiler_t *ctx)
{
  node_expr_mon_t *n;
  node_expr_t *l, *rest;

  GC_START(ctx->gc_ctx, 1);
  n = gc_alloc (ctx->gc_ctx, sizeof(node_expr_mon_t),
		&node_expr_mon_class);
  n->gc.type = T_NODE_BREAK;
  n->type = NODE_BREAK;
  GC_TOUCH (ctx->gc_ctx, n->file = ctx->currfile);
  n->lineno = ctx->issdllineno;
  n->hash = h_pair (NODE_BREAK, H_NULL);
  /* hash = '(NODE_BREAK)' mod bigprime */
  n->stype = NULL; /* not known yet */
  n->compute_stype = CS_BREAK;
  n->parents = NULL;
  n->op = -1;
  n->val = NULL;
  GC_UPDATE(ctx->gc_ctx, 0, n);
  if (ctx->returns==NULL)
    {
      if (ctx->currfile!=NULL)
	fprintf (stderr, "%s:", STR(ctx->currfile));
      /* STR() is NUL-terminated by construction */
      fprintf (stderr, "%u: error: break not in scope of a database collect expression.\n",
	       ctx->issdllineno);
      ctx->nerrors++;
    }
  else
    {
      rest = BIN_LVAL(ctx->returns);
      l = build_expr_cons (ctx, (node_expr_t *)n, rest);
      GC_TOUCH (ctx->gc_ctx, BIN_LVAL(ctx->returns) = l);
      l->hash = h_cons ((node_expr_t *)n, (rest==NULL)?H_NULL:rest->hash);
    }
  GC_END(ctx->gc_ctx);
  return (node_expr_t *)n;
}

node_expr_t *build_expr_cons(rule_compiler_t *ctx,
			     node_expr_t *left_node,
			     node_expr_t *right_node)
{
  node_expr_bin_t *n;

  n = gc_alloc (ctx->gc_ctx, sizeof(node_expr_bin_t),
		&node_expr_bin_class);
  n->gc.type = T_NODE_CONS;
  n->type = NODE_CONS;
  n->file = NULL;
  n->lineno = 0;
  n->hash = h_cons (left_node, (right_node==NULL)?H_NULL:right_node->hash);
  n->stype = NULL;
  n->compute_stype = CS_NULL;
  n->parents = NULL;
  n->op = 0;
  GC_TOUCH (ctx->gc_ctx, n->lval = left_node);
  GC_TOUCH (ctx->gc_ctx, n->rval = right_node);
  return (node_expr_t *)n;
}

node_expr_t *expr_cons_reverse(rule_compiler_t *ctx, node_expr_t *l)
{
  node_expr_t *res = NULL;
  node_expr_t *next;

  while (l!=NULL)
    {
      next = BIN_RVAL(l);
      GC_TOUCH (ctx->gc_ctx, BIN_RVAL(l) = res);
      res = l;
      l = next;
    }
  return res;
}

static void compute_stype_action (rule_compiler_t *ctx, node_expr_t *myself)
{
  /* just discard the result of the expression */
  set_type (ctx, myself, &t_void);
}

node_expr_t *build_expr_action(rule_compiler_t *ctx,
			       node_expr_t *action,
			       node_expr_t *rest)
{
  node_expr_t *n;

  GC_START(ctx->gc_ctx, 1);
  n = build_expr_cons (ctx, action, rest);
  GC_UPDATE(ctx->gc_ctx, 0, n);
  n->compute_stype = CS_ACTION;
  add_parent (ctx, action, (node_expr_t *)n);
  GC_END(ctx->gc_ctx);
  return n;
}

static void compute_stype_cond (rule_compiler_t *ctx, node_expr_t *myself)
{
  node_expr_bin_t *n = (node_expr_bin_t *)myself;
  type_t *ltype, *rtype, *restype;
  char *op_name;

  set_type (ctx, myself, &t_int); /* return type is boolean, whatever happens */
  if (BIN_LVAL(n)==NULL) /* unary operators, such as BANG */
    {
      ltype = NULL;
    }
  else
    {
      ltype = BIN_LVAL(n)->stype;
      if (ltype==NULL)
	return;
    }
  rtype = BIN_RVAL(n)->stype;
  if (rtype==NULL)
    return;
  if (ltype==&t_any || rtype==&t_any)
    return;
  switch (BIN_OP(n))
    {
    case ANDAND:
      op_name = "&&";
      goto bin_cond;
    case OROR:
      op_name = "||";
    bin_cond:
      if (!stype_below(ltype, &t_int) || !stype_below(rtype, &t_int))
	{
	  if (n->file!=NULL)
	    fprintf (stderr, "%s:", STR(n->file));
	  /* printing STR(n->file) is legal, since it
	     was created NUL-terminated, on purpose */
	  fprintf (stderr, "%u: type error: %s %s %s.\n",
		   n->lineno,
		   ltype->name, op_name, rtype->name);
	  ctx->nerrors++;
	}
      break;
    case BANG:
      if (!stype_below(rtype, &t_int))
	{
	  if (n->file!=NULL)
	    fprintf (stderr, "%s:", STR(n->file));
	  /* printing STR(n->file) is legal, since it
	     was created NUL-terminated, on purpose */
	  fprintf (stderr, "%u: type error: !%s.\n",
		   n->lineno, rtype->name);
	  ctx->nerrors++;
	}
      break;
    case OP_CRM:
      op_name = "=~";
      goto crm;
    case OP_CNRM:
      op_name = "!~";
    crm:
      if (!stype_below(ltype, &t_str) || !stype_below(rtype, &t_regex))
	{
	  if (n->file!=NULL)
	    fprintf (stderr, "%s:", STR(n->file));
	  /* printing STR(n->file) is legal, since it
	     was created NUL-terminated, on purpose */
	  fprintf (stderr, "%u: type error: %s %s %s.\n",
		   n->lineno,
		   ltype->name, op_name, rtype->name);
	  ctx->nerrors++;
	}
      break;
      /* comparisons: int, uint, float, str, ctime, ipv4, ipv6, timeval
       the two types should be equal; only OP_CEQ and OP_CNEQ make
       sense for ipv4 and ipv6 */
    case OP_CEQ:
      op_name = "==";
      goto ceq;
    case OP_CNEQ:
      op_name = "!=";
    ceq:
      if (stype_below(ltype, &t_ipv4)==0 && stype_below(rtype, &t_ipv4)==0)
	break;
      if (stype_below(ltype, &t_ipv6)==0 && stype_below(rtype, &t_ipv6)==0)
	break;
      goto cgt;
    case OP_CGT:
      op_name = ">";
      goto cgt;
    case OP_CLT:
      op_name = "<";
      goto cgt;
    case OP_CGE:
      op_name = ">=";
      goto cgt;
    case OP_CLE:
      op_name = "<=";
    cgt:
      restype = stype_join (ltype, rtype);
      if (restype==&t_any ||
	  (strcmp(restype->name, "int") &&
	   strcmp(restype->name, "uint") &&
	   strcmp(restype->name, "float") &&
	   strcmp(restype->name, "str") &&
	   strcmp(restype->name, "ctime") &&
	   strcmp(restype->name, "timeval") &&
	   memcmp(restype->name, "db[", 3) /* OK for any database type
					      as well,
					      provided that they are equal
					      or one is db[*] */
	   ))
	{
	  if (n->file!=NULL)
	    fprintf (stderr, "%s:", STR(n->file));
	  /* printing STR(n->file) is legal, since it
	     was created NUL-terminated, on purpose */
	  fprintf (stderr, "%u: type error: %s %s %s.\n",
		   n->lineno,
		   ltype->name, op_name, rtype->name);
	  ctx->nerrors++;
	}
      break;
    default:
      DebugLog(DF_OLC, DS_ERROR, "unknown cond %d.\n", BIN_OP(n));
      exit(EXIT_FAILURE);
      break;
    }
}

node_expr_t *build_expr_cond(rule_compiler_t *ctx,
			     int op, node_expr_t *left_node,
			     node_expr_t *right_node)
{
  node_expr_bin_t *n;

  GC_START(ctx->gc_ctx, 1);
  n = gc_alloc (ctx->gc_ctx, sizeof(node_expr_bin_t),
		&node_expr_bin_class);
  n->gc.type = T_NODE_COND;
  n->type = NODE_COND;
  GC_TOUCH (ctx->gc_ctx, n->file = left_node?left_node->file:right_node->file);
  n->lineno = left_node?left_node->lineno:right_node->lineno;
  n->hash = h_pair (NODE_COND,
		    h_pair (op,
			    h_cons (left_node,
				    h_cons (right_node,
					    H_NULL))));
  /* hash = '(NODE_COND op left_node right_node)' mod bigprime */
  n->stype = NULL; /* not known yet */
  n->compute_stype = CS_COND;
  n->parents = NULL;
  n->op = op;
  GC_TOUCH (ctx->gc_ctx, n->lval = left_node);
  GC_TOUCH (ctx->gc_ctx, n->rval = right_node);
  GC_UPDATE(ctx->gc_ctx, 0, n);
  if (left_node)
    add_parent (ctx, left_node, (node_expr_t *)n);
  add_parent (ctx, right_node, (node_expr_t *)n);
  GC_END(ctx->gc_ctx);
  return (node_expr_t *)n;
}

static void compute_stype_assoc (rule_compiler_t *ctx, node_expr_t *myself)
{
  node_expr_bin_t *n = (node_expr_bin_t *)myself;
  type_t *type, *vartype, *newtype;

  type = n->rval->stype;
  set_type (ctx, myself, type);
  if (type==NULL)
    return;
  vartype = n->lval->stype;
  newtype = stype_join (vartype, type);
  if (newtype==&t_any && vartype!=NULL && type!=NULL && vartype!=&t_any) /* type error */
    {
      if (n->file!=NULL)
	fprintf (stderr, "%s:", STR(n->file));
      /* printing STR(n->file) is legal, since it
	 was created NUL-terminated, on purpose */
      fprintf (stderr, "%u: type error: assigning value of type %s to variable %s of type %s.",
	       n->lineno,
	       type->name, ((node_expr_symbol_t *)n->lval)->name,
	       vartype->name);
      if (SYM_DEF(n->lval)!=NULL)
	{
	  fprintf (stderr, ", see assignment at ");
	  if (STR(SYM_DEF(n->lval)->file)!=NULL)
	    fprintf (stderr, "%s:", STR(SYM_DEF(n->lval)->file));
	  /* printing STR(SYM_DEF(n->lval)->file) is legal, since it
	     was created NUL-terminated, on purpose */
	  fprintf (stderr, "%u", SYM_DEF(n->lval)->lineno);
	}
      fprintf (stderr, ".\n");
      ctx->nerrors++;
    }
  else
    {
      GC_TOUCH (ctx->gc_ctx, SYM_DEF(n->lval) = myself);
    }
  set_type (ctx, (node_expr_t *)n->lval, newtype);
}

node_expr_t *build_assoc(rule_compiler_t *ctx, node_expr_t *nop_var,
			 node_expr_t *expr)
{
  node_expr_bin_t *n;
  node_expr_t *var;

  //gc_check(ctx->gc_ctx);
  var = MON_VAL(nop_var);
  GC_START(ctx->gc_ctx, 1);
  n = (node_expr_bin_t *)gc_alloc (ctx->gc_ctx, sizeof(node_expr_bin_t),
				   &node_expr_bin_class);
  n->gc.type = T_NODE_ASSOC;
  n->type = NODE_ASSOC;
  GC_TOUCH (ctx->gc_ctx, n->file = nop_var->file);
  n->lineno = nop_var->lineno;
  n->hash = h_pair (NODE_ASSOC,
		    h_cons (var,
			    h_cons (expr, H_NULL)));
  /* hash = '(NODE_ASSOC var expr)' mod bigprime */
  n->stype = NULL; /* not known yet */
  n->compute_stype = CS_ASSOC;
  n->parents = NULL;
  n->op = EQ;
  GC_TOUCH (ctx->gc_ctx, n->lval = var);
  GC_TOUCH (ctx->gc_ctx, n->rval = expr);
  //gc_check(ctx->gc_ctx);
  GC_UPDATE(ctx->gc_ctx, 0, n);
  add_parent (ctx, expr, (node_expr_t *)n);
  GC_END(ctx->gc_ctx);
  return (node_expr_t *)n;
}

static void compute_stype_call (rule_compiler_t *ctx, node_expr_t *myself)
{
  node_expr_call_t *n = (node_expr_call_t *)myself;
  issdl_function_t *f;
  size_t nparams;
  type_t ***sigs, **sig;
  node_expr_t *l, *arg;
  int i;
  
  f = n->f;
  if (f==NULL) // function was not found
    {
      set_type (ctx, myself, &t_any);
      return; // do not do anything
    }
  /* First check that no argument has type "*" */
  for (l=CALL_PARAMS(n); l!=NULL; l=BIN_RVAL(l))
    {
      arg = BIN_LVAL(l);
      if (arg->stype==&t_any)
	break;
    }
  if (l!=NULL) /* Some argument has type "*": this means
		  a type error, which we just propagate with no new error message */
    {
      set_type (ctx, myself, &t_any);
      return;
    }
  nparams = list_len (CALL_PARAMS(n));
  if (nparams!=f->args_nb)
    {
      if (n->file!=NULL)
	fprintf (stderr, "%s:", STR(n->file)); // NUL-terminated by construction
      fprintf (stderr, "%u: error: function %s expects %d arguments, but is applied to %zd arguments.\n",
	       myself->lineno, f->name, f->args_nb, nparams);
      ctx->nerrors++;
      set_type (ctx, myself, &t_any);
      return;
    }
  for (sigs=f->sigs; (sig = *sigs)!=NULL; sigs++)
    {
      for (l = CALL_PARAMS(n), i=1; l!=NULL; l=BIN_RVAL(l), i++)
	{
	  arg = BIN_LVAL(l);
	  if (!stype_below(arg->stype, sig[i]))
	    break; // incompatible types
	}
      if (l==NULL) /* went through to the end of the latter loop: signature
		      is compatible.  We have found our type. */
	{
	  set_type (ctx, myself, sig[0]);
	  break;
	}
    }
  if (sig==NULL)
    {
      char *delim = "(";

      if (n->file!=NULL)
	fprintf (stderr, "%s:", STR(n->file)); // NUL-terminated by construction
      fprintf (stderr, "%u: error: ill-typed function application %s",
	       myself->lineno, f->name);
      for (l=CALL_PARAMS(n); l!=NULL; l=BIN_RVAL(l))
	{
	  fputs (delim, stderr);
	  arg = BIN_LVAL(l);
	  if (arg->stype==NULL)
	    fputs ("null", stderr);
	  else fputs (arg->stype->name, stderr);
	  delim = ",";
	}
      fprintf (stderr, ").\n");
      ctx->nerrors++;
      set_type (ctx, myself, &t_any);
    }
}

static unsigned long h_str (char *s, size_t len)
{
  unsigned long res = H_NULL;
  char *t = s+len;

  while (t!=s)
    {
      t--;
      res = h_pair ((unsigned long)(unsigned char)*t, res);
    }
  return res;
}

static unsigned long h_string (char *s)
{
  return h_str (s, strlen(s));
}

node_expr_t *build_function_call(rule_compiler_t  *ctx,
				 symbol_token_t   *sym,
				 node_expr_t *paramlist)
{
  node_expr_call_t *call_node;
  issdl_function_t *func;
  ovm_var_t *currfile;
  size_t len;
  node_expr_t *l;

  GC_START(ctx->gc_ctx, 2);
  currfile = NULL;
  if (sym->real_file!=NULL)
    gc_base_free (sym->real_file);
  if (sym->filename!=NULL)
    {
      len = strlen(sym->filename);
      currfile = ovm_str_new (ctx->gc_ctx, len+1);
      strcpy (STR(currfile), sym->filename); /* include final NUL character, so
						as to ease printing */
      STRLEN(currfile) = len;
      GC_UPDATE (ctx->gc_ctx, 0, currfile);
      gc_base_free (sym->filename);
    }
  
  call_node = (node_expr_call_t *)gc_alloc (ctx->gc_ctx,
					    sizeof(node_expr_call_t),
					    &node_expr_call_class);
  call_node->gc.type = T_NODE_CALL;
  call_node->file = NULL;
  call_node->hash = h_pair (NODE_CALL,
			    h_pair (h_string (sym->name),
				    (paramlist==NULL)?H_NULL:
				    paramlist->hash));
  /* hash = '(NODE_CALL (n a m e) param1 ... paramn)' mod bigprime */
  call_node->parents = NULL;
  call_node->paramlist = NULL;
  GC_UPDATE(ctx->gc_ctx, 1, call_node);
  
  call_node->type = NODE_CALL;
  GC_TOUCH (ctx->gc_ctx, call_node->file = currfile);
  call_node->lineno = sym->line;
  call_node->stype = NULL; /* not known yet */
  call_node->compute_stype = CS_CALL;
  for (l=paramlist; l!=NULL; l=BIN_RVAL(l))
    add_parent (ctx, BIN_LVAL(l), (node_expr_t *)call_node);
  call_node->symbol = sym->name;
  GC_TOUCH (ctx->gc_ctx, call_node->paramlist = paramlist);
  func = strhash_get(ctx->functions_hash, sym->name);
  if (func==NULL)
    {
      if (currfile!=NULL)
	fprintf (stderr, "%s:", STR(currfile));
      /* NUL-terminated, see above */
      fprintf (stderr, "%u: error: unknown function '%s'.\n",
	       sym->line, sym->name);
      ctx->nerrors++;
    }
  call_node->f = func;
  GC_END(ctx->gc_ctx);
  return (node_expr_t *)call_node;
}

static void compute_stype_ifstmt (rule_compiler_t *ctx, node_expr_t *myself)
{
  node_expr_if_t *ifn = (node_expr_if_t *)myself;
  type_t *condtype;
  /*type_t *restype, *thentype, *elsetype;*/

  condtype = IF_COND(ifn)->stype;
  if (!stype_below(condtype, &t_int) && condtype!=&t_any)
    {
      if (myself->file!=NULL)
	fprintf (stderr, "%s:", STR(myself->file));
      /* printing STR(myself->file) is legal, since it
	 was created NUL-terminated, on purpose */
      fprintf (stderr, "%u: type error: condition of type %s, should be int.\n",
	       myself->lineno,
	       condtype->name);
      ctx->nerrors++;
    }
  set_type (ctx, myself, &t_void); /* does not return anything */
#if 0
  /* the following code is commented out; its purpose was to check
     that the two branches of the conditional had the same type;
     this would be useful for a C-like e?c1:c2 construction.
     But this is an if (e) c1 else c2 construction, where the types
     of c1 and c2 may be different (since they are types of values
     that are ignored).
  */
  if (IF_THEN(ifn)!=NULL) /* IF_THEN(ifn) may be NULL, in case of
			     nodes created by add_boolean_check(). */
    {
      thentype = IF_THEN(ifn)->stype;
      if (IF_ELSE(ifn)!=NULL)
	{
	  elsetype = IF_ELSE(ifn)->stype;
	  restype = stype_join (thentype, elsetype);
	  if (thentype!=NULL && elsetype!=NULL && restype==&t_any)
	    {
	      set_type (ctx, myself, NULL);
	      if (myself->file!=NULL)
		fprintf (stderr, "%s:", STR(myself->file));
	      /* printing STR(myself->file) is legal, since it
		 was created NUL-terminated, on purpose */
	      fprintf (stderr, "%u: type error: then branch has type %s, while else branch has type %s.\n",
		       myself->lineno,
		       thentype->name,
		       elsetype->name);
	      ctx->nerrors++;
	    }
	  set_type (ctx, myself, restype);
	}
      else
	set_type (ctx, myself, thentype);
    }
#endif
}

node_expr_t *build_expr_ifstmt(rule_compiler_t *ctx,
			       node_expr_t *cond, node_expr_t *then,
			       node_expr_t *els)
{
  node_expr_if_t *i;

  GC_START(ctx->gc_ctx, 1);
  i = gc_alloc (ctx->gc_ctx, sizeof(node_expr_if_t),
		&node_expr_ifstmt_class);
  i->gc.type = T_NODE_IFSTMT;
  i->type = NODE_IFSTMT;
  GC_TOUCH (ctx->gc_ctx, i->file = cond->file);
  i->lineno = cond->lineno;
  i->hash = h_pair (NODE_IFSTMT,
		    h_cons (cond,
			    h_cons (then,
				    h_cons (els,
					    H_NULL))));
  /* hash = '(NODE_IFSTMT cond then els)' mod bigprime */
  i->stype = NULL; /* not known yet */
  i->compute_stype = CS_IFSTMT;
  i->parents = NULL;
  GC_TOUCH (ctx->gc_ctx, i->cond = cond);
  GC_TOUCH (ctx->gc_ctx, i->then = then);
  GC_TOUCH (ctx->gc_ctx, i->els = els);
  GC_UPDATE(ctx->gc_ctx, 0, i);
  add_parent (ctx, cond, (node_expr_t *)i);
  add_parent (ctx, then, (node_expr_t *)i);
  if (els!=NULL)
    add_parent (ctx, els, (node_expr_t *)i);
  GC_END(ctx->gc_ctx);
  return (node_expr_t *)i;
}

static void compute_stype_fieldname (rule_compiler_t *ctx, node_expr_t *myself)
{
  node_expr_symbol_t *n = (node_expr_symbol_t *)myself;
  field_record_t *f;

  f = strhash_get(ctx->fields_hash, n->name);
  if (f == NULL)
    {
      if (myself->file!=NULL)
	fprintf (stderr, "%s:", STR(myself->file));
      /* printing STR(myself->file) is legal, since it
	 was created NUL-terminated, on purpose */
      fprintf (stderr, "%u: field %s not registered.\n",
	       myself->lineno, n->name);
      ctx->nerrors++;
    }
  else
    set_type (ctx, (node_expr_t *)n, f->type);
}

node_expr_t *build_fieldname(rule_compiler_t *ctx, char *fieldname)
{
  field_record_t *f;
  node_expr_symbol_t *n;

  /* check if field exists */
  /* XXX: resolution can be in the descendant phase (2) */
  f = strhash_get(ctx->fields_hash, fieldname);
  if (f == NULL) {
    if (ctx->currfile!=NULL)
      fprintf (stderr, "%s:", STR(ctx->currfile));
    /* printing STR(ctx->currfile) is legal, since it
       was created NUL-terminated, on purpose */
    fprintf (stderr, "%u: field %s not registered.\n",
	     ctx->issdllineno, fieldname);
    fflush (stderr);
    exit(EXIT_FAILURE);
  }
  GC_START(ctx->gc_ctx, 1);
  n = (node_expr_symbol_t *)gc_alloc (ctx->gc_ctx, sizeof(node_expr_symbol_t),
				      &node_expr_field_class);
  n->gc.type = T_NODE_FIELD;
  n->type = NODE_FIELD;
  GC_TOUCH (ctx->gc_ctx, n->file = ctx->currfile);
  n->lineno = ctx->issdllineno;
  n->hash = h_pair (NODE_FIELD,
		    h_string (fieldname));
  /* hash = '(NODE_FIELD n a m e)' mod bigprime */
  n->stype = NULL; /* not known yet */
  n->compute_stype = CS_FIELDNAME;
  n->parents = NULL;
  n->name = fieldname;
  n->res_id = f->id;
  n->mono = f->mono;
  n->def = NULL;
  GC_UPDATE(ctx->gc_ctx, 0, n);
  GC_END(ctx->gc_ctx);
  return (node_expr_t *)n;
}

static void compute_stype_dummy (rule_compiler_t *ctx, node_expr_t *myself)
{
  return; /* and do nothing */
}

node_expr_t *build_varname(rule_compiler_t *ctx, char *varname)
{
  node_expr_symbol_t *n;
  node_expr_t *tmp;

  /* resolve variable in environment variable hash table */
  tmp = strhash_get(ctx->rule_env, varname);
  if (tmp!=NULL)
    {
      gc_base_free (varname);
      return tmp;
    }

  n = (node_expr_symbol_t *)gc_alloc (ctx->gc_ctx, sizeof(node_expr_symbol_t),
				      &node_expr_var_class);
  n->gc.type = T_NODE_VARIABLE;
  n->type = NODE_VARIABLE;
  GC_TOUCH (ctx->gc_ctx, n->file = ctx->currfile);
  n->lineno = ctx->issdllineno;
  n->stype = NULL; /* not known yet */
  n->compute_stype = CS_DUMMY;
  n->parents = NULL;
  n->name = varname;
  n->res_id = ctx->rule_env->elmts;
  n->mono = 0;
  n->def = NULL;
  n->hash = h_pair (NODE_VARIABLE, h_pair (n->res_id, H_NULL));
  /* hash = '(NODE_VARIABLE n)' mod bigprime */
  GC_START(ctx->gc_ctx, 1);
  GC_UPDATE(ctx->gc_ctx, 0, n);
  gc_strhash_add(ctx->gc_ctx, ctx->rule_env, (gc_header_t *)n,
		 gc_strdup (ctx->gc_ctx, varname));
  dynamic_add(ctx, varname);
  GC_END(ctx->gc_ctx);
  return (node_expr_t *)n;
}

static void compute_stype_integer (rule_compiler_t *ctx, node_expr_t *myself)
{
  node_expr_t *n = (node_expr_t *)myself;

  set_type (ctx, n, &t_uint);
}

node_expr_t *build_integer(rule_compiler_t *ctx, unsigned long i)
{
  ovm_var_t    *integer;
  node_expr_term_t  *n;

  GC_START(ctx->gc_ctx, 1);
  integer = ovm_uint_new(ctx->gc_ctx, i);
  GC_UPDATE(ctx->gc_ctx, 0, integer);

  /* XXX Add hash for constant sharing here */

  n = (node_expr_term_t *) gc_alloc (ctx->gc_ctx, sizeof(node_expr_term_t),
				     &node_expr_term_class);
  n->gc.type = T_NODE_CONST;
  n->type = NODE_CONST;
  GC_TOUCH (ctx->gc_ctx, n->file = ctx->currfile);
  n->lineno = ctx->issdllineno;
  n->hash = h_pair (NODE_CONST,
		    h_pair (T_UINT,
			    h_pair (i % bigprime,
				    H_NULL)));
  /* hash = '(NODE_CONST T_UINT i)' mod bigprime */
  n->stype = NULL; /* not known yet */
  n->compute_stype = CS_INTEGER;
  n->parents = NULL;
  GC_TOUCH (ctx->gc_ctx, n->data = integer);
  n->res_id = ctx->statics_nb;
  GC_UPDATE(ctx->gc_ctx, 0, n);
  statics_add(ctx, integer);
  GC_END(ctx->gc_ctx);
  return (node_expr_t *)n;
}

static void compute_stype_double (rule_compiler_t *ctx, node_expr_t *myself)
{
  node_expr_t *n = (node_expr_t *)myself;

  set_type (ctx, n, &t_float);
}

node_expr_t *build_double(rule_compiler_t *ctx, double f)
{
  ovm_var_t    *fpdouble;
  node_expr_term_t  *n;

  GC_START(ctx->gc_ctx, 1);
  fpdouble = ovm_float_new(ctx->gc_ctx, f);
  GC_UPDATE(ctx->gc_ctx, 0, fpdouble);

  n = (node_expr_term_t *) gc_alloc (ctx->gc_ctx, sizeof(node_expr_term_t),
				     &node_expr_term_class);
  n->gc.type = T_NODE_CONST;
  n->type = NODE_CONST;
  GC_TOUCH (ctx->gc_ctx, n->file = ctx->currfile);
  n->lineno = ctx->issdllineno;
  n->hash = h_pair (NODE_CONST,
		    h_pair (T_FLOAT,
			    h_pair (h_str ((char *)&f, sizeof(double)),
				    H_NULL)));
  /* hash = '(NODE_CONST T_FLOAT byte1 ... byten)' mod bigprime */
  n->stype = NULL; /* not known yet */
  n->compute_stype = CS_DOUBLE;
  n->parents = NULL;
  GC_TOUCH (ctx->gc_ctx, n->data = fpdouble);
  n->res_id = ctx->statics_nb;
  GC_UPDATE(ctx->gc_ctx, 0, n);
  statics_add(ctx, fpdouble);
  GC_END(ctx->gc_ctx);
  return (node_expr_t *)n;
}

static void compute_stype_string (rule_compiler_t *ctx, node_expr_t *myself)
{
  node_expr_t *n = (node_expr_t *)myself;

  set_type (ctx, n, &t_str);
}

node_expr_t *build_string(rule_compiler_t *ctx, char *str)
{
  size_t       string_len;
  node_expr_term_t *n;
  ovm_var_t   *string;

  GC_START(ctx->gc_ctx, 1);
  string_len = strlen(str);
  string = ovm_str_new(ctx->gc_ctx, string_len);
  memcpy(STR(string), str, string_len);
  GC_UPDATE(ctx->gc_ctx, 0, string);

  gc_base_free(str); /* free string memory allocated by the lexer */

  /* XXX Add hash for constant sharing here */

  n = (node_expr_term_t *) gc_alloc (ctx->gc_ctx, sizeof(node_expr_term_t),
				     &node_expr_term_class);
  n->gc.type = T_NODE_CONST;
  n->type = NODE_CONST;
  GC_TOUCH (ctx->gc_ctx, n->file = ctx->currfile);
  n->lineno = ctx->issdllineno;
  n->hash = h_pair (NODE_CONST,
		    h_pair (T_STR,
			    h_string (str)));
  /* hash = '(NODE_CONST T_STR c h a r s)' mod bigprime */
  n->stype = NULL; /* not known yet */
  n->compute_stype = CS_STRING;
  n->parents = NULL;
  GC_TOUCH(ctx->gc_ctx, n->data = string);
  n->res_id = ctx->statics_nb;
  GC_UPDATE(ctx->gc_ctx, 0, n);
  statics_add(ctx, string);
  GC_END(ctx->gc_ctx);
  return (node_expr_t *)n;
}

static void compute_stype_db_empty (rule_compiler_t *ctx, node_expr_t *myself)
{
  node_expr_t *n = (node_expr_t *)myself;

  set_type (ctx, n, &t_db_any);
}

node_expr_t *build_db_nothing(rule_compiler_t *ctx)
{
  ovm_var_t *empty;
  node_expr_term_t  *n;

  GC_START(ctx->gc_ctx, 1);
  empty = (ovm_var_t *)db_empty (ctx->gc_ctx);
  GC_UPDATE(ctx->gc_ctx, 0, empty);

  n = (node_expr_term_t *) gc_alloc (ctx->gc_ctx, sizeof(node_expr_term_t),
				     &node_expr_term_class);
  n->gc.type = T_NODE_CONST;
  n->type = NODE_CONST;
  GC_TOUCH (ctx->gc_ctx, n->file = ctx->currfile);
  n->lineno = ctx->issdllineno;
  n->hash = h_pair (NODE_CONST,
		    h_pair (T_DB_EMPTY, H_NULL));
  /* hash = '(NODE_CONST T_DB_EMPTY)' mod bigprime */
  n->stype = NULL; /* not known yet */
  n->compute_stype = CS_DB_EMPTY;
  n->parents = NULL;
  GC_TOUCH (ctx->gc_ctx, n->data = empty);
  n->res_id = ctx->statics_nb;
  GC_UPDATE(ctx->gc_ctx, 0, n);
  statics_add(ctx, empty);
  GC_END(ctx->gc_ctx);
  return (node_expr_t *)n;
}

static void compute_stype_event (rule_compiler_t *ctx, node_expr_t *myself)
{
  node_expr_bin_t *n = (node_expr_bin_t *)myself;
  node_expr_t *evt;

  evt = BIN_LVAL(n);
  if (evt!=NULL)
    {
      if (!stype_below(evt->stype, &t_event))
	{
	  if (myself->file!=NULL)
	    fprintf (stderr, "%s:", STR(myself->file));
	  /* printing STR(myself->file) is legal, since it
	     was created NUL-terminated, on purpose */
	  fprintf (stderr, "%u: type error: event expected before '+.{', got %s.\n",
		   myself->lineno,
		   evt->stype->name);
	  ctx->nerrors++;
	}
    }
  set_type (ctx, myself, &t_event);
}

node_expr_t *build_expr_event(rule_compiler_t *ctx, int op,
			      node_expr_t *left_node,
			      node_expr_t *right_node)
{
  node_expr_bin_t *n;
  node_expr_t *l;

  n = gc_alloc (ctx->gc_ctx, sizeof(node_expr_bin_t),
		&node_expr_bin_class);
  n->gc.type = T_NODE_EVENT;
  n->type = NODE_EVENT;
  GC_TOUCH (ctx->gc_ctx, n->file = ctx->currfile);
  n->lineno = ctx->issdllineno;
  n->hash = h_pair (NODE_EVENT,
		    h_pair (op,
			    h_cons (left_node,
				    h_cons (right_node, H_NULL))));
  /* hash = '(NODE_EVENT op left_node right_node)' mod bigprime */
  n->stype = NULL; /* not known yet */
  n->compute_stype = CS_EVENT;
  n->parents = NULL;
  n->op = op;
  GC_TOUCH (ctx->gc_ctx, n->lval = left_node);
  GC_TOUCH (ctx->gc_ctx, n->rval = right_node);
  GC_START(ctx->gc_ctx, 1);
  GC_UPDATE(ctx->gc_ctx, 0, n);
  for (l=right_node; l!=NULL; l=BIN_RVAL(l))
    add_parent (ctx, BIN_LVAL(l), (node_expr_t *)n);
  if (left_node!=NULL)
    add_parent (ctx, left_node, (node_expr_t *)n);
  GC_END(ctx->gc_ctx);
  return (node_expr_t *)n;
}

static void compute_stype_db_pattern (rule_compiler_t *ctx, node_expr_t *myself)
{
  node_expr_bin_t *n = (node_expr_bin_t *)myself;
  node_expr_t *l, *e;
  char *type_name, *s, *arg_type_name;
  size_t nargs;
  size_t len;
  type_t *db_type, *argtype, *vartype;

  db_type = BIN_LVAL(n)->stype;
  if (db_type==NULL)
    return;
  if (db_type==&t_any)
    {
      set_type (ctx, myself, &t_any); /* propagate error, but do not bother
					 user with spurious error messages */
      return;
    }
  type_name = db_type->name;
  if (memcmp(type_name, "db[", 3)!=0)
    {
      if (myself->file!=NULL)
	fprintf (stderr, "%s:", STR(myself->file));
      /* printing STR(myself->file) is legal, since it
	 was created NUL-terminated, on purpose */
      fprintf (stderr, "%u: type error: database type expected, got %s.\n",
	       myself->lineno,
	       type_name);
      ctx->nerrors++;
      set_type (ctx, myself, &t_any);
    }
  else if (strcmp(type_name, "db[*]")==0)
    { /* database is empty: do not do anything */
      ;
    }
  else /* type is now of the form db[type1,type2,...,typen],
	  where each typei is a single string with no [, ], or comma in it;
       */
    {
      set_type (ctx, myself, &t_void);
      s = type_name+3;
      nargs = 0;
      for (l=BIN_RVAL(n); l!=NULL; l=BIN_RVAL(l))
	{
	  if (s[0]==']')
	    {
	      if (myself->file!=NULL)
		fprintf (stderr, "%s:", STR(myself->file));
	      /* printing STR(myself->file) is legal, since it
		 was created NUL-terminated, on purpose */
	      fprintf (stderr, "%u: too many arguments, expected %zd only.\n",
		       myself->lineno,
		       nargs);
	      ctx->nerrors++;
	      set_type (ctx, myself, &t_any);
	      break;
	    }
	  s++;
	  len = strcspn(s,",]");
	  nargs++;
	  arg_type_name = gc_base_malloc (ctx->gc_ctx, len+1);
	  memcpy (arg_type_name, s, len);
	  arg_type_name[len] = 0;
	  argtype = stype_from_string (ctx->gc_ctx, arg_type_name, 0, 0);
	  gc_base_free (arg_type_name);
	  e = BIN_LVAL(l);
	  if (is_logical_variable(e))
	    {
	      vartype = stype_join (e->stype, argtype);
	      if (vartype==&t_any && e->stype!=NULL && argtype!=NULL)
		  {
		    if (n->file!=NULL)
		      fprintf (stderr, "%s:", STR(n->file));
		    /* printing STR(n->file) is legal, since it
		       was created NUL-terminated, on purpose */
		    fprintf (stderr, "%u: type error: logical variable %s of type %s cannot match field of type %s.\n",
			     n->lineno,
			     SYM_NAME(e),
			     e->stype->name, argtype->name);
		    set_type (ctx, myself, &t_any);
		    ctx->nerrors++;
		  }
	      set_type (ctx, e, vartype);
	    }
	  else
	    {
	      vartype = e->stype;
	      if (stype_join(vartype, argtype)==&t_any && vartype!=NULL && argtype!=NULL)
		  {
		    if (n->file!=NULL)
		      fprintf (stderr, "%s:", STR(n->file));
		    /* printing STR(n->file) is legal, since it
		       was created NUL-terminated, on purpose */
		    fprintf (stderr, "%u: type error: pattern of type %s cannot match field of type %s.\n",
			     n->lineno,
			     vartype->name, argtype->name);
		    set_type (ctx, myself, &t_any);
		    ctx->nerrors++;
		  }
	    }
	  s += len;
	}
      if (s[0]!=']')
	{
	  while (s[0]!=']')
	    {
	      s++;
	      len = strcspn(s,",]");
	      nargs++;
	      s += len;
	    }
	  if (myself->file!=NULL)
	    fprintf (stderr, "%s:", STR(myself->file));
	  /* printing STR(myself->file) is legal, since it
	     was created NUL-terminated, on purpose */
	  fprintf (stderr, "%u: error: too few arguments, expected %zd.\n",
		   myself->lineno,
		   nargs);
	  set_type (ctx, myself, &t_any);
	  ctx->nerrors++;
	}
    }
}


node_expr_t *build_db_pattern(rule_compiler_t *ctx, node_expr_t *tuple, node_expr_t *db)
{
  node_expr_bin_t *n;
  node_expr_t *l, *e;

  //gc_check(ctx->gc_ctx);
  n = gc_alloc (ctx->gc_ctx, sizeof(node_expr_bin_t), &node_expr_bin_class);
  n->gc.type = T_NODE_DB_PATTERN;
  n->type = NODE_DB_PATTERN;
  GC_TOUCH (ctx->gc_ctx, n->file = ctx->currfile);
  n->lineno = ctx->issdllineno;
  n->hash = h_pair (NODE_DB_PATTERN,
		    h_cons (db, tuple->hash));
  /* hash = '(NODE_DB_PATTERN db arg1 ... argn)' mod bigprime */
  /* note that tuple->hash is correct, because tuples are never empty */
  n->stype = NULL;
  n->compute_stype = CS_DB_PATTERN;
  n->parents = NULL;
  n->op = -1;
  GC_TOUCH (ctx->gc_ctx, n->lval = db);
  GC_TOUCH (ctx->gc_ctx, n->rval = tuple);
  GC_START(ctx->gc_ctx, 1);
  GC_UPDATE(ctx->gc_ctx, 0, n);
  add_parent (ctx, db, (node_expr_t *)n);
  //gc_check(ctx->gc_ctx);
  for (l=tuple; l!=NULL; l=BIN_RVAL(l))
    {
      e = BIN_LVAL(l);
      if (!is_logical_variable(e))
	add_parent (ctx, e, (node_expr_t *)n);
    }
  GC_END(ctx->gc_ctx);
  //gc_check(ctx->gc_ctx);
  return (node_expr_t *)n;
}

static int is_basic_type (type_t *t)
{
  if (t==NULL)
    return 1;
  if (strcmp (t->name, "int") &&
      strcmp (t->name, "uint") &&
      strcmp (t->name, "str") &&
      strcmp (t->name, "bstr") &&
      strcmp (t->name, "ipv4") &&
      strcmp (t->name, "ipb6") &&
      strcmp (t->name, "float") &&
      strcmp (t->name, "ctime") &&
      strcmp (t->name, "timeval") &&
      strcmp (t->name, "snmpoid"))
    return 0;
  return 1;
}

static void buf_puts (gc_t *gc_ctx, char *s, char **bufp, size_t *lenp, size_t *sizep)
{
  size_t slen, newsize;

  slen = strlen(s);
  newsize = slen+(*lenp);
  if (newsize>=(*sizep))
    {
      if (newsize < (*sizep) + 128)
	newsize = (*sizep) + 128;
      *bufp = gc_base_realloc (gc_ctx, *bufp, newsize);
      *sizep = newsize;
    }
  strcpy ((*bufp)+(*lenp), s);
  *lenp += slen;
}

static void compute_stype_db_singleton (rule_compiler_t *ctx, node_expr_t *myself)
{
  node_expr_mon_t *n = (node_expr_mon_t *)myself;
  node_expr_t *tuple, *l;
  char *buf;
  size_t len, bufsize;
  char *delim;
  type_t *t;
  int err;

  tuple = MON_VAL(n);
  for (l=tuple; l!=NULL; l=BIN_RVAL(l))
    {
      t = BIN_LVAL(l)->stype;
      if (t==NULL) /* not enough info yet (NULL) */
	return;
      if (t==&t_any)
	{
	  set_type (ctx, myself, &t_any);
	  return; /* type error that we just propagate,
		     without any spurious error message */
	}
    }
  err = 0;
  for (l=tuple; l!=NULL; l=BIN_RVAL(l))
    {
      t = BIN_LVAL(l)->stype;
      if (!is_basic_type (t))
	{
	  if (BIN_LVAL(l)->file!=NULL)
	    fprintf (stderr, "%s:", STR(BIN_LVAL(l)->file));
	  /* printing STR(BIN_LVAL(l)->file) is legal, since it
	     was created NUL-terminated, on purpose */
	  fprintf (stderr, "%u: type error: tuple argument has non-basic type %s.\n",
	       BIN_LVAL(l)->lineno, t->name);
	  set_type (ctx, myself, &t_any);
	  ctx->nerrors++;
	  err++;
	}
    }
  if (err)
    return;
  bufsize = 128;
  len = 0;
  buf = gc_base_malloc (ctx->gc_ctx, bufsize);
  buf_puts (ctx->gc_ctx, "db[", &buf, &len, &bufsize);
  delim = "";
  for (l=tuple; l!=NULL; l=BIN_RVAL(l))
    {
      buf_puts (ctx->gc_ctx, delim, &buf, &len, &bufsize);
      delim = ",";
      buf_puts (ctx->gc_ctx, BIN_LVAL(l)->stype->name, &buf, &len, &bufsize);
    }
  buf_puts (ctx->gc_ctx, "]", &buf, &len, &bufsize);
  t = stype_from_string (ctx->gc_ctx, buf, 1, T_DB_MAP);
  gc_base_free (buf);
  set_type (ctx, myself, t);
}

node_expr_t *build_db_singleton(rule_compiler_t *ctx, node_expr_t *tuple)
{
  node_expr_mon_t *n;
  node_expr_t *l;

  n = gc_alloc (ctx->gc_ctx, sizeof(node_expr_mon_t), &node_expr_mon_class);
  n->gc.type = T_NODE_DB_SINGLETON;
  n->type = NODE_DB_SINGLETON;
  GC_TOUCH (ctx->gc_ctx, n->file = ctx->currfile);
  n->lineno = ctx->issdllineno;
  n->hash = h_pair (NODE_DB_SINGLETON,
		    ((tuple==NULL)?H_NULL:tuple->hash));
  /* hash = '(NODE_DB_SINGLETON arg1 ... argn)' mod bigprime
     where tuple = (arg1 ... argn) */
  /* note that tuple->hash is correct, because tuples are never empty */
  n->stype = NULL;
  n->compute_stype = CS_DB_SINGLETON;
  n->parents = NULL;
  n->op = -1;
  GC_TOUCH (ctx->gc_ctx, n->val = tuple);
  GC_START(ctx->gc_ctx, 1);
  GC_UPDATE(ctx->gc_ctx, 0, n);
  for (l=tuple; l!=NULL; l=BIN_RVAL(l))
    add_parent (ctx, BIN_LVAL(l), (node_expr_t *)n);
  GC_END(ctx->gc_ctx);
  return (node_expr_t *)n;
}

static void compute_stype_db_collect (rule_compiler_t *ctx, node_expr_t *myself)
{
  node_expr_bin_t *n = (node_expr_bin_t *)myself;
  node_expr_t *collect;
  node_expr_t *returns, *l, *ret;
  type_t *t, *rett, *restype;

  returns = BIN_LVAL(n);
  collect = BIN_RVAL(BIN_LVAL(BIN_RVAL(n)));
  t = collect->stype;
  for (l=returns; l!=NULL; l=BIN_RVAL(l))
    {
      ret = BIN_LVAL(l);
      if (ret->type==NODE_BREAK)
	continue; /* 'break' does not contribute to the final type */
      rett = MON_VAL(ret)->stype;
      restype = stype_join (t, rett);
      if (restype==&t_any)
	{
	  if (t!=NULL && rett!=NULL)
	    {
	      if (BIN_LVAL(l)->file!=NULL)
		fprintf (stderr, "%s:", STR(BIN_LVAL(l)->file));
	      /* printing STR(BIN_LVAL(l)->file) is legal, since it
		 was created NUL-terminated, on purpose */
	      fprintf (stderr, "%u: type error: return %s, expected %s.\n",
		       BIN_LVAL(l)->lineno,
		       t->name, rett->name);
	      ctx->nerrors++;
	    }
	  t = &t_any;
	  break;
	}
      t = restype;
    }
  set_type (ctx, (node_expr_t *)n, t);
}


node_expr_t *build_db_collect(rule_compiler_t *ctx,
			      node_expr_t *actions,
			      node_expr_t *collect,
			      node_expr_t *returns,
			      node_expr_t *patterns)
{
  node_expr_bin_t *n;
  node_expr_t *l;
  node_expr_t *action;

  n = gc_alloc (ctx->gc_ctx, sizeof(node_expr_bin_t), &node_expr_bin_class);
  n->gc.type = T_NODE_DB_COLLECT;
  n->type = NODE_DB_COLLECT;
  GC_TOUCH (ctx->gc_ctx, n->file = ctx->currfile);
  n->lineno = ctx->issdllineno;
  GC_START(ctx->gc_ctx, 1);
  action = build_expr_cons (ctx, actions, collect);
  GC_UPDATE(ctx->gc_ctx, 0, action);
  n->hash = h_pair (NODE_DB_COLLECT,
		    h_cons (returns,
			    h_cons (action,
				    (patterns==NULL)?H_NULL:patterns->hash)));
  /* hash = '(NODE_DB_COLLECT returns (actions . collect) pat1 ... patn)' mod bigprime */
  n->stype = NULL;
  n->compute_stype = CS_DB_COLLECT;
  n->parents = NULL;
  n->op = -1;
  action = build_expr_cons (ctx, action, patterns);
  GC_TOUCH (ctx->gc_ctx, n->rval = action);
  GC_TOUCH (ctx->gc_ctx, n->lval = returns);
  GC_UPDATE(ctx->gc_ctx, 0, n);
  for (l=patterns; l!=NULL; l=BIN_RVAL(l))
    add_parent (ctx, BIN_LVAL(l), (node_expr_t *)n);
  for (l=returns; l!=NULL; l=BIN_RVAL(l))
    add_parent (ctx, BIN_LVAL(l), (node_expr_t *)n);
  add_parent (ctx, collect, (node_expr_t *)n);
  GC_END(ctx->gc_ctx);
  return (node_expr_t *)n;
}

static void compute_stype_ipv4 (rule_compiler_t *ctx, node_expr_t *myself)
{
  node_expr_t *n = (node_expr_t *)myself;

  set_type (ctx, n, &t_ipv4);
}

node_expr_t *build_ipv4(rule_compiler_t *ctx, char *hostname)
{
  ovm_var_t *addr;
  node_expr_term_t *n;
  struct addrinfo *hptr;
  struct addrinfo hints;
  int status;

  /* XXX Add hash for constant sharing here */

  GC_START(ctx->gc_ctx, 1);
  addr = ovm_ipv4_new(ctx->gc_ctx);
  hints.ai_flags = 0;
  hints.ai_family = PF_INET; /* only allow for IPv4 */
  hints.ai_socktype = 0;
  hints.ai_protocol = 0;
  hints.ai_addrlen = 0;
  hints.ai_addr = NULL;
  hints.ai_canonname = NULL;
  hints.ai_next = NULL;
  status = getaddrinfo (hostname, NULL, &hints, &hptr);
  if (status != 0)
    {
      if (ctx->currfile!=NULL)
	fprintf (stderr, "%s:", STR(ctx->currfile));
      /* printing STR(ctx->currfile) is legal, since it
	 was created NUL-terminated, on purpose */
      fprintf (stderr, "%u: fatal error: hostname %s not found - %s.\n",
	       ctx->issdllineno, hostname, gai_strerror(status));
      fflush (stderr);
      exit(EXIT_FAILURE);
    }
  /* resolve host name */
  IPV4(addr) = ((struct sockaddr_in *)(hptr->ai_addr))->sin_addr;
  freeaddrinfo(hptr);
  GC_UPDATE(ctx->gc_ctx, 0, addr);

  gc_base_free(hostname); /* free string memory allocated by the lexer */

  n = (node_expr_term_t *) gc_alloc (ctx->gc_ctx, sizeof(node_expr_term_t),
				     &node_expr_term_class);
  n->gc.type = T_NODE_CONST;
  n->type = NODE_CONST;
  GC_TOUCH (ctx->gc_ctx, n->file = ctx->currfile);
  n->lineno = ctx->issdllineno;
  n->hash = h_pair (NODE_CONST,
		    h_pair (T_IPV4,
			    h_str ((char *)&IPV4(addr),
				   sizeof(struct in_addr))));
  /* hash = '(NODE_CONST T_IPV4 xx yy zz tt)' mod bigprime */
  n->stype = NULL; /* not known yet */
  n->compute_stype = CS_IPV4;
  n->parents = NULL;
  GC_TOUCH (ctx->gc_ctx, n->data = addr);
  n->res_id = ctx->statics_nb;
  GC_UPDATE(ctx->gc_ctx, 0, n);
  statics_add(ctx, addr);
  GC_END(ctx->gc_ctx);
  return (node_expr_t *)n;
}

static void compute_stype_ipv6 (rule_compiler_t *ctx, node_expr_t *myself)
{
  node_expr_t *n = (node_expr_t *)myself;

  set_type (ctx, n, &t_ipv6);
}

node_expr_t *build_ipv6(rule_compiler_t *ctx, char *hostname)
{
  ovm_var_t *addr;
  node_expr_term_t *n;
  struct addrinfo *hptr;
  int status;

  /* XXX Add hash for constant sharing here */

  GC_START(ctx->gc_ctx, 1);
  addr = ovm_ipv6_new(ctx->gc_ctx);
  status = getaddrinfo(hostname, NULL, NULL, &hptr);
  if (status != 0)
    {
      if (ctx->currfile!=NULL)
	fprintf (stderr, "%s:", STR(ctx->currfile));
      /* printing STR(ctx->currfile) is legal, since it
	 was created NUL-terminated, on purpose */
      fprintf (stderr, "%u: fatal error: hostname %s not found - %s.\n",
	       ctx->issdllineno, hostname, gai_strerror(status));
      fflush (stderr);
      exit(EXIT_FAILURE);
    }
  /* resolve host name */
  IPV6(addr) = ((struct sockaddr_in6 *)(hptr->ai_addr))->sin6_addr;
  GC_UPDATE(ctx->gc_ctx, 0, addr);

  gc_base_free(hostname); /* free string memory allocated by the lexer */
  freeaddrinfo(hptr);

  n = (node_expr_term_t *) gc_alloc (ctx->gc_ctx, sizeof(node_expr_term_t),
				     &node_expr_term_class);
  n->gc.type = T_NODE_CONST;
  n->type = NODE_CONST;
  GC_TOUCH (ctx->gc_ctx, n->file = ctx->currfile);
  n->lineno = ctx->issdllineno;
  n->hash = h_pair (NODE_CONST,
		    h_pair (T_IPV6,
			    h_str ((char *)&IPV6(addr),
				   sizeof(struct in6_addr))));
  /* hash = '(NODE_CONST T_IPV6 byte1 ... byte16)' mod bigprime */
  n->stype = NULL; /* not known yet */
  n->compute_stype = CS_IPV6;
  n->parents = NULL;
  GC_TOUCH (ctx->gc_ctx, n->data = addr);
  n->res_id = ctx->statics_nb;
  GC_UPDATE(ctx->gc_ctx, 0, n);
  statics_add(ctx, addr);
  GC_END(ctx->gc_ctx);
  return (node_expr_t *)n;
}

node_expr_t *build_ip(rule_compiler_t *ctx, char *hostname)
{
  ovm_var_t *addr;
  node_expr_term_t *n;
  struct addrinfo *sa;
  int status;

  /* XXX Add hash for constant sharing here */
  
  GC_START(ctx->gc_ctx,1);
  n = (node_expr_term_t *) gc_alloc (ctx->gc_ctx, sizeof(node_expr_term_t),
				     &node_expr_term_class);
  n->gc.type = T_NODE_CONST;
  n->type = NODE_CONST;
  GC_TOUCH (ctx->gc_ctx, n->file = ctx->currfile);
  n->lineno = ctx->issdllineno;
  n->stype = NULL; /* not known yet */
  n->compute_stype = CS_NULL;
  n->parents = NULL;
  
  /* resolve host name */
  status = getaddrinfo(hostname, NULL, NULL, &sa);
  if (status != 0)
    {
      if (ctx->currfile!=NULL)
	fprintf (stderr, "%s:", STR(ctx->currfile));
      /* printing STR(ctx->currfile) is legal, since it
	 was created NUL-terminated, on purpose */
      fprintf (stderr, "%u: fatal error: cannot connect to %s - %s.\n",
	       ctx->issdllineno, hostname, gai_strerror(status));
      fflush (stderr);
      exit(EXIT_FAILURE);
  }

  if (sa->ai_family == AF_INET)
    {
      addr = ovm_ipv4_new(ctx->gc_ctx);
      IPV4(addr) = ((struct sockaddr_in *)(sa->ai_addr))->sin_addr; 
      
      n->hash = h_pair (NODE_CONST,
			h_pair (T_IPV4,
				h_str ((char *)&IPV4(addr),
				       sizeof(struct in_addr))));
      /* hash = '(NODE_CONST T_IPV4 xx yy zz tt)' mod bigprime */
      GC_TOUCH (ctx->gc_ctx, n->data = addr);
      n->res_id = ctx->statics_nb;
      GC_UPDATE(ctx->gc_ctx, 0, n);
      n->compute_stype = CS_IPV4;
    }
  else
    {
      addr = ovm_ipv6_new(ctx->gc_ctx);
      IPV6(addr) = ((struct sockaddr_in6 *)(sa->ai_addr))->sin6_addr;
      
      n->hash = h_pair (NODE_CONST,
			h_pair (T_IPV6,
				h_str ((char *)&IPV6(addr),
				       sizeof(struct in6_addr))));
      /* hash = '(NODE_CONST T_IPV6 byte1 ... byte16)' mod bigprime */
      GC_TOUCH (ctx->gc_ctx, n->data = addr);
      n->res_id = ctx->statics_nb;
      GC_UPDATE(ctx->gc_ctx, 0, n);
      n->compute_stype = CS_IPV6;
    }
  
  freeaddrinfo(sa);
  /* free string memory allocated by the lexer */
  gc_base_free(hostname);
  
  statics_add(ctx, addr);
  GC_END(ctx->gc_ctx);
  return (node_expr_t *)n;
}

static void compute_stype_ctime (rule_compiler_t *ctx, node_expr_t *myself)
{
  node_expr_t *n = (node_expr_t *)myself;

  set_type (ctx, n, &t_ctime);
}

node_expr_t *build_ctime_from_int(rule_compiler_t *ctx, time_t ctime)
{
  node_expr_term_t  *n;
  ovm_var_t    *time;

  GC_START(ctx->gc_ctx, 1);
  time = ovm_ctime_new(ctx->gc_ctx, ctime);
  GC_UPDATE(ctx->gc_ctx, 0, time);

  n = (node_expr_term_t *) gc_alloc (ctx->gc_ctx, sizeof(node_expr_term_t),
				     &node_expr_term_class);
  n->gc.type = T_NODE_CONST;
  n->type = NODE_CONST;
  GC_TOUCH (ctx->gc_ctx, n->file = ctx->currfile);
  n->lineno = ctx->issdllineno;
  n->hash = h_pair (NODE_CONST,
		    h_pair (T_CTIME,
			    h_pair (CTIME(time) % bigprime,
				    H_NULL)));
  /* hash = '(NODE_CONST T_CTIME timeval)' mod bigprime */
  n->stype = NULL; /* not known yet */
  n->compute_stype = CS_CTIME;
  n->parents = NULL;
  GC_TOUCH (ctx->gc_ctx, n->data = time);
  n->res_id = ctx->statics_nb;
  GC_UPDATE(ctx->gc_ctx, 0, n);
  statics_add(ctx, time);
  GC_END(ctx->gc_ctx);
  return (node_expr_t *)n;
}

static char *strptime_formats[] = {
  "%a %m/%d/%Y %T %Z", /* US format */
  "%a %d %m %Y %T %Z", /* French format? */
  "%a %Y-%m-%d %T %Z", /* European format? */
  "%a %m/%d/%Y %T", /* US format */
  "%a %d %m %Y %T", /* French format? */
  "%a %Y-%m-%d %T", /* European format? */
  "%a %m/%d/%Y", /* US format */
  "%a %d %m %Y", /* French format? */
  "%a %Y-%m-%d", /* European format? */
  "%Y-%m-%dT%H:%M:%S", /* IDMEF format */
  NULL
};

static int orchids_getdate (char *date, struct tm *tm)
{
  char **p;

  memset (tm, 0, sizeof(struct tm));
  for (p=strptime_formats; *p!=NULL; p++)
    {
      if (strptime (date, *p, tm)!=NULL)
	return 0;
    }
  return -1;
}

node_expr_t *build_ctime_from_string(rule_compiler_t *ctx, char *date)
{
  node_expr_term_t  *n;
  ovm_var_t    *time;
  struct tm tm;

  GC_START(ctx->gc_ctx, 1);
  if (orchids_getdate (date, &tm))
    {
      if (ctx->currfile!=NULL)
	fprintf (stderr, "%s:", STR(ctx->currfile));
      /* printing STR(ctx->currfile) is legal, since it
	 was created NUL-terminated, on purpose */
      fprintf (stderr, "%u: error: unrecognized time format.\n",
	       ctx->issdllineno);
      ctx->nerrors++;
    }
  else memset (&tm, 0, sizeof(struct tm));
  gc_base_free (date); /* free string allocated by lexer */
  time = ovm_ctime_new(ctx->gc_ctx, mktime (&tm));
  GC_UPDATE(ctx->gc_ctx, 0, time);

  n = (node_expr_term_t *) gc_alloc (ctx->gc_ctx, sizeof(node_expr_term_t),
				     &node_expr_term_class);
  n->gc.type = T_NODE_CONST;
  n->type = NODE_CONST;
  GC_TOUCH (ctx->gc_ctx, n->file = ctx->currfile);
  n->lineno = ctx->issdllineno;
  n->hash = h_pair (NODE_CONST,
		    h_pair (T_CTIME,
			    h_pair (CTIME(time) % bigprime,
				    H_NULL)));
  /* hash = '(NODE_CONST T_CTIME timeval)' mod bigprime */
  n->stype = NULL; /* not known yet */
  n->compute_stype = CS_CTIME;
  n->parents = NULL;
  GC_TOUCH (ctx->gc_ctx, n->data = time);
  n->res_id = ctx->statics_nb;
  GC_UPDATE(ctx->gc_ctx, 0, n);
  statics_add(ctx, time);
  GC_END(ctx->gc_ctx);
  return (node_expr_t *)n;
}

static void compute_stype_timeval (rule_compiler_t *ctx, node_expr_t *myself)
{
  node_expr_t *n = (node_expr_t *)myself;

  set_type (ctx, n, &t_timeval);
}

node_expr_t *build_timeval_from_int(rule_compiler_t *ctx, long sec, long usec)
{
  node_expr_term_t  *n;
  ovm_var_t    *timeval;

  GC_START(ctx->gc_ctx, 1);
  timeval = ovm_timeval_new(ctx->gc_ctx);
  TIMEVAL(timeval).tv_sec  = sec;
  TIMEVAL(timeval).tv_usec = usec;
  GC_UPDATE(ctx->gc_ctx, 0, time);

  n = (node_expr_term_t *) gc_alloc (ctx->gc_ctx, sizeof(node_expr_term_t),
				     &node_expr_term_class);
  n->gc.type = T_NODE_CONST;
  n->type = NODE_CONST;
  GC_TOUCH (ctx->gc_ctx, n->file = ctx->currfile);
  n->lineno = ctx->issdllineno;
  n->hash = h_pair (NODE_CONST,
		    h_pair (T_TIMEVAL,
			    h_pair (sec % bigprime,
				    h_pair (usec % bigprime,
					    H_NULL))));
  /* hash = '(NODE_CONST T_TIMEVAL sec usec)' mod bigprime */
  n->stype = NULL; /* not known yet */
  n->compute_stype = CS_TIMEVAL;
  n->parents = NULL;
  GC_TOUCH (ctx->gc_ctx, n->data = timeval);
  n->res_id = ctx->statics_nb;
  GC_UPDATE(ctx->gc_ctx, 0, n);
  statics_add(ctx, timeval);
  GC_END(ctx->gc_ctx);
  return (node_expr_t *)n;
}

node_expr_t *build_timeval_from_string(rule_compiler_t *ctx, char *date, long usec)
{
  node_expr_term_t *n;
  ovm_var_t *timeval;
  struct tm tm;

  GC_START(ctx->gc_ctx, 1);
  if (orchids_getdate (date, &tm)) {
      if (ctx->currfile!=NULL)
	fprintf (stderr, "%s:", STR(ctx->currfile));
      /* printing STR(ctx->currfile) is legal, since it
	 was created NUL-terminated, on purpose */
      fprintf (stderr, "%u: error: unrecognized time format.\n",
	       ctx->issdllineno);
      ctx->nerrors++;
  }
  else memset (&tm, 0, sizeof(struct tm));
  gc_base_free (date); /* free string allocated by lexer */
  timeval = ovm_timeval_new(ctx->gc_ctx);
  TIMEVAL(timeval).tv_sec  = mktime (&tm);
  TIMEVAL(timeval).tv_usec = usec;
  GC_UPDATE(ctx->gc_ctx, 0, timeval);

  n = (node_expr_term_t *) gc_alloc (ctx->gc_ctx, sizeof(node_expr_term_t),
				     &node_expr_term_class);
  n->gc.type = T_NODE_CONST;
  n->type = NODE_CONST;
  GC_TOUCH (ctx->gc_ctx, n->file = ctx->currfile);
  n->lineno = ctx->issdllineno;
  n->hash = h_pair (NODE_CONST,
		    h_pair (T_TIMEVAL,
			    h_pair (TIMEVAL(timeval).tv_sec % bigprime,
				    h_pair (TIMEVAL(timeval).tv_usec % bigprime,
					    H_NULL))));
  /* hash = '(NODE_CONST T_TIMEVAL sec usec)' mod bigprime */
  n->stype = NULL; /* not known yet */
  n->compute_stype = CS_TIMEVAL;
  n->parents = NULL;
  GC_TOUCH (ctx->gc_ctx, n->data = timeval);
  n->res_id = ctx->statics_nb;
  GC_UPDATE(ctx->gc_ctx, 0, n);
  statics_add(ctx, timeval);
  GC_END(ctx->gc_ctx);
  return (node_expr_t *)n;
}

static void compute_stype_regex (rule_compiler_t *ctx, node_expr_t *myself)
{
  node_expr_t *n = (node_expr_t *)myself;

  set_type (ctx, n, &t_regex);
}

node_expr_t *build_split_regex(rule_compiler_t *ctx, char *regex_str)
{
  node_expr_term_t  *n;
  ovm_var_t    *regex;
  int           ret;

  GC_START(ctx->gc_ctx, 1);
  regex = ovm_regex_new(ctx->gc_ctx);
  REGEXSTR(regex) = regex_str;
  GC_UPDATE(ctx->gc_ctx, 0, regex);

  /* compile regex */
  ret = regcomp(&REGEX(regex), regex_str, REG_EXTENDED);
  if (ret)
    {
      char err_buf[64];

      if (ctx->currfile!=NULL)
	fprintf (stderr, "%s:", STR(ctx->currfile));
      /* printing STR(ctx->currfile) is legal, since it
	 was created NUL-terminated, on purpose */
      fprintf (stderr, "%u: error: ", ctx->issdllineno);
      regerror(ret, &REGEX(regex), err_buf, sizeof (err_buf));
      fprintf (stderr, "%s.\n", err_buf);
      ctx->nerrors++;
    }
  n = (node_expr_term_t *) gc_alloc (ctx->gc_ctx, sizeof(node_expr_term_t),
				     &node_expr_term_class);
  n->gc.type = T_NODE_CONST;
  n->type = NODE_CONST;
  GC_TOUCH (ctx->gc_ctx, n->file = ctx->currfile);
  n->lineno = ctx->issdllineno;
  n->stype = NULL; /* not known yet */
  n->compute_stype = CS_REGEX;
  n->parents = NULL;
  GC_TOUCH (ctx->gc_ctx, n->data = regex);
  n->res_id = ctx->statics_nb;
  GC_UPDATE(ctx->gc_ctx, 0, n);
  statics_add(ctx, regex);
  GC_END(ctx->gc_ctx);
  return (node_expr_t *)n;
}

node_expr_t *build_regex(rule_compiler_t *ctx, char *regex_str)
{
  node_expr_term_t  *n;
  ovm_var_t    *regex;
  int           ret;

  GC_START(ctx->gc_ctx, 1);
  regex = ovm_regex_new(ctx->gc_ctx);
  REGEXSTR(regex) = regex_str;

  /* compile regex */
  ret = regcomp(&REGEX(regex), regex_str, REG_EXTENDED | REG_NOSUB);
  if (ret) {
      char err_buf[64];

      if (ctx->currfile!=NULL)
	fprintf (stderr, "%s:", STR(ctx->currfile));
      /* printing STR(ctx->currfile) is legal, since it
	 was created NUL-terminated, on purpose */
      fprintf (stderr, "%u: error:",
	       ctx->issdllineno);
      regerror(ret, &REGEX(regex), err_buf, sizeof (err_buf));
      fprintf (stderr, "%s.\n", err_buf);
      ctx->nerrors++;
  }
  n = (node_expr_term_t *) gc_alloc (ctx->gc_ctx, sizeof(node_expr_term_t),
				     &node_expr_term_class);
  n->gc.type = T_NODE_CONST;
  n->type = NODE_CONST;
  GC_TOUCH (ctx->gc_ctx, n->file = ctx->currfile);
  n->lineno = ctx->issdllineno;
  n->hash = h_pair (NODE_CONST,
		    h_pair (T_REGEX,
			    h_string (regex_str)));
  /* hash = '(NODE_CONST T_REGEX r e g e x)' mod bigprime */
  n->stype = NULL; /* not known yet */
  n->compute_stype = CS_REGEX;
  n->parents = NULL;
  GC_TOUCH (ctx->gc_ctx, n->data = regex);
  n->res_id = ctx->statics_nb;
  GC_UPDATE(ctx->gc_ctx, 0, n);
  statics_add(ctx, regex);
  GC_END(ctx->gc_ctx);
  return (node_expr_t *)n;
}


/**
 * Build the field name hash table.
 * @param ctx Orchids context.
 **/
static void build_fields_hash(orchids_t *ctx)
{
  strhash_t *h;
  int        f;

  h = ctx->rule_compiler->fields_hash;
  for (f = 0; f < ctx->global_fields->num_fields; f++)
    strhash_add(ctx->gc_ctx, h, &ctx->global_fields->fields[f],
		ctx->global_fields->fields[f].name);

  DebugLog(DF_OLC, DS_INFO,
           "build_fields_hash(): size: %i elems: %i collides: %i\n",
           h->size, h->elmts, strhash_collide_count(h));
}


/**
 * Build the ISSDL built-in functions hash table.
 * @param ctx Orchids context.
 **/
static void build_functions_hash(orchids_t *ctx)
{
  strhash_t        *h;
  issdl_function_t *func_tbl;
  int               f;
  int               nf;

  h = ctx->rule_compiler->functions_hash;
  func_tbl = ctx->vm_func_tbl;
  nf = ctx->vm_func_tbl_sz;
  for (f = 0; f < nf; f++) {
    if (strhash_get(h, func_tbl[f].name)) {
      DebugLog(DF_ENG, DS_ERROR, "Function '%s' already registered\n", func_tbl[f].name);
      return ;
    }
    strhash_add(ctx->gc_ctx, h, &func_tbl[f], func_tbl[f].name);
  }

  DebugLog(DF_OLC, DS_INFO,
           "build_functions_hash(): size: %i elems: %i collides: %i\n",
           h->size, h->elmts, strhash_collide_count(h));
}


char *build_concat_string(rule_compiler_t *ctx, char *str1, char *str2)
{
  size_t str_len, len1, len2;
  char *string;

  len1 = strlen(str1);
  len2 = strlen(str2);
  str_len = len1 + len2 + 1;
  string = gc_base_realloc (ctx->gc_ctx, str1, str_len);
  strcpy(string+len1, str2);
  gc_base_free(str2);
  return string;
}


static unsigned long datahash_pjw(unsigned long h, void *key, size_t keylen)
{
  unsigned long g;
  char *p;

  p = (char *) key;
  for (; keylen > 0; keylen--) {
    h = (h << 4) + *p++;
    if ((g = h & 0xF0000000U) != 0) {
      h = h ^ (g >> 28);
      h = h ^ g;
    }
  }
  return h;
}


static unsigned long objhash_rule_instance(void *state_inst)
{
  state_instance_t *si;
  unsigned long h;
  size_t i;
  size_t sync_var_sz;
  int32_t sync_var;
  ovm_var_t *var;

  h = 0;
  si = state_inst;
  sync_var_sz = si->pid->rule->sync_vars_sz;

  for (i = 0; i < sync_var_sz; i++) {
    sync_var = si->pid->rule->sync_vars[i];
    var = ovm_read_value (si->env, sync_var);

    h = datahash_pjw(h, &sync_var, sizeof (sync_var));
    h = datahash_pjw(h, &var->gc.type, sizeof (var->gc.type));
    h = datahash_pjw(h, issdl_get_data(var), issdl_get_data_len(var));
  }

  DebugLog(DF_ENG, DS_INFO, "Hashed state instance %p hcode=0x%08lx\n",
           state_inst, h);

  return (h);
}


static int objhash_rule_instance_cmp(void *state_inst1, void *state_inst2)
{
  state_instance_t *si1;
  state_instance_t *si2;
  size_t sync_var_sz;
  int32_t sync_var;
  ovm_var_t *var1;
  ovm_var_t *var2;
  size_t i;
  int ret;

  si1 = state_inst1;
  si2 = state_inst2;
  /* assert: si1->rule_instance->rule == si2->rule_instance->rule */

  sync_var_sz = si1->pid->rule->sync_vars_sz;

  /* for each sync vars, compare type and value */
  for (i = 0; i < sync_var_sz; i++) {
    sync_var = si1->pid->rule->sync_vars[i];

    var1 = ovm_read_value (si1->env, sync_var);
    var2 = ovm_read_value (si2->env, sync_var);

    /* call comparison functions */
    ret = issdl_cmp (var1, var2, CMP_LEQ_MASK | CMP_GEQ_MASK);
    if (CMP_LESS(ret))
      {
	DebugLog(DF_ENG, DS_INFO, "si1=%p < si2=%p\n", si1, si2);
	return -1;
      }
    if (CMP_GREATER(ret))
      {
	DebugLog(DF_ENG, DS_INFO, "si1=%p > si2=%p\n", si1, si2);
	return 1;
      }
    /* the case CMP_INCOMPARABLE(ret) should not happen,
       because of typing; so, if we are here, this is because var1==var2
    */
  }

  /* If we are here, the two state instances are synchronized */
  DebugLog(DF_ENG, DS_INFO, "si1=%p == si2=%p\n", si1, si2);

  return 0;
}

static int get_and_check_syncvar (rule_compiler_t *ctx,
				  node_rule_t *node_rule,
				  char *varname)
{
  node_expr_t *sync_var;
  type_t *t;

  sync_var = strhash_get (ctx->rule_env, varname);
  if (sync_var==NULL)
    {
      if (node_rule->filename!=NULL)
	fprintf (stderr, "%s:", node_rule->filename);
      fprintf (stderr, "%u: error: synchronization variable %s not mentioned in rule %s.\n",
	       node_rule->line, varname, node_rule->name);
      ctx->nerrors++;
      return -1;
    }
  t = sync_var->stype;
  if (t==NULL)
    {
      if (node_rule->filename!=NULL)
	fprintf (stderr, "%s:", node_rule->filename);
      fprintf (stderr, "%u: error: synchronization variable %s never assigned to in rule %s.\n",
	       node_rule->line, varname, node_rule->name);
      ctx->nerrors++;
      return SYM_RES_ID(sync_var);
    }
  if (!is_basic_type(t))
    {
      if (node_rule->filename!=NULL)
	fprintf (stderr, "%s:", node_rule->filename);
      fprintf (stderr, "%u: type error: synchronization variable %s:%s does not have a basic type.\n",
	       node_rule->line, varname, t->name);
      ctx->nerrors++;
    }
  return SYM_RES_ID(sync_var);
}

/*!!!*/

static int rule_sync_lock_mark_walk (void *key, void *elmt, void *data)
{
  gc_t *gc_ctx = (gc_t *)data;

  GC_TOUCH (gc_ctx, key);
  GC_TOUCH (gc_ctx, elmt);
  return 0;
}

static void rule_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  rule_t *rule = (rule_t *)p;
  size_t i, n;
  state_t *state;
  ovm_var_t **static_env;
  int32_t k, m;

  state = rule->state;
  if (state!=NULL)
    for (i=0, n=rule->state_nb; i<n; i++)
      {
	GC_TOUCH (gc_ctx, state[i].rule);
      }
  static_env = rule->static_env;
  if (static_env!=NULL)
    for (k=0, m=rule->static_env_sz; k<m; k++)
      GC_TOUCH (gc_ctx, static_env[k]);
  GC_TOUCH (gc_ctx, rule->next);
  if (rule->sync_lock!=NULL)
    {
      (void) objhash_walk (rule->sync_lock,
			   rule_sync_lock_mark_walk,
			   (void *)gc_ctx);
    }
}

static void transition_finalize (transition_t *trans)
{
  if (trans->required_fields!=NULL)
    gc_base_free (trans->required_fields);
  if (trans->eval_code!=NULL)
    gc_base_free (trans->eval_code);
}

static void rule_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  rule_t *rule = (rule_t *)p;
  state_t *state;
  size_t i, m, j, n;

  if (rule->state!=NULL)
    {
      for (i=0, m=rule->state_nb; i<m; i++)
	{
	  state = &rule->state[i];
	  if (state->name!=NULL)
	    gc_base_free (state->name);
	  if (state->action!=NULL)
	    gc_base_free (state->action);
	  if (state->trans!=NULL)
	    {
	      for (j=0, n=state->trans_nb; j<n; j++)
		transition_finalize (&state->trans[j]);
	      gc_base_free (state->trans);
	    }
	}
      gc_base_free (rule->state);
    }
  if (rule->filename!=NULL)
    gc_base_free (rule->filename);
  if (rule->name!=NULL)
    gc_base_free (rule->name);
  if (rule->static_env!=NULL)
    gc_base_free (rule->static_env);
  if (rule->var_name!=NULL)
    {
      int32_t i, n;
      char *s;

      for (i=0, n=rule->dynamic_env_sz; i<n; i++)
	{
	  s = rule->var_name[i];
	  if (s!=NULL)
	    gc_base_free (s);
	}
      gc_base_free (rule->var_name);
    }
  if (rule->sync_vars!=NULL)
    gc_base_free (rule->sync_vars);
  if (rule->sync_lock!=NULL)
    free_objhash(rule->sync_lock, NULL);
}

struct rt_data {
  gc_traverse_ctx_t *gtc;
  void *data;
};

static int rule_traverse_hash_walk_func (void *key, void *elt, void *data)
{
  struct rt_data *walk_data = (struct rt_data *)data;
  int err;

  err = (*walk_data->gtc->do_subfield) (walk_data->gtc,
					(gc_header_t *)key,
					walk_data->data);
  if (err)
    return err;
  err = (*walk_data->gtc->do_subfield) (walk_data->gtc,
					(gc_header_t *)elt,
					walk_data->data);
  return err;
}

static int rule_traverse (gc_traverse_ctx_t *gtc, gc_header_t *p,
			  void *data)
{
  rule_t *rule = (rule_t *)p;
  int err = 0;
  subfield_action do_subfield = gtc->do_subfield;
  struct rct_data walk_data = { gtc, data };
  size_t i, n;
  state_t *state;
  ovm_var_t **static_env;
  int32_t k, m;

  state = rule->state;
  if (state!=NULL)
    for (i=0, n=rule->state_nb; i<n; i++)
      {
	err = (*do_subfield) (gtc, (gc_header_t *)state[i].rule, data);
	if (err)
	  return err;
      }
  static_env = rule->static_env;
  if (static_env!=NULL)
    for (k=0, m=rule->static_env_sz; k<m; k++)
      {
	err = (*do_subfield) (gtc, (gc_header_t *)static_env[k], data);
	if (err)
	  return err;
      }
  err = (*do_subfield) (gtc, (gc_header_t *)rule->next, data);
  if (err)
    return err;
  err = 0;
  if (rule->sync_lock!=NULL)
    {
      err = objhash_walk (rule->sync_lock,
			  rule_traverse_hash_walk_func,
			  (void *)&walk_data);
    }
  return err;
}

static int save_bytecode (save_ctx_t *sctx, bytecode_t *bc, size_t n)
{
  size_t i;
  int err;

  for (i=0; i<n; i++)
    {
      err = save_ulong (sctx, bc[i]);
      if (err) return err;
    }
  return 0;
}

extern unsigned long bitcnt_tbl[]; /* in lang.c */

static unsigned long bitcnt_int32 (int32_t n)
{
  return bitcnt_tbl[n & 0xff] + bitcnt_tbl[(n >> 8) & 0xff]
    + bitcnt_tbl[(n >> 16) & 0xff] + bitcnt_tbl[(n >> 24) & 0xff];
}

static unsigned long bitcnt_int32_array (size_t n, int32_t *tbl)
{
  unsigned long cnt = 0L;
  size_t i;

  for (i=0; i<n; i++)
    cnt += bitcnt_int32 (tbl[i]);
  return cnt;
}

static int transition_save (save_ctx_t *sctx, state_t *state_array,
			    transition_t *t)
{
  size_t stateno;
  size_t i, n;
  unsigned long nreqfields;
  int32_t shift;
  int32_t field_num, bitmask;
  int err;

  stateno = t->dest - state_array;
  err = save_size_t (sctx, stateno);
  if (err) return err;
  n = t->required_fields_nb;
  nreqfields = bitcnt_int32_array (n, t->required_fields);
  err = save_ulong (sctx, nreqfields);
  if (err) return err;
  for (i=0, field_num=0; i<n; i++, field_num+=32)
    {
      bitmask = t->required_fields[i];
      if (bitmask==0)
	continue;
      for (shift=0; shift<32; shift++)
	if (bitmask & (1 << shift))
	  {
	    err = save_int32 (sctx, field_num+shift);
	    if (err) return err;
	  }
    }
  n = t->eval_code_length;
  err = save_size_t (sctx, n);
  if (err) return err;
  err = save_bytecode (sctx, t->eval_code, n);
  if (err) return err;
  err = save_int32 (sctx, t->id);
  if (err) return err;
  err = save_int32 (sctx, t->global_id);
  if (err) return err;
  err = save_uint32 (sctx, t->flags);
  return err;
}

static int state_save (save_ctx_t *sctx, state_t *state_array, state_t *state)
{
  size_t i, n;
  int err;

  err = save_string (sctx, state->name);
  if (err) return err;
  err = save_int32 (sctx, state->line);
  if (err) return err;
  n = state->actionlength;
  err = save_size_t (sctx, n);
  if (err) return err;
  err = save_bytecode (sctx, state->action, n);
  if (err) return err;
  n = state->trans_nb;
  err = save_size_t (sctx, n);
  if (err) return err;
  for (i=0; i<n; i++)
    {
      err = transition_save (sctx, state_array, &state->trans[i]);
      if (err) return err;
    }
  /* Do not save state->rule, which must be our parent anyway */
  err = save_uint32 (sctx, state->flags);
  if (err) return err;
  err = save_int32 (sctx, state->id);
  return err;
}

static int rule_sync_lock_save_walk (void *key, void *elmt, void *data)
{
  save_ctx_t *sctx = data;
  int err;

  err = save_gc_struct (sctx, key);
  if (err) return err;
  err = save_gc_struct (sctx, elmt);
  return err;
}

static int rule_save (save_ctx_t *sctx, gc_header_t *p)
{
  rule_t *rule = (rule_t *)p;
  size_t i, n;
  int32_t var, nvars;
  objhash_t *sync_lock;
  int err;

  err = save_string (sctx, rule->filename);
  if (err) return err;
  err = save_ctime (sctx, rule->file_mtime);
  if (err) return err;
  err = save_int32 (sctx, rule->lineno);
  if (err) return err;
  err = save_string (sctx, rule->name);
  if (err) return err;
  n = rule->state_nb;
  err = save_size_t (sctx, n);
  if (err) return err;
  for (i=0; i<n; i++)
    {
      err = state_save (sctx, rule->state, &rule->state[i]);
      if (err) return err;
    }
  /* rule->trans_nb is not saved, and will be recomputed at restore time */
  n = rule->static_env_sz;
  err = save_size_t (sctx, n);
  if (err) return err;
  for (i=0; i<n; i++)
    {
      err = save_gc_struct (sctx, (gc_header_t *)rule->static_env[i]);
      if (err) return err;
    }
  nvars = rule->dynamic_env_sz;
  err = save_int32 (sctx, nvars);
  if (err) return err;
  for (var=0; var<nvars; var++)
    {
      err = save_string (sctx, rule->var_name[var]);
      if (err) return err;
    }
  err = save_size_t (sctx, rule->instances);
  if (err) return err;
  err = save_int32 (sctx, rule->id); /* saved, but useless (will be restored, but then
					recomputed by install_rule()) */
  if (err) return err;
  err = save_int (sctx, rule->flags);
  if (err) return err;
  err = save_size_t (sctx, rule->sync_vars_sz);
  if (err) return err;
  nvars = rule->sync_vars_sz; /* saved as a size_t, but nvars is an int32_t */
  for (var=0; var<nvars; var++)
    {
      err = save_int32 (sctx, rule->sync_vars[var]);
      if (err) return err;
    }
  sync_lock = rule->sync_lock;
  if (sync_lock==NULL)
    err = save_size_t (sctx, 0);
  else
    {
      n = sync_lock->elmts;
      err = save_size_t (sctx, n);
      if (err) return err;
      err = objhash_walk (sync_lock, rule_sync_lock_save_walk,
			  (void *)sctx);
    }
  err = save_ulong (sctx, rule->complexity_degree);
  /*
    if (err) return err;
    err = save_gc_struct (sctx, (gc_header_t *)rule->next);
    No: do not save rule->next; this is only used to traverse
    the first_rule list in the compiler context.
  */
  return err;
}

gc_class_t rule_class;

int update_field_number (restore_ctx_t *rctx, int32_t *fld)
{
  int32_t field = *fld;
  char *name;
  field_record_t *f;

  if (field<0 || field>=rctx->global_fields->num_fields)
    return -3;
  name = rctx->global_fields->fields[field].name;
  f = strhash_get (rctx->rule_compiler->fields_hash, name);
  if (f==NULL)
    rctx->errs |= RESTORE_UNKNOWN_FIELD_NAME; /* unknown field name */
  else
    *fld = f->id; /* modify field number: make it the new one */
  return 0;
}

int update_primitive_number (restore_ctx_t *rctx, int32_t *prim)
{
  int32_t func = *prim;
  issdl_function_t *f, *ff;
  char *name;
  int32_t args_nb;
  int32_t i, n;

  if (func>=0 && func<rctx->vm_func_tbl_sz)
    {
      f = &rctx->vm_func_tbl[func];
      name = f->name;
      args_nb = f->args_nb;
      n = rctx->new_vm_func_tbl_sz;
      /* Do a linear scan to find the same name/args_nb pair.
	 This is not efficient, but not that much of a problem.
	 Specially if we first try the frequent case where this
	 will occur at the same position (func) as it was already.
      */
      if (func<n)
	{
	  ff = &rctx->new_vm_func_tbl[func];
	  if (ff->args_nb==args_nb && strcmp (ff->name, name)==0)
	    return 0;
	}
      for (i=0; i<n; i++)
	{
	  if (i==func)
	    continue;
	  ff = &rctx->new_vm_func_tbl[i];
	  if (ff->args_nb==args_nb && strcmp (ff->name, name)==0)
	    {
	      *prim = i;
	      return 0;
	    }
	}
    }
  rctx->errs |= RESTORE_UNKNOWN_PRIMITIVE;
  return 0;
}

static int do_update_fields (bytecode_t *bc, size_t sz,
			     bytecode_t *start, void *data)
{
  restore_ctx_t *rctx = data;
  int32_t fld;
  int err;

  switch (bc[0])
    {
    case OP_PUSHFIELD:
    case OP_ADD_EVENT:
      fld = (int32_t)bc[1];
      err = update_field_number (rctx, &fld);
      if (err) return err;
      bc[1] = (bytecode_t)fld; /* modify field number: make it the new one */
      break;
    case OP_CALL: /* check that primitive is known!!! */
      fld = (int32_t)bc[1];
      err = update_primitive_number (rctx, &fld);
      if (err) return err;
      bc[1] = (bytecode_t)fld; /* modify primitive number: make it the new one */
      break;
    default:
      break;
    }
  return 0;
}

static int restore_bytecode (restore_ctx_t *rctx, bytecode_t *bc, size_t n)
{
  size_t i;
  int err;

  for (i=0; i<n; i++)
    {
      err = restore_ulong (rctx, &bc[i]);
      if (err) return err;
    }
  /* Now we change all field numbers.  Here is why.
     Imagine the saved bytecode was bytecode for '$x = .mymodule.myfield;'.
     The access to .mymodule.myfield would be compiled as
     OP_PUSHFIELD <some-number>
     where <some-number> is a unique integer associated with .mymodule.myfield.
     There is no reason that, on restoring, .mymodule.myfield would
     have the same number, so we change them.
     This is needed for the OP_PUSHFIELD and OP_ADD_EVENT opcodes.
   */
  err = review_bytecode (bc, n, do_update_fields, rctx);
  return err;
}

static int transition_restore (restore_ctx_t *rctx, state_t *state_array,
			       transition_t *trans, size_t nstates)
{
  size_t stateno;
  size_t i, n;
  unsigned long j, nreqfields;
  int32_t *reqfields;
  int32_t field_num, field_num_max = -1;
  size_t idx, shift;
  int err;

  err = restore_size_t (rctx, &stateno);
  if (err) return err;
  if (stateno>=nstates) return -3;
  trans->dest = &state_array[stateno];
  err = restore_ulong (rctx, &nreqfields);
  if (err) return err;
  if (nreqfields!=0)
    {
      reqfields = gc_base_malloc (rctx->gc_ctx, nreqfields*sizeof(int32_t));
      for (j=0; j<nreqfields; j++)
	{
	  err = restore_int32 (rctx, &field_num);
	  if (err) { err_freereqflds: gc_base_free (reqfields);
	    return err; }
	  err = update_field_number (rctx, &field_num);
	  if (err) goto err_freereqflds;
	  if (field_num>field_num_max)
	    field_num_max = field_num;
	  reqfields[j] = field_num;
	}
      n = field_num_max / (8 * sizeof(int32_t)) + 1;
      trans->required_fields_nb = n;
      trans->required_fields = gc_base_malloc (rctx->gc_ctx, n*sizeof(int32_t));
      for (i=0; i<n; i++)
	trans->required_fields[i] = 0;
      for (j=0; j<nreqfields; j++)
	{
	  field_num = reqfields[j];
	  idx = field_num / (8 * sizeof(int32_t));
	  shift = field_num % (8 * sizeof(int32_t));
	  trans->required_fields[idx] |= (1 << shift);
	}
      gc_base_free (reqfields);
    }
  else
    {
      trans->required_fields_nb = 0;
      trans->required_fields = NULL;
    }
  err = restore_size_t (rctx, &n);
  trans->eval_code_length = n;
  trans->eval_code = NULL;
  if (n!=0)
    {
      trans->eval_code = gc_base_malloc (rctx->gc_ctx, n*sizeof(bytecode_t));
      err = restore_bytecode (rctx, trans->eval_code, n);
      if (err)
	{
	err_freecode:
	  if (trans->eval_code!=NULL)
	    gc_base_free (trans->eval_code);
	  if (trans->required_fields!=NULL)
	    gc_base_free (trans->required_fields);
	  return err;
	}
    }
  err = restore_int32 (rctx, &trans->id);
  if (err) goto err_freecode;
  err = restore_int32 (rctx, &trans->global_id);
  if (err) goto err_freecode;
  err = restore_uint32 (rctx, &trans->flags);
  if (err) goto err_freecode;
  return 0;
}

static int state_restore (restore_ctx_t *rctx, state_t *state_array,
			  state_t *state, size_t nstates)
{
  size_t i, n;
  int err;

  err = restore_string (rctx, &state->name);
  if (err) return err;
  if (state->name==NULL) return -2;
  err = restore_int32 (rctx, &state->line);
  if (err) { err_freename: gc_base_free (state->name); return err; }
  err = restore_size_t (rctx, &n);
  if (err) goto err_freename;
  state->actionlength = n;
  state->action = NULL;
  if (n!=0)
    {
      state->action = gc_base_malloc (rctx->gc_ctx, n*sizeof(bytecode_t));
      err = restore_bytecode (rctx, state->action, n);
      if (err)
	{
	err_freebc:
	  if (state->action!=NULL)
	    gc_base_free (state->action);
	  goto err_freename;
	}
    }
  err = restore_size_t (rctx, &n);
  if (err) goto err_freebc;
  state->trans_nb = n;
  state->trans = NULL;
  if (n!=0)
    {
      state->trans = gc_base_malloc (rctx->gc_ctx, n*sizeof(transition_t));
      for (i=0; i<n; i++)
	{
	  err = transition_restore (rctx, state_array, &state->trans[i],
				    nstates);
	  if (err)
	    {
	    err_freetrans:
	      if (i!=0)
		while (--i!=0)
		  transition_finalize (&state->trans[i]);
	      goto err_freebc;
	    }
	}
    }
  i = n; /* for further 'goto err_freetrans' to work, even if n==0 */
  err = restore_uint32 (rctx, &state->flags);
  if (err) goto err_freetrans;
  err = restore_int32 (rctx, &state->id);
  if (err) goto err_freetrans;
  if ((state->flags & STATE_EPSILON) && n==0)
    { err = -2; goto err_freetrans; } /* every epsilon state should have at least one outgoing transition */
  return 0;
}

static void install_rule (rule_compiler_t *ctx, rule_t *rule)
{
  gc_strhash_add(ctx->gc_ctx, ctx->rulenames_hash,
		 (gc_header_t *)rule,
		 gc_strdup (ctx->gc_ctx, rule->name));
  if (ctx->first_rule!=NULL)
    GC_TOUCH (ctx->gc_ctx, ctx->last_rule->next = rule);
  else
    GC_TOUCH (ctx->gc_ctx, ctx->first_rule = rule);
  GC_TOUCH (ctx->gc_ctx, ctx->last_rule = rule);
  rule->id = ctx->rules++;
}

static int equal_trans (transition_t *t1, transition_t *t2,
			state_t *sa1, state_t *sa2)
{
  if (t1->dest - sa1 != t2->dest - sa2)
    return 0;
  if (t1->eval_code_length != t2->eval_code_length)
    return 0;
  if (t1->eval_code_length!=0 &&
      memcmp (t1->eval_code, t2->eval_code, t1->eval_code_length*sizeof(bytecode_t)))
    return 0;
  return 1;
}

static int equal_states (state_t *s1, state_t *s2, state_t *sa1, state_t *sa2)
{
  size_t i, n;
    
  /* disregard names of states */
  if (s1->actionlength!=s2->actionlength)
    return 0;
  if (s1->actionlength!=0 &&
      memcmp (s1->action, s2->action, s1->actionlength*sizeof(bytecode_t)))
    return 0;
  if (s1->flags!=s2->flags)
    return 0;
  n = s1->trans_nb;
  if (n!=s2->trans_nb)
    return 0;
  for (i=0; i<n; i++)
    if (!equal_trans (&s1->trans[i], &s2->trans[i], sa1, sa2))
      return 0;
  return 1;
}

static int equal_rules (rule_t *r1, rule_t *r2)
{
  size_t i, n;
  
  if (r1->state_nb!=r2->state_nb)
    return 0;
  n = r1->state_nb;
  for (i=0; i<n; i++)
    if (!equal_states (&r1->state[i], &r2->state[i], r1->state, r2->state))
      return 0;
  n = r1->static_env_sz;
  if (n!=r2->static_env_sz)
    return 0;
  for (i=0; i<n; i++)
    if (issdl_cmp (r1->static_env[i], r2->static_env[i], CMP_LEQ_MASK | CMP_GEQ_MASK)
	!= (CMP_LEQ_MASK | CMP_GEQ_MASK))
      return 0;
  /* do not compare dynamic_env (size dynamic_env_sz, names var[]):
     we are allowed to change variable names */
  n = r1->sync_vars_sz;
  if (n!=r2->sync_vars_sz)
    return 0;
  for (i=0; i<n; i++)
    if (r1->sync_vars[i]!=r2->sync_vars[i])
      return 0;
  return 1;
}

struct merge_sync_lock_data {
  gc_t *gc_ctx;
  rule_t *oldrule;
};

static int do_merge_sync_lock (void *key, void *elmt, void *data)
{
  struct merge_sync_lock_data *sld = data;

  (void) objhash_add (sld->gc_ctx, sld->oldrule->sync_lock, elmt, key);
  return 0;
}

static rule_t *install_restored_rule (rule_compiler_t *ctx, rule_t *rule)
{
  rule_t *oldrule;
  size_t number = 1;
  char *s, *start, *send, *newname;
  struct merge_sync_lock_data sld;
  int found;

 again:
  oldrule = strhash_get (ctx->rulenames_hash, rule->name);
  if (oldrule==NULL)
    {
      install_rule (ctx, rule);
      return rule;
    }
  else
    /* Here we have a rule of the same name.
       We must decide whether it is the same rule, or a modified rule. */
    if (equal_rules (rule, oldrule))
      { /* If so, we merge the two rules, and keep the old one. */
	oldrule->instances += rule->instances;
	if (rule->flags & RULE_INITIAL_ALREADY_LAUNCHED)
	  oldrule->flags |= RULE_INITIAL_ALREADY_LAUNCHED;
	sld.gc_ctx = ctx->gc_ctx;
	sld.oldrule = oldrule;
	if (rule->sync_lock!=NULL)
	  (void) objhash_walk (rule->sync_lock, do_merge_sync_lock, &sld);
	return oldrule;
      }
    else
      { /* We insert the new rule with a slightly different name, namely:
	   - assume the initial rule name is "name"
	   - we try "name (2)", then if this is also taken, "name (3)", and so on.
	*/
	++number;
	found = 0;
	start = rule->name;
	s = send = start + strlen(start);
	--s;
	if (s>=start && *s==')')
	  for (; --s>start; )
	    {
	      if (*s=='(')
		{
		  found = 1;
		  break;
		}
	      else if (!isdigit(*s))
		break;
	    }
	if (!found)
	  s = send;
	newname = gc_base_malloc (ctx->gc_ctx, s-start + 3 + 16);
	memcpy (newname, start, s-start);
	sprintf (newname + (s-start), " (%zi)", number);
	gc_base_free (rule->name);
	rule->name = newname;
	goto again;
      }
}

static gc_header_t *rule_restore (restore_ctx_t *rctx)
{
  gc_t *gc_ctx = rctx->gc_ctx;
  rule_t *rule;
  time_t mtime;
  int32_t lineno;
  int err;
  char *filename, *name;
  size_t i, nstates, n;
  gc_header_t *p;
  int32_t var, nvars;
  char *s;
  gc_header_t *si, *pid;
  int32_t save_errs;

  GC_START(gc_ctx, 3);
  rule = NULL;
  save_errs = rctx->errs;
  rctx->errs = 0;
  err = restore_string (rctx, &filename);
  if (err) { errno = err; goto end; }
  err = restore_ctime (rctx, &mtime);
  if (err)
    {
      errno = err;
    err_freefilename:
      if (filename!=NULL)
	gc_base_free (filename);
      goto end;
    }
  err = restore_int32 (rctx, &lineno);
  if (err) { errno = err; goto err_freefilename; }
  err = restore_string (rctx, &name);
  if (err) { errno = err; goto err_freefilename; }
  if (name==NULL) { errno = -2; goto err_freefilename; }
  err = restore_size_t (rctx, &nstates);
  if (err)
    {
      errno = err;
      if (name!=NULL)
	gc_base_free (name);
      goto err_freefilename;
    }
  rule = gc_alloc (gc_ctx, sizeof(rule_t), &rule_class);
  rule->gc.type = T_RULE;
  rule->filename = filename;
  rule->file_mtime = mtime;
  rule->lineno = lineno;
  rule->name = name;
  rule->state_nb = 0; /* will progressively be increased to nstates below */
  rule->state = NULL;
  rule->trans_nb = 0; /* will be recomputed below */
  rule->static_env = NULL;
  rule->static_env_sz = 0; /* will be set below */
  rule->dynamic_env_sz = 0; /* will be set below */
  rule->var_name = NULL;
  rule->next = NULL;
  rule->instances = 0; /* will be set below */
  rule->id = 0; /* will be set below */
  rule->flags = 0; /* will be set below */
  rule->sync_lock = NULL;
  rule->sync_vars = NULL;
  rule->sync_vars_sz = 0; /* will be set below */
  rule->complexity_degree = ULONG_MAX; /* will be set below */
  GC_UPDATE (gc_ctx, 0, rule);
  rule->state = gc_base_malloc (gc_ctx, nstates*sizeof(state_t));
  for (i=0; i<nstates; )
    {
      err = state_restore (rctx, rule->state, &rule->state[i], nstates);
      if (err) { rule = NULL; errno = err; goto end; }
      GC_TOUCH (gc_ctx, rule->state[i].rule = rule);
      rule->trans_nb += rule->state[i].trans_nb;
      rule->state_nb = ++i;
    }
  err = restore_size_t (rctx, &n);
  if (err) { rule = NULL; errno = err; goto end; }
  rule->static_env = gc_base_malloc (gc_ctx, n*sizeof(ovm_var_t *));
  for (i=0; i<n; )
    {
      p = restore_gc_struct (rctx);
      if (errno) { rule = NULL; goto end; }
      GC_TOUCH (gc_ctx, rule->static_env[i] = (ovm_var_t *)p);
      rule->static_env_sz = ++i;
    }
  err = restore_int32 (rctx, &nvars);
  if (err) { rule = NULL; errno = err; goto end; }
  if (nvars<0) { rule = NULL; errno = -2; goto end; }
  rule->var_name = gc_base_malloc (gc_ctx, nvars*sizeof(char *));
  for (var=0; var<nvars; )
    {
      err = restore_string (rctx, &s);
      if (err) { rule = NULL; errno = err; goto end; }
      if (s==NULL) { rule = NULL; errno = -2; goto end; }
      rule->var_name[var] = s;
      rule->dynamic_env_sz = ++var;
    }
  err = restore_size_t (rctx, &rule->instances);
  if (err) { rule = NULL; errno = err; goto end; }
  err = restore_int32 (rctx, &rule->id); /* restored, but this is useless: it will be
					    recomputed by install_rule(), as called by
					    install_restored_rule() */
  if (err) { rule = NULL; errno = err; goto end; }
  err = restore_int (rctx, &rule->flags);
  if (err) { rule = NULL; errno = err; goto end; }
  err = restore_size_t (rctx, &n);
  if (err) { rule = NULL; errno = err; goto end; }
  nvars = n;
  if (nvars<0 || nvars>=rule->dynamic_env_sz)
    { rule = NULL; errno = -2; goto end; }
  rule->sync_vars_sz = n;
  if (nvars!=0)
    {
      rule->sync_vars = gc_base_malloc (gc_ctx, nvars*sizeof(int32_t));
      for (var=0; var<nvars; var++)
	{
	  err = restore_int32 (rctx, &rule->sync_vars[var]);
	  if (err) { rule = NULL; errno = err; goto end; }
	}
    }
  err = restore_size_t (rctx, &n);
  if (err) { rule = NULL; errno = err; goto end; }
  if (n!=0 && rule->sync_vars==0) { rule = NULL; errno = -2; goto end; }
  if (rule->sync_vars!=NULL)
    {
      rule->sync_lock = new_objhash (gc_ctx, 1021);
      rule->sync_lock->hash = objhash_rule_instance;
      rule->sync_lock->cmp = objhash_rule_instance_cmp;
      for (i=0; i<n; i++)
	{
	  si = restore_gc_struct (rctx);
	  if (si==NULL || TYPE(si)!=T_STATE_INSTANCE)
	    { errno = -2; rule = NULL; goto end; }
	  if (errno) { rule = NULL; goto end; }
	  GC_UPDATE (gc_ctx, 1, si);
	  if (objhash_get (rule->sync_lock, si)!=NULL) /* duplicate entry */
	    { rule = NULL; errno = -2; goto end; }
	  pid = restore_gc_struct (rctx);
	  if (pid==NULL || TYPE(pid)!=T_THREAD_GROUP)
	    { errno = -2; rule = NULL; goto end; }
	  if (errno) { rule = NULL; goto end; }
	  GC_UPDATE (gc_ctx, 2, pid);
	  objhash_add (gc_ctx, rule->sync_lock, pid, si);
	}
    }
  err = restore_ulong (rctx, &rule->complexity_degree);
  if (err) { rule = NULL; errno = err; goto end; }
  /*
    p = restore_gc_struct (rctx);
    if (p==NULL && errno!=0) { rule = NULL; goto end; }
    rule->next = (rule_t *)p;
    No: since we don't save the next field, don't restore it.
  */
  if (rctx->errs!=0)
    {
      if (rctx->errs & RESTORE_UNKNOWN_FIELD_NAME)
	rule->flags |= RULE_RESTORE_UNKNOWN_FIELD_NAME;
      if (rctx->errs & RESTORE_UNKNOWN_PRIMITIVE)
	rule->flags |= RESTORE_UNKNOWN_PRIMITIVE;
      /* And do not install the restored rule */
    }
  else
    rule = install_restored_rule (rctx->rule_compiler, rule);
 end:
  rctx->errs = save_errs;
  GC_END (gc_ctx);
  return (gc_header_t *)rule;
}

gc_class_t rule_class = {
  GC_ID('r','u','l','e'),
  rule_mark_subfields,
  rule_finalize,
  rule_traverse,
  rule_save,
  rule_restore
};

void compile_and_add_rule_ast(rule_compiler_t *ctx, node_rule_t *node_rule)
{
  rule_t *rule;
  size_t  s, t, m, n;
  int32_t tid;
  struct stat filestat;
  node_expr_t *l;

  DebugLog(DF_OLC, DS_INFO,
           "----- compiling rule \"%s\" (from file %s:%i) -----\n",
           node_rule->name, STR(ctx->currfile), node_rule->line);
  /* It is legal to pass STR(ctx->currfile) here, which was created
     NUL-terminated for that purpose */

  /* Check if we don't already have this rule definition */
  if ((rule = strhash_get(ctx->rulenames_hash, node_rule->name))) {
    if (ctx->currfile!=NULL)
      fprintf (stderr, "%s:", STR(ctx->currfile));
    fprintf (stderr, "%u: error: rule '%s' already defined in %s:%i\n",
	     ctx->issdllineno,
             node_rule->name, rule->filename, rule->lineno);
    ctx->nerrors++;
    return;
  }

  if (stat(node_rule->real_file, &filestat))
    filestat.st_mtime = 0;

  check_var_usage (ctx, node_rule);
  check_epsilon_transitions (ctx, node_rule);
  type_check (ctx, node_rule);
  check_sync_vars (ctx, node_rule);
  mark_no_wait_transitions (ctx, node_rule);
  evaluate_complexity (ctx, node_rule);

  GC_START(ctx->gc_ctx, 1);
  rule = gc_alloc (ctx->gc_ctx, sizeof (rule_t), &rule_class);
  rule->gc.type = T_RULE;
  rule->filename = NULL;
  rule->name = NULL;
  rule->state = NULL;
  rule->static_env = NULL;
  rule->var_name = NULL;
  rule->next = NULL;
  rule->sync_lock = NULL;
  rule->sync_vars = NULL;
  rule->sync_vars_sz = 0;
  GC_UPDATE(ctx->gc_ctx, 0, rule);
  rule->complexity_degree = node_rule->complexity_degree;

  rule->filename = gc_strdup (ctx->gc_ctx, node_rule->real_file);
  rule->file_mtime = filestat.st_mtime;
  rule->name = gc_strdup (ctx->gc_ctx, node_rule->name);
  rule->lineno = node_rule->line;
  rule->static_env_sz = ctx->statics_nb;
  rule->dynamic_env_sz = ctx->rule_env->elmts;
  // rule->id = ctx->rules;   will be set by install_rule() below
  rule->flags = 0;

  /* Allocate static env */
  rule->static_env = gc_base_malloc (ctx->gc_ctx,
				     rule->static_env_sz *
				     sizeof (ovm_var_t *));
  for (s = 0; s < rule->static_env_sz; s++)
    GC_TOUCH (ctx->gc_ctx,
	      rule->static_env[s] = ctx->statics[s]);

  /* Allocate dynamic env. variable names */
  if (rule->dynamic_env_sz > 0) {
    rule->var_name = gc_base_malloc(ctx->gc_ctx,
				    rule->dynamic_env_sz * sizeof (char *));
    for (s = 0; s < rule->dynamic_env_sz; s++)
      rule->var_name[s] = NULL; /* to prevent garbage collector bugs:
				   in case the gc_strdup()s below
				   call the garbage collector, which
				   may call rule_finalize() on
				   rule->var_name, which should be
				   filled with valid pointers---or NULL. */
    for (s = 0; s < rule->dynamic_env_sz; s++)
      rule->var_name[s] = gc_strdup (ctx->gc_ctx,
				     ctx->dyn_var_name[s]);
  }

  /* Create synchronization array */
  if (node_rule->sync_vars)
    {
      DebugLog(DF_OLC, DS_INFO, "Creating synchronization array\n");
      rule->sync_vars_sz = node_rule->sync_vars->vars_nb;
      rule->sync_vars = gc_base_malloc (ctx->gc_ctx,
					rule->sync_vars_sz * sizeof (int32_t));
      for (s = 0; s < rule->sync_vars_sz; s++)
	{
	  rule->sync_vars[s] =
	    get_and_check_syncvar (ctx, node_rule,
				   node_rule->sync_vars->vars[s]);
	}
      rule->sync_lock = new_objhash (ctx->gc_ctx, 1021);
      rule->sync_lock->hash = objhash_rule_instance;
      rule->sync_lock->cmp = objhash_rule_instance_cmp;
      /* XXX: should dynamically resize sync_lock hash. */
    }

  /* Compile init state */
  if (node_rule->init==NULL) {
    DebugLog(DF_OLC, DS_WARN, "!!! null init state\n");
    return ; /* XXX: Debug assertion : init state should not be NULL */
  }
  /* XXX - clean here */

  for (s=1, l=node_rule->statelist; l!=NULL; l=BIN_RVAL(l), s++)
    ((node_state_t *)BIN_LVAL(l))->state_id = s;
  rule->state_nb = s;
  rule->state = gc_base_malloc (ctx->gc_ctx,
				rule->state_nb * sizeof (state_t));
  for (s=0, m=rule->state_nb; s<m; s++)
    {
      rule->state[s].trans_nb = 0;
      rule->state[s].trans = NULL; /* to prevent garbage collector bugs:
				     in case the allocations below
				     call the garbage collector, which
				     may call rule_finalize() on
				     rule->state[i], which should be
				     filled with valid pointers---or NULL. */
      rule->state[s].rule = NULL; /* same */
    }
  compile_state_ast(ctx, rule, &(rule->state[0]), node_rule->init);
  rule->state[0].id = 0; /* set state id */

  tid = 0;
  for (t = 0, n=rule->state[0].trans_nb; t < n; t++)
    {
      rule->state[0].trans[t].global_id = tid++;
    }

  for (s=0, l=node_rule->statelist; l!=NULL; l=BIN_RVAL(l))
    {
      s++;
      compile_state_ast (ctx, rule, &rule->state[s],
			 (node_state_t *)BIN_LVAL(l));
      rule->state[s].id = s; /* set state id */
      /* Renumber transitions */
      for (t = 0, n=rule->state[s].trans_nb ; t < n ; t++)
	{
	  rule->state[s].trans[t].global_id = tid++;
	}
    }

  DebugLog(DF_OLC, DS_INFO,
           "----- end of compilation of rule \"%s\" (from file %s:%i) -----\n",
           node_rule->name, STR(ctx->currfile), node_rule->line);
  /* It is legal to pass STR(ctx->currfile) here, which was created
     NUL-terminated for that purpose */
  install_rule (ctx, rule);

  GC_END(ctx->gc_ctx);
}


/**
 * Compile a state node abstract syntax tree.
 * @param ctx Rule compiler context.
 * @param rule Current rule in compilation.
 * @param state Current state in compilation.
 * @param node_state State node abstract syntax tree root to compile.
 **/
static void compile_state_ast(rule_compiler_t *ctx,
			      rule_t          *rule,
			      state_t         *state,
			      node_state_t    *node_state)
{
  DebugLog(DF_OLC, DS_INFO,
           "compiling state '%s' in rule '%s'.\n",
           node_state->name, rule->name);

  state->name  = gc_strdup (ctx->gc_ctx, node_state->name);
  state->line  = node_state->line;
  state->flags = node_state->flags;
  GC_TOUCH (ctx->gc_ctx, state->rule  = rule);

  compile_actions_ast(ctx, rule, state, node_state->actionlist);
  compile_transitions_ast(ctx, rule, state, node_state->translist);
}


static void compile_set_labels(bytecode_buffer_t *code)
{
  int	i;

  for (i = 0; i < code->labels.toset_nb; i++)
  {
    int code_pos = code->labels.toset[i];
    int	lbl_pos = code->labels.labels[code->bytecode[code_pos]];

    code->bytecode[code_pos] = lbl_pos - code_pos - 1;
  }
}

static void reset_used_fields (bytecode_buffer_t *code)
{
  if (code->used_fields!=NULL)
    {
      gc_base_free (code->used_fields);
      code->used_fields = NULL;
    }
  code->used_fields_sz = 0;
}

/**
 ** Compile an action list node abstract syntax tree.
 ** @param ctx Rule compiler context.
 ** @param rule Current rule in compilation.
 ** @param state Current state in compilation.
 ** @param actionlist Action list node abstract syntax tree root to compile.
 **/
static void compile_actions_ast(rule_compiler_t   *ctx,
				rule_t            *rule,
				state_t           *state,
				node_expr_t       *actionlist)
{
  bytecode_buffer_t code;
  bytecode_t *bytecode;


  DebugLog(DF_OLC, DS_DEBUG,
           "compiling actions in state '%s' in rule '%s'.\n",
           state->name, rule->name);

  if (actionlist!=NULL)
    {
      DebugLog(DF_OLC, DS_DEBUG, "compiling actions bytecode\n");

      code.bytecode[0] = '\0';
      code.pos = 0;
      code.used_fields_sz = 0;
      code.used_fields = NULL;
      code.flags = 0;
      code.ctx = ctx;
      code.stack_level = 0;
      code.logicals_size = 0;
      code.logicals = NULL;

      INIT_LABELS(ctx, code.labels);

      compile_actions_bytecode(actionlist, &code);
      if (code.pos >= BYTECODE_BUF_SZ - 1) {
	DebugLog(DF_OLC, DS_FATAL, "Bytecode buffer full\n");
	exit(EXIT_FAILURE);
      }
      code.bytecode[code.pos++] = OP_END;

      compile_set_labels(&code);

      DebugLog(DF_OLC, DS_DEBUG, "bytecode size: %i\n", code.pos);

      bytecode = gc_base_malloc (ctx->gc_ctx,
				 code.pos * sizeof (bytecode_t));
      memcpy(bytecode, code.bytecode, code.pos * sizeof (bytecode_t));

      /*
      if (code.flags & BYTECODE_HAS_PUSHFIELD) {
	state->flags |= BYTECODE_HAS_PUSHFIELD;
      }
      */

      state->actionlength = code.pos;
      state->action = bytecode;
      FREE_LABELS(code.labels);
      reset_used_fields (&code); /* save some memory */
    }
  else
    {
      state->actionlength = 0;
      state->action = NULL;
      DebugLog(DF_OLC, DS_INFO, "state '%s' has no action.\n", state->name);
    }
}

/**
 * Compile a list of action expressions into byte code.
 * @param expr A list of action expressions.
 * @param state State to compile.
 * @return An allocated byte code buffer.
 **/
static void compile_actions_bytecode(node_expr_t *actionlist,
				     bytecode_buffer_t *code)
{
  for (; actionlist!=NULL; actionlist=BIN_RVAL(actionlist))
    compile_bytecode_stmt(BIN_LVAL(actionlist), code);
  if (code->pos >= BYTECODE_BUF_SZ - 2) {
    DebugLog(DF_OLC, DS_FATAL, "Bytecode buffer full\n");
    exit(EXIT_FAILURE);
  }
}

static size_t count_conjuncts (node_expr_t *e, int neg)
{
  if (e==NULL)
    return 0;
  switch (e->type)
    {
    case NODE_COND:
      switch (BIN_OP (e))
	{
	case ANDAND:
	  if (neg)
	    return 1;
	  return count_conjuncts (BIN_LVAL(e), neg)+count_conjuncts (BIN_RVAL(e), neg);
	case OROR:
	  if (neg)
	    return count_conjuncts (BIN_LVAL(e), neg)+count_conjuncts (BIN_RVAL(e), neg);
	  return 1;
	case BANG:
	  return count_conjuncts (BIN_RVAL(e), !neg);
	default:
	  return 1;
	}
      break;
    default:
      return 1;
    }
}

typedef struct normalized_trans_s normalized_trans_t;
struct normalized_trans_s {
  size_t npersistent;
  node_expr_t **persistent;
  char *persneg;
  size_t nother;
  node_expr_t **other;
  char *otherneg;
};

static void normalize_trans_cond_rec (rule_compiler_t *ctx, node_expr_t *e,
				      normalized_trans_t *nt,
				      char neg);

static void normalize_trans_cond_rec (rule_compiler_t *ctx, node_expr_t *e,
				      normalized_trans_t *nt,
				      char neg)
{
  size_t n;
  monotony m;
  
  if (e==NULL)
    return;
  switch (e->type)
    {
    case NODE_COND:
      switch (BIN_OP(e))
	{
	case ANDAND:
	  if (neg)
	    goto basecase;
	  normalize_trans_cond_rec (ctx, BIN_LVAL(e), nt, neg);
	  normalize_trans_cond_rec (ctx, BIN_RVAL(e), nt, neg);
	  break;
	case OROR:
	  if (!neg)
	    goto basecase;
	  normalize_trans_cond_rec (ctx, BIN_LVAL(e), nt, neg);
	  normalize_trans_cond_rec (ctx, BIN_RVAL(e), nt, neg);
	  break;
	case BANG:
	  normalize_trans_cond_rec (ctx, BIN_RVAL(e), nt, !neg);
	  break;
	default: goto basecase;
	}
      break;
    default:
    basecase:
      m = compute_monotony_simple (ctx, e);
      /* put e into persistent[] list if and only if
	 neg==false and e is antitonic (if it fails now, it will fail forever),
	 or neg==true and e is monotonic */
      if (neg?(m & MONO_MONO):(m & MONO_ANTI))
	{
	  n = nt->npersistent++;
	  nt->persistent[n] = e;
	  nt->persneg[n] = neg;
	}
      else
	{
	  n = nt->nother++;
	  nt->other[n] = e;
	  nt->otherneg[n] = neg;
	}
      break;
    }
}

static normalized_trans_t normalize_trans_cond (rule_compiler_t *ctx, node_expr_t *e)
{
  size_t nconjuncts;
  normalized_trans_t nt;

  nconjuncts = count_conjuncts (e, 0);
  nt.npersistent = 0;
  nt.persistent = gc_base_malloc (ctx->gc_ctx, nconjuncts*sizeof(node_expr_t *));
  nt.persneg = gc_base_malloc (ctx->gc_ctx, nconjuncts*sizeof(char));
  nt.nother = 0;
  nt.other = gc_base_malloc (ctx->gc_ctx, nconjuncts*sizeof(node_expr_t *));
  nt.otherneg = gc_base_malloc (ctx->gc_ctx, nconjuncts*sizeof(char));
  normalize_trans_cond_rec (ctx, e, &nt, 0);
  return nt;
}

static void compile_bytecode_trans_cond (node_expr_t *e,
					 bytecode_buffer_t *code)
{
  normalized_trans_t nt;
  unsigned int label_ok, label_fail, label_will_always_fail;
  size_t i;

  label_ok = NEW_LABEL(code->labels)
  nt = normalize_trans_cond (code->ctx, e);
  if (nt.nother!=0)
    label_fail = NEW_LABEL(code->labels)
  else label_fail = 0;
  if (nt.npersistent!=0)
    label_will_always_fail = NEW_LABEL(code->labels)
  else label_will_always_fail = 0;
  for (i=0; i<nt.npersistent; i++)
    {
      if (nt.persneg[i])
	compile_bytecode_cond (nt.persistent[i], code, label_will_always_fail, label_ok,
			       COND_ELSE_IMMEDIATE);
      else
	compile_bytecode_cond (nt.persistent[i], code, label_ok, label_will_always_fail,
			       COND_THEN_IMMEDIATE);
      SET_LABEL (code->ctx, code->labels, code->pos, label_ok);
      label_ok = NEW_LABEL(code->labels)
    }
  for (i=0; i<nt.nother; i++)
    {
      if (nt.otherneg[i])
	compile_bytecode_cond (nt.other[i], code, label_fail, label_ok, COND_ELSE_IMMEDIATE);
      else
	compile_bytecode_cond (nt.other[i], code, label_ok, label_fail, COND_THEN_IMMEDIATE);
      SET_LABEL (code->ctx, code->labels, code->pos, label_ok);
      label_ok = NEW_LABEL(code->labels)
    }
  EXIT_IF_BYTECODE_BUFF_FULL(6);
  SET_LABEL (code->ctx, code->labels, code->pos, label_ok);
  code->bytecode[code->pos++] = OP_PUSHONE;
  code->bytecode[code->pos++] = OP_END;
  if (nt.nother!=0)
    {
      SET_LABEL (code->ctx, code->labels, code->pos, label_fail);
      code->bytecode[code->pos++] = OP_PUSHZERO;
      code->bytecode[code->pos++] = OP_END;
    }
  if (nt.npersistent!=0)
    {
      SET_LABEL (code->ctx, code->labels, code->pos, label_will_always_fail);
      code->bytecode[code->pos++] = OP_PUSHMINUSONE;
      code->bytecode[code->pos++] = OP_END;
    }
  gc_base_free (nt.persistent);
  gc_base_free (nt.persneg);
  gc_base_free (nt.other);
  gc_base_free (nt.otherneg);
}

/**
 * Compile an evaluation expression into bytecode.
 *   @param ctx Rule compiler context.
 *   @param expr  An evaluation expression.
 *   @param trans Transition to compile.
 *   @return An allocated byte code buffer.
 **/
static bytecode_t *compile_trans_bytecode(rule_compiler_t  *ctx,
					  node_expr_t	*expr,
					  transition_t	*trans)
{
  bytecode_buffer_t code;

  DebugLog(DF_OLC, DS_DEBUG, "compiling bytecode\n");

  code.bytecode[0] = '\0';
  code.pos = 0;
  code.used_fields_sz = 0;
  code.used_fields = NULL;
  code.flags = 0;
  code.ctx = ctx;
  code.stack_level = 0;
  code.logicals_size = 0;
  code.logicals = NULL;

  INIT_LABELS(ctx, code.labels);
  compile_bytecode_trans_cond(expr, &code);
  compile_set_labels(&code);

  DebugLog(DF_OLC, DS_TRACE, "bytecode size: %i\n", code.pos);
  DebugLog(DF_OLC, DS_TRACE, "used fields: %i\n", code.used_fields_sz);

  trans->eval_code_length = code.pos;
  trans->eval_code = gc_base_malloc (ctx->gc_ctx,
				     code.pos * sizeof (bytecode_t));
  memcpy(trans->eval_code, code.bytecode, code.pos * sizeof (bytecode_t));

  trans->required_fields_nb = code.used_fields_sz;
  if (code.used_fields_sz > 0)
    {
      trans->required_fields = code.used_fields;
      code.used_fields = NULL;
      code.used_fields_sz = 0;
      /*
      trans->required_fields = gc_base_malloc (ctx->gc_ctx,
					       code.used_fields_sz
					       * sizeof(int32_t));
      memcpy (trans->required_fields, code.used_fields,
	      code.used_fields_sz * sizeof(int32_t));
      */
    }
  FREE_LABELS(code.labels);
  return trans->eval_code;
}


/**
 * Byte code compiler : compile statement
 * @param expr An expression to compile.
 * @param code An internal byte code buffer structure.
 **/
static void compile_bytecode_stmt(node_expr_t *expr,
				  bytecode_buffer_t *code)
{
  switch (expr->type)
  {
    case NODE_ASSOC:
      compile_bytecode_expr(BIN_RVAL(expr), code);
      EXIT_IF_BYTECODE_BUFF_FULL(2);
      code->bytecode[ code->pos++ ] = OP_POP;
      code->bytecode[ code->pos++ ] = SYM_RES_ID(BIN_LVAL(expr));
      code->stack_level--;
      break;

    case NODE_BINOP:
      /* No: binops are expressions that return values, so
	 the following old code is wrong.
      compile_bytecode_stmt(gc_ctx, expr->bin.lval, code);
      compile_bytecode_stmt(gc_ctx, expr->bin.rval, code);
      code->bytecode[ code->pos++ ] = expr->bin.op;
      break;
      */
      /*FALLTHROUGH*/
  case NODE_MONOP: /*FALLTHROUGH*/
  case NODE_EVENT: /*FALLTHROUGH*/
  case NODE_DB_PATTERN: /*FALLTHROUGH*/
  case NODE_DB_COLLECT: /*FALLTHROUGH*/
  case NODE_DB_SINGLETON: /*FALLTHROUGH*/
  case NODE_CALL:
      compile_bytecode_expr(expr, code);
      EXIT_IF_BYTECODE_BUFF_FULL(1);
      /* Trash the function result */
      code->bytecode[ code->pos++ ] = OP_TRASH;
      code->stack_level--;
      break;

  case NODE_RETURN:
    compile_bytecode_expr (MON_VAL(expr), code);
    EXIT_IF_BYTECODE_BUFF_FULL(1);
    code->bytecode[code->pos++] = OP_END;
    code->stack_level--;
    break;

  case NODE_BREAK:
    EXIT_IF_BYTECODE_BUFF_FULL(2);
    code->bytecode[code->pos++] = OP_PUSHNULL;
    code->bytecode[code->pos++] = OP_END;
    /* code->stack_level += 0; */
    break;

  case NODE_COND:
    {
      unsigned int label_end = NEW_LABEL(code->labels);

      compile_bytecode_cond(expr, code, label_end, label_end,
			    COND_THEN_IMMEDIATE | COND_ELSE_IMMEDIATE);
      SET_LABEL (code->ctx, code->labels, code->pos, label_end);
      break;
    }

  case NODE_REGSPLIT:
    {
      size_t i, n;

      compile_bytecode_expr(REGSPLIT_STRING(expr), code);
      
      /* size of a regsplit compiled code:
	 2 PUSHSTATIC 1 REGSPLIT n POPs
	 = 3 + 2n */
      if (REGSPLIT_DEST_VARS(expr)!=NULL)
	n = REGSPLIT_DEST_VARS(expr)->vars_nb;
      else n = 0;
      EXIT_IF_BYTECODE_BUFF_FULL(3 + 2 * n);
      
      /* XXX actually, split string can only be in static env */
      code->bytecode[ code->pos++ ] = OP_PUSHSTATIC;
      code->bytecode[ code->pos++ ] = TERM_RES_ID(REGSPLIT_PAT(expr));
      
	/* the regsplit instruction itself */
      code->bytecode[ code->pos++ ] = OP_REGSPLIT;
      
      /* POPs instructions to associate value to variables */
      for (i = 0; i < n; i++)
	{
	  code->bytecode[ code->pos++ ] = OP_POP;
	  code->bytecode[ code->pos++ ] = SYM_RES_ID(REGSPLIT_DEST_VARS(expr)->vars[i]);
	}
      code->stack_level--;
      break;
    }
    
  case NODE_IFSTMT:
    {
      int label_then = NEW_LABEL(code->labels);
      int label_else = NEW_LABEL(code->labels);
      int label_end = NEW_LABEL(code->labels);
      int save_stack_level;

      save_stack_level = code->stack_level;
      compile_bytecode_cond(IF_COND(expr), code,
			    label_then, label_else, COND_THEN_IMMEDIATE);
      /* now compile then branch;
	 first set label_then to here */
      SET_LABEL (code->ctx, code->labels,  code->pos, label_then);
      code->stack_level = save_stack_level;
      compile_actions_bytecode(IF_THEN(expr), code);
      EXIT_IF_BYTECODE_BUFF_FULL(2);
      code->bytecode[ code->pos++ ] = OP_JMP;
      PUT_LABEL (code->ctx, code->labels, code, label_end);
      /* now compile else branch;
	 set label_else to here
	 and do not forget to reinstall stack_level
      */
      SET_LABEL (code->ctx, code->labels, code->pos, label_else);
      code->stack_level = save_stack_level;
      compile_actions_bytecode(IF_ELSE(expr), code);
      SET_LABEL (code->ctx, code->labels, code->pos, label_end);
      /* finally, proceed: reinstall old stack level first */
      code->stack_level = save_stack_level;
      break;
    }
  case NODE_UNKNOWN:
  case NODE_FIELD:
  case NODE_VARIABLE:
  case NODE_CONST:
    // XXX Should discard expression with no side effect
#if 0
    DPRINTF( ("Rule compiler expr : should be discarded %i\n", expr->type) );
    compile_bytecode_expr(gc_ctx, expr, code);
    EXIT_IF_BYTECODE_BUFF_FULL(1);
    /* Trash the result */
    code->bytecode[ code->pos++ ] = OP_TRASH;
    code->stack_level--;
#endif
    break;
  default:
    DPRINTF( ("unknown node type (%i)\n", expr->type) );
    /* No side effect, discard stmt */
    break;
  }
}

static int nlogicals_in_tuple (node_expr_t *tuple)
{
  int maxvar=0;
  int var;

  for (; tuple!=NULL; tuple=BIN_RVAL(tuple))
    {
      if (is_logical_variable(BIN_LVAL(tuple)))
	{
	  var = SYM_RES_ID(BIN_LVAL(tuple));
	  if (var>=maxvar)
	    maxvar = var+1;
	}
    }
  return maxvar;
}

static int nlogicals_in_db_patterns(node_expr_t *patterns)
{
  int maxvar=0;
  int var;

  for (; patterns!=NULL; patterns = BIN_RVAL(patterns))
    {
      var = nlogicals_in_tuple(BIN_RVAL(BIN_LVAL(patterns)));
      if (var>maxvar)
	maxvar = var;
    }
  return maxvar;
}


static void compile_bytecode_db_filter (int nvars,
					node_expr_t *pattern,
					bytecode_buffer_t *code,
					int *varpos,
					int *n)
  /* (possibly) compile an OP_DB_FILTER opcode
   ==> filtered-db
  */
{
  int i, nfields, nconsts, nfields_res;
  node_expr_t *l;
  int *pos; /* will map (logical) variable numbers to field positions */
  int var;
  db_var_or_cst *vals;

  compile_bytecode_expr (BIN_LVAL(pattern), code);
  pos = gc_base_malloc (code->ctx->gc_ctx, nvars*sizeof(int));
  for (i=0; i<nvars; i++)
    varpos[i] = pos[i] = -1;
  nfields_res = 0;
  nconsts = 0;
  nfields = list_len(BIN_RVAL(pattern));
  EXIT_IF_BYTECODE_BUFF_FULL(4+nfields);
  vals = &code->bytecode[code->pos+4];
  for (l=BIN_RVAL(pattern), i=0; l!=NULL; l=BIN_RVAL(l), i++)
    {
      if (is_logical_variable(BIN_LVAL(l)))
	{
	  var = SYM_RES_ID(BIN_LVAL(l));
	  if (pos[var] < 0)
	    {
	      vals[i] = DB_VAR_NONE; /* no condition on this field */
	      pos[var] = i;
	      varpos[var] = nfields_res++;
	    }
	  else
	    {
	      vals[i] = DB_MAKE_VAR(pos[var]); /* must match field number
						  pos[var], a strictly
						  lower field number */
	    }
	}
      else /* must evaluate pattern argument, which will be compared
	      against current field, literally */
	{
	  compile_bytecode_expr (BIN_LVAL(l), code);
	  nconsts++;
	}
    }
  gc_base_free (pos);
  if (nfields==nfields_res) /* then no filtering really has to take
			       place, so we don't compile the
			       OP_DB_FILTER opcode after all */
    ;
  else
    {
      code->bytecode[code->pos++] = OP_DB_FILTER;
      code->bytecode[code->pos++] = (bytecode_t)nfields_res;
      code->bytecode[code->pos++] = (bytecode_t)nfields;
      code->bytecode[code->pos++] = (bytecode_t)nconsts;
      code->pos += nfields;
      code->stack_level -= nconsts;
    }
  *n = nfields_res;
}

static void compile_bytecode_db_patterns (int nvars,
					  node_expr_t *patterns,
					  bytecode_buffer_t *code,
					  int *varpos,
					  int *n)
/* patterns should not be NULL here */
{
  int *varpos_right;
  int i, ii, j, nfields1, nfields2;
  db_var_or_cst *vals;

  if (BIN_RVAL(patterns)==NULL)
    {
      compile_bytecode_db_filter (nvars, BIN_LVAL(patterns),
				  code, varpos, n);
    }
  else
    {
      varpos_right = gc_base_malloc (code->ctx->gc_ctx, nvars*sizeof(int));
      compile_bytecode_db_filter (nvars, BIN_LVAL(patterns),
				  code, varpos, &nfields1);
      compile_bytecode_db_patterns (nvars, BIN_RVAL(patterns),
				    code, varpos_right, &nfields2);
      EXIT_IF_BYTECODE_BUFF_FULL(3+nfields2);
      vals = &code->bytecode[code->pos+3];
      for (i=0; i<nfields2; i++)
	vals[i] = DB_VAR_NONE;
      for (i=0; i<nvars; i++)
	{
	  if (varpos[i] >= 0 && varpos_right[i] >= 0)
	    vals[varpos_right[i]] = DB_MAKE_VAR(varpos[i]);
	}
      for (i=0, j=nfields1; i<nfields2; i++)
	{
	  if (vals[i]==DB_VAR_NONE)
	    j++;
	}
      *n = j;
      for (i=0; i<nvars; i++)
	if (varpos[i] < 0 && varpos_right[i] >= 0)
	  {
	    for (ii=0, j=nfields1; ii<varpos_right[i]; ii++)
	      if (vals[ii]==DB_VAR_NONE)
		j++;
	    varpos[i] = j;
	  }
      code->bytecode[code->pos++] = OP_DB_JOIN;
      code->bytecode[code->pos++] = nfields1;
      code->bytecode[code->pos++] = nfields2;
      code->pos += nfields2;
      code->stack_level--;
      gc_base_free (varpos_right);
    }
}

static void compile_bytecode_db_collect (node_expr_t *actions,
					 node_expr_t *head,
					 node_expr_t *patterns,
					 bytecode_buffer_t *code)
/* patterns is not NULL */
{
  int *varpos;
  int i;
  int nvars, nfields;
  size_t newsz, oldsz;
  int *old_logicals;
  int save_stack_level;
  node_expr_t *l;
  unsigned int label_end = NEW_LABEL(code->labels);

  save_stack_level = code->stack_level;
  nvars = nlogicals_in_db_patterns(patterns);
  varpos = gc_base_malloc (code->ctx->gc_ctx, nvars*sizeof(int));
  compile_bytecode_db_patterns(nvars, patterns, code, varpos, &nfields);
  /* XXX Test whether we are lucky, and the join we have just computed
     is the final database, directly. */
  EXIT_IF_BYTECODE_BUFF_FULL(3);
  code->bytecode[code->pos++] = OP_DB_MAP;
  PUT_LABEL (code->ctx, code->labels, code, label_end);
  code->bytecode[code->pos++] = nfields;
  old_logicals = code->logicals;
  newsz = nvars;
  oldsz = code->logicals_size;
  if (newsz < oldsz)
    {
      code->logicals = gc_base_malloc (code->ctx->gc_ctx, oldsz*sizeof(int));
      for (i=0; i<oldsz; i++)
	code->logicals[i] = old_logicals[i];
      newsz = oldsz;
    }
  else
    {
      code->logicals = gc_base_malloc (code->ctx->gc_ctx, newsz*sizeof(int));
      for (i=0; i<oldsz; i++)
	code->logicals[i] = old_logicals[i];
      for (; i<newsz; i++)
	code->logicals[i] = -1;
    }
  for (i=0; i<nvars; i++)
    if (varpos[i] >= 0)
      code->logicals[i] = code->stack_level + varpos[i];
  gc_base_free(varpos);
  for (l=actions; l!=NULL; l=BIN_RVAL(l))
    compile_bytecode_stmt(BIN_LVAL(l), code);
  compile_bytecode_expr (head, code);
  EXIT_IF_BYTECODE_BUFF_FULL(1);
  code->bytecode[code->pos++] = OP_END;
  gc_base_free(code->logicals);
  code->logicals = old_logicals;
  code->logicals_size = oldsz;
  SET_LABEL (code->ctx, code->labels, code->pos, label_end);
  code->stack_level = save_stack_level+1;
}

static void compile_bytecode_db_singleton(node_expr_t *tuple,
					  bytecode_buffer_t *code)
{
  node_expr_t *l;
  int nfields;

  for (l=tuple, nfields=0; l!=NULL; l=BIN_RVAL(l), nfields++)
    compile_bytecode_expr (BIN_LVAL(l), code);
  EXIT_IF_BYTECODE_BUFF_FULL(2);
  code->bytecode[ code->pos++ ] = OP_DB_SINGLE;
  code->bytecode[ code->pos++ ] = nfields;
  code->stack_level -= nfields-1;
}

/**
 * Byte code compiler recursive sub-routine.
 * @param expr An expression to compile.
 * @param code An internal byte code buffer structure.
 **/
static void compile_bytecode_expr(node_expr_t *expr, bytecode_buffer_t *code)
{
  switch (expr->type)
    {
    case NODE_ASSOC:
      compile_bytecode_expr(BIN_RVAL(expr), code);
      EXIT_IF_BYTECODE_BUFF_FULL(4);
      code->bytecode[ code->pos++ ] = OP_POP;
      code->bytecode[ code->pos++ ] = SYM_RES_ID(BIN_LVAL(expr));
      code->bytecode[ code->pos++ ] = OP_PUSH;
      code->bytecode[ code->pos++ ] = SYM_RES_ID(BIN_LVAL(expr));
      /*code->stack_level += 0; */
      break;

      /* binary operator */
    case NODE_BINOP:
      compile_bytecode_expr(BIN_LVAL(expr), code);
      compile_bytecode_expr(BIN_RVAL(expr), code);
      EXIT_IF_BYTECODE_BUFF_FULL(1);
      code->bytecode[ code->pos++ ] = BIN_OP(expr);
      code->stack_level--;
      break;

      /* unary operator */
    case NODE_MONOP:
      compile_bytecode_expr(MON_VAL(expr), code);
      if (MON_OP(expr)!=OP_NOP)
	{
	  EXIT_IF_BYTECODE_BUFF_FULL(1);
	  code->bytecode[ code->pos++ ] = MON_OP(expr);
	  /*code->stack_level += 0; */
	}
      break;

      /* meta-event */
    case NODE_EVENT:
      if (BIN_LVAL(expr)!=NULL)
	compile_bytecode_expr(BIN_LVAL(expr), code);
      else
	{
	  EXIT_IF_BYTECODE_BUFF_FULL(1);
	  code->bytecode[code->pos++] = OP_EMPTY_EVENT;
	  code->stack_level++;
	}
      {
	node_expr_t *l;

	for (l=BIN_RVAL(expr); l!=NULL; l=BIN_RVAL(l))
	  {
	    node_expr_t *add_event_expr = BIN_LVAL(l);
	    node_expr_t *field_name = BIN_LVAL(add_event_expr);
	    int res_id = SYM_RES_ID(field_name);
	    node_expr_t *field_value = BIN_RVAL(add_event_expr);

	    compile_bytecode_expr(field_value, code);
	    EXIT_IF_BYTECODE_BUFF_FULL(2);
	    code->bytecode[code->pos++] = OP_ADD_EVENT;
	    code->bytecode[code->pos++] = (bytecode_t)res_id;
	    code->stack_level--;
	  }
      }
      break;

    case NODE_DB_COLLECT:
      expr = BIN_RVAL(expr); /* skip 'returns' part */
      compile_bytecode_db_collect(BIN_LVAL(BIN_LVAL(expr)),
				  BIN_RVAL(BIN_LVAL(expr)),
				  BIN_RVAL(expr),
				  code);
      break;

    case NODE_DB_SINGLETON:
      compile_bytecode_db_singleton(MON_VAL(expr), code);
      break;

    case NODE_COND:
      switch (BIN_OP(expr))
	{
	case OP_CEQ:
	case OP_CNEQ:
	case OP_CRM:
	case OP_CNRM:
	case OP_CGT:
	case OP_CLT:
	case OP_CGE:
	case OP_CLE:
	  compile_bytecode_expr(BIN_LVAL(expr), code);
	  compile_bytecode_expr(BIN_RVAL(expr), code);
	  EXIT_IF_BYTECODE_BUFF_FULL(1);
	  code->bytecode[ code->pos++ ] = BIN_OP(expr);
	  code->stack_level--;
	  break;
	default:
	  {
	    unsigned int label_then = NEW_LABEL(code->labels)
	    unsigned int label_else = NEW_LABEL(code->labels)
	    unsigned int label_end = NEW_LABEL(code->labels)

	    compile_bytecode_cond(expr, code, label_then, label_else, COND_THEN_IMMEDIATE);
	    SET_LABEL (code->ctx, code->labels, code->pos, label_then);
	    EXIT_IF_BYTECODE_BUFF_FULL(4);
	    code->bytecode[ code->pos++ ] = OP_PUSHONE;
	    code->bytecode[ code->pos++ ] = OP_JMP;
	    PUT_LABEL (code->ctx, code->labels, code, label_end);
	    SET_LABEL (code->ctx, code->labels, code->pos, label_else);
	    code->bytecode[ code->pos++ ] = OP_PUSHZERO;
	    SET_LABEL (code->ctx, code->labels, code->pos, label_end);
	    code->stack_level++;
	    break;
	  }
	  break;
	}
      break;

    case NODE_FIELD:
      EXIT_IF_BYTECODE_BUFF_FULL(2);
      code->bytecode[ code->pos++ ] = OP_PUSHFIELD;
      code->bytecode[ code->pos++ ] = SYM_RES_ID(expr);
      code->stack_level++;
      {
	size_t i, idx, shift;
	size_t n;

	i = SYM_RES_ID(expr);
	idx = i / (8 * sizeof(int32_t));
	shift = i % (8 * sizeof(int32_t));
	/*
	if (idx>=MAX_FIELD_SZ)
	  {
	    DebugLog (DF_OLC, DS_FATAL,
		      "Record field id too large (%li)\n", i);
	    exit (EXIT_FAILURE);
	  }
	*/
	n = idx+1;
	if (n > code->used_fields_sz)
	  {
	    code->used_fields = gc_base_realloc (code->ctx->gc_ctx, code->used_fields,
						 n*sizeof(int32_t));
	    code->used_fields_sz = n;
	  }
	code->used_fields[idx] |= (1 << shift);
      }
      // code->flags |= BYTECODE_HAS_PUSHFIELD; // XXX useless (code->flags is never used)
      break;

    case NODE_VARIABLE:
      if (SYM_RES_ID(expr)<code->logicals_size &&
	  code->logicals[SYM_RES_ID(expr)]>=0)
	{ /* logical variable: already somewhere on the stack */
	  EXIT_IF_BYTECODE_BUFF_FULL(2);
	  code->bytecode[ code->pos++ ] = OP_DUP;
	  code->bytecode[ code->pos++ ] =
	    code->stack_level - code->logicals[SYM_RES_ID(expr)];
	}
      else
	{
	  EXIT_IF_BYTECODE_BUFF_FULL(2);
	  code->bytecode[ code->pos++ ] = OP_PUSH;
	  code->bytecode[ code->pos++ ] = SYM_RES_ID(expr);
	}
      code->stack_level++;
      break;

    case NODE_CONST:
      EXIT_IF_BYTECODE_BUFF_FULL(2);
      code->bytecode[ code->pos++ ] = OP_PUSHSTATIC;
      code->bytecode[ code->pos++ ] = TERM_RES_ID(expr);
      code->stack_level++;
      break;

    case NODE_CALL:
      {
	node_expr_t *params;
	int save_stack_level;

	save_stack_level = code->stack_level;
	for (params = CALL_PARAMS(expr); params!=NULL;
	     params = BIN_RVAL(params))
	  compile_bytecode_expr(BIN_LVAL(params), code);
	/*
	  if (expr->call.paramlist)
	  if (expr->call.paramlist->params_nb)
          for (i = expr->call.paramlist->params_nb - 1; i >= 0; --i)
	  compile_bytecode_expr(gc_ctx, expr->call.paramlist->params[i], code);
	*/
	EXIT_IF_BYTECODE_BUFF_FULL(2);
	code->bytecode[ code->pos++ ] = OP_CALL;
	code->bytecode[ code->pos++ ] = CALL_RES_ID(expr);
	code->stack_level = save_stack_level+1;
	break;
      }
    case NODE_REGSPLIT:
    case NODE_IFSTMT:
    case NODE_RETURN:
    case NODE_BREAK:
      DPRINTF( ("Rule compiler expr : statement encountered (%i)\n", expr->type) );
      break;
    case NODE_DB_PATTERN: /*should not happen */
    default:
      DPRINTF( ("unknown node type (%i)\n", expr->type) );
  }
}

bytecode_t jmp_bytecode (unsigned long op)
{
  switch (op)
    {
    case OP_CEQ: return OP_CEQJMP;
    case OP_CNEQ: return OP_CNEQJMP;
    case OP_CRM: return OP_CRMJMP;
    case OP_CNRM: return OP_CNRMJMP;
    case OP_CGT: return OP_CGTJMP;
    case OP_CLT: return OP_CLTJMP;
    case OP_CGE: return OP_CGEJMP;
    case OP_CLE: return OP_CLEJMP;
    default: DebugLog(DF_OLC, DS_FATAL, "unrecognized opcode\n"); return 0;
    }
}

bytecode_t jmp_bytecode_opposite (unsigned long op)
{
  switch (op)
    {
    case OP_CEQ: return OP_CEQJMP_OPPOSITE;
    case OP_CNEQ: return OP_CNEQJMP_OPPOSITE;
    case OP_CRM: return OP_CRMJMP_OPPOSITE;
    case OP_CNRM: return OP_CNRMJMP_OPPOSITE;
    case OP_CGT: return OP_CGTJMP_OPPOSITE;
    case OP_CLT: return OP_CLTJMP_OPPOSITE;
    case OP_CGE: return OP_CGEJMP_OPPOSITE;
    case OP_CLE: return OP_CLEJMP_OPPOSITE;
    default: DebugLog(DF_OLC, DS_FATAL, "unrecognized opcode\n"); return 0;
    }
}

/**
 * Byte code compiler recursive sub-routine.
 * @param expr An expression to compile.
 * @param code An internal byte code buffer structure.
 **/
static void compile_bytecode_cond(node_expr_t *expr,
				  bytecode_buffer_t *code,
				  int label_then,
				  int label_else,
				  int flags)
{
  switch (expr->type)
    {
    case NODE_COND:
      switch (BIN_OP(expr))
	{
	case OP_CEQ:
	case OP_CNEQ:
	case OP_CRM:
	case OP_CNRM:
	case OP_CGT:
	case OP_CLT:
	case OP_CGE:
	case OP_CLE:
	  {
	    bytecode_t op;
	    
	    compile_bytecode_expr(BIN_LVAL(expr), code);
	    compile_bytecode_expr(BIN_RVAL(expr), code);
	    switch (flags)
	      {
	      case COND_THEN_IMMEDIATE:
		op = jmp_bytecode_opposite (BIN_OP(expr));
		EXIT_IF_BYTECODE_BUFF_FULL(2);
		code->bytecode[ code->pos++ ] = op;
		PUT_LABEL (code->ctx, code->labels, code, label_else);
		break;
	      case COND_ELSE_IMMEDIATE:
		op = jmp_bytecode (BIN_OP(expr));
		EXIT_IF_BYTECODE_BUFF_FULL(2);
		code->bytecode[ code->pos++ ] = op;
		PUT_LABEL (code->ctx, code->labels, code, label_then);
		break;
	      case COND_THEN_IMMEDIATE | COND_ELSE_IMMEDIATE:
		EXIT_IF_BYTECODE_BUFF_FULL(1);
		code->bytecode[ code->pos++ ] = OP_TRASH2;
		break;
	      default: /* unoptimized, vanilla case (which should not happen that often) */
		op = jmp_bytecode (BIN_OP(expr));
		EXIT_IF_BYTECODE_BUFF_FULL(4);
		code->bytecode[ code->pos++ ] = op;
		PUT_LABEL (code->ctx, code->labels, code, label_then);
		code->bytecode[ code->pos++ ] = OP_JMP;
		PUT_LABEL (code->ctx, code->labels, code, label_else);
		break;
	      }
	    code->stack_level -= 2;
	    return;
	  }
	case ANDAND:
	  {
	    unsigned int label_i = NEW_LABEL(code->labels);

	    compile_bytecode_cond(BIN_LVAL(expr), code, label_i, label_else,
				  COND_THEN_IMMEDIATE);
	    SET_LABEL (code->ctx, code->labels, code->pos, label_i);
	    compile_bytecode_cond(BIN_RVAL(expr), code, label_then, label_else, flags);
	    return;
	  }
	case OROR:
	  {
	    unsigned int label_i = NEW_LABEL(code->labels);

	    compile_bytecode_cond(BIN_LVAL(expr), code, label_then, label_i,
				  COND_ELSE_IMMEDIATE);
	    SET_LABEL (code->ctx, code->labels, code->pos, label_i);
	    compile_bytecode_cond(BIN_RVAL(expr), code, label_then, label_else, flags);
	    return;
	  }
	case BANG:
	  {
	    compile_bytecode_cond(BIN_RVAL(expr), code, label_else, label_then,
				  ((flags & COND_THEN_IMMEDIATE)?COND_ELSE_IMMEDIATE:0)
				  | ((flags & COND_ELSE_IMMEDIATE)?COND_THEN_IMMEDIATE:0));
	    return;
	  }
	default:
	  DPRINTF( ("Rule compiler cond : unknown op (%i)\n", BIN_OP(expr)) );
	  return;
	}
    case NODE_ASSOC:
    case NODE_BINOP:
    case NODE_MONOP:
    case NODE_EVENT:
    case NODE_DB_PATTERN:
    case NODE_DB_COLLECT:
    case NODE_DB_SINGLETON:
    case NODE_CALL:
    case NODE_REGSPLIT:
    case NODE_FIELD:
    case NODE_VARIABLE:
    case NODE_CONST:
      // XXX Optimization : test const value at compilation time
      compile_bytecode_expr(expr, code);
      EXIT_IF_BYTECODE_BUFF_FULL(4);
      code->bytecode[ code->pos++ ] = OP_POPCJMP;
      PUT_LABEL (code->ctx, code->labels, code, label_then);
      code->bytecode[ code->pos++ ] = OP_JMP;
      PUT_LABEL (code->ctx, code->labels, code, label_else);
      code->stack_level--;
      return;
    case NODE_IFSTMT:
    case NODE_UNKNOWN:
    case NODE_RETURN:
    case NODE_BREAK:
      DPRINTF( ("Rule compiler cond : statement encountered (%i)\n", expr->type) );
      return;
    default:
      DPRINTF( ("unknown node type (%i)\n", expr->type) );
      return ;
  }
}


/**
 * Compile a transition list node abstract syntax tree.
 * @param ctx Rule compiler context.
 * @param rule Current rule in compilation.
 * @param state Current state in compilation.
 * @param translist Transition list node abstract syntax tree root to compile.
 **/
static void compile_transitions_ast(rule_compiler_t  *ctx,
				    rule_t           *rule,
				    state_t          *state,
				    node_expr_t      *translist)
{
  size_t i;

  DebugLog(DF_OLC, DS_INFO,
           "compiling transitions in state '%s' in rule '%s'.\n",
           state->name, rule->name);

  if (translist!=NULL)
    {
      size_t n;
      node_expr_t *l;
      transition_t *trans;
      node_trans_t *node_trans;

      n = list_len (translist);
      state->trans_nb = n;
      state->trans = gc_base_malloc (ctx->gc_ctx,
				     n * sizeof (transition_t));
      for (i=0; i<n; i++)
	{
	  trans = &state->trans[i];
	  trans->dest = NULL;
	  trans->required_fields_nb = 0;
	  trans->required_fields = NULL;
	  trans->eval_code_length = 0;
	  trans->eval_code = NULL;
	  /* to prevent garbage collector bugs:
	     in case the allocations below
	     call the garbage collector, which
	     may call rule_finalize() on
	     trans, which should be
	     filled with valid pointers---or NULL. */
	}
      for (i=0, l=translist; i<n; i++, l=BIN_RVAL(l))
	{
	  node_trans = (node_trans_t *)BIN_LVAL(l);
	  rule->trans_nb++; /* update rule stats */
	  trans = &state->trans[i];
	  trans->id = i; /* set trans id */
	  trans->flags = 0;
	  if ((state->flags & STATE_EPSILON) /* not needed, since mark_no_wait_transitions()
						will set the AN_TRANS_NO_WAIT flags anyway,
						see next line */
	      || (node_trans->an_flags & AN_TRANS_NO_WAIT) /* ... and for that, we need
							      to have called mark_no_wait_transitions()
							      earlier */
	      )
	    {
	      if (ctx->verbose>=2 && !(state->flags & STATE_EPSILON))
		{
		  if (node_trans->file!=NULL)
		    fprintf (stderr, "%s:", node_trans->file);
		  fprintf (stderr, "%u: info: transition #%zi of state '%s', if triggered, will not wait, good!\n",
			   node_trans->lineno, i+1,
			   state->name?state->name:"(inner state)");
		}
	      trans->flags |= TRANS_NO_WAIT;
	    }

	  DebugLog(DF_OLC, DS_DEBUG, "transition %i: \n", i);
	  if (node_trans->cond!=NULL)
	    {
	      if (node_trans->sub_state_dest!=NULL)
		{
		  /* Conditional indirect transition */
		  DebugLog(DF_OLC, DS_TRACE, "Indirect transition (sub-states).\n");
		}
	      else
		{
		  /* Conditional direct transition (general case) */
		  node_state_t *s;

		  s = strhash_get(ctx->statenames_hash,
				  node_trans->dest);
		  if (s!=NULL)
		    {
		      DebugLog(DF_OLC, DS_TRACE,
			       "resolve dest %s %i\n",
			       s->name, s->state_id);
		      trans->dest = &rule->state[s->state_id];
		    }
		  else
		    {
		      if (node_trans->file!=NULL)
			fprintf (stderr, "%s:", node_trans->file);
		      fprintf (stderr, "%u: fatal error: undefined state '%s'.\n",
			       node_trans->lineno, node_trans->dest);
		      fflush (stderr);
		      exit (EXIT_FAILURE);
		    }
		}
	      compile_trans_bytecode(ctx, node_trans->cond, trans);
	    }
	  else
	    {
	      node_state_t *s;

	      /* Unconditional transition */
	      DebugLog(DF_OLC, DS_TRACE,
		       "Unconditional transition to [%s]\n",
		       node_trans->dest);
	      s = strhash_get(ctx->statenames_hash,
			      node_trans->dest);
	      if (s!=NULL)
		{
		  DebugLog(DF_OLC, DS_TRACE,
			   "resolve dest %s %i\n", s->name, s->state_id);
		  trans->dest = &rule->state[s->state_id];
		}
	      else
		{
		  if (node_trans->file!=NULL)
		    fprintf (stderr, "%s:", node_trans->file);
		  fprintf (stderr, "%u: fatal error: undefined state '%s'.\n",
			   node_trans->lineno, node_trans->dest);
		  fflush (stderr);
		  exit (EXIT_FAILURE);
		}
	    }
	}
    }
  else
    {
      /* Terminal state */
      state->trans_nb = 0;
      state->trans = NULL;
      DebugLog(DF_OLC, DS_TRACE,
	       "state '%s' has no transition.\n",
	       state->name?state->name:"(inner state)");
    }
}


void compiler_reset(rule_compiler_t *ctx)
{
  int32_t n;

  DebugLog(DF_OLC, DS_DEBUG,
           "*** compiler reset (clear rule specific hashes) ***\n");

  gc_clear_strhash(ctx->statenames_hash, NULL);
  gc_clear_strhash(ctx->rule_env, NULL);
  ctx->statics_nb = 0; /* nothing else to do: the garbage collector
			  will free the statics if needed now */
  for (n=ctx->dyn_var_name_nb; n!=0; )
    gc_base_free (ctx->dyn_var_name[--n]);
  ctx->dyn_var_name_nb = 0;
}


void
fprintf_rule_environment(FILE *fp, orchids_t *ctx)
{
  int i;

  fprintf(fp, "--- rule environment ---\n");
  fprintf(fp, "  static resources:\n");
  for (i = 0; i < ctx->rule_compiler->statics_nb; i++) {
    fprintf(fp, "res_id: %3i: ", i);
    fprintf_issdl_val(fp, ctx, ctx->rule_compiler->statics[i]);
  }
  fprintf(fp, "  dynamic environment size : %zu\n",
	  ctx->rule_compiler->rule_env->elmts);
}


/**
 * fprintf_expr() sub-routine : display a terminal expression node.
 * @param fp Output stream.
 * @param expr A node to display.
 **/
static void fprintf_term_expr(FILE *fp, node_expr_t *expr)
{
  size_t i, n, c;
  char *s;

  switch (expr->type)
    {
    case NODE_FIELD:
      fprintf(fp, "push field \"%s\" (fid %i)\n",
              SYM_NAME(expr), SYM_RES_ID(expr));
      break;
    case NODE_VARIABLE:
      fprintf(fp, "push variable \"%s\"\n", SYM_NAME(expr));
      break;
    case NODE_CONST:
      switch (TYPE(TERM_DATA(expr)))
        {
        case T_INT:
          fprintf(fp, "push integer %li (res_id %i)\n",
                  INT(TERM_DATA(expr)),
                  TERM_RES_ID(expr));
          break;
        case T_STR:
	  s = STR(TERM_DATA(expr)); n = STRLEN(TERM_DATA(expr));
	  goto print_string;
	case T_VSTR:
	  s = VSTR(TERM_DATA(expr)); n = VSTRLEN(TERM_DATA(expr));
	print_string:
	  fprintf (fp, "push string \"");
	  flockfile (fp);
	  for (i=0; i<n; i++)
	    {
	      c = *s++;
	      putc_unlocked (c, fp);
	    }
	  funlockfile (fp);
	  fprintf (fp, "\" (res_id %i)\n",
		   TERM_RES_ID(expr));
          break;
        default:
          fprintf(fp, "push something of type %u\n",
		  TYPE(TERM_DATA(expr)));
        }
      break;
    default:
      fprintf(fp, "unknown node type (%i)\n", expr->type);
    }
}

static void fprintf_tuple(FILE *fp, node_expr_t *expr)
{
  int i;

  for (i=1; expr!=NULL; expr=BIN_RVAL(expr), i++)
    {
      fprintf (fp, "    %d. ", i);
      fprintf_expr (fp, BIN_LVAL(expr));
    }
}

void fprintf_expr(FILE *fp, node_expr_t *expr)
{
  switch (expr->type)
    {
    case NODE_BINOP:
      fprintf_expr(fp, BIN_LVAL(expr));
      fprintf_expr(fp, BIN_RVAL(expr));
      fprintf(fp, "opcode %s (%i)\n",
	      get_opcode_name(BIN_OP(expr)),
	      BIN_OP(expr));
      break;
    case NODE_MONOP:
      fprintf_expr(fp, MON_VAL(expr));
      fprintf(fp, "opcode %s (%i)\n",
	      get_opcode_name(MON_OP(expr)),
	      MON_OP(expr));
      break;
    case NODE_CALL:
      {
	node_expr_t *params;

	for (params = CALL_PARAMS(expr);
	     params!=NULL;
	     params = BIN_RVAL(params))
	  fprintf_expr(fp, BIN_LVAL(params));
	/*
	  if (expr->call.paramlist)
	  if (expr->call.paramlist->params_nb)
          for (i = expr->call.paramlist->params_nb - 1; i >= 0; --i)
	  fprintf_expr(fp, expr->call.paramlist->params[i]);
	*/
	fprintf(fp, "call %s\n", CALL_SYM(expr));
	break;
      }
    case NODE_RETURN:
      fputs ("return ", fp);
      fprintf_expr (fp, MON_VAL(expr));
      break;
    case NODE_BREAK:
      fputs ("break\n", fp);
      break;
    case NODE_EVENT:
      if (BIN_LVAL(expr)!=NULL)
	fprintf_expr(fp, BIN_LVAL(expr));
      else fprintf (fp, "push empty event\n");
      {
	node_expr_t *l;

	for (l=BIN_RVAL(expr); l!=NULL; l=BIN_RVAL(l))
	  {
	    node_expr_t *add_event_expr = BIN_LVAL(l);
	    node_expr_t *field_name = BIN_LVAL(add_event_expr);
	    node_expr_t *field_value = BIN_RVAL(add_event_expr);

	    fprintf_expr (fp, field_value);
	    fprintf(fp, "add field \"%s\" (fid %i) to event\n",
		    SYM_NAME(field_name), SYM_RES_ID(field_name));
	  }
      }
      break;
    case NODE_DB_COLLECT:
      {
	node_expr_t *l;
	node_expr_t *pat;

	expr = BIN_RVAL(expr); /* skip 'returns' part */
	for (l=BIN_RVAL(expr); l!=NULL; l=BIN_RVAL(l))
	  {
	    fputs ("for [", fp);
	    pat = BIN_LVAL(l);
	    fprintf_tuple (fp, BIN_RVAL(pat));
	    fputs ("    ] in ", fp);
	    fprintf_expr (fp, BIN_LVAL(pat));
	  }
	fputs ("collect ", fp);
	fprintf_expr (fp, BIN_LVAL(BIN_LVAL(expr)));
	fprintf_expr (fp, BIN_RVAL(BIN_LVAL(expr)));
      }
      break;
    case NODE_DB_SINGLETON:
      fputs ("db_singleton ", fp);
      fprintf_tuple (fp, MON_VAL(expr));
      break;
    case NODE_CONS:
      {
	node_expr_t *l;

	for (l=expr; l!=NULL; l=BIN_RVAL(l))
	  fprintf_expr (fp, BIN_LVAL(l));
      }
      break;
    case NODE_IFSTMT:
      fputs ("if: ", fp);
      fprintf_expr (fp, IF_COND (expr));
      fputs ("then: ", fp);
      fprintf_expr (fp, IF_THEN (expr));
      fputs ("else: ", fp);
      fprintf_expr (fp, IF_ELSE (expr));
      break;
    case NODE_ASSOC:
      fprintf (fp, "%s = ", SYM_NAME(BIN_LVAL(expr)));
      fprintf_expr (fp, BIN_RVAL(expr));
      break;
    case NODE_REGSPLIT:
      {
	size_t i, n;
	char *delim;

	fprintf (fp, "split %s ", REGEXSTR(REGSPLIT_PAT(expr)));
	if (REGSPLIT_DEST_VARS(expr)!=NULL)
	  {
	    delim = "/";
	    n = REGSPLIT_DEST_VARS(expr)->vars_nb;
	    for (i=0; i<n; i++)
	      {
		fputs (delim, fp);
		delim = ",";
		fputs (SYM_NAME(REGSPLIT_DEST_VARS(expr)->vars[i]), fp);
	      }
	    fputs ("/ ", fp);
	  }
	fprintf_expr (fp, REGSPLIT_STRING(expr));
	break;
      }
    default:
      fprintf_term_expr(fp, expr);
      break;
    }
}


static void fprintf_term_expr_infix(FILE *fp, node_expr_t *expr)
{
  switch (expr->type)
    {
    case NODE_FIELD:
      fprintf(fp, ".%s ", SYM_NAME(expr));
      break;
    case NODE_VARIABLE:
      fprintf(fp, "$%s ", SYM_NAME(expr));
      break;
    case NODE_CONST:
      switch (TYPE(TERM_DATA(expr)))
        {
        case T_INT:
          fprintf(fp, "%li ", INT(TERM_DATA(expr)));
          break;
        case T_STR:
          fprintf(fp, "\"%s\" ",STR(TERM_DATA(expr)));
          break;
        default:
          fprintf(fp, "data type %u\n", TYPE(TERM_DATA(expr)));
        }
      break;
    case NODE_CALL:
      fprintf(fp, "%s() ", CALL_SYM(expr));
      break;
    case NODE_RETURN:
      fprintf (fp, "return");
      break;
    case NODE_BREAK:
      fprintf (fp, "break");
      break;
    default:
      fprintf(fp, "unknown node type (%i)\n", expr->type);
    }
}


void fprintf_expr_infix(FILE *fp, node_expr_t *expr)
{
  if (expr->type == NODE_BINOP)
    {
      fprintf(fp, "( ");
      fprintf_expr_infix(fp, BIN_LVAL(expr));
      switch (BIN_OP(expr))
	{
	case OP_CEQ:
	  fprintf(fp, "== ");
	  break;
	case OP_CGT:
	  fprintf(fp, "> ");
	  break;
	case OP_CLE:
	  fprintf(fp, "<= ");
	  break;
	case ANDAND:
	  fprintf(fp, "&& ");
	  break;
	case OROR:
	  fprintf(fp, "|| ");
	  break;
	case BANG:
	  fprintf(fp, "! ");
	  break;
	}
      fprintf_expr_infix(fp, BIN_RVAL(expr));
      fprintf(fp, ") ");
    }
  else
    fprintf_term_expr_infix(fp, expr);
}


void
fprintf_rule_stats(const orchids_t *ctx, FILE *fp)
{
  int rn;
  rule_t *r;

  fprintf(fp, "------------------------------------[ rules ]-----------------------------------\n");
  fprintf(fp, " rid |    name      |state|trans|sta e|dyn e|activ| source\n");
  fprintf(fp, "-----+--------------+-----+-----+-----+-----+-----+-----------------------------\n");

  for (r = ctx->rule_compiler->first_rule, rn = 0; r; r = r->next, rn++)
    {
      fprintf(fp, " %3i | %12.12s | %3zi | %3zi | %3zu | %3i | %3zi | %.24s:%i\n",
              rn, r->name, r->state_nb, r->trans_nb,
              r->static_env_sz, r->dynamic_env_sz,
              r->instances, r->filename, r->lineno);
    }
  fprintf(fp, "-----+--------------+-----+-----+-----+-----+-----+-----------------------------\n");
}



/*
** Copyright (c) 2002-2005 by Julien OLIVAIN, Laboratoire Spécification
** et Vérification (LSV), CNRS UMR 8643 & ENS Cachan.
**
** Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
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
