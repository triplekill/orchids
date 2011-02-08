/**
 ** @file mod_remoteadm.c
 ** Add a remote administration console.
 ** Used to test/debug/orchids internals at runtime.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 ** @date  Started on: Mon Jan 27 17:32:49 2003
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
#include <netdb.h>

#include "orchids.h"

#include "engine.h"
#include "graph_output.h"
#include "mod_mgr.h"
#include "orchids_api.h"
#include "rule_compiler.h"

#include "mod_remoteadm.h"


input_module_t mod_remoteadm;


static void
show_prompt(FILE *fp)
{
  fprintf(fp, "orchids> ");
  fflush(fp);
}


static int
create_tcplisten_socket(void)
{
  int fd, on = 1;
  struct sockaddr_in sin;

  fd = Xsocket(AF_INET, SOCK_STREAM, 0);

  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(4242);

  Xsetsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

  Xbind(fd, (struct sockaddr *) &sin, sizeof(sin));

  Xlisten(fd, 5);

  return (fd);
}


static int
remoteadm_listen_callback(orchids_t *ctx, mod_entry_t *mod, int fd, void *data)
{
  struct sockaddr_in cliadd;
  socklen_t clilen;
  int adm_fd;
  FILE *fp;

  DebugLog(DF_MOD, DS_DEBUG, "remoteadm_listen_callback()\n");

  clilen = sizeof (cliadd);
  adm_fd = Xaccept(fd, (struct sockaddr *) &cliadd, &clilen);

  DebugLog(DF_MOD, DS_DEBUG,
           "accept from = %s\n", inet_ntoa(cliadd.sin_addr));

  fp = Xfdopen(adm_fd, "w");
  add_input_descriptor(ctx, mod, remoteadm_client_callback, adm_fd, fp);

  fprintf(fp, "%s\n", ORCHIDS_BANNER);
  fprintf(fp, "Welcome to ORCHIDS Remote Admin Console...\n\norchids> ");
  fflush(fp);

  return (0);
}


remoteadm_cmd_t radm_cmd_g[] = {
  { "help", radm_cmd_help, "give this beautiful help table" },
  { "?", radm_cmd_help, "same as 'help'" },
  { "lsmod", radm_cmd_lsmod, "list loaded modules" },
  { "cfgtree", radm_cmd_cfgtree, "show current configuration tree" },
  { "showdirs", radm_cmd_showdirs, "show known configuration directives" },
  { "showfields", radm_cmd_showfields, "show registered fields" },
  { "stats", radm_cmd_stats, "show orchids statistics" },
  { "lsrules", radm_cmd_lsrules, "list rules" },
  { "lsinsts", radm_cmd_lsinstances, "list rule instances" },
  { "lsthreads", radm_cmd_lsthreads, "list retrig queue" },
  { "lsevents", radm_cmd_lsevents, "list active events list" },
  { "lsfuncts", radm_cmd_lsfunctions, "list language functions" },
  { "dumpinst", radm_cmd_dumpinst, "dump a rule instance in AT&T GraphViz dot format" },
  { "dumprule", radm_cmd_dumprule, "dump rule in AT&T GraphViz dot format" },
  { "htmloutput", radm_cmd_htmloutput, "Request an html output generation" },
  { "about", radm_cmd_about, "some greetings ;-)" },
  { "exit", radm_cmd_exit, "close admin console" },
  { "quit", radm_cmd_exit, "same as 'exit'" },
  { "close", radm_cmd_exit, "same as 'exit'" },
  { "bye", radm_cmd_exit, "same as 'exit'" },
  { "shutdown", radm_cmd_shutdown, "shutdown orchids process"},
  { "feedback", radm_cmd_feedback, "set event feedback address"},
  { NULL, NULL, NULL }
};


static void
radm_cmd_help(FILE *fp, orchids_t *ctx, char *args)
{
  int i;

  fprintf(fp, "-----------+-------------------------------------------\n");
  fprintf(fp, "  command  | description\n");
  fprintf(fp, "-----------+-------------------------------------------\n");

  for (i = 0; radm_cmd_g[i].name; i++)
    fprintf(fp, "%10s | %.64s\n", radm_cmd_g[i].name, radm_cmd_g[i].help);

  fprintf(fp, "-----------+-------------------------------------------\n");

  show_prompt(fp);
}


static void
radm_cmd_lsmod(FILE *fp, orchids_t *ctx, char *args)
{
  fprintf_loaded_modules(ctx, fp);
  show_prompt(fp);
}


static void
radm_cmd_cfgtree(FILE *fp, orchids_t *ctx, char *args)
{
  fprintf_cfg_tree(fp, ctx->cfg_tree);
  show_prompt(fp);
}


static void
radm_cmd_showdirs(FILE *fp, orchids_t *ctx, char *args)
{
  fprintf_directives(fp, ctx);
  show_prompt(fp);
}


static void
radm_cmd_showfields(FILE *fp, orchids_t *ctx, char *args)
{
  fprintf_fields(fp, ctx);
  show_prompt(fp);
}


static void
radm_cmd_htmloutput(FILE *fp, orchids_t *ctx, char *args)
{
  fprintf(fp, "Generating HTML output...\n");
  call_mod_func(ctx, "htmlstate", "do_update", NULL);
  /* XXX: test return */
  show_prompt(fp);
}


static void
radm_cmd_exit(FILE *fp, orchids_t *ctx, char *args)
{
  fprintf(fp, "\nClosing connection... Goodbye...\n\n");
  fflush(fp);
  del_input_descriptor(ctx, fileno(fp));
  Xfclose(fp);
}


static void
radm_cmd_shutdown(FILE *fp, orchids_t *ctx, char *args)
{
  DebugLog(DF_MOD, DS_CRIT, "\n\n  shuting down orchids...\n\n");

  fprintf(fp, "\nStoping Orchids... Goodbye...\n\n");
  fflush(fp);
  del_input_descriptor(ctx, fileno(fp));
  Xfclose(fp);
  exit(EXIT_SUCCESS);
}


static void
radm_cmd_about(FILE *fp, orchids_t *ctx, char *args)
{
  /* [XXX] Put real credits here... */
  fprintf(fp, "%s%s", RADM_FLOWER1, RADM_FLOWER2);
  fprintf(fp, "%s\n", RADM_BANNER);
  /* fprintf(fp, "\nORCHIDS... LSV(c) 2002-2003\n\n"); */
  show_prompt(fp);
}


static void
radm_cmd_stats(FILE *fp, orchids_t *ctx, char *args)
{
  fprintf_orchids_stats(fp, ctx);
  show_prompt(fp);
}


static void
radm_cmd_lsrules(FILE *fp, orchids_t *ctx, char *args)
{
  fprintf_rule_stats(ctx, fp);
  show_prompt(fp);
}


static void
radm_cmd_lsinstances(FILE *fp, orchids_t *ctx, char *args)
{
  fprintf_rule_instances(fp, ctx);
  show_prompt(fp);
}


static void
radm_cmd_dumpinst(FILE *fp, orchids_t *ctx, char *args)
{
  rule_instance_t *r;
  int rid;

  r = NULL;

  if (ctx->first_rule_instance == NULL) {
    fprintf(fp, "no active rule instance.\n");
    show_prompt(fp);
    return ;
  }

  if (args == NULL) {
    fprintf(fp, "you must give a rule instance id.\n");
    show_prompt(fp);
    return ;
  }

  if ((args[0] >= '0') && (args[0] <= '9')) {
    rid = atoi(args);
    for (r = ctx->first_rule_instance; rid > 0 && r; --rid, r = r->next)
      ;
    if (rid) {
      fprintf(fp, "rule instance %i doesn't exists.\n", atoi(args));
      show_prompt(fp);
      return ;
    }
    fprintf_rule_instance_dot(fp,
                              r,
                              DOT_RETRIGLIST,
                              ctx->new_qh,
                              RADM_STATE_LIMIT);
    show_prompt(fp);
  }
  else {
    fprintf(fp, "invalid rule instance identifier '%s'.\n", args);
    show_prompt(fp);
    return ;
  }
}


static void
radm_cmd_dumprule(FILE *fp, orchids_t *ctx, char *args)
{
  rule_t *r;
  int rn;

  r = NULL;

  if (args == NULL) {
    fprintf(fp, "you must give a rule name or id.\n");
    show_prompt(fp);
    return ;
  }

  if (((args[0] - '0') >= 0) && ((args[0] - '0') <= 9)) {
    rn = atoi(args);
    for (r = ctx->rule_compiler->first_rule; rn > 0 && r; --rn, r = r->next)
      ;
  } else {
    r = strhash_get(ctx->rule_compiler->rulenames_hash, args);
  }

  if (r == NULL) {
    fprintf(fp, "invalid rule identifier '%s'.\n", args);
    show_prompt(fp);
    return ;
  }

  fprintf_rule_dot(fp, r);
  show_prompt(fp);
}


static void
radm_cmd_lsthreads(FILE *fp, orchids_t *ctx, char *args)
{
/*   fprintf_thread_queue(fp, ctx, ctx->cur_retrig_qh); */
  fprintf_thread_queue(fp, ctx, ctx->new_qh);
  show_prompt(fp);
}


static void
radm_cmd_lsevents(FILE *fp, orchids_t *ctx, char *args)
{
  fprintf_active_events(fp, ctx);
  show_prompt(fp);
}


static void
radm_cmd_lsfunctions(FILE *fp, orchids_t *ctx, char *args)
{
  fprintf_issdl_functions(fp, ctx);
  show_prompt(fp);
}


static void
radm_cmd_feedback(FILE *fp, orchids_t *ctx, char *args)
{
  struct sockaddr_in srvaddr;
  char hostname[256];
  int port;
  struct hostent *he;
  int evt_fb_sd;

  if (args == NULL) {
    fprintf(fp, "disable event feedback.\n");
    if (ctx->evt_fb_fp) {
      Xfclose(ctx->evt_fb_fp);
      ctx->evt_fb_fp = NULL;
    }
    show_prompt(fp);
    return ;
  }

  if (ctx->evt_fb_fp) {
    fprintf(fp, "close old feedback.\n");
    Xfclose(ctx->evt_fb_fp);
  }

  hostname[0] = '\0';
  port = 0;
  sscanf(args, "%256s %i", hostname, &port);

  he = gethostbyname(hostname);
  if (he == NULL) {
    fprintf(fp, "Unknown hostname '%s'.\n", hostname);
    show_prompt(fp);
    return ;
  }

  memset(&srvaddr, 0, sizeof (struct sockaddr_in));
  memcpy(&srvaddr.sin_addr, he->h_addr, he->h_length);
  srvaddr.sin_family = he->h_addrtype;
  srvaddr.sin_port = htons(port);

  evt_fb_sd = Xsocket(PF_INET, SOCK_DGRAM, 0);
  Xconnect(evt_fb_sd, (struct sockaddr *) &srvaddr, sizeof (struct sockaddr));
  ctx->evt_fb_fp = Xfdopen(evt_fb_sd, "w");

  fprintf(fp, "Enable event feedback to '%s:%i'.\n", hostname, port);

  show_prompt(fp);
}


static int
remoteadm_client_callback(orchids_t *ctx, mod_entry_t *mod, int fd, void *data)
{
  int sz;
  char buf[RECV_BUF_SZ];
  char *args;
  FILE *fp;

  fp = (FILE *) data;

  DebugLog(DF_MOD, DS_TRACE, "Remote ADM client callback.\n");

  /* [XXX] Beware of connection reset by peer */
  sz = Xread(fd, buf, RECV_BUF_SZ);
  if (sz == 0) {
    DebugLog(DF_MOD, DS_DEBUG, "connexion closed...\n");
    del_input_descriptor(ctx, fd);
    Xfclose(fp);
    return (0);
  }

  /* bypass \r and \n if user connects with telnet or netcat */
  while (sz > 0 && (buf[sz-1] == '\r' || buf[sz-1] == '\n'))
    sz--;

  /* remove \r and/or \n */
  if (sz == RECV_BUF_SZ)
    buf[RECV_BUF_SZ - 1] = '\0';
  else
    buf[sz] = '\0';

  /* find args */
  args = buf;
  while ((*args != ' ') && (*args != '\0'))
    args++;
  if (*args == '\0')
    args = NULL;
  else
    *args++ = '\0';

  DebugLog(DF_MOD, DS_TRACE,
           "command: '%s' args: '%s'\n", buf, args ? args : "none");

  sz = 0;
  while (radm_cmd_g[sz].name && strcmp(radm_cmd_g[sz].name, buf))
    sz++;
  if (radm_cmd_g[sz].cmd) {
    radm_cmd_g[sz].cmd(fp, ctx, args);
  }
  else {
    fprintf(fp, "command '%s' not found... try 'help' or '?'...\n", buf);
    show_prompt(fp);
  }

  return (0);
}


static void *
radm_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  radmcfg_t *mod_cfg;

  DebugLog(DF_MOD, DS_INFO,
           "loading remoteadm module @ %p\n", (void *) &mod_remoteadm);

  mod_cfg = Xzmalloc(sizeof (radmcfg_t));
  mod_cfg->listen_port = DEFAULT_RADM_PORT;

  return (mod_cfg);
}


static void
radm_postconfig(orchids_t *ctx, mod_entry_t *mod)
{
  int fd;

  fd = create_tcplisten_socket();
  add_input_descriptor(ctx, mod, remoteadm_listen_callback, fd, NULL);
}


static void
set_listen_port(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  int port;

  port = atoi(dir->args);
  DebugLog(DF_MOD, DS_INFO, "setting tcp listen port on %i\n", port);

  if (port > 0)
    ((radmcfg_t *)mod->config)->listen_port = port;
}


static mod_cfg_cmd_t radm_dir[] = {
  { "ListenPort", set_listen_port, "Set listen port for remonte admin" },
  { NULL, NULL }
};


input_module_t mod_remoteadm = {
  MOD_MAGIC,
  ORCHIDS_VERSION,
  "remoteadm",
  "CeCILL2",
  NULL,
  radm_dir,
  radm_preconfig,
  radm_postconfig,
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
