/**
 ** @file mod_bsm.c
 ** The bsm module.
 **
 ** @author Jean Goubault-Larrecq <goubault@lsv.ens-cachan.fr>
 ** @version 0.1
 ** @ingroup modules
 ** 
 **
 ** @date  Started on: Sam 14 jan 2012 14:32:21
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_BSM_H
#include <stdlib.h>
#include <stdio.h>
#include "orchids.h"
#include "orchids_api.h"
#include "mod_bsm.h"

input_module_t mod_bsm;

#ifdef READABLETYPE
struct s_ls {
  size_t len;
  char *s;
};

#define S(s) { sizeof(s)-1, s }

static struct s_ls s_bsm_event_low_types[] = {
  S("null"), // AUE_NULL=0
  S("exit"), // AUE_EXIT=1
  S("fork"), // AUE_FORK=2
  S("open"), // AUE_OPEN=3
  S("creat"), // AUE_CREAT=4
  S("link"), // AUE_LINK=5
  S("unlink"), // AUE_UNLINK=6
  S("exec"), // AUE_EXEC=7
  S("chdir"), // AUE_CHDIR=8
  S("mknod"), // AUE_MKNOD=9
  S("chmod"), // AUE_CHMOD=10
  S("chown"), // AUE_CHOWN=11
  S("umount"), // AE_UMOUNT=12
  S("junk"), // AUE_JUNK=13 (Solaris-specific)
  S("access"), // AUE_ACCESS=14
  S("kill"), // AUE_KILL=15
  S("stat"), // AUE_STAT=16
  S("lstat"), // AUE_LSTAT=17
  S("acct"), // AUE_ACCT=18
  S("mctl"), // AUE_MCTL=19 (Solaris-specific)
  S("reboot"), // AUE_REBOOT=20 (Darwin conflict with AUE_DARWIN_REBOOT=308)
  S("symlink"), // AUE_SYMLINK=21
  S("readlink"), // AUE_READLINK=22
  S("execve"), // AUE_EXECVE=23
  S("chroot"), // AUE_CHROOT=24
  S("vfork"), // AUE_VFORK=25
  S("setgroups"), // AUE_SETGROUPS=26
  S("setpgrp"), // AUE_SETPGRP=27
  S("swapon"), // AUE_SWAPON=28
  S("sethostname"), // AUE_SETHOSTNAME=29 (Darwin conflict with AUE_O_SETHOSTNAME=AUE_SYSCTL=43021)
  S("fcntl"), // AUE_FCNTL=30
  S("setpriority"), // AUE_SETPRIORITY=31 (Darwin conflict with AUE_DARWIN_SETPRIORITY=312)
  S("connect"), // AUE_CONNECT=32
  S("accept"), // AUE_ACCEPT=33
  S("bind"), // AUE_BIND=34
  S("setsockopt"), // AUE_SETSOCKOPT=35
  S("vtrace"), // AUE_VTRACE=36 (Solaris-specific)
  S("settimeofday"), // AUE_SETTIMEOFDAY=37 (Darwin conflict with AUE_DARWIN_SETTIMEOFDAY=313),
  // stopped here: really too long to do so:
  // rather include <bsm/audit_kevents.h> in any Orchids rule
  // that uses the bsm module.
};
#endif

static int
bsm_callback(orchids_t *ctx, mod_entry_t *mod, int sd, void *data)
{
  event_t *event; //orchids event
  event = NULL;
  // ovm_var_t *attr[BSM_FIELDS];

  DebugLog(DF_MOD, DS_TRACE, "bsm_callback()\n");
  // memset(attr, 0, sizeof(attr));

  u_char *buf, *buf0, *bufend;
  int n;
  n = au_read_rec ((FILE *)data, &buf);
  // !!! au_read_rec() requires a FILE *, but we only have a fildes sd;
  // should make data = fdopen(sd, "r")
  if (n<0)
    {
      DebugLog(DF_MOD, DS_TRACE, "bsm_callback(): cannot read bsm record.\n");
    error:
      free_event(event);
      return -1;
    }
  buf0 = buf;
  bufend = buf+n;
  do {
    tokenstr_t tok;
    int m;

    m = au_fetch_tok (&tok, buf, n);
    if (m!=0)
      {
	DebugLog(DF_MOD, DS_TRACE, "bsm_callback(): cannot fetch next token from bsm record.\n");
	break;
      }
    buf += tok.len;
    switch (tok.id)
      {
      case AUT_INVALID:
	DebugLog(DF_MOD, DS_TRACE, "bsm_callback(): token is invalid.\n");
	break;
      case AUT_HEADER: /* = case AUT_HEADER32: */
	{
	  ovm_var_t *attr[F_BSM_HDR_SIZE];
	  memset(attr, 0, sizeof(attr));
	  
	  attr[F_BSM_HDR_VERSION] = ovm_int_new();
	  INT(attr[F_BSM_HDR_VERSION]) = tok.tt.hdr32.version;
	  attr[F_BSM_HDR_TIME] = ovm_timeval_new();
	  {
	    struct timeval *tv = &TIMEVAL(attr[F_BSM_HDR_TIME]);
	    tv->tv_sec = tok.tt.hdr32.s;
	    tv->tv_usec = tok.tt.hdr32.ms*1000;
	    // hdr32 only provides milliseconds, but tv requires microseconds
	  }
	  attr[F_BSM_HDR_TYPE] = ovm_int_new();
	  INT(attr[F_BSM_HDR_TYPE]) = tok.tt.hdr32.e_type;
	  // type denotes what this event is, see <bsm/audit_kevents.h>,
	  // and the AUE_ macros.
	  // e.g., 43050 is fsctl() (this one is Darwin-specific)
	  attr[F_BSM_HDR_MODIFIER] = ovm_int_new();
	  INT(attr[F_BSM_HDR_MODIFIER]) = tok.tt.hdr32.e_mod;
	  add_fields_to_event_stride(ctx, mod, &event, attr,
				     F_BSM_HDR_START,
				     F_BSM_HDR_END);
	  break;
	}
      case AUT_HEADER32_EX: // almost the same as AUT_HEADER32
	{
	  ovm_var_t *attr[F_BSM_HDR_SIZE];
	  memset(attr, 0, sizeof(attr));
	  
	  attr[F_BSM_HDR_VERSION] = ovm_int_new();
	  INT(attr[F_BSM_HDR_VERSION]) = tok.tt.hdr32_ex.version;
	  attr[F_BSM_HDR_TIME] = ovm_timeval_new();
	  {
	    struct timeval *tv = &TIMEVAL(attr[F_BSM_HDR_TIME]);
	    tv->tv_sec = tok.tt.hdr32_ex.s;
	    tv->tv_usec = tok.tt.hdr32_ex.ms/1000;
	    // hdr32_ex provides a ms field, but it is documented (in libbsm.h)
	    // as returning nanoseconds, not milliseconds (?)
	    // I've not been able to test.
	  }
	  attr[F_BSM_HDR_TYPE] = ovm_int_new();
	  INT(attr[F_BSM_HDR_TYPE]) = tok.tt.hdr32_ex.e_type;
	  // type denotes what this event is, see <bsm/audit_kevents.h>,
	  // and the AUE_ macros.
	  // e.g., 43050 is fsctl() (this one is Darwin-specific)
	  attr[F_BSM_HDR_MODIFIER] = ovm_int_new();
	  INT(attr[F_BSM_HDR_MODIFIER]) = tok.tt.hdr32.e_mod;
	  // extra fields, compared to AUT_HEADER32:
	  attr[F_BSM_HDR_ADTYPE] = ovm_int_new();
	  INT(attr[F_BSM_HDR_ADTYPE]) = tok.tt.hdr32_ex.ad_type;
	  attr[F_BSM_HDR_IP0] = new_ovm_ipv4();
	  IPV4(attr[F_BSM_HDR_IP0])->s_addr = tok.tt.hdr32_ex.addr[0];
	  attr[F_BSM_HDR_IP1] = new_ovm_ipv4();
	  IPV4(attr[F_BSM_HDR_IP1])->s_addr = tok.tt.hdr32_ex.addr[1];
	  attr[F_BSM_HDR_IP2] = new_ovm_ipv4();
	  IPV4(attr[F_BSM_HDR_IP2])->s_addr = tok.tt.hdr32_ex.addr[2];
	  attr[F_BSM_HDR_IP3] = new_ovm_ipv4();
	  IPV4(attr[F_BSM_HDR_IP3])->s_addr = tok.tt.hdr32_ex.addr[3];
	  add_fields_to_event_stride(ctx, mod, &event, attr,
				     F_BSM_HDR_START,
				     F_BSM_HDR_END);
	  break;
	}
      case AUT_PATH:
	// used if syscall has a path argument.
	// E.g., in fsctl():
	//     int fsctl(const char *path, unsigned long request, void *data, unsigned long options);
	// argument 1 is passed in the path field (and will be stored
	// in the bsm.path Orchids field, not in bsm.arg1)
	{
	  ovm_var_t *attr[F_BSM_PATH_SIZE];

	  attr[F_BSM_PATH] = ovm_vstr_new();
	  {
	    ovm_vstr_t *s = &VSTR(attr[F_BSM_PATH]);
	    s->len = tok.tt.path.len;
	    s->str = tok.tt.path.path;
	    // warning: apparently, a path field may be given twice in a row.
	    // I do not know why.  When it happens, it seems to be the same
	    // value anyway.
	  }
	  add_fields_to_event_stride(ctx, mod, &event, attr,
				     F_BSM_PATH_START,
				     F_BSM_PATH_END);
	  break;
	}
      case AUT_XATPATH: /* maybe some extra (X) path argument? or is it
			 an X-Window related event? I don't know, and
			 I'm assuming data is in tok.tt.path, as for
			 AUT_PATH. */
	{
	  ovm_var_t *attr[F_BSM_XATPATH_SIZE];

	  attr[F_BSM_XATPATH] = ovm_vstr_new();
	  {
	    ovm_vstr_t *s = &VSTR(attr[F_BSM_XATPATH]);
	    s->len = tok.tt.path.len;
	    s->str = tok.tt.path.path;
	    // warning: apparently, a path field may be given twice in a row.
	    // I do not know why.  When it happens, it seems to be the same
	    // value anyway.
	  }
	  add_fields_to_event_stride(ctx, mod, &event, attr,
				     F_BSM_XATPATH_START,
				     F_BSM_XATPATH_END);
	  break;
	}
      case AUT_TEXT: // I don't know what this is used for
	{
	  ovm_var_t *attr[F_BSM_TEXT_SIZE];
	  attr[F_BSM_TEXT] = ovm_vstr_new();
	  {
	    ovm_vstr_t *s = &VSTR(attr[F_BSM_TEXT]);
	    s->len = tok.tt.text.len;
	    s->str = tok.tt.text.path;
	  }
	  add_fields_to_event_stride(ctx, mod, &event, attr,
				     F_BSM_TEXT_START,
				     F_BSM_TEXT_END);
	  break;
	}
      case AUT_SUBJECT: /* = case AUT_SUBJECT32: */
	{
	  ovm_var_t *attr[F_BSM_SUBJ_SIZE];

	  attr[F_BSM_SUBJ_AUID] = new_ovm_int();
	  INT(attr[F_BSM_SUBJ_AUID]) = tok.tt.subj32.auid;
	  attr[F_BSM_SUBJ_EUID] = new_ovm_int();
	  INT(attr[F_BSM_SUBJ_EUID]) = tok.tt.subj32.euid;
	  attr[F_BSM_SUBJ_EGID] = new_ovm_int();
	  INT(attr[F_BSM_SUBJ_EGID]) = tok.tt.subj32.egid;
	  attr[F_BSM_SUBJ_RUID] = new_ovm_int();
	  INT(attr[F_BSM_SUBJ_RUID]) = tok.tt.subj32.ruid;
	  attr[F_BSM_SUBJ_RGID] = new_ovm_int();
	  INT(attr[F_BSM_SUBJ_RGID]) = tok.tt.subj32.rgid;
	  attr[F_BSM_SUBJ_PID] = new_ovm_int();
	  INT(attr[F_BSM_SUBJ_PID]) = tok.tt.subj32.pid;
	  attr[F_BSM_SUBJ_SID] = new_ovm_int();
	  INT(attr[F_BSM_SUBJ_SID]) = tok.tt.subj32.sid;
	  attr[F_BSM_SUBJ_PORT] = new_ovm_int();
	  INT(attr[F_BSM_SUBJ_PORT]) = tok.tt.subj32.tid.port;
	  attr[F_BSM_SUBJ_ADDR] = new_ovm_ipv4();
	  IPV4(attr[F_BSM_SUBJ_ADDR])->s_addr = tok.tt.subj32.tid.addr;
	  add_fields_to_event_stride(ctx, mod, &event, attr,
				     F_BSM_SUBJ_START,
				     F_BSM_SUBJ_END);
	  break;
	}
      case AUT_RETURN32:
	{
	  ovm_var_t *attr[F_BSM_RET_SIZE];

	  attr[F_BSM_RET_VAL] = new_ovm_int();
	  INT(attr[F_BSM_RET_VAL]) = tok.tt.exit.ret;
	  attr[F_BSM_RET_STATUS] = new_ovm_int();
	  INT(attr[F_BSM_RET_STATUS]) = tok.tt.exit.status;
	  add_fields_to_event_stride(ctx, mod, &event, attr,
				     F_BSM_RET_START,
				     F_BSM_RET_END);
	  break;
	}
      case AUT_ATTR32:
      case AUT_ATTR: /* duplicate of AUT_ATTR32? */
	{
	  ovm_var_t *attr[F_BSM_ATTR_SIZE];

	  attr[F_BSM_ATTR_MODE] = new_ovm_int();
	  INT(attr[F_BSM_ATTR_MODE]) = tok.tt.attr32.mode;
	  attr[F_BSM_ATTR_UID] = new_ovm_int();
	  INT(attr[F_BSM_ATTR_UID]) = tok.tt.attr32.uid;
	  attr[F_BSM_ATTR_GID] = new_ovm_int();
	  INT(attr[F_BSM_ATTR_GID]) = tok.tt.attr32.gid;
	  attr[F_BSM_ATTR_FSID] = new_ovm_int();
	  INT(attr[F_BSM_ATTR_FSID]) = tok.tt.attr32.fsid;
	  attr[F_BSM_ATTR_NID] = new_ovm_int();
	  INT(attr[F_BSM_ATTR_NID]) = tok.tt.attr32.nid;
	  attr[F_BSM_ATTR_DEV] = new_ovm_int();
	  INT(attr[F_BSM_ATTR_DEV]) = tok.tt.attr32.dev;
	  add_fields_to_event_stride(ctx, mod, &event, attr,
				     F_BSM_ATTR_START,
				     F_BSM_ATTR_SIZE);
	  break;
	}
      case AUT_ARG32:
	if (tok.tt.arg32.no>F_BSM_MAX_ARGS || tok.tt.arg32.no<1)
	  {
	    DebugLog(DF_MOD, DS_TRACE, "bsm_callback(): argument number out of bounds.\n");
	  }
	else
	  {
	    int i = tok.tt.arg32.no-1;
	    ovm_var_t *attr[1];
	    memset(attr, 0, sizeof(attr));

	    attr[0] = new_ovm_int();
	    INT(attr[0]) = tok.tt.arg32.val;
	    // I'm not recording the tok.tt.arg32.text field,
	    // which is merely the name of the formal parameter
	    // to the syscall
	    add_fields_to_event_stride(ctx, mod, &event, attr,
				       F_BSM_ARG_START+i,
				       F_BSM_ARG_START+i+1);
	  }
	break;
      case AUT_ARG64:
	if (tok.tt.arg64.no>F_BSM_MAX_ARGS || tok.tt.arg64.no<1)
	  {
	    DebugLog(DF_MOD, DS_TRACE, "bsm_callback(): argument number out of bounds.\n");
	  }
	else
	  {
	    int i = F_BSM_ARG_START+tok.tt.arg64.no-1;
	    ovm_var_t *attr[1];
	    memset(attr, 0, sizeof(attr));

	    attr[0] = new_ovm_int();
	    INT(attr[0]) = tok.tt.arg64.val; /* this is 64 bits, */
	    /* hopefully this will hold in a long (=ovm_int_t)
	       the argument is that 64 bit values only occur on
	       64-bit machines anyway (this is not quite right,
	       since in particular an Orchids on a 32-bit machine
	       might monitor events from a 64-bit machine).
	     */
	    // I'm not recording the tok.tt.arg64.text field,
	    // which is merely the name of the formal parameter
	    // to the syscall
	    add_fields_to_event_stride(ctx, mod, &event, attr,
				       F_BSM_ARG_START+i,
				       F_BSM_ARG_START+i+1);
	  }
	break;
      case AUT_TRAILER:
	buf = bufend; /* will force outer 'do' loop to exit */
	break;
	/*
	 * And now for some other tok.ids that I've not experimented with:
	 */
      case AUT_OTHER_FILE: /* = case AUT_OTHER_FILE32: */
      case AUT_OTHER_FILE64: // dealt with here as well, but beware:
	// this should really parse some (inexistent) au_file64_t structure.
	{
	  ovm_var_t *attr[F_BSM_FILE_SIZE];
	  attr[F_BSM_FILE_TIME] = ovm_timeval_new();
	  {
	    struct timeval *tv = &TIMEVAL(attr[F_BSM_FILE_TIME]);
	    tv->tv_sec = tok.tt.file.s;
	    tv->tv_usec = tok.tt.file.ms*1000;
	    // file only provides milliseconds, but tv requires microseconds
	  }
	  {
	    ovm_vstr_t *s = &VSTR(attr[F_BSM_FILE_NAME]);
	    s->len = tok.tt.file.len;
	    s->str = tok.tt.file.name;
	  }
	  add_fields_to_event_stride(ctx, mod, &event, attr,
				     F_BSM_FILE_START,
				     F_BSM_FILE_END);
	  break;
	}
      case AUT_IN_ADDR:
	{
	  ovm_var_t *attr[F_BSM_INADDR_SIZE];
	  attr[F_BSM_INADDR] = new_ovm_ipv4();
	  IPV4(attr[F_BSM_INADDR])->s_addr = tok.tt.inaddr.addr;
	  add_fields_to_event_stride(ctx, mod, &event, attr,
				     F_BSM_INADDR_START,
				     F_BSM_INADDR_END);
	  break;
	}
      case AUT_IPORT:
	{
	  ovm_var_t * attr[F_BSM_IPORT_SIZE];
	  attr[F_BSM_IPORT] = new_ovm_int();
	  INT(attr[F_BSM_IPORT]) = tok.tt.iport.port;
	  add_fields_to_event_stride(ctx, mod, &event, attr,
				     F_BSM_IPORT_START,
				     F_BSM_IPORT_END);
	  break;
	}
      case AUT_SOCKET:
	{
	  ovm_var_t *attr[F_BSM_SOCK_SIZE];
	  attr[F_BSM_SOCK_TYPE] = new_ovm_int();
	  INT(attr[F_BSM_SOCK_TYPE]) = tok.tt.socket.type;
	  attr[F_BSM_SOCK_LPORT] = new_ovm_int();
	  INT(attr[F_BSM_SOCK_LPORT]) = tok.tt.socket.l_port;
	  attr[F_BSM_SOCK_LADDR] = new_ovm_ipv4();
	  IPV4(attr[F_BSM_SOCK_LADDR])->s_addr = tok.tt.socket.l_addr;
	  attr[F_BSM_SOCK_RPORT] = new_ovm_int();
	  INT(attr[F_BSM_SOCK_RPORT]) = tok.tt.socket.r_port;
	  attr[F_BSM_SOCK_RADDR] = new_ovm_ipv4();
	  IPV4(attr[F_BSM_SOCK_RADDR])->s_addr = tok.tt.socket.r_addr;
	  add_fields_to_event_stride(ctx, mod, &event, attr,
				     F_BSM_SOCK_START,
				     F_BSM_SOCK_END);
	  break;
	}
      case AUT_IPC:
	{
	  ovm_var_t *attr[F_BSM_IPC_SIZE];
	  attr[F_BSM_IPC_TYPE] = new_ovm_int();
	  INT(attr[F_BSM_IPC_TYPE]) = tok.tt.ipc.type;
	  attr[F_BSM_IPC_ID] = new_ovm_int();
	  INT(attr[F_BSM_IPC_ID]) = tok.tt.ipc.id;
	  add_fields_to_event_stride(ctx, mod, &event, attr,
				     F_BSM_IPC_START,
				     F_BSM_IPC_END);
	  break;
	}
      case AUT_IPC_PERM:
	{
	  ovm_var_t *attr[F_BSM_IPCPERM_SIZE];
	  attr[F_BSM_IPCPERM_UID] = new_ovm_int();
	  INT(attr[F_BSM_IPCPERM_UID]) = tok.tt.ipcperm.uid;
	  attr[F_BSM_IPCPERM_GID] = new_ovm_int();
	  INT(attr[F_BSM_IPCPERM_GID]) = tok.tt.ipcperm.gid;
	  attr[F_BSM_IPCPERM_PUID] = new_ovm_int();
	  INT(attr[F_BSM_IPCPERM_PUID]) = tok.tt.ipcperm.puid;
	  attr[F_BSM_IPCPERM_PGID] = new_ovm_int();
	  INT(attr[F_BSM_IPCPERM_PGID]) = tok.tt.ipcperm.pgid;
	  attr[F_BSM_IPCPERM_MOD] = new_ovm_int();
	  INT(attr[F_BSM_IPCPERM_MOD]) = tok.tt.ipcperm.mod;
	  attr[F_BSM_IPCPERM_SEQ] = new_ovm_int();
	  INT(attr[F_BSM_IPCPERM_SEQ]) = tok.tt.ipcperm.seq;
	  attr[F_BSM_IPCPERM_KEY] = new_ovm_int();
	  INT(attr[F_BSM_IPCPERM_KEY]) = tok.tt.ipcperm.key;
	  add_fields_to_event_stride(ctx, mod, &event, attr,
				     F_BSM_IPCPERM_START,
				     F_BSM_IPCPERM_END);
	}
      case AUT_GROUPS: /* [au_groups_t] list of integers */
	// We compile this list as a null-terminated list
	// of fields of MAX_AUDIT_GROUPS elements
	{
	  ovm_var_t *attr[F_BSM_GROUPS_SIZE];
	  memset(attr, 0, sizeof(attr));
	  int i, n;
	  n = tok.tt.groups.no;
	  if (n>MAX_AUDIT_GROUPS)
	    {
	      DebugLog(DF_MOD, DS_TRACE, "bsm_callback(): too many groups.\n");
	      n = MAX_AUDIT_GROUPS;
	    }
	  for (i=0; i<n; i++)
	    {
	      attr[i] = new_ovm_int();
	      INT(attr[i]) = tok.tt.groups.list[i];
	    }
	  add_fields_to_event_stride(ctx, mod, &event, attr,
				     F_BSM_GROUPS_START,
				     F_BSM_GROUPS_END+n);
	  break;
	}
      case AUT_NEWGROUPS: // I'm assuming this works as AUT_GROUPS */
	{
	  ovm_var_t *attr[F_BSM_NEWGROUPS_SIZE];
	  memset(attr, 0, sizeof(attr));
	  int i, n;
	  n = tok.tt.groups.no;
	  if (n>MAX_AUDIT_GROUPS)
	    {
	      DebugLog(DF_MOD, DS_TRACE, "bsm_callback(): too many groups.\n");
	      n = MAX_AUDIT_GROUPS;
	    }
	  for (i=0; i<n; i++)
	    {
	      attr[i] = new_ovm_int();
	      INT(attr[i]) = tok.tt.groups.list[i];
	    }
	  add_fields_to_event_stride(ctx, mod, &event, attr,
				     F_BSM_NEWGROUPS_START,
				     F_BSM_NEWGROUPS_END+n);
	  break;
	}
      case AUT_PROCESS32:
	{
	  ovm_var_t *attr[F_BSM_PROC_SIZE];
	  memset(attr, 0, sizeof(attr));
	  attr[F_BSM_PROC_AUID] = new_ovm_int();
	  INT(attr[F_BSM_PROC_AUID]) = tok.tt.proc32.auid;
	  attr[F_BSM_PROC_EUID] = new_ovm_int();
	  INT(attr[F_BSM_PROC_EUID]) = tok.tt.proc32.euid;
	  attr[F_BSM_PROC_EGID] = new_ovm_int();
	  INT(attr[F_BSM_PROC_EGID]) = tok.tt.proc32.egid;
	  attr[F_BSM_PROC_RUID] = new_ovm_int();
	  INT(attr[F_BSM_PROC_RUID]) = tok.tt.proc32.ruid;
	  attr[F_BSM_PROC_RGID] = new_ovm_int();
	  INT(attr[F_BSM_PROC_RGID]) = tok.tt.proc32.rgid;
	  attr[F_BSM_PROC_PID] = new_ovm_int();
	  INT(attr[F_BSM_PROC_PID]) = tok.tt.proc32.pid;
	  attr[F_BSM_PROC_SID] = new_ovm_int();
	  INT(attr[F_BSM_PROC_SID]) = tok.tt.proc32.sid;
	  attr[F_BSM_PROC_PORT] = new_ovm_int();
	  INT(attr[F_BSM_PROC_PORT]) = tok.tt.proc32.tid.port;
	  attr[F_BSM_PROC_IP] = new_ovm_ipv4();
	  IPV4(attr[F_BSM_PROC_IP])->s_addr = tok.tt.proc32.tid.addr;
	  add_fields_to_event_stride(ctx, mod, &event, attr,
				     F_BSM_PROC_START,
				     F_BSM_PROC_END);
	  break;
	}
      case AUT_IP:
	{
	  ovm_var_t *attr[F_BSM_IP_SIZE];
	  memset(attr, 0, sizeof(attr));
	  attr[F_BSM_IP_VERSION] = new_ovm_int();
	  INT(attr[F_BSM_IP_VERSION]) = tok.tt.ip.version;
	  attr[F_BSM_IP_TOS] = new_ovm_int();
	  INT(attr[F_BSM_IP_TOS]) = tok.tt.ip.tos;
	  attr[F_BSM_IP_LEN] = new_ovm_int();
	  INT(attr[F_BSM_IP_LEN]) = tok.tt.ip.len;
	  attr[F_BSM_IP_ID] = new_ovm_int();
	  INT(attr[F_BSM_IP_ID]) = tok.tt.ip.id;
	  attr[F_BSM_IP_OFFSET] = new_ovm_int();
	  INT(attr[F_BSM_IP_OFFSET]) = tok.tt.ip.offset;
	  attr[F_BSM_IP_TTL] = new_ovm_int();
	  INT(attr[F_BSM_IP_TTL]) = tok.tt.ip.ttl;
	  attr[F_BSM_IP_PROT] = new_ovm_int();
	  INT(attr[F_BSM_IP_PROT]) = tok.tt.ip.prot;
	  attr[F_BSM_IP_CHKSM] = new_ovm_int();
	  INT(attr[F_BSM_IP_CHKSM]) = tok.tt.ip.chksm;
	  attr[F_BSM_IP_SRC] = new_ovm_ipv4();
	  IPV4(attr[F_BSM_IP_SRC])->s_addr = tok.tt.ip.src;
	  attr[F_BSM_IP_DEST] = new_ovm_ipv4();
	  IPV4(attr[F_BSM_IP_DEST])->s_addr = tok.tt.ip.dest;
	  add_fields_to_event_stride(ctx, mod, &event, attr,
				     F_BSM_IP_START,
				     F_BSM_IP_END);
	  break;
	}
      case AUT_SEQ: /* sequence number */
	{
	  ovm_var_t *attr[F_BSM_SEQ_SIZE];
	  attr[F_BSM_SEQ] = new_ovm_int();
	  INT(attr[F_BSM_SEQ]) = tok.tt.seq.seqno;
	  add_fields_to_event_stride(ctx, mod, &event, attr,
				     F_BSM_SEQ_START,
				     F_BSM_SEQ_END);
	  break;
	}
	// !!! Stopped at AUT_EXEC_ARGS (0x3c)
	// The following are ignored
	// First, those that are ignored because I don't know
	// what they are for, or which subfields of tok.tt should be used
      case AUT_DATA:
      case AUT_OPAQUE: /* seems to be some form of unusable, catch-all type */
      case AUT_ACL:
      case AUT_LABEL:
      case AUT_ACE:
      case AUT_PRIV:
      case AUT_UPRIV:
      case AUT_LIAISON:
	// Then, those I am not sure of
	// Then, those I fear are out of scope
	// Then, those that are obsolete
      case AUT_OHEADER: // for "old header"
      default:
	DebugLog(DF_MOD, DS_TRACE, "bsm_callback(): unknown token id.\n");
	break;
      }
  } while (buf < bufend);

  /* finally, post the event */
  post_event(ctx, mod, event);
  free(buf0);
  return 0;
}

static field_t bsm_fields[] = {
  { "bsm.header.version", T_INT, "bsm header: version" },
  { "bsm.header.time", T_TIMEVAL, "bsm header: time" },
  { "bsm.header.type", T_INT, "bsm header: type" },
  { "bsm.header.modifier", T_INT, "bsm header: modifier" },
  { "bsm.header.ad_type", T_INT, "bsm header (extended): IP address type" },
  { "bsm.header.ip0", T_IPV4, "bsm header (extended): IPv4 address, or first part of IPv6 address" },
  { "bsm.header.ip1", T_IPV4, "bsm header (extended): second part of IPv6 address" },
  { "bsm.header.ip2", T_IPV4, "bsm header (extended): third part of IPv6 address" },
  { "bsm.header.ip3", T_IPV4, "bsm header (extended): fourth part of IPv6 address" },
  { "bsm.path", T_VSTR, "bsm path argument (whatever its position in the argument list)" },
  { "bsm.subject.auid", T_INT, "bsm subject: audit ID" },
  { "bsm.subject.euid", T_INT, "bsm subject: effective user ID" },
  { "bsm.subject.egid", T_INT, "bsm subject: effective group ID" },
  { "bsm.subject.ruid", T_INT, "bsm subject: real user ID" },
  { "bsm.subject.rgid", T_INT, "bsm subject: real group ID" },
  { "bsm.subject.pid", T_INT, "bsm subject: process ID" },
  { "bsm.subject.sid", T_INT, "bsm subject: session ID" },
  { "bsm.subject.port", T_INT, "bsm subject: port ID" },
  { "bsm.subject.addr", T_IPV4, "bsm subject: ip v4 address" },
  { "bsm.return.val", T_INT, "bsm return value" },
  { "bsm.return.status", T_INT, "bsm return status (256 if success)" },
  { "bsm.attr.mode", T_INT, "bsm attribute: file access mode" },
  { "bsm.attr.uid", T_INT, "bsm attribute: owner user ID" },
  { "bsm.attr.gid", T_INT, "bsm attribute: owner group ID" },
  { "bsm.attr.fsid", T_INT, "bsm attribute: file system ID" },
  { "bsm.attr.nid", T_INT, "bsm attribute: node ID" },
  { "bsm.attr.dev", T_INT, "bsm attribute: device number" },
  { "bsm.file.time", T_TIMEVAL, "bsm file: time" },
  { "bsm.file.name", T_VSTR, "bsm file name" },
  { "bsm.text", T_VSTR, "bsm text argument" },
  { "bsm.inaddr", T_IPV4, "bsm internet address" },
  { "bsm.iport", T_INT, "bsm internet port" },
  { "bsm.socket.type", T_INT, "bsm socket: type" },
  { "bsm.socket.lport", T_INT, "bsm socket: local port" },
  { "bsm.socket.laddr", T_IPV4, "bsm socket: local internet address" },
  { "bsm.socket.rport", T_INT, "bsm socket: remote port" },
  { "bsm.socket.raddr", T_IPV4, "bsm socket: remote internet address" },
  { "bsm.ipc.type", T_INT, "bsm ipc: type" },
  { "bsm.ipc.id", T_INT, "bsm ipc: id" },
  { "bsm.xatpath", T_VSTR, "bsm xatpath" },
  { "bsm.process.auid", T_INT, "bsm process: audit id" },
  { "bsm.process.euid", T_INT, "bsm process: effective user id" },
  { "bsm.process.egid", T_INT, "bsm process: effective group id" },
  { "bsm.process.ruid", T_INT, "bsm process: real user id" },
  { "bsm.process.rgid", T_INT, "bsm process: real group id" },
  { "bsm.process.pid", T_INT, "bsm process: process id" },
  { "bsm.process.sid", T_INT, "bsm process: session id" },
  { "bsm.process.tid.port", T_INT, "bsm process: terminal id: port" },
  { "bsm.process.tid.ip", T_INT, "bsm process: terminal id: ip address" },
  { "bsm.ip.version", T_INT, "bsm ip packet: version" },
  { "bsm.ip.tos", T_INT, "bsm ip packet: type of service" },
  { "bsm.ip.len", T_INT, "bsm ip packet: length" },
  { "bsm.ip.id", T_INT, "bsm ip packet: id" },
  { "bsm.ip.offset", T_INT, "bsm ip packet: offset" },
  { "bsm.ip.ttl", T_INT, "bsm ip packet: time to live" },
  { "bsm.ip.prot", T_INT, "bsm ip packet: protocol number" },
  { "bsm.ip.chksm", T_INT, "bsm ip packet: checksum (may not be reliable, and even not filled in at all)" },
  { "bsm.ip.src", T_IPV4, "bsm ip packet: source ip address" },
  { "bsm.ip.dest", T_INT, "bsm ip packet: destination ip address" },
  { "bsm.seq", T_INT, "bsm sequence number" },
#if AUDIT_MAX_GROUPS>=1
  { "bsm.groups.elt1", T_INT, "bsm groups: 1st element" },
#endif
#if AUDIT_MAX_GROUPS>=2
  { "bsm.groups.elt2", T_INT, "bsm groups: 2nd element" },
#endif
#if AUDIT_MAX_GROUPS>=3
  { "bsm.groups.elt3", T_INT, "bsm groups: 3rd element" },
#endif
#if AUDIT_MAX_GROUPS>=4
  { "bsm.groups.elt4", T_INT, "bsm groups: 4th element" },
#endif
#if AUDIT_MAX_GROUPS>=5
  { "bsm.groups.elt5", T_INT, "bsm groups: 5th element" },
#endif
#if AUDIT_MAX_GROUPS>=6
  { "bsm.groups.elt6", T_INT, "bsm groups: 6th element" },
#endif
#if AUDIT_MAX_GROUPS>=7
  { "bsm.groups.elt7", T_INT, "bsm groups: 7th element" },
#endif
#if AUDIT_MAX_GROUPS>=8
  { "bsm.groups.elt8", T_INT, "bsm groups: 8th element" },
#endif
#if AUDIT_MAX_GROUPS>=9
  { "bsm.groups.elt9", T_INT, "bsm groups: 9th element" },
#endif
#if AUDIT_MAX_GROUPS>=10
  { "bsm.groups.elt10", T_INT, "bsm groups: 10th element" },
#endif
#if AUDIT_MAX_GROUPS>=11
  { "bsm.groups.elt11", T_INT, "bsm groups: 11th element" },
#endif
#if AUDIT_MAX_GROUPS>=12
  { "bsm.groups.elt12", T_INT, "bsm groups: 12th element" },
#endif
#if AUDIT_MAX_GROUPS>=13
  { "bsm.groups.elt13", T_INT, "bsm groups: 13th element" },
#endif
#if AUDIT_MAX_GROUPS>=14
  { "bsm.groups.elt14", T_INT, "bsm groups: 14th element" },
#endif
#if AUDIT_MAX_GROUPS>=15
  { "bsm.groups.elt15", T_INT, "bsm groups: 15th element" },
#endif
#if AUDIT_MAX_GROUPS>=16
  { "bsm.groups.elt16", T_INT, "bsm groups: 16th element" },
#endif
#if AUDIT_MAX_GROUPS>16
  Error: AUDIT_MAX_GROUPS too large (add a few more lines above)
#endif
#if AUDIT_MAX_GROUPS>=1
  { "bsm.newgroups.elt1", T_INT, "bsm newgroups: 1st element" },
#endif
#if AUDIT_MAX_GROUPS>=2
  { "bsm.newgroups.elt2", T_INT, "bsm newgroups: 2nd element" },
#endif
#if AUDIT_MAX_GROUPS>=3
  { "bsm.newgroups.elt3", T_INT, "bsm newgroups: 3rd element" },
#endif
#if AUDIT_MAX_GROUPS>=4
  { "bsm.newgroups.elt4", T_INT, "bsm newgroups: 4th element" },
#endif
#if AUDIT_MAX_GROUPS>=5
  { "bsm.newgroups.elt5", T_INT, "bsm newgroups: 5th element" },
#endif
#if AUDIT_MAX_GROUPS>=6
  { "bsm.newgroups.elt6", T_INT, "bsm newgroups: 6th element" },
#endif
#if AUDIT_MAX_GROUPS>=7
  { "bsm.newgroups.elt7", T_INT, "bsm newgroups: 7th element" },
#endif
#if AUDIT_MAX_GROUPS>=8
  { "bsm.newgroups.elt8", T_INT, "bsm newgroups: 8th element" },
#endif
#if AUDIT_MAX_GROUPS>=9
  { "bsm.newgroups.elt9", T_INT, "bsm newgroups: 9th element" },
#endif
#if AUDIT_MAX_GROUPS>=10
  { "bsm.newgroups.elt10", T_INT, "bsm newgroups: 10th element" },
#endif
#if AUDIT_MAX_GROUPS>=11
  { "bsm.newgroups.elt11", T_INT, "bsm newgroups: 11th element" },
#endif
#if AUDIT_MAX_GROUPS>=12
  { "bsm.newgroups.elt12", T_INT, "bsm newgroups: 12th element" },
#endif
#if AUDIT_MAX_GROUPS>=13
  { "bsm.newgroups.elt13", T_INT, "bsm newgroups: 13th element" },
#endif
#if AUDIT_MAX_GROUPS>=14
  { "bsm.newgroups.elt14", T_INT, "bsm newgroups: 14th element" },
#endif
#if AUDIT_MAX_GROUPS>=15
  { "bsm.newgroups.elt15", T_INT, "bsm newgroups: 15th element" },
#endif
#if AUDIT_MAX_GROUPS>=16
  { "bsm.newgroups.elt16", T_INT, "bsm newgroups: 16th element" },
#endif
#if AUDIT_MAX_GROUPS>16
  Error: AUDIT_MAX_GROUPS too large (add a few more lines above)
#endif
  // The following are written assuming F_BSM_MAX_ARGS==16
  // Do not add new fields after this line
  // unless they are bsm.arg<i> fields;
  // in which case please update F_BSM_MAX_ARGS
  { "bsm.arg1", T_INT, "bsm argument 1" },
  { "bsm.arg2", T_INT, "bsm argument 2" },
  { "bsm.arg3", T_INT, "bsm argument 3" },
  { "bsm.arg4", T_INT, "bsm argument 4" },
  { "bsm.arg5", T_INT, "bsm argument 5" },
  { "bsm.arg6", T_INT, "bsm argument 6" },
  { "bsm.arg7", T_INT, "bsm argument 7" },
  { "bsm.arg8", T_INT, "bsm argument 8" },
  { "bsm.arg9", T_INT, "bsm argument 9" },
  { "bsm.arg10", T_INT, "bsm argument 10" },
  { "bsm.arg11", T_INT, "bsm argument 11" },
  { "bsm.arg12", T_INT, "bsm argument 12" },
  { "bsm.arg13", T_INT, "bsm argument 13" },
  { "bsm.arg14", T_INT, "bsm argument 14" },
  { "bsm.arg15", T_INT, "bsm argument 15" },
  { "bsm.arg16", T_INT, "bsm argument 16" },
  // do not add any new field names here, except of the form bsm.arg<x>
  // and change F_BSM_MAX_ARGS accordingly
};

#endif /* HAVE_BSM_H */

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
