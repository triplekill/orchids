/**
 ** @file mod_ruletrace.c
 ** Rule tracer module.
 ** 
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 ** 
 ** @version 0.1.0
 ** 
 ** @date  Started on: Thu Jul  5 13:10:33 2007
 **/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <string.h>
#include <errno.h>

#include "orchids.h"

#include "safelib.h"
#include "strhash.h"
#include "graph_output.h"
#include "orchids_api.h"

#include "mod_ruletrace.h"

/*
  File naming scheme for .dot files for rule instances:
  $OUTPUT_DIR/$D1/$D2/$D3/$D4-$NTPTSH-$NTPTSL/$FILEPREFIX$NTPTSH-$NTPTSL-$RNAME.dot

  File naming scheme for event files:
  $OUTPUT_DIR/$D1/$D2/$D3/$D4-$NTPTSH-$NTPTSL/event
 */


static int
ruletrace_hook(orchids_t *ctx, mod_entry_t *mod, void *data, event_t *event)
{
  ruletrace_ctx_t *ruletracectx;

  DebugLog(DF_MOD, DS_DEBUG, "Executing ruletrace_hook.\n");

  ruletracectx = (ruletrace_ctx_t *)mod->config;

  ruletrace_output_rule_instances(ctx, ruletracectx, event);

  return (0);
}


static void
ruletrace_output_rule_instances(orchids_t *ctx,
                                ruletrace_ctx_t *ruletracectx,
                                event_t *event)
{
  rule_instance_t *r;
  int rilim;
  int i;

  i = 0;
  rilim = ruletracectx->rule_limit;
  for (r = ctx->first_rule_instance; r && i < rilim; r = r->next) {
    if (timercmp(&r->new_last_act, &ctx->cur_loop_time, ==)) {
      if (i == 0) {
        ruletrace_output_create_dirs(ctx, ruletracectx);
        ruletrace_output_event(ctx, ruletracectx, event);
      }
      ruletrace_output_rule_inst(ctx, ruletracectx, r);
      i++;
    }
    else {
      DebugLog(DF_MOD, DS_DEBUG,
               "Skipping rule: "
               "cur_loop_time=%li.%06li new_last_act=%li.%06li\n",
               ctx->cur_loop_time.tv_sec, ctx->cur_loop_time.tv_usec,
               r->new_last_act.tv_sec, r->new_last_act.tv_usec);
    }
  }
}


static void
ruletrace_output_create_dirs(orchids_t *ctx, ruletrace_ctx_t *ruletracectx)
{
  uint32_t evtid;
  int d1, d2, d3, d4;
  char pathname[PATH_MAX];
  uint32_t ntp1h, ntp1l;

  evtid = ctx->events;
  d4 = evtid & 0xFF;
  evtid >>= 8;
  d3 = evtid & 0xFF;
  evtid >>= 8;
  d2 = evtid & 0xFF;
  evtid >>= 8;
  d1 = evtid & 0xFF;
  Timer_to_NTP(&ctx->cur_loop_time, ntp1h, ntp1l);

  if ( !ruletracectx->created_pid_dir ) {
    snprintf(pathname, sizeof (pathname),
             "%s/%05i",
             ruletracectx->output_dir,
             ruletracectx->ruletracepid);
    mkdir(pathname, 0777);
    ruletracectx->created_pid_dir = TRUE;
  }

  snprintf(pathname, sizeof (pathname),
           "%s/%05i/%02x",
           ruletracectx->output_dir,
           ruletracectx->ruletracepid,
           d1);

  mkdir(pathname, 0777);

  snprintf(pathname, sizeof (pathname),
           "%s/%05i/%02x/%02x",
           ruletracectx->output_dir,
           ruletracectx->ruletracepid,
           d1, d2);

  mkdir(pathname, 0777);

  snprintf(pathname, sizeof (pathname),
           "%s/%05i/%02x/%02x/%02x",
           ruletracectx->output_dir,
           ruletracectx->ruletracepid,
           d1, d2, d3);

  mkdir(pathname, 0777);

  snprintf(pathname, sizeof (pathname),
           "%s/%05i/%02x/%02x/%02x/%02x-%08x-%08x",
           ruletracectx->output_dir,
           ruletracectx->ruletracepid,
           d1, d2, d3, d4,
           ntp1h, ntp1l);

  mkdir(pathname, 0777);

  DebugLog(DF_MOD, DS_INFO, "Created directory: '%s'\n", pathname);
}


static void
ruletrace_output_event(orchids_t *ctx,
                       ruletrace_ctx_t *ruletracectx,
                       event_t *event)
{
  FILE *fp;

  fp = ruletrace_fopen_event_file(ctx, ruletracectx);
  if (fp == NULL)
    return ;

  fprintf_event(fp, ctx, event);

  fclose(fp);
}


static FILE *
ruletrace_fopen_event_file(orchids_t *ctx, ruletrace_ctx_t *ruletracectx)
{
  FILE *fp;
  uint32_t evtid;
  int d1, d2, d3, d4;
  char pathname[PATH_MAX];
  uint32_t ntp1h, ntp1l;

  evtid = ctx->events;
  d4 = evtid & 0xFF;
  evtid >>= 8;
  d3 = evtid & 0xFF;
  evtid >>= 8;
  d2 = evtid & 0xFF;
  evtid >>= 8;
  d1 = evtid & 0xFF;
  Timer_to_NTP(&ctx->cur_loop_time, ntp1h, ntp1l);

  snprintf(pathname, sizeof (pathname),
           "%s/%05i/%02x/%02x/%02x/%02x-%08x-%08x/event",
           ruletracectx->output_dir,
           ruletracectx->ruletracepid,
           d1, d2, d3, d4,
           ntp1h, ntp1l);

  fp = fopen(pathname, "w");
  if (fp == NULL) {
    DebugLog(DF_MOD, DS_INFO,
             "Error while opening %s: errno %i: %s\n",
             pathname, errno, strerror(errno));
  }

  return (fp);
}


static FILE *
ruletrace_fopen_dot_file(orchids_t *ctx,
                         ruletrace_ctx_t *ruletracectx,
                         rule_instance_t *rule)
{
  FILE *fp;
  uint32_t evtid;
  int d1, d2, d3, d4;
  char pathname[PATH_MAX];
  uint32_t ntp1h, ntp1l;
  uint32_t ntp2h, ntp2l;

  evtid = ctx->events;
  d4 = evtid & 0xFF;
  evtid >>= 8;
  d3 = evtid & 0xFF;
  evtid >>= 8;
  d2 = evtid & 0xFF;
  evtid >>= 8;
  d1 = evtid & 0xFF;
  Timer_to_NTP(&rule->new_last_act, ntp1h, ntp1l);
  Timer_to_NTP(&rule->new_creation_date, ntp2h, ntp2l);

  snprintf(pathname, sizeof (pathname),
           "%s/%05i/%02x/%02x/%02x/%02x-%08x-%08x/%s%08x-%08x-%s.dot",
           ruletracectx->output_dir,
           ruletracectx->ruletracepid,
           d1, d2, d3, d4,
           ntp1h, ntp1l,
           ruletracectx->file_prefix ? ruletracectx->file_prefix : "",
           ntp2h, ntp2l,
           rule->rule->name);

  DebugLog(DF_MOD, DS_INFO, "dot pathname: %s\n", pathname);

  fp = fopen(pathname, "w");
  if (fp == NULL) {
    DebugLog(DF_MOD, DS_INFO,
             "Error while opening %s: errno %i: %s\n",
             pathname, errno, strerror(errno));
  }

  return (fp);
}


static void
ruletrace_output_rule_inst(orchids_t *ctx,
                           ruletrace_ctx_t *ruletracectx,
                           rule_instance_t *rule)
{
  FILE *fp;

  fp = ruletrace_fopen_dot_file(ctx, ruletracectx, rule);
  if (fp == NULL) {
    return ;
  }

  fprintf_rule_instance_dot(fp, rule, DOT_RETRIGLIST,
                            ctx->new_qh, ruletracectx->state_limit);

  fclose(fp);
}


static void *
ruletrace_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  ruletrace_ctx_t *modcfg;

  DebugLog(DF_MOD, DS_TRACE, "ruletrace_preconfig()\n");

  modcfg = Xzmalloc(sizeof (ruletrace_ctx_t));
  modcfg->file_prefix = MOD_RULETRACE_DEFAULT_PREFIX;
  modcfg->rule_limit = MOD_RULETRACE_DEFAULT_RULE_LIMIT;
  modcfg->ruletracepid = getpid();

  return (modcfg);
}


static void
ruletrace_postconfig(orchids_t *ctx, mod_entry_t *mod)
{
  ruletrace_ctx_t *ruletracectx;

  DebugLog(DF_MOD, DS_TRACE, "ruletrace_postconfig()\n");

  ruletracectx = (ruletrace_ctx_t *)mod->config;

  if (ruletracectx->output_dir == NULL) {
    DebugLog(DF_MOD, DS_ERROR, "Output directory is not set.\n");
    return ;
  }

  register_post_inject_hook(ctx, mod, ruletrace_hook, NULL);
}


static void
set_output_directory(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  ruletrace_ctx_t *ruletracectx;

  DebugLog(DF_MOD, DS_INFO, "Setting output dir to '%s'\n", dir->args);

  ruletracectx = (ruletrace_ctx_t *)mod->config;

  ruletracectx->output_dir = dir->args;
}


static void
set_file_prefix(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  ruletrace_ctx_t *ruletracectx;

  DebugLog(DF_MOD, DS_INFO, "Setting file prefix to '%s'\n", dir->args);

  ruletracectx = (ruletrace_ctx_t *)mod->config;

  ruletracectx->file_prefix = dir->args;
}


static void
set_rule_limit(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  ruletrace_ctx_t *ruletracectx;

  DebugLog(DF_MOD, DS_INFO, "Setting rule instance limit to '%s'\n", dir->args);

  ruletracectx = (ruletrace_ctx_t *)mod->config;

  ruletracectx->rule_limit = atoi(dir->args);
}


static void
set_state_limit(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  ruletrace_ctx_t *ruletracectx;

  DebugLog(DF_MOD, DS_INFO, "Setting state instance limit to '%s'\n", dir->args);

  ruletracectx = (ruletrace_ctx_t *)mod->config;

  ruletracectx->state_limit = atoi(dir->args);
}


static mod_cfg_cmd_t ruletrace_config_commands[] = 
{
  { "OutputDirectory", set_output_directory, "Set the output directory" },
  { "FilePrefix", set_file_prefix, "Set the output file prefix" },
  { "RuleLimit", set_rule_limit, "Set the rule instance limit per event" },
  { "StateLimit", set_state_limit, "Set the state instance limit per rule instance" },
  { NULL, NULL, NULL }
};

input_module_t mod_ruletrace = {
  MOD_MAGIC,
  ORCHIDS_VERSION,
  "ruletrace",
  "CeCILL2",
  NULL,
  ruletrace_config_commands,
  ruletrace_preconfig,
  ruletrace_postconfig,
  NULL
};

/* End-of-file */
