%{
/**
 ** @file snare.y
 ** Snare yaccer.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 **
 ** @date  Started on: Thu Sep 11 18:37:40 2003
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

#ifdef __APPLE__
#define __APPLE_API_PRIVATE
  /* to access syscall names SYS_* in sys/syscall.h */
#endif

#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/stat.h>

#include <netinet/in.h>

#ifdef __APPLE__
  // On __APPLE__ machines (at least on __MACOSX__), a few syscalls are undefined
  // Anyway, snare is not available here.  So keep compiler happy:
#define SYS_creat -1
#define SYS_setresuid -1
#define SYS_setresgid -1
#define SYS_create_module -1
#define SYS_delete_module -1
# define SYS_truncate64	 -1
# define SYS_ftruncate64 -1
# define SYS_chown32	 -1
# define SYS_lchown32	 -1
# define SYS_socketcall  -1
# define SYS_umount      -1
# define SYS_setuid32    -1
# define SYS_setgid32    -1
# define SYS_setreuid32  -1
# define SYS_setregid32  -1
# define SYS_setresuid32 -1
# define SYS_setresgid32 -1
#elif __WORDSIZE == 64
// On a 64 bit architecture, the following syscalls are not present
# define SYS_truncate64	 -1
# define SYS_ftruncate64 -1
# define SYS_chown32	 -1
# define SYS_lchown32	 -1
# define SYS_socketcall  -1
# define SYS_umount      -1
# define SYS_setuid32    -1
# define SYS_setgid32    -1
# define SYS_setreuid32  -1
# define SYS_setregid32  -1
# define SYS_setresuid32 -1
# define SYS_setresgid32 -1
#endif


#include "orchids.h"

/* from  /usr/include/bits/fcntl.h or /usr/include/sys/fcntl.h, should be
 included automatically from fcntl.h */
//#define O_DIRECT        040000 /* Direct disk access.  */  UNUSED
#ifndef O_DIRECTORY
#define O_DIRECTORY    0200000 /* Must be a directory.  */
#endif
#ifndef O_NOFOLLOW
#define O_NOFOLLOW     0400000 /* Do not follow links.  */
#endif
#ifndef O_LARGEFILE
#define O_LARGEFILE    0100000
#endif

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

%}

%union {
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
}

%token LINUX_AUDIT TOKEN_EVENT TOKEN_USER TOKEN_PROCESS TOKEN_PATH
%token TOKEN_OWNER TOKEN_ATTRIBUTES TOKEN_ARGUMENTS TOKEN_TARGET
%token TOKEN_DESTPATH TOKEN_SOCKET
%token TOKEN_RETURN TOKEN_SEQUENCE

%token <time> TOKEN_DATE

%token <open_mode> OPT_WRONLY OPT_RDONLY OPT_RDWR

%token <open_flags> OPT_CREAT OPT_EXCL OPT_NOCTTY OPT_TRUNC OPT_APPEND
%token <open_flags> OPT_NONBLOCK OPT_SYNC OPT_NOFOLLOW OPT_DIRECTORY
%token <open_flags> OPT_LARGEFILE

%token <mknod_opt> OPT_IFREG OPT_IFCHR OPT_IFBLK OPT_IFIFO

%token <syscall> SYS_OPEN SYS_CREAT SYS_MKDIR SYS_UNLINK SYS_MKNOD SYS_RMDIR
%token <syscall> SYS_CHMOD SYS_TRUNCATE SYS_CHROOT SYS_FTRUNCATE SYS_FCHMOD
%token <syscall> SYS_CHOWN SYS_LCHOWN SYS_FCHOWN SYS_EXECVE SYS_EXIT
%token <syscall> SYS_SYMLINK SYS_LINK SYS_RENAME SYS_MOUNT SYS_UMOUNT
%token <syscall> SYS_SETUID SYS_SETREUID SYS_SETRESUID SYS_SETGID SYS_SETREGID
%token <syscall> SYS_SETRESGID SYS_LCHOWN32 SYS_CHOWN32 SYS_SETUID32
%token <syscall> SYS_SETREUID32 SYS_SETRESUID32 SYS_SETGID32 SYS_SETREGID32
%token <syscall> SYS_SETRESGID32 SYS_CREATEMODULE SYS_DELETEMODULE
%token <syscall> SYS_FTRUNCATE64 SYS_TRUNCATE64 SYS_REBOOT SYS_SOCKETCALL

%token <str> STRING DATA_STRING
%token <integer> INTEGER
%token <ipv4addr> INETADDR

%token <mode> UNIXPERM

%token SOCK_ACCEPT SOCK_CONNECT SOCK_UNKNOWN

%token TOKEN_DAY TOKEN_MONTH

%type <syscall> io_syscall trunc_syscall chown_syscall proc_syscall
%type <syscall> copy_syscall setid_syscall setreid_syscall setresid_syscall
%type <syscall> kern_syscall perm_io_syscall

%type <open_flags> sys_open_opts sys_open_opt sys_open_optlist
%type <open_mode> sys_open_mode

%type <mknod_opt> sys_mknod_opt sys_mknod_optlist sys_mknod_opts

%type <socket_call> socketcall_opt 

%type <str> opt_username

%%

snareline:
  STRING '\t' LINUX_AUDIT '\t'
  TOKEN_EVENT ',' event
  TOKEN_RETURN ',' INTEGER '\t'
  TOKEN_SEQUENCE ',' INTEGER
    {
      fields_g[F_RETCODE] = ovm_int_new();
      INT(fields_g[F_RETCODE]) = $10;
      fields_g[F_SEQUENCE] = ovm_int_new();
      INT(fields_g[F_SEQUENCE]) = $14;
      fields_g[F_SEQUENCE]->flags |= TYPE_MONO;
    }
;

event:
  io_event
| chown_event
| exec_event
| proc_event
| net_event
| copy_event
| setid_event
| kern_event
;

io_event:
  SYS_OPEN '(' sys_open_mode sys_open_opts ')' ',' date '\t'
  field_user field_process field_path perm_attributes
    {
      fields_g[F_MODE] = ovm_int_new();
      INT(fields_g[F_MODE]) = $3 | $4;
      fields_g[F_SYSCALL] = ovm_vstr_new();
      VSTR( fields_g[F_SYSCALL] ) = linux24_syscall_name_g[ SYS_open ];
      VSTRLEN( fields_g[F_SYSCALL] ) = strlen( linux24_syscall_name_g[ SYS_open ] );
    }
| perm_io_syscall '(' ')' ',' date '\t'
  field_user field_process field_path perm_attributes
    {
      fields_g[F_SYSCALL] = ovm_vstr_new();
      VSTR( fields_g[F_SYSCALL] ) = linux24_syscall_name_g[ $1 ];
      VSTRLEN( fields_g[F_SYSCALL] ) = strlen( linux24_syscall_name_g[ $1 ] );
    }
| SYS_MKNOD '(' sys_mknod_opts ')' ',' date '\t'
  field_user field_process field_path dev_attributes
    {
      DPRINTF( ("set io_event mknod()\n") );
      fields_g[F_SYSCALL] = ovm_vstr_new();
      VSTR( fields_g[F_SYSCALL] ) = linux24_syscall_name_g[ SYS_mknod ];
      VSTRLEN( fields_g[F_SYSCALL] ) = strlen( linux24_syscall_name_g[ SYS_mknod ] );
      /* XXX set mknod opts */
    }
| trunc_syscall '(' sys_mknod_opts ')' ',' date '\t'
  field_user field_process field_path
  TOKEN_ATTRIBUTES ',' INTEGER '\t'
    {
      DPRINTF( ("set trunc attributes\n") );
      fields_g[F_OFFSET] = ovm_int_new();
      INT(fields_g[F_OFFSET]) = $13;
      DPRINTF( ("set io_event syscall (f)truncate(64)()\n") );
      fields_g[F_SYSCALL] = ovm_vstr_new();
      VSTR( fields_g[F_SYSCALL] ) = linux24_syscall_name_g[ $1 ];
      VSTRLEN( fields_g[F_SYSCALL] ) = strlen( linux24_syscall_name_g[ $1 ] );
    }
| io_syscall '(' ')' ',' date '\t'
  field_user field_process field_path
    {
      fields_g[F_SYSCALL] = ovm_vstr_new();
      VSTR( fields_g[F_SYSCALL] ) = linux24_syscall_name_g[ $1 ];
      VSTRLEN( fields_g[F_SYSCALL] ) = strlen( linux24_syscall_name_g[ $1 ] );
    }
;

perm_io_syscall:
  SYS_MKDIR
    { $$ = SYS_mkdir;  }
| SYS_CHMOD
    { $$ = SYS_chmod;  }
| SYS_FCHMOD
    { $$ = SYS_fchmod; }
;

io_syscall:
  SYS_UNLINK
    { $$ = SYS_unlink; }
| SYS_CHROOT
    { $$ = SYS_chroot; }
| SYS_CREAT
    { $$ = SYS_creat;  }
| SYS_RMDIR
    { $$ = SYS_rmdir;  }
;

perm_attributes:
  /* empty */
    { /* do_nothing() */; }
| TOKEN_ATTRIBUTES ',' UNIXPERM '\t'
    {
      fields_g[F_CREATEMODE] = ovm_int_new();
      INT(fields_g[F_CREATEMODE]) = $3;
    }
;

dev_attributes:
  /* empty */
    { /* do_nothing() */; }
| TOKEN_ATTRIBUTES ',' INTEGER ' ' INTEGER '\t'
    {
      DPRINTF( ("set device attribute") );
      fields_g[F_DEVMAJ] = ovm_int_new();
      INT(fields_g[F_DEVMAJ]) = $3;
      fields_g[F_DEVMIN] = ovm_int_new();
      INT(fields_g[F_DEVMIN]) = $5;
    }
;

sys_open_mode:
  OPT_WRONLY
    { $$ = O_WRONLY; }
| OPT_RDONLY
    { $$ = O_RDONLY; }
| OPT_RDWR
    { $$ = O_RDWR;   }
;

sys_open_opts:
  /* empty */
    { $$ = 0;  }
| '|' sys_open_optlist
    { $$ = $2; }
;

sys_open_optlist:
  sys_open_optlist '|' sys_open_opt
    { $$ = $1 | $3; }
| sys_open_opt
    { $$ = $1;      }
;

sys_open_opt:
  OPT_CREAT
    { $$ = O_CREAT;     }
| OPT_EXCL
    { $$ = O_EXCL;      }
| OPT_NOCTTY
    { $$ = O_NOCTTY;    }
| OPT_TRUNC
    { $$ = O_TRUNC;     }
| OPT_APPEND
    { $$ = O_APPEND;    }
| OPT_NONBLOCK
    { $$ = O_NONBLOCK;  }
| OPT_SYNC
    { $$ = O_SYNC;      }
| OPT_NOFOLLOW
    { $$ = O_NOFOLLOW;  }
| OPT_DIRECTORY
    { $$ = O_DIRECTORY; }
| OPT_LARGEFILE
    { $$ = O_LARGEFILE; }
;

sys_mknod_opts:
  /* empty */
    { $$ = 0; }
| sys_mknod_optlist
    { $$ = $1; }
;

sys_mknod_optlist:
  sys_mknod_optlist '|' sys_mknod_opt
    { $$ = $1 | $3; }
| sys_mknod_opt
    { $$ = $1; }
;

sys_mknod_opt:
  OPT_IFREG
    { $$ = S_IFREG; }
| OPT_IFCHR
    { $$ = S_IFCHR; }
| OPT_IFBLK
    { $$ = S_IFBLK; }
| OPT_IFIFO
    { $$ = S_IFIFO; }
;

trunc_syscall:
  SYS_TRUNCATE
    { $$ = SYS_truncate; }
| SYS_TRUNCATE64
    { $$ = SYS_truncate64; }
| SYS_FTRUNCATE
    { $$ = SYS_ftruncate; }
| SYS_FTRUNCATE64
    { $$ = SYS_ftruncate64; }
;

chown_event:
  chown_syscall '(' ')' ',' date '\t'
  field_user field_process field_path
  TOKEN_OWNER ',' STRING '(' INTEGER ')' ',' opt_username '(' INTEGER ')' '\t'
    {
      fields_g[F_OWNERUID] = ovm_int_new();
      INT(fields_g[F_OWNERUID]) = $14;
      fields_g[F_OWNERGID] = ovm_int_new();
      INT(fields_g[F_OWNERGID]) = $19;
      fields_g[F_SYSCALL] = ovm_vstr_new();
      VSTR( fields_g[F_SYSCALL] ) = linux24_syscall_name_g[ $1 ];
      VSTRLEN( fields_g[F_SYSCALL] ) = strlen( linux24_syscall_name_g[ $1 ] );
    }
;

chown_syscall:
  SYS_CHOWN
    { $$ = SYS_chown;    }
| SYS_CHOWN32
    { $$ = SYS_chown32;  }
| SYS_LCHOWN
    { $$ = SYS_lchown;   }
| SYS_LCHOWN32
    { $$ = SYS_lchown32; }
| SYS_FCHOWN
    { $$ = SYS_fchown;   }
;

exec_event:
  SYS_EXECVE '(' ')' ',' date '\t'
  field_user field_process field_path
  TOKEN_ARGUMENTS ',' DATA_STRING '\t'
    {
      fields_g[F_CMDLINE] = ovm_str_new( strlen($12) );
      memcpy( STR(fields_g[F_CMDLINE]), $12, strlen($12));
      free($12); /* XXX: compute offsets and sizes instead of dup()ing strings */
      fields_g[F_SYSCALL] = ovm_vstr_new();
      VSTR( fields_g[F_SYSCALL] ) = linux24_syscall_name_g[ SYS_execve ];
      VSTRLEN( fields_g[F_SYSCALL] ) = strlen( linux24_syscall_name_g[ SYS_execve ] );
    }
;

proc_event:
  proc_syscall '(' ')' ',' date '\t'
  field_user field_process
    {
      fields_g[F_SYSCALL] = ovm_vstr_new();
      VSTR( fields_g[F_SYSCALL] ) = linux24_syscall_name_g[ $1 ];
      VSTRLEN( fields_g[F_SYSCALL] ) = strlen( linux24_syscall_name_g[ $1 ] );
    }
;

proc_syscall:
  SYS_EXIT
    { $$ = SYS_exit;   }
| SYS_REBOOT
    { $$ = SYS_reboot; }
;

net_event:
  SYS_SOCKETCALL '(' socketcall_opt ')' ',' date '\t'
  field_user field_process
  TOKEN_SOCKET ',' INETADDR ',' INETADDR ',' INTEGER ',' INTEGER '\t'
    {
      fields_g[F_SRCIP] = ovm_ipv4_new();
      IPV4( fields_g[F_SRCIP] ) = $12;
      fields_g[F_DSTIP] = ovm_ipv4_new();
      IPV4( fields_g[F_DSTIP] ) = $14;
      fields_g[F_SRCPORT] = ovm_int_new();
      INT(fields_g[F_SRCPORT]) = $16;
      fields_g[F_DSTPORT] = ovm_int_new();
      INT(fields_g[F_DSTPORT]) = $18;

      fields_g[F_SOCKCALL] = ovm_vstr_new();
      VSTR( fields_g[F_SOCKCALL] ) = linux24_sockcall_name_g[ $3 ];
      VSTRLEN( fields_g[F_SOCKCALL] ) = strlen( linux24_sockcall_name_g[ $3 ] );

      fields_g[F_SYSCALL] = ovm_vstr_new();
      VSTR( fields_g[F_SYSCALL] ) = linux24_syscall_name_g[ SYS_socketcall ];
      VSTRLEN( fields_g[F_SYSCALL] ) = strlen( linux24_syscall_name_g[ SYS_socketcall ] );
    }
;

socketcall_opt:
  SOCK_ACCEPT
    { $$ = SOCKET_ACCEPT;  }
| SOCK_CONNECT
    { $$ = SOCKET_CONNECT; }
| SOCK_UNKNOWN
    { $$ = SOCKET_UNKNOWN; }
;

copy_event:
  copy_syscall '(' ')' ',' date '\t'
  field_user field_process field_path
  TOKEN_DESTPATH ',' DATA_STRING '\t'
    {
      DPRINTF( ("set destpath\n") );
      fields_g[F_DSTPATH] = ovm_str_new(strlen($12));
      memcpy( STR(fields_g[F_DSTPATH]), $12, strlen($12));
      free($12); /* XXX: compute offsets and sizes instead of dup()ing strings */
      fields_g[F_SYSCALL] = ovm_vstr_new();
      VSTR( fields_g[F_SYSCALL] ) = linux24_syscall_name_g[ $1 ];
      VSTRLEN( fields_g[F_SYSCALL] ) = strlen( linux24_syscall_name_g[ $1 ] );
    }
;

copy_syscall:
  SYS_LINK
    { $$ = SYS_link;    }
| SYS_MOUNT
    { $$ = SYS_mount;   }
| SYS_RENAME
    { $$ = SYS_rename;  }
| SYS_SYMLINK
    { $$ = SYS_symlink; }
| SYS_UMOUNT
    { $$ = SYS_umount;  }
;

setid_event:
  setid_syscall '(' ')' ',' date '\t'
  field_user field_process
  TOKEN_TARGET ',' STRING '(' INTEGER ')' '\t'
    {
      /* DPRINTF( ("set token target\n") ); */
      fields_g[F_TARGETID] = ovm_int_new();
      INT(fields_g[F_TARGETID]) = $13;
      /* XXX */

      /* DPRINTF( ("setid_event\n") ); */
      fields_g[F_SYSCALL] = ovm_vstr_new();
      VSTR( fields_g[F_SYSCALL] ) = linux24_syscall_name_g[ $1 ];
      VSTRLEN( fields_g[F_SYSCALL] ) = strlen( linux24_syscall_name_g[ $1 ] );
    }
| setreid_syscall '(' ')' ',' date '\t'
  field_user field_process
  TOKEN_TARGET ',' STRING '(' INTEGER ')' ',' opt_username '(' INTEGER ')' '\t'
    {
      /* DPRINTF( ("set token target\n") ); */
      fields_g[F_TARGETID] = ovm_int_new();
      INT(fields_g[F_TARGETID]) = $13;
      fields_g[F_TARGETRID] = ovm_int_new();
      INT(fields_g[F_TARGETRID]) = $18;

      /* DPRINTF( ("setreid_event\n") ); */
      fields_g[F_SYSCALL] = ovm_vstr_new();
      VSTR( fields_g[F_SYSCALL] ) = linux24_syscall_name_g[ $1 ];
      VSTRLEN( fields_g[F_SYSCALL] ) = strlen( linux24_syscall_name_g[ $1 ] );
    }
| setresid_syscall '(' ')' ',' date '\t'
  field_user field_process
  TOKEN_TARGET ',' STRING '(' INTEGER ')' ','
             opt_username '(' INTEGER ')' ','
             opt_username '(' INTEGER ')' '\t'
    {
      /* DPRINTF( ("set token target\n") ); */
      fields_g[F_TARGETID] = ovm_int_new();
      INT(fields_g[F_TARGETID]) = $13;
      fields_g[F_TARGETRID] = ovm_int_new();
      INT(fields_g[F_TARGETRID]) = $18;
      fields_g[F_TARGETSID] = ovm_int_new();
      INT(fields_g[F_TARGETSID]) = $23;

      /* DPRINTF( ("setresid_event\n") ); */
      fields_g[F_SYSCALL] = ovm_vstr_new();
      VSTR( fields_g[F_SYSCALL] ) = linux24_syscall_name_g[ $1 ];
      VSTRLEN( fields_g[F_SYSCALL] ) = strlen( linux24_syscall_name_g[ $1 ] );
    }
;

opt_username:
  /* empty */
    { /* do_nothing() */; }
| STRING
    { $$ = $1; }
;

setid_syscall:
  SYS_SETUID
    { $$ = SYS_setuid;   }
| SYS_SETUID32
    { $$ = SYS_setuid32; }
| SYS_SETGID
    { $$ = SYS_setgid;   }
| SYS_SETGID32
    { $$ = SYS_setgid32; }
;

setreid_syscall:
  SYS_SETREUID
    { $$ = SYS_setreuid;   }
| SYS_SETREUID32
    { $$ = SYS_setreuid32; }
| SYS_SETREGID
    { $$ = SYS_setregid;   }
| SYS_SETREGID32
    { $$ = SYS_setregid32; }
;

setresid_syscall:
  SYS_SETRESUID
    { $$ = SYS_setresuid;   }
| SYS_SETRESUID32
    { $$ = SYS_setresuid32; }
| SYS_SETRESGID
    { $$ = SYS_setresgid;   }
| SYS_SETRESGID32
    { $$ = SYS_setresgid32; }
;

kern_event:
  kern_syscall '(' ')' ',' date '\t'
  field_user field_process field_path '\t'
    {
      DPRINTF( ("kern_event\n") );
      fields_g[F_SYSCALL] = ovm_vstr_new();
      VSTR( fields_g[F_SYSCALL] ) = linux24_syscall_name_g[ $1 ];
      VSTRLEN( fields_g[F_SYSCALL] ) = strlen( linux24_syscall_name_g[ $1 ] );
    }
;

kern_syscall:
  SYS_CREATEMODULE
    { $$ = SYS_create_module; }
| SYS_DELETEMODULE
    { $$ = SYS_delete_module; }
;

field_path:
  TOKEN_PATH ',' DATA_STRING '\t'
    {
      fields_g[F_PATH] = ovm_str_new(strlen($3));
      memcpy( STR(fields_g[F_PATH]), $3, strlen($3));
      free($3); /* XXX: compute offsets and sizes instead of dup()ing strings */
    }
;

field_process:
  TOKEN_PROCESS ',' INTEGER ',' STRING '\t'
    {
      fields_g[F_PID] = ovm_int_new();
      INT(fields_g[F_PID]) = $3;
      fields_g[F_PROCNAME] = ovm_str_new( strlen($5) );
      memcpy( STR(fields_g[F_PROCNAME]), $5, strlen($5));
      free($5); /* XXX: compute offsets and sizes instead of dup()ing strings */
    }
;

field_user:
  TOKEN_USER ',' 
  STRING '(' INTEGER ')' ','
  STRING '(' INTEGER ')' ','
  STRING '(' INTEGER ')' ','
  STRING '(' INTEGER ')' '\t'
    {
      fields_g[F_RUID] = ovm_int_new();
      INT(fields_g[F_RUID]) = $5;
      fields_g[F_RGID] = ovm_int_new();
      INT(fields_g[F_RGID]) = $10;
      fields_g[F_EUID] = ovm_int_new();
      INT(fields_g[F_EUID]) = $15;
      fields_g[F_EGID] = ovm_int_new();
      INT(fields_g[F_EGID]) = $20;
    }
;

date:
  TOKEN_DATE
    {
      fields_g[F_TIME] = ovm_ctime_new();
      CTIME(fields_g[F_TIME]) = $1;
      fields_g[F_TIME]->flags |= TYPE_MONO;
    }
;

%%

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
