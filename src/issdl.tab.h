/* A Bison parser, made by GNU Bison 2.0.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     EQ = 258,
     OROR = 259,
     ANDAND = 260,
     REGEX_NOTMATCH = 261,
     REGEX_MATCH = 262,
     NOTEQUAL = 263,
     EQBIND = 264,
     EQCOMPARE = 265,
     LESS_EQ = 266,
     GREATER_EQ = 267,
     LESS = 268,
     GREATER = 269,
     O_MINUS = 270,
     O_PLUS = 271,
     O_MOD = 272,
     O_DIV = 273,
     O_TIMES = 274,
     MINUSMINUS = 275,
     PLUSPLUS = 276,
     RULE = 277,
     STATE = 278,
     INIT = 279,
     IF = 280,
     GOTO = 281,
     WAITFOR = 282,
     O_BRACE = 283,
     C_BRACE = 284,
     O_PARENT = 285,
     C_PARENT = 286,
     SEMICOLUMN = 287,
     COMMA = 288,
     ONLY_ONCE = 289,
     SYNCHRONIZE = 290,
     KW_CTIME = 291,
     KW_IPV4 = 292,
     KW_TIMEVAL = 293,
     KW_COUNTER = 294,
     KW_REGEX = 295,
     SYMNAME = 296,
     FIELD = 297,
     VARIABLE = 298,
     NUMBER = 299,
     FPDOUBLE = 300,
     STRING = 301
   };
#endif
#define EQ 258
#define OROR 259
#define ANDAND 260
#define REGEX_NOTMATCH 261
#define REGEX_MATCH 262
#define NOTEQUAL 263
#define EQBIND 264
#define EQCOMPARE 265
#define LESS_EQ 266
#define GREATER_EQ 267
#define LESS 268
#define GREATER 269
#define O_MINUS 270
#define O_PLUS 271
#define O_MOD 272
#define O_DIV 273
#define O_TIMES 274
#define MINUSMINUS 275
#define PLUSPLUS 276
#define RULE 277
#define STATE 278
#define INIT 279
#define IF 280
#define GOTO 281
#define WAITFOR 282
#define O_BRACE 283
#define C_BRACE 284
#define O_PARENT 285
#define C_PARENT 286
#define SEMICOLUMN 287
#define COMMA 288
#define ONLY_ONCE 289
#define SYNCHRONIZE 290
#define KW_CTIME 291
#define KW_IPV4 292
#define KW_TIMEVAL 293
#define KW_COUNTER 294
#define KW_REGEX 295
#define SYMNAME 296
#define FIELD 297
#define VARIABLE 298
#define NUMBER 299
#define FPDOUBLE 300
#define STRING 301




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 39 "/home/olivain/work/orchids/src/issdl.y"
typedef union YYSTYPE {
  int integer;
  char *string;
  unsigned long flags;
  double fp_double;
  /* ovm_var_t *val; */
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
} YYSTYPE;
/* Line 1318 of yacc.c.  */
#line 149 "/home/olivain/work/orchids/src/issdl.tab.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE issdllval;



