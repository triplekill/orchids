/**
 ** @file mod_rawsnare.c
 ** Read from raw snare kernel module output.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 ** @author Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
 **
 ** @version 1.0
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

static int read_io(gc_t *gc_ctx,
		   ovm_var_t *delegate,
		   ovm_var_t *attr[RAWSNARE_FIELDS], header_token_t *hdr);

static int read_pc(gc_t *gc_ctx,
		   ovm_var_t *delegate,
		   ovm_var_t *attr[RAWSNARE_FIELDS], header_token_t *hdr);

static int read_exec(gc_t *gc_ctx,
		     ovm_var_t *delegate,
		     ovm_var_t *attr[RAWSNARE_FIELDS], header_token_t *hdr);

static int read_net(gc_t *gc_ctx,
		    ovm_var_t *delegate,
		    ovm_var_t *attr[RAWSNARE_FIELDS], header_token_t *hdr);

static int read_pt(gc_t *gc_ctx,
		   ovm_var_t *delegate,
		   ovm_var_t *attr[RAWSNARE_FIELDS], header_token_t *hdr);

static int read_kill(gc_t *gc_ctx,
		     ovm_var_t *delegate,
		     ovm_var_t *attr[RAWSNARE_FIELDS], header_token_t *hdr);

static int read_ch(gc_t *gc_ctx,
		   ovm_var_t *delegate,
		   ovm_var_t *attr[RAWSNARE_FIELDS], header_token_t *hdr);

static int read_cp(gc_t *gc_ctx,
		   ovm_var_t *delegate,
		   ovm_var_t *attr[RAWSNARE_FIELDS], header_token_t *hdr);

static int read_su(gc_t *gc_ctx,
		   ovm_var_t *delegate,
		   ovm_var_t *attr[RAWSNARE_FIELDS], header_token_t *hdr);

static int read_ad(gc_t *gc_ctx,
		   ovm_var_t *delegate,
		   ovm_var_t *attr[RAWSNARE_FIELDS], header_token_t *hdr);


static int rawsnare_dissect(orchids_t *ctx, mod_entry_t *mod, event_t *event,
			    void *data, int dissection_level);

static void *rawsnare_preconfig(orchids_t *ctx, mod_entry_t *mod);

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

  val = ovm_uint_new (gc_ctx, io->t_process.pid);
  GC_TOUCH (gc_ctx, attr[F_PID] = val);

  val = ovm_uint_new (gc_ctx, io->t_process.ppid);
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

  val = ovm_uint_new (gc_ctx, pc->t_process.pid);
  GC_TOUCH (gc_ctx, attr[F_PID] = val);

  val = ovm_uint_new (gc_ctx, pc->t_process.ppid);
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

  val = ovm_uint_new (gc_ctx, ex->t_process.pid);
  GC_TOUCH (gc_ctx, attr[F_PID] = val);

  val = ovm_uint_new (gc_ctx, ex->t_process.ppid);
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

  val = ovm_uint_new (gc_ctx, net->t_process.pid);
  GC_TOUCH (gc_ctx, attr[F_PID] = val);

  val = ovm_uint_new (gc_ctx, net->t_process.ppid);
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

  val = ovm_uint_new (gc_ctx, net->t_connection.src_port);
  GC_TOUCH (gc_ctx, attr[F_SRCPORT] = val);

  val = ovm_ipv4_new (gc_ctx);
  IPV4(val).s_addr = inet_addr(net->t_connection.dst_ip);
  GC_TOUCH (gc_ctx, attr[F_DSTIP] = val);

  val = ovm_uint_new (gc_ctx, net->t_connection.dst_port);
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

  val = ovm_uint_new (gc_ctx, pt->t_process.pid);
  GC_TOUCH (gc_ctx, attr[F_PID] = val);

  val = ovm_uint_new (gc_ctx, pt->t_process.ppid);
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

  val = ovm_uint_new (gc_ctx, pt->pid);
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

  val = ovm_uint_new (gc_ctx, kill->t_process.pid);
  GC_TOUCH (gc_ctx, attr[F_PID] = val);

  val = ovm_uint_new (gc_ctx, kill->t_process.ppid);
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

  val = ovm_uint_new (gc_ctx, kill->pid);
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

  val = ovm_uint_new (gc_ctx, ch->t_process.pid);
  GC_TOUCH (gc_ctx, attr[F_PID] = val);

  val = ovm_uint_new (gc_ctx, ch->t_process.ppid);
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

  val = ovm_uint_new (gc_ctx, cp->t_process.pid);
  GC_TOUCH (gc_ctx, attr[F_PID] = val);

  val = ovm_uint_new (gc_ctx, cp->t_process.ppid);
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

  val = ovm_uint_new (gc_ctx, su->t_process.pid);
  GC_TOUCH (gc_ctx, attr[F_PID] = val);

  val = ovm_uint_new (gc_ctx, su->t_process.ppid);
  GC_TOUCH (gc_ctx, attr[F_PPID] = val);

  val = ovm_vstr_new (gc_ctx, delegate);
  VSTR(val) = su->t_process.name;
  VSTRLEN(val) = strlen(su->t_process.name);
  GC_TOUCH (gc_ctx, attr[F_PROCNAME] = val);

  val = ovm_uint_new (gc_ctx, su->t_target.id);
  GC_TOUCH (gc_ctx, attr[F_TARGETID] = val);

  val = ovm_uint_new (gc_ctx, su->t_target.rid);
  GC_TOUCH (gc_ctx, attr[F_TARGETRID] = val);

  val = ovm_uint_new (gc_ctx, su->t_target.sid);
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

  val = ovm_uint_new (gc_ctx, ad->t_process.pid);
  GC_TOUCH (gc_ctx, attr[F_PID] = val);

  val = ovm_uint_new (gc_ctx, ad->t_process.ppid);
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


static int rawsnare_dissect(orchids_t *ctx, mod_entry_t *mod, event_t *event, void *data,
			    int dissection_level)
{
  header_token_t *snare_hdr;
  ovm_var_t *val;
  int err = 0;
  gc_t *gc_ctx = ctx->gc_ctx;
  GC_START(gc_ctx, RAWSNARE_FIELDS+1);
  GC_UPDATE(gc_ctx, RAWSNARE_FIELDS, event);

  DebugLog(DF_MOD, DS_DEBUG, "rawsnare_dissect()\n");

  snare_hdr = (header_token_t *) BSTR(event->value);

  val = ovm_uint_new (gc_ctx, snare_hdr->event_class);
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

      REGISTER_EVENTS(ctx, mod, RAWSNARE_FIELDS, dissection_level);
    }
  GC_END(gc_ctx);
  return err;
}

/*
** Field names must be complete, because they will be used as hash keys
** in the rule compiler... so, no additional memory will be required.
*/

static field_t rawsnare_fields[] = {
  { "rawsnare.time",       &t_timeval, MONO_MONO,  "event time"           },
  { "rawsnare.class",      &t_uint, MONO_UNKNOWN,      "snare event class"    },
  { "rawsnare.syscall",    &t_uint, MONO_UNKNOWN,     "system call number"   },
  { "rawsnare.ruid",       &t_uint, MONO_UNKNOWN,      "user id"              },
  { "rawsnare.rgid",       &t_uint, MONO_UNKNOWN,      "main group id"        },
  { "rawsnare.euid",       &t_uint, MONO_UNKNOWN,      "effective user id"    },
  { "rawsnare.egid",       &t_uint, MONO_UNKNOWN,      "effective id"         },
  { "rawsnare.pid",        &t_uint, MONO_UNKNOWN,      "process id"           },
  { "rawsnare.ppid",       &t_uint, MONO_UNKNOWN,      "parent process id"    },
  { "rawsnare.procname",   &t_str, MONO_UNKNOWN,     "process name"         },
  { "rawsnare.retcode",    &t_int, MONO_UNKNOWN,      "return code"          },
  { "rawsnare.workdir",    &t_str, MONO_UNKNOWN,     "working directory"    },
  { "rawsnare.path",       &t_str, MONO_UNKNOWN,     "path"                 },
  { "rawsnare.mode",       &t_int, MONO_UNKNOWN,      "permissions"          },
  { "rawsnare.createmode", &t_int, MONO_UNKNOWN,      "creation permissions" },
  { "rawsnare.cmdline",    &t_str, MONO_UNKNOWN,     "command line"         },
  { "rawsnare.src_path",   &t_str, MONO_UNKNOWN,     "source path"          },
  { "rawsnare.dst_path",   &t_str, MONO_UNKNOWN,     "destination path"     },
  { "rawsnare.sockcall",   &t_uint, MONO_UNKNOWN,     "socket_call number"   },
  { "rawsnare.dst_ip",     &t_ipv4, MONO_UNKNOWN,     "destination ip"       },
  { "rawsnare.dst_port",   &t_uint, MONO_UNKNOWN,      "destination port"     },
  { "rawsnare.src_ip",     &t_ipv4, MONO_UNKNOWN,     "source ip"            },
  { "rawsnare.src_port",   &t_uint, MONO_UNKNOWN,      "source port"          },
  { "rawsnare.owner_uid",  &t_uint, MONO_UNKNOWN,      "owner user id"        },
  { "rawsnare.owner_gid",  &t_uint, MONO_UNKNOWN,      "owner group id"       },
  { "rawsnare.target_id",  &t_uint, MONO_UNKNOWN,      "caller user/group id" },
  { "rawsnare.target_rid", &t_uint, MONO_UNKNOWN,      "real user/group id"   },
  { "rawsnare.target_sid", &t_uint, MONO_UNKNOWN,      "saved user/group id"  },
  { "rawsnare.mod_name",   &t_str, MONO_UNKNOWN,     "module name"          },
  { "rawsnare.ptrace_req", &t_uint, MONO_UNKNOWN,     "ptrace request"       },
  { "rawsnare.ptrace_pid", &t_uint, MONO_UNKNOWN,      "ptrace pid"           },
  { "rawsnare.ptrace_addr",&t_uint, MONO_UNKNOWN,     "ptrace address"       },
  { "rawsnare.ptrace_data",&t_uint, MONO_UNKNOWN,     "ptrace data"          },
  { "rawsnare.kill_pid",   &t_uint, MONO_UNKNOWN,      "kill dest pid"        },
  { "rawsnare.kill_sig",   &t_uint, MONO_UNKNOWN,     "signal to send"       },
};


static void *rawsnare_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  DebugLog(DF_MOD, DS_INFO, "load() rawsnare@%p\n", (void *) &mod_rawsnare);

  register_fields(ctx, mod, rawsnare_fields, RAWSNARE_FIELDS);

  return (NULL);
}

input_module_t mod_rawsnare = {
  MOD_MAGIC,
  ORCHIDS_VERSION,
  0,			    /* flags */
  "rawsnare",
  "CeCILL2",
  NULL,
  NULL,
  rawsnare_preconfig,
  NULL,
  NULL,
  NULL,
  rawsnare_dissect,
  &t_bstr,		    /* type of fields it expects to dissect */
  NULL,
  NULL,
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
