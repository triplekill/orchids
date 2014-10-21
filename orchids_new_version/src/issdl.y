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

#define YYPARSE_PARAM __compiler_ctx_g
#define compiler_ctx_g ((rule_compiler_t *)__compiler_ctx_g)
#define YYLEX_PARAM compiler_ctx_g, compiler_ctx_g->scanner
//static rule_compiler_t *compiler_ctx_g = NULL;

/* Redefine yyerror so that it now takes an extra ctx argument */
#undef yyerror
#define yyerror(msg) issdlerror (compiler_ctx_g, (msg))

#define RESULT(dollar,p) compile_gc_protect(compiler_ctx_g, (gc_header_t *)((dollar) = (p)))
#define RESULT_DROP(dollar, n, p) compile_gc_protect_drop(compiler_ctx_g, (gc_header_t *)((dollar) = (p)), (n))

%}

%pure-parser

%union {
  unsigned long integer;
  char *string;
  unsigned long flags;
  double fp_double;
  symbol_token_t sym;
  node_rule_t *node_rule;
  node_state_t *node_state;
  node_trans_t *node_trans;
  node_expr_t *node_expr;
  node_varlist_t *node_varlist;
  node_syncvarlist_t *node_syncvarlist;
  size_t depth;
}

%left COMMA
%right EQ
%left OROR
%left ANDAND
%left O_OR
%left O_XOR
%left O_AND
%left EQCOMPARE NOTEQUAL REGEX_MATCH REGEX_NOTMATCH
%left GREATER LESS GREATER_EQ LESS_EQ
%left O_PLUS O_PLUS_EVENT O_MINUS
%left O_TIMES O_DIV O_MOD
%right PLUSPLUS MINUSMINUS O_NOT BANG

%token RULE STATE IF ELSE EXPECT GOTO /* Special keywords */
%token O_BRACE O_OPEN_EVENT C_BRACE O_PARENT C_PARENT EQ /* Punctuation */
%token SEMICOLUMN SYNCHRONIZE
%token KW_CTIME KW_IPV4 KW_IPV6 KW_TIMEVAL KW_REGEX
%token <sym> SYMNAME
%token <string> FIELD VARIABLE /* Raw data types */
%token <integer> NUMBER
%token <fp_double> FPDOUBLE
%token <string> STRING

%type <flags> state_options

%type <node_expr> params paramlist
%type <node_expr> var regsplit
%type <node_expr> expr
%type <node_expr> event_record event_recordlist event_records
%type <node_state> state statedefs
%type <node_expr> states statelist
%type <node_rule> rule
%type <node_expr> actionlist actions actionblock
%type <node_expr> action ifstatement
%type <node_expr> transitionlist transitions
%type <node_trans> transition
%type <node_varlist> var_list
%type <node_syncvarlist> sync_var_list synchro

%type <string> string
%type <depth> o_div o_parent eq ifkw o_brace o_open_event o_plus_event
%type <depth> expect goto statekw rulekw o_minus

%%


globaldef:
  /* Empty rule */
    { DPRINTF( ("*** warning *** empty rule file.\n") ); }
| rulelist
;


rulelist:
  rulelist rule_to_compile
| rule_to_compile
;

rule_to_compile: rule
    {
      compile_and_add_rule_ast(compiler_ctx_g, $1);
      compiler_reset(compiler_ctx_g);
    }
;

rulekw : RULE { $$ = COMPILE_GC_DEPTH(compiler_ctx_g); }
;

rule:
  rulekw SYMNAME synchro O_BRACE state states C_BRACE
  { RESULT_DROP($$,$1, build_rule(compiler_ctx_g, &($2), $5, $6, $3)); }
;


synchro:
  /* No synchro */
    { $$ = NULL; }
| SYNCHRONIZE O_PARENT sync_var_list C_PARENT
    { $$ = $3; }
;


sync_var_list:
  sync_var_list COMMA VARIABLE
  { $$ = syncvarlist_add(compiler_ctx_g, $1, $3); }
| VARIABLE
{ $$ = build_syncvarlist(compiler_ctx_g, $1); }
;


states:
  /* Empty */
  { $$ = NULL; }
| statelist
  { RESULT($$, expr_cons_reverse(compiler_ctx_g, $1)); }
;


statelist: /* returns reversed list of parameters */
  statelist state
  { RESULT($$, build_expr_cons(compiler_ctx_g, (node_expr_t *)$2, $1)); }
| state
  { RESULT($$, build_expr_cons(compiler_ctx_g, (node_expr_t *)$1, NULL)); }
;

statekw : STATE { $$ = COMPILE_GC_DEPTH(compiler_ctx_g); }
;

state:
  statekw SYMNAME state_options O_BRACE statedefs C_BRACE
  { RESULT_DROP($$,$1, set_state_label(compiler_ctx_g, $5, &($2), $3)); }
;


state_options:
  /* Empty */
    { $$ = SF_NOFLAGS; }
;


string:
  string STRING
  { $$ = build_concat_string(compiler_ctx_g, $1, $2); }
| STRING
    { $$ = $1; }
;


statedefs:
  /* empty state */
  { RESULT($$, build_state(compiler_ctx_g, NULL, NULL)); }
| actions  /* actionlist only - we have a terminal state */
  { RESULT($$, build_state(compiler_ctx_g, $1, NULL)); }
| transitions /* transitionlist only - nothing to do */
  { RESULT($$, build_state(compiler_ctx_g, NULL, $1)); }
| actions transitions /* actionlist and transitionlist */
  { RESULT($$, build_state(compiler_ctx_g, $1, $2)); }
;

actions:
  actionlist
  { RESULT($$, expr_cons_reverse(compiler_ctx_g, $1)); }
;

actionlist: /* returns reversed list of actions */
  actionlist action
  { RESULT($$, build_expr_action(compiler_ctx_g, $2, $1)); }
| action
  { RESULT($$, build_expr_action(compiler_ctx_g, $1, NULL)); }
;


var:
  VARIABLE
  { RESULT($$,build_varname(compiler_ctx_g, $1)); }
;


regsplit:
  string
  { RESULT($$,build_split_regex(compiler_ctx_g, $1)); }
;


var_list:
  var_list O_DIV var
  { varlist_add(compiler_ctx_g, $1, $3); $$ = $1; }
| O_DIV var
  { $$ = build_varlist(compiler_ctx_g, $2); }
;

o_div : O_DIV { $$ = COMPILE_GC_DEPTH(compiler_ctx_g); }
;

action:
  expr SEMICOLUMN
  { RESULT($$, $1); }
| o_div var O_DIV regsplit var_list SEMICOLUMN
  { RESULT_DROP($$,$1, build_string_split(compiler_ctx_g, $2, $4, $5)); }
| ifstatement
  { RESULT($$, $1); }
;

o_parent : O_PARENT { $$ = COMPILE_GC_DEPTH(compiler_ctx_g); }
;

eq : EQ { $$ = COMPILE_GC_DEPTH(compiler_ctx_g); }
;

o_minus : O_MINUS { $$ = COMPILE_GC_DEPTH(compiler_ctx_g); }
;

expr:
  o_parent expr C_PARENT
  { RESULT_DROP($$,$1, $2); }
| var eq expr %prec EQ
  { RESULT_DROP($$,$2, build_assoc(compiler_ctx_g, $1, $3)); }
| expr O_PLUS expr
  { RESULT($$, build_expr_binop(compiler_ctx_g, OP_ADD, $1, $3)); }
| expr O_MINUS expr
  { RESULT($$, build_expr_binop(compiler_ctx_g, OP_SUB, $1, $3)); }
| o_minus expr %prec O_MINUS
  { RESULT($$, build_expr_monop(compiler_ctx_g, OP_OPP, $2)); }
| expr O_TIMES expr
  { RESULT($$, build_expr_binop(compiler_ctx_g, OP_MUL, $1, $3)); }
| expr O_DIV expr
  { RESULT($$, build_expr_binop(compiler_ctx_g, OP_DIV, $1, $3)); }
| expr O_MOD expr
  { RESULT($$, build_expr_binop(compiler_ctx_g, OP_MOD, $1, $3)); }
| expr ANDAND expr
  { RESULT($$, build_expr_cond(compiler_ctx_g, ANDAND, $1, $3)); }
| expr OROR expr
  { RESULT($$, build_expr_cond(compiler_ctx_g, OROR, $1, $3)); }
| BANG expr
  { RESULT($$, build_expr_cond(compiler_ctx_g, BANG, NULL, $2)); }
| expr O_AND expr
  { RESULT($$, build_expr_binop(compiler_ctx_g, OP_AND, $1, $3)); }
| expr O_OR expr
  { RESULT($$, build_expr_binop(compiler_ctx_g, OP_OR, $1, $3)); }
| expr O_XOR expr
  { RESULT($$, build_expr_binop(compiler_ctx_g, OP_XOR, $1, $3)); }
| O_NOT expr
  { RESULT($$, build_expr_monop(compiler_ctx_g, OP_NOT, $2)); }
| expr EQCOMPARE expr
  { RESULT($$, build_expr_cond(compiler_ctx_g, OP_CEQ, $1, $3)); }
| expr NOTEQUAL expr
  { RESULT($$, build_expr_cond(compiler_ctx_g, OP_CNEQ, $1, $3)); }
| expr REGEX_MATCH expr
  { RESULT($$, build_expr_cond(compiler_ctx_g, OP_CRM, $1, $3)); }
| expr REGEX_NOTMATCH expr
  { RESULT($$, build_expr_cond(compiler_ctx_g, OP_CNRM, $1, $3)); }
| expr GREATER expr
  { RESULT($$, build_expr_cond(compiler_ctx_g, OP_CGT, $1, $3)); }
| expr LESS expr
  { RESULT($$, build_expr_cond(compiler_ctx_g, OP_CLT, $1, $3)); }
| expr GREATER_EQ expr
  { RESULT($$, build_expr_cond(compiler_ctx_g, OP_CGE, $1, $3)); }
| expr LESS_EQ expr
  { RESULT($$, build_expr_cond(compiler_ctx_g, OP_CLE, $1, $3)); }
| string /* Constant string */
  { RESULT($$,build_string(compiler_ctx_g, $1)); }
| NUMBER /* Constant integer */
  { RESULT($$,build_integer(compiler_ctx_g, $1)); }
| FPDOUBLE /* Constant floating point (double precision) value */
  { RESULT($$,build_double(compiler_ctx_g, $1)); }
| VARIABLE /* Variable */
  { RESULT($$,build_varname(compiler_ctx_g, $1)); }
| FIELD
  { RESULT($$, build_fieldname(compiler_ctx_g, $1)); }
| KW_CTIME o_parent NUMBER C_PARENT
  { RESULT_DROP($$,$2, build_ctime_from_int(compiler_ctx_g, (time_t)$3)); }
| KW_CTIME o_parent string C_PARENT
  { RESULT_DROP($$,$2, build_ctime_from_string(compiler_ctx_g, $3)); }
| KW_IPV4 O_PARENT string C_PARENT
    { $$ = build_ipv4(compiler_ctx_g, $3); }
| KW_IPV6 O_PARENT string C_PARENT
    { $$ = build_ipv6(compiler_ctx_g, $3); }
| KW_TIMEVAL o_parent NUMBER COMMA NUMBER C_PARENT
  { RESULT_DROP($$,$2, build_timeval_from_int(compiler_ctx_g, $3, $5)); }
| KW_TIMEVAL o_parent string COMMA NUMBER C_PARENT
  { RESULT_DROP($$,$2, build_timeval_from_string(compiler_ctx_g, $3, $5)); }
| KW_REGEX o_parent string C_PARENT
  { RESULT_DROP($$,$2, build_regex(compiler_ctx_g, $3)); }
| SYMNAME o_parent params C_PARENT
  { RESULT_DROP($$,$2, build_function_call(compiler_ctx_g, &($1), $3)); }
| o_open_event event_records C_BRACE
  { RESULT_DROP($$,$1, build_expr_event(compiler_ctx_g,
					OP_ADD_EVENT, NULL, $2)); }
| expr o_plus_event event_records C_BRACE %prec O_PLUS
  { RESULT_DROP($$,$2, build_expr_event(compiler_ctx_g,
					OP_ADD_EVENT, $1, $3)); }
;


event_records:
  /* Empty */
  { $$ = NULL; }
| event_recordlist
  { RESULT($$, expr_cons_reverse(compiler_ctx_g, $1)); }
;


event_recordlist: /* returns reversed list of assignments */
  event_recordlist COMMA event_record
  { RESULT($$, build_expr_cons(compiler_ctx_g, $3, $1)); }
| event_record
  { RESULT($$, build_expr_cons(compiler_ctx_g, $1, NULL)); }
;

event_record: FIELD EQ expr
  { RESULT($$, build_expr_binop(compiler_ctx_g,
				OP_ADD_EVENT,
				build_fieldname(compiler_ctx_g, $1),
				$3)); }
;

params:
  /* Empty */
  { $$ = NULL; }
| paramlist
  { RESULT($$, expr_cons_reverse(compiler_ctx_g, $1)); }
;


paramlist: /* returns reversed list of parameters */
  paramlist COMMA expr
  { RESULT($$, build_expr_cons(compiler_ctx_g, $3, $1)); }
| expr
  { RESULT($$, build_expr_cons(compiler_ctx_g, $1, NULL)); }
;

transitions : transitionlist
  { RESULT($$, expr_cons_reverse(compiler_ctx_g, $1)); }
;

transitionlist: /* returns reversed list of transitions */
  transitionlist transition
  { RESULT($$, build_expr_cons(compiler_ctx_g, (node_expr_t *)$2, $1)); }
| transition
  { RESULT($$, build_expr_cons(compiler_ctx_g, (node_expr_t *)$1, NULL)); }
;

o_brace : O_BRACE { $$ = COMPILE_GC_DEPTH(compiler_ctx_g); }
;

o_open_event : O_OPEN_EVENT { $$ = COMPILE_GC_DEPTH(compiler_ctx_g); }
;

o_plus_event : O_PLUS_EVENT { $$ = COMPILE_GC_DEPTH(compiler_ctx_g); }
;

actionblock:
  o_brace actions C_BRACE
  { RESULT_DROP($$,$1, $2); }
| o_brace C_BRACE
  { $$ = NULL; }
| action
  {  RESULT($$, build_expr_action(compiler_ctx_g, $1, NULL)); }

ifkw : IF { $$ = COMPILE_GC_DEPTH(compiler_ctx_g); }
;

ifstatement:
  ifkw O_PARENT expr C_PARENT actionblock
  { RESULT_DROP($$,$1, build_expr_ifstmt(compiler_ctx_g, $3, $5, NULL)); }
| ifkw O_PARENT expr C_PARENT actionblock ELSE actionblock
  { RESULT_DROP($$,$1, build_expr_ifstmt(compiler_ctx_g, $3, $5, $7)); }


expect : EXPECT { $$ = COMPILE_GC_DEPTH(compiler_ctx_g); }
;

goto : GOTO { $$ = COMPILE_GC_DEPTH(compiler_ctx_g); }
;

transition:
  expect O_PARENT expr C_PARENT GOTO SYMNAME SEMICOLUMN
  {  if ($6.file!=NULL)
       gc_base_free ($6.file);
     RESULT_DROP($$,$1,
		build_direct_transition(compiler_ctx_g, $3, $6.name)); }
| expect O_PARENT expr C_PARENT O_BRACE statedefs C_BRACE
  { RESULT_DROP($$,$1,
		build_indirect_transition(compiler_ctx_g, $3, $6)); }
| goto SYMNAME SEMICOLUMN
  { if ($2.file!=NULL)
       gc_base_free ($2.file);
     RESULT_DROP($$,$1,
		build_direct_transition(compiler_ctx_g,
					NULL, $2.name)); }
;

%%

void issdlerror(rule_compiler_t *ctx, char *s)
{
  DebugLog(DF_OLC, DS_ERROR, "%s on line %i.\n", s,
	   ctx->issdllineno);
  // DebugLog(DF_OLC, DS_DEBUG, "have to clear current compiler context (XXX: ToDo)\n");
}

/*
void
set_yaccer_context(rule_compiler_t *ctx)
{
  compiler_ctx_g = ctx;
}
*/


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
