/**
 ** @file mod_rawsnare.c
 ** Read from raw snare kernel module output.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 ** @date  Started on: Fri Feb 14 18:36:36 2003
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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "orchids.h"
#include "orchids_api.h"

#include "rawsnare.h"

#include "mod_rawsnare.h"

input_module_t mod_rawsnare;

#if 0
static char *linux24_syscall_name_g[256];

static char *linux24_socketcall_name_g[19];

static char *linux24_ptrace_reqname_g[26];

static char *linux24_signal_g[32];
#endif

static int read_io(gc_t *gc_ctx,
		   ovm_var_t *delegate,
		   ovm_var_t *attr[RAWSNARE_FIELDS], header_token_t *hdr)
{
  io_class_t *io;
  ovm_var_t *val;

  io = (io_class_t *) hdr;

  val = ovm_int_new (gc_ctx, io->t_return.returncode);
  GC_TOUCH (gc_ctx, attr[F_RETCODE] = val);

  val = ovm_int_new (gc_ctx, io->t_process.pid);
  GC_TOUCH (gc_ctx, attr[F_PID] = val);

  val = ovm_int_new (gc_ctx, io->t_process.ppid);
  GC_TOUCH (gc_ctx, attr[F_PPID] = val);

  val = ovm_vstr_new (gc_ctx, delegate);
  VSTR(val) = io->t_process.name;
  VSTRLEN(val) = strlen(io->t_process.name);
  GC_TOUCH (gc_ctx, attr[F_PROCNAME] = val);

  val = ovm_vstr_new (gc_ctx, delegate);
  VSTR(val) = io->t_path.path;
  VSTRLEN(val) = strlen(io->t_path.path);
  GC_TOUCH (gc_ctx, attr[F_PATH] = val);

  val = ovm_vstr_new (gc_ctx, delegate);
  VSTR(val) = io->t_pwd.path;
  VSTRLEN(val) = strlen(io->t_pwd.path);
  GC_TOUCH (gc_ctx, attr[F_WORKDIR] = val);

  val = ovm_int_new (gc_ctx, io->t_attributes.mode);
  GC_TOUCH (gc_ctx, attr[F_MODE] = val);

  val = ovm_int_new (gc_ctx, io->t_attributes.createmode);
  GC_TOUCH (gc_ctx, attr[F_CREATEMODE] = val);

  return 0;
}


static int read_pc(gc_t *gc_ctx,
		   ovm_var_t *delegate,
		   ovm_var_t *attr[RAWSNARE_FIELDS], header_token_t *hdr)
{
  ovm_var_t *val;
  pc_class_t *pc;

  pc = (pc_class_t *) hdr;

  val = ovm_int_new (gc_ctx, pc->t_return.returncode);
  GC_TOUCH (gc_ctx, attr[F_RETCODE] = val);

  val = ovm_int_new (gc_ctx, pc->t_process.pid);
  GC_TOUCH (gc_ctx, attr[F_PID] = val);

  val = ovm_int_new (gc_ctx, pc->t_process.ppid);
  GC_TOUCH (gc_ctx, attr[F_PPID] = val);

  val = ovm_vstr_new (gc_ctx, delegate);
  VSTR(val) = pc->t_process.name;
  VSTRLEN(val) = strlen(pc->t_process.name);
  GC_TOUCH (gc_ctx, attr[F_PROCNAME] = val);

  return 0;
}


static int read_exec(gc_t *gc_ctx,
		     ovm_var_t *delegate,
		     ovm_var_t *attr[RAWSNARE_FIELDS], header_token_t *hdr)
{
  ovm_var_t *val;
  ex_class_t *ex;

  ex = (ex_class_t *) hdr;

  val = ovm_int_new (gc_ctx, ex->t_return.returncode);
  GC_TOUCH (gc_ctx, attr[F_RETCODE] = val);

  val = ovm_int_new (gc_ctx, ex->t_process.pid);
  GC_TOUCH (gc_ctx, attr[F_PID] = val);

  val = ovm_int_new (gc_ctx, ex->t_process.ppid);
  GC_TOUCH (gc_ctx, attr[F_PPID] = val);

  val = ovm_vstr_new (gc_ctx, delegate);
  VSTR(val) = ex->t_process.name;
  VSTRLEN(val) = strlen(ex->t_process.name);
  GC_TOUCH (gc_ctx, attr[F_PROCNAME] = val);

  val = ovm_vstr_new (gc_ctx, delegate);
  VSTR(val) = ex->t_path.path;
  VSTRLEN(val) = strlen(ex->t_path.path);
  GC_TOUCH (gc_ctx, attr[F_PATH] = val);

  val = ovm_vstr_new (gc_ctx, delegate);
  VSTR(val) = ex->t_pwd.path;
  VSTRLEN(val) = strlen(ex->t_pwd.path);
  GC_TOUCH (gc_ctx, attr[F_WORKDIR] = val);

  val = ovm_vstr_new (gc_ctx, delegate);
  VSTR(val) = ex->t_execargs.args;
  VSTRLEN(val) = strlen(ex->t_execargs.args);
  GC_TOUCH (gc_ctx, attr[F_CMDLINE] = val);

  return 0;
}

static int read_net(gc_t *gc_ctx,
		    ovm_var_t *delegate,
		    ovm_var_t *attr[RAWSNARE_FIELDS], header_token_t *hdr)
{
  nt_class_t       *net;
  ovm_var_t *val;

  net = (nt_class_t *) hdr;

  val = ovm_int_new (gc_ctx, net->t_return.returncode);
  GC_TOUCH (gc_ctx, attr[F_RETCODE] = val);

  val = ovm_int_new (gc_ctx, net->t_process.pid);
  GC_TOUCH (gc_ctx, attr[F_PID] = val);

  val = ovm_int_new (gc_ctx, net->t_process.ppid);
  GC_TOUCH (gc_ctx, attr[F_PPID] = val);

  val = ovm_vstr_new (gc_ctx, delegate);
  VSTR(val) = net->t_process.name;
  VSTRLEN(val) = strlen(net->t_process.name);
  GC_TOUCH (gc_ctx, attr[F_PROCNAME] = val);

  val = ovm_uint_new (gc_ctx, net->syscall);
  GC_TOUCH (gc_ctx, attr[F_SOCKCALL] = val);
#if 0
  /* XXX -- For demo only */
  if (net->syscall < 20)
    {
      val = ovm_vstr_new (gc_ctx, NULL);
      VSTR(val) = linux24_socketcall_name_g[net->syscall];
      VSTRLEN(val) = strlen(linux24_socketcall_name_g[net->syscall]);
      GC_TOUCH (gc_ctx, attr[F_SOCKCALL] = val);
  }
#endif

  val = ovm_ipv4_new (gc_ctx);
  IPV4(val).s_addr = inet_addr(net->t_connection.src_ip);
  GC_TOUCH (gc_ctx, attr[F_SRCIP] = val);

  val = ovm_int_new (gc_ctx, net->t_connection.src_port);
  GC_TOUCH (gc_ctx, attr[F_SRCPORT] = val);

  val = ovm_ipv4_new (gc_ctx);
  IPV4(val).s_addr = inet_addr(net->t_connection.dst_ip);
  GC_TOUCH (gc_ctx, attr[F_DSTIP] = val);

  val = ovm_int_new (gc_ctx, net->t_connection.dst_port);
  GC_TOUCH (gc_ctx, attr[F_DSTPORT] = val);

  return 0;
}


static int read_pt(gc_t *gc_ctx,
		   ovm_var_t *delegate,
		   ovm_var_t *attr[RAWSNARE_FIELDS], header_token_t *hdr)
{
  pt_class_t       *pt;
  ovm_var_t *val;

  pt = (pt_class_t *) hdr;

  val = ovm_int_new (gc_ctx, pt->t_return.returncode);
  GC_TOUCH (gc_ctx, attr[F_RETCODE] = val);

  val = ovm_int_new (gc_ctx, pt->t_process.pid);
  GC_TOUCH (gc_ctx, attr[F_PID] = val);

  val = ovm_int_new (gc_ctx, pt->t_process.ppid);
  GC_TOUCH (gc_ctx, attr[F_PPID] = val);

  val = ovm_vstr_new (gc_ctx, delegate);
  VSTR(val) = pt->t_process.name;
  VSTRLEN(val) = strlen(pt->t_process.name);
  GC_TOUCH (gc_ctx, attr[F_PROCNAME] = val);

  val = ovm_uint_new (gc_ctx, pt->request);
  GC_TOUCH (gc_ctx, attr[F_PTRACEREQ] = val);
#if 0
  /* XXX -- For demo only */
  if (pt->request < 25)
    {
      val = ovm_vstr_new (gc_ctx, NULL);
      VSTR(val) = linux24_ptrace_reqname_g[ pt->request ];
      VSTRLEN(val) = strlen(linux24_ptrace_reqname_g[pt->request]);
      GC_TOUCH (gc_ctx, attr[F_PTRACEREQ] = val);
    }
#endif

  val = ovm_int_new (gc_ctx, pt->pid);
  GC_TOUCH (gc_ctx, attr[F_PTRACEPID] = val);

  val = ovm_uint_new (gc_ctx, (unsigned long)(void *) pt->addr);
  GC_TOUCH (gc_ctx, attr[F_PTRACEADDR] = val);

  val = ovm_uint_new (gc_ctx, (unsigned long)(void *) pt->data);
  GC_TOUCH (gc_ctx, attr[F_PTRACEDATA] = val);

  return (0);
}


static int read_kill(gc_t *gc_ctx,
		     ovm_var_t *delegate,
		     ovm_var_t *attr[RAWSNARE_FIELDS], header_token_t *hdr)
{
  kill_class_t       *kill;
  ovm_var_t *val;

  kill = (kill_class_t *) hdr;

  val = ovm_int_new (gc_ctx, kill->t_return.returncode);
  GC_TOUCH (gc_ctx, attr[F_RETCODE] = val);

  val = ovm_int_new (gc_ctx, kill->t_process.pid);
  GC_TOUCH (gc_ctx, attr[F_PID] = val);

  val = ovm_int_new (gc_ctx, kill->t_process.ppid);
  GC_TOUCH (gc_ctx, attr[F_PPID] = val);

  val = ovm_vstr_new (gc_ctx, delegate);
  VSTR(val) = kill->t_process.name;
  VSTRLEN(val) = strlen(kill->t_process.name);
  GC_TOUCH (gc_ctx, attr[F_PROCNAME] = val);

  val = ovm_uint_new (gc_ctx, kill->sig);
  GC_TOUCH (gc_ctx, attr[F_KILLSIG] = val);
#if 0
  if (kill->sig < 32)
    {
      val = ovm_vstr_new (gc_ctx, NULL);
      VSTR(val) = linux24_signal_g[kill->sig];
      VSTRLEN(val) = strlen(linux24_signal_g[kill->sig]);
      GC_TOUCH (gc_ctx, attr[F_KILLSIG] = val);
    }
#endif

  val = ovm_int_new (gc_ctx, kill->pid);
  GC_TOUCH (gc_ctx, attr[F_KILLPID] = val);

  return 0;
}


#if 0
static int read_admin(gc_t *gc_ctx,
		      ovm_var_t *delegate,
		      ovm_var_t *attr[RAWSNARE_FIELDS], header_token_t *hdr)
{
  /* XXX: Not Used */

  DPRINTF( ("Warning !...\n") );

  return 0;
}
#endif

static int read_ch(gc_t *gc_ctx,
		   ovm_var_t *delegate,
		   ovm_var_t *attr[RAWSNARE_FIELDS], header_token_t *hdr)
{
  ch_class_t *ch;
  ovm_var_t *val;

  ch = (ch_class_t *) hdr;

  val = ovm_int_new (gc_ctx, ch->t_return.returncode);
  GC_TOUCH (gc_ctx, attr[F_RETCODE] = val);

  val = ovm_int_new (gc_ctx, ch->t_process.pid);
  GC_TOUCH (gc_ctx, attr[F_PID] = val);

  val = ovm_int_new (gc_ctx, ch->t_process.ppid);
  GC_TOUCH (gc_ctx, attr[F_PPID] = val);

  val = ovm_vstr_new (gc_ctx, delegate);
  VSTR(val) = ch->t_process.name;
  VSTRLEN(val) = strlen(ch->t_process.name);
  GC_TOUCH (gc_ctx, attr[F_PROCNAME] = val);

  val = ovm_vstr_new (gc_ctx, delegate);
  VSTR(val) = ch->t_path.path;
  VSTRLEN(val) = strlen(ch->t_path.path);
  GC_TOUCH (gc_ctx, attr[F_PATH] = val);

  val = ovm_vstr_new (gc_ctx, delegate);
  VSTR(val) = ch->t_pwd.path;
  VSTRLEN(val) = strlen(ch->t_pwd.path);
  GC_TOUCH (gc_ctx, attr[F_WORKDIR] = val);

  val = ovm_uint_new (gc_ctx, ch->t_owner.owner);
  GC_TOUCH (gc_ctx, attr[F_OWNERUID] = val);

  val = ovm_uint_new (gc_ctx, ch->t_owner.group);
  GC_TOUCH (gc_ctx, attr[F_OWNERGID] = val);

  return 0;
}


static int read_cp(gc_t *gc_ctx,
		   ovm_var_t *delegate,
		   ovm_var_t *attr[RAWSNARE_FIELDS], header_token_t *hdr)
{
  cp_class_t *cp;
  ovm_var_t *val;

  cp = (cp_class_t *) hdr;

  val = ovm_int_new (gc_ctx, cp->t_return.returncode);
  GC_TOUCH (gc_ctx, attr[F_RETCODE] = val);

  val = ovm_int_new (gc_ctx, cp->t_process.pid);
  GC_TOUCH (gc_ctx, attr[F_PID] = val);

  val = ovm_int_new (gc_ctx, cp->t_process.ppid);
  GC_TOUCH (gc_ctx, attr[F_PPID] = val);

  val = ovm_vstr_new (gc_ctx, delegate);
  VSTR(val) = cp->t_process.name;
  VSTRLEN(val) = strlen(cp->t_process.name);
  GC_TOUCH (gc_ctx, attr[F_PROCNAME] = val);

  val = ovm_vstr_new (gc_ctx, delegate);
  VSTR(val) = cp->t_sourcepath.path;
  VSTRLEN(val) = strlen(cp->t_sourcepath.path);
  GC_TOUCH (gc_ctx, attr[F_SRCPATH] = val);

  val = ovm_vstr_new (gc_ctx, delegate);
  VSTR(val) = cp->t_pwd.path;
  VSTRLEN(val) = strlen(cp->t_pwd.path);
  GC_TOUCH (gc_ctx, attr[F_WORKDIR] = val);

  val = ovm_vstr_new (gc_ctx, delegate);
  VSTR(val) = cp->t_destpath.path;
  VSTRLEN(val) = strlen(cp->t_destpath.path);
  GC_TOUCH (gc_ctx, attr[F_DSTPATH] = val);

  return 0;
}


static int read_su(gc_t *gc_ctx,
		   ovm_var_t *delegate,
		   ovm_var_t *attr[RAWSNARE_FIELDS], header_token_t *hdr)
{
  su_class_t *su;
  ovm_var_t *val;

  su = (su_class_t *) hdr;

  val = ovm_int_new (gc_ctx, su->t_return.returncode);
  GC_TOUCH (gc_ctx, attr[F_RETCODE] = val);

  val = ovm_int_new (gc_ctx, su->t_process.pid);
  GC_TOUCH (gc_ctx, attr[F_PID] = val);

  val = ovm_int_new (gc_ctx, su->t_process.ppid);
  GC_TOUCH (gc_ctx, attr[F_PPID] = val);

  val = ovm_vstr_new (gc_ctx, delegate);
  VSTR(val) = su->t_process.name;
  VSTRLEN(val) = strlen(su->t_process.name);
  GC_TOUCH (gc_ctx, attr[F_PROCNAME] = val);

  val = ovm_int_new (gc_ctx, su->t_target.id);
  GC_TOUCH (gc_ctx, attr[F_TARGETID] = val);

  val = ovm_int_new (gc_ctx, su->t_target.rid);
  GC_TOUCH (gc_ctx, attr[F_TARGETRID] = val);

  val = ovm_int_new (gc_ctx, su->t_target.sid);
  GC_TOUCH (gc_ctx, attr[F_TARGETSID] = val);

  return 0;
}


static int read_ad(gc_t *gc_ctx,
		   ovm_var_t *delegate,
		   ovm_var_t *attr[RAWSNARE_FIELDS], header_token_t *hdr)
{
  ad_class_t *ad;
  ovm_var_t *val;

  ad = (ad_class_t *) hdr;

  val = ovm_int_new (gc_ctx, ad->t_return.returncode);
  GC_TOUCH (gc_ctx, attr[F_RETCODE] = val);

  val = ovm_int_new (gc_ctx, ad->t_process.pid);
  GC_TOUCH (gc_ctx, attr[F_PID] = val);

  val = ovm_int_new (gc_ctx, ad->t_process.ppid);
  GC_TOUCH (gc_ctx, attr[F_PPID] = val);

  val = ovm_vstr_new (gc_ctx, delegate);
  VSTR(val) = ad->t_process.name;
  VSTRLEN(val) = strlen(ad->t_process.name);
  GC_TOUCH (gc_ctx, attr[F_PROCNAME] = val);

  val = ovm_vstr_new (gc_ctx, delegate);
  VSTR(val) = ad->t_name.path;
  VSTRLEN(val) = strlen(ad->t_name.path);
  GC_TOUCH (gc_ctx, attr[F_MODNAME] = val);

  return 0;
}


static int (*read_class[])(gc_t *gc_ctx,
			   ovm_var_t *delegate,
			   ovm_var_t *attr[RAWSNARE_FIELDS],
			   header_token_t *hdr) =
{
  NULL,
  read_io,
  read_pc,
  read_exec,
  read_net,
  NULL, /* read_admin, */
  read_ch,
  read_cp,
  read_su,
  read_ad,
  read_pt,
  read_kill,
  NULL
};


static int rawsnare_dissect(orchids_t *ctx, mod_entry_t *mod, event_t *event, void *data)
{
  header_token_t *snare_hdr;
  ovm_var_t *val;
  int err = 0;
  gc_t *gc_ctx = ctx->gc_ctx;
  GC_START(gc_ctx, RAWSNARE_FIELDS+1);
  GC_UPDATE(gc_ctx, RAWSNARE_FIELDS, event);

  DebugLog(DF_MOD, DS_DEBUG, "rawsnare_dissect()\n");

  snare_hdr = (header_token_t *) BSTR(event->value);

  val = ovm_int_new (gc_ctx, snare_hdr->event_class);
  GC_UPDATE(gc_ctx, F_CLASS, val);

  val = ovm_uint_new (gc_ctx, snare_hdr->event_id);
  GC_UPDATE(gc_ctx, F_SYSCALL, val);
#if 0
  if (snare_hdr->event_id < LIN24_SYSCALL_MAX)
    {
      /* XXX -- Text for demo !!! */
      val = ovm_vstr_new (gc_ctx, NULL);
      VSTR(val) = linux24_syscall_name_g[snare_hdr->event_id];
      VSTRLEN(val) = strlen(VSTR(val));
      GC_UPDATE(gc_ctx, F_SYSCALL, val);
  }
#endif

  val = ovm_timeval_new (gc_ctx);
  TIMEVAL(val) = snare_hdr->time;
  GC_UPDATE(gc_ctx, F_TIME, val);

  val = ovm_uint_new(gc_ctx, snare_hdr->user_id);
  GC_UPDATE(gc_ctx, F_RUID, val);

  val = ovm_uint_new(gc_ctx, snare_hdr->group_id);
  GC_UPDATE(gc_ctx, F_RGID, val);

  val = ovm_uint_new(gc_ctx, snare_hdr->euser_id);
  GC_UPDATE(gc_ctx, F_EUID, val);

  val = ovm_uint_new(gc_ctx, snare_hdr->egroup_id);
  GC_UPDATE(gc_ctx, F_EGID, val);

  if (snare_hdr->event_class > NUMCLASS)
    {
      DebugLog(DF_MOD, DS_WARN, "bad event class.\n");
      err = 1;
    }
  else
    {
      if (read_class[ snare_hdr->event_class ])
	(*read_class[ snare_hdr->event_class ]) (gc_ctx,
						 event->value,
						 (ovm_var_t **)GC_DATA(),
						 snare_hdr);

      REGISTER_EVENTS(ctx, mod, RAWSNARE_FIELDS);
    }
  GC_END(gc_ctx);
  return err;
}

/*
** field name must be complete, because it will be used as hashkey
** for rule compiler... so, no more memory will be required...
*/

static field_t rawsnare_fields[] = {
  { "rawsnare.time",       &t_timeval,  "event time"           },
  { "rawsnare.class",      &t_int,      "snare event class"    },
  { "rawsnare.syscall",    &t_uint,     "system call number"   },
  { "rawsnare.ruid",       &t_uint,      "user id"              },
  { "rawsnare.rgid",       &t_uint,      "main group id"        },
  { "rawsnare.euid",       &t_uint,      "effective user id"    },
  { "rawsnare.egid",       &t_uint,      "effective id"         },
  { "rawsnare.pid",        &t_int,      "process id"           },
  { "rawsnare.ppid",       &t_int,      "parent process id"    },
  { "rawsnare.procname",   &t_str,     "process name"         },
  { "rawsnare.retcode",    &t_int,      "return code"          },
  { "rawsnare.workdir",    &t_str,     "working directory"    },
  { "rawsnare.path",       &t_str,     "path"                 },
  { "rawsnare.mode",       &t_int,      "permissions"          },
  { "rawsnare.createmode", &t_int,      "creation permissions" },
  { "rawsnare.cmdline",    &t_str,     "command line"         },
  { "rawsnare.src_path",   &t_str,     "source path"          },
  { "rawsnare.dst_path",   &t_str,     "destination path"     },
  { "rawsnare.sockcall",   &t_uint,     "socket_call number"   },
  { "rawsnare.dst_ip",     &t_ipv4,     "destination ip"       },
  { "rawsnare.dst_port",   &t_int,      "destination port"     },
  { "rawsnare.src_ip",     &t_ipv4,     "source ip"            },
  { "rawsnare.src_port",   &t_int,      "source port"          },
  { "rawsnare.owner_uid",  &t_uint,      "owner user id"        },
  { "rawsnare.owner_gid",  &t_uint,      "owner group id"       },
  { "rawsnare.target_id",  &t_int,      "caller user/group id" },
  { "rawsnare.target_rid", &t_int,      "real user/group id"   },
  { "rawsnare.target_sid", &t_int,      "saved user/group id"  },
  { "rawsnare.mod_name",   &t_str,     "module name"          },
  { "rawsnare.ptrace_req", &t_uint,     "ptrace request"       },
  { "rawsnare.ptrace_pid", &t_int,      "ptrace pid"           },
  { "rawsnare.ptrace_addr",&t_uint,     "ptrace address"       },
  { "rawsnare.ptrace_data",&t_uint,     "ptrace data"          },
  { "rawsnare.kill_pid",   &t_int,      "kill dest pid"        },
  { "rawsnare.kill_sig",   &t_uint,     "signal to send"       },
};


static void *
rawsnare_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  DebugLog(DF_MOD, DS_INFO, "load() rawsnare@%p\n", (void *) &mod_rawsnare);

  register_fields(ctx, mod, rawsnare_fields, RAWSNARE_FIELDS);

  return (NULL);
}

/*
int
generic_dissect(orchids_t *ctx, mod_entry_t *mod, event_t *event, void *data)
{
  return rawsnare_dissect(ctx, mod, event, data);
}
*/

static char *rawsnare_deps[] = {
  "udp",
  NULL
};

input_module_t mod_rawsnare = {
  MOD_MAGIC,
  ORCHIDS_VERSION,
  0,			    /* flags */
  "rawsnare",
  "CeCILL2",
  rawsnare_deps,
  NULL,
  rawsnare_preconfig,
  NULL,
  NULL,
  rawsnare_dissect,
  &t_bstr		    /* type of fields it expects to dissect */
};

#if 0
static char *linux24_syscall_name_g[255] = {
  "",
  "SYS_exit",
  "SYS_fork",
  "SYS_read",
  "SYS_write",
  "SYS_open",
  "SYS_close",
  "SYS_waitpid",
  "SYS_creat",
  "SYS_link",
  "SYS_unlink",
  "SYS_execve",
  "SYS_chdir",
  "SYS_time",
  "SYS_mknod",
  "SYS_chmod",
  "SYS_lchown",
  "SYS_break",
  "SYS_oldstat",
  "SYS_lseek",
  "SYS_getpid",
  "SYS_mount",
  "SYS_umount",
  "SYS_setuid",
  "SYS_getuid",
  "SYS_stime",
  "SYS_ptrace",
  "SYS_alarm",
  "SYS_oldfstat",
  "SYS_pause",
  "SYS_utime",
  "SYS_stty",
  "SYS_gtty",
  "SYS_access",
  "SYS_nice",
  "SYS_ftime",
  "SYS_sync",
  "SYS_kill",
  "SYS_rename",
  "SYS_mkdir",
  "SYS_rmdir",
  "SYS_dup",
  "SYS_pipe",
  "SYS_times",
  "SYS_prof",
  "SYS_brk",
  "SYS_setgid",
  "SYS_getgid",
  "SYS_signal",
  "SYS_geteuid",
  "SYS_getegid",
  "SYS_acct",
  "SYS_umount2",
  "SYS_lock",
  "SYS_ioctl",
  "SYS_fcntl",
  "SYS_mpx",
  "SYS_setpgid",
  "SYS_ulimit",
  "SYS_oldolduname",
  "SYS_umask",
  "SYS_chroot",
  "SYS_ustat",
  "SYS_dup2",
  "SYS_getppid",
  "SYS_getpgrp",
  "SYS_setsid",
  "SYS_sigaction",
  "SYS_sgetmask",
  "SYS_ssetmask",
  "SYS_setreuid",
  "SYS_setregid",
  "SYS_sigsuspend",
  "SYS_sigpending",
  "SYS_sethostname",
  "SYS_setrlimit",
  "SYS_getrlimit",
  "SYS_getrusage",
  "SYS_gettimeofday",
  "SYS_settimeofday",
  "SYS_getgroups",
  "SYS_setgroups",
  "SYS_select",
  "SYS_symlink",
  "SYS_oldlstat",
  "SYS_readlink",
  "SYS_uselib",
  "SYS_swapon",
  "SYS_reboot",
  "SYS_readdir",
  "SYS_mmap",
  "SYS_munmap",
  "SYS_truncate",
  "SYS_ftruncate",
  "SYS_fchmod",
  "SYS_fchown",
  "SYS_getpriority",
  "SYS_setpriority",
  "SYS_profil",
  "SYS_statfs",
  "SYS_fstatfs",
  "SYS_ioperm",
  "SYS_socketcall",
  "SYS_syslog",
  "SYS_setitimer",
  "SYS_getitimer",
  "SYS_stat",
  "SYS_lstat",
  "SYS_fstat",
  "SYS_olduname",
  "SYS_iopl",
  "SYS_vhangup",
  "SYS_idle",
  "SYS_vm86old",
  "SYS_wait4",
  "SYS_swapoff",
  "SYS_sysinfo",
  "SYS_ipc",
  "SYS_fsync",
  "SYS_sigreturn",
  "SYS_clone",
  "SYS_setdomainname",
  "SYS_uname",
  "SYS_modify_ldt",
  "SYS_adjtimex",
  "SYS_mprotect",
  "SYS_sigprocmask",
  "SYS_create_module",
  "SYS_init_module",
  "SYS_delete_module",
  "SYS_get_kernel_syms",
  "SYS_quotactl",
  "SYS_getpgid",
  "SYS_fchdir",
  "SYS_bdflush",
  "SYS_sysfs",
  "SYS_personality",
  "SYS_afs_syscall",
  "SYS_setfsuid",
  "SYS_setfsgid",
  "SYS__llseek",
  "SYS_getdents",
  "SYS__newselect",
  "SYS_flock",
  "SYS_msync",
  "SYS_readv",
  "SYS_writev",
  "SYS_getsid",
  "SYS_fdatasync",
  "SYS__sysctl",
  "SYS_mlock",
  "SYS_munlock",
  "SYS_mlockall",
  "SYS_munlockall",
  "SYS_sched_setparam",
  "SYS_sched_getparam",
  "SYS_sched_setscheduler",
  "SYS_sched_getscheduler",
  "SYS_sched_yield",
  "SYS_sched_get_priority_max",
  "SYS_sched_get_priority_min",
  "SYS_sched_rr_get_interval",
  "SYS_nanosleep",
  "SYS_mremap",
  "SYS_setresuid",
  "SYS_getresuid",
  "SYS_vm86",
  "SYS_query_module",
  "SYS_poll",
  "SYS_nfsservctl",
  "SYS_setresgid",
  "SYS_getresgid",
  "SYS_prctl",
  "SYS_rt_sigreturn",
  "SYS_rt_sigaction",
  "SYS_rt_sigprocmask",
  "SYS_rt_sigpending",
  "SYS_rt_sigtimedwait",
  "SYS_rt_sigqueueinfo",
  "SYS_rt_sigsuspend",
  "SYS_pread",
  "SYS_pwrite",
  "SYS_chown",
  "SYS_getcwd",
  "SYS_capget",
  "SYS_capset",
  "SYS_sigaltstack",
  "SYS_sendfile",
  "SYS_getpmsg",
  "SYS_putpmsg",
  "SYS_vfork",
  "SYS_ugetrlimit",
  "SYS_mmap2",
  "SYS_truncate64",
  "SYS_ftruncate64",
  "SYS_stat64",
  "SYS_lstat64",
  "SYS_fstat64",
  "SYS_lchown32",
  "SYS_getuid32",
  "SYS_getgid32",
  "SYS_geteuid32",
  "SYS_getegid32",
  "SYS_setreuid32",
  "SYS_setregid32",
  "SYS_getgroups32",
  "SYS_setgroups32",
  "SYS_fchown32",
  "SYS_setresuid32",
  "SYS_getresuid32",
  "SYS_setresgid32",
  "SYS_getresgid32",
  "SYS_chown32",
  "SYS_setuid32",
  "SYS_setgid32",
  "SYS_setfsuid32",
  "SYS_setfsgid32",
  "SYS_pivot_root",
  "SYS_mincore",
  "SYS_madvise",
  "SYS_madvise1",
  "SYS_getdents64",
  "SYS_fcntl64",
  "SYS_security",
  "SYS_gettid",
  "SYS_readahead",
  "SYS_setxattr",
  "SYS_lsetxattr",
  "SYS_fsetxattr",
  "SYS_getxattr",
  "SYS_lgetxattr",
  "SYS_fgetxattr",
  "SYS_listxattr",
  "SYS_llistxattr",
  "SYS_flistxattr",
  "SYS_removexattr",
  "SYS_lremovexattr",
  "SYS_fremovexattr",
  "SYS_tkill",
  "SYS_sendfile64",
  "SYS_futex",
  "SYS_sched_setaffinity",
  "SYS_sched_getaffinity"
};

static char *linux24_socketcall_name_g[19] = {
  "Not Defined",
  "SYS_SOCKET",
  "SYS_BIND",
  "SYS_CONNECT",
  "SYS_LISTEN",
  "SYS_ACCEPT",
  "SYS_GETSOCKNAME",
  "SYS_GETPEERNAME",
  "SYS_SOCKETPAIR",
  "SYS_SEND",
  "SYS_RECV",
  "SYS_SENDTO",
  "SYS_RECVFROM",
  "SYS_SHUTDOWN",
  "SYS_SETSOCKOPT",
  "SYS_GETSOCKOPT",
  "SYS_SENDMSG",
  "SYS_RECVMSG",
  NULL
}

PTRACE_TRACEME = 0,
PTRACE_PEEKTEXT = 1,
PTRACE_PEEKDATA = 2,
PTRACE_PEEKUSER = 3,
PTRACE_POKETEXT = 4,
PTRACE_POKEDATA = 5,
PTRACE_POKEUSER = 6,
PTRACE_CONT = 7,
PTRACE_KILL = 8,
PTRACE_SINGLESTEP = 9,
PTRACE_GETREGS = 12,
PTRACE_SETREGS = 13,
PTRACE_GETFPREGS = 14,
PTRACE_SETFPREGS = 15,
PTRACE_ATTACH = 16,
PTRACE_DETACH = 17,
PTRACE_GETFPXREGS = 18,
PTRACE_SETFPXREGS = 19,
PTRACE_SYSCALL = 24

static char *linux24_ptrace_reqname_g[25] = {
"PTRACE_TRACEME"
"PTRACE_PEEKTEXT"
"PTRACE_PEEKDATA"
"PTRACE_PEEKUSER"
"PTRACE_POKETEXT"
"PTRACE_POKEDATA"
"PTRACE_POKEUSER"
"PTRACE_CONT"
"PTRACE_KILL"
"PTRACE_SINGLESTEP"
"N/A"
"N/A"
"PTRACE_GETREGS"
"PTRACE_SETREGS"
"PTRACE_GETFPREGS"
"PTRACE_SETFPREGS"
"PTRACE_ATTACH"
"PTRACE_DETACH"
"PTRACE_GETFPXREGS"
"PTRACE_SETFPXREGS"
"N/A"
"N/A"
"N/A"
"N/A"
"PTRACE_SYSCALL"
}

static char *linux24_ptrace_reqname_g[25] = {
"(0) PTRACE_TRACEME",
"(1) PTRACE_PEEKTEXT",
"(2) PTRACE_PEEKDATA",
"(3) PTRACE_PEEKUSER",
"(4) PTRACE_POKETEXT",
"(5) PTRACE_POKEDATA",
"(6) PTRACE_POKEUSER",
"(7) PTRACE_CONT",
"(8) PTRACE_KILL",
"(9) PTRACE_SINGLESTEP",
"(10) N/A",
"(11) N/A",
"(12) PTRACE_GETREGS",
"(13) PTRACE_SETREGS",
"(14) PTRACE_GETFPREGS",
"(15) PTRACE_SETFPREGS",
"(16) PTRACE_ATTACH",
"(17) PTRACE_DETACH",
"(18) PTRACE_GETFPXREGS",
"(19) PTRACE_SETFPXREGS",
"(20) N/A",
"(21) N/A",
"(22) N/A",
"(23) N/A",
"(24) PTRACE_SYSCALL"
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

static char *linux24_socketcall_name_g[19] = {
  "(0) Not Defined",
  "(1) SYS_SOCKET",
  "(2) SYS_BIND",
  "(3) SYS_CONNECT",
  "(4) SYS_LISTEN",
  "(5) SYS_ACCEPT",
  "(6) SYS_GETSOCKNAME",
  "(7) SYS_GETPEERNAME",
  "(8) SYS_SOCKETPAIR",
  "(9) SYS_SEND",
  "(10) SYS_RECV",
  "(11) SYS_SENDTO",
  "(12) SYS_RECVFROM",
  "(13) SYS_SHUTDOWN",
  "(14) SYS_SETSOCKOPT",
  "(15) SYS_GETSOCKOPT",
  "(16) SYS_SENDMSG",
  "(17) SYS_RECVMSG",
  NULL
};

static char *linux24_ptrace_reqname_g[26] = {
  "(0) PTRACE_TRACEME",
  "(1) PTRACE_PEEKTEXT",
  "(2) PTRACE_PEEKDATA",
  "(3) PTRACE_PEEKUSER",
  "(4) PTRACE_POKETEXT",
  "(5) PTRACE_POKEDATA",
  "(6) PTRACE_POKEUSER",
  "(7) PTRACE_CONT",
  "(8) PTRACE_KILL",
  "(9) PTRACE_SINGLESTEP",
  "(10) N/A",
  "(11) N/A",
  "(12) PTRACE_GETREGS",
  "(13) PTRACE_SETREGS",
  "(14) PTRACE_GETFPREGS",
  "(15) PTRACE_SETFPREGS",
  "(16) PTRACE_ATTACH",
  "(17) PTRACE_DETACH",
  "(18) PTRACE_GETFPXREGS",
  "(19) PTRACE_SETFPXREGS",
  "(20) N/A",
  "(21) N/A",
  "(22) N/A",
  "(23) N/A",
  "(24) PTRACE_SYSCALL",
  NULL
};

static char *linux24_signal_g[32] = {
  "(0) N/A",
  "(1) SIGHUP",
  "(2) SIGINT",
  "(3) SIGQUIT",
  "(4) SIGILL",
  "(5) SIGTRAP",
  "(6) SIGABRT",
  "(7) SIGBUS",
  "(8) SIGFPE",
  "(9) SIGKILL",
  "(10) SIGUSR1",
  "(11) SIGSEGV",
  "(12) SIGUSR2",
  "(13) SIGPIPE",
  "(14) SIGALRM",
  "(15) SIGTERM",
  "(16) SIGSTKFLT",
  "(17) SIGCHLD",
  "(18) SIGCONT",
  "(19) SIGSTOP",
  "(20) SIGTSTP",
  "(21) SIGTTIN",
  "(22) SIGTTOU",
  "(23) SIGURG",
  "(24) SIGXCPU",
  "(25) SIGXFSZ",
  "(26) SIGVTALRM",
  "(27) SIGPROF",
  "(28) SIGWINCH",
  "(29) SIGIO",
  "(30) SIGPWR",
  "(31) SIGSYS",
};
#endif


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
