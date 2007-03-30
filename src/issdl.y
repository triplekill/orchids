%{

/**
 ** @file issdl.y
 ** ISSDL Yaccer.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 1.0
 **
 ** @date  Started on: Tue Feb 25 18:51:02 2003
 ** @date Last update: Fri Mar 30 09:32:50 2007
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>

#include "orchids.h"
#include "rule_compiler.h"
#include "ovm.h"

extern int display_func(void *data, void *param);

extern int issdllineno_g;
extern char *issdltext;

static rule_compiler_t *compiler_ctx_g = NULL;

%}

%union {
  int integer;
  char *string;
  unsigned long flags;
  double fp_double;
  symbol_token_t sym;
  node_rule_t *node_rule;
  node_statelist_t *node_statelist;
  node_state_t *node_state;
  node_trans_t *node_trans;
  node_translist_t *node_translist;
  node_action_t *node_action;
  node_actionlist_t *node_actionlist;
  node_expr_t *node_expr;
  node_paramlist_t *node_paramlist;
  node_varlist_t *node_varlist;
  node_syncvarlist_t *node_syncvarlist;
}

%right EQ
%right OROR
%right ANDAND
%left EQCOMPARE EQBIND NOTEQUAL REGEX_MATCH REGEX_NOTMATCH
%left GREATER LESS GREATER_EQ LESS_EQ
%left O_PLUS O_MINUS
%left O_TIMES O_DIV O_MOD
%right PLUSPLUS MINUSMINUS

%token RULE STATE INIT IF GOTO IF WAITFOR /* special keywords */
%token O_BRACE C_BRACE O_PARENT C_PARENT EQ /* ponctuation */
%token SEMICOLUMN COMMA ONLY_ONCE SYNCHRONIZE
%token KW_CTIME KW_IPV4 KW_TIMEVAL KW_COUNTER KW_REGEX
%token <sym> SYMNAME INIT
%token <string> FIELD VARIABLE /* raw data types */
%token <integer> NUMBER
%token <fp_double> FPDOUBLE
%token <string> STRING

%type <node> globaldef rulelist

%type <flags> state_options

%type <node_paramlist> params paramlist
%type <node_expr> param var regsplit
%type <node_expr> expr condexpr
%type <node_state> firststate state statedefs
%type <node_statelist> states statelist
%type <node_rule> rule
%type <node_actionlist> actionlist
%type <node_action> action
%type <node_translist> transitionlist
%type <node_trans> transition
%type <node_varlist> var_list
%type <node_syncvarlist> sync_var_list synchro

%type <string> string

%%


globaldef:
  /* empty rule */
    { DPRINTF( ("*** warning *** empty rule file.\n") ); }
| rulelist
;


rulelist:
  rulelist rule
    {
      compile_and_add_rule_ast(compiler_ctx_g, $2);
      compiler_reset(compiler_ctx_g);
    }
| rule
    {
      compile_and_add_rule_ast(compiler_ctx_g, $1);
      compiler_reset(compiler_ctx_g);
    }
;


rule:
  RULE SYMNAME synchro O_BRACE firststate states C_BRACE
    { $$ = build_rule(&($2), $5, $6, $3); }
;


synchro:
  /* no synchro */
    { $$ = NULL; }
| SYNCHRONIZE O_PARENT sync_var_list C_PARENT
    { $$ = $3; }
;


sync_var_list:
  sync_var_list COMMA VARIABLE
    { $$ = syncvarlist_add($1, $3); }
| VARIABLE
    { $$ = build_syncvarlist($1); }
;


states:
  /* empty rule */
    { $$ = NULL; DPRINTF( ("only init state\n") ); }
| statelist
    { $$ = $1; }
;


statelist:
  statelist state
    { statelist_add($1, $2); $$ = $1; }
| state
    { $$ = build_statelist($1); }
;


firststate:
  STATE INIT state_options O_BRACE statedefs C_BRACE
    { $$ = set_state_label(compiler_ctx_g, $5, &($2), $3); }
;


state:
  STATE SYMNAME state_options O_BRACE statedefs C_BRACE
    { $$ = set_state_label(compiler_ctx_g, $5, &($2), $3); }
;


state_options:
  /* empty */
    { $$ = SF_NOFLAGS; }
| ONLY_ONCE
    { $$ |= SF_ONLYONCE; }
;


string:
  string STRING
    { $$ = build_concat_string($1, $2); }
| STRING
    { $$ = $1; }
;


statedefs:
  /* empty state */
    { $$ = build_state(NULL, NULL); }
| actionlist  /* actionlist only - we have a terminal state */
    { $$ = build_state($1, NULL); }
| transitionlist /* transitionlist only - nothing to do */
    { $$ = build_state(NULL, $1); }
| actionlist transitionlist /* actionlist and transisionlist */
    { $$ = build_state($1, $2); }
| WAITFOR O_PARENT condexpr C_PARENT SEMICOLUMN statedefs
    { $$ = NULL; /* XXX not used */ }
| actionlist WAITFOR O_PARENT condexpr C_PARENT SEMICOLUMN statedefs
    { $$ = NULL; /* XXX not used */ }
;


actionlist:
  actionlist action
    { actionlist_add($1, $2); $$ = $1; }
| action
    { $$ = build_actionlist($1); }
;


var:
  VARIABLE
    { $$ = build_varname(compiler_ctx_g, $1); }
;


regsplit:
  string
    { $$ = build_split_regex(compiler_ctx_g, $1); }
;


var_list:
  var_list O_DIV var
    { varlist_add($1, $3); $$ = $1; }
| O_DIV var
    { $$ = build_varlist($2); }
;


action:
  expr SEMICOLUMN
    { $$ = $1; }
| O_DIV var O_DIV regsplit var_list SEMICOLUMN
    { $$ = build_string_split($2, $4, $5); }
;


expr:
  O_PARENT expr C_PARENT
    { $$ = $2; }
| expr O_PLUS expr
  { $$ = build_expr(OP_ADD, $1, $3); }
| expr O_MINUS expr
  { $$ = build_expr(OP_SUB, $1, $3); }
| expr O_TIMES expr
  { $$ = build_expr(OP_MUL, $1, $3); }
| expr O_DIV expr
  { $$ = build_expr(OP_DIV, $1, $3); }
| expr O_MOD expr
  { $$ = build_expr(OP_MOD, $1, $3); }
| string /* type static string */
    { $$ = build_string(compiler_ctx_g, $1); }
| NUMBER /* type static int */
  { $$ = build_integer(compiler_ctx_g, $1); }
| FPDOUBLE /* type static double */
  { $$ = build_double(compiler_ctx_g, $1); }
| VARIABLE /* type varname */
  { $$ = build_varname(compiler_ctx_g, $1); }
| FIELD
  { $$ = build_fieldname(compiler_ctx_g, $1); }
| KW_CTIME O_PARENT NUMBER C_PARENT
    { $$ = build_ctime_from_int(compiler_ctx_g, $3); }
| KW_CTIME O_PARENT string C_PARENT
    { $$ = build_ctime_from_string(compiler_ctx_g, $3); }
| KW_IPV4 O_PARENT string C_PARENT
    { $$ = build_ipv4(compiler_ctx_g, $3); }
| KW_TIMEVAL O_PARENT NUMBER COMMA NUMBER C_PARENT
    { $$ = build_timeval_from_int(compiler_ctx_g, $3, $5); }
| KW_TIMEVAL O_PARENT string COMMA NUMBER C_PARENT
    { $$ = build_timeval_from_string(compiler_ctx_g, $3, $5); }
| KW_COUNTER O_PARENT NUMBER C_PARENT
    { $$ = build_counter(compiler_ctx_g, $3); }
| KW_REGEX O_PARENT string C_PARENT
    { $$ = build_regex(compiler_ctx_g, $3); }
| SYMNAME O_PARENT params C_PARENT
    { $$ = build_function_call(compiler_ctx_g, &($1), $3); }
| VARIABLE EQ expr
    { $$ = build_assoc(compiler_ctx_g, $1, $3); }
;


params:
  /* empty */
    { $$ = NULL; }
| paramlist
    { $$ = $1; }
;


paramlist:
  paramlist COMMA param
    { paramlist_add($1, $3); $$ = $1; }
| param
    { $$ = build_paramlist($1); }
;


param:
  NUMBER
  { $$ = build_integer(compiler_ctx_g, $1); }
| string
  { $$ = build_string(compiler_ctx_g, $1); }
| VARIABLE
  { $$ = build_varname(compiler_ctx_g, $1); }
| FIELD
  { $$ = build_fieldname(compiler_ctx_g, $1); }
;


transitionlist:
  transitionlist transition
    { transitionlist_add($1, $2); $$ = $1; }
| transition
    { $$ = build_transitionlist($1); }
;


transition:
  IF O_PARENT condexpr C_PARENT GOTO SYMNAME SEMICOLUMN
    { $$ = build_direct_transition($3, $6.name); }
| IF O_PARENT condexpr C_PARENT O_BRACE statedefs C_BRACE
    { $$ = build_indirect_transition($3, $6); }
| GOTO SYMNAME SEMICOLUMN
    { $$ = build_unconditional_transition_test($2.name); }
;


condexpr:
  NUMBER
  { $$ = build_integer(compiler_ctx_g, $1); }
| string
  { $$ = build_string(compiler_ctx_g, $1); }
| VARIABLE
  { $$ = build_varname(compiler_ctx_g, $1); }
| FIELD
  { $$ = build_fieldname(compiler_ctx_g, $1); }
| KW_IPV4 O_PARENT string C_PARENT
    { $$ = build_ipv4(compiler_ctx_g, $3); }
| KW_REGEX O_PARENT string C_PARENT
    { $$ = build_regex(compiler_ctx_g, $3); }
| O_PARENT condexpr C_PARENT
  { $$ = $2; }
| condexpr O_PLUS condexpr
  { $$ = build_expr(OP_ADD, $1, $3); }
| condexpr O_MINUS condexpr
  { $$ = build_expr(OP_SUB, $1, $3); }
| condexpr O_TIMES condexpr
  { $$ = build_expr(OP_MUL, $1, $3); }
| condexpr O_DIV condexpr
  { $$ = build_expr(OP_DIV, $1, $3); }
| condexpr O_MOD condexpr
  { $$ = build_expr(OP_MOD, $1, $3); }
| condexpr ANDAND condexpr
  { $$ = build_expr(ANDAND, $1, $3); }
| condexpr OROR condexpr
  { $$ = build_expr(OROR, $1, $3); }
| condexpr EQCOMPARE condexpr
  { $$ = build_expr(OP_CEQ, $1, $3); }
| condexpr NOTEQUAL condexpr
  { $$ = build_expr(OP_CNEQ, $1, $3); }
| condexpr REGEX_MATCH condexpr
  { $$ = build_expr(OP_CRM, $1, $3); }
| condexpr REGEX_NOTMATCH condexpr
  { $$ = build_expr(OP_CNRM, $1, $3); }
| condexpr GREATER condexpr
  { $$ = build_expr(OP_CGT, $1, $3); }
| condexpr LESS condexpr
  { $$ = build_expr(OP_CLT, $1, $3); }
| condexpr GREATER_EQ condexpr
  { $$ = build_expr(OP_CGE, $1, $3); }
| condexpr LESS_EQ condexpr
  { $$ = build_expr(OP_CLE, $1, $3); }
;

%%

void
issdlerror(char *s)
{
  DebugLog(DF_OLC, DS_ERROR, "%s on line %i at '%s'.\n", s, issdllineno_g, issdltext);
  DebugLog(DF_OLC, DS_DEBUG, "have to clear current compiler context (XXX: ToDo)\n");
}

void
set_yaccer_context(rule_compiler_t *ctx)
{
  compiler_ctx_g = ctx;
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
