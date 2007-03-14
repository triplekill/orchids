/**
 ** @file orchids_cfg.c
 ** Functions for configuring Orchids core and modules.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup core
 **
 ** @date  Started on: Wed Jan 29 13:50:41 2003
 ** @date Last update: Tue Dec  6 15:37:04 2005
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif


/* XXX: Ugly conf tree builder !... Have to rewrite it !... */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h> /* for PATH_MAX */

#include <sys/types.h> /* for stat() */
#include <sys/stat.h>
#include <unistd.h>

#include <sys/time.h> /* gettimeofday() */


#include <errno.h>


#include "safelib.h"

#include "orchids.h"

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

static config_directive_t *
build_config_section(FILE *fp, config_directive_t *parent);
static void build_config_tree(orchids_t *ctx);
static void fprintf_cfg_tree_sub(FILE *fp, config_directive_t *section,
                                 int depth);
static void proceed_config_tree(orchids_t *ctx);
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
    build_config_tree(ctx);
    proceed_config_tree(ctx);
  } else {
#if 0
    /* Deprecated */
    switch (ctx->off_line_mode) {

    case MODE_SYSLOG:
      mod_textfile = load_add_module(ctx, "textfile");
      load_add_module(ctx, "udp");
      mod_syslog = load_add_module(ctx, "syslog");

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
      load_add_module(ctx, "textfile");
      load_add_module(ctx, "udp");
      load_add_module(ctx, "snare");
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

static void
build_config_tree(orchids_t *ctx)
{
  FILE *fp;
  int i;
  char cfg_file_path[PATH_MAX];
  char line[2048];
  char op[64];
  char param[2014];
  config_directive_t *new_dir = NULL;
  config_directive_t *last_dir = NULL;

  Xrealpath(ctx->config_file, cfg_file_path);

  DebugLog(DF_CORE, DS_NOTICE,
           "builing config tree from cfg file [%s]\n", cfg_file_path);

  fp = Xfopen(cfg_file_path, "r");
  while (!feof(fp))
    {
      line[0] = '\0';
      Xfgets(line, 2048, fp);
      op[0] = param[0] = '\0';
      i = sscanf(line, "%64s %2014[^\n]", op, param);

      /* skip blanks */
      if ((op[0] == '#') || (op[0] == '\0'))
        continue;

      /* handle section */
      if (op[0] == '<')
        {
          if (op[1] != '/')
            {
              DebugLog(DF_CORE, DS_TRACE, "Begin Section %s\n", op);
              new_dir = Xzmalloc(sizeof (config_directive_t));
              if (op[strlen(op)-1] == '>')
                {
                  op[ strlen(op) - 1 ] = '\0';
                  new_dir->directive = strdup(op);
                }
              else
                new_dir->directive = strdup(op);
              new_dir->args = strdup(param);
              /* Read section resursively */
              new_dir->first_child = build_config_section(fp, new_dir);
              /* Add directive */
              if (last_dir) /* add to current list */
                last_dir->next = new_dir;
              else /* special case for first element */
                ctx->cfg_tree = new_dir;
              last_dir = new_dir;
              continue ;
            }
          else
            {
              DebugLog(DF_CORE, DS_FATAL, "Closing section in globals ?.\n");
              exit(EXIT_FAILURE);
            }
        }
      
      DebugLog(DF_CORE, DS_DEBUG, "Directive '%s' Parameter '%s'\n", op, param);
      new_dir = Xzmalloc(sizeof (config_directive_t));
      new_dir->directive = strdup(op);
      new_dir->args = strdup(param);
      /* Add directive */
      if (last_dir) /* add to current list */
        last_dir->next = new_dir;
      else /* special case for first element */
        ctx->cfg_tree = new_dir;
      last_dir = new_dir;
    }

  Xfclose(fp);
}

static config_directive_t *
build_config_section(FILE *fp, config_directive_t *parent)
{
  config_directive_t *new_dir;
  config_directive_t *section_root;
  config_directive_t *last_dir; /* current directive */
  int i;
  char line[2048];
  char op[64];
  char param[2014];

  last_dir = new_dir = section_root = NULL;

  while (!feof(fp))
    {
      line[0] = '\0';
      Xfgets(line, 2048, fp);
      op[0] = param[0] = '\0';
      i = sscanf(line, "%64s %2014[^\n]", op, param);

      /* skip comment and blank lines */
      if ((op[0] == '#') || (op[0] == '\0'))
        continue;

      /* handle section */
      if (op[0] == '<')
        {
          if (op[1] != '/')
            {
              DebugLog(DF_CORE, DS_TRACE, "Begin Section %s\n", op);
              new_dir = Xzmalloc(sizeof (config_directive_t));
              if (op[strlen(op)-1] == '>')
                {
                  op[ strlen(op) - 1 ] = '\0';
                  new_dir->directive = strdup(op);
                }
              else
                new_dir->directive = strdup(op);
              new_dir->args = strdup(param);
              /* Read section resursively */
              new_dir->first_child = build_config_section(fp, new_dir);
              new_dir->parent = parent;
              /* Add directive */
              if (last_dir) /* add to current list */
                last_dir->next = new_dir;
              else /* special case for first element */
                section_root = new_dir;
              last_dir = new_dir;
              continue ;
            }
          else /* handle closing section this time... */
            {
              /* check if we are closing right section */
              if (!strncmp((op + 2), parent->directive + 1, strlen(op) - 3))
                {
                  /* XXX - check if section was empty before returning */
                  return (section_root);
                }
              else
                {
                  DebugLog(DF_CORE, DS_FATAL, "Bad closing... [%s] -> [%s]\n",
                           parent->directive, op);
                  exit(EXIT_FAILURE);
                }
            }
        }

      DebugLog(DF_CORE, DS_TRACE, "Directive '%s' Parameter '%s'\n", op, param);
      new_dir = Xzmalloc(sizeof (config_directive_t));
      new_dir->directive = strdup(op);
      new_dir->args = strdup(param);
      /* Add directive */
      if (last_dir) /* add to current list */
        last_dir->next = new_dir;
      else /* special case for first element */
        section_root = new_dir;
      last_dir = new_dir;
    }

  /* XXX - keep this ???*/
  DebugLog(DF_CORE, DS_FATAL, "Error... Missing closing section before EOF...\n");
  exit(EXIT_FAILURE);

  return (NULL);
}

void
fprintf_cfg_tree(FILE *fp, config_directive_t *root)
{
  fprintf(fp, "---[ configuration tree ]---\n");
  fprintf_cfg_tree_sub(fp, root, 0);
  fprintf(fp, "---[ end of configuration tree ]---\n\n");
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
  /*  char filepath[PATH_MAX]; */
  rulefile_t *rulefile;

  DebugLog(DF_CORE, DS_INFO, "adding rule file %s\n", dir->args);
/*   compile_and_add_rulefile(ctx, dir->args); */

  rulefile = Xmalloc(sizeof (rulefile_t));
/*
  Xrealpath(dir->args, filepath);
  rulefile->name = strdup(filepath);
*/
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
    } else {
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

  input_mod = load_shared_module(ctx, dir->args);
  if (input_mod == NULL) {
    DebugLog(DF_CORE, DS_FATAL, "module %s not loaded.\n", dir->args);
    return ;
/*     exit(EXIT_FAILURE); */
  }

  add_module(ctx, input_mod);
}


static void
set_html_output_dir(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  struct stat stat_buf;

  DebugLog(DF_CORE, DS_INFO, "setting HTML output directory to '%s'\n", dir->args);

  ctx->html_output_dir = dir->args;

  /* check if the output directory exists */
  Xstat(ctx->html_output_dir, &stat_buf);
/*   if (ret < 0) { */
/*     if (errno == ENOENT) { */
/*       /\* create output dir *\/ */
/*       DebugLog(DF_CORE, DS_INFO, "Creating output directory '%s'.\n", */
/*                ctx->html_output_dir); */
/*       mkdir(ctx->html_output_dir, 493); */
/*     } */
/*     else { */
/*       fprintf(stderr, "%s:%i:stat(%s): errno=%i: %s\n", */
/*               __FILE__, __LINE__, ctx->html_output_dir, */
/*               errno, strerror(errno)); */
/*       exit(EXIT_FAILURE); */
/*     } */
/*   } */
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
    } else {
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

  for (; section; section = section->next)
    {
      if (section->first_child)
        {
          /*          if (section->args[0] == '\0')
            fprintf(f, "Section %s>\n", section->directive);
          else
          fprintf(f, "Section %s %s\n", section->directive, section->args);*/
          if (base)
            {
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
      else
        {
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

  for (mod = 0; mod < ctx->loaded_modules; mod++)
    {
      fprintf(fp, "\nmod %s directives :\n", ctx->mods[mod].mod->name);
      fprintf(fp, "---------------------------------------------\n");
      if (ctx->mods[mod].mod->cfg_cmds)
        {
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
  /* move this into orhcids stats */
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
