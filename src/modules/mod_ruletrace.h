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

#ifndef MOD_RULETRACE_H
#define MOD_RULETRACE_H

#include <sys/types.h>

#include "orchids.h"

typedef struct ruletrace_ctx_s ruletrace_ctx_t;
struct ruletrace_ctx_s
{
  char *output_dir;
  char *file_prefix;
  int rule_limit;
  int state_limit;

  pid_t ruletracepid;
  int created_pid_dir;
};

#define MOD_RULETRACE_DEFAULT_PREFIX      "ruletrace-"
#define MOD_RULETRACE_DEFAULT_RULE_LIMIT  1000


static void
ruletrace_output_rule_instances(orchids_t *ctx,
                                ruletrace_ctx_t *ruletracectx,
                                event_t *event);

static FILE *
ruletrace_fopen_dot_file(orchids_t *ctx,
                         ruletrace_ctx_t *ruletracectx,
                         rule_instance_t *rule);

static void
ruletrace_output_rule_inst(orchids_t *ctx,
                           ruletrace_ctx_t *ruletracectx,
                           rule_instance_t *rule);
static void
ruletrace_output_create_dirs(orchids_t *ctx, ruletrace_ctx_t *ruletracectx);


static void
ruletrace_output_event(orchids_t *ctx,
                       ruletrace_ctx_t *ruletracectx,
                       event_t *event);


static FILE *
ruletrace_fopen_event_file(orchids_t *ctx, ruletrace_ctx_t *ruletracectx);


static int
ruletrace_hook(orchids_t *ctx, mod_entry_t *mod, void *data, event_t *event);


static void *
ruletrace_preconfig(orchids_t *ctx, mod_entry_t *mod);


static void
ruletrace_postconfig(orchids_t *ctx, mod_entry_t *mod);


static void
set_output_directory(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir);


static void
set_file_prefix(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir);


static void
set_rule_limit(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir);


static void
set_state_limit(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir);





#endif /* MOD_RULETRACE_H */
