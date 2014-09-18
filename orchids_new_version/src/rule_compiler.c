/**
 ** @file rule_compiler.c
 ** High-level functions for the yaccer for abstract syntax tree building
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
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
#include <stdio.h>
#include <string.h>
#include <time.h>

/* for inet_addr() */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "orchids.h"
#include "lang.h"
#include "rule_compiler.h"
#include "issdl.tab.h"
#include "ovm.h"

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

static void
compile_and_add_rulefile(orchids_t *ctx, char *rulefile);

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

static void compile_bytecode_cond(node_expr_t *expr,
				  bytecode_buffer_t *code,
				  int	label_then,
				  int	label_else);


static void statics_add(rule_compiler_t *ctx, ovm_var_t *data);

static void build_fields_hash(orchids_t *ctx);

static void build_functions_hash(orchids_t *ctx);

static void
fprintf_term_expr(FILE *fp, node_expr_t *expr);

static int rule_compiler_mark_hash_walk_func (void *elt, void *data)
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
}

static void rule_compiler_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  rule_compiler_t *ctx = (rule_compiler_t *)p;

  gc_base_free (ctx->protected);
  issdllex_destroy (ctx->scanner);
  if (ctx->issdlin!=NULL)
    {
      int ret;

#ifdef ENABLE_PREPROC
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

static int rule_compiler_traverse_hash_walk_func (void *elt, void *data)
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
  fields_hash     = new_strhash(gc_ctx, 1021);
  rulenames_hash  = new_strhash(gc_ctx, 251);
  statenames_hash = new_strhash(gc_ctx, 251);
  rule_env        = new_strhash(gc_ctx, 251);
  protected = gc_base_malloc (gc_ctx, NPROTECTED * sizeof(gc_header_t *));

  GC_START (gc_ctx, 1);
  ctx = gc_alloc (gc_ctx, sizeof (rule_compiler_t), &rule_compiler_class);
  ctx->gc.type = T_NULL;
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
}

static void node_state_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  char *name = ((node_state_t *)p)->name;

  if (name!=NULL)
    gc_base_free (name);
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
  return err;
}

static gc_class_t node_state_class = {
  GC_ID('n','s','t','e'),
  node_state_mark_subfields,
  node_state_finalize,
  node_state_traverse
};

static void node_trans_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  node_trans_t *n = (node_trans_t *)p;

  GC_TOUCH (gc_ctx, n->cond);
  GC_TOUCH (gc_ctx, n->sub_state_dest);
}

static void node_trans_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  char *name = ((node_trans_t *)p)->dest;

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

static gc_class_t node_trans_class = {
  GC_ID('n','t','r','n'),
  node_trans_mark_subfields,
  node_trans_finalize,
  node_trans_traverse
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

  if (n->file!=NULL)
    gc_base_free (n->file);
  if (n->name!=NULL)
    gc_base_free (n->name);
  sv = n->sync_vars;
  if (sv!=NULL)
    {
      size_t i, n;

      for (i=0, n=sv->vars_nb; i<n; i++)
	if (sv->vars[i]!=NULL)
	  gc_base_free (sv->vars[i]);
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

static gc_class_t node_rule_class = {
  GC_ID('n','r','u','l'),
  node_rule_mark_subfields,
  node_rule_finalize,
  node_rule_traverse
};

static void node_expr_term_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  node_expr_term_t *n = (node_expr_term_t *)p;

  GC_TOUCH (gc_ctx, n->data);
}

static void node_expr_term_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  return;
}

static int node_expr_term_traverse (gc_traverse_ctx_t *gtc, gc_header_t *p,
				    void *data)
{
  node_expr_term_t *n = (node_expr_term_t *)p;

  return (*gtc->do_subfield) (gtc, (gc_header_t *)n->data, data);
}

static gc_class_t node_expr_term_class = {
  GC_ID('n','t','r','m'),
  node_expr_term_mark_subfields,
  node_expr_term_finalize,
  node_expr_term_traverse
};


static void node_expr_sym_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  return;
}

static void node_expr_sym_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  gc_base_free (SYM_NAME(p));
}

static int node_expr_sym_traverse (gc_traverse_ctx_t *gtc, gc_header_t *p,
				   void *data)
{
  return 0;
}

static gc_class_t node_expr_sym_class = {
  GC_ID('n','s','y','m'),
  node_expr_sym_mark_subfields,
  node_expr_sym_finalize,
  node_expr_sym_traverse
};

static void node_expr_bin_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  node_expr_bin_t *n = (node_expr_bin_t *)p;

  GC_TOUCH (gc_ctx, n->lval);
  GC_TOUCH (gc_ctx, n->rval);
}

static void node_expr_bin_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  return;
}

static int node_expr_bin_traverse (gc_traverse_ctx_t *gtc, gc_header_t *p,
				   void *data)
{
  node_expr_bin_t *n = (node_expr_bin_t *)p;
  int err;

  err = (*gtc->do_subfield) (gtc, (gc_header_t *)n->lval, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *)n->rval, data);
  return err;
}

static gc_class_t node_expr_bin_class = {
  GC_ID('n','b','i','n'),
  node_expr_bin_mark_subfields,
  node_expr_bin_finalize,
  node_expr_bin_traverse
};

static void node_expr_mon_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  node_expr_mon_t *n = (node_expr_mon_t *)p;

  GC_TOUCH (gc_ctx, n->val);
}

static void node_expr_mon_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  return;
}

static int node_expr_mon_traverse (gc_traverse_ctx_t *gtc, gc_header_t *p,
				   void *data)
{
  node_expr_mon_t *n = (node_expr_mon_t *)p;
  int err;

  err = (*gtc->do_subfield) (gtc, (gc_header_t *)n->val, data);
  return err;
}

static gc_class_t node_expr_mon_class = {
  GC_ID('n','m','o','n'),
  node_expr_mon_mark_subfields,
  node_expr_mon_finalize,
  node_expr_mon_traverse
};

static void node_expr_call_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  node_expr_call_t *n = (node_expr_call_t *)p;

  GC_TOUCH (gc_ctx, n->paramlist);
}

static void node_expr_call_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  gc_base_free (CALL_SYM(p));
}

static int node_expr_call_traverse (gc_traverse_ctx_t *gtc, gc_header_t *p,
				    void *data)
{
  node_expr_call_t *n = (node_expr_call_t *)p;

  return (*gtc->do_subfield) (gtc, (gc_header_t *)n->paramlist, data);
}

static gc_class_t node_expr_call_class = {
  GC_ID('n','c','l','l'),
  node_expr_call_mark_subfields,
  node_expr_call_finalize,
  node_expr_call_traverse
};

static void node_expr_ifstmt_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  node_expr_if_t *n = (node_expr_if_t *)p;

  GC_TOUCH (gc_ctx, n->cond);
  GC_TOUCH (gc_ctx, n->then);
  GC_TOUCH (gc_ctx, n->els);
}

static void node_expr_ifstmt_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  return;
}

static int node_expr_ifstmt_traverse (gc_traverse_ctx_t *gtc,
				      gc_header_t *p,
				      void *data)
{
  node_expr_if_t *n = (node_expr_if_t *)p;
  int err;

  err = (*gtc->do_subfield) (gtc, (gc_header_t *)n->cond, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *)n->then, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *)n->els, data);
  return err;
}

static gc_class_t node_expr_ifstmt_class = {
  GC_ID('n','i','f','s'),
  node_expr_ifstmt_mark_subfields,
  node_expr_ifstmt_finalize,
  node_expr_ifstmt_traverse
};

static void node_expr_regsplit_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  node_expr_regsplit_t *rs = (node_expr_regsplit_t *)p;

  GC_TOUCH (gc_ctx, rs->string);
  GC_TOUCH (gc_ctx, rs->split_pat);
}

static void node_expr_regsplit_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  node_expr_regsplit_t *rs = (node_expr_regsplit_t *)p;
  node_varlist_t *dv = rs->dest_vars;
  size_t i, n;

  for (i=0, n=dv->vars_nb; i<n; i++)
    gc_base_free (dv->vars[i]);
  gc_base_free (dv->vars);
  gc_base_free (dv);
}

static int node_expr_regsplit_traverse (gc_traverse_ctx_t *gtc,
					gc_header_t *p, void *data)
{
  node_expr_regsplit_t *rs = (node_expr_regsplit_t *)p;
  subfield_action do_subfield = gtc->do_subfield;
  int err;

  err = (*do_subfield) (gtc, (gc_header_t *)rs->string, data);
  if (err)
    return err;
  err = (*do_subfield) (gtc, (gc_header_t *)rs->split_pat, data);
  return err;
}

static gc_class_t node_expr_regsplit_class = {
  GC_ID('r','g','s','p'),
  node_expr_regsplit_mark_subfields,
  node_expr_regsplit_finalize,
  node_expr_regsplit_traverse
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


void
compile_rules(orchids_t *ctx)
{
  rulefile_t *rulefile;

  DebugLog(DF_OLC, DS_NOTICE, "*** beginning rule compilation ***\n");

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

/**
 * Lex/yacc parser entry point.
 * @param ctx Orchids context.
 * @param rulefile File to parse.
 **/
static void
compile_and_add_rulefile(orchids_t *ctx, char *rulefile)
{
  int ret;
  FILE *issdlin;
#ifdef ENABLE_PREPROC
  const char *ppcmd;
  char cmd[4096];
#endif /* ENABLE_PREPROC */

  DebugLog(DF_OLC, DS_NOTICE, "Compiling rule file '%s'\n", rulefile);

  /* set some compiler context values */
  ctx->rule_compiler->currfile = rulefile;
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
    DebugLog(DF_OLC, DS_FATAL,
             "Error while compiling rule file '%s'\n", rulefile);
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


char *
build_substate_name(const char *refname, int n)
{
  char *subname;
  char num[12]; /* maxlen: -2^31 = -2147483648 (11 chars + '\0') */

  snprintf(num, 12, "_%i", n);
  subname = Xmalloc( strlen(refname) + strlen(num) );
  strcat(subname, refname);
  strcat(subname, num);

  return (subname);
}


/* Abstract syntax tree building functions (called by the yaccer issdl.y) */

node_state_t *build_state(rule_compiler_t *ctx,
			  node_expr_t *actions,
			  node_expr_t *transitions)
{
  node_state_t *s;

  s = gc_alloc (ctx->gc_ctx, sizeof(node_state_t),
		&node_state_class);
  s->gc.type = T_NULL;
  s->state_id = 0; /* not set */
  s->line = 0; /* not set: will be set by set_state_label() */
  s->name = NULL; /* not set: will be set by set_state_label() */
  s->flags = 0; /* not set: will be set by set_state_label() */
  GC_TOUCH (ctx->gc_ctx, s->actionlist = actions);
  GC_TOUCH (ctx->gc_ctx, s->translist = transitions);
  return s;
}


node_state_t *set_state_label(rule_compiler_t *ctx,
			      node_state_t *state,
			      symbol_token_t *sym,
			      unsigned long flags)
{
  if (sym->file!=NULL)
    gc_base_free (sym->file);
  if (state == NULL)
    {
      gc_base_free (sym->name);
      return NULL;
    }
  state->name = sym->name;
  state->line = sym->line;
  state->flags = flags;
  /* add state name in current compiler context */
  if (strhash_get(ctx->statenames_hash, sym->name)) {
    DebugLog(DF_OLC, DS_FATAL,
             "state %s already defined.\n", sym->name);
    exit(EXIT_FAILURE);
  }
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

node_trans_t *build_direct_transition(rule_compiler_t *ctx,
				      node_expr_t *cond, char *dest)
{
  node_trans_t *trans;

  trans = gc_alloc (ctx->gc_ctx, sizeof(node_trans_t),
		    &node_trans_class);
  trans->gc.type = T_NULL;
  trans->type = -1;
  GC_TOUCH (ctx->gc_ctx, trans->cond = cond);
  trans->dest = dest;
  trans->sub_state_dest = NULL; /* not set */
  return trans;
}


node_trans_t *build_indirect_transition(rule_compiler_t *ctx,
					node_expr_t *cond,
					node_state_t *substate)
{
  node_trans_t *trans;

  trans = gc_alloc (ctx->gc_ctx, sizeof(node_trans_t),
		    &node_trans_class);
  trans->gc.type = T_NULL;
  trans->type = -1;
  GC_TOUCH (ctx->gc_ctx, trans->cond = cond);
  trans->dest = NULL; /* not set */
  GC_TOUCH (ctx->gc_ctx, trans->sub_state_dest = substate);
  return trans;
}


node_rule_t *build_rule(rule_compiler_t *ctx,
			symbol_token_t   *sym,
			node_state_t     *init_state,
			node_expr_t *states,
			node_syncvarlist_t   *sync_vars)
{
  node_rule_t *new_rule;

  new_rule = gc_alloc (ctx->gc_ctx, sizeof(node_rule_t),
		       &node_rule_class);
  new_rule->gc.type = T_NULL;
  new_rule->name = sym->name;
  new_rule->file = sym->file;
  new_rule->line = sym->line;
  GC_TOUCH (ctx->gc_ctx, new_rule->init = init_state);
  GC_TOUCH (ctx->gc_ctx, new_rule->statelist = states);
  new_rule->sync_vars = sync_vars;
  return new_rule;
}

/*
** expression building functions...
*/

node_expr_t *build_string_split(rule_compiler_t *ctx,
				node_expr_t *source,
				node_expr_t *pattern,
				node_varlist_t *dest_list)
{
  node_expr_regsplit_t *n;

  n = gc_alloc (ctx->gc_ctx, sizeof (node_expr_t),
		&node_expr_regsplit_class);
  n->gc.type = T_NULL;
  n->type = NODE_REGSPLIT;
  GC_TOUCH (ctx->gc_ctx, n->string = source);
  GC_TOUCH (ctx->gc_ctx, n->split_pat = pattern);
  n->dest_vars = dest_list;

  DPRINTF( ("splits = %i\n", dest_list->vars_nb) );

  REGEXNUM(TERM_DATA(pattern)) = dest_list->vars_nb;
  return (node_expr_t *)n;
}

node_expr_t *build_expr_binop(rule_compiler_t *ctx, int op,
			      node_expr_t *left_node,
			      node_expr_t *right_node)
{
  node_expr_bin_t *n;

  n = gc_alloc (ctx->gc_ctx, sizeof(node_expr_bin_t),
		&node_expr_bin_class);
  n->gc.type = T_NULL;
  n->type = NODE_BINOP;
  n->op = op;
  GC_TOUCH (ctx->gc_ctx, n->lval = left_node);
  GC_TOUCH (ctx->gc_ctx, n->rval = right_node);
  return (node_expr_t *)n;
}

node_expr_t *build_expr_monop(rule_compiler_t *ctx, int op,
			      node_expr_t *arg_node)
{
  node_expr_mon_t *n;

  n = gc_alloc (ctx->gc_ctx, sizeof(node_expr_mon_t),
		&node_expr_mon_class);
  n->gc.type = T_NULL;
  n->type = NODE_MONOP;
  n->op = op;
  GC_TOUCH (ctx->gc_ctx, n->val = arg_node);
  return (node_expr_t *)n;
}


node_expr_t *build_expr_cons(rule_compiler_t *ctx,
			     node_expr_t *left_node,
			     node_expr_t *right_node)
{
  node_expr_bin_t *n;

  n = gc_alloc (ctx->gc_ctx, sizeof(node_expr_bin_t),
		&node_expr_bin_class);
  n->gc.type = T_NULL;
  n->type = NODE_CONS;
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


node_expr_t *build_expr_cond(rule_compiler_t *ctx,
			     int op, node_expr_t *left_node,
			     node_expr_t *right_node)
{
  node_expr_bin_t *n;

  n = gc_alloc (ctx->gc_ctx, sizeof(node_expr_bin_t),
		&node_expr_bin_class);
  n->gc.type = T_NULL;
  n->type = NODE_COND;
  n->op = op;
  GC_TOUCH (ctx->gc_ctx, n->lval = left_node);
  GC_TOUCH (ctx->gc_ctx, n->rval = right_node);
  return (node_expr_t *)n;
}

node_expr_t *build_assoc(rule_compiler_t *ctx, char *varname, node_expr_t *expr)
{
  node_expr_bin_t *n;
  node_expr_t *var;

  GC_START(ctx->gc_ctx, 1);
  var = build_varname(ctx, varname);
  GC_UPDATE(ctx->gc_ctx, 0, var);
  n = (node_expr_bin_t *)gc_alloc (ctx->gc_ctx, sizeof(node_expr_bin_t),
				   &node_expr_bin_class);
  n->gc.type = T_NULL;
  n->type = NODE_ASSOC;
  n->op = EQ;
  GC_TOUCH (ctx->gc_ctx, n->lval = var);
  GC_TOUCH (ctx->gc_ctx, n->rval = expr);
  GC_END(ctx->gc_ctx);
  return (node_expr_t *)n;
}

node_expr_t *build_function_call(rule_compiler_t  *ctx,
				 symbol_token_t   *sym,
				 node_expr_t *paramlist)
{
  node_expr_call_t *call_node;
  issdl_function_t *func;

gc_check(ctx->gc_ctx); 
  if (sym->file!=NULL)
    gc_base_free (sym->file);
  func = strhash_get(ctx->functions_hash, sym->name);
  if (func == NULL) {
    DebugLog(DF_OLC, DS_FATAL,
             "unresolved symbol for function '%s'\n", sym->name);
    exit(EXIT_FAILURE);
  }

gc_check(ctx->gc_ctx); 

  call_node = (node_expr_call_t *)gc_alloc (ctx->gc_ctx,
					    sizeof(node_expr_call_t),
					    &node_expr_call_class);
  call_node->gc.type = T_NULL;
  call_node->paramlist = NULL;
gc_check(ctx->gc_ctx); 
  call_node->type = NODE_CALL;
  call_node->symbol = sym->name;
  GC_TOUCH (ctx->gc_ctx, call_node->paramlist = paramlist);
  call_node->res_id = func->id;
gc_check(ctx->gc_ctx); 
  return (node_expr_t *)call_node;
}

node_expr_t *build_expr_ifstmt(rule_compiler_t *ctx,
			       node_expr_t *cond, node_expr_t *then,
			       node_expr_t *els)
{
  node_expr_if_t *i;

  i = gc_alloc (ctx->gc_ctx, sizeof(node_expr_if_t),
		&node_expr_ifstmt_class);
  i->gc.type = T_NULL;
  i->type = NODE_IFSTMT;
  GC_TOUCH (ctx->gc_ctx, i->cond = cond);
  GC_TOUCH (ctx->gc_ctx, i->then = then);
  GC_TOUCH (ctx->gc_ctx, i->els = els);
  return (node_expr_t *)i;
}

node_expr_t *build_fieldname(rule_compiler_t *ctx, char *fieldname)
{
  field_record_t *f;
  node_expr_symbol_t *n;

  /* check if field exists */
  /* XXX: resolution can be in the descendant phase (2) */
  f = strhash_get(ctx->fields_hash, fieldname);
  if (f == NULL) {
    DebugLog(DF_OLC, DS_FATAL, "field %s not registered\n", fieldname);
    exit(EXIT_FAILURE);
  }
  n = (node_expr_symbol_t *)gc_alloc (ctx->gc_ctx, sizeof(node_expr_symbol_t),
				      &node_expr_sym_class);
  n->gc.type = T_NULL;
  n->type = NODE_FIELD;
  n->name = fieldname;
  n->res_id = f->id;
  return (node_expr_t *)n;
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
				      &node_expr_sym_class);
  n->gc.type = T_NULL;
  n->type = NODE_VARIABLE;
  n->name = varname;
  n->res_id = ctx->rule_env->elmts;
  GC_START(ctx->gc_ctx, 1);
  GC_UPDATE(ctx->gc_ctx, 0, n);
  gc_strhash_add(ctx->gc_ctx, ctx->rule_env, (gc_header_t *)n,
		 gc_strdup (ctx->gc_ctx, varname));
  dynamic_add(ctx, varname);
  GC_END(ctx->gc_ctx);
  return (node_expr_t *)n;
}

node_expr_t *build_integer(rule_compiler_t *ctx, int i)
{
  ovm_var_t    *integer;
  node_expr_term_t  *n;

  GC_START(ctx->gc_ctx, 1);
  integer = ovm_int_new(ctx->gc_ctx, i);
  GC_UPDATE(ctx->gc_ctx, 0, integer);

  /* XXX Add hash for constant sharing here */

  n = (node_expr_term_t *) gc_alloc (ctx->gc_ctx, sizeof(node_expr_term_t),
				     &node_expr_term_class);
  n->gc.type = T_NULL;
  n->type = NODE_CONST;
  GC_TOUCH (ctx->gc_ctx, n->data = integer);
  n->res_id = ctx->statics_nb;
  GC_UPDATE(ctx->gc_ctx, 0, n);

  statics_add(ctx, integer);
  GC_END(ctx->gc_ctx);
  return (node_expr_t *)n;
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
  n->gc.type = T_NULL;
  n->type = NODE_CONST;
  GC_TOUCH (ctx->gc_ctx, n->data = fpdouble);
  n->res_id = ctx->statics_nb;
  GC_UPDATE(ctx->gc_ctx, 0, n);

  statics_add(ctx, fpdouble);
  GC_END(ctx->gc_ctx);
  return (node_expr_t *)n;
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
  n->gc.type = T_NULL;
  n->type = NODE_CONST;
  GC_TOUCH(ctx->gc_ctx, n->data = string);
  n->res_id = ctx->statics_nb;
  GC_UPDATE(ctx->gc_ctx, 0, n);

  statics_add(ctx, string);
  GC_END(ctx->gc_ctx);
  return (node_expr_t *)n;
}

node_expr_t *build_expr_event(rule_compiler_t *ctx, int op,
			      node_expr_t *left_node,
			      node_expr_t *right_node)
{
  node_expr_bin_t *n;

  n = gc_alloc (ctx->gc_ctx, sizeof(node_expr_bin_t),
		&node_expr_bin_class);
  n->gc.type = T_NULL;
  n->type = NODE_EVENT;
  n->op = op;
  GC_TOUCH (ctx->gc_ctx, n->lval = left_node);
  GC_TOUCH (ctx->gc_ctx, n->rval = right_node);
  return (node_expr_t *)n;
}

node_expr_t *build_ipv4(rule_compiler_t *ctx, char *hostname)
{
  ovm_var_t *addr;
  node_expr_term_t *n;
  struct hostent  *hptr;

  /* XXX Add hash for constant sharing here */

  GC_START(ctx->gc_ctx, 1);
  addr = ovm_ipv4_new(ctx->gc_ctx);
  hptr = gethostbyname(hostname);
  if (hptr == NULL) {
    DebugLog(DF_OLC, DS_FATAL,
             "hostname %s doesn't exist\n");
    exit(EXIT_FAILURE);
  }
  /* resolve host name */
  IPV4(addr).s_addr = *(in_addr_t *)(hptr->h_addr_list[0]);
  GC_UPDATE(ctx->gc_ctx, 0, addr);

  gc_base_free(hostname); /* free string memory allocated by the lexer */

  n = (node_expr_term_t *) gc_alloc (ctx->gc_ctx, sizeof(node_expr_term_t),
				     &node_expr_term_class);
  n->gc.type = T_NULL;
  n->type = NODE_CONST;
  GC_TOUCH (ctx->gc_ctx, n->data = addr);
  n->res_id = ctx->statics_nb;
  GC_UPDATE(ctx->gc_ctx, 0, n);

  statics_add(ctx, addr);
  GC_END(ctx->gc_ctx);
  return (node_expr_t *)n;
}

node_expr_t *build_ipv6(rule_compiler_t *ctx, char *hostname)
{
  ovm_var_t *addr;
  node_expr_term_t *n;
  struct hostent  *hptr;

  /* XXX Add hash for constant sharing here */

  GC_START(ctx->gc_ctx, 1);
  addr = ovm_ipv6_new(ctx->gc_ctx);
  hptr = gethostbyname(hostname);
  if (hptr == NULL) {
    DebugLog(DF_OLC, DS_FATAL,
             "hostname %s doesn't exist\n");
    exit(EXIT_FAILURE);
  }
  /* resolve host name */
  IPV6(addr) = *(struct in6_addr *)(hptr->h_addr_list[0]);
  GC_UPDATE(ctx->gc_ctx, 0, addr);

  gc_base_free(hostname); /* free string memory allocated by the lexer */

  n = (node_expr_term_t *) gc_alloc (ctx->gc_ctx, sizeof(node_expr_term_t),
				     &node_expr_term_class);
  n->gc.type = T_NULL;
  n->type = NODE_CONST;
  GC_TOUCH (ctx->gc_ctx, n->data = addr);
  n->res_id = ctx->statics_nb;
  GC_UPDATE(ctx->gc_ctx, 0, n);

  statics_add(ctx, addr);
  GC_END(ctx->gc_ctx);
  return (node_expr_t *)n;
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
  n->gc.type = T_NULL;
  n->type = NODE_CONST;
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

  if (orchids_getdate (date, &tm)) {
    DebugLog(DF_OLC, DS_FATAL,
             "Cannot parse date\n");
    exit(EXIT_FAILURE);
  }
  gc_base_free (date); /* free string allocated by lexer */
  GC_START(ctx->gc_ctx, 1);
  time = ovm_ctime_new(ctx->gc_ctx, mktime (&tm));
  GC_UPDATE(ctx->gc_ctx, 0, time);

  n = (node_expr_term_t *) gc_alloc (ctx->gc_ctx, sizeof(node_expr_term_t),
				     &node_expr_term_class);
  n->gc.type = T_NULL;
  n->type = NODE_CONST;
  GC_TOUCH (ctx->gc_ctx, n->data = time);
  n->res_id = ctx->statics_nb;
  GC_UPDATE(ctx->gc_ctx, 0, n);

  statics_add(ctx, time);
  GC_END(ctx->gc_ctx);
  return (node_expr_t *)n;
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
  n->gc.type = T_NULL;
  n->type = NODE_CONST;
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

  if (orchids_getdate (date, &tm)) {
    DebugLog(DF_OLC, DS_FATAL,
             "Cannot parse date\n");
    exit(EXIT_FAILURE);
  }
  gc_base_free (date); /* free string allocated by lexer */
  GC_START(ctx->gc_ctx, 1);
  timeval = ovm_timeval_new(ctx->gc_ctx);
  TIMEVAL(timeval).tv_sec  = mktime (&tm);
  TIMEVAL(timeval).tv_usec = usec;
  GC_UPDATE(ctx->gc_ctx, 0, timeval);

  n = (node_expr_term_t *) gc_alloc (ctx->gc_ctx, sizeof(node_expr_term_t),
				     &node_expr_term_class);
  n->gc.type = T_NULL;
  n->type = NODE_CONST;
  GC_TOUCH (ctx->gc_ctx, n->data = timeval);
  n->res_id = ctx->statics_nb;
  GC_UPDATE(ctx->gc_ctx, 0, n);

  statics_add(ctx, timeval);
  GC_END(ctx->gc_ctx);
  return (node_expr_t *)n;
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

      DPRINTF( ("REGEX compilation error (%s)\n", regex_str) );
      regerror(ret, &REGEX(regex), err_buf, sizeof (err_buf));
      exit(EXIT_FAILURE);
    }

  n = (node_expr_term_t *) gc_alloc (ctx->gc_ctx, sizeof(node_expr_term_t),
				     &node_expr_term_class);
  n->gc.type = T_NULL;
  n->type = NODE_CONST;
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
    DebugLog(DF_OLC, DS_ERROR, "REGEX compilation error (%s)\n", regex_str);
    exit(EXIT_FAILURE);
  }

  n = (node_expr_term_t *) gc_alloc (ctx->gc_ctx, sizeof(node_expr_term_t),
				     &node_expr_term_class);
  n->gc.type = T_NULL;
  n->type = NODE_CONST;
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
    strhash_add(h, &ctx->global_fields->fields[f],
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
    strhash_add(h, &func_tbl[f], func_tbl[f].name);
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
  int i;
  int sync_var_sz;
  int sync_var;
  ovm_var_t *var;

  h = 0;
  si = state_inst;
  sync_var_sz = si->rule_instance->rule->sync_vars_sz;

  for (i = 0; i < sync_var_sz; i++) {
    sync_var = si->rule_instance->rule->sync_vars[i];
    var = ovm_read_value (si->env, sync_var);

    h = datahash_pjw(h, &sync_var, sizeof (sync_var));
    h = datahash_pjw(h, &var->gc.type, sizeof (var->gc.type));
    h = datahash_pjw(h, issdl_get_data(var), issdl_get_data_len(var));
  }

  DebugLog(DF_ENG, DS_INFO, "Hashed state instance %p hcode=0x%08lx\n",
           state_inst, h);

  return (h);
}


static int
objhash_rule_instance_cmp(void *state_inst1, void *state_inst2)
{
  state_instance_t *si1;
  state_instance_t *si2;
  int sync_var_sz;
  int sync_var;
  ovm_var_t *var1;
  ovm_var_t *var2;
  int i;
  int ret;

  si1 = state_inst1;
  si2 = state_inst2;
  /* assert: si1->rule_instance->rule == si2->rule_instance->rule */

  sync_var_sz = si1->rule_instance->rule->sync_vars_sz;

  /* for each sync vars, compare type and value */
  for (i = 0; i < sync_var_sz; i++) {
    sync_var = si1->rule_instance->rule->sync_vars[i];

    var1 = ovm_read_value (si1->env, sync_var);
    var2 = ovm_read_value (si2->env, sync_var);

    /* call comparison functions */
    ret = issdl_cmp(var1, var2);
    if (ret) {
      DebugLog(DF_ENG, DS_INFO, "si1=%p %c si2=%p\n",
               si1, ret > 0 ? '>' : '<', si2);
      return ret;
    }
  }

  /* If we are here, the two state instances are synchronized */
  DebugLog(DF_ENG, DS_INFO, "si1=%p == si2=%p\n", si1, si2);

  return 0;
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

static void rule_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  rule_t *rule = (rule_t *)p;
  state_t *state;
  transition_t *trans;
  int32_t i, m, j, n;

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
		{
		  trans = &state->trans[j];
		  if (trans->required_fields!=NULL)
		    gc_base_free (trans->required_fields);
		  if (trans->eval_code!=NULL)
		    gc_base_free (trans->eval_code);
		}
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

static gc_class_t rule_class = {
  GC_ID('r','u','l','e'),
  rule_mark_subfields,
  rule_finalize,
  rule_traverse
};

void compile_and_add_rule_ast(rule_compiler_t *ctx, node_rule_t *node_rule)
{
  rule_t *rule;
  size_t  s, t, m, n;
  int32_t tid;
  struct stat filestat;
  node_expr_t *sync_var;
  node_expr_t *l;

  DebugLog(DF_OLC, DS_INFO,
           "----- compiling rule \"%s\" (from file %s:%i) -----\n",
           node_rule->name, ctx->currfile, node_rule->line);

  /* Check if we don't already have this rule definition */
  if ((rule = strhash_get(ctx->rulenames_hash, node_rule->name))) {
    DebugLog(DF_OLC, DS_FATAL,
             "rule %s already defined in %s:%i\n",
             node_rule->name, rule->filename, rule->lineno);
    exit(EXIT_FAILURE);
  }

  Xstat(node_rule->file, &filestat);

  GC_START(ctx->gc_ctx, 1);
  rule = gc_alloc (ctx->gc_ctx, sizeof (rule_t), &rule_class);
  rule->gc.type = T_NULL;
  rule->filename = NULL;
  rule->name = NULL;
  rule->state = NULL;
  rule->static_env = NULL;
  rule->var_name = NULL;
  rule->start_conds = NULL;
  rule->next = NULL;
  rule->init = NULL;
  rule->ith = NULL;
  rule->itt = NULL;
  rule->sync_lock = NULL;
  rule->sync_vars = NULL;
  GC_UPDATE(ctx->gc_ctx, 0, rule);

  rule->filename = gc_strdup (ctx->gc_ctx, node_rule->file);
  rule->file_mtime = filestat.st_mtime;
  rule->name = gc_strdup (ctx->gc_ctx, node_rule->name);
  rule->lineno = node_rule->line;
  rule->static_env_sz = ctx->statics_nb;
  rule->dynamic_env_sz = ctx->rule_env->elmts;
  rule->id = ctx->rules;

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
	  sync_var = strhash_get(ctx->rule_env,
				 node_rule->sync_vars->vars[s]);
	  if (sync_var == NULL)
	    {
	      DebugLog(DF_OLC, DS_ERROR,
		       "Unknown reference to variable %s\n",
		       node_rule->sync_vars->vars[s]);
	      exit(EXIT_FAILURE);
	    }
	  DebugLog(DF_OLC, DS_INFO,
		   "Found syncvar $%s resid=%i\n",
		   node_rule->sync_vars->vars[s],
		   SYM_RES_ID(sync_var));
	  rule->sync_vars[s] = SYM_RES_ID(sync_var);
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

  if (node_rule->statelist!=NULL)
    rule->state_nb = list_len (node_rule->statelist) + 1;
  else
    rule->state_nb = 1;
  rule->state = gc_base_malloc (ctx->gc_ctx,
				rule->state_nb * sizeof (state_t));
  for (s=0, m=rule->state_nb; s<m; s++)
    rule->state[s].trans = NULL; /* to prevent garbage collector bugs:
				   in case the allocations below
				   call the garbage collector, which
				   may call rule_finalize() on
				   rule->state[i], which should be
				   filled with valid pointers---or NULL. */
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
           node_rule->name, ctx->currfile, node_rule->line);

  gc_strhash_add(ctx->gc_ctx, ctx->rulenames_hash,
		 (gc_header_t *)rule,
		 gc_strdup (ctx->gc_ctx, rule->name));

  if (ctx->first_rule!=NULL)
    GC_TOUCH (ctx->gc_ctx, ctx->last_rule->next = rule);
  else
    GC_TOUCH (ctx->gc_ctx, ctx->first_rule = rule);
  GC_TOUCH (ctx->gc_ctx, ctx->last_rule = rule);

  ctx->rules++;
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
           "compiling state \"%s\" in rule \"%s\"\n",
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
           "compiling actions in state \"%s\" in rule \"%s\"\n",
           state->name, rule->name);

  if (actionlist!=NULL)
    {
      DebugLog(DF_OLC, DS_DEBUG, "compiling actions bytecode\n");

      code.bytecode[0] = '\0';
      code.pos = 0;
      code.used_fields_sz = 0;
      code.flags = 0;
      code.ctx = ctx;

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

      if (code.flags & BYTECODE_HAS_PUSHFIELD) {
	state->flags |= BYTECODE_HAS_PUSHFIELD;
      }

      state->action = bytecode;
      FREE_LABELS(code.labels);
    }
  else
    {
      DebugLog(DF_OLC, DS_INFO, "state \"%s\" has no action\n", state->name);
    }
}

/**
 * Compile a list of action expressions into byte code.
 * @param expr A list of action expressions.
 * @param state State to compile.
 * @return An allocated byte code buffer.
 **/
static void compile_actions_bytecode(node_expr_t *actionlist, bytecode_buffer_t *code)
{
  for (; actionlist!=NULL; actionlist=BIN_RVAL(actionlist))
    compile_bytecode_stmt(BIN_LVAL(actionlist), code);
  if (code->pos >= BYTECODE_BUF_SZ - 2) {
    DebugLog(DF_OLC, DS_FATAL, "Bytecode buffer full\n");
    exit(EXIT_FAILURE);
  }
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
  code.flags = 0;
  code.ctx = ctx;

  INIT_LABELS(ctx, code.labels);

  compile_bytecode_expr(expr, &code);
  compile_set_labels(&code);

  if (code.pos >= BYTECODE_BUF_SZ - 1) {
    DebugLog(DF_OLC, DS_FATAL, "Bytecode buffer full\n");
    exit(EXIT_FAILURE);
  }
  code.bytecode[code.pos++] = OP_END;

  DebugLog(DF_OLC, DS_TRACE, "bytecode size: %i\n", code.pos);
  DebugLog(DF_OLC, DS_TRACE, "used fields: %i\n", code.used_fields_sz);

  trans->eval_code = gc_base_malloc (ctx->gc_ctx,
				     code.pos * sizeof (bytecode_t));
  memcpy(trans->eval_code, code.bytecode, code.pos * sizeof (bytecode_t));

  trans->required_fields_nb = code.used_fields_sz;
  if (code.used_fields_sz > 0)
    {
      trans->required_fields = gc_base_malloc (ctx->gc_ctx,
					       code.used_fields_sz
					       * sizeof(int));
      memcpy (trans->required_fields, code.used_fields,
	      code.used_fields_sz * sizeof(int));
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
      break;

    case NODE_BINOP:
      /* No: binops are expressions that return values, so
	 the following old code is wrong.
      compile_bytecode_stmt(expr->bin.lval, code);
      compile_bytecode_stmt(expr->bin.rval, code);
      code->bytecode[ code->pos++ ] = expr->bin.op;
      break;
      */
      /*FALLTHROUGH*/
  case NODE_MONOP: /*FALLTHROUGH*/
  case NODE_EVENT: /*FALLTHROUGH*/
  case NODE_CALL:
      compile_bytecode_expr(expr, code);
      EXIT_IF_BYTECODE_BUFF_FULL(1);
      /* Trash the function result */
      code->bytecode[ code->pos++ ] = OP_TRASH;
      break;

  case NODE_COND:
    {
      unsigned int label_end = NEW_LABEL(code->labels);

      compile_bytecode_cond(expr, code, label_end, label_end);
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
      n = REGSPLIT_DEST_VARS(expr)->vars_nb;
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
      break;
    }
    
  case NODE_IFSTMT:
    {
      int label_then = NEW_LABEL(code->labels);
      int label_else = NEW_LABEL(code->labels);
      int label_end = NEW_LABEL(code->labels);

      compile_bytecode_cond(IF_COND(expr), code,
			    label_then, label_else);
      SET_LABEL (code->ctx, code->labels,  code->pos, label_then);
      compile_actions_bytecode(IF_THEN(expr), code);
      EXIT_IF_BYTECODE_BUFF_FULL(2);
      code->bytecode[ code->pos++ ] = OP_JMP;
      PUT_LABEL (code->ctx, code->labels, code, label_end);

      SET_LABEL (code->ctx, code->labels, code->pos, label_else);
      compile_actions_bytecode(IF_ELSE(expr), code);
      SET_LABEL (code->ctx, code->labels, code->pos, label_end);
      break;
    }
  case NODE_UNKNOWN:
  case NODE_FIELD:
  case NODE_VARIABLE:
  case NODE_CONST:
    // XXX Should discard expression with no side effect
#if 0
    DPRINTF( ("Rule compiler expr : should be discarded %i\n", expr->type) );
    compile_bytecode_expr(expr, code);
    EXIT_IF_BYTECODE_BUFF_FULL(1);
    /* Trash the result */
    code->bytecode[ code->pos++ ] = OP_TRASH;
#endif
    break;
  default:
    DPRINTF( ("unknown node type (%i)\n", expr->type) );
    /* No side effect, discard stmt */
    break;
  }
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
      break;

      /* binary operator */
    case NODE_BINOP:
      compile_bytecode_expr(BIN_LVAL(expr), code);
      compile_bytecode_expr(BIN_RVAL(expr), code);
      EXIT_IF_BYTECODE_BUFF_FULL(1);
      code->bytecode[ code->pos++ ] = BIN_OP(expr);
      break;

      /* unary operator */
    case NODE_MONOP:
      compile_bytecode_expr(MON_VAL(expr), code);
      EXIT_IF_BYTECODE_BUFF_FULL(1);
      code->bytecode[ code->pos++ ] = MON_OP(expr);
      break;

      /* meta-event */
    case NODE_EVENT:
      if (BIN_LVAL(expr)!=NULL)
	compile_bytecode_expr(BIN_LVAL(expr), code);
      else
	{
	  EXIT_IF_BYTECODE_BUFF_FULL(1);
	  code->bytecode[code->pos++] = OP_EMPTY_EVENT;
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
	  }
      }
      break;

    case NODE_COND:
    {
      unsigned int	label_then = NEW_LABEL(code->labels);
      unsigned int	label_else = NEW_LABEL(code->labels);
      unsigned int	label_end = NEW_LABEL(code->labels);

      compile_bytecode_cond(expr, code, label_then, label_else);
      SET_LABEL (code->ctx, code->labels, code->pos, label_then);
      EXIT_IF_BYTECODE_BUFF_FULL(4);
      code->bytecode[ code->pos++ ] = OP_PUSHONE;
      code->bytecode[ code->pos++ ] = OP_JMP;
      PUT_LABEL (code->ctx, code->labels, code, label_end);
      SET_LABEL (code->ctx, code->labels, code->pos, label_else);
      code->bytecode[ code->pos++ ] = OP_PUSHZERO;
      SET_LABEL (code->ctx, code->labels, code->pos, label_end);
      break;
    }

    case NODE_FIELD:
      EXIT_IF_BYTECODE_BUFF_FULL(2);
      code->bytecode[ code->pos++ ] = OP_PUSHFIELD;
      code->bytecode[ code->pos++ ] = SYM_RES_ID(expr);
      {
	size_t i, idx, shift;
	size_t n;

	i = SYM_RES_ID(expr);
	idx = i / (8 * sizeof(int));
	shift = i % (8 * sizeof(int));
	if (idx>=MAX_FIELD_SZ)
	  {
	    DebugLog (DF_OLC, DS_FATAL,
		      "Record field id too large (%li)\n", i);
	    exit (EXIT_FAILURE);
	  }
	n = idx+1;
	if (n > code->used_fields_sz)
	  code->used_fields_sz = n;
	code->used_fields[idx] |= (1 << shift);
      }
      code->flags |= BYTECODE_HAS_PUSHFIELD;
      break;

    case NODE_VARIABLE:
      EXIT_IF_BYTECODE_BUFF_FULL(2);
      code->bytecode[ code->pos++ ] = OP_PUSH;
      code->bytecode[ code->pos++ ] = SYM_RES_ID(expr);
      break;

    case NODE_CONST:
      EXIT_IF_BYTECODE_BUFF_FULL(2);
      code->bytecode[ code->pos++ ] = OP_PUSHSTATIC;
      code->bytecode[ code->pos++ ] = TERM_RES_ID(expr);
      break;

    case NODE_CALL:
      {
	node_expr_t *params;

	for (params = CALL_PARAMS(expr); params!=NULL;
	     params = BIN_RVAL(params))
	  compile_bytecode_expr(BIN_LVAL(params), code);
	/*
	  if (expr->call.paramlist)
	  if (expr->call.paramlist->params_nb)
          for (i = expr->call.paramlist->params_nb - 1; i >= 0; --i)
	  compile_bytecode_expr(expr->call.paramlist->params[i], code);
	*/
	EXIT_IF_BYTECODE_BUFF_FULL(2);
	code->bytecode[ code->pos++ ] = OP_CALL;
	code->bytecode[ code->pos++ ] = CALL_RES_ID(expr);
	break;
      }
    case NODE_REGSPLIT:
    case NODE_IFSTMT:
      DPRINTF( ("Rule compiler expr : statement encountered (%i)\n", expr->type) );
      break;
    default:
      DPRINTF( ("unknown node type (%i)\n", expr->type) );
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
				  int label_else)
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
	  // XXX Optimization : Create Op code CEQJMP, CNEQJMP ...
	  // remove a push + pop
	  compile_bytecode_expr(BIN_LVAL(expr), code);
	  compile_bytecode_expr(BIN_RVAL(expr), code);
	  EXIT_IF_BYTECODE_BUFF_FULL(5);
	  code->bytecode[ code->pos++ ] = BIN_OP(expr);
	  code->bytecode[ code->pos++ ] = OP_POPCJMP;
	  PUT_LABEL (code->ctx, code->labels, code, label_then);
	  code->bytecode[ code->pos++ ] = OP_JMP;
	  PUT_LABEL (code->ctx, code->labels, code, label_else);
	  return;
	case ANDAND:
	  {
	    unsigned int label_i = NEW_LABEL(code->labels);

	    compile_bytecode_cond(BIN_LVAL(expr), code, label_i, label_else);
	    SET_LABEL (code->ctx, code->labels, code->pos, label_i);
	    compile_bytecode_cond(BIN_RVAL(expr), code, label_then, label_else);
	    return;
	  }
	case OROR:
	  {
	    unsigned int label_i = NEW_LABEL(code->labels);

	    compile_bytecode_cond(BIN_LVAL(expr), code, label_then, label_i);
	    SET_LABEL (code->ctx, code->labels, code->pos, label_i);
	    compile_bytecode_cond(BIN_RVAL(expr), code, label_then, label_else);
	    return;
	  }
	case BANG:
	  {
	    compile_bytecode_cond(BIN_LVAL(expr), code, label_else, label_then); 
	    return;
	  }
	default:
	  DPRINTF( ("Rule compiler cond : unknown op (%i)\n", BINOP(expr)) );
	  return;
	}
    case NODE_ASSOC:
    case NODE_BINOP:
    case NODE_MONOP:
    case NODE_EVENT:
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
      return;
    case NODE_IFSTMT:
    case NODE_UNKNOWN:
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
  int i;

  DebugLog(DF_OLC, DS_INFO,
           "compiling transitions in state \"%s\" in rule \"%s\"\n",
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
	  trans->required_fields = NULL;
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
	  state->trans[i].id = i; /* set trans id */

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
		      state->trans[i].dest = &rule->state[s->state_id];
		    }
		  else
		    {
		      DebugLog(DF_OLC, DS_FATAL,
			       "undefined state reference '%s'\n",
			       node_trans->dest);
		      exit (EXIT_FAILURE);
		    }
		}
	      compile_trans_bytecode(ctx, node_trans->cond,
				     &state->trans[i]);
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
		  state->trans[i].dest = &rule->state[s->state_id];
		}
	      else
		{
		  DebugLog(DF_OLC, DS_FATAL,
			   "undefined state reference '%s'\n",
			   node_trans->dest);
		  exit (EXIT_FAILURE);
		}
	    }
	}
    }
  else
    {
      /* Terminal state */
      DebugLog(DF_OLC, DS_TRACE,
	       "state \"%s\" has no transition (TERMINAL STATE)\n",
	       state->name);
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
  fprintf(fp, "  dynamic environment size : %i\n",
	  ctx->rule_compiler->rule_env->elmts);
}


/**
 * fprintf_expr() sub-routine : display a terminal expression node.
 * @param fp Output stream.
 * @param expr A node to display.
 **/
static void fprintf_term_expr(FILE *fp, node_expr_t *expr)
{
  size_t i, n;
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
	  for (i=0; i<n; i++)
	    fputc (*s++, fp);
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
      fprintf(fp, " %3i | %12.12s | %3i | %3i | %3i | %3i | %3i | %.24s:%i\n",
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
