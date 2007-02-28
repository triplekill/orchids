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
     CISCO_SEC = 258,
     CISCO_IPACCESSLOGDP = 259,
     CISCO_IPACCESSLOGNP = 260,
     CISCO_IPACCESSLOGRL = 261,
     CISCO_IPACCESSLOGP = 262,
     CISCO_IPACCESSLOGRP = 263,
     CISCO_IPACCESSLOGS = 264,
     CISCO_LIST = 265,
     CISCO_DENIED = 266,
     CISCO_PERMITTED = 267,
     CISCO_ICMP = 268,
     CISCO_IGMP = 269,
     CISCO_OSPF = 270,
     CISCO_TCP = 271,
     CISCO_UDP = 272,
     CISCO_ARROW = 273,
     CISCO_PACKETS = 274,
     CISCO_RLTEXT = 275,
     STRING = 276,
     INTEGER = 277,
     INETADDR = 278
   };
#endif
#define CISCO_SEC 258
#define CISCO_IPACCESSLOGDP 259
#define CISCO_IPACCESSLOGNP 260
#define CISCO_IPACCESSLOGRL 261
#define CISCO_IPACCESSLOGP 262
#define CISCO_IPACCESSLOGRP 263
#define CISCO_IPACCESSLOGS 264
#define CISCO_LIST 265
#define CISCO_DENIED 266
#define CISCO_PERMITTED 267
#define CISCO_ICMP 268
#define CISCO_IGMP 269
#define CISCO_OSPF 270
#define CISCO_TCP 271
#define CISCO_UDP 272
#define CISCO_ARROW 273
#define CISCO_PACKETS 274
#define CISCO_RLTEXT 275
#define STRING 276
#define INTEGER 277
#define INETADDR 278




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 36 "/home/olivain/work/orchids/src/modules/cisco.y"
typedef union YYSTYPE {
  int integer;
  char *string;
  struct in_addr ipv4addr;
} YYSTYPE;
/* Line 1318 of yacc.c.  */
#line 89 "/home/olivain/work/orchids/src/modules/cisco.tab.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE ciscolval;



