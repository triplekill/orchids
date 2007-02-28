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

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0

/* Substitute the variable and function names.  */
#define yyparse issdlparse
#define yylex   issdllex
#define yyerror issdlerror
#define yylval  issdllval
#define yychar  issdlchar
#define yydebug issdldebug
#define yynerrs issdlnerrs


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




/* Copy the first part of user declarations.  */
#line 1 "/home/olivain/work/orchids/src/issdl.y"


/**
 ** @file issdl.y
 ** ISSDL Yaccer.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 **
 ** @date  Started on: Tue Feb 25 18:51:02 2003
 ** @date Last update: Tue Nov 29 11:15:23 2005
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



/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

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
/* Line 190 of yacc.c.  */
#line 234 "/home/olivain/work/orchids/src/issdl.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 213 of yacc.c.  */
#line 246 "/home/olivain/work/orchids/src/issdl.tab.c"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

# ifndef YYFREE
#  define YYFREE free
# endif
# ifndef YYMALLOC
#  define YYMALLOC malloc
# endif

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   else
#    define YYSTACK_ALLOC alloca
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (defined (YYSTYPE_IS_TRIVIAL) && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short int yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short int) + sizeof (YYSTYPE))			\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined (__GNUC__) && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  register YYSIZE_T yyi;		\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short int yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  6
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   327

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  47
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  25
/* YYNRULES -- Number of rules. */
#define YYNRULES  89
/* YYNRULES -- Number of states. */
#define YYNSTATES  189

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   301

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short int yyprhs[] =
{
       0,     0,     3,     4,     6,     9,    11,    19,    20,    25,
      29,    31,    32,    34,    37,    39,    46,    53,    54,    56,
      59,    61,    62,    64,    66,    69,    76,    84,    87,    89,
      91,    93,    97,   100,   103,   110,   114,   118,   122,   126,
     130,   134,   136,   138,   140,   142,   144,   149,   154,   159,
     166,   173,   178,   183,   188,   192,   193,   195,   199,   201,
     203,   205,   207,   209,   212,   214,   222,   230,   234,   236,
     238,   240,   242,   247,   252,   256,   260,   264,   268,   272,
     276,   280,   284,   288,   292,   296,   300,   304,   308,   312
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      48,     0,    -1,    -1,    49,    -1,    49,    50,    -1,    50,
      -1,    22,    41,    51,    28,    55,    53,    29,    -1,    -1,
      35,    30,    52,    31,    -1,    52,    33,    43,    -1,    43,
      -1,    -1,    54,    -1,    54,    56,    -1,    56,    -1,    23,
      24,    57,    28,    59,    29,    -1,    23,    41,    57,    28,
      59,    29,    -1,    -1,    34,    -1,    58,    46,    -1,    46,
      -1,    -1,    60,    -1,    69,    -1,    60,    69,    -1,    27,
      30,    71,    31,    32,    59,    -1,    60,    27,    30,    71,
      31,    32,    59,    -1,    60,    64,    -1,    64,    -1,    43,
      -1,    58,    -1,    63,    18,    61,    -1,    18,    61,    -1,
      65,    32,    -1,    18,    61,    18,    62,    63,    32,    -1,
      30,    65,    31,    -1,    65,    16,    65,    -1,    65,    15,
      65,    -1,    65,    19,    65,    -1,    65,    18,    65,    -1,
      65,    17,    65,    -1,    58,    -1,    44,    -1,    45,    -1,
      43,    -1,    42,    -1,    36,    30,    44,    31,    -1,    36,
      30,    58,    31,    -1,    37,    30,    58,    31,    -1,    38,
      30,    44,    33,    44,    31,    -1,    38,    30,    58,    33,
      44,    31,    -1,    39,    30,    44,    31,    -1,    40,    30,
      58,    31,    -1,    41,    30,    66,    31,    -1,    43,     3,
      65,    -1,    -1,    67,    -1,    67,    33,    68,    -1,    68,
      -1,    44,    -1,    58,    -1,    43,    -1,    42,    -1,    69,
      70,    -1,    70,    -1,    25,    30,    71,    31,    26,    41,
      32,    -1,    25,    30,    71,    31,    28,    59,    29,    -1,
      26,    41,    32,    -1,    44,    -1,    58,    -1,    43,    -1,
      42,    -1,    37,    30,    58,    31,    -1,    40,    30,    58,
      31,    -1,    30,    71,    31,    -1,    71,    16,    71,    -1,
      71,    15,    71,    -1,    71,    19,    71,    -1,    71,    18,
      71,    -1,    71,    17,    71,    -1,    71,     5,    71,    -1,
      71,     4,    71,    -1,    71,    10,    71,    -1,    71,     8,
      71,    -1,    71,     7,    71,    -1,    71,     6,    71,    -1,
      71,    14,    71,    -1,    71,    13,    71,    -1,    71,    12,
      71,    -1,    71,    11,    71,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,   101,   101,   102,   106,   111,   119,   139,   140,   145,
     147,   154,   155,   160,   162,   167,   172,   178,   179,   184,
     186,   192,   193,   195,   197,   199,   201,   206,   208,   213,
     218,   223,   225,   230,   236,   245,   247,   249,   251,   253,
     255,   257,   259,   261,   263,   265,   267,   269,   271,   273,
     275,   277,   279,   281,   283,   289,   290,   295,   297,   302,
     304,   306,   308,   322,   324,   332,   334,   336,   349,   351,
     353,   355,   357,   359,   361,   363,   365,   367,   369,   371,
     373,   375,   377,   379,   381,   383,   385,   387,   389,   391
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "EQ", "OROR", "ANDAND", "REGEX_NOTMATCH",
  "REGEX_MATCH", "NOTEQUAL", "EQBIND", "EQCOMPARE", "LESS_EQ",
  "GREATER_EQ", "LESS", "GREATER", "O_MINUS", "O_PLUS", "O_MOD", "O_DIV",
  "O_TIMES", "MINUSMINUS", "PLUSPLUS", "RULE", "STATE", "INIT", "IF",
  "GOTO", "WAITFOR", "O_BRACE", "C_BRACE", "O_PARENT", "C_PARENT",
  "SEMICOLUMN", "COMMA", "ONLY_ONCE", "SYNCHRONIZE", "KW_CTIME", "KW_IPV4",
  "KW_TIMEVAL", "KW_COUNTER", "KW_REGEX", "SYMNAME", "FIELD", "VARIABLE",
  "NUMBER", "FPDOUBLE", "STRING", "$accept", "globaldef", "rulelist",
  "rule", "synchro", "sync_var_list", "states", "statelist", "firststate",
  "state", "state_options", "string", "statedefs", "actionlist", "var",
  "regsplit", "var_list", "action", "expr", "params", "paramlist", "param",
  "transitionlist", "transition", "condexpr", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short int yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    47,    48,    48,    49,    49,    50,    51,    51,    52,
      52,    53,    53,    54,    54,    55,    56,    57,    57,    58,
      58,    59,    59,    59,    59,    59,    59,    60,    60,    61,
      62,    63,    63,    64,    64,    65,    65,    65,    65,    65,
      65,    65,    65,    65,    65,    65,    65,    65,    65,    65,
      65,    65,    65,    65,    65,    66,    66,    67,    67,    68,
      68,    68,    68,    69,    69,    70,    70,    70,    71,    71,
      71,    71,    71,    71,    71,    71,    71,    71,    71,    71,
      71,    71,    71,    71,    71,    71,    71,    71,    71,    71
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     0,     1,     2,     1,     7,     0,     4,     3,
       1,     0,     1,     2,     1,     6,     6,     0,     1,     2,
       1,     0,     1,     1,     2,     6,     7,     2,     1,     1,
       1,     3,     2,     2,     6,     3,     3,     3,     3,     3,
       3,     1,     1,     1,     1,     1,     4,     4,     4,     6,
       6,     4,     4,     4,     3,     0,     1,     3,     1,     1,
       1,     1,     1,     2,     1,     7,     7,     3,     1,     1,
       1,     1,     4,     4,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       2,     0,     0,     3,     5,     7,     1,     4,     0,     0,
       0,     0,    10,     0,     0,    11,     8,     0,    17,     0,
       0,    12,    14,     9,    18,     0,    17,     6,    13,    21,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    45,    44,    42,    43,    20,    41,     0,    22,
      28,     0,    23,    64,    21,    29,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    55,     0,    19,    15,
       0,    27,    24,     0,     0,     0,     0,     0,    33,    63,
       0,     0,     0,     0,     0,    71,    70,    68,    69,     0,
      67,     0,    35,     0,     0,     0,     0,     0,     0,     0,
      62,    61,    59,    60,     0,    56,    58,    54,     0,    37,
      36,    40,    39,    38,    16,    30,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    46,    47,    48,
       0,     0,    51,    52,    53,     0,     0,     0,     0,    74,
       0,     0,    81,    80,    85,    84,    83,    82,    89,    88,
      87,    86,    76,    75,    79,    78,    77,     0,    21,    21,
       0,     0,    57,     0,    32,     0,    34,    72,    73,     0,
       0,    25,    49,    50,    21,    31,    65,    66,    26
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short int yydefgoto[] =
{
      -1,     2,     3,     4,     9,    13,    20,    21,    15,    22,
      25,    88,    48,    49,    56,   116,   148,    50,    51,   104,
     105,   106,    52,    53,    89
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -146
static const short int yypact[] =
{
      -7,   -19,    42,    -7,  -146,    19,  -146,  -146,    47,    50,
      52,    62,  -146,    20,    84,    91,  -146,    72,    86,    80,
     108,    91,  -146,  -146,  -146,   110,    86,  -146,  -146,    61,
     125,   118,   147,   158,   186,   214,   187,   188,   189,   190,
     191,   208,  -146,   236,  -146,  -146,  -146,   194,   212,   105,
    -146,    -6,    35,  -146,    61,  -146,   224,    82,   211,    82,
      65,    83,   199,    90,   202,   199,    67,   214,  -146,  -146,
     217,  -146,    35,   214,   214,   214,   214,   214,  -146,  -146,
     219,   199,    82,   251,   271,  -146,  -146,  -146,   194,   152,
    -146,   174,  -146,   235,   -27,   -23,   269,   -28,   273,   -17,
    -146,  -146,  -146,   194,   274,   270,  -146,   157,    82,    22,
      22,  -146,  -146,  -146,  -146,   194,   288,   196,   199,   199,
      82,    82,    82,    82,    82,    82,    82,    82,    82,    82,
      82,    82,    82,    82,    82,   126,   275,  -146,  -146,  -146,
     264,   265,  -146,  -146,  -146,    67,   218,   118,     3,  -146,
     -15,    -3,   257,   272,   281,   281,   281,   281,   179,   179,
     179,   179,    75,    75,  -146,  -146,  -146,   276,    61,    61,
     279,   280,  -146,   282,  -146,   118,  -146,  -146,  -146,   283,
     284,  -146,  -146,  -146,    61,  -146,  -146,  -146,  -146
};

/* YYPGOTO[NTERM-NUM].  */
static const short int yypgoto[] =
{
    -146,  -146,  -146,   309,  -146,  -146,  -146,  -146,  -146,   295,
     292,   -29,   -51,  -146,  -145,  -146,  -146,   277,   -18,  -146,
    -146,   175,   278,   -45,   -58
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const unsigned char yytable[] =
{
      47,    91,   174,    80,   138,   141,    47,    79,   139,    73,
      74,    75,    76,    77,   143,     1,   177,    60,    68,    68,
      47,   175,     5,    68,   117,    47,    78,    79,   178,    68,
     185,    68,    94,    95,    97,   176,    99,   103,    47,    75,
      76,    77,     6,    68,    47,    47,    47,    47,    47,   107,
     146,    16,   115,    17,     8,   109,   110,   111,   112,   113,
      32,    33,   152,   153,   154,   155,   156,   157,   158,   159,
     160,   161,   162,   163,   164,   165,   166,    10,    11,    31,
      73,    74,    75,    76,    77,    14,    32,    33,    34,   150,
     151,    35,   132,   133,   134,    12,    92,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    18,   100,
     101,   102,    82,    46,    19,    23,   103,   180,   181,    83,
      24,    26,    84,    31,    85,    86,    87,    93,    46,    46,
      32,    33,    70,   188,    96,    35,    46,    27,    29,    47,
      47,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,   167,    54,   168,    47,   120,   121,   122,   123,
     124,    55,   125,   126,   127,   128,   129,   130,   131,   132,
     133,   134,    73,    74,    75,    76,    77,    57,   120,   121,
     122,   123,   124,   135,   125,   126,   127,   128,   129,   130,
     131,   132,   133,   134,   130,   131,   132,   133,   134,    58,
     120,   121,   122,   123,   124,   136,   125,   126,   127,   128,
     129,   130,   131,   132,   133,   134,    59,    61,    62,    63,
      64,    65,   120,   121,   122,   123,   124,   149,   125,   126,
     127,   128,   129,   130,   131,   132,   133,   134,    66,    67,
      68,    69,    81,    90,    35,    46,    98,   108,   114,   173,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,   120,   121,   122,   123,   124,   137,   125,   126,   127,
     128,   129,   130,   131,   132,   133,   134,   121,   122,   123,
     124,   118,   125,   126,   127,   128,   129,   130,   131,   132,
     133,   134,   126,   127,   128,   129,   130,   131,   132,   133,
     134,   119,   140,   145,   142,   144,   147,   169,   170,   171,
     182,   183,     7,   187,   184,   186,    28,   179,    30,     0,
     172,     0,     0,     0,     0,     0,    71,    72
};

static const short int yycheck[] =
{
      29,    59,   147,    54,    31,    33,    35,    52,    31,    15,
      16,    17,    18,    19,    31,    22,    31,    35,    46,    46,
      49,    18,    41,    46,    82,    54,    32,    72,    31,    46,
     175,    46,    61,    62,    63,    32,    65,    66,    67,    17,
      18,    19,     0,    46,    73,    74,    75,    76,    77,    67,
     108,    31,    81,    33,    35,    73,    74,    75,    76,    77,
      25,    26,   120,   121,   122,   123,   124,   125,   126,   127,
     128,   129,   130,   131,   132,   133,   134,    30,    28,    18,
      15,    16,    17,    18,    19,    23,    25,    26,    27,   118,
     119,    30,    17,    18,    19,    43,    31,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    24,    42,
      43,    44,    30,    46,    23,    43,   145,   168,   169,    37,
      34,    41,    40,    18,    42,    43,    44,    44,    46,    46,
      25,    26,    27,   184,    44,    30,    46,    29,    28,   168,
     169,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    26,    28,    28,   184,     4,     5,     6,     7,
       8,    43,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    15,    16,    17,    18,    19,    30,     4,     5,
       6,     7,     8,    31,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    15,    16,    17,    18,    19,    41,
       4,     5,     6,     7,     8,    31,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    30,    30,    30,    30,
      30,    30,     4,     5,     6,     7,     8,    31,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    30,     3,
      46,    29,    18,    32,    30,    46,    44,    30,    29,    31,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,     4,     5,     6,     7,     8,    31,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,     5,     6,     7,
       8,    30,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    30,    33,    33,    31,    31,    18,    32,    44,    44,
      31,    31,     3,    29,    32,    32,    21,    41,    26,    -1,
     145,    -1,    -1,    -1,    -1,    -1,    49,    49
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,    22,    48,    49,    50,    41,     0,    50,    35,    51,
      30,    28,    43,    52,    23,    55,    31,    33,    24,    23,
      53,    54,    56,    43,    34,    57,    41,    29,    56,    28,
      57,    18,    25,    26,    27,    30,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    58,    59,    60,
      64,    65,    69,    70,    28,    43,    61,    30,    41,    30,
      65,    30,    30,    30,    30,    30,    30,     3,    46,    29,
      27,    64,    69,    15,    16,    17,    18,    19,    32,    70,
      59,    18,    30,    37,    40,    42,    43,    44,    58,    71,
      32,    71,    31,    44,    58,    58,    44,    58,    44,    58,
      42,    43,    44,    58,    66,    67,    68,    65,    30,    65,
      65,    65,    65,    65,    29,    58,    62,    71,    30,    30,
       4,     5,     6,     7,     8,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    31,    31,    31,    31,    31,
      33,    33,    31,    31,    31,    33,    71,    18,    63,    31,
      58,    58,    71,    71,    71,    71,    71,    71,    71,    71,
      71,    71,    71,    71,    71,    71,    71,    26,    28,    32,
      44,    44,    68,    31,    61,    18,    32,    31,    31,    41,
      59,    59,    31,    31,    32,    61,    32,    29,    59
};

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");\
      YYERROR;							\
    }								\
while (0)


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (N)								\
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (0)
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
              (Loc).first_line, (Loc).first_column,	\
              (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (0)

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr, 					\
                  Type, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short int *bottom, short int *top)
#else
static void
yy_stack_print (bottom, top)
    short int *bottom;
    short int *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
             yyrule - 1, yylno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname [yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname [yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  register const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  register char *yyd = yydest;
  register const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

#endif /* !YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);


# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short int yyssa[YYINITDEPTH];
  short int *yyss = yyssa;
  register short int *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;


  yyvsp[0] = yylval;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short int *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short int *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a look-ahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to look-ahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 101 "/home/olivain/work/orchids/src/issdl.y"
    { DPRINTF( ("*** warning *** empty rule file.\n") ); ;}
    break;

  case 4:
#line 107 "/home/olivain/work/orchids/src/issdl.y"
    {
      compile_and_add_rule_ast(compiler_ctx_g, (yyvsp[0].node_rule));
      compiler_reset(compiler_ctx_g);
    ;}
    break;

  case 5:
#line 112 "/home/olivain/work/orchids/src/issdl.y"
    {
      compile_and_add_rule_ast(compiler_ctx_g, (yyvsp[0].node_rule));
      compiler_reset(compiler_ctx_g);
    ;}
    break;

  case 6:
#line 120 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_rule) = build_rule(&((yyvsp[-5].sym)), (yyvsp[-2].node_state), (yyvsp[-1].node_statelist), (yyvsp[-4].node_syncvarlist)); ;}
    break;

  case 7:
#line 139 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_syncvarlist) = NULL; ;}
    break;

  case 8:
#line 141 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_syncvarlist) = (yyvsp[-1].node_syncvarlist); ;}
    break;

  case 9:
#line 146 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_syncvarlist) = syncvarlist_add((yyvsp[-2].node_syncvarlist), (yyvsp[0].string)); ;}
    break;

  case 10:
#line 148 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_syncvarlist) = build_syncvarlist((yyvsp[0].string)); ;}
    break;

  case 11:
#line 154 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_statelist) = NULL; DPRINTF( ("only init state\n") ); ;}
    break;

  case 12:
#line 156 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_statelist) = (yyvsp[0].node_statelist); ;}
    break;

  case 13:
#line 161 "/home/olivain/work/orchids/src/issdl.y"
    { statelist_add((yyvsp[-1].node_statelist), (yyvsp[0].node_state)); (yyval.node_statelist) = (yyvsp[-1].node_statelist); ;}
    break;

  case 14:
#line 163 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_statelist) = build_statelist((yyvsp[0].node_state)); ;}
    break;

  case 15:
#line 168 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_state) = set_state_label(compiler_ctx_g, (yyvsp[-1].node_state), &((yyvsp[-4].sym)), (yyvsp[-3].flags)); ;}
    break;

  case 16:
#line 173 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_state) = set_state_label(compiler_ctx_g, (yyvsp[-1].node_state), &((yyvsp[-4].sym)), (yyvsp[-3].flags)); ;}
    break;

  case 17:
#line 178 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.flags) = SF_NOFLAGS; ;}
    break;

  case 18:
#line 180 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.flags) |= SF_ONLYONCE; ;}
    break;

  case 19:
#line 185 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.string) = build_concat_string((yyvsp[-1].string), (yyvsp[0].string)); ;}
    break;

  case 20:
#line 187 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.string) = (yyvsp[0].string); ;}
    break;

  case 21:
#line 192 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_state) = build_state(NULL, NULL); ;}
    break;

  case 22:
#line 194 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_state) = build_state((yyvsp[0].node_actionlist), NULL); ;}
    break;

  case 23:
#line 196 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_state) = build_state(NULL, (yyvsp[0].node_translist)); ;}
    break;

  case 24:
#line 198 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_state) = build_state((yyvsp[-1].node_actionlist), (yyvsp[0].node_translist)); ;}
    break;

  case 25:
#line 200 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_state) = NULL; /* XXX not used */ ;}
    break;

  case 26:
#line 202 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_state) = NULL; /* XXX not used */ ;}
    break;

  case 27:
#line 207 "/home/olivain/work/orchids/src/issdl.y"
    { actionlist_add((yyvsp[-1].node_actionlist), (yyvsp[0].node_action)); (yyval.node_actionlist) = (yyvsp[-1].node_actionlist); ;}
    break;

  case 28:
#line 209 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_actionlist) = build_actionlist((yyvsp[0].node_action)); ;}
    break;

  case 29:
#line 214 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_varname(compiler_ctx_g, (yyvsp[0].string)); ;}
    break;

  case 30:
#line 219 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_split_regex(compiler_ctx_g, (yyvsp[0].string)); ;}
    break;

  case 31:
#line 224 "/home/olivain/work/orchids/src/issdl.y"
    { varlist_add((yyvsp[-2].node_varlist), (yyvsp[0].node_expr)); (yyval.node_varlist) = (yyvsp[-2].node_varlist); ;}
    break;

  case 32:
#line 226 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_varlist) = build_varlist((yyvsp[0].node_expr)); ;}
    break;

  case 33:
#line 231 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_action) = (yyvsp[-1].node_expr); ;}
    break;

  case 34:
#line 237 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_action) = build_string_split((yyvsp[-4].node_expr), (yyvsp[-2].node_expr), (yyvsp[-1].node_varlist)); ;}
    break;

  case 35:
#line 246 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = (yyvsp[-1].node_expr); ;}
    break;

  case 36:
#line 248 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_expr(OP_ADD, (yyvsp[-2].node_expr), (yyvsp[0].node_expr)); ;}
    break;

  case 37:
#line 250 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_expr(OP_SUB, (yyvsp[-2].node_expr), (yyvsp[0].node_expr)); ;}
    break;

  case 38:
#line 252 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_expr(OP_MUL, (yyvsp[-2].node_expr), (yyvsp[0].node_expr)); ;}
    break;

  case 39:
#line 254 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_expr(OP_DIV, (yyvsp[-2].node_expr), (yyvsp[0].node_expr)); ;}
    break;

  case 40:
#line 256 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_expr(OP_MOD, (yyvsp[-2].node_expr), (yyvsp[0].node_expr)); ;}
    break;

  case 41:
#line 258 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_string(compiler_ctx_g, (yyvsp[0].string)); ;}
    break;

  case 42:
#line 260 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_integer(compiler_ctx_g, (yyvsp[0].integer)); ;}
    break;

  case 43:
#line 262 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_double(compiler_ctx_g, (yyvsp[0].fp_double)); ;}
    break;

  case 44:
#line 264 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_varname(compiler_ctx_g, (yyvsp[0].string)); ;}
    break;

  case 45:
#line 266 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_fieldname(compiler_ctx_g, (yyvsp[0].string)); ;}
    break;

  case 46:
#line 268 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_ctime_from_int(compiler_ctx_g, (yyvsp[-1].integer)); ;}
    break;

  case 47:
#line 270 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_ctime_from_string(compiler_ctx_g, (yyvsp[-1].string)); ;}
    break;

  case 48:
#line 272 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_ipv4(compiler_ctx_g, (yyvsp[-1].string)); ;}
    break;

  case 49:
#line 274 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_timeval_from_int(compiler_ctx_g, (yyvsp[-3].integer), (yyvsp[-1].integer)); ;}
    break;

  case 50:
#line 276 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_timeval_from_string(compiler_ctx_g, (yyvsp[-3].string), (yyvsp[-1].integer)); ;}
    break;

  case 51:
#line 278 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_counter(compiler_ctx_g, (yyvsp[-1].integer)); ;}
    break;

  case 52:
#line 280 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_regex(compiler_ctx_g, (yyvsp[-1].string)); ;}
    break;

  case 53:
#line 282 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_function_call(compiler_ctx_g, &((yyvsp[-3].sym)), (yyvsp[-1].node_paramlist)); ;}
    break;

  case 54:
#line 284 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_assoc(compiler_ctx_g, (yyvsp[-2].string), (yyvsp[0].node_expr)); ;}
    break;

  case 55:
#line 289 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_paramlist) = NULL; ;}
    break;

  case 56:
#line 291 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_paramlist) = (yyvsp[0].node_paramlist); ;}
    break;

  case 57:
#line 296 "/home/olivain/work/orchids/src/issdl.y"
    { paramlist_add((yyvsp[-2].node_paramlist), (yyvsp[0].node_expr)); (yyval.node_paramlist) = (yyvsp[-2].node_paramlist); ;}
    break;

  case 58:
#line 298 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_paramlist) = build_paramlist((yyvsp[0].node_expr)); ;}
    break;

  case 59:
#line 303 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_integer(compiler_ctx_g, (yyvsp[0].integer)); ;}
    break;

  case 60:
#line 305 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_string(compiler_ctx_g, (yyvsp[0].string)); ;}
    break;

  case 61:
#line 307 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_varname(compiler_ctx_g, (yyvsp[0].string)); ;}
    break;

  case 62:
#line 309 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_fieldname(compiler_ctx_g, (yyvsp[0].string)); ;}
    break;

  case 63:
#line 323 "/home/olivain/work/orchids/src/issdl.y"
    { transitionlist_add((yyvsp[-1].node_translist), (yyvsp[0].node_trans)); (yyval.node_translist) = (yyvsp[-1].node_translist); ;}
    break;

  case 64:
#line 325 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_translist) = build_transitionlist((yyvsp[0].node_trans)); ;}
    break;

  case 65:
#line 333 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_trans) = build_direct_transition((yyvsp[-4].node_expr), (yyvsp[-1].sym).name); ;}
    break;

  case 66:
#line 335 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_trans) = build_indirect_transition((yyvsp[-4].node_expr), (yyvsp[-1].node_state)); ;}
    break;

  case 67:
#line 337 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_trans) = build_unconditional_transition_test((yyvsp[-1].sym).name); DPRINTF( ("goto [%s]\n", (yyvsp[-1].sym).name) ); ;}
    break;

  case 68:
#line 350 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_integer(compiler_ctx_g, (yyvsp[0].integer)); ;}
    break;

  case 69:
#line 352 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_string(compiler_ctx_g, (yyvsp[0].string)); ;}
    break;

  case 70:
#line 354 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_varname(compiler_ctx_g, (yyvsp[0].string)); ;}
    break;

  case 71:
#line 356 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_fieldname(compiler_ctx_g, (yyvsp[0].string)); ;}
    break;

  case 72:
#line 358 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_ipv4(compiler_ctx_g, (yyvsp[-1].string)); ;}
    break;

  case 73:
#line 360 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_regex(compiler_ctx_g, (yyvsp[-1].string)); ;}
    break;

  case 74:
#line 362 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = (yyvsp[-1].node_expr); ;}
    break;

  case 75:
#line 364 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_expr(OP_ADD, (yyvsp[-2].node_expr), (yyvsp[0].node_expr)); ;}
    break;

  case 76:
#line 366 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_expr(OP_SUB, (yyvsp[-2].node_expr), (yyvsp[0].node_expr)); ;}
    break;

  case 77:
#line 368 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_expr(OP_MUL, (yyvsp[-2].node_expr), (yyvsp[0].node_expr)); ;}
    break;

  case 78:
#line 370 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_expr(OP_DIV, (yyvsp[-2].node_expr), (yyvsp[0].node_expr)); ;}
    break;

  case 79:
#line 372 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_expr(OP_MOD, (yyvsp[-2].node_expr), (yyvsp[0].node_expr)); ;}
    break;

  case 80:
#line 374 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_expr(ANDAND, (yyvsp[-2].node_expr), (yyvsp[0].node_expr)); ;}
    break;

  case 81:
#line 376 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_expr(OROR, (yyvsp[-2].node_expr), (yyvsp[0].node_expr)); ;}
    break;

  case 82:
#line 378 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_expr(OP_CEQ, (yyvsp[-2].node_expr), (yyvsp[0].node_expr)); ;}
    break;

  case 83:
#line 380 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_expr(OP_CNEQ, (yyvsp[-2].node_expr), (yyvsp[0].node_expr)); ;}
    break;

  case 84:
#line 382 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_expr(OP_CRM, (yyvsp[-2].node_expr), (yyvsp[0].node_expr)); ;}
    break;

  case 85:
#line 384 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_expr(OP_CNRM, (yyvsp[-2].node_expr), (yyvsp[0].node_expr)); ;}
    break;

  case 86:
#line 386 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_expr(OP_CGT, (yyvsp[-2].node_expr), (yyvsp[0].node_expr)); ;}
    break;

  case 87:
#line 388 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_expr(OP_CLT, (yyvsp[-2].node_expr), (yyvsp[0].node_expr)); ;}
    break;

  case 88:
#line 390 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_expr(OP_CGE, (yyvsp[-2].node_expr), (yyvsp[0].node_expr)); ;}
    break;

  case 89:
#line 392 "/home/olivain/work/orchids/src/issdl.y"
    { (yyval.node_expr) = build_expr(OP_CLE, (yyvsp[-2].node_expr), (yyvsp[0].node_expr)); ;}
    break;


    }

/* Line 1037 of yacc.c.  */
#line 1785 "/home/olivain/work/orchids/src/issdl.tab.c"

  yyvsp -= yylen;
  yyssp -= yylen;


  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  int yytype = YYTRANSLATE (yychar);
	  const char* yyprefix;
	  char *yymsg;
	  int yyx;

	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  int yyxbegin = yyn < 0 ? -yyn : 0;

	  /* Stay within bounds of both yycheck and yytname.  */
	  int yychecklim = YYLAST - yyn;
	  int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
	  int yycount = 0;

	  yyprefix = ", expecting ";
	  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      {
		yysize += yystrlen (yyprefix) + yystrlen (yytname [yyx]);
		yycount += 1;
		if (yycount == 5)
		  {
		    yysize = 0;
		    break;
		  }
	      }
	  yysize += (sizeof ("syntax error, unexpected ")
		     + yystrlen (yytname[yytype]));
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "syntax error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[yytype]);

	      if (yycount < 5)
		{
		  yyprefix = ", expecting ";
		  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
		    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
		      {
			yyp = yystpcpy (yyp, yyprefix);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yyprefix = " or ";
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("syntax error; also virtual memory exhausted");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror ("syntax error");
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* If at end of input, pop the error token,
	     then the rest of the stack, then return failure.  */
	  if (yychar == YYEOF)
	     for (;;)
	       {

		 YYPOPSTACK;
		 if (yyssp == yyss)
		   YYABORT;
		 yydestruct ("Error: popping",
                             yystos[*yyssp], yyvsp);
	       }
        }
      else
	{
	  yydestruct ("Error: discarding", yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

#ifdef __GNUC__
  /* Pacify GCC when the user code never invokes YYERROR and the label
     yyerrorlab therefore never appears in user code.  */
  if (0)
     goto yyerrorlab;
#endif

yyvsp -= yylen;
  yyssp -= yylen;
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping", yystos[yystate], yyvsp);
      YYPOPSTACK;
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;


  /* Shift the error token. */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yydestruct ("Error: discarding lookahead",
              yytoken, &yylval);
  yychar = YYEMPTY;
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*----------------------------------------------.
| yyoverflowlab -- parser overflow comes here.  |
`----------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 395 "/home/olivain/work/orchids/src/issdl.y"


void
issdlerror(char *s)
{
  DPRINTF( ("%s on line %i at '%s'.\n", s, issdllineno_g, issdltext) );
  DPRINTF( ("have to clear current compiler context (XXX: ToDo)\n") );
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

