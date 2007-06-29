/**
 ** @file orchids_cfg.c
 ** Functions for configuring Orchids core and modules.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 1.0
 ** @ingroup core
 **
 ** @date  Started on: Wed Jan 29 13:50:41 2003
 ** @date Last update: Fri Jun 29 11:31:35 2007
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
#include <limits.h> /* for PATH_MAX */

#include <sys/types.h> /* for stat() */
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/time.h> /* gettimeofday() */
#include <unistd.h>

#include <glob.h> /* glob() */

#include <errno.h>


#include "safelib.h"

#include "orchids.h"

#ifdef ORCHIDS_STATIC
/* declare builtins modules */
extern input_module_t mod_remoteadm;
extern input_module_t mod_textfile;
extern input_module_t mod_udp;
extern input_module_t mod_syslog;
extern input_module_t mod_snare;
extern input_module_t mod_netfilter;
extern input_module_t mod_rawsnare;
extern input_module_t mod_generic;
extern input_module_t mod_test;
extern input_module_t mod_test2;
extern input_module_t mod_cisco;
extern input_module_t mod_snmp;
#ifdef HAVE_NETSNMP
extern input_module_t mod_snmptrap;
#endif /* HAVE_NETSNMP */
extern input_module_t mod_sunbsm;
extern input_module_t mod_win32evt;
extern input_module_t mod_consoles;
extern input_module_t mod_autohtml;
extern input_module_t mod_sockunix;
#endif

static int
build_config_tree(const char *config_filepath, config_directive_t **root);
static void proceed_config_tree(orchids_t *ctx);
static int
build_config_tree_sub(FILE *fp,
                      config_directive_t **sect_root,
                      config_directive_t *parent,
                      const char *file,
                      int *lineno);
static int
split_line(char *line, char **directive, char **argument);
static void
cfg_get_next_token(const char *text, size_t *offset, size_t *length);
static void
config_load_module(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir);


void
proceed_pre_config(orchids_t *ctx)
{
#if 0
  input_module_t *mod_textfile;
  input_module_t *mod_syslog;
  dir_handler_t dir_handler;
  config_directive_t dir;
#endif
  struct timeval diff_time;

  DebugLog(DF_CORE, DS_NOTICE, "*** beginning ORCHIDS configuration ***\n");

  if (ctx->off_line_mode == MODE_ONLINE) {
    build_config_tree(ctx->config_file, &ctx->cfg_tree);
    proceed_config_tree(ctx);
  } else {
#if 0
    /* Deprecated */
    switch (ctx->off_line_mode) {

    case MODE_SYSLOG:
      mod_textfile = load_add_shared_module(ctx, "textfile");
      load_add_shared_module(ctx, "udp");
      mod_syslog = load_add_shared_module(ctx, "syslog");

      dir.args = "1";
      dir_handler = dir_handler_lookup(ctx, mod_textfile, "ProceedAll");
      dir_handler(ctx, &ctx->mods[0], &dir);

      dir.args = ctx->off_line_input_file;
      dir_handler = dir_handler_lookup(ctx, mod_textfile, "AddInputFile");
      dir_handler(ctx, &ctx->mods[0], &dir);

      dir_handler = dir_handler_lookup(ctx, mod_syslog, "AddTextfileSource");
      dir_handler(ctx, &ctx->mods[2], &dir);
      break;

    case MODE_SNARE:
      load_add_shared_module(ctx, "textfile");
      load_add_shared_module(ctx, "udp");
      load_add_shared_module(ctx, "snare");
      break;

    default:
      fprintf(stderr, "Offline mode '%i' not supported\n",
              ctx->off_line_mode);
      exit(EXIT_FAILURE);
      break;
    }
#endif
  }

  gettimeofday(&ctx->preconfig_time, NULL);
  Timer_Sub(&diff_time, &ctx->preconfig_time, &ctx->start_time);
  DebugLog(DF_CORE, DS_NOTICE, "pre-config (real) time: %li ms\n",
           (diff_time.tv_sec * 1000) + (diff_time.tv_usec) / 1000);
}

static int
build_config_tree(const char *config_filepath, config_directive_t **root)
{
  FILE *fp;
  int ret;
  int line;

  fp = Xfopen(config_filepath, "r");
  line = 0;
  ret = build_config_tree_sub(fp, root, NULL, config_filepath, &line);

  return (ret);
}

static config_directive_t *
get_last_dir(config_directive_t *list)
{
  if (list == NULL)
    return (NULL);

  for ( ; list->next ; list = list->next )
    ;

  return (list);
}

static int
proceed_includes(char *pattern, config_directive_t **root, config_directive_t **last)
{
  int ret;
  int i;
  glob_t globbuf;
  config_directive_t *new_root;

  DebugLog(DF_CORE, DS_DEBUG, "Include config file pattern '%s'\n", pattern);

  ret = glob(pattern, 0, NULL, &globbuf);
  if (ret) {
    if (ret == GLOB_NOMATCH)
      fprintf(stderr,
              "WARNING: Pattern returned no match.\n");
    else
      fprintf(stderr,
              "WARNING: glob() error.\n");
  }

  for (i = 0; i < globbuf.gl_pathc; i++) {
    DebugLog(DF_CORE, DS_DEBUG, "Include config file '%s'\n", globbuf.gl_pathv[i]);
    new_root = NULL;
    ret = build_config_tree(globbuf.gl_pathv[i], &new_root);
    if (ret)
      return (ret);
    if (new_root == NULL)
      continue ;
    if (*last)
      (*last)->next = new_root;
    else
      (*root) = new_root;
    *last = get_last_dir(new_root);
  }

  return (RETURN_SUCCESS);
}


static int
build_config_tree_sub(FILE *fp,
                      config_directive_t **sect_root,
                      config_directive_t *parent,
                      const char *file,
                      int *lineno)
{
  char line[LINE_MAX];
  char *directive;
  char *arguments;
  config_directive_t *new_dir = NULL;
  config_directive_t *last_dir = NULL;
  int skip;
  int ret;

  while (!feof(fp)) {
    line[0] = '\0';
    Xfgets(line, LINE_MAX, fp);
    (*lineno)++;

    skip = split_line(line, &directive, &arguments);

    /* skip blanks */
    if (skip)
      continue;

    /* handle section */
    if (directive[0] == '<') {
      if (directive[1] != '/') {
        /* open a section */
        new_dir = Xzmalloc(sizeof (config_directive_t));
        if (directive[ strlen(directive) - 1 ] == '>') {
          directive[ strlen(directive) - 1 ] = '\0';
          new_dir->directive = strdup(directive);
        }
        else {
          /* XXX warn if a section is empty ? */
          new_dir->directive = strdup(directive);
        }
        if (arguments)
          new_dir->args = strdup(arguments);
        new_dir->parent = parent;
        new_dir->file = file;
        new_dir->line = *lineno;

        /* Add the directive to the section */
        if (last_dir) /* add to current list */
          last_dir->next = new_dir;
        else /* special case for first element */
          *sect_root = new_dir;
        last_dir = new_dir;

        /* Read section resursively */
        ret = build_config_tree_sub(fp,
                                    &new_dir->first_child,
                                    new_dir,
                                    file,
                                    lineno);

        if (ret)
          return (ret);

        continue ;
      }
      else if (    parent
               && !strncmp((directive + 2),
                           parent->directive + 1,
                           strlen(directive) - 3)) {
        /* close a section */
        return (RETURN_SUCCESS);
      }
      else {
        return (ERR_CFG_SECT);
      }
    }

    if ( !strcmp("Include", directive) ) {
      proceed_includes(arguments, sect_root, &last_dir);
    }
    else {
      new_dir = Xzmalloc(sizeof (config_directive_t));
      new_dir->directive = strdup(directive);
      if (arguments)
        new_dir->args = strdup(arguments);
      new_dir->file = file;
      new_dir->line = *lineno;

      if (last_dir) /* add to current list */
        last_dir->next = new_dir;
      else /* special case for first element */
        *sect_root = new_dir;
      last_dir = new_dir;
    }
  }

  Xfclose(fp);

  if (parent)
    return (ERR_CFG_PEOF);

  return (RETURN_SUCCESS);
}

static void
strip_trailing_garbage(char *string)
{
  char *last_effective;
  int escaping;
  int in_quote;
  int c;

  in_quote = 0;
  escaping = FALSE;

  for ( last_effective = string; *string && *last_effective; string++ ) {

    c = *string;

    if (escaping) {
      escaping = FALSE;
      last_effective = string;
      continue ;
    }

    switch (c) {

    case '#':
      if ( !escaping && !in_quote) {
        *(last_effective+1) = '\0';
        return ;
      }
      last_effective = string;
      break ;

    case '\n':
      *(last_effective+1) = '\0';
      return ;

    case '\t':
    case ' ':
      break ;

    case '\\':
      last_effective = string;
      escaping = TRUE;
      break ;

    case '\'':
      last_effective = string;
      if (in_quote == '\'')
        in_quote = 0;
      else if (in_quote == 0)
        in_quote = '\'';
      break;

    case '"':
      last_effective = string;
      if (in_quote == '"')
        in_quote = 0;
      else if (in_quote == 0)
        in_quote = '\"';
      break;

    default:
      last_effective = string;
      break;
    }
  }
}


static int
split_line(char *line, char **directive, char **argument)
{
  size_t offset;
  size_t length;
  int c;
  char *ptr;

  cfg_get_next_token(line, &offset, &length);

  ptr = line + offset;
  c = *ptr;
  if (c == '#' || c == '\n' || c == '\0')
    return (CONFIG_IGNORE_LINE);

  *directive = ptr;
  ptr += length;

  offset = strspn(ptr, " \t\n");

  *ptr = '\0';
  ptr += offset;

  c = *ptr;
  if (c == '#' || c == '\n' || c == '\0') {
    *argument = NULL;
  }
  else {
    *argument = ptr;
    strip_trailing_garbage(ptr);
  }

  return (RETURN_SUCCESS);
}


static void
cfg_get_next_token(const char *text, size_t *offset, size_t *length)
{
  int i;
  int c;
  const char *token;

  i = strspn(text, " \t\n");
  token = &text[i];

  c = *token;
  *offset = i;
  if (c == '\0' || c == '\n') {
    *length = 0;
    return ;
  }

  i = strcspn(token, " \t\n");
  *length = i;
}


static void
fprintf_cfg_tree_sub(FILE *fp, config_directive_t *section, int depth)
{
  int i;

  for (; section; section = section->next)
    {
      for (i = 0; i < depth; i++)
        fprintf(fp, "    ");
      if (section->first_child)
        {
          if (section->args[0] == '\0')
            fprintf(fp, "Section %s>\n", section->directive);
          else
            fprintf(fp, "Section %s %s\n", section->directive, section->args);

          fprintf_cfg_tree_sub(fp, section->first_child, depth + 1);
        }
      else
        fprintf(fp, "[%s] [%s]\n", section->directive, section->args);
    }
  fprintf(fp, "\n");
}

void
fprintf_cfg_tree(FILE *fp, config_directive_t *root)
{
  fprintf(fp, "---[ configuration tree ]---\n");
  fprintf_cfg_tree_sub(fp, root, 0);
  fprintf(fp, "---[ end of configuration tree ]---\n\n");
}

static void
set_poll_period(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  ctx->poll_period.tv_sec = atoi(dir->args);

  /* keep a minimal (reasonable) value of 1 second */
  if (ctx->poll_period.tv_sec < 1) {
    DebugLog(DF_CORE, DS_WARN, "Warning, PollPeriod too small, set to 1\n");
    ctx->poll_period.tv_sec = 1;
  }
}

static void
add_rule_file(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  rulefile_t *rulefile;

  DebugLog(DF_CORE, DS_INFO, "adding rule file %s\n", dir->args);

  rulefile = Xmalloc(sizeof (rulefile_t));
  rulefile->name = dir->args;
  rulefile->next = NULL;

  /* if it is the first rulefile... */
  if (ctx->rulefile_list == NULL) {
    ctx->rulefile_list = rulefile;
    ctx->last_rulefile = rulefile;
  } else {
    ctx->last_rulefile->next = rulefile;
    ctx->last_rulefile = rulefile;
  }
}

static void
module_config(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  char mod_name[MODNAME_MAX];
  mod_entry_t *m;
  mod_cfg_cmd_t *c;
  config_directive_t *d;
  int i;

  i = sscanf(dir->args, "%32[a-zA-Z]>", mod_name);
  DebugLog(DF_CORE, DS_INFO, "Doing module configuration for %s\n", mod_name);
  m = find_module_entry(ctx, mod_name);
  if (!m) {
    DebugLog(DF_CORE, DS_WARN, "WARNING: module %s not loaded...\n", mod_name);
    return ;
  }
  c = m->mod->cfg_cmds;
  if (c == NULL) {
    DebugLog(DF_CORE, DS_INFO, "Module %s have no directive table.\n", m->mod->name);
    return ;
  }
  for (d = dir->first_child; d; d = d->next) {
    i = 0;
    while (c[i].name && strcmp(c[i].name, d->directive))
      i++;
    if (c[i].cmd) {
      c[i].cmd(ctx, m, d);
    }
    else {
      DebugLog(DF_CORE, DS_WARN,
               "no handler defined in module [%s] for [%s] directive\n",
               m->mod->name, d->directive);
    }
  }
}


#ifdef ORCHIDS_STATIC
static void
config_add_module(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  static input_module_t *builtin_mods[] = {
    &mod_textfile,
    &mod_udp,
    &mod_syslog,
    &mod_snare,
    &mod_remoteadm,
    &mod_rawsnare,
    &mod_netfilter,
    &mod_generic,
    &mod_cisco,
    &mod_snmp,
#ifdef HAVE_NETSNMP
    &mod_snmptrap,
#endif /* HAVE_NETSNMP */
    &mod_sunbsm,
    &mod_win32evt,
    &mod_consoles,
    &mod_autohtml,
    &mod_sockunix,
    NULL
  };
  int i;

  i = 0;
  while (builtin_mods[i] && strcmp(builtin_mods[i]->name, dir->args) )
    ++i;
  if (builtin_mods[i]) {
    add_module(ctx, builtin_mods[i]);
  } else {
    DebugLog(DF_CORE, DS_FATAL, "module %s not found.\n", dir->args);
    exit(EXIT_FAILURE);
  }
}
#endif /* ORCHIDS_STATIC */


static void
config_load_module(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  input_module_t *input_mod;

  input_mod = load_add_shared_module(ctx, dir->args);
  if (input_mod == NULL) {
    DebugLog(DF_CORE, DS_FATAL, "module %s not loaded.\n", dir->args);
    return ;
  }
}

/* XXX move this to mod_htmlstate */
static void
set_html_output_dir(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  struct stat stat_buf;

  DebugLog(DF_CORE, DS_INFO, "setting HTML output directory to '%s'\n", dir->args);

  ctx->html_output_dir = dir->args;

  /* check if the output directory exists */
  Xstat(ctx->html_output_dir, &stat_buf);
}


static void
set_idmef_dtd(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  DebugLog(DF_CORE, DS_INFO, "setting IMDEF-DTD file to '%s'\n", dir->args);

  ctx->idmef_dtd = dir->args;
}


static void
set_idmef_analyzer_id(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  DebugLog(DF_CORE, DS_INFO,
           "setting IMDEF analyzed identifier to '%s'\n", dir->args);

  ctx->idmef_analyzer_id = dir->args;
}


static void
set_idmef_analyzer_loc(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  DebugLog(DF_CORE, DS_INFO,
           "setting IMDEF analyzed location to '%s'\n", dir->args);

  ctx->idmef_analyzer_loc = dir->args;
}


static void
set_idmef_sensor_hostname(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  DebugLog(DF_CORE, DS_INFO,
           "setting IMDEF sensor hostname to '%s'\n", dir->args);

  ctx->idmef_sensor_hostname = dir->args;
}


static void
set_runtime_user(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  DebugLog(DF_CORE, DS_INFO,
           "setting RuntimeUser to '%s'\n", dir->args);

  ctx->runtime_user = dir->args;
}


static void
set_report_output_dir(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  struct stat stat_buf;

  DebugLog(DF_CORE, DS_INFO, "setting report output directory to '%s'\n",
           dir->args);

  ctx->report_dir = dir->args;

  /* check if the directory exists */
  Xstat(ctx->report_dir, &stat_buf);
}


static void
set_report_prefix(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  DebugLog(DF_CORE, DS_INFO, "setting report prefix to '%s'\n", dir->args);

  ctx->report_prefix = dir->args;
}


static void
set_report_ext(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  DebugLog(DF_CORE, DS_INFO, "setting report extension to '%s'\n", dir->args);

  ctx->report_ext = dir->args;
}


static void
set_preproc_cmd(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  DebugLog(DF_CORE, DS_INFO, "setting pre-processor command to '%s'\n", dir->args);

  ctx->preproc_cmd = dir->args;
}


static void
set_modules_dir(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  DebugLog(DF_CORE, DS_INFO, "setting modules directory to '%s'\n", dir->args);

  ctx->modules_dir = dir->args;
}

static void
set_lock_file(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  DebugLog(DF_CORE, DS_INFO, "setting lock file to '%s'\n", dir->args);

  ctx->lockfile = dir->args;
}

static void
set_max_memory_limit(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  int ret;
  int l;
  struct rlimit rl;

  DebugLog(DF_CORE, DS_INFO, "setting max memory limit '%s'\n", dir->args);

  l = atoi(dir->args);

  ret = getrlimit(RLIMIT_AS, &rl);
  if (ret) {
    DebugLog(DF_CORE, DS_ERROR,
             "getrlimit(): error %i: %s\n",
             errno, strerror(errno));
    return ;
  }

  DebugLog(DF_CORE, DS_INFO, "current memory limit is %i %i\n",
           rl.rlim_cur, rl.rlim_max);

  if (rl.rlim_max != RLIM_INFINITY && rl.rlim_max < l) {
    DebugLog(DF_CORE, DS_ERROR,
             "can't set memory limit: hard limit already set to %i "
             "(requesting %i)\n",
             rl.rlim_max, l);
    return ;
  }

  rl.rlim_cur = l;
  rl.rlim_max = l;
  ret = setrlimit(RLIMIT_AS, &rl);
  if (ret) {
    DebugLog(DF_CORE, DS_ERROR,
             "setrlimit(): error %i: %s\n",
             errno, strerror(errno));
    return ;
  }
}


static mod_cfg_cmd_t config_dir_g[] = 
{
  { "PollPeriod", set_poll_period, "Set Poll Period (en Second)" },
#ifdef ORCHIDS_STATIC
  { "AddModule" , config_add_module, "Activate a (built-in) module"},
#endif /* ORCHIDS_STATIC */
  { "LoadModule" , config_load_module, "Load a shared object module"},
  { "<module"   , module_config  , "Configuration section for a module" },
  { "AddRuleFile", add_rule_file , "Add a rule file" },
  { "HTMLOutputDir", set_html_output_dir , "Set the HTML output directory" },
  { "IDMEFdtd", set_idmef_dtd, "Set the IDMEF dtd"},
  { "IDMEFAnalyzerId", set_idmef_analyzer_id, "Set the IDMEF analyzer identifier"},
  { "IDMEFAnalyzerLocation", set_idmef_analyzer_loc, "Set the IDMEF analyzer location"},
  { "IDMEFSensorHostname", set_idmef_sensor_hostname, "Set the IDMEF sensor hostname"},
  { "RuntimeUser", set_runtime_user, "Set the runtime user." },
  { "ReportDir", set_report_output_dir, "Set the outpout directory" },
  { "ReportPrefix", set_report_prefix, "Set the report prefix" },
  { "ReportExt", set_report_ext, "Set the report extension" },
  { "SetPreprocessorCmd", set_preproc_cmd, "Set the preprocessor command" },
  { "SetModuleDir", set_modules_dir, "Set the modules directory" },
  { "SetLockFile", set_lock_file, "Set the lock file name" },
  { "MaxMemorySize", set_max_memory_limit, "Set maximum memory limit" },
  { NULL, NULL, NULL }
};


static void
proceed_config_tree(orchids_t *ctx)
{
  config_directive_t *d;
  int i;

  DebugLog(DF_CORE, DS_INFO, "*** pre-config (tree interpretation) ***\n");

  for (d = ctx->cfg_tree; d; d = d->next) {
    i = 0;
    while (config_dir_g[i].name && strcmp(config_dir_g[i].name, d->directive))
      i++;
    if (config_dir_g[i].cmd) {
      config_dir_g[i].cmd(ctx, NULL, d);
    }
    else {
      DebugLog(DF_CORE, DS_WARN,
               "No handler defined for [%s] direcrive\n", d->directive);
    }
  }
}


/* ------------------- TEST ----------------------*/

static void
fprintf_cfg_mib_sub(FILE *f, config_directive_t *section, char *base)
{
  char *newbase;

  for (; section; section = section->next) {
    if (section->first_child) {
      /* if (section->args[0] == '\0')
         fprintf(f, "Section %s>\n", section->directive);
         else
         fprintf(f, "Section %s %s\n", section->directive, section->args); */
      if (base) {
        /* 3 = ">." + '\0' */
        newbase = Xmalloc(strlen(base) + strlen(section->directive) + 3);
        strcpy(newbase, base);
        strcat(newbase, ">.");
        strcat(newbase, section->directive);
      }
      else
        newbase = strdup(section->directive);

      fprintf_cfg_mib_sub(f, section->first_child, newbase);

      /* free(newbase); */
    }
    else {
      if (base)
        fprintf(f, "%s.%s %s\n", base, section->directive, section->args);
      else
        fprintf(f, "%s %s\n", section->directive, section->args);
    }
  }
}

void
fprintf_cfg_mib(FILE *f, config_directive_t *section)
{
  fprintf(f, "---[ config mib test ]---\n");
  fprintf_cfg_mib_sub(f, section, NULL);
  fprintf(f, "---[ end of config mib test ]---\n\n");
}


void
fprintf_directives(FILE *fp, orchids_t *ctx)
{
  int mod;
  mod_cfg_cmd_t *d;

  fprintf(fp, "---[ known directives ]---\n");
  fprintf(fp, "core directives :\n");
  fprintf(fp, "---------------------------------------------\n");

  /* first, built-in directives */
  for (d = config_dir_g; (*d).cmd; d++)
    fprintf(fp, "%20s | %.32s\n", (*d).name, (*d).help);

  for (mod = 0; mod < ctx->loaded_modules; mod++) {
    fprintf(fp, "\nmod %s directives :\n", ctx->mods[mod].mod->name);
    fprintf(fp, "---------------------------------------------\n");
    if (ctx->mods[mod].mod->cfg_cmds) {
      for (d = ctx->mods[mod].mod->cfg_cmds; d->name; d++)
        fprintf(fp, "%20s | %.32s\n", d->name, d->help);
    }
    else
      fprintf(fp, "no directive.\n");
  }
}


void
proceed_post_config(orchids_t *ctx)
{
  int mod_id;
  input_module_t *mod;
  struct timeval diff_time;

  DebugLog(DF_CORE, DS_NOTICE, "*** proceeding to post configuration... ***\n");

  for (mod_id = 0; mod_id < ctx->loaded_modules; mod_id++) {
    mod = ctx->mods[mod_id].mod;
    if (mod->post_config)
      mod->post_config(ctx, &ctx->mods[mod_id]);
  }

  gettimeofday(&ctx->postconfig_time, NULL);
  Timer_Sub(&diff_time, &ctx->postconfig_time, &ctx->preconfig_time);
  /* move this into orchids stats */
  DebugLog(DF_CORE, DS_NOTICE, "post-config (real) time: %li.%03li ms\n",
           (diff_time.tv_sec) * 1000 + (diff_time.tv_usec) / 1000,
           (diff_time.tv_usec) % 1000);
}


void
proceed_post_compil(orchids_t *ctx)
{
  int mod_id;
  input_module_t *mod;
  struct timeval diff_time;

  DebugLog(DF_CORE, DS_NOTICE, "*** proceeding to post rule compilation configuration... ***\n");

  for (mod_id = 0; mod_id < ctx->loaded_modules; mod_id++) {
    mod = ctx->mods[mod_id].mod;
    if (mod->post_compil)
      mod->post_compil(ctx, &ctx->mods[mod_id]);
  }

  gettimeofday(&ctx->postcompil_time, NULL);
  Timer_Sub(&diff_time, &ctx->postcompil_time, &ctx->compil_time);
  /* move this into orhcids stats */
  DebugLog(DF_CORE, DS_NOTICE, "post-compilation (real) time: %li.%03li ms\n",
           (diff_time.tv_sec) * 1000 +
           (diff_time.tv_usec) / 1000,
           (diff_time.tv_usec) % 1000);
}



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
