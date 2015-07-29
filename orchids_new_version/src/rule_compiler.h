/**
 ** @file rule_compiler.h
 ** Compiler specific data structures.
 ** These structures are only required during the compilation time.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup compiler
 **
 ** @date  Started on: Mon Mar  3 11:20:53 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

/**
 ** @defgroup compiler Orchids Rule Compiler
 **/

#ifndef RULE_COMPILER_H
#define RULE_COMPILER_H

#include "orchids.h"

#define NODE_UNKNOWN  0
#define NODE_FIELD    1
#define NODE_VARIABLE 2
#define NODE_CONST    3
#define NODE_CALL     4
#define NODE_BINOP    5
#define NODE_MONOP    6
#define NODE_REGSPLIT 7
#define NODE_IFSTMT   8
#define NODE_ASSOC    9
#define NODE_COND     10
#define NODE_CONS     11
#define NODE_EVENT    12
#define NODE_DB_PATTERN 13
#define NODE_DB_COLLECT 14
#define NODE_DB_SINGLETON 15
#define NODE_RETURN 16
#define NODE_BREAK 17

#define EXIT_IF_BYTECODE_BUFF_FULL(x)		\
  do { \
  if (code->pos >= BYTECODE_BUF_SZ - (x)) {		       \
        DebugLog(DF_OLC, DS_FATAL, "Byte code buffer full\n"); \
        exit(EXIT_FAILURE); }				       \
  } while (0); \

/*
** node_* structure represents nodes in the abstract syntax tree (AST)
** AST is build with a lex/yacc parser (ascending building)
** and then, AST is proceeded (in a descending way) to build rules
*/

typedef struct node_call_s node_call_t;
typedef struct node_expr_s node_expr_t;
typedef struct node_trans_s node_trans_t;
typedef struct node_state_s node_state_t;
typedef struct node_rule_s node_rule_t;
typedef struct symbol_token_s symbol_token_t;
typedef struct node_varlist_s node_varlist_t;
typedef struct node_syncvarlist_s node_syncvarlist_t;
typedef struct node_expr_if_s node_expr_if_t;

#define NODE_EXPR_S							\
  gc_header_t gc;							\
  int   type; /* node type: NODE_FIELD, or NODE_VARIABLE, or ... */	\
  ovm_var_t *file; /* really a NUL-terminated ovm_str_t * */		\
  unsigned int lineno;							\
  unsigned long hash;							\
  type_t *stype; /* static type, used in static type checking (&t_int,	\
		    &t_str, etc.) */					\
  void (*compute_stype) (rule_compiler_t *ctx, node_expr_t *myself);	\
  node_expr_t *parents; /* list of nodes of which this node is an	\
			   argument, for type checking */

/**
 ** @struct node_expr_bin_s
 **   Binary expression.
 **/
/**   @var node_expr_bin_s::type
 **     Expression type: NODE_BINOP.
 **/
/**   @var node_expr_bin_s::op
 **     Operator.
 **/
/**   @var node_expr_bin_s::lval
 **     Left value.
 **/
/**   @var node_expr_bin_s::rval
 **     Right value.
 **/
typedef struct node_expr_bin_s node_expr_bin_t;
struct node_expr_bin_s
{
  NODE_EXPR_S
  int          op;
  node_expr_t *lval;
  node_expr_t *rval;
};

#define BIN_OP(e) ((node_expr_bin_t *)e)->op
#define BIN_LVAL(e) ((node_expr_bin_t *)e)->lval
#define BIN_RVAL(e) ((node_expr_bin_t *)e)->rval

/**
 ** @struct node_expr_mon_s
 **   Unary expression.
 **/
/**   @var node_expr_bin_s::type
 **     Expression type: NODE_MONOP.
 **/
/**   @var node_expr_bin_s::op
 **     Operator.
 **/
/**   @var node_expr_bin_s::val
 **     Argument value.
 **/
typedef struct node_expr_mon_s node_expr_mon_t;
struct node_expr_mon_s
{
  NODE_EXPR_S
  int          op;
  node_expr_t *val;
};

#define MON_OP(e) ((node_expr_mon_t *)e)->op
#define MON_VAL(e) ((node_expr_mon_t *)e)->val


/**
 ** @struct node_expr_call_s
 **   Function call node.
 **/
/**   @var node_expr_call_s::type
 **     Expression type: NODE_CALL.
 **/
/**   @var node_expr_call_s::symbol
 **     Symbol (function) name.
 **/
/**   @var node_expr_call_s::res_id
 **     Resource identifier (function id).
 **/
/**   @var node_expr_call_s::paramlist
 **     Parameter List.
 **/
typedef struct node_expr_call_s node_expr_call_t;
struct node_expr_call_s
{
  NODE_EXPR_S
  char             *symbol;
  issdl_function_t *f;
  node_expr_t      *paramlist;
};

#define CALL_SYM(e) ((node_expr_call_t *)e)->symbol
#define CALL_RES_ID(e) (((node_expr_call_t *)e)->f==NULL?0:((node_expr_call_t *)e)->f->id)
#define CALL_COMPUTE_MONOTONY(e) (((node_expr_call_t *)e)->f==NULL?NULL:((node_expr_call_t *)e)->f->cm)
#define CALL_PARAMS(e) ((node_expr_call_t *)e)->paramlist



/* Reverse a NODE_CONS based, NULL-terminated list in place. */
node_expr_t *expr_cons_reverse(rule_compiler_t *ctx, node_expr_t *l);


/**
 ** @struct node_expr_symbol_s
 **   Symbol node (usually variable)
 **/
/**   @var node_expr_symbol_s::type
 **     Expression type: NODE_VARIABLE.
 **/
/**   @var node_expr_symbol_s::res_id
 **     Variable identifier (id in environment, or in fields table).
 **/
/**   @var node_expr_symbol_s::mono
 **     Monotonicity info (for fields)
 **/
/**   @var node_expr_symbol_s::name
 **     Variable name.
 **/
typedef struct node_expr_symbol_s node_expr_symbol_t;
struct node_expr_symbol_s
{
  NODE_EXPR_S
  int   res_id;
  char *name;
  node_expr_t *def; /* for variables, gives the assignment that determined
		       the variable's type, if any. */
  monotony mono; /* for field names */
};

#define SYM_RES_ID(e) ((node_expr_symbol_t *)e)->res_id
#define SYM_MONO(e) ((node_expr_symbol_t *)e)->mono
#define SYM_NAME(e) ((node_expr_symbol_t *)e)->name
#define SYM_DEF(e) ((node_expr_symbol_t *)e)->def


/**
 ** @struct node_expr_term_s
 **   Terminal node (in our case, static values, integers and strings).
 **/
/**   @var node_expr_term_s::type
 **     Expression type: NODE_TERM
 **/
/**   @var node_expr_term_s::res_id
 **     Resource id in the current static rule environment.
 **/
/**   @var node_expr_term_s::data
 **     Associated data.
 **/
typedef struct node_expr_term_s node_expr_term_t;
struct node_expr_term_s
{
  NODE_EXPR_S
  int   res_id;
  ovm_var_t *data;
};

#define TERM_RES_ID(e) ((node_expr_term_t *)e)->res_id
#define TERM_DATA(e) ((node_expr_term_t *)e)->data

/**
 ** @struct node_expr_regsplit_s
 **   String Reg Split special instruction.
 **/
/**   @var node_expr_regsplit_s::type
 **     Expression type: NODE_REGSPLIT.
 **/
/**   @var node_expr_regsplit_s::string
 **     Source string to split (may be a variable or a static const).
 **/
/**   @var node_expr_regsplit_s::split_pat
 **     Split pattern (may be a variable or a static const).
 **/
/**   @var node_expr_regsplit_s::dest_vars
 **     Destination variable list.
 **/
typedef struct node_expr_regsplit_s node_expr_regsplit_t;
struct node_expr_regsplit_s
{
  NODE_EXPR_S
  node_expr_t    *string;
  node_expr_t    *split_pat;
  node_varlist_t *dest_vars;
};

#define REGSPLIT_STRING(e) ((node_expr_regsplit_t *)e)->string
#define REGSPLIT_PAT(e) ((node_expr_regsplit_t *)e)->split_pat
#define REGSPLIT_DEST_VARS(e) ((node_expr_regsplit_t *)e)->dest_vars

/**
 ** @struct node_expr_if_s
 ** If statement
 **/
/**   @var node_expr_if_s::cond
 **     Transition condition (NULL if it is an unconditional transition).
 **/
/**   @var node_expr_if_s::cond
 **     Statement condition
 **/
/**   @var node_expr_if_s::then
 **     Code to execute if cond true
 **/
/**   @var node_expr_if_s::els
 **     Code to exectute if cond false (can be NULL) if no "else" in the statement
 **/
struct node_expr_if_s
{
  NODE_EXPR_S
  node_expr_t  *cond;
  node_expr_t *then;
  node_expr_t *els;
};

#define IF_COND(e) ((node_expr_if_t *)e)->cond
#define IF_THEN(e) ((node_expr_if_t *)e)->then
#define IF_ELSE(e) ((node_expr_if_t *)e)->els

/**
 ** @union node_expr_u
 **   Generic expression node
 **/
/**   FIXME BACK TYPE
 **/
/**   @var node_expr_u::bin
 **     Binary expression node.
 **/
/**   @var node_expr_u::call
 **     Function call node.
 **/
/**   @var node_expr_u::sym
 **     Symbol (variable) expression node.
 **/
/**   @var node_expr_u::term
 **     Terminal (static int/string) expression node.
 **/
/**   @var node_expr_u::regsplit
 **     Regsplit special instruction.
 **/
struct node_expr_s
{
  NODE_EXPR_S
  /* in union with:
  node_expr_bin_t    bin;
  node_expr_bin_t    cond;
  node_expr_call_t   call;
  node_expr_symbol_t sym;
  node_expr_term_t   term;
  node_expr_regsplit_t regsplit;
  node_expr_if_t      	ifstmt;
  */
};

typedef struct varset_s varset_t; /* set of variable numbers;
				     garbage collectable */
#define VS_EMPTY NULL
#define VS_IS_EMPTY(vs) ((vs)==NULL)
varset_t *vs_one_element (gc_t *gc_ctx, int n);
varset_t *vs_union (gc_t *gc_ctx, varset_t *vs1, varset_t *vs2);
varset_t *vs_inter (gc_t *gc_ctx, varset_t *vs1, varset_t *vs2);
varset_t *vs_diff (gc_t *gc_ctx, varset_t *vs1, varset_t *vs2);
int vs_subset (gc_t *gc_ctx, varset_t *vs1, varset_t *vs2);
int vs_sweep (varset_t *vs, int (*p) (int var, void *data), void *data);

typedef struct node_expr_vars_s {
  varset_t *mayread;
  varset_t *mustset;
} node_expr_vars_t;

node_expr_vars_t node_expr_vars (gc_t *gc_ctx, node_expr_t *e);
/* returns set of variables that e may read in mayread,
   and set of variables that e is sure to set in mustset.

   To explain what 'may read' and 'sure to set' mean, consider
   the following examples:
   '$x && $y' may read $x, and may read $y, but
   will only read $x for certain;
   '$x = 3' is sure to set $x, but in
   'if ($x) { $y=0; $z=1; } else { $y= 1; }' only $y is sure to be set.
*/

int node_expr_equal (node_expr_t *e1, node_expr_t *e2);

/**
 ** @struct node_trans_s
 **   Transition (if (cond) goto somewhere;)
 **/
/**   @var node_trans_s::cond
 **     Transition condition (NULL if it is an unconditional transition).
 **/
/**   @var node_trans_s::dest
 **     Destination state name, if direct transition (NULL otherwise)
 **/
/**   @var node_trans_s::file
 **     File in which goto occurred, if direct transition (NULL otherwise)
 **/
/**   @var node_trans_s::lineno
 **     Line at which goto occurred, if direct transition (-1 otherwise)
 **/
/**   @var node_trans_s::sub_state_dest
 **     Sub-states (for nested states/actions).
 **/
struct node_trans_s
{
  gc_header_t  gc;
  int type; /* = -1 */
  node_expr_t  *cond;
  char         *dest;
  char         *file;
  unsigned int lineno;
  node_state_t *sub_state_dest;
  unsigned long an_flags;
#define AN_NONEXISTENT_TARGET_ALREADY_SAID 0x01
};


/**
 ** @struct node_state_s
 **   State declaration
 **/
/**   @var node_state_s::state_id
 **     State identifier. (in the current rule).
 **/
/**   @var node_state_s::line
 **     Line of declaration in (current) rule file.
 **/
/**   @var node_state_s::name
 **     State name.
 **/
/**   @var node_state_s::actionlist
 **     Action list.
 **/
/**   @var node_state_s::translist
 **     Transition list.
 **/
/**   @var node_state_s::flags
 **     Optional state flags.
 **/
/**   @var node_state_s::mustset
 **     Set of variables that are set (initialized) for sure
 **     at the beginning of this state.  Will be computed by
 **     static analyzer.
 **/
struct node_state_s
{
  gc_header_t        gc;
  int                state_id;
  char              *file;
  int                line;
  char              *name;
  node_expr_t *actionlist;
  node_expr_t  *translist;
  unsigned long      flags;
  unsigned long      an_flags;
#define AN_MUSTSET_REACHABLE 0x1
#define AN_EPSILON_REACHABLE 0x2
#define AN_EPSILON_DONE 0x4
#define AN_EXPLORED 0x8
  varset_t *mustset;
};


/**
 ** @struct node_rule_s
 ** Node_rule IS the root of the rule AST
 **/
/**   @var node_rule_s::line
 **     Line of declaration. (in the current rule file).
 **/
/**   @var node_rule_s::name
 **     Rule name.
 **/
/**   @var node_rule_s::init
 **     Initial state.
 **/
/**   @var node_rule_s::statelist
 **     Other state list.
 **/
struct node_rule_s
{
  gc_header_t         gc;
  int                 line;
  char               *file;
  char               *name;
  node_state_t       *init;
  node_expr_t   *statelist;
  node_syncvarlist_t *sync_vars;
};

/**
 ** @struct node_varlist_s
 ** Variable list dynamic array (used for string splitting)
 **/
/**   @var node_varlist_s::vars_nb
 **     Number of variables.
 **/
/**   @var node_varlist_s::size
 **     Allocated size.
 **/
/**   @var node_varlist_s::grow
 **     Grow size, for reallocation.
 **/
/**   @var node_varlist_s::vars
 **     Parameters.
 **/
struct node_varlist_s
{
  size_t        vars_nb;
  size_t        size;
  size_t        grow;
  node_expr_t **vars;
};


/**
 ** @struct node_syncvarlist_s
 **   Synchronization variable list dynamic array.
 **/
/**   @var node_syncvarlist_s::vars_nb
 **     Number of variables.
 **/
/**   @var node_syncvarlist_s::size
 **     Allocated size.
 **/
/**   @var node_syncvarlist_s::grow
 **     Grow size, for reallocation.
 **/
/**   @var node_syncvarlist_s::vars
 **     Variable names.
 **/
struct node_syncvarlist_s
{
  size_t        vars_nb;
  size_t        size;
  size_t        grow;
  char        **vars;
};


/**
 ** @struct symbol_token_s
 **   Symbol and its line of declaration in the current rule file.
 **   Used for reporting informations and errors.
 **/
/**   @var symbol_token_s::name
 **     Symbol name.
 **/
/**   @var symbol_token_s::file
 **     Declaration file.
 **/
/**   @var symbol_token_s::line
 **     Declaration line.
 **/
struct symbol_token_s
{
  char *name;
  char *file;
  int   line;
};

/**
 ** @struct label_s
 **   Labels are used during test expression compilation
 **   During compilation, each jump operator has a label to jump to
 **   The jumps value are resolved after the complete code compilation.
 **/
/**   @var label_s::nb
 **     Number of labels created
 **/
/**   @var label_s::labels
 **     Array of all labels positions
 **/
/**   @var label_s::toset_nb
 **     Number of jump offsets to resolve
 **/
/**   @var label_s::toset
 **     Array of code position where a jump offset has to be resolved
 **/
typedef struct	labels_s {
    unsigned int	labels_nb;
    unsigned int	*labels;
    unsigned int	labels_sz;
    unsigned int	toset_nb;
    unsigned int	*toset;
    unsigned int	toset_sz;
}		labels_t;

#define LABEL_GROW	256

/**
 ** @def NEW_LABEL(labels)
 **   Return a new label id
 **/
#define NEW_LABEL(l)  (l).labels_nb++;

/**
 ** @def SET_LABEL(ctx, labels, code, label)
 **   Set the label position at the current code position
 **/
#define SET_LABEL(ctx, l, pos, label)					\
  do {									\
    if ((l).labels_nb >= (l).labels_sz)					\
    {									\
      (l).labels = gc_base_realloc ((ctx)->gc_ctx, (l).labels, ((l).labels_sz + LABEL_GROW) * sizeof (unsigned int)); \
      (l).labels_sz += LABEL_GROW;					\
    }									\
    l.labels[label] = pos;} while (0)

/**
 ** @def PUT_LABEL(ctx, l, code, label)
 **   Add a label after a jmp operator. The jump offset will be
 **   resolved after compilation
 **/
#define PUT_LABEL(ctx, l, code, label)					\
  do {									\
    if ((l).toset_nb >= (l).toset_sz)					\
    {									\
      (l).toset = gc_base_realloc((ctx)->gc_ctx, (l).toset, ((l).toset_sz + LABEL_GROW) * sizeof (unsigned int)); \
      (l).toset_sz += LABEL_GROW;					\
    }									\
    (l).toset[(l).toset_nb++] = (code)->pos;				\
    (code)->bytecode[(code)->pos++] = label;				\
  } while(0)

/**
 ** @def INIT_LABELS(l)
 **   Initialize the labels_t structure
 **/
#define INIT_LABELS(ctx, l)						\
  do {									\
  (l).labels_nb = 0;							\
  (l).labels = gc_base_malloc((ctx)->gc_ctx, LABEL_GROW * sizeof (unsigned int)); \
  (l).labels_sz = LABEL_GROW;						\
  (l).toset_nb = 0;							\
  (l).toset = gc_base_malloc((ctx)->gc_ctx, LABEL_GROW * sizeof (unsigned int)); \
  (l).toset_sz = LABEL_GROW;						\
  /* LABEL 0 => JMP (0) */						\
  (l).labels[0] = 0;							\
  (l).toset[0] = 0;							\
  NEW_LABEL(l);							\
  } while(0)

#define FREE_LABELS(l)				\
  do {						\
  gc_base_free ((l).labels);			\
  gc_base_free ((l).toset);			\
  } while (0)

/**
 ** @struct bytecode_buffer_s
 ** Used for converting list of actions into an OVM byte code sequence
 **/
/**   @var bytecode_buffer_s::pos
 **     Position in the buffer.
 **/
/**   @var bytecode_buffer_s::bytecode
 **     Byte code buffer.
 **/
/**   @var bytecode_buffer_s::used_fields_sz
 **     Number of ints in used_fields bit array.
 **/
/**   @var bytecode_buffer_s::used_fields
 **     Used field bit array.
 **/
/**   @var bytecode_buffer_s::stack_level
 **     Current stack depth
 **/
/**   @var bytecode_buffer_s::logicals
 **     Resizable array, mapping each (logical) variable to the
 **     offset at which it can be found in the stack, or -1 if not
 **/

#define BYTECODE_BUF_SZ 16384
#define MAX_FIELD_SZ 256
typedef struct bytecode_buffer_s bytecode_buffer_t;
struct bytecode_buffer_s
{
  size_t pos;
  bytecode_t bytecode[BYTECODE_BUF_SZ];
  size_t used_fields_sz;
  int used_fields[MAX_FIELD_SZ];
  unsigned long flags;
  labels_t	labels;
  int stack_level;
  size_t logicals_size;
  int *logicals;
  rule_compiler_t *ctx;
};


/**
 ** @struct filestack_s
 **   The file stack structure for handling line control directives
 **   (@#line directive) generated by preprocessors to deal with
 **   file inclusions.  This stack is implemented as a simply linked
 **   list.  This structure represent exactly one element of the stack.
 **/
/** @var filestack_s::file
 **   The file name in the current level of inclusion.
 **/
/** @var filestack_s::line
 **   The line number in the current level of inclusion.
 **/
/** @var filestack_s::elmt
 **   This linked list structure.
 **/
typedef struct filestack_s filestack_t;
struct filestack_s
{
  char *file;
  int line;
  SLIST_ENTRY(filestack_t) elmt;
};


/*----------------------------------------------------------------------------*
** functions prototypes                                                      **
*----------------------------------------------------------------------------*/


/**
 ** Rule compiler context constructor.
 **
 ** @return A compiler context initialised with default values.
 **/
rule_compiler_t * new_rule_compiler_ctx(gc_t *gc_ctx);


/**
 ** Compile all rule files included in the configuration file.
 **
 ** @param ctx Orchids application context.
 **/
void
compile_rules(orchids_t *ctx);

#define COMPILE_GC_DEPTH(ctx) ((ctx)->nprotected)
void compile_gc_protect_drop (rule_compiler_t *ctx, gc_header_t *p,
			      size_t old_nprotected);
void compile_gc_protect (rule_compiler_t *ctx, gc_header_t *p);

/**
 * Build a rule node.
 * @param  sym          The rule name (symbol).
 * @param  init_state   The initial state of the rule.
 * @param  states       Additional state list.
 * @param  sync_vars   Synchronization variable list.
 * @return  A new allocated rule node.
 **/
node_rule_t *build_rule(rule_compiler_t *ctx,
			symbol_token_t *sym,
			node_state_t *init_state,
			node_expr_t *states,
			node_syncvarlist_t *sync_vars);


/**
 * Build a state node from an actionlist node and a transitionlist node.
 * @param actions Action list node.
 * @param transitions Transitions list node.
 * @return A new state node.
 **/
node_state_t *build_state(rule_compiler_t *ctx,
			  node_expr_t  *actions,
			  node_expr_t *transitions);


/**
 * Set a state node name (or label) to a given symbol.
 * @param ctx Rule compiler context.
 * @param state The state to label.
 * @param sym Symbol (name and line) to associate to the state node.
 * @param flags Optional state flags.
 * @return The labeled state (NOT REALLOCATED !)
 **/
node_state_t *
set_state_label(rule_compiler_t *ctx,
                node_state_t *state,
                symbol_token_t *sym,
                unsigned long flags);


/**
 * Build a direct transition node.
 * A direct transition is "if (cond) goto somewhere".
 * @param cond The condition expression of the transition.
 * @param sym The name string of the destination state, with location.
 * @return The new transition node.
 **/
node_trans_t *build_direct_transition(rule_compiler_t *ctx,
				      node_expr_t *cond,
				      symbol_token_t *sym);

/**
 * Build a if statement node.
 * @param cond The condition expression.
 * @param then Action code to execute if cond true
 * @param els Action code to execute if cond false
 * @return The new if statement node.
 **/
node_expr_t *build_expr_ifstmt(rule_compiler_t *ctx,
			       node_expr_t *cond, node_expr_t *then,
			       node_expr_t *els);


/**
 * Build indirect transition.
 * An indirect transition is "if (cond) { do something }".
 * @param cond The condition expression of the transition.
 * @param substate The nested sub-state block.
 * @return The new transition node.
 **/
node_trans_t *build_indirect_transition(rule_compiler_t *ctx,
					node_expr_t *cond,
					node_state_t *substate);


/**
 * Build an unconditional transition node.
 * @param dest The name string of the destination state.
 * @return The new unconditional transition node.
 **/
/*
node_expr_t *build_unconditional_transition(rule_compiler_t *ctx,
					    char *dest);
*/

/**
 * Build a binary expression node.
 * @param op The operator identifier.
 * @param left_node The left part of the binary expression.
 * @param right_node The right part of the binary expression.
 * @return A new allocated expression node.
 **/
node_expr_t *build_expr_binop(rule_compiler_t *ctx, int op,
			      node_expr_t *left_node, node_expr_t *right_node);

/**
 * Build a unary expression node.
 * @param op The operator identifier.
 * @param arg_node The argument of the unary expression.
 * @return A new allocated expression node.
 **/
node_expr_t *build_expr_monop(rule_compiler_t *ctx, int op,
			      node_expr_t *arg_node);

/**
 * Build a return expression node.
 * @param arg_node The argument of the return expression.
 * @return A new allocated expression node.
 **/
node_expr_t *build_expr_return(rule_compiler_t *ctx,
			       node_expr_t *arg_node);

/**
 * Build a break expression node.
 * @return A new allocated expression node.
 **/
node_expr_t *build_expr_break(rule_compiler_t *ctx);

/**
 * Build a cons node.
 * @param left_node The left part of the binary expression.
 * @param right_node The right part of the binary expression.
 * @return A new allocated expression node.
 **/
node_expr_t *build_expr_cons(rule_compiler_t *ctx,
			      node_expr_t *left_node, node_expr_t *right_node);

node_expr_t *build_expr_action(rule_compiler_t *ctx,
			       node_expr_t *action, node_expr_t *rest);

/**
 * Build a logic expression node.
 * @param op The operator identifier.
 * @param left_node The left part of the binary expression.
 * @param right_node The right part of the binary expression.
 * @return A new allocated expression node.
 **/
node_expr_t *build_expr_cond(rule_compiler_t *ctx, int op,
			     node_expr_t *left_node, node_expr_t *right_node);


/**
 * Build an association (affectation).
 * @param ctx Rule compiler context. Needed to resolve the variable name.
 * @param var The variable (left value).
 * @param expr The expression to associate to the variable (right value).
 * @return A new allocated expression node.
 **/
node_expr_t *build_assoc(rule_compiler_t *ctx, node_expr_t *var,
			 node_expr_t *expr);


/**
 * Build a function call node.
 * @param ctx Rule compiler context. Needed to resolve function name.
 * @param sym The function name (symbol) to call.
 * @param paramlist The parameter list for this function.
 **/
node_expr_t *build_function_call(rule_compiler_t *ctx,
				 symbol_token_t *sym,
				 node_expr_t *paramlist);


/**
 * Build a fieldname (of a module) expression node.
 * @param ctx Rule compiler context. Needed for field name resolution.
 * @param fieldname A registered module field name.
 * @return A new allocated expression node.
 **/
node_expr_t *build_fieldname(rule_compiler_t *ctx, char *fieldname);


/**
 * Build a variable name expression node.
 * @param ctx Rule compiler context. Needed to resolve/add
 *   variable to the global symbol table.
 * @param varname The variable name.
 * @return A new allocated variable name expression node.
 **/
node_expr_t *
build_varname(rule_compiler_t *ctx, char *varname);


/**
 * Build an integer expression node.
 * @param ctx Rule compiler context.
 * @param i The integer value.
 * @return A new allocated integer expression node.
 **/
node_expr_t *build_integer(rule_compiler_t *ctx, unsigned long i);


/**
 * Build a floating-point expression node.
 * @param ctx  Rule compiler context.
 * @param f    The double value.
 * @return A new allocated floating-point expression node.
 **/
node_expr_t *
build_double(rule_compiler_t *ctx, double f);


/**
 * Build a string expression node.
 * @param ctx Rule compiler context.
 * @param str The string value.
 * @return A new allocated string expression node.
 **/
node_expr_t *
build_string(rule_compiler_t *ctx, char *str);

/**
 * Build a db_nothing expression node.
 * @param ctx Rule compiler context.
 * @return A new allocated integer expression node.
 **/
node_expr_t *build_db_nothing(rule_compiler_t *ctx);

/**
 * Build an event expression node.
 * @param ctx Rule compiler context.
 * @param op The operator identifier.
 * @param left_node The expression to add fields to, or NULL
 * @param right_node The NODE_CONS based list of OP_ADD_EVENT binop nodes
 * @return A new allocated expression node.
 **/
node_expr_t *build_expr_event(rule_compiler_t *ctx, int op,
			      node_expr_t *left_node, node_expr_t *right_node);

/**
 * Build a database pattern specification node.
 * @param ctx Rule compiler context.
 * @param tuple The linked list of all tuple arguments
 * @param db The expression that denotes the database we must sweep through
 **/
node_expr_t *build_db_pattern(rule_compiler_t *ctx, node_expr_t *tuple, node_expr_t *db);

/**
 * Build a database comprehension expression.
 * @param ctx Rule compiler context.
 * @param actions The list of actions to be done for each match
 * @param collect The expression denoting databases whose union we will finally take
 * @param returns The list of return statements inside actions
 * @param patterns The db_patterns describing the comprehension
 **/
node_expr_t *build_db_collect(rule_compiler_t *ctx,
			      node_expr_t *actions,
			      node_expr_t *collect,
			      node_expr_t *returns,
			      node_expr_t *patterns);

/**
 * Build a singleton expression
 * @param ctx Rule compiler context.
 * @param tuple The linked list of all tuple arguments
 **/
node_expr_t *build_db_singleton(rule_compiler_t *ctx, node_expr_t *tuple);

/**
 * Create a sub-state name for nested anonymous states.
 * state => state_0, state_1, etc...
 * @param refname Reference name.
 * @param n Number to append.
 * @return An allocated string buffer containing the new name.
 **/
char *
build_substate_name(const char *refname, int n);

node_expr_t *build_ctime_from_int(rule_compiler_t *ctx, time_t ctime);
node_expr_t *build_ctime_from_string(rule_compiler_t *ctx, char *date);


/**
 * Build a constant ipv4addr expression node.
 * @param ctx Rule compiler context.
 * @param hostname The hostname string.
 * @return A new allocated ipv4addr expression node.
 **/
node_expr_t *
build_ipv4(rule_compiler_t *ctx, char *hostname);


/**
 * Build a constant ipv6addr expression node.
 * @param ctx Rule compiler context.
 * @param hostname The hostname string.
 * @return A new allocated ipv6addr expression node.
 **/
node_expr_t *
build_ipv6(rule_compiler_t *ctx, char *hostname);

/**
 * Build a constant ipv4addr or ipv6addr expression node.
 * @param ctx Rule compiler context.
 * @param hostname The hostname string.
 * @return A new allocated ipv4addr or ipv6addr expression node.
 **/
node_expr_t *
build_ip(rule_compiler_t *ctx, char *hostname);

node_expr_t *
build_timeval_from_int(rule_compiler_t *ctx, long sec, long usec);


node_expr_t *
build_timeval_from_string(rule_compiler_t *ctx, char *date, long tv_usec);


#ifdef COUNTER_OBSOLETE
node_expr_t *
build_counter(rule_compiler_t *ctx, long value);
#endif


node_expr_t *
build_regex(rule_compiler_t *ctx, char *regex_str);


node_expr_t *build_split_regex(rule_compiler_t *ctx, char *regex_str);


/**
 * Build a variable list node.
 * @param first_param The first variable to add to the list.
 * @return A new allocated variable list node.
 **/
node_varlist_t *build_varlist(rule_compiler_t *ctx, node_expr_t *first_param);


/**
 * Add a variable to a parameter list.
 * @param list The variable list node.
 * @param expr The expression node to add to the variable list node.
 **/
void varlist_add(rule_compiler_t *ctx, node_varlist_t *list, node_expr_t *expr);


/**
 * Build a synchronization variable list node.
 * @param first_syncvar  The first variable to add to the list.
 * @return A new allocated variable list node.
 **/
node_syncvarlist_t *build_syncvarlist(rule_compiler_t *ctx, char *first_syncvar);


/**
 * Add a synchronization variable to a parameter list.
 * @param list The variable list node.
 * @param syncvar The synchronization variable to add to the list node.
 **/
node_syncvarlist_t *syncvarlist_add(rule_compiler_t *ctx, node_syncvarlist_t *list, char *syncvar);


node_expr_t *build_string_split(rule_compiler_t *ctx,
				node_expr_t *source,
				node_expr_t *pattern,
				node_varlist_t *dest_list);


/**
 * Final phase of rule compilation.
 * - all symbols should by added in entry table
 * - this phase resolve all symbols, create environments and generate byte code
 * @param ctx Rule compiler context.
 * @param node_rule The rule node (abstract syntax tree root) to compile.
 * @callgraph
 **/
void
compile_and_add_rule_ast(rule_compiler_t *ctx, node_rule_t *node_rule);


/**
 * Reset a compiler context.
 * The compiler context need to be reseted for each new rule.
 * @param ctx Rule compiler context to reset.
 **/
void
compiler_reset(rule_compiler_t *ctx);


/**
 * Concatenate the second string into the first.  The first string
 * is reallocated to the correct size.  The second string is freed.
 * @param str1 First string.
 * @param str2 Second string.
 **/
char *build_concat_string(rule_compiler_t *ctx, char *str1, char *str2);


/* DEBUG */
/**
 * Display a expression tree in a postfix order.
 * @param fp Output stream.
 * @param expr An expression tree to display.
 **/
void
fprintf_expr(FILE *fp, node_expr_t *expr);


void
fprintf_expr_infix(FILE *fp, node_expr_t *expr);


/**
 * Display the current rule environment in the compiler context.
 * @param fp Output stream.
 * @param ctx Global Orchids context.
 **/
void
fprintf_rule_environment(FILE *fp, orchids_t *ctx);


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
fprintf_rule_stats(const orchids_t *ctx, FILE *fp);


#endif /* RULE_COMPILER_H */



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
