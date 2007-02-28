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
#define yyparse netfilterparse
#define yylex   netfilterlex
#define yyerror netfiltererror
#define yylval  netfilterlval
#define yychar  netfilterchar
#define yydebug netfilterdebug
#define yynerrs netfilternerrs


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     NF_IN = 258,
     NF_PHYSIN = 259,
     NF_OUT = 260,
     NF_PHYSOUT = 261,
     NF_MAC = 262,
     NF_SRC = 263,
     NF_DST = 264,
     NF_LEN = 265,
     NF_TOS = 266,
     NF_PREC = 267,
     NF_TTL = 268,
     NF_ID = 269,
     NF_CE = 270,
     NF_DF = 271,
     NF_MF = 272,
     NF_FRAG = 273,
     NF_OPT = 274,
     NF_PROTO = 275,
     NF_INCOMPLETE = 276,
     NF_TCP = 277,
     NF_SPT = 278,
     NF_DPT = 279,
     NF_SEQ = 280,
     NF_ACK = 281,
     NF_WINDOW = 282,
     NF_RES = 283,
     NF_CWR = 284,
     NF_ECE = 285,
     NF_URG = 286,
     NF_PSH = 287,
     NF_RST = 288,
     NF_SYN = 289,
     NF_FIN = 290,
     NF_URGP = 291,
     NF_UDP = 292,
     NF_ICMP = 293,
     NF_TYPE = 294,
     NF_CODE = 295,
     NF_PARAM = 296,
     NF_GATEWAY = 297,
     NF_MTU = 298,
     NF_BYTES = 299,
     STRING = 300,
     INTEGER = 301,
     HEXBYTE = 302,
     INETADDR = 303,
     MACINFO = 304
   };
#endif
#define NF_IN 258
#define NF_PHYSIN 259
#define NF_OUT 260
#define NF_PHYSOUT 261
#define NF_MAC 262
#define NF_SRC 263
#define NF_DST 264
#define NF_LEN 265
#define NF_TOS 266
#define NF_PREC 267
#define NF_TTL 268
#define NF_ID 269
#define NF_CE 270
#define NF_DF 271
#define NF_MF 272
#define NF_FRAG 273
#define NF_OPT 274
#define NF_PROTO 275
#define NF_INCOMPLETE 276
#define NF_TCP 277
#define NF_SPT 278
#define NF_DPT 279
#define NF_SEQ 280
#define NF_ACK 281
#define NF_WINDOW 282
#define NF_RES 283
#define NF_CWR 284
#define NF_ECE 285
#define NF_URG 286
#define NF_PSH 287
#define NF_RST 288
#define NF_SYN 289
#define NF_FIN 290
#define NF_URGP 291
#define NF_UDP 292
#define NF_ICMP 293
#define NF_TYPE 294
#define NF_CODE 295
#define NF_PARAM 296
#define NF_GATEWAY 297
#define NF_MTU 298
#define NF_BYTES 299
#define STRING 300
#define INTEGER 301
#define HEXBYTE 302
#define INETADDR 303
#define MACINFO 304




/* Copy the first part of user declarations.  */
#line 1 "/home/olivain/work/orchids/src/modules/netfilter.y"

/**
 ** @file netfilter.y
 ** Netfilter yaccer.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 ** @date  Started on: Mon Jan 27 17:37:12 2003
 ** @date Last update: Tue Nov 29 11:02:10 2005
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

/* #include <fcntl.h> */
/* #include <sys/syscall.h> */
/* #include <sys/stat.h> */
#include <netinet/in.h>

  /* #include "netfilter.h" */

#include "orchids.h"

typedef struct string_s string_t;
struct string_s {
  char *str;
  size_t pos;
  size_t len;
};

static ovm_var_t **fields_g;
static char *vstr_base_g;
extern char *netfiltertext;

extern int netfilterlex(void);
extern void netfiltererror(char *s);
void netfilter_set_attrs(ovm_var_t **attr_fields);

static char *ip_proto_g[5];

#define NETFILTER_FIELDS 32
#define F_IN           0
#define F_PHYSIN       1
#define F_OUT          2
#define F_PHYSOUT      3
#define F_MAC          4
#define F_SRC          5
#define F_DST          6
#define F_IPLEN        7
#define F_TOS          8
#define F_PREC         9
#define F_TTL         10
#define F_IPID        11
#define F_IPFLAGS     12
#define F_FRAG        13
#define F_IPOPTS      14
#define F_PROTO       15
#define F_SPT         16
#define F_DPT         17
#define F_SEQ         18
#define F_ACK         19
#define F_WINDOW      20
#define F_RES         21
#define F_TCPFLAGS    22
#define F_URGP        23
#define F_UDPLEN      24
#define F_ICMPTYPE    25
#define F_ICMPCODE    26
#define F_ICMPID      27
#define F_ICMPSEQ     28
#define F_ICMPPARAM   29
#define F_ICMPGATEWAY 30
#define F_ICMPMTU     31

#define IP_PROTO_TCP   0
#define IP_PROTO_UDP   1
#define IP_PROTO_ICMP  2
#define IP_PROTO_OTHER 3

#define IP_FLAG_CE 0x01
#define IP_FLAG_MF 0x02
#define IP_FLAG_DF 0x04

#define TCP_FLAG_CWR 0x01
#define TCP_FLAG_ECE 0x02
#define TCP_FLAG_URG 0x04
#define TCP_FLAG_ACK 0x08
#define TCP_FLAG_PSH 0x10
#define TCP_FLAG_RST 0x20
#define TCP_FLAG_SYN 0x40
#define TCP_FLAG_FIN 0x80



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
#line 109 "/home/olivain/work/orchids/src/modules/netfilter.y"
typedef union YYSTYPE {
  int integer;
  char *string;
  string_t new_string;
  unsigned int ip_flags;
  unsigned int tcp_flags;
  unsigned char *mac_info;
  struct in_addr ipv4addr;
} YYSTYPE;
/* Line 190 of yacc.c.  */
#line 300 "/home/olivain/work/orchids/src/modules/netfilter.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 213 of yacc.c.  */
#line 312 "/home/olivain/work/orchids/src/modules/netfilter.tab.c"

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
#define YYFINAL  4
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   193

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  56
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  34
/* YYNRULES -- Number of rules. */
#define YYNRULES  66
/* YYNRULES -- Number of states. */
#define YYNSTATES  217

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   304

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,    51,     2,     2,     2,     2,     2,     2,     2,
      52,    53,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    50,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    54,     2,    55,     2,     2,     2,     2,     2,     2,
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
      45,    46,    47,    48,    49
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short int yyprhs[] =
{
       0,     0,     3,    47,    48,    50,    51,    56,    57,    62,
      63,    68,    69,    71,    75,    76,    79,    80,    83,    84,
      87,    88,    93,    94,   101,   103,   105,   107,   109,   114,
     126,   153,   154,   163,   172,   173,   176,   177,   180,   181,
     184,   185,   188,   189,   192,   193,   196,   197,   200,   201,
     204,   209,   221,   238,   243,   255,   269,   271,   273,   275,
     277,   286,   291,   296,   301,   302,   307
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      57,     0,    -1,     3,    50,    58,    51,    59,     5,    50,
      58,    51,    60,    61,     8,    50,    48,    51,     9,    50,
      48,    51,    10,    50,    46,    51,    11,    50,    47,    51,
      12,    50,    47,    51,    13,    50,    46,    51,    14,    50,
      46,    51,    63,    67,    68,    69,    -1,    -1,    45,    -1,
      -1,     4,    50,    45,    51,    -1,    -1,     6,    50,    45,
      51,    -1,    -1,     7,    50,    62,    51,    -1,    -1,    49,
      -1,    64,    65,    66,    -1,    -1,    15,    51,    -1,    -1,
      16,    51,    -1,    -1,    17,    51,    -1,    -1,    18,    50,
      46,    51,    -1,    -1,    19,    51,    52,    45,    53,    51,
      -1,    70,    -1,    81,    -1,    82,    -1,    89,    -1,    20,
      50,    22,    51,    -1,    20,    50,    22,    51,    21,    51,
      54,    46,    51,    44,    55,    -1,    20,    50,    22,    51,
      23,    50,    46,    51,    24,    50,    46,    51,    71,    27,
      50,    46,    51,    28,    50,    47,    51,    72,    36,    50,
      46,    51,    -1,    -1,    25,    50,    46,    51,    26,    50,
      46,    51,    -1,    73,    74,    75,    76,    77,    78,    79,
      80,    -1,    -1,    29,    51,    -1,    -1,    30,    51,    -1,
      -1,    31,    51,    -1,    -1,    26,    51,    -1,    -1,    32,
      51,    -1,    -1,    33,    51,    -1,    -1,    34,    51,    -1,
      -1,    35,    51,    -1,    20,    50,    37,    51,    -1,    20,
      50,    37,    51,    21,    51,    54,    46,    51,    44,    55,
      -1,    20,    50,    37,    51,    23,    50,    46,    51,    24,
      50,    46,    51,    10,    50,    46,    51,    -1,    20,    50,
      38,    51,    -1,    20,    50,    38,    51,    21,    51,    54,
      46,    51,    44,    55,    -1,    20,    50,    38,    51,    39,
      50,    46,    51,    40,    50,    46,    51,    83,    -1,    84,
      -1,    85,    -1,    86,    -1,    87,    -1,    14,    50,    46,
      51,    25,    50,    46,    51,    -1,    41,    50,    46,    51,
      -1,    42,    50,    48,    51,    -1,    54,    57,    55,    88,
      -1,    -1,    43,    50,    46,    51,    -1,    20,    50,    46,
      51,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,   144,   144,   200,   201,   207,   208,   218,   219,   229,
     230,   236,   237,   242,   252,   253,   259,   260,   266,   267,
     273,   274,   283,   284,   289,   293,   297,   301,   311,   312,
     313,   346,   347,   360,   370,   371,   377,   378,   384,   385,
     391,   392,   398,   399,   405,   406,   412,   413,   419,   420,
     429,   430,   431,   451,   452,   453,   467,   468,   469,   470,
     474,   485,   490,   496,   501,   503,   507
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "NF_IN", "NF_PHYSIN", "NF_OUT",
  "NF_PHYSOUT", "NF_MAC", "NF_SRC", "NF_DST", "NF_LEN", "NF_TOS",
  "NF_PREC", "NF_TTL", "NF_ID", "NF_CE", "NF_DF", "NF_MF", "NF_FRAG",
  "NF_OPT", "NF_PROTO", "NF_INCOMPLETE", "NF_TCP", "NF_SPT", "NF_DPT",
  "NF_SEQ", "NF_ACK", "NF_WINDOW", "NF_RES", "NF_CWR", "NF_ECE", "NF_URG",
  "NF_PSH", "NF_RST", "NF_SYN", "NF_FIN", "NF_URGP", "NF_UDP", "NF_ICMP",
  "NF_TYPE", "NF_CODE", "NF_PARAM", "NF_GATEWAY", "NF_MTU", "NF_BYTES",
  "STRING", "INTEGER", "HEXBYTE", "INETADDR", "MACINFO", "'='", "' '",
  "'('", "')'", "'['", "']'", "$accept", "nfline", "interface", "physin",
  "physout", "macinfo", "mac_header", "ip_flags",
  "ecn_congestion_experienced", "dont_fragment", "more_fragments",
  "frag_offset", "ip_options", "proto_specifics", "proto_tcp",
  "sequence_numbers", "tcp_flags", "cwr", "ece", "urg", "ack", "psh",
  "rst", "syn", "fin", "proto_udp", "proto_icmp", "icmp_specific",
  "icmp_echo", "icmp_parameter", "icmp_redirect", "icmp_other", "mtu",
  "proto_other", 0
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
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
      61,    32,    40,    41,    91,    93
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    56,    57,    58,    58,    59,    59,    60,    60,    61,
      61,    62,    62,    63,    64,    64,    65,    65,    66,    66,
      67,    67,    68,    68,    69,    69,    69,    69,    70,    70,
      70,    71,    71,    72,    73,    73,    74,    74,    75,    75,
      76,    76,    77,    77,    78,    78,    79,    79,    80,    80,
      81,    81,    81,    82,    82,    82,    83,    83,    83,    83,
      84,    85,    86,    87,    88,    88,    89
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,    43,     0,     1,     0,     4,     0,     4,     0,
       4,     0,     1,     3,     0,     2,     0,     2,     0,     2,
       0,     4,     0,     6,     1,     1,     1,     1,     4,    11,
      26,     0,     8,     8,     0,     2,     0,     2,     0,     2,
       0,     2,     0,     2,     0,     2,     0,     2,     0,     2,
       4,    11,    16,     4,    11,    13,     1,     1,     1,     1,
       8,     4,     4,     4,     0,     4,     4
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       0,     0,     0,     3,     1,     4,     0,     5,     0,     0,
       0,     0,     0,     3,     6,     0,     7,     0,     9,     0,
       0,     0,     0,    11,     0,     8,    12,     0,     0,    10,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    14,     0,    20,    16,    15,
       0,    22,     0,    18,     0,     0,     0,    17,     0,    13,
       0,     0,     0,     2,    24,    25,    26,    27,    19,    21,
       0,     0,     0,     0,     0,     0,     0,     0,    28,    50,
      53,    66,    23,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    29,
       0,    51,     0,    54,     0,    31,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    55,    56,    57,    58,    59,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    64,     0,     0,    52,     0,    61,    62,
       0,    63,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    65,    32,     0,    60,    34,     0,     0,
      36,    35,     0,     0,    38,     0,    37,     0,    40,     0,
      39,     0,    42,    30,    41,     0,    44,    43,     0,    46,
      45,     0,    48,    47,     0,    33,    49
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short int yydefgoto[] =
{
      -1,     2,     6,     9,    18,    21,    27,    57,    58,    63,
      69,    61,    66,    73,    74,   139,   189,   190,   194,   198,
     202,   206,   209,   212,   215,    75,    76,   145,   146,   147,
     148,   149,   171,    77
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -80
static const short int yypact[] =
{
       4,   -42,     9,   -35,   -80,   -80,   -40,     8,   -37,    10,
     -31,   -32,   -30,   -35,   -80,   -29,    13,   -27,    17,   -16,
     -24,    22,   -20,   -15,   -13,   -80,   -80,   -12,   -10,   -80,
      -9,    23,    -7,    -4,    -6,    25,    -3,    -5,    -2,    35,
       0,   -11,     1,    21,     3,     7,     5,    38,    11,     2,
       6,    41,    12,    14,    15,    43,    16,    45,    48,   -80,
      18,    40,    19,    52,    26,    20,    53,   -80,    24,   -80,
      27,    28,    29,   -80,   -80,   -80,   -80,   -80,   -80,   -80,
      31,   -21,    30,    33,    34,    36,    37,    39,   -18,   -17,
     -19,   -80,   -80,    42,    32,    44,    46,    47,    49,    50,
      51,    54,    55,    56,    57,    59,    58,    60,    61,    65,
      62,    63,    67,    64,    68,    66,    76,    74,    69,    77,
      70,    78,    73,    71,    79,    72,    82,    75,    83,   -80,
      80,   -80,    81,   -80,    84,    99,    90,   -14,    86,   106,
      87,    88,    89,    91,     4,   -80,   -80,   -80,   -80,   -80,
      94,    92,    97,    98,   100,   101,    93,    96,   104,   102,
     103,   105,   107,   108,   119,   109,   -80,   127,   -80,   -80,
     111,   -80,   112,   129,   113,   118,   120,   115,   121,   117,
     122,   123,   124,   -80,   -80,   125,   -80,   126,   128,   133,
     141,   -80,   130,   131,   143,   132,   -80,   134,   146,   135,
     -80,   136,   145,   -80,   -80,   137,   148,   -80,   138,   149,
     -80,   139,   156,   -80,   142,   -80,   -80
};

/* YYPGOTO[NTERM-NUM].  */
static const short int yypgoto[] =
{
     -80,   -79,   171,   -80,   -80,   -80,   -80,   -80,   -80,   -80,
     -80,   -80,   -80,   -80,   -80,   -80,   -80,   -80,   -80,   -80,
     -80,   -80,   -80,   -80,   -80,   -80,   -80,   -80,   -80,   -80,
     -80,   -80,   -80,   -80
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const unsigned char yytable[] =
{
     141,    83,    97,    93,    95,    94,    96,     1,     3,     4,
       5,     7,     8,    10,    12,    11,    84,    85,    13,    17,
      98,    14,    16,    19,    20,    86,    23,   142,   143,    22,
      24,    25,    32,    44,    26,    36,    42,    28,    30,    29,
     144,    38,    31,    33,    34,    35,    40,    37,    50,    39,
      41,    48,    43,    45,    46,    52,    47,    51,    56,    65,
      54,    49,    53,    60,    62,   156,    55,    59,    64,    68,
      67,    71,    70,    72,     0,    78,    82,     0,    79,    81,
      80,     0,   100,    87,    88,    89,     0,    90,    91,     0,
      92,   118,   120,    99,     0,   101,   102,   106,   103,   104,
     140,   108,     0,   110,   105,   111,   113,     0,   107,   112,
     109,   115,   114,   116,   117,   119,   122,   121,   123,   124,
     126,   125,   127,   128,   138,   130,   129,   131,   132,   134,
     133,   135,   136,   151,     0,   137,   150,   152,   153,   154,
     157,   155,   158,   159,   160,   172,   161,   164,   163,   162,
     165,   170,   174,   166,   167,   188,   168,   177,   169,     0,
     173,   175,   176,   178,   179,   181,   180,   182,   183,   192,
     185,   193,   201,   184,   197,   186,   187,   205,   199,   191,
     195,   208,   196,   211,    15,   200,   203,   204,   207,   210,
     213,   214,     0,   216
};

static const short int yycheck[] =
{
      14,    22,    21,    21,    21,    23,    23,     3,    50,     0,
      45,    51,     4,    50,    45,     5,    37,    38,    50,     6,
      39,    51,    51,    50,     7,    46,    50,    41,    42,    45,
       8,    51,     9,    12,    49,    10,    47,    50,    48,    51,
      54,    46,    51,    50,    48,    51,    11,    50,    46,    51,
      50,    13,    51,    50,    47,    14,    51,    51,    15,    19,
      46,    50,    50,    18,    16,   144,    51,    51,    50,    17,
      51,    51,    46,    20,    -1,    51,    45,    -1,    51,    50,
      52,    -1,    50,    53,    51,    51,    -1,    51,    51,    -1,
      51,    24,    24,    51,    -1,    51,    50,    46,    51,    50,
      10,    46,    -1,    46,    54,    46,    46,    -1,    54,    51,
      54,    46,    51,    51,    51,    51,    40,    51,    44,    50,
      50,    44,    44,    50,    25,    46,    55,    55,    46,    46,
      55,    51,    51,    27,    -1,    51,    50,    50,    50,    50,
      46,    50,    50,    46,    46,    26,    46,    51,    55,    48,
      46,    43,    25,    51,    51,    29,    51,    28,    51,    -1,
      51,    50,    50,    50,    46,    50,    46,    46,    51,    36,
      47,    30,    26,    51,    31,    51,    51,    32,    46,    51,
      50,    33,    51,    34,    13,    51,    51,    51,    51,    51,
      51,    35,    -1,    51
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,     3,    57,    50,     0,    45,    58,    51,     4,    59,
      50,     5,    45,    50,    51,    58,    51,     6,    60,    50,
       7,    61,    45,    50,     8,    51,    49,    62,    50,    51,
      48,    51,     9,    50,    48,    51,    10,    50,    46,    51,
      11,    50,    47,    51,    12,    50,    47,    51,    13,    50,
      46,    51,    14,    50,    46,    51,    15,    63,    64,    51,
      18,    67,    16,    65,    50,    19,    68,    51,    17,    66,
      46,    51,    20,    69,    70,    81,    82,    89,    51,    51,
      52,    50,    45,    22,    37,    38,    46,    53,    51,    51,
      51,    51,    51,    21,    23,    21,    23,    21,    39,    51,
      50,    51,    50,    51,    50,    54,    46,    54,    46,    54,
      46,    46,    51,    46,    51,    46,    51,    51,    24,    51,
      24,    51,    40,    44,    50,    44,    50,    44,    50,    55,
      46,    55,    46,    55,    46,    51,    51,    51,    25,    71,
      10,    14,    41,    42,    54,    83,    84,    85,    86,    87,
      50,    27,    50,    50,    50,    50,    57,    46,    50,    46,
      46,    46,    48,    55,    51,    46,    51,    51,    51,    51,
      43,    88,    26,    51,    25,    50,    50,    28,    50,    46,
      46,    50,    46,    51,    51,    47,    51,    51,    29,    72,
      73,    51,    36,    30,    74,    50,    51,    31,    75,    46,
      51,    26,    76,    51,    51,    32,    77,    51,    33,    78,
      51,    34,    79,    51,    35,    80,    51
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
#line 157 "/home/olivain/work/orchids/src/modules/netfilter.y"
    {
      DPRINTF( ("LEN=%i TOS=0x%02X PREC=0x%02X TTL=%i ID=%i\n",
                (yyvsp[-21].integer), (yyvsp[-17].integer), (yyvsp[-13].integer), (yyvsp[-9].integer), (yyvsp[-5].integer)) );

      if ((yyvsp[-40].new_string).len) {
        fields_g[F_IN] = ovm_vstr_new();
        VSTR(fields_g[F_IN]) = vstr_base_g + (yyvsp[-40].new_string).pos;
        VSTRLEN(fields_g[F_IN]) = (yyvsp[-40].new_string).len;
      }

      if ((yyvsp[-35].new_string).len) {
        fields_g[F_OUT] = ovm_vstr_new();
        VSTR(fields_g[F_OUT]) = vstr_base_g + (yyvsp[-35].new_string).pos;
        VSTRLEN(fields_g[F_OUT]) = (yyvsp[-35].new_string).len;
      }

      fields_g[F_SRC] = ovm_ipv4_new();
      IPV4(fields_g[F_SRC]) = (yyvsp[-29].ipv4addr);

      fields_g[F_DST] = ovm_ipv4_new();
      IPV4(fields_g[F_DST]) = (yyvsp[-25].ipv4addr);

      fields_g[F_IPLEN] = ovm_int_new();
      INT(fields_g[F_IPLEN]) = (yyvsp[-21].integer);

      fields_g[F_TOS] = ovm_int_new();
      INT(fields_g[F_TOS]) = (yyvsp[-17].integer);

      fields_g[F_PREC] = ovm_int_new();
      INT(fields_g[F_PREC]) = (yyvsp[-13].integer);

      fields_g[F_TTL] = ovm_int_new();
      INT(fields_g[F_TTL]) = (yyvsp[-9].integer);

      fields_g[F_IPID] = ovm_int_new();
      INT(fields_g[F_IPID]) = (yyvsp[-5].integer);

      DPRINTF( ("*** good line ***\n") );
    ;}
    break;

  case 3:
#line 200 "/home/olivain/work/orchids/src/modules/netfilter.y"
    { (yyval.new_string).len = 0; ;}
    break;

  case 4:
#line 202 "/home/olivain/work/orchids/src/modules/netfilter.y"
    { (yyval.new_string) = (yyvsp[0].new_string); ;}
    break;

  case 5:
#line 207 "/home/olivain/work/orchids/src/modules/netfilter.y"
    { /* do nothing */ ; ;}
    break;

  case 6:
#line 209 "/home/olivain/work/orchids/src/modules/netfilter.y"
    {
      fields_g[F_PHYSIN] = ovm_vstr_new();
      VSTR(fields_g[F_PHYSIN]) = vstr_base_g + (yyvsp[-1].new_string).pos;
      VSTRLEN(fields_g[F_PHYSIN]) = (yyvsp[-1].new_string).len;
    ;}
    break;

  case 7:
#line 218 "/home/olivain/work/orchids/src/modules/netfilter.y"
    { /* do nothing */ ; ;}
    break;

  case 8:
#line 220 "/home/olivain/work/orchids/src/modules/netfilter.y"
    {
      fields_g[F_PHYSOUT] = ovm_vstr_new();
      VSTR(fields_g[F_PHYSOUT]) = vstr_base_g + (yyvsp[-1].new_string).pos;
      VSTRLEN(fields_g[F_PHYSOUT]) = (yyvsp[-1].new_string).len;
    ;}
    break;

  case 9:
#line 229 "/home/olivain/work/orchids/src/modules/netfilter.y"
    { DPRINTF( ("no MAC info\n") ); ;}
    break;

  case 10:
#line 231 "/home/olivain/work/orchids/src/modules/netfilter.y"
    { DPRINTF( ("MAC=\n") ); ;}
    break;

  case 11:
#line 236 "/home/olivain/work/orchids/src/modules/netfilter.y"
    { DPRINTF( ("no MAC header\n") ); ;}
    break;

  case 12:
#line 238 "/home/olivain/work/orchids/src/modules/netfilter.y"
    { DPRINTF( ("MAC header\n") ); ;}
    break;

  case 13:
#line 243 "/home/olivain/work/orchids/src/modules/netfilter.y"
    {
      fields_g[F_IPFLAGS] = ovm_int_new();
      INT( fields_g[F_IPFLAGS] ) = (yyvsp[-2].ip_flags) | (yyvsp[-1].ip_flags) | (yyvsp[0].ip_flags);
    ;}
    break;

  case 14:
#line 252 "/home/olivain/work/orchids/src/modules/netfilter.y"
    { (yyval.ip_flags) = 0; ;}
    break;

  case 15:
#line 254 "/home/olivain/work/orchids/src/modules/netfilter.y"
    { (yyval.ip_flags) = IP_FLAG_CE; ;}
    break;

  case 16:
#line 259 "/home/olivain/work/orchids/src/modules/netfilter.y"
    { (yyval.ip_flags) = 0; ;}
    break;

  case 17:
#line 261 "/home/olivain/work/orchids/src/modules/netfilter.y"
    { (yyval.ip_flags) = IP_FLAG_DF; ;}
    break;

  case 18:
#line 266 "/home/olivain/work/orchids/src/modules/netfilter.y"
    { (yyval.ip_flags) = 0; ;}
    break;

  case 19:
#line 268 "/home/olivain/work/orchids/src/modules/netfilter.y"
    { (yyval.ip_flags) = IP_FLAG_MF; ;}
    break;

  case 20:
#line 273 "/home/olivain/work/orchids/src/modules/netfilter.y"
    { /* do nothing */ ; ;}
    break;

  case 21:
#line 275 "/home/olivain/work/orchids/src/modules/netfilter.y"
    {
      fields_g[F_FRAG] = ovm_int_new();
      INT(fields_g[F_FRAG]) = (yyvsp[-1].integer);
    ;}
    break;

  case 22:
#line 283 "/home/olivain/work/orchids/src/modules/netfilter.y"
    { /* do nothing */ ; ;}
    break;

  case 23:
#line 285 "/home/olivain/work/orchids/src/modules/netfilter.y"
    { DPRINTF( ("OPT\n") ); ;}
    break;

  case 24:
#line 290 "/home/olivain/work/orchids/src/modules/netfilter.y"
    {
      DPRINTF( ("PROTO=TCP\n") );
    ;}
    break;

  case 25:
#line 294 "/home/olivain/work/orchids/src/modules/netfilter.y"
    {
      DPRINTF( ("PROTO=UDP\n") );
    ;}
    break;

  case 26:
#line 298 "/home/olivain/work/orchids/src/modules/netfilter.y"
    {
      DPRINTF( ("PROTO=ICMP\n") );
    ;}
    break;

  case 27:
#line 302 "/home/olivain/work/orchids/src/modules/netfilter.y"
    {
      DPRINTF( ("PROTO=OTHER\n") );
    ;}
    break;

  case 30:
#line 321 "/home/olivain/work/orchids/src/modules/netfilter.y"
    {
      DPRINTF( ("SPT=%i DPT=%i WINDOWS=%i URGPP=%i\n", (yyvsp[-19].integer), (yyvsp[-15].integer), (yyvsp[-10].integer), (yyvsp[-1].integer)) );
      fields_g[F_PROTO] = ovm_vstr_new();
      VSTR(fields_g[F_PROTO]) = ip_proto_g[IP_PROTO_TCP];
      VSTRLEN(fields_g[F_PROTO]) = strlen(ip_proto_g[IP_PROTO_TCP]);

      fields_g[F_SPT] = ovm_int_new();
      INT(fields_g[F_SPT]) = (yyvsp[-19].integer);

      fields_g[F_DPT] = ovm_int_new();
      INT(fields_g[F_DPT]) = (yyvsp[-15].integer);

      fields_g[F_WINDOW] = ovm_int_new();
      INT(fields_g[F_WINDOW]) = (yyvsp[-10].integer);

      fields_g[F_RES] = ovm_int_new();
      INT(fields_g[F_RES]) = (yyvsp[-6].integer);

      fields_g[F_URGP] = ovm_int_new();
      INT(fields_g[F_URGP]) = (yyvsp[-1].integer);
    ;}
    break;

  case 31:
#line 346 "/home/olivain/work/orchids/src/modules/netfilter.y"
    { /* do nothing */ ; ;}
    break;

  case 32:
#line 348 "/home/olivain/work/orchids/src/modules/netfilter.y"
    {
      DPRINTF( ("SEQ=%i ACK=%i\n", (yyvsp[-5].integer), (yyvsp[-1].integer)) );

      fields_g[F_SEQ] = ovm_uint_new();
      UINT(fields_g[F_SEQ]) = (yyvsp[-5].integer);

      fields_g[F_ACK] = ovm_uint_new();
      UINT(fields_g[F_ACK]) = (yyvsp[-1].integer);
    ;}
    break;

  case 33:
#line 361 "/home/olivain/work/orchids/src/modules/netfilter.y"
    {
      fields_g[F_TCPFLAGS] = ovm_int_new();
      INT( fields_g[F_TCPFLAGS] ) = (yyvsp[-7].tcp_flags) | (yyvsp[-6].tcp_flags) | (yyvsp[-5].tcp_flags) | (yyvsp[-4].tcp_flags) | (yyvsp[-3].tcp_flags) | (yyvsp[-2].tcp_flags) | (yyvsp[-1].tcp_flags) | (yyvsp[0].tcp_flags);
    ;}
    break;

  case 34:
#line 370 "/home/olivain/work/orchids/src/modules/netfilter.y"
    { (yyval.tcp_flags) = 0; ;}
    break;

  case 35:
#line 372 "/home/olivain/work/orchids/src/modules/netfilter.y"
    { (yyval.tcp_flags) = TCP_FLAG_CWR; ;}
    break;

  case 36:
#line 377 "/home/olivain/work/orchids/src/modules/netfilter.y"
    { (yyval.tcp_flags) = 0; ;}
    break;

  case 37:
#line 379 "/home/olivain/work/orchids/src/modules/netfilter.y"
    { (yyval.tcp_flags) = TCP_FLAG_ECE; ;}
    break;

  case 38:
#line 384 "/home/olivain/work/orchids/src/modules/netfilter.y"
    { (yyval.tcp_flags) = 0; ;}
    break;

  case 39:
#line 386 "/home/olivain/work/orchids/src/modules/netfilter.y"
    { (yyval.tcp_flags) = TCP_FLAG_URG; ;}
    break;

  case 40:
#line 391 "/home/olivain/work/orchids/src/modules/netfilter.y"
    { (yyval.tcp_flags) = 0; ;}
    break;

  case 41:
#line 393 "/home/olivain/work/orchids/src/modules/netfilter.y"
    { (yyval.tcp_flags) = TCP_FLAG_ACK; ;}
    break;

  case 42:
#line 398 "/home/olivain/work/orchids/src/modules/netfilter.y"
    { (yyval.tcp_flags) = 0; ;}
    break;

  case 43:
#line 400 "/home/olivain/work/orchids/src/modules/netfilter.y"
    { (yyval.tcp_flags) = TCP_FLAG_PSH; ;}
    break;

  case 44:
#line 405 "/home/olivain/work/orchids/src/modules/netfilter.y"
    { (yyval.tcp_flags) = 0; ;}
    break;

  case 45:
#line 407 "/home/olivain/work/orchids/src/modules/netfilter.y"
    { (yyval.tcp_flags) = TCP_FLAG_RST; ;}
    break;

  case 46:
#line 412 "/home/olivain/work/orchids/src/modules/netfilter.y"
    { (yyval.tcp_flags) = 0; ;}
    break;

  case 47:
#line 414 "/home/olivain/work/orchids/src/modules/netfilter.y"
    { (yyval.tcp_flags) = TCP_FLAG_SYN; ;}
    break;

  case 48:
#line 419 "/home/olivain/work/orchids/src/modules/netfilter.y"
    { (yyval.tcp_flags) = 0; ;}
    break;

  case 49:
#line 421 "/home/olivain/work/orchids/src/modules/netfilter.y"
    { (yyval.tcp_flags) = TCP_FLAG_FIN; ;}
    break;

  case 52:
#line 434 "/home/olivain/work/orchids/src/modules/netfilter.y"
    {
      fields_g[F_SPT] = ovm_int_new();
      INT(fields_g[F_SPT]) = (yyvsp[-9].integer);

      fields_g[F_DPT] = ovm_int_new();
      INT(fields_g[F_DPT]) = (yyvsp[-5].integer);

      fields_g[F_UDPLEN] = ovm_int_new();
      INT(fields_g[F_UDPLEN]) = (yyvsp[-1].integer);
    ;}
    break;

  case 55:
#line 457 "/home/olivain/work/orchids/src/modules/netfilter.y"
    {
      fields_g[F_ICMPTYPE] = ovm_int_new();
      INT( fields_g[F_ICMPTYPE] ) = (yyvsp[-6].integer);

      fields_g[F_ICMPCODE] = ovm_int_new();
      INT( fields_g[F_ICMPCODE] ) = (yyvsp[-2].integer);
    ;}
    break;

  case 60:
#line 475 "/home/olivain/work/orchids/src/modules/netfilter.y"
    {
      fields_g[F_ICMPID] = ovm_int_new();
      INT( fields_g[F_ICMPID] ) = (yyvsp[-5].integer);

      fields_g[F_ICMPSEQ] = ovm_int_new();
      INT( fields_g[F_ICMPSEQ] ) = (yyvsp[-1].integer);
    ;}
    break;

  case 61:
#line 486 "/home/olivain/work/orchids/src/modules/netfilter.y"
    { DPRINTF( ("PARAMETER\n") ); ;}
    break;

  case 62:
#line 491 "/home/olivain/work/orchids/src/modules/netfilter.y"
    { DPRINTF( ("GATEWAY\n") ); ;}
    break;

  case 63:
#line 498 "/home/olivain/work/orchids/src/modules/netfilter.y"
    { DPRINTF( ("IP-in-ICMP\n") ); ;}
    break;

  case 66:
#line 508 "/home/olivain/work/orchids/src/modules/netfilter.y"
    { DPRINTF( ("PROTO=%i\n", (yyvsp[-1].integer)) ); ;}
    break;


    }

/* Line 1037 of yacc.c.  */
#line 1769 "/home/olivain/work/orchids/src/modules/netfilter.tab.c"

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


#line 511 "/home/olivain/work/orchids/src/modules/netfilter.y"


void
netfiltererror(char *s)
{
  DebugLog(DF_MOD, DS_ERROR, "%s at '%s'.\n",
           s, netfiltertext);
}

void
netfilter_set_attrs(ovm_var_t **attr_fields)
{
  fields_g = attr_fields;
}

void
netfilter_set_vstr_base(char *str)
{
  vstr_base_g = str;
}

static char *ip_proto_g[5] = {
  "TCP",
  "UDP",
  "ICMP",
  "OTHER",
  NULL
};


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

