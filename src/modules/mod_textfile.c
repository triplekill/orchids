/**
 ** @file mod_textfile.c
 ** textfile input module, for text log files.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 ** @date  Started on: Wed Jan 15 17:07:26 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>

#include "orchids.h"

#include "evt_mgr.h"
#include "orchids_api.h"

#include "mod_textfile.h"


input_module_t mod_textfile;


static void
textfile_buildevent(orchids_t *ctx, mod_entry_t *mod, textfile_t *tf, char *buf)
{
  ovm_var_t *attr[TF_FIELDS];
  event_t *event;

  DebugLog(DF_MOD, DS_TRACE, "Dissecting: %s", buf);

  memset(attr, 0, sizeof(attr));

  attr[F_LINE_NUM] = ovm_int_new();
  attr[F_LINE_NUM]->flags |= TYPE_MONO;
  INT(attr[F_LINE_NUM]) = tf->line;

  attr[F_FILE] = ovm_vstr_new();
  VSTR(attr[F_FILE]) = tf->filename;
  VSTRLEN(attr[F_FILE]) = strlen(tf->filename);

  attr[F_LINE] = ovm_str_new(strlen(buf) - 1);
  memcpy(STR(attr[F_LINE]), buf, STRLEN(attr[F_LINE]));

  event = NULL;
  add_fields_to_event(ctx, mod, &event, attr, TF_FIELDS);

  post_event(ctx, mod, event);
}


static int
textfile_callback(orchids_t *ctx, mod_entry_t *mod, void *dummy)
{
  textfile_config_t *cfg;
  textfile_t *tf;
  struct stat st;
  char buff[1024];

  DebugLog(DF_MOD, DS_TRACE, "textfile_callback();\n");

  cfg = (textfile_config_t *)mod->config;
  for (tf = cfg->file_list; tf; tf = tf->next) {
    /* Checks if mtime has changed for each file... */
    Xfstat(fileno(tf->fd), &st);
    if (tf->file_stat.st_size > st.st_size) {
      DebugLog(DF_MOD, DS_NOTICE,
               "File [%s] has been truncated (%lu->%lu) rewind()ing\n",
                tf->filename, tf->file_stat.st_size, st.st_size);
      rewind(tf->fd);
    }
    if (st.st_mtime > tf->file_stat.st_mtime) {
      DebugLog(DF_MOD, DS_DEBUG, "mtime updated for [%s]\n", tf->filename);
      /* Update file stat */
      tf->file_stat = st;
      /* read and proceed new lines */
      while ( Xfgets(buff, 1024, tf->fd) != NULL ) {
        /* update line # */
        tf->line++;
        /* update crc */
        *(unsigned int *)&tf->hash =
          crc32( *(unsigned int *)&tf->hash, buff, strlen(buff));
        DebugLog(DF_MOD, DS_DEBUG,
                 "read line %i crc(0x%08X) %s",
                 tf->line, *(unsigned int *)&tf->hash, buff);

        textfile_buildevent(ctx, mod, tf, buff);
      }
    }
#ifdef ORCHIDS_DEBUG
    else if (st.st_mtime < tf->file_stat.st_mtime) {
      DebugLog(DF_MOD, DS_DEBUG, "mtime older for [%s] ?.\n", tf->filename);
    } else if (st.st_mtime == tf->file_stat.st_mtime) {
      DebugLog(DF_MOD, DS_DEBUG, "no changes for [%s]\n", tf->filename);
    }
#endif
  }

  return (0);
}

static field_t tf_fields[] = {
  { "textfile.line_num", T_INT,  "line number"                },
  { "textfile.file",     T_VSTR, "source filename"            },
  { "textfile.line",     T_STR,  "a line of a given textfile" }
};


static void *
textfile_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  textfile_config_t *mod_cfg;

  DebugLog(DF_MOD, DS_INFO, "load() textfile@%p\n", (void *) &mod_textfile);

  mod_cfg = Xzmalloc(sizeof (textfile_config_t));
  add_polled_input_callback(ctx, mod, textfile_callback, NULL);

  register_fields(ctx, mod, tf_fields, TF_FIELDS);

  return (mod_cfg);
}


static void
textfile_postconfig(orchids_t *ctx, mod_entry_t *mod)
{
  textfile_config_t *cfg;

  DebugLog(DF_MOD, DS_TRACE,
           "textfile_postconfig() -- registering file polling if needed...\n");

  cfg = (textfile_config_t *)mod->config;

  /* register real-time action for file polling, if requested. */
  if (cfg->poll_period > 0) {
    DebugLog(DF_MOD, DS_INFO, "Activating file polling\n");
    register_rtcallback(ctx,
			rtaction_read_files,
			mod,
			INITIAL_MODTEXT_POLL_DEFAY);
  }
}


static void
textfile_postcompil(orchids_t *ctx, mod_entry_t *mod)
{
  textfile_config_t *cfg;
  textfile_t *tf;
  char buff[1024];

  DebugLog(DF_MOD, DS_TRACE,
           "textfile_postcompil() -- reading initial file content...\n");

  cfg = (textfile_config_t *)mod->config;

  /* if we don't have to compute hashes, just fseek to the end-of-file */
  for (tf = cfg->file_list; tf; tf = tf->next) {
    DebugLog(DF_MOD, DS_DEBUG, "proceed file : %s\n", tf->filename);
    while ( Xfgets(buff, 1024, tf->fd) != NULL ) {
      tf->line++;
      *(unsigned int *)&tf->hash = 
        crc32( *(unsigned int *)&tf->hash, buff, strlen(buff));
      DebugLog(DF_MOD, DS_DEBUG,
               "read line %i crc(0x%08X) %s",
               tf->line, *(unsigned int *)&tf->hash, buff);
      if (cfg->proceed_all_data)
        textfile_buildevent(ctx, mod, tf, buff);
    }
  }
}


static void
add_input_file(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  char filepath[PATH_MAX];
  textfile_config_t *cfg;
  textfile_t *f;

  cfg = (textfile_config_t *)mod->config;
  f = Xzmalloc(sizeof (textfile_t));
  Xrealpath(dir->args, filepath);
  f->filename = strdup(filepath);
  f->fd = Xfopen(f->filename, "r");
  Xfstat(fileno(f->fd), &f->file_stat);

  DebugLog(DF_MOD, DS_INFO, "adding file '%s' to poll inputs (fp=%p fd=%i).\n",
           f->filename, f->fd, fileno(f->fd));

  /* if this file is the first added and poll period is not set,
     activate the file polling with the default poll period */
  if (cfg->file_list == NULL && cfg->poll_period == 0) {
    DebugLog(DF_MOD, DS_INFO,
	     "Automatically activate file polling "
	     "with PollPeriod=%i\n",
	     DEFAULT_MODTEXT_POLL_PERIOD);
    cfg->poll_period = DEFAULT_MODTEXT_POLL_PERIOD;
  }

  /* add to file list */
  f->next = cfg->file_list;
  cfg->file_list = f;
}


static void
set_proceed_all(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  int flag;

  DebugLog(DF_MOD, DS_INFO, "setting ProoceedAll to %s\n", dir->args);

  flag = atoi(dir->args);
  if (flag)
    ((textfile_config_t *)mod->config)->proceed_all_data = 1;
}


static void
set_poll_period(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  int value;

  DebugLog(DF_MOD, DS_INFO, "setting PollPeriod to %s\n", dir->args);

  value = atoi(dir->args);
  if (value < 0) {
    value = 0;
  }

  ((textfile_config_t *)mod->config)->poll_period = value;
}


static int
rtaction_read_files(orchids_t *ctx, rtaction_t *e)
{
  mod_entry_t *mod;
  textfile_config_t *cfg;

  DebugLog(DF_MOD, DS_TRACE,
           "Real-time action: Checking files...\n");

  mod = (mod_entry_t *)(e->data);
  cfg = (textfile_config_t *)(mod->config);

  textfile_callback(ctx, mod, NULL);

  e->date.tv_sec += cfg->poll_period;

  register_rtaction(ctx, e);

  return (0);
}


static mod_cfg_cmd_t textfile_dir[] = 
{
  { "AddInputFile", add_input_file, "Add a file as input source" },
  { "ProceedAll", set_proceed_all, "Proceed all lines from start" },
  { "SetPollPeriod", set_poll_period, "Set poll period in second for files" },
  { NULL, NULL }
};


input_module_t mod_textfile = {
  MOD_MAGIC,
  ORCHIDS_VERSION,
  "textfile",
  "CeCILL2",
  NULL,
  textfile_dir,
  textfile_preconfig,
  textfile_postconfig,
  textfile_postcompil
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
