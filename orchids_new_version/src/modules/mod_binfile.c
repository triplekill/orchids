/**
 ** @file mod_binfile.c
 ** binary file input module, for binary log files.
 **
 ** @author Jean Goubault-Larrecq <goubault@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 ** @date  Started on: Sam  9 mai 2015 16:51:40 UTC
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
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#ifndef PATH_MAX
#define PATH_MAX 8192
/* PATH_MAX is undefined on systems without a limit of filename length,
   such as GNU/Hurd.  Also, defining _XOPEN_SOURCE on Linux will make
   PATH_MAX undefined.
*/
#endif

#include <errno.h>

#include "orchids.h"

#include "evt_mgr.h"
#include "orchids_api.h"

#include "mod_binfile.h"

input_module_t mod_binfile;

static int binfile_process_new_lines(orchids_t *ctx, mod_entry_t *mod, binfile_t *bf)
{
  gc_t *gc_ctx = ctx->gc_ctx;
  ovm_var_t *val;
  ssize_t len;
  int eof=0;

  GC_START(gc_ctx, BF_FIELDS+1);
  val = ovm_bstr_new (gc_ctx, BIN_BUFF_SIZE);
  GC_UPDATE(gc_ctx, BF_BLOCK, val);
  len = read (bf->fd, BSTR(val), BIN_BUFF_SIZE);
  if (len < 0)
    {
      DebugLog (DF_MOD, DS_ERROR, "error while reading, errno=%d.\n", errno);
      eof = 1;
    }
  else if (len==0)
    eof = 1;
  else
    {
      BSTRLEN(val) = len;
      GC_UPDATE(gc_ctx, BF_FILE, bf->file_name);
      REGISTER_EVENTS(ctx, mod, BF_FIELDS);
    }
  GC_END(gc_ctx);
  bf->eof = eof;
  return eof;
}

static int binfile_callback(orchids_t *ctx, mod_entry_t *mod, void *dummy)
{
  binfile_config_t *cfg;
  binfile_t *bf;
  struct stat st;
  char eof = 1;

  /*  DebugLog(DF_MOD, DS_TRACE, "binfile_callback();\n"); */

  cfg = (binfile_config_t *)mod->config;
  for (bf = cfg->file_list; bf!=NULL; bf = bf->next)
    {
      if (bf->eof)
	{
	  /* Checks if mtime has changed for each file... */
	  if (fstat(bf->fd, &st))
	    {
	      DebugLog(DF_MOD, DS_ERROR,
		       "file no longer exists in binfile_callback\n");
	      continue;
	    }
	  if (bf->file_stat.st_size > st.st_size)
	    {
	      DebugLog(DF_MOD, DS_NOTICE,
		       "binfile_callback file has been truncated (%lu->%lu) rewind()ing\n",
		       bf->file_stat.st_size, st.st_size);
	      (void) lseek (bf->fd, 0, SEEK_SET);
	    }
	  if (st.st_mtime > bf->file_stat.st_mtime)
	    {
	      DebugLog(DF_MOD, DS_DEBUG, "mtime updated in binfile_callback\n");
	      /* Update file stat */
	      bf->file_stat = st;
	      /* read and process new lines */
	      eof &= binfile_process_new_lines(ctx, mod, bf);
	    }
#ifdef ORCHIDS_DEBUG
	  else if (st.st_mtime < bf->file_stat.st_mtime)
	    {
	      DebugLog(DF_MOD, DS_DEBUG,
		       "mtime older in binfile_callback ?.\n");
	    }
	  else if (st.st_mtime == bf->file_stat.st_mtime)
	    {
	      DebugLog(DF_MOD, DS_DEBUG,
		       "no changes in binfile_callback file\n");
	    }
#endif
	}
      else
	eof &= binfile_process_new_lines(ctx, mod, bf);
    }

  if (cfg->exit_process_all_data && eof)
    exit(EXIT_SUCCESS);
  return eof;
}

static int binsocket_try_reconnect(orchids_t *ctx, heap_entry_t *he)
{
  binsock_t *f = (binsock_t *)he->data;
  struct sockaddr_un sunx;
  int fd, n;

  fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd<0)
    { // cannot even (re)create socket
      DebugLog(DF_MOD, DS_DEBUG,
	       "binsocket_try_reconnect: cannot recreate socket, errno=%d.\n",
	       errno);
      he->date.tv_sec += 5; // XXX hard-coded: 5 seconds into the future
      register_rtaction(ctx, he);
      return 1;
    }
  memset(&sunx, 0, sizeof(sunx));
  sunx.sun_family = AF_UNIX;
  strncpy(sunx.sun_path, STR(f->file_name), STRLEN(f->file_name));
  sunx.sun_path[sizeof(sunx.sun_path)-1] = '\0';
  n = connect(fd, (struct sockaddr *)&sunx, SUN_LEN(&sunx));
  if (n!=0)
    {
       close(fd);
       DebugLog(DF_MOD, DS_DEBUG,
		"binsocket_try_reconnect: cannot connect, errno=%d.\n",
		errno);
       he->date.tv_sec += 1; // XXX hard-coded: 1 second into the future
       register_rtaction(ctx, he);
       return 2;
    }
  reincarnate_fd(ctx,f->fd,fd);
  f->fd = fd;
  gc_base_free(he);
  return 0;
}

static int binsocket_callback(orchids_t *ctx, mod_entry_t *mod,
			      int fd, void *data)
{
  gc_t *gc_ctx = ctx->gc_ctx;
  binsock_t *f = (binsock_t *)data;
  ssize_t len;
  ovm_var_t *val;
  int res=0;

  DebugLog(DF_MOD, DS_TRACE, "binsocket_callback();\n");
  GC_START(gc_ctx, BF_FIELDS+1);
  val = ovm_bstr_new (gc_ctx, BIN_BUFF_SIZE);
  GC_UPDATE(gc_ctx, BF_BLOCK, val);
  len = read (f->fd, BSTR(val), BIN_BUFF_SIZE);
  if (len<0)
    {
      DebugLog (DF_MOD, DS_ERROR, "error while reading, errno=%d.\n", errno);
      res = -1;
    }
  else if (len==0) /* connection lost */
    {
      DebugLog(DF_MOD, DS_INFO, "mod_binfile: connection lost, trying to reconnect.\n");
      //close(fd);
      binfile_config_t *cfg = (binfile_config_t *)mod->config;

      if (cfg->poll_period==0)
	cfg->poll_period = DEFAULT_MODBIN_POLL_PERIOD;

      (void) register_rtcallback(ctx, binsocket_try_reconnect,
				 NULL,
				 data,
				 cfg->poll_period);
      res = 1; // a positive return value will force the event manager
      // to remove fd from the file descriptors watched by select().
    }
  else
    {
      BSTRLEN(val) = len;
      GC_UPDATE(gc_ctx, BF_FILE, f->file_name);
      REGISTER_EVENTS(ctx, mod, BF_FIELDS);
    }
  GC_END(gc_ctx);
  return res;
}

static field_t bf_fields[] = {
  { "binfile.file",     &t_str, MONO_UNKNOWN, "source file name"            },
  { "binfile.block",     &t_bstr, MONO_UNKNOWN,  "current data block" }
};


static void *binfile_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  binfile_config_t *mod_cfg;

  DebugLog(DF_MOD, DS_INFO, "load() binfile@%p\n", (void *) &mod_binfile);

  mod_cfg = gc_base_malloc(ctx->gc_ctx, sizeof (binfile_config_t));
  mod_cfg->flags = 0;
  mod_cfg->process_all_data = 0;
  mod_cfg->exit_process_all_data = 0;
  mod_cfg->poll_period = 0;
  mod_cfg->file_list = NULL;

  //add_polled_input_callback(ctx, mod, binfile_callback, NULL);

  register_fields(ctx, mod, bf_fields, BF_FIELDS);

  return (mod_cfg);
}


static void binfile_postconfig(orchids_t *ctx, mod_entry_t *mod)
{
  DebugLog(DF_MOD, DS_TRACE,
           "binfile_postconfig() -- registering file polling if needed...\n");

  /* register real-time action for file polling, if requested. */
  DebugLog(DF_MOD, DS_INFO, "Activating file polling\n");
  register_rtcallback(ctx,
		      rtaction_read_binfiles,
		      NULL,
		      mod,
		      INITIAL_MODBIN_POLL_DELAY);
}


static void binfile_postcompil(orchids_t *ctx, mod_entry_t *mod)
{
  binfile_config_t *cfg;
  binfile_t *bf;

  DebugLog(DF_MOD, DS_TRACE,
           "binfile_postcompil() -- reading initial file content...\n");

  cfg = (binfile_config_t *)mod->config;

  /* fseek to the end-of-file */
  if (!cfg->process_all_data)
    for (bf = cfg->file_list; bf!=NULL; bf = bf->next)
      (void) lseek (bf->fd, 0, SEEK_END);
}


static void add_input_binfile(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  char filepath[PATH_MAX];
  binfile_config_t *cfg;
  struct stat sbuf;
  char *argstring, *filename;

  cfg = (binfile_config_t *)mod->config;
  argstring = dir->args;
  argstring = dir_parse_string (ctx, dir->file, dir->line, argstring, &filename);
  if (argstring==NULL)
    {
      fprintf (stderr, "%s:%u: missing file name\n", dir->file, dir->line);
      fflush (stderr);
      exit (EXIT_FAILURE);
    }
  if (realpath(filename, filepath)==NULL)
    {
      fprintf (stderr, "%s:%u: could not find real path of file '%s'\n",
	       dir->file, dir->line, dir->args);
      fflush (stderr);
      exit(EXIT_FAILURE);
    }
  gc_base_free (filename);
  if (filepath[0] == '\0')
    {
      fprintf (stderr, "%s:%u: empty file name\n", dir->file, dir->line);
      fflush (stderr);
      exit (EXIT_FAILURE);
    }
  if (stat(filepath, &sbuf))
    {
      fprintf (stderr, "%s:%u: file '%s' does not exist\n", dir->file, dir->line, filepath);
      fflush (stderr);
      exit (EXIT_FAILURE);
    }

  if (S_ISSOCK(sbuf.st_mode)) // warning, S_ISSOCK() is not POSIX...
    { // hope for a AF_UNIX, SOCK_STREAM socket
      DebugLog(DF_MOD, DS_INFO, "adding socket '%s' to selected inputs.\n", filepath);

      struct sockaddr_un sunx;
      int fd;
      binsock_t *f;
      size_t len;

      f = gc_base_malloc(ctx->gc_ctx, sizeof (binsock_t));
      f->flags = BINSOCK_ISSOCK;
      f->file_name = NULL;
      f->fd = 0;

      len = strlen(dir->args);
      GC_TOUCH (ctx->gc_ctx, f->file_name = ovm_str_new (ctx->gc_ctx, len+1));
      /* allocate string, keeping the final '\0' */
      strcpy (STR(f->file_name), dir->args);
      STRLEN(f->file_name) = len;
      gc_add_root (ctx->gc_ctx, (gc_header_t **)&f->file_name);

      fd = socket(AF_UNIX, SOCK_STREAM, 0);
      if (fd<0)
	{
	  fprintf (stderr, "%s:%u: could not open Unix socket '%s'\n",
		   dir->file, dir->line, filepath);
	  exit(EXIT_FAILURE);
	}
      /*
      Xbind(fd, (struct sockaddr *) &sunx,
            sizeof(sunx.sun_family) + strlen(sunx.sun_path));
      */
      memset(&sunx, 0, sizeof(sunx));
      sunx.sun_family = AF_UNIX;
      strncpy(sunx.sun_path, filepath, sizeof(sunx.sun_path));
      sunx.sun_path[sizeof(sunx.sun_path)-1] = '\0';
      len = SUN_LEN(&sunx);
      if (connect(fd, (struct sockaddr *)&sunx, len) < 0)
	{
	  fprintf (stderr, "%s:%u: could not connect to Unix socket '%s': errno=%d: %s",
		   dir->file, dir->line, filepath, errno, strerror(errno));
	  exit (EXIT_FAILURE);
	}

      f->fd = fd;
      add_input_descriptor(ctx, mod, binsocket_callback, fd, (void *)f);
    }
  else if (S_ISFIFO(sbuf.st_mode))
    {
      DebugLog(DF_MOD, DS_INFO, "adding named pipe '%s' to selected inputs.\n", filepath);

      int fd;
      binsock_t *f;
      size_t len;

      f = gc_base_malloc(ctx->gc_ctx, sizeof (binsock_t));
      f->flags = 0;
      f->file_name = NULL;
      f->fd = 0;

      len = strlen(dir->args);
      GC_TOUCH (ctx->gc_ctx, f->file_name = ovm_str_new (ctx->gc_ctx, len+1));
      /* allocate string, keeping the final '\0' */
      strcpy (STR(f->file_name), dir->args);
      STRLEN(f->file_name) = len;
      gc_add_root (ctx->gc_ctx, (gc_header_t **)&f->file_name);

      fd = open(filepath, O_RDWR, 0); // not O_RDONLY, otherwise open() may block
      if (fd < 0)
	{
	  fprintf (stderr, "%s:%u: could not open pipe '%s' for reading\n",
		   dir->file, dir->line, filepath);
	  exit (EXIT_FAILURE);
	}

      f->fd = fd;
      add_input_descriptor(ctx, mod, binsocket_callback, fd, (void *)f);
    }
  else
    { // We assume we read from a regular file (or a pipe) here.
      binfile_t *f;
      size_t len;

      f = gc_base_malloc(ctx->gc_ctx, sizeof (binfile_t));
      f->next = NULL;

      len = strlen(dir->args);
      GC_TOUCH (ctx->gc_ctx, f->file_name = ovm_str_new (ctx->gc_ctx, len+1));
      /* allocate string, keeping the final '\0' */
      strcpy (STR(f->file_name), dir->args);
      STRLEN(f->file_name) = len;
      gc_add_root (ctx->gc_ctx, (gc_header_t **)&f->file_name);

      f->fd = open(filepath, O_RDONLY);
      if (f->fd < 0)
	{
	  fprintf (stderr, "%s:%u: could not open file '%s' for reading\n",
		   dir->file, dir->line, filepath);
	  exit (EXIT_FAILURE);
	}
      Xfstat(f->fd, &f->file_stat);
      f->eof = 0;

      DebugLog(DF_MOD, DS_INFO, "adding next file to polled inputs (fp=%p fd=%i).\n",
	       f->fd, f->fd);

      /* if this file is the first added and poll period is not set,
         activate the file polling with the default poll period */
      if (cfg->file_list == NULL && cfg->poll_period == 0) {
        DebugLog(DF_MOD, DS_INFO,
	         "Automatically activate file polling "
	         "with PollPeriod=%i\n",
	         DEFAULT_MODBIN_POLL_PERIOD);
        cfg->poll_period = DEFAULT_MODBIN_POLL_PERIOD;
      }

      /* add to file list */
      f->next = cfg->file_list;
      cfg->file_list = f;
    }
}


static void binfile_set_process_all(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  int flag;

  DebugLog(DF_MOD, DS_INFO, "setting ProceedAll to %s\n", dir->args);

  flag = strtol(dir->args, (char **)NULL, 10);
  ((binfile_config_t *)mod->config)->process_all_data = flag;
}

static void binfile_set_exit_process_all(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  int flag;

  DebugLog(DF_MOD, DS_INFO, "setting ExitAfterProoceedAll to %s\n", dir->args);

  flag = strtol(dir->args, (char **)NULL, 10);
  ((binfile_config_t *)mod->config)->exit_process_all_data = flag;
}


static void binfile_set_poll_period(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  int value;

  DebugLog(DF_MOD, DS_INFO, "setting PollPeriod to %s\n", dir->args);

  value = strtol(dir->args, (char **)NULL, 10);
  if (value < 0) {
    value = 0;
  }
  ((binfile_config_t *)mod->config)->poll_period = value;
}


static int rtaction_read_binfiles(orchids_t *ctx, heap_entry_t *he)
{
  mod_entry_t *mod;
  binfile_config_t *cfg;
  char	      eof;

  /* DebugLog(DF_MOD, DS_TRACE,
	              "Real-time action: Checking files...\n"); */

  mod = (mod_entry_t *)he->data;
  cfg = (binfile_config_t *)mod->config;

  eof = binfile_callback(ctx, mod, NULL);

  he->date = ctx->cur_loop_time;
  if (eof)
    he->date.tv_sec += cfg->poll_period;
  register_rtaction(ctx, he);
  return 0;
}


static mod_cfg_cmd_t binfile_dir[] =
{
  { "AddInputFile", add_input_binfile, "Add a binary file as input source" },
  { "ProcessAll", binfile_set_process_all, "Process all lines from start" },
  { "ExitAfterProcessAll", binfile_set_exit_process_all, "Exit after processing all files" },
  { "SetPollPeriod", binfile_set_poll_period, "Set poll period in second for files" },
  { "INPUT", add_input_binfile, "Add a binary file as input source" },
  { NULL, NULL }
};


input_module_t mod_binfile = {
  MOD_MAGIC,
  ORCHIDS_VERSION,
  MODULE_DISSECTABLE,
  "binfile",
  "CeCILL2",
  NULL,
  binfile_dir,
  binfile_preconfig,
  binfile_postconfig,
  binfile_postcompil,
  NULL,
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
