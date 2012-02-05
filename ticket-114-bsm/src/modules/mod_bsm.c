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
#include <bsm/libbsm.h>
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

#define F_INT(fname,bsmname) { attr[F_BSM_ ## fname] = ovm_int_new();	\
    INT(attr[F_BSM_ ## fname]) = tok.tt.bsmname; }
#define F_VSTR(fname,bsmname) { attr[F_BSM_ ## fname] = ovm_vstr_new(); \
  {									\
    ovm_vstr_t *s = &VSTR(attr[F_BSM_ ## fname]);			\
    s->len = tok.tt.bsmname.len;					\
    s->str = tok.tt.bsmname.path;					\
  }
#define F_IPV4(fname,bsmname) { attr[F_BSM_ ## fname] = ovm_ipv4_new(); \
    IPV4(attr[F_BSM_ ## fname])->s_addr = tok.tt.bsnname; }
#define F_IPV6(fname,bsmname) {	  \
    F_IPV4(fname ## 0, bsmname[0]);		\
    F_IPV4(fname ## 1, bsmname[1]);		\
    F_IPV4(fname ## 2, bsmname[2]);		\
    F_IPV4(fname ## 3, bsmname[3]);		\
  }
#define F_TIME_S_MS(fname,bsmname) {	     \
    attr[F_BSM_ ## fname] = ovm_timeval_new();		\
    {							\
      struct timeval *tv = &TIMEVAL(attr[F_BSM_ ## fname]);	\
      tv->tv_sec = tok.tt.bsmname.s;				\
      tv->tv_usec = tok.tt.bsmname.ms*1000;			\
    }								\
  }
#define F_TIME_S_NS(fname,bsmname) {	     \
    attr[F_BSM_ ## fname] = ovm_timeval_new();		\
    {							\
      struct timeval *tv = &TIMEVAL(attr[F_BSM_ ## fname]);	\
      tv->tv_sec = tok.tt.bsmname.s;				\
      tv->tv_usec = tok.tt.bsmname.ms/1000;			\
    }								\
  }

#define F_WRAP(type) add_fields_to_event_stride(ctx, mod, &event, attr, \
						F_BSM_ ## type ## _START, \
						F_BSM_ ## type ## _END);

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

	  F_INT(HDR_VERSION,hdr32.version);
	  F_TIME_S_MS(HDR_TIME,hdr32);
	  // hdr32 only provides milliseconds, but tv requires microseconds
	  F_INT(HDR_TYPE,hdr32.e_type);
	  // type denotes what this event is, see <bsm/audit_kevents.h>,
	  // and the AUE_ macros.
	  // e.g., 43050 is fsctl() (this one is Darwin-specific)
	  F_INT(HDR_MODIFIER,hdr32.e_mod);
	  F_WRAP(HDR);
	  break;
	}
      case AUT_HEADER32_EX: // almost the same as AUT_HEADER32
	{
	  ovm_var_t *attr[F_BSM_HDR_SIZE];
	  memset(attr, 0, sizeof(attr));

	  F_INT(HDR_VERSION,hdr32_ex.version);
	  F_TIME_S_NS(HDR_TIME,hdr32_ex);
	  // hdr32_ex provides a ms field, but it is documented (in libbsm.h)
	  // as returning nanoseconds, not milliseconds (?)
	  // I've not been able to test.  Sounds like a bug.
	  // More likely: use F_TIME_S_MS, not F_TIME_S_NS
	  F_INT(HDR_TYPE,hdr32_ex.e_type);
	  // type denotes what this event is, see <bsm/audit_kevents.h>,
	  // and the AUE_ macros.
	  // e.g., 43050 is fsctl() (this one is Darwin-specific)
	  F_INT(HDR_MODIFIER,hdr32_ex.e_mod);
	  // extra fields, compared to AUT_HEADER32:
	  F_INT(HDR_ADTYPE,hdr32_ex.ad_type);
	  F_IPV6(HDR_IP,hdr32_ex.addr);
	  F_WRAP(HDR);
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

	  F_VSTR(PATH,path);
	  // warning: apparently, a path field may be given twice in a row.
	  // I do not know why.  When it happens, it seems to be the same
	  // value anyway.
	  F_WRAP(PATH);
	  break;
	}
      case AUT_XATPATH: /* maybe some extra (X) path argument? or is it
			 an X-Window related event? I don't know, and
			 I'm assuming data is in tok.tt.path, as for
			 AUT_PATH. */
	{
	  ovm_var_t *attr[F_BSM_XATPATH_SIZE];

	  F_VSTR(XATPATH,path);
	  F_WRAP(XATPATH);
	  break;
	}
      case AUT_TEXT: // I don't know what this is used for
	{
	  ovm_var_t *attr[F_BSM_TEXT_SIZE];

	  F_VSTR(TEXT,text);
	  F_WRAP(TEXT);
	  break;
	}
      case AUT_SUBJECT: /* = case AUT_SUBJECT32: */
	{
	  ovm_var_t *attr[F_BSM_SUBJ_SIZE];

	  F_INT(SUBJ_AUID,subj32.auid);
	  F_INT(SUBJ_EUID,subj32.euid);
	  F_INT(SUBJ_EGID,subj32.egid);
	  F_INT(SUBJ_RUID,subj32.ruid);
	  F_INT(SUBJ_RGID,subj32.rgid);
	  F_INT(SUBJ_PID,subj32.pid);
	  F_INT(SUBJ_SID,subj32.sid);
	  F_INT(SUBJ_PORT,subj32.tid.port);
	  F_IPV4(SUBJ_ADDR,subj32.tid.addr);
	  F_WRAP(SUBJ);
	  break;
	}
      case AUT_RETURN32:
	{
	  ovm_var_t *attr[F_BSM_RET_SIZE];

	  F_INT(RET_VAL,exit.ret);
	  F_INT(RET_STATUS,exit.status);
	  F_WRAP(RET);
	  break;
	}
      case AUT_ATTR32:
      case AUT_ATTR: /* duplicate of AUT_ATTR32? */
	{
	  ovm_var_t *attr[F_BSM_ATTR_SIZE];

	  F_INT(ATTR_MODE,attr32.mode);
	  F_INT(ATTR_UID,attr32.uid);
	  F_INT(ATTR_GID,attr32.gid);
	  F_INT(ATTR_FSID,attr32.fsid);
	  F_INT(ATTR_NID,attr32.nid);
	  F_INT(ATTR_DEV,attr32.dev);
	  F_WRAP(ATTR);
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

	  F_TIME_S_MS(FILE_TIME,file);
	  // file only provides milliseconds, but tv requires microseconds
	  F_VSTR(FILE_NAME,file);
	  F_WRAP(FILE);
	  break;
	}
      case AUT_IN_ADDR:
	{
	  ovm_var_t *attr[F_BSM_INADDR_SIZE];

	  F_IPV4(INADDR,inaddr.addr);
	  F_WRAP(INADDR);
	  break;
	}
      case AUT_IPORT:
	{
	  ovm_var_t * attr[F_BSM_IPORT_SIZE];

	  F_INT(IPORT,iport.port);
	  F_WRAP(IPORT);
	  break;
	}
      case AUT_SOCKET:
	{
	  ovm_var_t *attr[F_BSM_SOCK_SIZE];

	  F_INT(SOCK_TYPE,socket.type);
	  F_INT(SOCK_LPORT,socket.l_port);
	  F_INT(SOCK_LADDR,socket.l_addr);
	  F_INT(SOCK_RPORT,socket.r_port);
	  F_INT(SOCK_RADDR,socket.r_addr);
	  F_WRAP(SOCK);
	  break;
	}
      case AUT_IPC:
	{
	  ovm_var_t *attr[F_BSM_IPC_SIZE];

	  F_INT(IPC_TYPE,ipc.type);
	  F_INT(IPC_ID,ipc.id);
	  F_WRAP(IPC);
	  break;
	}
      case AUT_IPC_PERM:
	{
	  ovm_var_t *attr[F_BSM_IPCPERM_SIZE];

	  F_INT(IPCPERM_UID,ipcperm.uid);
	  F_INT(IPCPERM_GID,ipcperm.gid);
	  F_INT(IPCPERM_PUID,ipcperm.puid);
	  F_INT(IPCPERM_PGID,ipcperm.pgid);
	  F_INT(IPCPERM_MOD,ipcperm.mod);
	  F_INT(IPCPERM_SEQ,ipcperm.seq);
	  F_INT(IPCPERM_KEY,ipcperm.key);
	  F_WRAP(IPCPERM);
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
	  attr[F_BSM_GROUPS_LEN] = ovm_int_new();
	  INT(attr[F_BSM_GROUPS_LEN]) = n;
	  for (i=0; i<n; i++)
	    {
	      attr[i+1] = new_ovm_int();
	      INT(attr[i+1]) = tok.tt.groups.list[i];
	    }
	  add_fields_to_event_stride(ctx, mod, &event, attr,
				     F_BSM_GROUPS_START,
				     F_BSM_GROUPS_END+n+1);
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
	  attr[F_BSM_NEWGROUPS_LEN] = ovm_int_new();
	  INT(attr[F_BSM_NEWGROUPS_LEN]) = n;
	  for (i=0; i<n; i++)
	    {
	      attr[i+1] = new_ovm_int();
	      INT(attr[i+1]) = tok.tt.groups.list[i];
	    }
	  add_fields_to_event_stride(ctx, mod, &event, attr,
				     F_BSM_NEWGROUPS_START,
				     F_BSM_NEWGROUPS_END+n+1);
	  break;
	}
      case AUT_PROCESS32:
	{
	  ovm_var_t *attr[F_BSM_PROC_SIZE];
	  memset(attr, 0, sizeof(attr));

	  F_INT(PROC_AUID,proc32.auid);
	  F_INT(PROC_EUID,proc32.euid);
	  F_INT(PROC_EGID,proc32.egid);
	  F_INT(PROC_RUID,proc32.ruid);
	  F_INT(PROC_RGID,proc32.rgid);
	  F_INT(PROC_PID,proc32.pid);
	  F_INT(PROC_SID,proc32.sid);
	  F_INT(PROC_PORT,proc32.tid.port);
	  F_IPV4(PROC_IP,proc32.tid.addr);
	  F_WRAP(PROC);
	  break;
	}
      case AUT_IP:
	{
	  ovm_var_t *attr[F_BSM_IP_SIZE];
	  memset(attr, 0, sizeof(attr));

	  F_INT(IP_VERSION,ip.version);
	  F_INT(IP_TOS,ip.tos);
	  F_INT(IP_LEN,ip.len);
	  F_INT(IP_ID,ip.id);
	  F_INT(IP_OFFSET,ip.offset);
	  F_INT(IP_TTL,ip.ttl);
	  F_INT(IP_PROT,ip.prot);
	  F_INT(IP_CHKSM,ip.chksm);
	  F_IPV4(IP_SRC,ip.src);
	  F_IPV4(IP_DEST,ip.dest);
	  F_WRAP(IP);
	  break;
	}
      case AUT_SEQ: /* sequence number */
	{
	  ovm_var_t *attr[F_BSM_SEQ_SIZE];

	  F_INT(SEQ,seq.seqno);
	  F_WRAP(SEQ);
	  break;
	}
      case AUT_EXEC_ARGS:
	{
	  ovm_var_t *attr[F_BSM_EXECARGS_SIZE];
	  memset(attr, 0, sizeof(attr));
	  int i, n;
	  n = tok.tt.execarg.count;
	  if (n>MAX_AUDIT_ARGS)
	    {
	      DebugLog(DF_MOD, DS_TRACE, "bsm_callback(): too many args in exec.\n");
	      n = MAX_AUDIT_ARGS;
	    }
	  attr[F_BSM_EXECARGS_LEN] = ovm_int_new();
	  INT(attr[F_BSM_EXECARGS_LEN]) = n;
	  for (i=0; i<n; i++)
	    {
	      attr[i+1] = new_ovm_vstr();
	      ovm_vstr_t *s = &VSTR(attr[i+1]);
	      s->len = strlen(s->str = tok.tt.execarg.text[i]);
	    }
	  add_fields_to_event_stride(ctx, mod, &event, attr,
				     F_BSM_EXECARGS_START,
				     F_BSM_EXECARGS_END+n+1);
	  break;
	}
      case AUT_EXEC_ENV:
	{
	  ovm_var_t *attr[F_BSM_EXECENV_SIZE];
	  memset(attr, 0, sizeof(attr));
	  int i, n;
	  n = tok.tt.execenv.count;
	  if (n>MAX_AUDIT_ENV)
	    {
	      DebugLog(DF_MOD, DS_TRACE, "bsm_callback(): too many elements in exec environment.\n");
	      n = MAX_AUDIT_ENV;
	    }
	  attr[F_BSM_EXECENV_LEN] = ovm_int_new();
	  INT(attr[F_BSM_EXECENV_LEN]) = n;
	  for (i=0; i<n; i++)
	    {
	      attr[i+1] = new_ovm_vstr();
	      ovm_vstr_t *s = &VSTR(attr[i+1]);
	      s->len = strlen(s->str = tok.tt.execenv.text[i]);
	    }
	  add_fields_to_event_stride(ctx, mod, &event, attr,
				     F_BSM_EXECENV_START,
				     F_BSM_EXECENV_END+n+1);
	  break;
	}
	// !!! Stopped at AUT_UNAUTH (0x3f)
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
  // The following file generated automatically by gen_mod_bsm.c
  // handles the bsm.groups.elt<i>, bsm.newgroups.elt<i>
  // fields
#include "defs_bsm.c"
  // Do not add new fields after this line
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
