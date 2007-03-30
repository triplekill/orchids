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
 ** @date Last update: Fri Mar 30 10:20:06 2007
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
extern FILE *issdlin;
extern int issdllineno_g;
extern char *issdlcurrentfile_g;

/* XXX: static function prototype definition:
 * (should be moved in a private header) */

static void
compile_and_add_rulefile(orchids_t *ctx, char *rulefile);

static void
compile_state_ast(rule_compiler_t *ctx,
                  rule_t *rule,
                  state_t *state,
                  node_state_t *node_state);

static void
compile_actions_ast(rule_compiler_t *ctx,
                    rule_t *rule,
                    state_t *state,
                    node_actionlist_t *actionlist);

static void
compile_transitions_ast(rule_compiler_t *ctx,
                        rule_t *rule,
                        state_t *state,
                        node_translist_t *translist);

static bytecode_t *
compile_actions_bytecode(node_expr_t **expr, int n, state_t *state);

static bytecode_t *
compile_bytecode(node_expr_t *expr, transition_t *trans);

static void
compile_bytecode_sub(node_expr_t *expr, bytecode_buffer_t *code);

static void
statics_add(rule_compiler_t *ctx, ovm_var_t *data);

static void
build_fields_hash(orchids_t *ctx);

static void
build_functions_hash(orchids_t *ctx);

static void
fprintf_term_expr(FILE *fp, node_expr_t *expr);

rule_compiler_t *
new_rule_compiler_ctx(void)
{
  rule_compiler_t *ctx;

  ctx = Xzmalloc(sizeof (rule_compiler_t));

  ctx->fields_hash     = new_strhash(1021);
  ctx->functions_hash  = new_strhash(1021);
  ctx->rulenames_hash  = new_strhash(251);

  ctx->statenames_hash = new_strhash(251);
  ctx->rule_env        = new_strhash(251);
  ctx->statics = Xzmalloc(STATICS_SZ * sizeof (ovm_var_t *));
  ctx->dyn_var_name = Xzmalloc(DYNVARNAME_SZ * sizeof (char *));

  return (ctx);
}

/**
 * Add a new entry in the static data environment.
 * @param ctx Rule compiler context.
 * @param data The new 'static' value.
 **/
static void
statics_add(rule_compiler_t *ctx, ovm_var_t *data)
{
  if (ctx->statics_nb == ctx->statics_sz)
    {
      ctx->statics_sz += STATICS_SZ;
      ctx->statics = Xrealloc(ctx->statics,
                              ctx->statics_sz * sizeof (ovm_var_t *));
    }

  ctx->statics[ ctx->statics_nb++ ] = data;
}

/**
 * Add a new entry in the dynamic environment variable name table.
 * @param ctx Rule compiler context.
 * @param var_name The new variable name.
 **/
static void
dynamic_add(rule_compiler_t *ctx, char *var_name)
{
  if (ctx->dyn_var_name_nb == ctx->dyn_var_name_sz) {
    ctx->dyn_var_name_sz += STATICS_SZ;
    ctx->dyn_var_name = Xrealloc(ctx->dyn_var_name,
                                 ctx->dyn_var_name_sz * sizeof (char *));
  }
  ctx->dyn_var_name[ ctx->dyn_var_name_nb++ ] = var_name;
}

void
compile_rules(orchids_t *ctx)
{
  rulefile_t *rulefile;

  DebugLog(DF_OLC, DS_NOTICE, "*** beginning rule compilation ***\n");

  build_fields_hash(ctx);
  /* XXX functions hash construction moved to register_lan_function() */
  build_functions_hash(ctx);

  for (rulefile = ctx->rulefile_list; rulefile; rulefile = rulefile->next)
    compile_and_add_rulefile(ctx, rulefile->name);

  gettimeofday(&ctx->compil_time, NULL);

/*   DebugLog(DF_OLC, DS_DEBUG, "Pre-compute reachable init states\n"); */
  /* precomp_rule_init_threads(ctx); */
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
#ifdef ENABLE_PREPROC
  char cmd[4096];
#endif /* ENABLE_PREPROC */

  DebugLog(DF_OLC, DS_NOTICE, "Compiling rule file '%s'\n", rulefile);

  /* set some compiler context values */
  ctx->rule_compiler->currfile = rulefile;
  issdlcurrentfile_g = strdup(rulefile);
  issdllineno_g = 1;
#ifdef ENABLE_PREPROC
    snprintf(cmd, sizeof (cmd), "%s %s", ctx->preproc_cmd, rulefile);
    issdlin = Xpopen(cmd, "r");
#else
    issdlin = Xfopen(rulefile, "r");
#endif /* ENABLE_PREPROC */

  /* parse rule file */
  ret = issdlparse();
  if (ret > 0) {
    DebugLog(DF_OLC, DS_FATAL,
             "Error while compiling rulefile '%s'\n", rulefile);
    exit(EXIT_FAILURE);
  }

#ifdef ENABLE_PREPROC
  ret = Xpclose(issdlin);
  if (ret > 0) {
    DebugLog(DF_OLC, DS_ERROR, "error: preprocessor returned %i.\n", ret);
    exit(EXIT_FAILURE);
  }
#endif /* ENABLE_PREPROC */

  gettimeofday(&ctx->last_rule_act, NULL);
}

/**
 * Create substate names for nested anonymous states.
 * state => state_0, state_1, etc...
 * @param refname Reference name.
 * @param n Number to append.
 * @return Anallocated string buffer containing the new name.
 **/
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

/**
 * Build a 'statelist' node and add the first state.
 * @param first_state The first state to add to the state list.
 * @return An allocated state list node.
 **/
node_statelist_t *
build_statelist(node_state_t *first_state)
{
  node_statelist_t *sl;

  /* DPRINTF( ("building statelist\n") ); */

  /* XXX: Hard-coded allocation parameters !!! */
  sl = Xmalloc(sizeof (node_statelist_t));
  sl->states_nb = 1;
  sl->size = 16;
  sl->grow = 16;
  sl->states = Xmalloc(16 * sizeof (node_state_t *));
  sl->states[0] = first_state;
  first_state->state_id = 1; /* Begin at 1 because there is the 'init' state */

  return (sl);
}

/**
 * Add a state node to a state list node.
 * @param list The state list node.
 * @param state The state to add to the state list.
 **/
void
statelist_add(node_statelist_t *list, node_state_t *state)
{
  state->state_id = list->states_nb + 1;

  if (list->states_nb == list->size)
    {
      list->size += list->grow;
      list->states = Xrealloc(list->states,
                              list->size * sizeof (node_state_t *));
    }

  list->states[ list->states_nb++ ] = state;
}

/**
 * Build a state node from an actionlist node and an transitionlist node.
 * @param actions Action list node.
 * @param transitions Transitions list node.
 * @return A new state node.
 **/
node_state_t *
build_state(node_actionlist_t *actions, node_translist_t *transitions)
{
  node_state_t *s;

  s = Xzmalloc(sizeof (node_state_t));
  s->actionlist = actions;
  s->translist = transitions;

  return (s);
}

/**
 * Set a state node name (or label) to a given symbol.
 * @param ctx Rule compiler context.
 * @param state The state to label.
 * @param sym Symbol (name and line) to associate to the state node.
 * @param flags Optional state flags.
 * @return The labelised state (NOT REALLOCATED !)
 **/
node_state_t *
set_state_label(rule_compiler_t *ctx, node_state_t *state, symbol_token_t *sym, unsigned long flags)
{
  if (state == NULL)
    return (NULL);

  state->name = sym->name;
  state->line = sym->line;
  state->flags = flags;

  /* add state name in current compiler context */
  if (strhash_get(ctx->statenames_hash, sym->name)) {
    DebugLog(DF_OLC, DS_FATAL,
             "state %s already defined.\n", sym->name);
    exit(EXIT_FAILURE);
  }

  strhash_add(ctx->statenames_hash, state, state->name);

  return (state);
}

/**
 * Build a function call parameter list node.
 * @param first_param The first parameter to add to the list.
 * @return A new allocated paremeter list node.
 **/
node_paramlist_t *
build_paramlist(node_expr_t *first_param)
{
  node_paramlist_t *pl;

  pl = Xmalloc(sizeof (node_paramlist_t));
  pl->params_nb = 1;
  pl->size = 8;
  pl->grow = 8;
  pl->params = Xmalloc(8 * sizeof (node_expr_t *));
  pl->params[0] = first_param;

  return (pl);
}

/**
 * Add an expression to a parameter list.
 * @param list The parameter list node.
 * @param expr The expression node to add to the parameter list node.
 **/
void
paramlist_add(node_paramlist_t *list, node_expr_t *expr)
{
  if (list->params_nb == list->size)
    {
      list->size += list->grow;
      list->params = Xrealloc(list->params,
                              list->size * sizeof (node_expr_t *));
    }

  list->params[ list->params_nb++ ] = expr;
}

/**
 * Build a variable list node.
 * @param first_param The first variable to add to the list.
 * @return A new allocated variable list node.
 **/
node_varlist_t *
build_varlist(node_expr_t *first_param)
{
  node_varlist_t *vl;

  vl = Xmalloc(sizeof (node_varlist_t));
  vl->vars_nb = 1;
  vl->size = 8;
  vl->grow = 8;
  vl->vars = Xmalloc(8 * sizeof (node_expr_t *));
  vl->vars[0] = first_param;

  return (vl);
}

/**
 * Add a variable to a parameter list.
 * @param list The variable list node.
 * @param expr The expression node to add to the variable list node.
 **/
void
varlist_add(node_varlist_t *list, node_expr_t *expr)
{
  if (list->vars_nb == list->size) {
    list->size += list->grow;
    list->vars = Xrealloc(list->vars,
                          list->size * sizeof (node_expr_t *));
  }

  list->vars[ list->vars_nb++ ] = expr;
}


/**
 * Build a synchronization variable list node.
 * @param first_param  The first variable to add to the list.
 * @return A new allocated variable list node.
 **/
node_syncvarlist_t *
build_syncvarlist(char *first_syncvar)
{
  node_syncvarlist_t *vl;

  vl = Xmalloc(sizeof (node_syncvarlist_t));
  vl->vars_nb = 1;
  vl->size = 8;
  vl->grow = 8;
  vl->vars = Xmalloc(8 * sizeof (char *));
  vl->vars[0] = first_syncvar;

  return (vl);
}

/**
 * Add a synchronization variable to a parameter list.
 * @param list The variable list node.
 * @param syncvar The synchronization variable to add to the list node.
 **/
node_syncvarlist_t *
syncvarlist_add(node_syncvarlist_t *list, char *syncvar)
{
  if (list->vars_nb == list->size) {
    list->size += list->grow;
    list->vars = Xrealloc(list->vars,
                          list->size * sizeof (char *));
  }

  list->vars[ list->vars_nb++ ] = syncvar;

  return (list);
}


/**
 * Build an unconditional transition node.
 * @param dest The name string of the destination state.
 * @return The new unconditinoal transition node.
 **/
node_translist_t *
build_unconditional_transition(char *dest)
{
  node_trans_t *n;
  node_translist_t *nl;

  n = build_direct_transition(NULL, dest);
  nl = build_transitionlist(n);

  return (nl);
}

node_trans_t *
build_unconditional_transition_test(char *dest)
{
  node_trans_t *n;

  n = build_direct_transition(NULL, dest);

  return (n);
}


/**
 * Build a direct transition node.
 * A direct transition is "if (cond) goto somewhere".
 * @param cond The condition expression of the transition.
 * @param dest The name string of the destination state.
 * @return The new transition node.
 **/
node_trans_t *
build_direct_transition(node_expr_t *cond, char *dest)
{
  node_trans_t *trans;

  trans = Xzmalloc(sizeof (node_trans_t));
  trans->cond = cond;
  trans->dest = dest;

  return (trans);
}

/**
 * Build indirect transition.
 * An indirect transition is "if (cond) { do something }".
 * @param cond The condition expression of the transition.
 * @param substate The nested sub-state block.
 * @return The new transition node.
 **/
node_trans_t *
build_indirect_transition(node_expr_t *cond, node_state_t *substate)
{
  node_trans_t *trans;

  trans = Xzmalloc(sizeof (node_trans_t));
  trans->cond = cond;
  trans->sub_state_dest = substate;

  return (trans);
}

/**
 * Build transition list.
 **/
node_translist_t *
build_transitionlist(node_trans_t *trans)
{
  node_translist_t *tl;

  /* XXX: Hard-coded allocation parameters !!! */
  tl = Xmalloc(sizeof (node_translist_t));
  tl->trans_nb = 1;
  tl->size = 16;
  tl->grow = 16;
  tl->trans = Xmalloc(16 * sizeof (node_trans_t *));
  tl->trans[0] = trans;

  return (tl);
}

/**
 * Add a transition to a transition list.
 * @param list The transition list node.
 * @param trans The transition node to add to the transition list node.
 **/
void
transitionlist_add(node_translist_t *list, node_trans_t *trans)
{
  if (list->trans_nb == list->size)
    {
      list->size += list->grow;
      list->trans = Xrealloc(list->trans,
                             list->size * sizeof (node_trans_t *));
    }

  list->trans[ list->trans_nb++ ] = trans;
}

/**
 * Build an action list node.
 * @param first_action The first parameter to add to the list.
 * @return A new allocated action list node.
 **/
node_actionlist_t *
build_actionlist(node_action_t *first_action)
{
  node_actionlist_t *al;

  /* XXX: Hard-coded allocation parameters !!! */
  al = Xmalloc(sizeof (node_actionlist_t));
  al->actions_nb = 1;
  al->size = 16;
  al->grow = 16;
  al->actions = Xmalloc(16 * sizeof (node_action_t *));
  al->actions[0] = first_action;

  return (al);
}

/**
 * Add an action to an action list node.
 * @param list The action list node.
 * @param action The action node to add to the action list node.
 **/
void
actionlist_add(node_actionlist_t *list, node_action_t *action)
{
  if (list->actions_nb == list->size)
    {
      list->size += list->grow;
      list->actions = Xrealloc(list->actions,
                               list->size * sizeof (node_action_t *));
    }

  list->actions[ list->actions_nb++ ] = action;
}

/**
 * Build a rule node.
 * @param  sym          The rule name (symbol).
 * @param  init_state   The initial state of the rule.
 * @param  states       Additional state list.
 * @param  sync_vars   Synchronization variable list.
 * @return  A new allocated rule node.
 **/
node_rule_t *
build_rule(symbol_token_t   *sym,
           node_state_t     *init_state,
           node_statelist_t *states,
           node_syncvarlist_t   *sync_vars)
{
  node_rule_t *new_rule;

  new_rule = Xzmalloc(sizeof (node_rule_t));
  new_rule->name = sym->name;
  new_rule->file = sym->file;
  new_rule->line = sym->line;
  new_rule->init = init_state;
  new_rule->statelist = states;
  new_rule->sync_vars = sync_vars;

  return (new_rule);
}

/*
** expression building functions...
*/

node_expr_t *
build_string_split(node_expr_t *source,
                   node_expr_t *pattern,
                   node_varlist_t *dest_list)
{
  node_expr_t *n;

  n = Xmalloc(sizeof (node_expr_t));
  n->type = NODE_REGSPLIT;
  n->regsplit.string = source;
  n->regsplit.split_pat = pattern;
  n->regsplit.dest_vars = dest_list;

  DPRINTF( ("splits = %i\n", dest_list->vars_nb) );

  SREGEXNUM(pattern->term.data) = dest_list->vars_nb;

  return (n);
}


/**
 * Build a binary expression node.
 * @param op The operator identifier.
 * @param left_node The left part of the binary expression.
 * @param right_node The right part of the binary expression.
 * @return A new allocated expression node.
 **/
node_expr_t *
build_expr(int op, node_expr_t *left_node, node_expr_t *right_node)
{
  node_expr_t *n;

  n = Xmalloc(sizeof (node_expr_t));
  n->type = NODE_BINOP;
  n->bin.op = op;
  n->bin.lval = left_node;
  n->bin.rval = right_node;

  return (n);
}

/**
 * Build an association (affectation).
 * @param ctx Rule compiler context. Needed to resolve the variable name.
 * @param var The variable name (left value).
 * @param expr The expression to associate to the variable (right value).
 * @return A new allocated expression node.
 **/
node_expr_t *
build_assoc(rule_compiler_t *ctx, char *var, node_expr_t *expr)
{
  node_expr_t *n;

  n = Xmalloc(sizeof (node_expr_t));
  n->type = NODE_BINOP;
  n->bin.op = EQ;
  n->bin.lval = build_varname(ctx, var);
  n->bin.rval = expr;

  return (n);
}

/**
 * Build a function call node.
 * @param ctx Rule compiler context. Needed to resolve function name.
 * @param sym The function name (symbol) to call.
 * @param paramlist The parameter list for this function.
 **/
node_expr_t *
build_function_call(rule_compiler_t  *ctx,
                    symbol_token_t   *sym,
                    node_paramlist_t *paramlist)
{
  node_expr_t *call_node;
  issdl_function_t *func;

  func = strhash_get(ctx->functions_hash, sym->name);
  if (func == NULL) {
    DebugLog(DF_OLC, DS_FATAL,
             "unresolved symbol for function '%s'\n", sym->name);
    exit(EXIT_FAILURE);
  }

  call_node = Xmalloc(sizeof (node_expr_t));
  call_node->type = NODE_CALL;
  call_node->call.symbol = sym->name;
  call_node->call.paramlist = paramlist;
  call_node->call.res_id = func->id;

  return (call_node);
}

/**
 * Build a fieldname (of a module) expression node.
 * @param ctx Rule compiler context. Needed for field name resolution.
 * @param fieldname A registered module fieldname.
 * @return A new allocated expression node.
 **/
node_expr_t *
build_fieldname(rule_compiler_t *ctx, char *fieldname)
{
  field_record_t *f;
  node_expr_t *n;

  /* check if field exists */
  /* XXX: resolution can be in the descendant phase (2) */
  f = strhash_get(ctx->fields_hash, fieldname);
  if (f == NULL) {
    DebugLog(DF_OLC, DS_FATAL, "field %s not registered\n", fieldname);
    exit(EXIT_FAILURE);
  }

  n = Xmalloc(sizeof (node_expr_t));
  n->type = NODE_FIELD;
  n->sym.name = fieldname;
  n->sym.res_id = f->id;

  return (n);
}

/**
 * Build a variable name expression node.
 * @param ctx Rule compiler context. Needed to resolve/add
 *   variable to the global symbol table.
 * @param varname The variable name.
 * @return A new allocated variable name expression node.
 **/
node_expr_t *
build_varname(rule_compiler_t *ctx, char *varname)
{
  node_expr_t *n;
  node_expr_t *tmp;

  n = Xmalloc(sizeof (node_expr_t));
  n->type = NODE_VARIABLE;
  n->sym.name = varname;

  /* resolve variable in envrironment variable hash table */
  tmp = strhash_get(ctx->rule_env, varname);

  /* if it's doesn't exist, add it to the hash table */
  if (tmp == NULL) {
    n->sym.res_id = ctx->rule_env->elmts;
    strhash_add(ctx->rule_env, n, varname);
    dynamic_add(ctx, varname);
  } else {
    n->sym.res_id = tmp->sym.res_id;
  }

  return (n);
}



/**
 * Build an interger expression node.
 * @param ctx Rule compiler context.
 * @param i The integer value.
 * @return A new allocated integer expression node.
 **/
node_expr_t *
build_integer(rule_compiler_t *ctx, int i)
{
  ovm_var_t    *integer;
  node_expr_t  *n;

  integer = ovm_int_new();
  integer->flags |= TYPE_CONST;
  INT(integer) = i;

  /* XXX Add hash for constant sharing here */

  n = Xmalloc(sizeof (node_expr_t));
  n->type = NODE_CONST;
  n->term.data = integer;
  n->term.res_id = ctx->statics_nb;

  statics_add(ctx, integer);

  return (n);
}


/**
 * Build a floating-point expression node.
 * @param ctx Rule compiler context.
 * @param i The double value.
 * @return A new allocated floating-point expression node.
 **/
node_expr_t *
build_double(rule_compiler_t *ctx, double f)
{
  ovm_var_t    *fpdouble;
  node_expr_t  *n;

  fpdouble = ovm_float_new();
  fpdouble->flags |= TYPE_CONST;
  FLOAT(fpdouble) = f;

  n = Xmalloc(sizeof (node_expr_t));
  n->type = NODE_CONST;
  n->term.data = fpdouble;
  n->term.res_id = ctx->statics_nb;

  statics_add(ctx, fpdouble);

  return (n);
}



/**
 * Build a string expression node.
 * @param ctx Rule compiler context.
 * @param str The string value.
 * @return A new allocated string expression node.
 **/
node_expr_t *
build_string(rule_compiler_t *ctx, char *str)
{
  size_t       string_len;
  node_expr_t *n;
  ovm_var_t   *string;

  string_len = strlen(str);
  string = ovm_str_new(string_len);
  string->flags |= TYPE_CONST;
  strcpy(STR(string), str);
  Xfree(str); /* free string memory allocated by the lexer */

  /* XXX Add hash for constant sharing here */

  n = Xmalloc(sizeof (node_expr_t));
  n->type = NODE_CONST;
  n->term.data = (void *) string;
  n->term.res_id = ctx->statics_nb;

  statics_add(ctx, string);

  return (n);
}

/**
 * Build a constant ipv4addr expression node.
 * @param ctx Rule compiler context.
 * @param hostname The hostname string.
 * @return A new allocated ipv4addr expression node.
 **/
node_expr_t *
build_ipv4(rule_compiler_t *ctx, char *hostname)
{
  ovm_var_t *addr;
  node_expr_t *n;
  struct hostent  *hptr;

  /* XXX Add hash for constant sharing here */

  addr = ovm_ipv4_new();
  addr->flags |= TYPE_CONST;
  hptr = gethostbyname(hostname);
  if (hptr == NULL) {
    DebugLog(DF_OLC, DS_FATAL,
             "hostname %s doesn't exist\n");
    exit(EXIT_FAILURE);
  }

  /* resolve host name */
  IPV4(addr).s_addr = *(in_addr_t *)(hptr->h_addr);

  n = Xmalloc(sizeof (node_expr_t));
  n->type = NODE_CONST;
  n->term.data = addr;
  n->term.res_id = ctx->statics_nb;

  statics_add(ctx, addr);

  return (n);
}

node_expr_t *
build_ctime_from_int(rule_compiler_t *ctx, long ctime)
{
  node_expr_t  *n;
  ovm_var_t    *time;

  time = ovm_ctime_new();
  time->flags |= TYPE_CONST;
  CTIME(time) = ctime;

  n = Xmalloc(sizeof (node_expr_t));
  n->type = NODE_CONST;
  n->term.data = time;
  n->term.res_id = ctx->statics_nb;

  statics_add(ctx, time);

  return (n);
}

node_expr_t *
build_ctime_from_string(rule_compiler_t *ctx, char *date)
{
  node_expr_t  *n;
  ovm_var_t    *time;

  time = ovm_ctime_new();
  time->flags |= TYPE_CONST;
  CTIME(time) = 0;

  n = Xmalloc(sizeof (node_expr_t));
  n->type = NODE_CONST;
  n->term.data = time;
  n->term.res_id = ctx->statics_nb;

  statics_add(ctx, time);

  return (n);
}

node_expr_t *
build_timeval_from_int(rule_compiler_t *ctx, long sec, long usec)
{
  node_expr_t  *n;
  ovm_var_t    *timeval;

  timeval = ovm_timeval_new();
  TIMEVAL(timeval).tv_sec  = sec;
  TIMEVAL(timeval).tv_usec = usec;

  n = Xmalloc(sizeof (node_expr_t));
  n->type = NODE_CONST;
  n->term.data = timeval;
  n->term.res_id = ctx->statics_nb;

  statics_add(ctx, timeval);

  return (n);
}

node_expr_t *
build_timeval_from_string(rule_compiler_t *ctx, char *date, long usec)
{
  node_expr_t *n;
  ovm_var_t *timeval;

  timeval = ovm_timeval_new();
  TIMEVAL(timeval).tv_sec  = 0;
  TIMEVAL(timeval).tv_usec = usec;

  n = Xmalloc(sizeof (node_expr_t));
  n->type = NODE_CONST;
  n->term.data = timeval;
  n->term.res_id = ctx->statics_nb;

  statics_add(ctx, timeval);

  return (n);
}

node_expr_t *
build_counter(rule_compiler_t *ctx, long initial_value)
{
  ovm_var_t    *counter;
  node_expr_t  *n;

  counter = ovm_counter_new();
  counter->flags |= TYPE_MONO;
  COUNTER(counter) = initial_value;

  n = Xmalloc(sizeof (node_expr_t));
  n->type = NODE_CONST;
  n->term.data = counter;
  n->term.res_id = ctx->statics_nb;

  statics_add(ctx, counter);

  return (n);
}

node_expr_t *
build_split_regex(rule_compiler_t *ctx, char *regex_str)
{
  node_expr_t  *n;
  ovm_var_t    *regex;
  int           ret;

  regex = ovm_sregex_new();
  regex->flags |= TYPE_CONST;
  SREGEXSTR(regex) = regex_str;

  /* compile regex */
  ret = regcomp(&(SREGEX(regex)), regex_str, REG_EXTENDED);
  if (ret)
    {
      char err_buf[64];

      DPRINTF( ("SPLITREGEX compilation error (%s)\n", regex_str) );
      regerror(ret, &(SREGEX(regex)), err_buf, sizeof (err_buf));
      exit(EXIT_FAILURE);
    }

  n = Xmalloc(sizeof (node_expr_t));
  n->type = NODE_CONST;
  n->term.data = regex;
  n->term.res_id = ctx->statics_nb;

  statics_add(ctx, regex);

  return (n);
}


node_expr_t *
build_regex(rule_compiler_t *ctx, char *regex_str)
{
  node_expr_t  *n;
  ovm_var_t    *regex;
  int           ret;

  regex = ovm_regex_new();
  regex->flags |= TYPE_CONST;
  REGEXSTR(regex) = regex_str;

  /* compile regex */
  ret = regcomp(&(REGEX(regex)), regex_str, REG_EXTENDED | REG_NOSUB);
  if (ret) {
    DPRINTF( ("REGEX compilation error (%s)\n", regex_str) );
    exit(EXIT_FAILURE);
  }

  n = Xmalloc(sizeof (node_expr_t));
  n->type = NODE_CONST;
  n->term.data = regex;
  n->term.res_id = ctx->statics_nb;

  statics_add(ctx, regex);

  return (n);
}


/**
 * Build the field name hash table.
 * @param ctx Orchids context.
 **/
static void
build_fields_hash(orchids_t *ctx)
{
  strhash_t *h;
  int        f;

  h = ctx->rule_compiler->fields_hash;
  for (f = 0; f < ctx->num_fields; f++)
    strhash_add(h, &ctx->global_fields[f], ctx->global_fields[f].name);

  DebugLog(DF_OLC, DS_INFO,
           "build_fields_hash(): size: %i elems: %i collides: %i\n",
           h->size, h->elmts, strhash_collide_count(h));
}

/**
 * Build the ISSDL built-in functions hash table.
 * @param ctx Orchids context.
 **/
static void
build_functions_hash(orchids_t *ctx)
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


/**
 * Concatenate the second string into the first.  The first string
 * is reallocated to the correct size.  The second string is freed.
 * @param str1 First string.
 * @param str2 Second string.
 **/
char *
build_concat_string(char *str1, char *str2)
{
  size_t str_len;
  char *string;

  str_len = strlen(str1) + strlen(str2) + 1;
  string = Xrealloc(str1, str_len);
  strncat(string, str2, str_len);
  free(str2);

  return (string);
}


static unsigned long
datahash_pjw(unsigned long h, void *key, size_t keylen)
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

  return (h);
}

static unsigned long
objhash_rule_instance(void *state_inst)
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
    if (si->current_env[ sync_var ])
      var = si->current_env[ sync_var ];
    else
      var = si->inherit_env[ sync_var ];

    h = datahash_pjw(h, &sync_var, sizeof (sync_var));
    h = datahash_pjw(h, &TYPE(var), sizeof (TYPE(var)));
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

    /* get var 1 */
    if (si1->current_env[ sync_var ])
      var1 = si1->current_env[ sync_var ];
    else
      var1 = si1->inherit_env[ sync_var ];

    /* get var 2 */
    if (si2->current_env[ sync_var ])
      var2 = si2->current_env[ sync_var ];
    else
      var2 = si2->inherit_env[ sync_var ];

    /* call comparison functions */
    ret = issdl_cmp(var1, var2);
    if (ret) {
      DebugLog(DF_ENG, DS_INFO, "si1=%p %c si2=%p\n",
               si1, ret > 0 ? '>' : '<', si2);
      return (ret);
    }
  }

  /* If we are here, the two state instances are synchronized */
  DebugLog(DF_ENG, DS_INFO, "si1=%p == si2=%p\n", si1, si2);

  return (0);
}


/**
 * Final phase of rule compilation.
 * - all symbols should by added in entry table
 * - this phase resolve all symbols, create environements and generate bytecode
 * @param ctx Rule compiler context.
 * @param node_rule The rule node (abstract syntaxt tree root) to compile.
 * @callgraph
 **/
void
compile_and_add_rule_ast(rule_compiler_t *ctx, node_rule_t *node_rule)
{
  rule_t *rule;
  int     s;
  int     t, tid;
  struct stat filestat;
  node_expr_t *sync_var;

  DebugLog(DF_OLC, DS_INFO,
           "----- compiling rule \"%s\" (from file %s:%i) -----\n",
           node_rule->name, ctx->currfile, node_rule->line);

  /* check if we don't already have this rule definition */
  if ((rule = strhash_get(ctx->rulenames_hash, node_rule->name))) {
    DebugLog(DF_OLC, DS_FATAL,
             "rule %s already defined in %s:%i\n",
             node_rule->name, rule->filename, rule->lineno);
    exit(EXIT_FAILURE);
  }

  Xstat(node_rule->file, &filestat);

  rule = Xzmalloc(sizeof (rule_t));
  rule->filename = node_rule->file;
  rule->file_mtime = filestat.st_mtime;
  rule->name = node_rule->name;
  rule->lineno = node_rule->line;
  rule->static_env_sz = ctx->statics_nb;
  rule->dynamic_env_sz = ctx->rule_env->elmts;
  rule->id = ctx->rules;

  /* allocate static env */
  rule->static_env = Xmalloc(rule->static_env_sz * sizeof (ovm_var_t *));
  for (s = 0; s < rule->static_env_sz; s++)
    rule->static_env[s] = ctx->statics[s]; /* XXX use memcpy */

  /* allocate dynamic env. variable names */
  if (rule->dynamic_env_sz > 0) {
    rule->var_name = Xmalloc(rule->dynamic_env_sz * sizeof (char *));
    for (s = 0; s < rule->dynamic_env_sz; s++)
      rule->var_name[s] = ctx->dyn_var_name[s]; /* XXX use memcpy */
  }

  /* create synchronization array */
  if (node_rule->sync_vars) {
    DebugLog(DF_OLC, DS_INFO, "Creating synchronization array\n");
    rule->sync_vars_sz = node_rule->sync_vars->vars_nb;
    rule->sync_vars = Xmalloc( rule->sync_vars_sz * sizeof (int));
    for (s = 0; s < rule->sync_vars_sz; s++) {
      sync_var = strhash_get(ctx->rule_env,
                             node_rule->sync_vars->vars[s]);
      if (sync_var == NULL) {
        DebugLog(DF_OLC, DS_ERROR,
                 "Unknown reference to variable %s\n",
                 node_rule->sync_vars->vars[s]);
        exit(EXIT_FAILURE);
      }
      DebugLog(DF_OLC, DS_INFO,
               "Found syncvar $%s resid=%i\n",
               node_rule->sync_vars->vars[s],
               sync_var->sym.res_id);
      rule->sync_vars[s] = sync_var->sym.res_id;
    }
    rule->sync_lock = new_objhash( 1021 );
    rule->sync_lock->hash = objhash_rule_instance;
    rule->sync_lock->cmp = objhash_rule_instance_cmp;
    /* XXX: should dynamically resize sync_lock hash. */
  }

  /* compile init state */
  if (!node_rule->init) {
    DebugLog(DF_OLC, DS_WARN, "!!! null init state\n");
    return ; /* XXX: Debug assertion : init state should not be NULL */
  }
  /* XXX - clean here */

  if (node_rule->statelist)
    rule->state_nb = node_rule->statelist->states_nb + 1;
  else
    rule->state_nb = 1;

  rule->state = Xzmalloc(rule->state_nb * sizeof (state_t));

  compile_state_ast(ctx, rule, &(rule->state[0]), node_rule->init);
  rule->state[0].id = 0; /* set state id */

  tid = 0;
  for (t = 0 ; t < rule->state[0].trans_nb; t++) {
    rule->state[0].trans[t].global_id = tid++;
  }

  for (s = 0; s < rule->state_nb - 1; s++) {
    compile_state_ast(ctx, rule, &(rule->state[s+1]),
                      node_rule->statelist->states[s]);
    rule->state[s].id = s; /* set state id */

    /* renumber transitions */
    for ( t = 0 ; t < rule->state[s+1].trans_nb ; t++) {
      rule->state[s+1].trans[t].global_id = tid++;
    }
  }

  DebugLog(DF_OLC, DS_INFO,
           "----- end of compilation of rule \"%s\" (from file %s:%i) -----\n",
           node_rule->name, ctx->currfile, node_rule->line);

  /* add rule name in the rule hash table... */
  strhash_add(ctx->rulenames_hash, rule, rule->name);

  /* ...and in the rule list */
  if (ctx->first_rule)
    ctx->last_rule->next = rule;
  else
    ctx->first_rule = rule;
  ctx->last_rule = rule;

  ctx->rules++;
}

/**
 * Compile a state node abstract syntax tree.
 * @param ctx Rule compiler context.
 * @param rule Current rule in compilation.
 * @param state Current state in compilation.
 * @param node_state State node abstract syntax tree root to compile.
 **/
static void
compile_state_ast(rule_compiler_t *ctx,
                  rule_t          *rule,
                  state_t         *state,
                  node_state_t    *node_state)
{
  DebugLog(DF_OLC, DS_INFO,
           "compiling state \"%s\" in rule \"%s\"\n",
           node_state->name, rule->name);

  state->name  = node_state->name;
  state->line  = node_state->line;
  state->flags = node_state->flags;
  state->rule  = rule;

  compile_actions_ast(ctx, rule, state, node_state->actionlist);
  compile_transitions_ast(ctx, rule, state, node_state->translist);
}


/**
 ** Compile an action list node abstract syntax tree.
 ** @param ctx Rule compiler context.
 ** @param rule Current rule in compilation.
 ** @param state Current state in compilation.
 ** @param actionlist Action list node abstract syntax tree root to compile.
 **/
static void
compile_actions_ast(rule_compiler_t   *ctx,
                    rule_t            *rule,
                    state_t           *state,
                    node_actionlist_t *actionlist)
{
  DebugLog(DF_OLC, DS_DEBUG,
           "compiling actions in state \"%s\" in rule \"%s\"\n",
           state->name, rule->name);

  if (actionlist) {
    state->action = compile_actions_bytecode(actionlist->actions,
                                             actionlist->actions_nb,
                                             state);
  }
  else {
    DebugLog(DF_OLC, DS_INFO, "state \"%s\" have no action\n", state->name);
  }
}


/**
 * Compile a list of action expressions into bytecode.
 * @param expr An array of action expression.
 * @param n Size on the array.
 * @param state State to compile.
 * @return An allocatated bytecode buffer.
 **/
static bytecode_t *
compile_actions_bytecode(node_expr_t **expr, int n, state_t *state)
{
  bytecode_buffer_t code;
  bytecode_t *bytecode;
  int a;

  DebugLog(DF_OLC, DS_DEBUG, "compiling actions bytecode\n");

  code.bytecode[0] = '\0';
  code.pos = 0;
  code.used_fields_pos = 0;
  code.flags = 0;

  for (a = 0; a < n; a++)
    compile_bytecode_sub(expr[a], &code);
  if (code.pos >= BYTECODE_BUF_SZ - 2) {
    DebugLog(DF_OLC, DS_FATAL, "Bytecode buffer full\n");
    exit(EXIT_FAILURE);
  }
  code.bytecode[ code.pos++ ] = OP_END;

  DebugLog(DF_OLC, DS_DEBUG, "bytecode size: %i\n", code.pos);

  bytecode = Xzmalloc(code.pos * sizeof (bytecode_t));
  memcpy(bytecode, code.bytecode, code.pos * sizeof (bytecode_t));

  if (code.flags & BYTECODE_HAVE_PUSHFIELD) {
    state->flags |= BYTECODE_HAVE_PUSHFIELD;
  }

  return (bytecode);
}


/**
 * Compile an evaluation expression into bytecode.
 *   @param expr  An evaluation expression.
 *   @param trans Transition to compile.
 *   @return An allocatated bytecode buffer.
 **/
static bytecode_t *
compile_bytecode(node_expr_t *expr, transition_t *trans)
{
  bytecode_buffer_t code;

  DebugLog(DF_OLC, DS_DEBUG, "compiling bytecode\n");

  code.bytecode[0] = '\0';
  code.pos = 0;
  code.used_fields_pos = 0;

  compile_bytecode_sub(expr, &code);

  if (code.pos >= BYTECODE_BUF_SZ - 2) {
    DebugLog(DF_OLC, DS_FATAL, "Bytecode buffer full\n");
    exit(EXIT_FAILURE);
  }
  code.bytecode[ code.pos++ ] = OP_END;

  DebugLog(DF_OLC, DS_TRACE, "bytecode size: %i\n", code.pos);
  DebugLog(DF_OLC, DS_TRACE, "used fields: %i\n", code.used_fields_pos);

  trans->eval_code = Xzmalloc(code.pos * sizeof (bytecode_t));
  memcpy(trans->eval_code, code.bytecode, code.pos * sizeof (bytecode_t));

  trans->required_fields_nb = code.used_fields_pos;
  if (code.used_fields_pos > 0)
    trans->required_fields = Xmalloc(code.used_fields_pos * sizeof (int));
  memcpy(trans->required_fields, code.used_fields, code.used_fields_pos * sizeof (int));

  return (trans->eval_code);
}


/**
 * Bytecode compiler recursive sub-routine.
 * @param expr An expression to compile.
 * @param code An internal bytecode buffer structure.
 **/
static void
compile_bytecode_sub(node_expr_t *expr, bytecode_buffer_t *code)
{
  if (expr->type == NODE_BINOP) {
    /* binary operator */
    if (expr->bin.op == EQ) {
      /* assoc is a special case, we must handle rval before lval */
      compile_bytecode_sub(expr->bin.rval, code);
      code->bytecode[ code->pos++ ] = OP_POP;
      code->bytecode[ code->pos++ ] = expr->bin.lval->sym.res_id;
      return ;
    }

    compile_bytecode_sub(expr->bin.lval, code);
    compile_bytecode_sub(expr->bin.rval, code);

    if (expr->bin.op == ANDAND) 
      return ;

    if (expr->bin.op == OROR) {
      DebugLog(DF_OLC, DS_ERROR, "or || not yet implemented (use parallel if instead)\n");
      return ;
    }
    code->bytecode[ code->pos++ ] = expr->bin.op;
  } else { /* (rval) terminal expression */
    int i;

    switch (expr->type) {

    case NODE_FIELD:
      if (code->pos >= BYTECODE_BUF_SZ - 3) {
        DebugLog(DF_OLC, DS_FATAL, "Bytecode buffer full\n");
        exit(EXIT_FAILURE);
      }
      code->bytecode[ code->pos++ ] = OP_PUSHFIELD;
      code->bytecode[ code->pos++ ] = expr->sym.res_id;
      /* XXX UGLY HACK */
      code->used_fields[ code->used_fields_pos++ ] = expr->sym.res_id;
      code->flags |= BYTECODE_HAVE_PUSHFIELD;
      break;

    case NODE_VARIABLE:
      if (code->pos >= BYTECODE_BUF_SZ - 3) {
        DebugLog(DF_OLC, DS_FATAL, "Bytecode buffer full\n");
        exit(EXIT_FAILURE);
      }
      code->bytecode[ code->pos++ ] = OP_PUSH;
      code->bytecode[ code->pos++ ] = expr->sym.res_id;
      break;

    case NODE_CONST:
      if (code->pos >= BYTECODE_BUF_SZ - 3) {
        DebugLog(DF_OLC, DS_FATAL, "Bytecode buffer full\n");
        exit(EXIT_FAILURE);
      }
      code->bytecode[ code->pos++ ] = OP_PUSHSTATIC;
      code->bytecode[ code->pos++ ] = expr->sym.res_id;
      break;

    case NODE_CALL:
      if (expr->call.paramlist)
        if (expr->call.paramlist->params_nb)
          for (i = expr->call.paramlist->params_nb - 1; i >= 0; --i)
            compile_bytecode_sub(expr->call.paramlist->params[i], code);
      if (code->pos >= BYTECODE_BUF_SZ - 2) {
        DebugLog(DF_OLC, DS_FATAL, "Bytecode buffer full\n");
        exit(EXIT_FAILURE);
      }
      code->bytecode[ code->pos++ ] = OP_CALL;
      code->bytecode[ code->pos++ ] = expr->call.res_id;
      break;

    case NODE_REGSPLIT:
      /* size of a regsplit compiled code: 2 PUSH 1 REGSPLIT n POPs
       * = 5 + 2n */
      if (code->pos >= (BYTECODE_BUF_SZ -
                        (5 + 2 * expr->regsplit.dest_vars->vars_nb))) {
        DebugLog(DF_OLC, DS_FATAL, "Bytecode buffer full\n");
        exit(EXIT_FAILURE);
      }

      /* push right string (CONT, VAR or FIELD) */
      switch (expr->regsplit.string->type) {
      case NODE_CONST:
        code->bytecode[ code->pos++ ] = OP_PUSHSTATIC;
        break;

      case NODE_VARIABLE:
        code->bytecode[ code->pos++ ] = OP_PUSH;
        break;

      case NODE_FIELD:
        code->bytecode[ code->pos++ ] = OP_PUSHFIELD;
        code->flags |= BYTECODE_HAVE_PUSHFIELD;
        break;

      default:
        DPRINTF( ("OUCH\n") );
      }
      code->bytecode[ code->pos++ ] = expr->regsplit.string->sym.res_id;

      /* XXX actually, split string can only be in static env */
      code->bytecode[ code->pos++ ] = OP_PUSHSTATIC;
      code->bytecode[ code->pos++ ] = expr->regsplit.split_pat->sym.res_id;

      /* the regsplit instruction itself */
      code->bytecode[ code->pos++ ] = OP_REGSPLIT;

      /* POPs instructions to associate value to variables */
      for (i = 0; i < expr->regsplit.dest_vars->vars_nb; i++) {
        code->bytecode[ code->pos++ ] = OP_POP;
        code->bytecode[ code->pos++ ] = expr->regsplit.dest_vars->vars[i]->sym.res_id;
      }
      break;

    default:
      DPRINTF( ("unknown node type (%i)\n", expr->type) );
    }
  }
}


/**
 * Compile a transition list node abstract syntax tree.
 * @param ctx Rule compiler context.
 * @param rule Current rule in compilation.
 * @param state Current state in compilation.
 * @param translist Transition list node abstract syntax tree root to compile.
 **/
static void
compile_transitions_ast(rule_compiler_t  *ctx,
                        rule_t           *rule,
                        state_t          *state,
                        node_translist_t *translist)
{
  int i;

  DebugLog(DF_OLC, DS_INFO,
           "compiling transitions in state \"%s\" in rule \"%s\"\n",
           state->name, rule->name);

  if (translist) {
    state->trans_nb = translist->trans_nb;
    state->trans = Xzmalloc(state->trans_nb * sizeof (transition_t));

    for (i = 0; i < translist->trans_nb; i++) {
      rule->trans_nb++; /* update rule stats */
      state->trans[i].id = i; /* set trans id */

      DebugLog(DF_OLC, DS_DEBUG, "transition %i: \n", i);
      if (translist->trans[i]->cond) {
        if (translist->trans[i]->sub_state_dest) {
          /* Conditional undirect transition */
          DebugLog(DF_OLC, DS_TRACE, "Undirect transition (substates).\n");
        }
        else {
          /* Conditional direct transition (general case) */
          node_state_t *s;
          s = strhash_get(ctx->statenames_hash,
                          translist->trans[i]->dest);
          if (s) {
            DebugLog(DF_OLC, DS_TRACE,
                     "resolve dest %s %i\n", s->name, s->state_id);
            state->trans[i].dest = &rule->state[ s->state_id ];
          }
          else {
            DebugLog(DF_OLC, DS_FATAL,
                     "undefined state reference '%s'\n",
                     translist->trans[i]->dest);
            exit (EXIT_FAILURE);
          }
        }

        compile_bytecode(translist->trans[i]->cond, &state->trans[i]);

      }
      else {
        node_state_t *s;

        /* Unconditional transition */
        DebugLog(DF_OLC, DS_TRACE,
                 "Unconditional transition to [%s]\n",
                 translist->trans[i]->dest);
        s = strhash_get(ctx->statenames_hash, translist->trans[i]->dest);
        if (s) {
          DebugLog(DF_OLC, DS_TRACE,
                   "resolve dest %s %i\n", s->name, s->state_id);
          state->trans[i].dest = &rule->state[ s->state_id ];
        }
        else {
          DebugLog(DF_OLC, DS_FATAL,
                   "undefined state reference '%s'\n",
                   translist->trans[i]->dest);
          exit (EXIT_FAILURE);
        }
      }
    }
  }
  else {
    /* Terminal state */
    DebugLog(DF_OLC, DS_TRACE,
             "state \"%s\" have no transition (TERMINAL STATE)\n",
             state->name);
  }
}


/**
 * Reset a compiler context.
 * The compiler context need to be reseted for each new rule.
 * @param ctx Rule compiler context to reset.
 **/
void
compiler_reset(rule_compiler_t *ctx)
{
  DebugLog(DF_OLC, DS_DEBUG,
           "*** compiler reset (clear rule specific hashes) ***\n");

  clear_strhash(ctx->statenames_hash, NULL);
  clear_strhash(ctx->rule_env, NULL);
  ctx->statics_nb = 0;
  ctx->dyn_var_name_nb = 0;
}


/**
 * Display the current rule environment in the compiler context.
 * @param fp Output stream.
 * @param ctx Rule compiler context.
 **/
void
fprintf_rule_environment(FILE *fp, rule_compiler_t *ctx)
{
  int i;

  fprintf(fp, "--- rule environment ---\n");
  fprintf(fp, "  static resources:\n");
  for (i = 0; i < ctx->statics_nb; i++) {
    fprintf(fp, "res_id: %3i: ", i);
    fprintf_issdl_val(fp, ctx->statics[i]);
  }
  fprintf(fp, "  dynamic environment size : %i\n", ctx->rule_env->elmts);
}


/**
 * fprintf_expr() sub-routine : display a terminal expression node.
 * @param fp Output stream.
 * @param expr A node to display.
 **/
static void
fprintf_term_expr(FILE *fp, node_expr_t *expr)
{
  int i;

  switch (expr->type)
    {
    case NODE_FIELD:
      fprintf(fp, "push field \"%s\" (fid %i)\n",
              expr->sym.name, expr->sym.res_id);
      break;

    case NODE_VARIABLE:
      fprintf(fp, "push variable \"%s\"\n", expr->sym.name);
      break;

    case NODE_CONST:
      switch (TYPE(expr->term.data))
        {
        case T_INT:
          fprintf(fp, "push integer %li (res_id %i)\n",
                  INT(expr->term.data),
                  expr->term.res_id);
          break;

        case T_STR:
          fprintf(fp, "push string \"%s\" (res_id %i)\n",
                  STR(expr->term.data),
                  expr->term.res_id);
          break;

        default:
          fprintf(fp, "push something of type %u\n", TYPE(expr->term.data));
        }
      break;

    case NODE_CALL:
      if (expr->call.paramlist)
        if (expr->call.paramlist->params_nb)
          for (i = expr->call.paramlist->params_nb - 1; i >= 0; --i)
            fprintf_expr(fp, expr->call.paramlist->params[i]);
      fprintf(fp, "call %s\n", expr->call.symbol);
      break;

    default:
      fprintf(fp, "unknown node type (%i)\n", expr->type);
    }
}


/**
 * Display a expression tree in a postfix order.
 * @param fp Output stream.
 * @param expr An expression tree to display.
 **/
void
fprintf_expr(FILE *fp, node_expr_t *expr)
{
  if (expr->type == NODE_BINOP) {
    fprintf_expr(fp, expr->bin.lval);
    fprintf_expr(fp, expr->bin.rval);
    fprintf(fp, "opcode %s (%i)\n",
            get_opcode_name(expr->bin.op), expr->bin.op);
  }
  else
    fprintf_term_expr(fp, expr);
}

static void
fprintf_term_expr_infix(FILE *fp, node_expr_t *expr)
{

  switch (expr->type)
    {
    case NODE_FIELD:
      fprintf(fp, ".%s ", expr->sym.name);
      break;

    case NODE_VARIABLE:
      fprintf(fp, "$%s ", expr->sym.name);
      break;

    case NODE_CONST:
      switch (TYPE(expr->term.data))
        {
        case T_INT:
          fprintf(fp, "%li ", INT(expr->term.data));
          break;

        case T_STR:
          fprintf(fp, "\"%s\" ",STR(expr->term.data));
          break;

        default:
          fprintf(fp, "data type %u\n", TYPE(expr->term.data));
        }
      break;

    case NODE_CALL:
      fprintf(fp, "%s() ", expr->call.symbol);
      break;

    default:
      fprintf(fp, "unknown node type (%i)\n", expr->type);
    }
}


void
fprintf_expr_infix(FILE *fp, node_expr_t *expr)
{
  if (expr->type == NODE_BINOP) {
    fprintf(fp, "( ");
    fprintf_expr_infix(fp, expr->bin.lval);
    switch (expr->bin.op) {

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
    }
    fprintf_expr_infix(fp, expr->bin.rval);
    fprintf(fp, ") ");
  }
  else
    fprintf_term_expr_infix(fp, expr);
}


/**
 * Display rule informations.
 * Given informations are :
 * Rule identifier (rid),
 * Rule name,
 * Number of state,
 * Number of transition,
 * Static environment size (sta e),
 * Dynamic environment size (dyn e),
 * Active instances of this rule (activ) and
 * Source file name.
 * @param ctx Orchids context.
 * @param fp Output stream.
 **/
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
