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
#define yyparse snareparse
#define yylex   snarelex
#define yyerror snareerror
#define yylval  snarelval
#define yychar  snarechar
#define yydebug snaredebug
#define yynerrs snarenerrs


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     LINUX_AUDIT = 258,
     TOKEN_EVENT = 259,
     TOKEN_USER = 260,
     TOKEN_PROCESS = 261,
     TOKEN_PATH = 262,
     TOKEN_OWNER = 263,
     TOKEN_ATTRIBUTES = 264,
     TOKEN_ARGUMENTS = 265,
     TOKEN_TARGET = 266,
     TOKEN_DESTPATH = 267,
     TOKEN_SOCKET = 268,
     TOKEN_RETURN = 269,
     TOKEN_SEQUENCE = 270,
     TOKEN_DATE = 271,
     OPT_WRONLY = 272,
     OPT_RDONLY = 273,
     OPT_RDWR = 274,
     OPT_CREAT = 275,
     OPT_EXCL = 276,
     OPT_NOCTTY = 277,
     OPT_TRUNC = 278,
     OPT_APPEND = 279,
     OPT_NONBLOCK = 280,
     OPT_SYNC = 281,
     OPT_NOFOLLOW = 282,
     OPT_DIRECTORY = 283,
     OPT_LARGEFILE = 284,
     OPT_IFREG = 285,
     OPT_IFCHR = 286,
     OPT_IFBLK = 287,
     OPT_IFIFO = 288,
     SYS_OPEN = 289,
     SYS_CREAT = 290,
     SYS_MKDIR = 291,
     SYS_UNLINK = 292,
     SYS_MKNOD = 293,
     SYS_RMDIR = 294,
     SYS_CHMOD = 295,
     SYS_TRUNCATE = 296,
     SYS_CHROOT = 297,
     SYS_FTRUNCATE = 298,
     SYS_FCHMOD = 299,
     SYS_CHOWN = 300,
     SYS_LCHOWN = 301,
     SYS_FCHOWN = 302,
     SYS_EXECVE = 303,
     SYS_EXIT = 304,
     SYS_SYMLINK = 305,
     SYS_LINK = 306,
     SYS_RENAME = 307,
     SYS_MOUNT = 308,
     SYS_UMOUNT = 309,
     SYS_SETUID = 310,
     SYS_SETREUID = 311,
     SYS_SETRESUID = 312,
     SYS_SETGID = 313,
     SYS_SETREGID = 314,
     SYS_SETRESGID = 315,
     SYS_LCHOWN32 = 316,
     SYS_CHOWN32 = 317,
     SYS_SETUID32 = 318,
     SYS_SETREUID32 = 319,
     SYS_SETRESUID32 = 320,
     SYS_SETGID32 = 321,
     SYS_SETREGID32 = 322,
     SYS_SETRESGID32 = 323,
     SYS_CREATEMODULE = 324,
     SYS_DELETEMODULE = 325,
     SYS_FTRUNCATE64 = 326,
     SYS_TRUNCATE64 = 327,
     SYS_REBOOT = 328,
     SYS_SOCKETCALL = 329,
     STRING = 330,
     DATA_STRING = 331,
     INTEGER = 332,
     INETADDR = 333,
     UNIXPERM = 334,
     SOCK_ACCEPT = 335,
     SOCK_CONNECT = 336,
     SOCK_UNKNOWN = 337,
     TOKEN_DAY = 338,
     TOKEN_MONTH = 339
   };
#endif
#define LINUX_AUDIT 258
#define TOKEN_EVENT 259
#define TOKEN_USER 260
#define TOKEN_PROCESS 261
#define TOKEN_PATH 262
#define TOKEN_OWNER 263
#define TOKEN_ATTRIBUTES 264
#define TOKEN_ARGUMENTS 265
#define TOKEN_TARGET 266
#define TOKEN_DESTPATH 267
#define TOKEN_SOCKET 268
#define TOKEN_RETURN 269
#define TOKEN_SEQUENCE 270
#define TOKEN_DATE 271
#define OPT_WRONLY 272
#define OPT_RDONLY 273
#define OPT_RDWR 274
#define OPT_CREAT 275
#define OPT_EXCL 276
#define OPT_NOCTTY 277
#define OPT_TRUNC 278
#define OPT_APPEND 279
#define OPT_NONBLOCK 280
#define OPT_SYNC 281
#define OPT_NOFOLLOW 282
#define OPT_DIRECTORY 283
#define OPT_LARGEFILE 284
#define OPT_IFREG 285
#define OPT_IFCHR 286
#define OPT_IFBLK 287
#define OPT_IFIFO 288
#define SYS_OPEN 289
#define SYS_CREAT 290
#define SYS_MKDIR 291
#define SYS_UNLINK 292
#define SYS_MKNOD 293
#define SYS_RMDIR 294
#define SYS_CHMOD 295
#define SYS_TRUNCATE 296
#define SYS_CHROOT 297
#define SYS_FTRUNCATE 298
#define SYS_FCHMOD 299
#define SYS_CHOWN 300
#define SYS_LCHOWN 301
#define SYS_FCHOWN 302
#define SYS_EXECVE 303
#define SYS_EXIT 304
#define SYS_SYMLINK 305
#define SYS_LINK 306
#define SYS_RENAME 307
#define SYS_MOUNT 308
#define SYS_UMOUNT 309
#define SYS_SETUID 310
#define SYS_SETREUID 311
#define SYS_SETRESUID 312
#define SYS_SETGID 313
#define SYS_SETREGID 314
#define SYS_SETRESGID 315
#define SYS_LCHOWN32 316
#define SYS_CHOWN32 317
#define SYS_SETUID32 318
#define SYS_SETREUID32 319
#define SYS_SETRESUID32 320
#define SYS_SETGID32 321
#define SYS_SETREGID32 322
#define SYS_SETRESGID32 323
#define SYS_CREATEMODULE 324
#define SYS_DELETEMODULE 325
#define SYS_FTRUNCATE64 326
#define SYS_TRUNCATE64 327
#define SYS_REBOOT 328
#define SYS_SOCKETCALL 329
#define STRING 330
#define DATA_STRING 331
#define INTEGER 332
#define INETADDR 333
#define UNIXPERM 334
#define SOCK_ACCEPT 335
#define SOCK_CONNECT 336
#define SOCK_UNKNOWN 337
#define TOKEN_DAY 338
#define TOKEN_MONTH 339




/* Copy the first part of user declarations.  */
#line 1 "/home/olivain/work/orchids/src/modules/snare.y"

/**
 ** @file snare.y
 ** Snare yaccer.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 **
 ** @date  Started on: Thu Sep 11 18:37:40 2003
 ** @date Last update: Tue Nov 29 11:09:11 2005
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/* #define _GNU_SOURCE */ /* for O_LARGEFILE O_DIRECTORY */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/stat.h>

#include <netinet/in.h>

#include "orchids.h"

/* from  /usr/include/bits/fcntl.h */
#define O_DIRECT        040000 /* Direct disk access.  */
#define O_DIRECTORY    0200000 /* Must be a directory.  */
#define O_NOFOLLOW     0400000 /* Do not follow links.  */
#define O_LARGEFILE    0100000

static ovm_var_t **fields_g;
static char *linux24_syscall_name_g[256];
static char *linux24_sockcall_name_g[4];

extern int snarelex(void);
extern void snareerror(char *s);
extern char *snaretext;

void snareparse_set_attrs(ovm_var_t **attr_fields);

#define SOCKET_ACCEPT  0
#define SOCKET_CONNECT 1
#define SOCKET_UNKNOWN 2

#define F_TIME      0
#define F_CLASS     1
#define F_SYSCALL   2
#define F_RUID      3
#define F_RGID      4
#define F_EUID      5
#define F_EGID      6
#define F_PID       7
#define F_PROCNAME  8
#define F_RETCODE   9
#define F_WORKDIR   10
#define F_PATH      11
#define F_MODE      12
#define F_CREATEMODE 13
#define F_CMDLINE   14
#define F_SRCPATH   15
#define F_DSTPATH   16
#define F_SOCKCALL  17
#define F_DSTIP     18
#define F_DSTPORT   19
#define F_SRCIP     20
#define F_SRCPORT   21
#define F_OWNERUID  22
#define F_OWNERGID  23
#define F_TARGETID  24
#define F_TARGETRID 25
#define F_TARGETSID 26
#define F_MODNAME   27
#define F_SEQUENCE   28
#define F_DEVMAJ    29
#define F_DEVMIN    30
#define F_OFFSET    31



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
#line 91 "/home/olivain/work/orchids/src/modules/snare.y"
typedef union YYSTYPE {
  int integer;
  int syscall;
  int open_flags;
  int open_mode;
  int mknod_opt;
  int socket_call;
  char *str;
  time_t time;
  mode_t mode;
  struct in_addr ipv4addr;
} YYSTYPE;
/* Line 190 of yacc.c.  */
#line 355 "/home/olivain/work/orchids/src/modules/snare.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 213 of yacc.c.  */
#line 367 "/home/olivain/work/orchids/src/modules/snare.tab.c"

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
#define YYLAST   313

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  91
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  36
/* YYNRULES -- Number of rules. */
#define YYNRULES  99
/* YYNRULES -- Number of states. */
#define YYNSTATES  335

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   339

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,    85,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,    89,     2,     2,     2,     2,     2,     2,     2,
      87,    88,     2,     2,    86,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    90,     2,     2,     2,     2,     2,
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
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short int yyprhs[] =
{
       0,     0,     3,    18,    20,    22,    24,    26,    28,    30,
      32,    34,    47,    58,    70,    85,    95,    97,    99,   101,
     103,   105,   107,   109,   110,   115,   116,   123,   125,   127,
     129,   130,   133,   137,   139,   141,   143,   145,   147,   149,
     151,   153,   155,   157,   159,   160,   162,   166,   168,   170,
     172,   174,   176,   178,   180,   182,   184,   206,   208,   210,
     212,   214,   216,   230,   239,   241,   243,   263,   265,   267,
     269,   283,   285,   287,   289,   291,   293,   309,   330,   356,
     357,   359,   361,   363,   365,   367,   369,   371,   373,   375,
     377,   379,   381,   383,   394,   396,   398,   403,   410,   433
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      92,     0,    -1,    75,    85,     3,    85,     4,    86,    93,
      14,    86,    77,    85,    15,    86,    77,    -1,    94,    -1,
     107,    -1,   109,    -1,   110,    -1,   112,    -1,   114,    -1,
     116,    -1,   121,    -1,    34,    87,    99,   100,    88,    86,
     126,    85,   125,   124,   123,    97,    -1,    95,    87,    88,
      86,   126,    85,   125,   124,   123,    97,    -1,    38,    87,
     103,    88,    86,   126,    85,   125,   124,   123,    98,    -1,
     106,    87,   103,    88,    86,   126,    85,   125,   124,   123,
       9,    86,    77,    85,    -1,    96,    87,    88,    86,   126,
      85,   125,   124,   123,    -1,    36,    -1,    40,    -1,    44,
      -1,    37,    -1,    42,    -1,    35,    -1,    39,    -1,    -1,
       9,    86,    79,    85,    -1,    -1,     9,    86,    77,    89,
      77,    85,    -1,    17,    -1,    18,    -1,    19,    -1,    -1,
      90,   101,    -1,   101,    90,   102,    -1,   102,    -1,    20,
      -1,    21,    -1,    22,    -1,    23,    -1,    24,    -1,    25,
      -1,    26,    -1,    27,    -1,    28,    -1,    29,    -1,    -1,
     104,    -1,   104,    90,   105,    -1,   105,    -1,    30,    -1,
      31,    -1,    32,    -1,    33,    -1,    41,    -1,    72,    -1,
      43,    -1,    71,    -1,   108,    87,    88,    86,   126,    85,
     125,   124,   123,     8,    86,    75,    87,    77,    88,    86,
     117,    87,    77,    88,    85,    -1,    45,    -1,    62,    -1,
      46,    -1,    61,    -1,    47,    -1,    48,    87,    88,    86,
     126,    85,   125,   124,   123,    10,    86,    76,    85,    -1,
     111,    87,    88,    86,   126,    85,   125,   124,    -1,    49,
      -1,    73,    -1,    74,    87,   113,    88,    86,   126,    85,
     125,   124,    13,    86,    78,    86,    78,    86,    77,    86,
      77,    85,    -1,    80,    -1,    81,    -1,    82,    -1,   115,
      87,    88,    86,   126,    85,   125,   124,   123,    12,    86,
      76,    85,    -1,    51,    -1,    53,    -1,    52,    -1,    50,
      -1,    54,    -1,   118,    87,    88,    86,   126,    85,   125,
     124,    11,    86,    75,    87,    77,    88,    85,    -1,   119,
      87,    88,    86,   126,    85,   125,   124,    11,    86,    75,
      87,    77,    88,    86,   117,    87,    77,    88,    85,    -1,
     120,    87,    88,    86,   126,    85,   125,   124,    11,    86,
      75,    87,    77,    88,    86,   117,    87,    77,    88,    86,
     117,    87,    77,    88,    85,    -1,    -1,    75,    -1,    55,
      -1,    63,    -1,    58,    -1,    66,    -1,    56,    -1,    64,
      -1,    59,    -1,    67,    -1,    57,    -1,    65,    -1,    60,
      -1,    68,    -1,   122,    87,    88,    86,   126,    85,   125,
     124,   123,    85,    -1,    69,    -1,    70,    -1,     7,    86,
      76,    85,    -1,     6,    86,    77,    86,    75,    85,    -1,
       5,    86,    75,    87,    77,    88,    86,    75,    87,    77,
      88,    86,    75,    87,    77,    88,    86,    75,    87,    77,
      88,    85,    -1,    16,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,   155,   155,   169,   170,   171,   172,   173,   174,   175,
     176,   180,   189,   196,   205,   217,   227,   229,   231,   236,
     238,   240,   242,   248,   249,   258,   259,   270,   272,   274,
     280,   281,   286,   288,   293,   295,   297,   299,   301,   303,
     305,   307,   309,   311,   317,   318,   323,   325,   330,   332,
     334,   336,   341,   343,   345,   347,   352,   367,   369,   371,
     373,   375,   380,   394,   404,   406,   411,   435,   437,   439,
     444,   459,   461,   463,   465,   467,   472,   486,   501,   524,
     525,   530,   532,   534,   536,   541,   543,   545,   547,   552,
     554,   556,   558,   563,   574,   576,   581,   590,   601,   619
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "LINUX_AUDIT", "TOKEN_EVENT",
  "TOKEN_USER", "TOKEN_PROCESS", "TOKEN_PATH", "TOKEN_OWNER",
  "TOKEN_ATTRIBUTES", "TOKEN_ARGUMENTS", "TOKEN_TARGET", "TOKEN_DESTPATH",
  "TOKEN_SOCKET", "TOKEN_RETURN", "TOKEN_SEQUENCE", "TOKEN_DATE",
  "OPT_WRONLY", "OPT_RDONLY", "OPT_RDWR", "OPT_CREAT", "OPT_EXCL",
  "OPT_NOCTTY", "OPT_TRUNC", "OPT_APPEND", "OPT_NONBLOCK", "OPT_SYNC",
  "OPT_NOFOLLOW", "OPT_DIRECTORY", "OPT_LARGEFILE", "OPT_IFREG",
  "OPT_IFCHR", "OPT_IFBLK", "OPT_IFIFO", "SYS_OPEN", "SYS_CREAT",
  "SYS_MKDIR", "SYS_UNLINK", "SYS_MKNOD", "SYS_RMDIR", "SYS_CHMOD",
  "SYS_TRUNCATE", "SYS_CHROOT", "SYS_FTRUNCATE", "SYS_FCHMOD", "SYS_CHOWN",
  "SYS_LCHOWN", "SYS_FCHOWN", "SYS_EXECVE", "SYS_EXIT", "SYS_SYMLINK",
  "SYS_LINK", "SYS_RENAME", "SYS_MOUNT", "SYS_UMOUNT", "SYS_SETUID",
  "SYS_SETREUID", "SYS_SETRESUID", "SYS_SETGID", "SYS_SETREGID",
  "SYS_SETRESGID", "SYS_LCHOWN32", "SYS_CHOWN32", "SYS_SETUID32",
  "SYS_SETREUID32", "SYS_SETRESUID32", "SYS_SETGID32", "SYS_SETREGID32",
  "SYS_SETRESGID32", "SYS_CREATEMODULE", "SYS_DELETEMODULE",
  "SYS_FTRUNCATE64", "SYS_TRUNCATE64", "SYS_REBOOT", "SYS_SOCKETCALL",
  "STRING", "DATA_STRING", "INTEGER", "INETADDR", "UNIXPERM",
  "SOCK_ACCEPT", "SOCK_CONNECT", "SOCK_UNKNOWN", "TOKEN_DAY",
  "TOKEN_MONTH", "'\\t'", "','", "'('", "')'", "' '", "'|'", "$accept",
  "snareline", "event", "io_event", "perm_io_syscall", "io_syscall",
  "perm_attributes", "dev_attributes", "sys_open_mode", "sys_open_opts",
  "sys_open_optlist", "sys_open_opt", "sys_mknod_opts",
  "sys_mknod_optlist", "sys_mknod_opt", "trunc_syscall", "chown_event",
  "chown_syscall", "exec_event", "proc_event", "proc_syscall", "net_event",
  "socketcall_opt", "copy_event", "copy_syscall", "setid_event",
  "opt_username", "setid_syscall", "setreid_syscall", "setresid_syscall",
  "kern_event", "kern_syscall", "field_path", "field_process",
  "field_user", "date", 0
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
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,     9,    44,    40,    41,    32,
     124
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    91,    92,    93,    93,    93,    93,    93,    93,    93,
      93,    94,    94,    94,    94,    94,    95,    95,    95,    96,
      96,    96,    96,    97,    97,    98,    98,    99,    99,    99,
     100,   100,   101,   101,   102,   102,   102,   102,   102,   102,
     102,   102,   102,   102,   103,   103,   104,   104,   105,   105,
     105,   105,   106,   106,   106,   106,   107,   108,   108,   108,
     108,   108,   109,   110,   111,   111,   112,   113,   113,   113,
     114,   115,   115,   115,   115,   115,   116,   116,   116,   117,
     117,   118,   118,   118,   118,   119,   119,   119,   119,   120,
     120,   120,   120,   121,   122,   122,   123,   124,   125,   126
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,    14,     1,     1,     1,     1,     1,     1,     1,
       1,    12,    10,    11,    14,     9,     1,     1,     1,     1,
       1,     1,     1,     0,     4,     0,     6,     1,     1,     1,
       0,     2,     3,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     0,     1,     3,     1,     1,     1,
       1,     1,     1,     1,     1,     1,    21,     1,     1,     1,
       1,     1,    13,     8,     1,     1,    19,     1,     1,     1,
      13,     1,     1,     1,     1,     1,    15,    20,    25,     0,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,    10,     1,     1,     4,     6,    22,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       0,     0,     0,     0,     1,     0,     0,     0,     0,     0,
      21,    16,    19,     0,    22,    17,    52,    20,    54,    18,
      57,    59,    61,     0,    64,    74,    71,    73,    72,    75,
      81,    85,    89,    83,    87,    91,    60,    58,    82,    86,
      90,    84,    88,    92,    94,    95,    55,    53,    65,     0,
       0,     3,     0,     0,     0,     4,     0,     5,     6,     0,
       7,     8,     0,     9,     0,     0,     0,    10,     0,     0,
      44,     0,     0,     0,     0,     0,    44,     0,     0,     0,
       0,     0,     0,     0,    27,    28,    29,    30,    48,    49,
      50,    51,     0,    45,    47,     0,    67,    68,    69,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    31,    33,
       0,     0,    46,    99,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    32,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     2,     0,     0,     0,
       0,    63,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    23,    15,     0,     0,     0,     0,
       0,     0,     0,     0,    25,     0,     0,     0,     0,     0,
       0,    12,     0,     0,     0,     0,     0,     0,    93,    23,
       0,    13,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    11,     0,     0,     0,    96,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      97,    62,     0,    24,     0,     0,    70,     0,     0,     0,
       0,     0,     0,    14,     0,     0,     0,     0,     0,     0,
       0,     0,    76,    79,    79,    26,     0,     0,    79,    80,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    66,     0,     0,     0,     0,     0,    77,
      79,     0,    56,     0,     0,     0,     0,     0,     0,     0,
       0,    78,     0,     0,    98
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short int yydefgoto[] =
{
      -1,     2,    50,    51,    52,    53,   231,   241,    87,   112,
     138,   139,    92,    93,    94,    54,    55,    56,    57,    58,
      59,    60,    99,    61,    62,    63,   300,    64,    65,    66,
      67,    68,   212,   194,   177,   144
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -232
static const short int yypact[] =
{
     -32,   -24,    53,    62,  -232,   -16,    67,    -9,   -34,     3,
    -232,  -232,  -232,     9,  -232,  -232,  -232,  -232,  -232,  -232,
    -232,  -232,  -232,    12,  -232,  -232,  -232,  -232,  -232,  -232,
    -232,  -232,  -232,  -232,  -232,  -232,  -232,  -232,  -232,  -232,
    -232,  -232,  -232,  -232,  -232,  -232,  -232,  -232,  -232,    13,
      88,  -232,    17,    18,    23,  -232,    28,  -232,  -232,    29,
    -232,  -232,    30,  -232,    42,    44,    45,  -232,    46,    90,
      43,    47,    32,    48,    49,    50,    43,    51,    52,    54,
      55,    56,    57,    58,  -232,  -232,  -232,    40,  -232,  -232,
    -232,  -232,    59,    60,  -232,    63,  -232,  -232,  -232,    64,
      71,    65,    68,    69,    70,    72,    73,    74,    75,    76,
      77,    99,    78,    79,    43,   120,    81,    83,   120,   120,
      84,   120,   120,   120,   120,   120,   120,   120,  -232,  -232,
    -232,  -232,  -232,  -232,  -232,  -232,  -232,  -232,    82,  -232,
      85,   120,  -232,  -232,    89,   120,   126,    91,    92,   120,
      93,    94,    95,    96,    97,    98,   100,    99,   120,   101,
     148,   102,    87,   148,   148,   103,   148,   148,   148,   148,
     148,   148,   148,  -232,   104,   148,   105,   149,   148,   107,
     149,   149,   148,   149,   149,   149,   149,   149,   149,   149,
     148,   149,   115,   106,   157,   149,  -232,   157,   157,   149,
     157,  -232,   157,   158,   164,   182,   157,   149,   157,   108,
     117,   110,   187,   185,   190,  -232,   157,   192,   189,   116,
     118,   119,   121,   157,   194,   130,   122,   133,   124,   125,
     127,  -232,   203,   128,   129,   141,   142,   143,  -232,   190,
     134,  -232,   131,   146,   137,   147,   150,   145,   139,   151,
     153,   140,   144,   152,  -232,   155,   154,   156,  -232,   159,
     160,   162,   161,   163,   166,   165,   168,   171,   167,   174,
    -232,  -232,   175,  -232,   169,   178,  -232,   170,   172,   173,
     180,   176,   179,  -232,   181,   177,   184,   186,   183,   196,
     197,   191,  -232,   200,   200,  -232,   188,   193,   200,  -232,
     195,   198,   201,   204,   199,   206,   207,   205,   208,   211,
     202,   209,   212,  -232,   210,   215,   216,   214,   218,  -232,
     200,   213,  -232,   217,   219,   229,   220,   221,   223,   222,
     231,  -232,   224,   226,  -232
};

/* YYPGOTO[NTERM-NUM].  */
static const short int yypgoto[] =
{
    -232,  -232,  -232,  -232,  -232,  -232,    -6,  -232,  -232,  -232,
    -232,    80,   237,  -232,   138,  -232,  -232,  -232,  -232,  -232,
    -232,  -232,  -232,  -232,  -232,  -232,  -231,  -232,  -232,  -232,
    -232,  -232,  -105,  -129,   -84,   -77
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const unsigned short int yytable[] =
{
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,   147,   148,     1,   150,   151,   152,   153,   154,   155,
     156,   197,   198,     4,   200,   201,   202,   203,   204,   205,
     206,     3,   208,   301,   159,     5,   213,   304,   161,     6,
     216,     7,   165,    88,    89,    90,    91,     8,   223,   180,
     181,   174,   183,   184,   185,   186,   187,   188,   189,   323,
      69,   191,   214,   215,   195,   217,    70,   218,   199,    71,
      72,   222,    73,   224,    74,    75,   207,    84,    85,    86,
      76,   232,    96,    97,    98,    77,    78,    79,   239,   128,
     129,   130,   131,   132,   133,   134,   135,   136,   137,    80,
     111,    81,    82,    83,   100,    95,   143,   101,   102,   104,
     105,   162,   106,   107,   108,   109,   110,   113,   117,   115,
     114,   118,   116,   176,   119,   193,   121,   120,   122,   123,
     124,   125,   126,   127,   211,   141,   140,   145,   146,   219,
     149,   158,   157,   179,   160,   220,   163,   164,   166,   167,
     168,   169,   170,   171,   196,   172,   175,   178,   182,   190,
     209,   192,   210,   221,   226,   225,   227,   228,   229,   230,
     233,   234,   235,   240,   236,   237,   238,   242,   243,   244,
     245,   246,   248,   247,   249,   250,   251,   252,   253,   256,
     255,   257,   258,   259,   261,   262,   263,   265,   260,   264,
       0,   266,   268,   254,     0,     0,     0,   173,   274,   267,
     269,   270,   277,     0,   271,   278,   272,   273,   279,   281,
     275,   276,   142,   282,   283,   284,   280,   288,   285,     0,
     286,   287,   292,   289,     0,   290,     0,     0,   295,   291,
     293,     0,   294,   296,   297,   299,   302,   298,     0,   303,
     312,   308,   305,   310,   311,   306,   309,   307,   314,     0,
     315,   321,     0,   313,     0,   328,     0,   316,   318,   317,
     319,   324,   320,   322,   325,   326,   327,   331,   332,   329,
     330,   334,   333,   103
};

static const short int yycheck[] =
{
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,   118,   119,    75,   121,   122,   123,   124,   125,   126,
     127,   180,   181,     0,   183,   184,   185,   186,   187,   188,
     189,    85,   191,   294,   141,     3,   195,   298,   145,    85,
     199,     4,   149,    30,    31,    32,    33,    86,   207,   163,
     164,   158,   166,   167,   168,   169,   170,   171,   172,   320,
      87,   175,   197,   198,   178,   200,    87,   202,   182,    87,
      87,   206,    14,   208,    87,    87,   190,    17,    18,    19,
      87,   216,    80,    81,    82,    87,    87,    87,   223,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    87,
      90,    87,    87,    87,    86,    88,    16,    88,    88,    88,
      88,    15,    88,    88,    88,    88,    88,    88,    77,    86,
      90,    86,    88,     5,    86,     6,    86,    88,    86,    86,
      86,    86,    86,    86,     7,    86,    88,    86,    85,    11,
      86,    86,    90,    86,    85,    11,    85,    85,    85,    85,
      85,    85,    85,    85,    77,    85,    85,    85,    85,    85,
      75,    86,    86,    11,    77,    87,    86,    10,    13,     9,
       8,    12,    86,     9,    86,    86,    85,    77,    86,    76,
      86,    86,     9,    86,    86,    86,    75,    75,    75,    88,
      86,    75,    85,    76,    79,    86,    75,    87,    78,    76,
      -1,    87,    77,   239,    -1,    -1,    -1,   157,    77,    87,
      86,    85,    77,    -1,    85,    77,    86,    85,    77,    75,
      87,    85,   114,    78,    85,    77,    89,    77,    88,    -1,
      88,    88,    85,    87,    -1,    86,    -1,    -1,    85,    88,
      86,    -1,    86,    77,    77,    75,    88,    86,    -1,    86,
      75,    77,    87,    77,    77,    87,    87,    86,    77,    -1,
      88,    77,    -1,    85,    -1,    75,    -1,    88,    88,    87,
      85,    88,    86,    85,    87,    86,    77,    85,    77,    88,
      87,    85,    88,    76
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,    75,    92,    85,     0,     3,    85,     4,    86,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      93,    94,    95,    96,   106,   107,   108,   109,   110,   111,
     112,   114,   115,   116,   118,   119,   120,   121,   122,    87,
      87,    87,    87,    14,    87,    87,    87,    87,    87,    87,
      87,    87,    87,    87,    17,    18,    19,    99,    30,    31,
      32,    33,   103,   104,   105,    88,    80,    81,    82,   113,
      86,    88,    88,   103,    88,    88,    88,    88,    88,    88,
      88,    90,   100,    88,    90,    86,    88,    77,    86,    86,
      88,    86,    86,    86,    86,    86,    86,    86,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,   101,   102,
      88,    86,   105,    16,   126,    86,    85,   126,   126,    86,
     126,   126,   126,   126,   126,   126,   126,    90,    86,   126,
      85,   126,    15,    85,    85,   126,    85,    85,    85,    85,
      85,    85,    85,   102,   126,    85,     5,   125,    85,    86,
     125,   125,    85,   125,   125,   125,   125,   125,   125,   125,
      85,   125,    86,     6,   124,   125,    77,   124,   124,   125,
     124,   124,   124,   124,   124,   124,   124,   125,   124,    75,
      86,     7,   123,   124,   123,   123,   124,   123,   123,    11,
      11,    11,   123,   124,   123,    87,    77,    86,    10,    13,
       9,    97,   123,     8,    12,    86,    86,    86,    85,   123,
       9,    98,    77,    86,    76,    86,    86,    86,     9,    86,
      86,    75,    75,    75,    97,    86,    88,    75,    85,    76,
      78,    79,    86,    75,    76,    87,    87,    87,    77,    86,
      85,    85,    86,    85,    77,    87,    85,    77,    77,    77,
      89,    75,    78,    85,    77,    88,    88,    88,    77,    87,
      86,    88,    85,    86,    86,    85,    77,    77,    86,    75,
     117,   117,    88,    86,   117,    87,    87,    86,    77,    87,
      77,    77,    75,    85,    77,    88,    88,    87,    88,    85,
      86,    77,    85,   117,    88,    87,    86,    77,    75,    88,
      87,    85,    77,    88,    85
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
#line 159 "/home/olivain/work/orchids/src/modules/snare.y"
    {
      fields_g[F_RETCODE] = ovm_int_new();
      INT(fields_g[F_RETCODE]) = (yyvsp[-4].integer);
      fields_g[F_SEQUENCE] = ovm_int_new();
      INT(fields_g[F_SEQUENCE]) = (yyvsp[0].integer);
      fields_g[F_SEQUENCE]->flags |= TYPE_MONO;
    ;}
    break;

  case 11:
#line 182 "/home/olivain/work/orchids/src/modules/snare.y"
    {
      fields_g[F_MODE] = ovm_int_new();
      INT(fields_g[F_MODE]) = (yyvsp[-9].open_mode) | (yyvsp[-8].open_flags);
      fields_g[F_SYSCALL] = ovm_vstr_new();
      VSTR( fields_g[F_SYSCALL] ) = linux24_syscall_name_g[ SYS_open ];
      VSTRLEN( fields_g[F_SYSCALL] ) = strlen( linux24_syscall_name_g[ SYS_open ] );
    ;}
    break;

  case 12:
#line 191 "/home/olivain/work/orchids/src/modules/snare.y"
    {
      fields_g[F_SYSCALL] = ovm_vstr_new();
      VSTR( fields_g[F_SYSCALL] ) = linux24_syscall_name_g[ (yyvsp[-9].syscall) ];
      VSTRLEN( fields_g[F_SYSCALL] ) = strlen( linux24_syscall_name_g[ (yyvsp[-9].syscall) ] );
    ;}
    break;

  case 13:
#line 198 "/home/olivain/work/orchids/src/modules/snare.y"
    {
      DPRINTF( ("set io_event mknod()\n") );
      fields_g[F_SYSCALL] = ovm_vstr_new();
      VSTR( fields_g[F_SYSCALL] ) = linux24_syscall_name_g[ SYS_mknod ];
      VSTRLEN( fields_g[F_SYSCALL] ) = strlen( linux24_syscall_name_g[ SYS_mknod ] );
      /* XXX set mknod opts */
    ;}
    break;

  case 14:
#line 208 "/home/olivain/work/orchids/src/modules/snare.y"
    {
      DPRINTF( ("set trunc attributes\n") );
      fields_g[F_OFFSET] = ovm_int_new();
      INT(fields_g[F_OFFSET]) = (yyvsp[-1].integer);
      DPRINTF( ("set io_event syscall (f)truncate(64)()\n") );
      fields_g[F_SYSCALL] = ovm_vstr_new();
      VSTR( fields_g[F_SYSCALL] ) = linux24_syscall_name_g[ (yyvsp[-13].syscall) ];
      VSTRLEN( fields_g[F_SYSCALL] ) = strlen( linux24_syscall_name_g[ (yyvsp[-13].syscall) ] );
    ;}
    break;

  case 15:
#line 219 "/home/olivain/work/orchids/src/modules/snare.y"
    {
      fields_g[F_SYSCALL] = ovm_vstr_new();
      VSTR( fields_g[F_SYSCALL] ) = linux24_syscall_name_g[ (yyvsp[-8].syscall) ];
      VSTRLEN( fields_g[F_SYSCALL] ) = strlen( linux24_syscall_name_g[ (yyvsp[-8].syscall) ] );
    ;}
    break;

  case 16:
#line 228 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.syscall) = SYS_mkdir;  ;}
    break;

  case 17:
#line 230 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.syscall) = SYS_chmod;  ;}
    break;

  case 18:
#line 232 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.syscall) = SYS_fchmod; ;}
    break;

  case 19:
#line 237 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.syscall) = SYS_unlink; ;}
    break;

  case 20:
#line 239 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.syscall) = SYS_chroot; ;}
    break;

  case 21:
#line 241 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.syscall) = SYS_creat;  ;}
    break;

  case 22:
#line 243 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.syscall) = SYS_rmdir;  ;}
    break;

  case 23:
#line 248 "/home/olivain/work/orchids/src/modules/snare.y"
    { /* do_nothing() */; ;}
    break;

  case 24:
#line 250 "/home/olivain/work/orchids/src/modules/snare.y"
    {
      fields_g[F_CREATEMODE] = ovm_int_new();
      INT(fields_g[F_CREATEMODE]) = (yyvsp[-1].mode);
    ;}
    break;

  case 25:
#line 258 "/home/olivain/work/orchids/src/modules/snare.y"
    { /* do_nothing() */; ;}
    break;

  case 26:
#line 260 "/home/olivain/work/orchids/src/modules/snare.y"
    {
      DPRINTF( ("set device attribute") );
      fields_g[F_DEVMAJ] = ovm_int_new();
      INT(fields_g[F_DEVMAJ]) = (yyvsp[-3].integer);
      fields_g[F_DEVMIN] = ovm_int_new();
      INT(fields_g[F_DEVMIN]) = (yyvsp[-1].integer);
    ;}
    break;

  case 27:
#line 271 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.open_mode) = O_WRONLY; ;}
    break;

  case 28:
#line 273 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.open_mode) = O_RDONLY; ;}
    break;

  case 29:
#line 275 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.open_mode) = O_RDWR;   ;}
    break;

  case 30:
#line 280 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.open_flags) = 0;  ;}
    break;

  case 31:
#line 282 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.open_flags) = (yyvsp[0].open_flags); ;}
    break;

  case 32:
#line 287 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.open_flags) = (yyvsp[-2].open_flags) | (yyvsp[0].open_flags); ;}
    break;

  case 33:
#line 289 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.open_flags) = (yyvsp[0].open_flags);      ;}
    break;

  case 34:
#line 294 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.open_flags) = O_CREAT;     ;}
    break;

  case 35:
#line 296 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.open_flags) = O_EXCL;      ;}
    break;

  case 36:
#line 298 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.open_flags) = O_NOCTTY;    ;}
    break;

  case 37:
#line 300 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.open_flags) = O_TRUNC;     ;}
    break;

  case 38:
#line 302 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.open_flags) = O_APPEND;    ;}
    break;

  case 39:
#line 304 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.open_flags) = O_NONBLOCK;  ;}
    break;

  case 40:
#line 306 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.open_flags) = O_SYNC;      ;}
    break;

  case 41:
#line 308 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.open_flags) = O_NOFOLLOW;  ;}
    break;

  case 42:
#line 310 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.open_flags) = O_DIRECTORY; ;}
    break;

  case 43:
#line 312 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.open_flags) = O_LARGEFILE; ;}
    break;

  case 44:
#line 317 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.mknod_opt) = 0; ;}
    break;

  case 45:
#line 319 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.mknod_opt) = (yyvsp[0].mknod_opt); ;}
    break;

  case 46:
#line 324 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.mknod_opt) = (yyvsp[-2].mknod_opt) | (yyvsp[0].mknod_opt); ;}
    break;

  case 47:
#line 326 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.mknod_opt) = (yyvsp[0].mknod_opt); ;}
    break;

  case 48:
#line 331 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.mknod_opt) = S_IFREG; ;}
    break;

  case 49:
#line 333 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.mknod_opt) = S_IFCHR; ;}
    break;

  case 50:
#line 335 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.mknod_opt) = S_IFBLK; ;}
    break;

  case 51:
#line 337 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.mknod_opt) = S_IFIFO; ;}
    break;

  case 52:
#line 342 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.syscall) = SYS_truncate; ;}
    break;

  case 53:
#line 344 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.syscall) = SYS_truncate64; ;}
    break;

  case 54:
#line 346 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.syscall) = SYS_ftruncate; ;}
    break;

  case 55:
#line 348 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.syscall) = SYS_ftruncate64; ;}
    break;

  case 56:
#line 355 "/home/olivain/work/orchids/src/modules/snare.y"
    {
      fields_g[F_OWNERUID] = ovm_int_new();
      INT(fields_g[F_OWNERUID]) = (yyvsp[-7].integer);
      fields_g[F_OWNERGID] = ovm_int_new();
      INT(fields_g[F_OWNERGID]) = (yyvsp[-2].integer);
      fields_g[F_SYSCALL] = ovm_vstr_new();
      VSTR( fields_g[F_SYSCALL] ) = linux24_syscall_name_g[ (yyvsp[-20].syscall) ];
      VSTRLEN( fields_g[F_SYSCALL] ) = strlen( linux24_syscall_name_g[ (yyvsp[-20].syscall) ] );
    ;}
    break;

  case 57:
#line 368 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.syscall) = SYS_chown;    ;}
    break;

  case 58:
#line 370 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.syscall) = SYS_chown32;  ;}
    break;

  case 59:
#line 372 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.syscall) = SYS_lchown;   ;}
    break;

  case 60:
#line 374 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.syscall) = SYS_lchown32; ;}
    break;

  case 61:
#line 376 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.syscall) = SYS_fchown;   ;}
    break;

  case 62:
#line 383 "/home/olivain/work/orchids/src/modules/snare.y"
    {
      fields_g[F_CMDLINE] = ovm_str_new( strlen((yyvsp[-1].str)) );
      memcpy( STR(fields_g[F_CMDLINE]), (yyvsp[-1].str), strlen((yyvsp[-1].str)));
      free((yyvsp[-1].str)); /* XXX: compute offsets and sizes instead of dup()ing strings */
      fields_g[F_SYSCALL] = ovm_vstr_new();
      VSTR( fields_g[F_SYSCALL] ) = linux24_syscall_name_g[ SYS_execve ];
      VSTRLEN( fields_g[F_SYSCALL] ) = strlen( linux24_syscall_name_g[ SYS_execve ] );
    ;}
    break;

  case 63:
#line 396 "/home/olivain/work/orchids/src/modules/snare.y"
    {
      fields_g[F_SYSCALL] = ovm_vstr_new();
      VSTR( fields_g[F_SYSCALL] ) = linux24_syscall_name_g[ (yyvsp[-7].syscall) ];
      VSTRLEN( fields_g[F_SYSCALL] ) = strlen( linux24_syscall_name_g[ (yyvsp[-7].syscall) ] );
    ;}
    break;

  case 64:
#line 405 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.syscall) = SYS_exit;   ;}
    break;

  case 65:
#line 407 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.syscall) = SYS_reboot; ;}
    break;

  case 66:
#line 414 "/home/olivain/work/orchids/src/modules/snare.y"
    {
      fields_g[F_SRCIP] = ovm_ipv4_new();
      IPV4( fields_g[F_SRCIP] ) = (yyvsp[-7].ipv4addr);
      fields_g[F_DSTIP] = ovm_ipv4_new();
      IPV4( fields_g[F_DSTIP] ) = (yyvsp[-5].ipv4addr);
      fields_g[F_SRCPORT] = ovm_int_new();
      INT(fields_g[F_SRCPORT]) = (yyvsp[-3].integer);
      fields_g[F_DSTPORT] = ovm_int_new();
      INT(fields_g[F_DSTPORT]) = (yyvsp[-1].integer);

      fields_g[F_SOCKCALL] = ovm_vstr_new();
      VSTR( fields_g[F_SOCKCALL] ) = linux24_sockcall_name_g[ (yyvsp[-16].socket_call) ];
      VSTRLEN( fields_g[F_SOCKCALL] ) = strlen( linux24_sockcall_name_g[ (yyvsp[-16].socket_call) ] );

      fields_g[F_SYSCALL] = ovm_vstr_new();
      VSTR( fields_g[F_SYSCALL] ) = linux24_syscall_name_g[ SYS_socketcall ];
      VSTRLEN( fields_g[F_SYSCALL] ) = strlen( linux24_syscall_name_g[ SYS_socketcall ] );
    ;}
    break;

  case 67:
#line 436 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.socket_call) = SOCKET_ACCEPT;  ;}
    break;

  case 68:
#line 438 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.socket_call) = SOCKET_CONNECT; ;}
    break;

  case 69:
#line 440 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.socket_call) = SOCKET_UNKNOWN; ;}
    break;

  case 70:
#line 447 "/home/olivain/work/orchids/src/modules/snare.y"
    {
      DPRINTF( ("set destpath\n") );
      fields_g[F_DSTPATH] = ovm_str_new(strlen((yyvsp[-1].str)));
      memcpy( STR(fields_g[F_DSTPATH]), (yyvsp[-1].str), strlen((yyvsp[-1].str)));
      free((yyvsp[-1].str)); /* XXX: compute offsets and sizes instead of dup()ing strings */
      fields_g[F_SYSCALL] = ovm_vstr_new();
      VSTR( fields_g[F_SYSCALL] ) = linux24_syscall_name_g[ (yyvsp[-12].syscall) ];
      VSTRLEN( fields_g[F_SYSCALL] ) = strlen( linux24_syscall_name_g[ (yyvsp[-12].syscall) ] );
    ;}
    break;

  case 71:
#line 460 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.syscall) = SYS_link;    ;}
    break;

  case 72:
#line 462 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.syscall) = SYS_mount;   ;}
    break;

  case 73:
#line 464 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.syscall) = SYS_rename;  ;}
    break;

  case 74:
#line 466 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.syscall) = SYS_symlink; ;}
    break;

  case 75:
#line 468 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.syscall) = SYS_umount;  ;}
    break;

  case 76:
#line 475 "/home/olivain/work/orchids/src/modules/snare.y"
    {
      /* DPRINTF( ("set token target\n") ); */
      fields_g[F_TARGETID] = ovm_int_new();
      INT(fields_g[F_TARGETID]) = (yyvsp[-2].integer);
      /* XXX */

      /* DPRINTF( ("setid_event\n") ); */
      fields_g[F_SYSCALL] = ovm_vstr_new();
      VSTR( fields_g[F_SYSCALL] ) = linux24_syscall_name_g[ (yyvsp[-14].syscall) ];
      VSTRLEN( fields_g[F_SYSCALL] ) = strlen( linux24_syscall_name_g[ (yyvsp[-14].syscall) ] );
    ;}
    break;

  case 77:
#line 489 "/home/olivain/work/orchids/src/modules/snare.y"
    {
      /* DPRINTF( ("set token target\n") ); */
      fields_g[F_TARGETID] = ovm_int_new();
      INT(fields_g[F_TARGETID]) = (yyvsp[-7].integer);
      fields_g[F_TARGETRID] = ovm_int_new();
      INT(fields_g[F_TARGETRID]) = (yyvsp[-2].integer);

      /* DPRINTF( ("setreid_event\n") ); */
      fields_g[F_SYSCALL] = ovm_vstr_new();
      VSTR( fields_g[F_SYSCALL] ) = linux24_syscall_name_g[ (yyvsp[-19].syscall) ];
      VSTRLEN( fields_g[F_SYSCALL] ) = strlen( linux24_syscall_name_g[ (yyvsp[-19].syscall) ] );
    ;}
    break;

  case 78:
#line 506 "/home/olivain/work/orchids/src/modules/snare.y"
    {
      /* DPRINTF( ("set token target\n") ); */
      fields_g[F_TARGETID] = ovm_int_new();
      INT(fields_g[F_TARGETID]) = (yyvsp[-12].integer);
      fields_g[F_TARGETRID] = ovm_int_new();
      INT(fields_g[F_TARGETRID]) = (yyvsp[-7].integer);
      fields_g[F_TARGETSID] = ovm_int_new();
      INT(fields_g[F_TARGETSID]) = (yyvsp[-2].integer);

      /* DPRINTF( ("setresid_event\n") ); */
      fields_g[F_SYSCALL] = ovm_vstr_new();
      VSTR( fields_g[F_SYSCALL] ) = linux24_syscall_name_g[ (yyvsp[-24].syscall) ];
      VSTRLEN( fields_g[F_SYSCALL] ) = strlen( linux24_syscall_name_g[ (yyvsp[-24].syscall) ] );
    ;}
    break;

  case 79:
#line 524 "/home/olivain/work/orchids/src/modules/snare.y"
    { /* do_nothing() */; ;}
    break;

  case 80:
#line 526 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.str) = (yyvsp[0].str); ;}
    break;

  case 81:
#line 531 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.syscall) = SYS_setuid;   ;}
    break;

  case 82:
#line 533 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.syscall) = SYS_setuid32; ;}
    break;

  case 83:
#line 535 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.syscall) = SYS_setgid;   ;}
    break;

  case 84:
#line 537 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.syscall) = SYS_setgid32; ;}
    break;

  case 85:
#line 542 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.syscall) = SYS_setreuid;   ;}
    break;

  case 86:
#line 544 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.syscall) = SYS_setreuid32; ;}
    break;

  case 87:
#line 546 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.syscall) = SYS_setregid;   ;}
    break;

  case 88:
#line 548 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.syscall) = SYS_setregid32; ;}
    break;

  case 89:
#line 553 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.syscall) = SYS_setresuid;   ;}
    break;

  case 90:
#line 555 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.syscall) = SYS_setresuid32; ;}
    break;

  case 91:
#line 557 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.syscall) = SYS_setresgid;   ;}
    break;

  case 92:
#line 559 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.syscall) = SYS_setresgid32; ;}
    break;

  case 93:
#line 565 "/home/olivain/work/orchids/src/modules/snare.y"
    {
      DPRINTF( ("kern_event\n") );
      fields_g[F_SYSCALL] = ovm_vstr_new();
      VSTR( fields_g[F_SYSCALL] ) = linux24_syscall_name_g[ (yyvsp[-9].syscall) ];
      VSTRLEN( fields_g[F_SYSCALL] ) = strlen( linux24_syscall_name_g[ (yyvsp[-9].syscall) ] );
    ;}
    break;

  case 94:
#line 575 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.syscall) = SYS_create_module; ;}
    break;

  case 95:
#line 577 "/home/olivain/work/orchids/src/modules/snare.y"
    { (yyval.syscall) = SYS_delete_module; ;}
    break;

  case 96:
#line 582 "/home/olivain/work/orchids/src/modules/snare.y"
    {
      fields_g[F_PATH] = ovm_str_new(strlen((yyvsp[-1].str)));
      memcpy( STR(fields_g[F_PATH]), (yyvsp[-1].str), strlen((yyvsp[-1].str)));
      free((yyvsp[-1].str)); /* XXX: compute offsets and sizes instead of dup()ing strings */
    ;}
    break;

  case 97:
#line 591 "/home/olivain/work/orchids/src/modules/snare.y"
    {
      fields_g[F_PID] = ovm_int_new();
      INT(fields_g[F_PID]) = (yyvsp[-3].integer);
      fields_g[F_PROCNAME] = ovm_str_new( strlen((yyvsp[-1].str)) );
      memcpy( STR(fields_g[F_PROCNAME]), (yyvsp[-1].str), strlen((yyvsp[-1].str)));
      free((yyvsp[-1].str)); /* XXX: compute offsets and sizes instead of dup()ing strings */
    ;}
    break;

  case 98:
#line 606 "/home/olivain/work/orchids/src/modules/snare.y"
    {
      fields_g[F_RUID] = ovm_int_new();
      INT(fields_g[F_RUID]) = (yyvsp[-17].integer);
      fields_g[F_RGID] = ovm_int_new();
      INT(fields_g[F_RGID]) = (yyvsp[-12].integer);
      fields_g[F_EUID] = ovm_int_new();
      INT(fields_g[F_EUID]) = (yyvsp[-7].integer);
      fields_g[F_EGID] = ovm_int_new();
      INT(fields_g[F_EGID]) = (yyvsp[-2].integer);
    ;}
    break;

  case 99:
#line 620 "/home/olivain/work/orchids/src/modules/snare.y"
    {
      fields_g[F_TIME] = ovm_ctime_new();
      CTIME(fields_g[F_TIME]) = (yyvsp[0].time);
      fields_g[F_TIME]->flags |= TYPE_MONO;
    ;}
    break;


    }

/* Line 1037 of yacc.c.  */
#line 2150 "/home/olivain/work/orchids/src/modules/snare.tab.c"

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


#line 627 "/home/olivain/work/orchids/src/modules/snare.y"


void
snareparse_set_attrs(ovm_var_t **attr_fields)
{
  fields_g = attr_fields;
}


void
snareerror(char *s)
{
  DebugLog(DF_MOD, DS_WARN, "%s at '%s'.\n", s, snaretext);
}


static char *linux24_syscall_name_g[256] = {
  "(0) ***",
  "(1) SYS_exit",
  "(2) SYS_fork",
  "(3) SYS_read",
  "(4) SYS_write",
  "(5) SYS_open",
  "(6) SYS_close",
  "(7) SYS_waitpid",
  "(8) SYS_creat",
  "(9) SYS_link",
  "(10) SYS_unlink",
  "(11) SYS_execve",
  "(12) SYS_chdir",
  "(13) SYS_time",
  "(14) SYS_mknod",
  "(15) SYS_chmod",
  "(16) SYS_lchown",
  "(17) SYS_break",
  "(18) SYS_oldstat",
  "(19) SYS_lseek",
  "(20) SYS_getpid",
  "(21) SYS_mount",
  "(22) SYS_umount",
  "(23) SYS_setuid",
  "(24) SYS_getuid",
  "(25) SYS_stime",
  "(26) SYS_ptrace",
  "(27) SYS_alarm",
  "(28) SYS_oldfstat",
  "(29) SYS_pause",
  "(30) SYS_utime",
  "(31) SYS_stty",
  "(32) SYS_gtty",
  "(33) SYS_access",
  "(34) SYS_nice",
  "(35) SYS_ftime",
  "(36) SYS_sync",
  "(37) SYS_kill",
  "(38) SYS_rename",
  "(39) SYS_mkdir",
  "(40) SYS_rmdir",
  "(41) SYS_dup",
  "(42) SYS_pipe",
  "(43) SYS_times",
  "(44) SYS_prof",
  "(45) SYS_brk",
  "(46) SYS_setgid",
  "(47) SYS_getgid",
  "(48) SYS_signal",
  "(49) SYS_geteuid",
  "(50) SYS_getegid",
  "(51) SYS_acct",
  "(52) SYS_umount2",
  "(53) SYS_lock",
  "(54) SYS_ioctl",
  "(55) SYS_fcntl",
  "(56) SYS_mpx",
  "(57) SYS_setpgid",
  "(58) SYS_ulimit",
  "(59) SYS_oldolduname",
  "(60) SYS_umask",
  "(61) SYS_chroot",
  "(62) SYS_ustat",
  "(63) SYS_dup2",
  "(64) SYS_getppid",
  "(65) SYS_getpgrp",
  "(66) SYS_setsid",
  "(67) SYS_sigaction",
  "(68) SYS_sgetmask",
  "(69) SYS_ssetmask",
  "(70) SYS_setreuid",
  "(71) SYS_setregid",
  "(72) SYS_sigsuspend",
  "(73) SYS_sigpending",
  "(74) SYS_sethostname",
  "(75) SYS_setrlimit",
  "(76) SYS_getrlimit",
  "(77) SYS_getrusage",
  "(78) SYS_gettimeofday",
  "(79) SYS_settimeofday",
  "(80) SYS_getgroups",
  "(81) SYS_setgroups",
  "(82) SYS_select",
  "(83) SYS_symlink",
  "(84) SYS_oldlstat",
  "(85) SYS_readlink",
  "(86) SYS_uselib",
  "(87) SYS_swapon",
  "(88) SYS_reboot",
  "(89) SYS_readdir",
  "(90) SYS_mmap",
  "(91) SYS_munmap",
  "(92) SYS_truncate",
  "(93) SYS_ftruncate",
  "(94) SYS_fchmod",
  "(95) SYS_fchown",
  "(96) SYS_getpriority",
  "(97) SYS_setpriority",
  "(98) SYS_profil",
  "(99) SYS_statfs",
  "(100) SYS_fstatfs",
  "(101) SYS_ioperm",
  "(102) SYS_socketcall",
  "(103) SYS_syslog",
  "(104) SYS_setitimer",
  "(105) SYS_getitimer",
  "(106) SYS_stat",
  "(107) SYS_lstat",
  "(108) SYS_fstat",
  "(109) SYS_olduname",
  "(110) SYS_iopl",
  "(111) SYS_vhangup",
  "(112) SYS_idle",
  "(113) SYS_vm86old",
  "(114) SYS_wait4",
  "(115) SYS_swapoff",
  "(116) SYS_sysinfo",
  "(117) SYS_ipc",
  "(118) SYS_fsync",
  "(119) SYS_sigreturn",
  "(120) SYS_clone",
  "(121) SYS_setdomainname",
  "(122) SYS_uname",
  "(123) SYS_modify_ldt",
  "(124) SYS_adjtimex",
  "(125) SYS_mprotect",
  "(126) SYS_sigprocmask",
  "(127) SYS_create_module",
  "(128) SYS_init_module",
  "(129) SYS_delete_module",
  "(130) SYS_get_kernel_syms",
  "(131) SYS_quotactl",
  "(132) SYS_getpgid",
  "(133) SYS_fchdir",
  "(134) SYS_bdflush",
  "(135) SYS_sysfs",
  "(136) SYS_personality",
  "(137) SYS_afs_syscall",
  "(138) SYS_setfsuid",
  "(139) SYS_setfsgid",
  "(140) SYS__llseek",
  "(141) SYS_getdents",
  "(142) SYS__newselect",
  "(143) SYS_flock",
  "(144) SYS_msync",
  "(145) SYS_readv",
  "(146) SYS_writev",
  "(147) SYS_getsid",
  "(148) SYS_fdatasync",
  "(149) SYS__sysctl",
  "(150) SYS_mlock",
  "(151) SYS_munlock",
  "(152) SYS_mlockall",
  "(153) SYS_munlockall",
  "(154) SYS_sched_setparam",
  "(155) SYS_sched_getparam",
  "(156) SYS_sched_setscheduler",
  "(157) SYS_sched_getscheduler",
  "(158) SYS_sched_yield",
  "(159) SYS_sched_get_priority_max",
  "(160) SYS_sched_get_priority_min",
  "(161) SYS_sched_rr_get_interval",
  "(162) SYS_nanosleep",
  "(163) SYS_mremap",
  "(164) SYS_setresuid",
  "(165) SYS_getresuid",
  "(166) SYS_vm86",
  "(167) SYS_query_module",
  "(168) SYS_poll",
  "(169) SYS_nfsservctl",
  "(170) SYS_setresgid",
  "(171) SYS_getresgid",
  "(172) SYS_prctl",
  "(173) SYS_rt_sigreturn",
  "(174) SYS_rt_sigaction",
  "(175) SYS_rt_sigprocmask",
  "(176) SYS_rt_sigpending",
  "(177) SYS_rt_sigtimedwait",
  "(178) SYS_rt_sigqueueinfo",
  "(179) SYS_rt_sigsuspend",
  "(180) SYS_pread",
  "(181) SYS_pwrite",
  "(182) SYS_chown",
  "(183) SYS_getcwd",
  "(184) SYS_capget",
  "(185) SYS_capset",
  "(186) SYS_sigaltstack",
  "(187) SYS_sendfile",
  "(188) SYS_getpmsg",
  "(189) SYS_putpmsg",
  "(190) SYS_vfork",
  "(191) SYS_ugetrlimit",
  "(192) SYS_mmap2",
  "(193) SYS_truncate64",
  "(194) SYS_ftruncate64",
  "(195) SYS_stat64",
  "(196) SYS_lstat64",
  "(197) SYS_fstat64",
  "(198) SYS_lchown32",
  "(199) SYS_getuid32",
  "(200) SYS_getgid32",
  "(201) SYS_geteuid32",
  "(202) SYS_getegid32",
  "(203) SYS_setreuid32",
  "(204) SYS_setregid32",
  "(205) SYS_getgroups32",
  "(206) SYS_setgroups32",
  "(207) SYS_fchown32",
  "(208) SYS_setresuid32",
  "(209) SYS_getresuid32",
  "(210) SYS_setresgid32",
  "(211) SYS_getresgid32",
  "(212) SYS_chown32",
  "(213) SYS_setuid32",
  "(214) SYS_setgid32",
  "(215) SYS_setfsuid32",
  "(216) SYS_setfsgid32",
  "(217) SYS_pivot_root",
  "(218) SYS_mincore",
  "(219) SYS_madvise",
  "(220) SYS_madvise1",
  "(221) SYS_getdents64",
  "(222) SYS_fcntl64",
  "(223) SYS_security",
  "(224) SYS_gettid",
  "(225) SYS_readahead",
  "(226) SYS_setxattr",
  "(227) SYS_lsetxattr",
  "(228) SYS_fsetxattr",
  "(229) SYS_getxattr",
  "(230) SYS_lgetxattr",
  "(231) SYS_fgetxattr",
  "(232) SYS_listxattr",
  "(233) SYS_llistxattr",
  "(234) SYS_flistxattr",
  "(235) SYS_removexattr",
  "(236) SYS_lremovexattr",
  "(237) SYS_fremovexattr",
  "(238) SYS_tkill",
  "(239) SYS_sendfile64",
  "(240) SYS_futex",
  "(241) SYS_sched_setaffinity",
  "(242) SYS_sched_getaffinity",
  NULL
};

static char *linux24_sockcall_name_g[4] = {
  "(5) SYS_ACCEPT",
  "(3) SYS_CONNECT",
  "(0) Unknown",
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

