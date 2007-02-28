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
/* Line 1318 of yacc.c.  */
#line 218 "/home/olivain/work/orchids/src/modules/snare.tab.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE snarelval;



