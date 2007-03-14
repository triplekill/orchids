/**
 ** @file rule_compiler.h
 ** Compiler specific data structures.
 ** Theses structures are only required during the compilation time.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup compiler
 **
 ** @date  Started on: Mon Mar  3 11:20:53 2003
 ** @date Last update: Tue Nov 29 11:16:54 2005
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
#define NODE_REGSPLIT 6

/*
** node_* structure represents nodes in the abstract syntax tree (AST)
** AST is build with a lex/yacc parser (ascending building)
** and then, AST is proceeded (in a descending way) to build rules
*/

typedef struct node_call_s node_call_t;
typedef union node_expr_u node_expr_t;
typedef struct node_trans_s node_trans_t;
typedef struct node_translist_s node_translist_t;
typedef struct node_state_s node_state_t;
typedef struct node_statelist_s node_statelist_t;
typedef union node_expr_u node_action_t;
typedef struct node_actionlist_s node_actionlist_t;
typedef struct node_rule_s node_rule_t;
typedef struct node_paramlist_s node_paramlist_t;
typedef struct symbol_token_s symbol_token_t;
typedef struct node_varlist_s node_varlist_t;
typedef struct node_syncvarlist_s node_syncvarlist_t;


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
  int          type;
  int          op;
  node_expr_t *lval;
  node_expr_t *rval;
};


/**
 ** @struct node_expr_call_s
 **   Function call node.
 **/
/**   @var node_expr_call_s::type
 **     Expr. type: NODE_CALL.
 **/
/**   @var node_expr_call_s::symbol
 **     Symbol (function) name.
 **/
/**   @var node_expr_call_s::res_id
 **     Resource identifier (function id).
 **/
/**   @var node_expr_call_s::paramlist
 **     Paramater List
 **/
typedef struct node_expr_call_s node_expr_call_t;
struct node_expr_call_s
{
  int               type;
  char             *symbol;
  int               res_id;
  node_paramlist_t *paramlist;
};


/**
 ** @struct node_expr_symbol_s
 **   Symbol node (usually variable)
 **/
/**   @var node_expr_symbol_s::type
 **     Expr. type: NODE_VARIABLE.
 **/
/**   @var node_expr_symbol_s::res_id
 **     Variable identifier (id in environment).
 **/
/**   @var node_expr_symbol_s::name
 **     Variable name.
 **/
typedef struct node_expr_symbol_s node_expr_symbol_t;
struct node_expr_symbol_s
{
  int   type;
  int   res_id;
  char *name;
};


/**
 ** @struct node_expr_term_s
 **   Terminal node (in our case, static values, integers and strings).
 **/
/**   @var node_expr_term_s::type
 **     Expr. type: NODE_TERM                                                
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
  int   type;
  int   res_id;
  void *data;
};


/**
 ** @struct node_expr_regsplit_s
 **   String Reg Split special instruction.
 **/
/**   @var node_expr_regsplit_s::type
 **     Expr. type: NODE_REGSPLIT.
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
  int             type;
  node_expr_t    *string;
  node_expr_t    *split_pat;
  node_varlist_t *dest_vars;
};


/**
 ** @union node_expr_u
 **   Generic expression node
 **/
/**   @var node_expr_u::type
 **     Expression type.
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
union node_expr_u
{
  int                type;
  node_expr_bin_t    bin;
  node_expr_call_t   call;
  node_expr_symbol_t sym;
  node_expr_term_t   term;
  node_expr_regsplit_t regsplit;
};

/**
 ** @struct node_trans_s
 **   Transition (if (cond) goto somewhere;)
 **/
/**   @var node_trans_s::cond
 **     Transision condition (NULL if it is an unconditional transition).
 **/
/**   @var node_trans_s::dest
 **     Destination state name.
 **/
/**   @var node_trans_s::sub_state_dest
 **     Sub-states (for nested states/actions).
 **/
struct node_trans_s
{
  node_expr_t  *cond;
  char         *dest;
  node_state_t *sub_state_dest;
};


/**
 ** @struct node_state_s
 **   State declaration
 **/
/**   @var node_state_s::state_id
 **     State identifier. (in the current rule).
 **/
/**   @var node_state_s::line
 **     Line of declaration in (current) rulefile.
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
 **     Optional state flags..
 **/
struct node_state_s
{
  int                state_id;
  int                line;
  char              *name;
  node_actionlist_t *actionlist;
  node_translist_t  *translist;
  unsigned long      flags;
};

/**
 ** @struct node_action_s
 **   An action IS an expression :-)
 **/
struct node_action_s
{
  node_expr_t *expr;
};

/**
 ** @struct node_actionlist_s
 **   Actionlist dynamic array
 **/
/**   @var node_actionlist_s::actions_nb
 **     Number of actions.
 **/
/**   @var node_actionlist_s::size
 **     Allocated size.
 **/
/**   @var node_actionlist_s::grow
 **     Grow size, for reallocation.
 **/
/**   @var node_actionlist_s::actions
 **     Actions.
 **/
struct node_actionlist_s
{
  size_t          actions_nb;
  size_t          size;
  size_t          grow;
  node_action_t **actions;
};

/**
 ** @struct @node_statelist_s
 ** statelist dymanic array
 **/
/**   @var node_statelist_s::states_nb
 **     Number of states.
 **/
/**   @var node_statelist_s::size
 **     Allocated size.
 **/
/**   @var node_statelist_s::grow
 **     Grow size, for reallocation.
 **/
/**   @var node_statelist_s::states
 **     States.
 **/
struct node_statelist_s
{
  size_t         states_nb;
  size_t         size;
  size_t         grow;
  node_state_t **states;
};

/**
 ** @struct node_translist_s
 ** Transition list dynamic array
 **/
/**   @var node_translist_s::trans_nb
 **     Number of transitions.
 **/
/**   @var node_translist_s::size
 **     Allocated size.
 **/
/**   @var node_translist_s::grow
 **     Grow size, for reallocation.
 **/
/**   @var node_translist_s::trans
 **     Transitions.
 **/
struct node_translist_s
{
  size_t         trans_nb;
  size_t         size;
  size_t         grow;
  node_trans_t **trans;
};

/**
 ** @struct node_paramlist_s
 ** Parameter list dynamic array
 **/
/**   @var node_paramlist_s::params_nb
 **     Number of parameters.
 **/
/**   @var node_paramlist_s::size
 **     Allocated size.
 **/
/**   @var node_paramlist_s::grow
 **     Grow size, for reallocation.
 **/
/**   @var node_paramlist_s::params
 **     Parameters.
 **/
struct node_paramlist_s
{
  size_t        params_nb;
  size_t        size;
  size_t        grow;
  node_expr_t **params;
};


/**
 ** @struct node_rule_s
 ** Node_rule IS the root of the rule AST
 **/
/**   @var node_rule_s::line
 **     Line of declaration. (in the current rulefile).
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
  int                 line;
  char               *file;
  char               *name;
  node_state_t       *init;
  node_statelist_t   *statelist;
  node_syncvarlist_t *sync_vars;
};

/**
 ** @struct node_varlist_s
 ** Variable list dynamic array (used for string spliting)
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
/**   @var node_syncvarlist_s::varnames
 **     Elements.
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
 **   Symbol and its line of declaration in the current rulefile.
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
 ** @struct bytecode_buffer_s
 ** Used for converting list of actions into a ovm-bytecode sequence
 **/
/**   @var bytecode_buffer_s::pos
 **     Position in the buffer.
 **/
/**   @var bytecode_buffer_s::bytecode
 **     Bytecode buffer.
 **/
/**   @var bytecode_buffer_s::used_fields_pos
 **     Used fields array position.
 **/
/**   @var bytecode_buffer_s::used_fields
 **     Used field array.
 **/

#define BYTECODE_BUF_SZ 1024
#define MAX_FIELDS 16
typedef struct bytecode_buffer_s bytecode_buffer_t;
struct bytecode_buffer_s
{
  size_t pos;
  bytecode_t bytecode[BYTECODE_BUF_SZ];
  size_t used_fields_pos;
  int used_fields[MAX_FIELDS];
  unsigned long flags;
};


/*
 * file stack for cpp info's
 */
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

node_rule_t *build_rule(symbol_token_t *name,
                        node_state_t *init_state,
                        node_statelist_t *states,
                        node_syncvarlist_t *sync_vars);

node_statelist_t *build_statelist(node_state_t *state);
void statelist_add(node_statelist_t *list, node_state_t *state);
node_state_t *build_state(node_actionlist_t  *actions,
                          node_translist_t *transitions);
node_state_t *set_state_label(rule_compiler_t *ctx,
                              node_state_t *state,
                              symbol_token_t *sym,
                              unsigned long flags);

node_actionlist_t *build_actionlist(node_action_t *action);
void actionlist_add(node_actionlist_t *list, node_action_t *action);

node_translist_t *build_transitionlist(node_trans_t *trans);
node_trans_t *build_direct_transition(node_expr_t *cond, char *dest);
node_trans_t *build_indirect_transition(node_expr_t *cond, node_state_t *substate);
node_translist_t *build_unconditional_transition(char *dest);
void transitionlist_add(node_translist_t *list, node_trans_t *trans);

node_trans_t *build_unconditional_transition_test(char *dest);


node_expr_t *build_expr(int type, node_expr_t *left, node_expr_t *right);
node_expr_t *build_assoc(rule_compiler_t *ctx, char *var, node_expr_t *expr);
node_expr_t *build_function_call(rule_compiler_t *ctx, symbol_token_t *fnctname, node_paramlist_t *params);
node_expr_t *build_fieldname(rule_compiler_t *ctx, char *fieldname);
node_expr_t *build_varname(rule_compiler_t *ctx, char *varname);
node_expr_t *build_integer(rule_compiler_t *ctx, int i);
node_expr_t *build_double(rule_compiler_t *ctx, double f);
node_expr_t *build_string(rule_compiler_t *ctx, char *str);
char *build_substate_name(const char *refname, int n);
node_expr_t *build_ctime_from_int(rule_compiler_t *ctx, long ctime);
node_expr_t *build_ctime_from_string(rule_compiler_t *ctx, char *date);
node_expr_t *build_ipv4(rule_compiler_t *ctx, char *ipv4addr_str);
node_expr_t *build_timeval_from_int(rule_compiler_t *ctx, long sec, long usec);
node_expr_t *build_timeval_from_string(rule_compiler_t *ctx, char *date, long tv_usec);
node_expr_t *build_counter(rule_compiler_t *ctx, long value);
node_expr_t *build_regex(rule_compiler_t *ctx, char *regex_str);
node_expr_t *build_split_regex(rule_compiler_t *ctx, char *regex_str);

node_paramlist_t *build_paramlist(node_expr_t *first_param);
void paramlist_add(node_paramlist_t *list, node_expr_t *expr);

node_varlist_t *build_varlist(node_expr_t *first_param);
void varlist_add(node_varlist_t *list, node_expr_t *expr);

node_syncvarlist_t *build_syncvarlist(char *first_syncvar);
node_syncvarlist_t *syncvarlist_add(node_syncvarlist_t *list, char *syncvar);


node_expr_t *build_string_split(node_expr_t *source, node_expr_t *pattern, node_varlist_t *dest_list);

void compile_and_add_rule_ast(rule_compiler_t *ctx, node_rule_t *rule);
void compiler_reset(rule_compiler_t *ctx);

char *build_concat_string(char *str1, char *str2);


/* DEBUG */
void fprintf_expr(FILE *fp, node_expr_t *expr);
void fprintf_expr_infix(FILE *fp, node_expr_t *expr);


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
