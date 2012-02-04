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
  ovm_var_t *attr[BSM_FIELDS];

  DebugLog(DF_MOD, DS_TRACE, "bsm_callback()\n");
  memset(attr, 0, sizeof(attr));

  u_char *buf, *buf0, *bufend;
  int n;
  n = au_read_rec ((FILE *)data, &buf);
  // !!! au_read_rec() requires a FILE *, but we only have a fildes sd;
  // should make data = fdopen(sd, "r")
  if (n<0)
    {
      DebugLog(DF_MOD, DS_TRACE, "bsm_callback(): cannot read bsm record.\n");
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
	break;
      case AUT_HEADER32_EX: // almost the same as AUT_HEADER32
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
	break;
      case AUT_PATH:
	// used if syscall has a path argument.
	// E.g., in fsctl():
	//     int fsctl(const char *path, unsigned long request, void *data, unsigned long options);
	// argument 1 is passed in the path field (and will be stored
	// in the bsm.path Orchids field, not in bsm.arg1)
	attr[F_BSM_PATH] = ovm_vstr_new();
	{
	  ovm_vstr_t *s = &VSTR(attr[F_BSM_PATH]);
	  s->len = tok.tt.path.len;
	  s->str = tok.tt.path.path;
	  // warning: apparently, a path field may be given twice in a row.
	  // I do not know why.  When it happens, it seems to be the same
	  // value anyway.
	}
	break;
      case AUT_TEXT: // I don't know what this is used for
	attr[F_BSM_TEXT] = ovm_vstr_new();
	{
	  ovm_vstr_t *s = &VSTR(attr[F_BSM_TEXT]);
	  s->len = tok.tt.text.len;
	  s->str = tok.tt.text.path;
	}
	break;
      case AUT_SUBJECT: /* = case AUT_SUBJECT32: */
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
	break;
      case AUT_RETURN32:
	attr[F_BSM_RET_VAL] = new_ovm_int();
	INT(attr[F_BSM_RET_VAL]) = tok.tt.exit.ret;
	attr[F_BSM_RET_STATUS] = new_ovm_int();
	INT(attr[F_BSM_RET_STATUS]) = tok.tt.exit.status;
	break;
      case AUT_ATTR32:
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
	break;
      case AUT_ARG32:
	if (tok.tt.arg32.no>F_BSM_MAX_ARGS || tok.tt.arg32.no<1)
	  {
	    DebugLog(DF_MOD, DS_TRACE, "bsm_callback(): argument number out of bounds.\n");
	  }
	else
	  {
	    int i = F_BSM_ARG_START+tok.tt.arg32.no-1;

	    attr[i] = new_ovm_int();
	    INT(attr[i]) = tok.tt.arg32.val;
	    // I'm not recording the tok.tt.arg32.text field,
	    // which is merely the name of the formal parameter
	    // to the syscall
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

	    attr[i] = new_ovm_int();
	    INT(attr[i]) = tok.tt.arg64.val; /* this is 64 bits, */
	    /* hopefully this will hold in a long (=ovm_int_t)
	       the argument is that 64 bit values only occur on
	       64-bit machines anyway (this is not quite right,
	       since in particular an Orchids on a 32-bit machine
	       might monitor events from a 64-bit machine).
	     */
	    // I'm not recording the tok.tt.arg64.text field,
	    // which is merely the name of the formal parameter
	    // to the syscall
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
	break;
      case AUT_IN_ADDR:
	attr[F_BSM_INADDR] = new_ovm_ipv4();
	IPV4(attr[F_BSM_INADDR])->s_addr = tok.tt.inaddr.addr;
	break;
      case AUT_IPORT:
	attr[F_BSM_IPORT] = new_ovm_int();
	INT(attr[F_BSM_IPORT]) = tok.tt.iport.port;
	break;
      case AUT_SOCKET:
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
	break;
	// !!! Stopped at AUT_EXEC_ARGS (0x3c)
	// The following are ignored
	// First, those that are ignored because I don't know
	// what they are for, or which subfields of tok.tt should be used
      case AUT_DATA:
      case AUT_IPC:
      case AUT_XATPATH: /* maybe some extra (X) path argument? or is it
			 an X-Window related event? */
      case AUT_OPAQUE: /* seems to be some form of unusable, catch-all type */
      case AUT_ACL:
      case AUT_IPC_PERM:
      case AUT_LABEL:
      case AUT_GROUPS: /* [au_groups_t] list of integers? */
      case AUT_ACE:
      case AUT_PRIV:
      case AUT_UPRIV:
      case AUT_LIAISON:
      case AUT_NEWGROUPS:
	// Then, those I am not sure of
      case AUT_PROCESS32: /* something that describes a process: what is
			     the difference with AUT_ATTR32? */
      case AUT_ATTR: /* duplicate of AUT_ATTR32? */
	// Then, those I fear are out of scope
      case AUT_IP: /* apparently meant to monitor IP packets
		      I only want to do system monitoring here */
      case AUT_SEQ: /* sequence number */
	// Then, those that are obsolete
      case AUT_OHEADER: // for "old header"
      default:
	DebugLog(DF_MOD, DS_TRACE, "bsm_callback(): unknown token id.\n");
	break;
      }
  } while (buf < bufend);

  /*  fill in orchids event */
  event_t *event;
  event = NULL;
  add_fields_to_event(ctx, mod, &event, attr, AUDITD_FIELDS);

  /* then, post the Orchids event */
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
  // The following are written assuming F_BSM_MAX_ARGS==4
  // Do not add new fields after this line
  // unless they are bsm.arg<i> fields;
  // in which case please update F_BSM_MAX_ARGS
  { "bsm.arg1", T_INT, "bsm argument 1" },
  { "bsm.arg2", T_INT, "bsm argument 2" },
  { "bsm.arg3", T_INT, "bsm argument 3" },
  { "bsm.arg4", T_INT, "bsm argument 4" },
#ifdef F_BSM_NEED_MORE
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
#endif
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
