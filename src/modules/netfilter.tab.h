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
/* Line 1318 of yacc.c.  */
#line 145 "/home/olivain/work/orchids/src/modules/netfilter.tab.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE netfilterlval;



