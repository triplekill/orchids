%{
/**
 ** @file snare.y
 ** Snare yaccer.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 ** @author Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
 **
 ** @version 0.2
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

#include <fcntl.h>
#include <sys/stat.h>
#include <netinet/in.h>

  // #include <sys/syscall.h>
  /* Do not included syscall.h, which is OS-dependent.
     Snare is for Linux 32-bit machines, for which the list of
     syscalls is the following. (This is also included in
     dist/rules/linux32syscall.h)
  */
#define SYS_restart_syscall     0
#define SYS_exit		1
#define SYS_fork		2
#define SYS_read		3
#define SYS_write		4
#define SYS_open		5
#define SYS_close		6
#define SYS_waitpid		7
#define SYS_creat		8
#define SYS_link		9
#define SYS_unlink		10
#define SYS_execve		11
#define SYS_chdir		12
#define SYS_time		13
#define SYS_mknod		14
#define SYS_chmod		15
#define SYS_lchown		16
#define SYS_break		17
#define SYS_oldstat		18
#define SYS_lseek		19
#define SYS_getpid		20
#define SYS_mount		21
#define SYS_umount		22
#define SYS_setuid		23
#define SYS_getuid		24
#define SYS_stime		25
#define SYS_ptrace		26
#define SYS_alarm		27
#define SYS_oldfstat		28
#define SYS_pause		29
#define SYS_utime		30
#define SYS_stty		31
#define SYS_gtty		32
#define SYS_access		33
#define SYS_nice		34
#define SYS_ftime		35
#define SYS_sync		36
#define SYS_kill		37
#define SYS_rename		38
#define SYS_mkdir		39
#define SYS_rmdir		40
#define SYS_dup			41
#define SYS_pipe		42
#define SYS_times		43
#define SYS_prof		44
#define SYS_brk			45
#define SYS_setgid		46
#define SYS_getgid		47
#define SYS_signal		48
#define SYS_geteuid		49
#define SYS_getegid		50
#define SYS_acct		51
#define SYS_umount2		52
#define SYS_lock		53
#define SYS_ioctl		54
#define SYS_fcntl		55
#define SYS_mpx			56
#define SYS_setpgid		57
#define SYS_ulimit		58
#define SYS_oldolduname		59
#define SYS_umask		60
#define SYS_chroot		61
#define SYS_ustat		62
#define SYS_dup2		63
#define SYS_getppid		64
#define SYS_getpgrp		65
#define SYS_setsid		66
#define SYS_sigaction		67
#define SYS_sgetmask		68
#define SYS_ssetmask		69
#define SYS_setreuid		70
#define SYS_setregid		71
#define SYS_sigsuspend		72
#define SYS_sigpending		73
#define SYS_sethostname		74
#define SYS_setrlimit		75
#define SYS_getrlimit		76   /* Back compatible 2Gig limited rlimit */
#define SYS_getrusage		77
#define SYS_gettimeofday	78
#define SYS_settimeofday	79
#define SYS_getgroups		80
#define SYS_setgroups		81
#define SYS_select		82
#define SYS_symlink		83
#define SYS_oldlstat		84
#define SYS_readlink		85
#define SYS_uselib		86
#define SYS_swapon		87
#define SYS_reboot		88
#define SYS_readdir		89
#define SYS_mmap		90
#define SYS_munmap		91
#define SYS_truncate		92
#define SYS_ftruncate		93
#define SYS_fchmod		94
#define SYS_fchown		95
#define SYS_getpriority		96
#define SYS_setpriority		97
#define SYS_profil		98
#define SYS_statfs		99
#define SYS_fstatfs		100
#define SYS_ioperm		101
#define SYS_socketcall		102
#define SYS_syslog		103
#define SYS_setitimer		104
#define SYS_getitimer		105
#define SYS_stat		106
#define SYS_lstat		107
#define SYS_fstat		108
#define SYS_olduname		109
#define SYS_iopl		110
#define SYS_vhangup		111
#define SYS_idle		112
#define SYS_vm86old		113
#define SYS_wait4		114
#define SYS_swapoff		115
#define SYS_sysinfo		116
#define SYS_ipc			117
#define SYS_fsync		118
#define SYS_sigreturn		119
#define SYS_clone		120
#define SYS_setdomainname	121
#define SYS_uname		122
#define SYS_modify_ldt		123
#define SYS_adjtimex		124
#define SYS_mprotect		125
#define SYS_sigprocmask		126
#define SYS_create_module	127
#define SYS_init_module		128
#define SYS_delete_module	129
#define SYS_get_kernel_syms	130
#define SYS_quotactl		131
#define SYS_getpgid		132
#define SYS_fchdir		133
#define SYS_bdflush		134
#define SYS_sysfs		135
#define SYS_personality		136
#define SYS_afs_syscall		137 /* Syscall for Andrew File System */
#define SYS_setfsuid		138
#define SYS_setfsgid		139
#define SYS__llseek		140
#define SYS_getdents		141
#define SYS__newselect		142
#define SYS_flock		143
#define SYS_msync		144
#define SYS_readv		145
#define SYS_writev		146
#define SYS_getsid		147
#define SYS_fdatasync		148
#define SYS__sysctl		149
#define SYS_mlock		150
#define SYS_munlock		151
#define SYS_mlockall		152
#define SYS_munlockall		153
#define SYS_sched_setparam	154
#define SYS_sched_getparam	155
#define SYS_sched_setscheduler	156
#define SYS_sched_getscheduler	157
#define SYS_sched_yield		158
#define SYS_sched_get_priority_max	159
#define SYS_sched_get_priority_min	160
#define SYS_sched_rr_get_interval	161
#define SYS_nanosleep		162
#define SYS_mremap		163
#define SYS_setresuid		164
#define SYS_getresuid		165
#define SYS_vm86		166
#define SYS_query_module	167
#define SYS_poll		168
#define SYS_nfsservctl		169
#define SYS_setresgid		170
#define SYS_getresgid		171
#define SYS_prctl		172
#define SYS_rt_sigreturn	173
#define SYS_rt_sigaction	174
#define SYS_rt_sigprocmask	175
#define SYS_rt_sigpending	176
#define SYS_rt_sigtimedwait	177
#define SYS_rt_sigqueueinfo	178
#define SYS_rt_sigsuspend	179
#define SYS_pread64		180
#define SYS_pwrite64		181
#define SYS_chown		182
#define SYS_getcwd		183
#define SYS_capget		184
#define SYS_capset		185
#define SYS_sigaltstack		186
#define SYS_sendfile		187
#define SYS_getpmsg		188
#define SYS_putpmsg		189
#define SYS_vfork		190
#define SYS_ugetrlimit		191	/* SuS compliant getrlimit */
#define SYS_mmap2		192
#define SYS_truncate64		193
#define SYS_ftruncate64		194
#define SYS_stat64		195
#define SYS_lstat64		196
#define SYS_fstat64		197
#define SYS_lchown32		198
#define SYS_getuid32		199
#define SYS_getgid32		200
#define SYS_geteuid32		201
#define SYS_getegid32		202
#define SYS_setreuid32		203
#define SYS_setregid32		204
#define SYS_getgroups32		205
#define SYS_setgroups32		206
#define SYS_fchown32		207
#define SYS_setresuid32		208
#define SYS_getresuid32		209
#define SYS_setresgid32		210
#define SYS_getresgid32		211
#define SYS_chown32		212
#define SYS_setuid32		213
#define SYS_setgid32		214
#define SYS_setfsuid32		215
#define SYS_setfsgid32		216
#define SYS_pivot_root		217
#define SYS_mincore		218
#define SYS_madvise		219
#define SYS_getdents64		220
#define SYS_fcntl64		221
#define SYS_security		223     /* 223 is unused, really */
#define SYS_gettid		224
#define SYS_readahead		225
#define SYS_setxattr		226
#define SYS_lsetxattr		227
#define SYS_fsetxattr		228
#define SYS_getxattr		229
#define SYS_lgetxattr		230
#define SYS_fgetxattr		231
#define SYS_listxattr		232
#define SYS_llistxattr		233
#define SYS_flistxattr		234
#define SYS_removexattr		235
#define SYS_lremovexattr	236
#define SYS_fremovexattr	237
#define SYS_tkill		238
#define SYS_sendfile64		239
#define SYS_futex		240
#define SYS_sched_setaffinity	241
#define SYS_sched_getaffinity	242
#define SYS_set_thread_area	243
#define SYS_get_thread_area	244
#define SYS_io_setup		245
#define SYS_io_destroy		246
#define SYS_io_getevents	247
#define SYS_io_submit		248
#define SYS_io_cancel		249
#define SYS_fadvise64		250
/* 251 is available for reuse (was briefly sys_set_zone_reclaim) */
#define SYS_exit_group		252
#define SYS_lookup_dcookie	253
#define SYS_epoll_create	254
#define SYS_epoll_ctl		255
#define SYS_epoll_wait		256
#define SYS_remap_file_pages	257
#define SYS_set_tid_address	258
#define SYS_timer_create	259
#define SYS_timer_settime	260
#define SYS_timer_gettime	261
#define SYS_timer_getoverrun	262
#define SYS_timer_delete	263
#define SYS_clock_settime	264
#define SYS_clock_gettime	265
#define SYS_clock_getres	266
#define SYS_clock_nanosleep	267
#define SYS_statfs64		268
#define SYS_fstatfs64		269
#define SYS_tgkill		270
#define SYS_utimes		271
#define SYS_fadvise64_64	272
#define SYS_vserver		273
#define SYS_mbind		274
#define SYS_get_mempolicy	275
#define SYS_set_mempolicy	276
#define SYS_mq_open 		277
#define SYS_mq_unlink		278
#define SYS_mq_timedsend	279
#define SYS_mq_timedreceive	280
#define SYS_mq_notify		281
#define SYS_mq_getsetattr	282
#define SYS_kexec_load		283
#define SYS_waitid		284
#define SYS_sys_setaltroot	285 /* no longer exists */
#define SYS_add_key		286
#define SYS_request_key		287
#define SYS_keyctl		288
#define SYS_ioprio_set		289
#define SYS_ioprio_get		290
#define SYS_inotify_init	291
#define SYS_inotify_add_watch	292
#define SYS_inotify_rm_watch	293
#define SYS_migrate_pages	294
#define SYS_openat		295
#define SYS_mkdirat		296
#define SYS_mknodat		297
#define SYS_fchownat		298
#define SYS_futimesat		299
#define SYS_fstatat64		300
#define SYS_unlinkat		301
#define SYS_renameat		302
#define SYS_linkat		303
#define SYS_symlinkat		304
#define SYS_readlinkat		305
#define SYS_fchmodat		306
#define SYS_faccessat		307
#define SYS_pselect6		308
#define SYS_ppoll		309
#define SYS_unshare		310
#define SYS_set_robust_list	311
#define SYS_get_robust_list	312
#define SYS_splice		313
#define SYS_sync_file_range	314
#define SYS_tee			315
#define SYS_vmsplice		316
#define SYS_move_pages		317
#define SYS_getcpu		318
#define SYS_epoll_pwait		319
#define SYS_utimensat		320
#define SYS_signalfd		321
#define SYS_timerfd_create	322
#define SYS_eventfd		323
#define SYS_fallocate		324
#define SYS_timerfd_settime	325
#define SYS_timerfd_gettime	326
#define SYS_signalfd4		327
#define SYS_eventfd2		328
#define SYS_epoll_create1	329
#define SYS_dup3		330
#define SYS_pipe2		331
#define SYS_inotify_init1	332
#define SYS_preadv		333
#define SYS_pwritev		334
#define SYS_rt_tgsigqueueinfo	335
#define SYS_perf_event_open	336


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

static gc_t *gc_ctx_g;
static ovm_var_t *delegate_g;
static ovm_var_t **fields_g;
static char *linux24_syscall_name_g[338];
//static char *linux24_sockcall_name_g[4];

extern int snarelex(void);
extern void snareerror(char *s);
extern char *snaretext;

void snareparse_set_gc_ctx(gc_t *gc_ctx);
void snareparse_set_delegate(ovm_var_t *delegate);
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
    ovm_var_t *val;

    val = ovm_int_new(gc_ctx_g, $10);
    GC_TOUCH (gc_ctx_g, fields_g[F_RETCODE] = val);

    val = ovm_int_new(gc_ctx_g, $14);
    GC_TOUCH (gc_ctx_g, fields_g[F_SEQUENCE] = val);
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
      ovm_var_t *val;

      val = ovm_int_new (gc_ctx_g, $3 | $4);
      GC_TOUCH (gc_ctx_g, fields_g[F_MODE] = val);

      val = ovm_vstr_new (gc_ctx_g, NULL);
      VSTR(val) = linux24_syscall_name_g[SYS_open];
      VSTRLEN(val) = strlen(VSTR(val));
      GC_TOUCH (gc_ctx_g, fields_g[F_SYSCALL] = val);
    }
| perm_io_syscall '(' ')' ',' date '\t'
  field_user field_process field_path perm_attributes
    {
      ovm_var_t *val;

      val = ovm_vstr_new (gc_ctx_g, NULL);
      VSTR(val) = linux24_syscall_name_g[$1];
      VSTRLEN(val) = strlen(VSTR(val));
      GC_TOUCH (gc_ctx_g, fields_g[F_SYSCALL] = val);
    }
| SYS_MKNOD '(' sys_mknod_opts ')' ',' date '\t'
  field_user field_process field_path dev_attributes
    {
      ovm_var_t *val;

      DPRINTF( ("set io_event mknod()\n") );

      val = ovm_vstr_new (gc_ctx_g, NULL);
      VSTR(val) = linux24_syscall_name_g[SYS_mknod];
      VSTRLEN(val) = strlen(VSTR(val));
      GC_TOUCH (gc_ctx_g, fields_g[F_SYSCALL] = val);

      /* XXX set mknod opts */
    }
| trunc_syscall '(' sys_mknod_opts ')' ',' date '\t'
  field_user field_process field_path
  TOKEN_ATTRIBUTES ',' INTEGER '\t'
    {
      ovm_var_t *val;

      DPRINTF( ("set trunc attributes\n") );

      val = ovm_int_new (gc_ctx_g, $13);
      GC_TOUCH (gc_ctx_g, fields_g[F_OFFSET] = val);

      DPRINTF( ("set io_event syscall (f)truncate(64)()\n") );

      val = ovm_vstr_new (gc_ctx_g, NULL);
      VSTR(val) = linux24_syscall_name_g[ $1 ];
      VSTRLEN(val) = strlen(VSTR(val));
      GC_TOUCH (gc_ctx_g, fields_g[F_SYSCALL] = val);
    }
| io_syscall '(' ')' ',' date '\t'
  field_user field_process field_path
    {
      ovm_var_t *val;

      val = ovm_vstr_new (gc_ctx_g, NULL);
      VSTR(val) = linux24_syscall_name_g[ $1 ];
      VSTRLEN(val) = strlen(VSTR(val));
      GC_TOUCH (gc_ctx_g, fields_g[F_SYSCALL] = val);
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
      ovm_var_t *val;

      val = ovm_int_new (gc_ctx_g, $3);
      GC_TOUCH (gc_ctx_g, fields_g[F_CREATEMODE] = val);
    }
;

dev_attributes:
  /* empty */
    { /* do_nothing() */; }
| TOKEN_ATTRIBUTES ',' INTEGER ' ' INTEGER '\t'
    {
      ovm_var_t *val;

      DPRINTF( ("set device attribute") );

      val = ovm_int_new (gc_ctx_g, $3);
      GC_TOUCH (gc_ctx_g, fields_g[F_DEVMAJ] = val);

      val = ovm_int_new (gc_ctx_g, $5);
      GC_TOUCH (gc_ctx_g, fields_g[F_DEVMIN] = val);
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
      ovm_var_t *val;

      val = ovm_int_new (gc_ctx_g, $14);
      GC_TOUCH (gc_ctx_g, fields_g[F_OWNERUID] = val);

      val = ovm_int_new (gc_ctx_g, $19);
      GC_TOUCH (gc_ctx_g, fields_g[F_OWNERGID] = val);

      val = ovm_vstr_new (gc_ctx_g, NULL);
      VSTR(val) = linux24_syscall_name_g[ $1 ];
      VSTRLEN(val) = strlen(VSTR(val));
      GC_TOUCH (gc_ctx_g, fields_g[F_SYSCALL] = val);
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
      ovm_var_t *val;
      size_t len;

      len = strlen($12);
      val = ovm_str_new (gc_ctx_g, len);
      memcpy(STR(val), $12, len);
      GC_TOUCH (gc_ctx_g, fields_g[F_CMDLINE] = val);
      free($12); /* XXX: compute offsets and sizes instead of dup()ing strings */

      val = ovm_vstr_new (gc_ctx_g, NULL);
      VSTR(val) = linux24_syscall_name_g[ SYS_execve ];
      VSTRLEN(val) = strlen(VSTR(val));
      GC_TOUCH (gc_ctx_g, fields_g[F_SYSCALL] = val);
    }
;

proc_event:
  proc_syscall '(' ')' ',' date '\t'
  field_user field_process
    {
      ovm_var_t *val;

      val = ovm_vstr_new (gc_ctx_g, NULL);
      VSTR(val) = linux24_syscall_name_g[ $1 ];
      VSTRLEN(val) = strlen(VSTR(val));
      GC_TOUCH (gc_ctx_g, fields_g[F_SYSCALL] = val);
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
      ovm_var_t *val;

      val = ovm_ipv4_new (gc_ctx_g);
      IPV4(val) = $12;
      GC_TOUCH (gc_ctx_g, fields_g[F_SRCIP] = val);

      val = ovm_ipv4_new (gc_ctx_g);
      IPV4(val) = $14;
      GC_TOUCH (gc_ctx_g, fields_g[F_DSTIP] = val);

      val = ovm_int_new (gc_ctx_g, $16);
      GC_TOUCH (gc_ctx_g, fields_g[F_SRCPORT] = val);

      val = ovm_int_new (gc_ctx_g, $18);
      GC_TOUCH (gc_ctx_g, fields_g[F_DSTPORT] = val);

      val = ovm_vstr_new (gc_ctx_g, NULL);
      VSTR(val) = linux24_syscall_name_g[$3];
      VSTRLEN(val) = strlen(VSTR(val));
      GC_TOUCH (gc_ctx_g, fields_g[F_SOCKCALL] = val);

      val = ovm_vstr_new (gc_ctx_g, NULL);
      VSTR(val) = linux24_syscall_name_g[SYS_socketcall];
      VSTRLEN(val) = strlen(VSTR(val));
      GC_TOUCH (gc_ctx_g, fields_g[F_SYSCALL] = val);
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
      ovm_var_t *val;
      size_t len;

      DPRINTF( ("set destpath\n") );

      len = strlen($12);
      val = ovm_str_new (gc_ctx_g, len);
      memcpy(STR(val), $12, len);
      free($12); /* XXX: compute offsets and sizes instead of dup()ing strings */
      GC_TOUCH (gc_ctx_g, fields_g[F_DSTPATH] = val);

      val = ovm_vstr_new (gc_ctx_g, NULL);
      VSTR(val) = linux24_syscall_name_g[$1];
      VSTRLEN(val) = strlen(VSTR(val));
      GC_TOUCH (gc_ctx_g, fields_g[F_SYSCALL] = val);
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
      ovm_var_t *val;

      /* DPRINTF( ("set token target\n") ); */
      val = ovm_int_new (gc_ctx_g, $13);
      GC_TOUCH (gc_ctx_g, fields_g[F_TARGETID] = val);
      /* XXX */

      /* DPRINTF( ("setid_event\n") ); */
      val = ovm_vstr_new (gc_ctx_g, NULL);
      VSTR(val) = linux24_syscall_name_g[$1];
      VSTRLEN(val) = strlen(VSTR(val));
      GC_TOUCH (gc_ctx_g, fields_g[F_SYSCALL] = val);
    }
| setreid_syscall '(' ')' ',' date '\t'
  field_user field_process
  TOKEN_TARGET ',' STRING '(' INTEGER ')' ',' opt_username '(' INTEGER ')' '\t'
    {
      ovm_var_t *val;

      /* DPRINTF( ("set token target\n") ); */
      val = ovm_int_new (gc_ctx_g, $13);
      GC_TOUCH (gc_ctx_g, fields_g[F_TARGETID] = val);

      val = ovm_int_new (gc_ctx_g, $18);
      GC_TOUCH (gc_ctx_g, fields_g[F_TARGETRID] = val);

      /* DPRINTF( ("setreid_event\n") ); */
      val = ovm_vstr_new (gc_ctx_g, NULL);
      VSTR(val) = linux24_syscall_name_g[$1];
      VSTRLEN(val) = strlen(VSTR(val));
      GC_TOUCH (gc_ctx_g, fields_g[F_SYSCALL] = val);
    }
| setresid_syscall '(' ')' ',' date '\t'
  field_user field_process
  TOKEN_TARGET ',' STRING '(' INTEGER ')' ','
             opt_username '(' INTEGER ')' ','
             opt_username '(' INTEGER ')' '\t'
    {
      ovm_var_t *val;

      /* DPRINTF( ("set token target\n") ); */
      val = ovm_int_new (gc_ctx_g, $13);
      GC_TOUCH (gc_ctx_g, fields_g[F_TARGETID] = val);

      val = ovm_int_new (gc_ctx_g, $18);
      GC_TOUCH (gc_ctx_g, fields_g[F_TARGETRID] = val);

      val = ovm_int_new (gc_ctx_g, $23);
      GC_TOUCH (gc_ctx_g, fields_g[F_TARGETSID] = val);

      /* DPRINTF( ("setresid_event\n") ); */
      val = ovm_vstr_new (gc_ctx_g, NULL);
      VSTR(val) = linux24_syscall_name_g[$1];
      VSTRLEN(val) = strlen(VSTR(val));
      GC_TOUCH (gc_ctx_g, fields_g[F_SYSCALL] = val);
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
      ovm_var_t *val;

      DPRINTF( ("kern_event\n") );
      val = ovm_vstr_new (gc_ctx_g, NULL);
      VSTR(val) = linux24_syscall_name_g[$1];
      VSTRLEN(val) = strlen(VSTR(val));
      GC_TOUCH (gc_ctx_g, fields_g[F_SYSCALL] = val);
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
      ovm_var_t *val;
      size_t len;

      len = strlen($3);
      val = ovm_str_new (gc_ctx_g, len);
      memcpy(STR(val), $3, len);
      free($3); /* XXX: compute offsets and sizes instead of dup()ing strings */
      GC_TOUCH (gc_ctx_g, fields_g[F_PATH] = val);
    }
;

field_process:
  TOKEN_PROCESS ',' INTEGER ',' STRING '\t'
    {
      ovm_var_t *val;
      size_t len;

      val = ovm_int_new (gc_ctx_g, $3);
      GC_TOUCH (gc_ctx_g, fields_g[F_PID] = val);

      len = strlen($5);
      val = ovm_str_new (gc_ctx_g, len);
      memcpy(STR(val), $5, len);
      free($5); /* XXX: compute offsets and sizes instead of dup()ing strings */
      GC_TOUCH (gc_ctx_g, fields_g[F_PROCNAME] = val);
    }
;

field_user:
  TOKEN_USER ',' 
  STRING '(' INTEGER ')' ','
  STRING '(' INTEGER ')' ','
  STRING '(' INTEGER ')' ','
  STRING '(' INTEGER ')' '\t'
    {
      ovm_var_t *val;

      val = ovm_int_new (gc_ctx_g, $5);
      GC_TOUCH (gc_ctx_g, fields_g[F_RUID] = val);

      val = ovm_int_new (gc_ctx_g, $10);
      GC_TOUCH (gc_ctx_g, fields_g[F_RGID] = val);

      val = ovm_int_new (gc_ctx_g, $15);
      GC_TOUCH (gc_ctx_g, fields_g[F_EUID] = val);

      val = ovm_int_new (gc_ctx_g, $20);
      GC_TOUCH (gc_ctx_g, fields_g[F_EGID] = val);
    }
;

date:
  TOKEN_DATE
    {
      ovm_var_t *val;

      val = ovm_ctime_new (gc_ctx_g, $1);
      GC_TOUCH (gc_ctx_g, fields_g[F_TIME] = val);
    }
;

%%

void snareparse_set_gc_ctx(gc_t *gc_ctx)
{
  gc_ctx_g = gc_ctx;
}

void snareparse_set_delegate(ovm_var_t *delegate)
{
  delegate_g = delegate;
}

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


static char *linux24_syscall_name_g[338] = {
  "(0) SYS_restart_syscall",
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
  "(243) SYS_set_thread_area",
  "(244) SYS_get_thread_area",
  "(245) SYS_io_setup",
  "(246) SYS_io_destroy",
  "(247) SYS_io_getevents",
  "(248) SYS_io_submit",
  "(249) SYS_io_cancel",
  "(250) SYS_fadvise64",
  "(251) SYS_set_zone_reclaim", /* obsolete */
  "(252) SYS_exit_group",
  "(253) SYS_lookup_dcookie",
  "(254) SYS_epoll_create",
  "(255) SYS_epoll_ctl",
  "(256) SYS_epoll_wait",
  "(257) SYS_remap_file_pages",
  "(258) SYS_set_tid_address",
  "(259) SYS_timer_create",
  "(260) SYS_timer_settime",
  "(261) SYS_timer_gettime",
  "(262) SYS_timer_getoverrun",
  "(263) SYS_timer_delete",
  "(264) SYS_clock_settime",
  "(265) SYS_clock_gettime",
  "(266) SYS_clock_getres",
  "(267) SYS_clock_nanosleep",
  "(268) SYS_statfs64",
  "(269) SYS_fstatfs64",
  "(270) SYS_tgkill",
  "(271) SYS_utimes",
  "(272) SYS_fadvise64_64",
  "(273) SYS_vserver",
  "(274) SYS_mbind",
  "(275) SYS_get_mempolicy",
  "(276) SYS_set_mempolicy",
  "(277) SYS_mq_open",
  "(278) SYS_mq_unlink",
  "(279) SYS_mq_timedsend",
  "(280) SYS_mq_timedreceive",
  "(281) SYS_mq_notify",
  "(282) SYS_mq_getsetattr",
  "(283) SYS_kexec_load",
  "(284) SYS_waitid",
  "(285) SYS_sys_setaltroot", /* obsolete */
  "(286) SYS_add_key",
  "(287) SYS_request_key",
  "(288) SYS_keyctl",
  "(289) SYS_ioprio_set",
  "(290) SYS_ioprio_get",
  "(291) SYS_inotify_init",
  "(292) SYS_inotify_add_watch",
  "(293) SYS_inotify_rm_watch",
  "(294) SYS_migrate_pages",
  "(295) SYS_openat",
  "(296) SYS_mkdirat",
  "(297) SYS_mknodat",
  "(298) SYS_fchownat",
  "(299) SYS_futimesat",
  "(300) SYS_fstatat64",
  "(301) SYS_unlinkat",
  "(302) SYS_renameat",
  "(303) SYS_linkat",
  "(304) SYS_symlinkat",
  "(305) SYS_readlinkat",
  "(306) SYS_fchmodat",
  "(307) SYS_faccessat",
  "(308) SYS_pselect6",
  "(309) SYS_ppoll",
  "(310) SYS_unshare",
  "(311) SYS_set_robust_list",
  "(312) SYS_get_robust_list",
  "(313) SYS_splice",
  "(314) SYS_sync_file_range",
  "(315) SYS_tee",
  "(316) SYS_vmsplice",
  "(317) SYS_move_pages",
  "(318) SYS_getcpu",
  "(319) SYS_epoll_pwait",
  "(320) SYS_utimensat",
  "(321) SYS_signalfd",
  "(322) SYS_timerfd_create",
  "(323) SYS_eventfd",
  "(324) SYS_fallocate",
  "(325) SYS_timerfd_settime",
  "(326) SYS_timerfd_gettime",
  "(327) SYS_signalfd4",
  "(328) SYS_eventfd2",
  "(329) SYS_epoll_create1",
  "(330) SYS_dup3",
  "(331) SYS_pipe2",
  "(332) SYS_inotify_init1",
  "(333) SYS_preadv",
  "(334) SYS_pwritev",
  "(335) SYS_rt_tgsigqueueinfo",
  "(336) SYS_perf_event_open",
  NULL
};



/*
** Copyright (c) 2002-2005 by Julien OLIVAIN, Laboratoire Spécification
** et Vérification (LSV), CNRS UMR 8643 & ENS Cachan.
** Copyright (c) 2013-2015 by Jean GOUBAULT-LARRECQ, Laboratoire Spécification
** et Vérification (LSV), CNRS UMR 8643 & ENS Cachan.
**
** Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
** Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
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
