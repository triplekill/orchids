/**
 ** @file mod_textfile.c
 ** textfile input module, for text log files.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 ** @author Jean Goubault-Larrecq <goubault@lsv.ens-cachan.fr>
 **
 ** @version 0.2
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

#include "mod_textfile.h"


input_module_t mod_textfile;


static void textfile_buildevent(orchids_t *ctx, mod_entry_t *mod,
				textfile_t *tf, char *buf)
{
  ovm_var_t *val;
  size_t len;
  gc_t* gc_ctx = ctx->gc_ctx;
  GC_START(gc_ctx, TF_FIELDS+1);

  DebugLog(DF_MOD, DS_TRACE, "Dissecting: %s", buf);

  val = ovm_int_new (gc_ctx, tf->line);
  GC_UPDATE(gc_ctx, F_LINE_NUM, val);

  GC_UPDATE(gc_ctx, F_FILE, tf->file_name);

  len = strlen(buf);
  val = ovm_str_new (gc_ctx, len);
  memcpy (STR(val), buf, len);
  GC_UPDATE(gc_ctx, F_LINE, val);

  REGISTER_EVENTS(ctx, mod, TF_FIELDS);
  GC_END(gc_ctx);
}


static int textfile_process_new_lines(orchids_t *ctx, mod_entry_t *mod, textfile_t *tf)
{
  char s_buff[BUFF_SZ];
  char *d_buff = NULL;
  char *buff = NULL;
  unsigned int buff_sz = BUFF_SZ;
  int eof = 0;
  /* read and process new lines */

  buff_sz = BUFF_SZ;
  eof = (fgets(s_buff, BUFF_SZ, tf->fd) == NULL);
  if (!eof)
    {
      if (s_buff[strlen (s_buff) - 1] != '\n')
	{
	  d_buff = gc_base_malloc (ctx->gc_ctx, buff_sz);
	  do {
	    buff_sz += BUFF_SZ;
	    if (buff_sz < MAX_LINE_SZ)
	      {
		d_buff = gc_base_realloc(ctx->gc_ctx, d_buff, buff_sz);
		strcat (d_buff, s_buff);
	      }
	    eof = (fgets(s_buff, BUFF_SZ, tf->fd)==NULL);
	    if (eof)
	      break;
	  } while (s_buff[strlen (s_buff) - 1] != '\n');
	  
	  buff_sz += BUFF_SZ;
	  d_buff = gc_base_realloc(ctx->gc_ctx, d_buff, buff_sz);
	  strcat (d_buff, s_buff);
	  
	  buff = d_buff;
	}
      else
	buff = s_buff;
      
      if (buff_sz >= MAX_LINE_SZ)
	{
	  DebugLog(DF_MOD, DS_WARN,
		   "Line too long, dropping event (max line size : %i)",
		   MAX_LINE_SZ);
	  if (buff == d_buff)
	    gc_base_free(buff);
	}
      else
	{
	  /* update line # */
	  tf->line++;
	  DebugLog(DF_MOD, DS_DEBUG,
		   "read line %i %s",
		   tf->line, buff);

	  textfile_buildevent(ctx, mod, tf, buff);
	  if (buff == d_buff)
	    gc_base_free(buff);
	}
    }
  tf->eof = eof;
  return eof;
}

static int textfile_callback(orchids_t *ctx, mod_entry_t *mod, void *dummy)
{
  textfile_config_t *cfg;
  textfile_t *tf;
  struct stat st;
  char		eof = 1;

  /*  DebugLog(DF_MOD, DS_TRACE, "textfile_callback();\n"); */

  cfg = (textfile_config_t *)mod->config;
  for (tf = cfg->file_list; tf!=NULL; tf = tf->next)
    {
      if (tf->eof)
	{
	  /* Checks if mtime has changed for each file... */
	  if (fstat(fileno(tf->fd), &st))
	    {
	      DebugLog(DF_MOD, DS_ERROR,
		       "file no longer exists in textfile_callback\n");
	      continue;
	    }
	  if (tf->file_stat.st_size > st.st_size)
	    {
	      DebugLog(DF_MOD, DS_NOTICE,
		       "textfile_callback file has been truncated (%lu->%lu) rewind()ing\n",
		       tf->file_stat.st_size, st.st_size);
	      rewind(tf->fd);
	    }
	  if (st.st_mtime > tf->file_stat.st_mtime)
	    {
	      DebugLog(DF_MOD, DS_DEBUG, "mtime updated in textfile_callback\n");
	      /* Update file stat */
	      tf->file_stat = st;
	      /* read and process new lines */
	      eof &= textfile_process_new_lines(ctx, mod, tf);
	    }
#ifdef ORCHIDS_DEBUG
	  else if (st.st_mtime < tf->file_stat.st_mtime)
	    {
	      DebugLog(DF_MOD, DS_DEBUG,
		       "mtime older in textfile_callback ?.\n");
	    }
	  else if (st.st_mtime == tf->file_stat.st_mtime)
	    {
	      DebugLog(DF_MOD, DS_DEBUG,
		       "no changes in textfile_callback file\n");
	    }
#endif
	}
      else
	eof &= textfile_process_new_lines(ctx, mod, tf);
    }

  if (cfg->exit_process_all_data && eof)
    exit(EXIT_SUCCESS);
  return eof;
}

static int textsocket_try_reconnect(orchids_t *ctx, heap_entry_t *he)
{
  textsock_t *f = (textsock_t *)he->data;
  struct sockaddr_un sunx;
  int fd, n;

  fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd<0)
    { // cannot even (re)create socket
      DebugLog(DF_MOD, DS_DEBUG,
	       "textsocket_try_reconnect: cannot recreate socket, errno=%d.\n",
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
		"textsocket_try_reconnect: cannot connect, errno=%d.\n",
		errno);
       he->date.tv_sec += 1; // XXX hard-coded: 1 second into the future
       register_rtaction(ctx, he);
       return 2;
    }
  reincarnate_fd(ctx,f->fd,fd);
  f->fd = fd;
  gc_base_free(he);
/*
  reincarnate_fd(ctx, f->fd, f->fd);
  free(e);
*/
  return 0;
}

static int textsocket_callback(orchids_t *ctx, mod_entry_t *mod,
			       int fd, void *data)
{
  textsock_t *f = (textsock_t *)data;
  ssize_t sz;

  DebugLog(DF_MOD, DS_TRACE, "textsocket_callback();\n");
  // The structure of f->buf is as follows:
  // 0                   read_off                write_off              buf_sz
  // |-----old-data------|-----unread-data-------|------free-space------|

  // First, if unread-data is not too long, we move it back to the beginning of
  // the buffer.  We assume no line should be longer than MAX_LINE_SZ.
  // So write_off-read_off < MAX_LINE_SZ, while end_off >= 2*MAX_LINE_SZ normally.

  if (f->buf_sz - f->read_off < MAX_LINE_SZ) // then move (in particular read_off > 0 here)
    { // and in fact read_off > MAX_LINE_SZ since end_off >= 2*MAX_LINE_SZ,
      // so the following memcpy() is safe (no overlapping memory)
      if (f->read_off < f->write_off)
        memcpy (f->buf, f->buf+f->read_off, f->write_off - f->read_off);
      f->write_off -= f->read_off;
      f->read_off = 0;
    }
  sz = read(fd,f->buf+f->write_off,f->buf_sz-f->write_off);
  //sz = Xrecvfrom(fd,f->buf+f->write_off,f->buf_sz-f->write_off,0, NULL, NULL);
  if (sz<0)
    {
      DebugLog(DF_MOD, DS_ERROR, "read() failed, errno=%d.\n", errno);
      return -1;
    }
  else if (sz==0) // connection lost
    {
      DebugLog(DF_MOD, DS_INFO, "mod_textfile: connection lost, trying to reconnect.\n");
      //close(fd);
      textfile_config_t *cfg = (textfile_config_t *)mod->config;

      if (cfg->poll_period==0)
	cfg->poll_period = DEFAULT_MODTEXT_POLL_PERIOD;

      (void) register_rtcallback(ctx, textsocket_try_reconnect,
				 NULL,
				 data,
				 cfg->poll_period);
      return 1; // a positive return value will force the event manager
	// to remove fd from the file descriptors watched by select().
    }

  char *p, *pstart, *pend, c;

  p = f->buf + f->write_off;
  f->write_off += sz;
  pend = f->buf + f->write_off;
  // Now, parse.  We may have several lines, so this may yield several events.

  pstart = f->buf+f->read_off;
  for (; p<pend; )
    {
      c = *p++;
      if (c=='\n')
        { /* Found a line, extending from pstart to p */
	  f->line++;
	  if (f->flags & TEXTSOCK_LINE_TOO_LONG)
	    ; // just drop the line
	  else if (p-pstart > MAX_LINE_SZ) // line too long
            {
	      DebugLog(DF_MOD, DS_WARN,
		       "Line too long (%td bytes), dropping event (max line size : %i)",
		       p-pstart, MAX_LINE_SZ);
            }
          else
            {
	      ovm_var_t *val;
	      size_t len;
	      gc_t* gc_ctx = ctx->gc_ctx;
	      GC_START(gc_ctx, TF_FIELDS+1);

	      DebugLog(DF_MOD, DS_TRACE, "Dissecting socket input");

	      val = ovm_int_new (gc_ctx, f->line);
	      GC_UPDATE(gc_ctx, F_LINE_NUM, val);

	      GC_UPDATE(gc_ctx, F_FILE, f->file_name);

	      len = p-pstart;
	      val = ovm_str_new (gc_ctx, len);
	      memcpy (STR(val), pstart, len);
	      GC_UPDATE(gc_ctx, F_LINE, val);

	      REGISTER_EVENTS(ctx, mod, TF_FIELDS);
	      GC_END(gc_ctx);
            }
          pstart = p;
	  f->read_off = pstart - f->buf;
	  f->flags &= ~TEXTSOCK_LINE_TOO_LONG;
        }
    }
  if (p-pstart > MAX_LINE_SZ) // line not finished, but already too long
    {
      DebugLog(DF_MOD, DS_WARN,
	       "Line too long (%td bytes), dropping event (max line size : %i)",
	       p-pstart, MAX_LINE_SZ);
      f->flags |= TEXTSOCK_LINE_TOO_LONG; // so as to remember to drop the rest of the line
	// when we read it.
    }
  return 0;
}

static field_t tf_fields[] = {
  { "textfile.line_num", &t_int, MONO_MONO,  "line number"                },
  { "textfile.file",     &t_str, MONO_UNKNOWN, "source file name"            },
  { "textfile.line",     &t_str, MONO_UNKNOWN,  "current line" }
};


static void *
textfile_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  textfile_config_t *mod_cfg;

  DebugLog(DF_MOD, DS_INFO, "load() textfile@%p\n", (void *) &mod_textfile);

  mod_cfg = gc_base_malloc(ctx->gc_ctx, sizeof (textfile_config_t));
  mod_cfg->flags = 0;
  mod_cfg->process_all_data = 0;
  mod_cfg->exit_process_all_data = 0;
  mod_cfg->poll_period = 0;
  mod_cfg->file_list = NULL;

  //add_polled_input_callback(ctx, mod, textfile_callback, NULL);

  register_fields(ctx, mod, tf_fields, TF_FIELDS);

  return (mod_cfg);
}


static void textfile_postconfig(orchids_t *ctx, mod_entry_t *mod)
{
  DebugLog(DF_MOD, DS_TRACE,
           "textfile_postconfig() -- registering file polling if needed...\n");

  /* register real-time action for file polling, if requested. */
  DebugLog(DF_MOD, DS_INFO, "Activating file polling\n");
  register_rtcallback(ctx,
		      rtaction_read_textfiles,
		      NULL,
		      mod,
		      INITIAL_MODTEXT_POLL_DELAY);
}


static void textfile_postcompil(orchids_t *ctx, mod_entry_t *mod)
{
  textfile_config_t *cfg;
  textfile_t *tf;

  DebugLog(DF_MOD, DS_TRACE,
           "textfile_postcompil() -- reading initial file content...\n");

  cfg = (textfile_config_t *)mod->config;

  /* fseek to the end-of-file */
  for (tf = cfg->file_list; tf!=NULL; tf = tf->next)
    (void) fseek (tf->fd, 0, SEEK_END);
}


static void add_input_textfile(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  char filepath[PATH_MAX];
  textfile_config_t *cfg;
  struct stat sbuf;

  cfg = (textfile_config_t *)mod->config;
  if (realpath(dir->args, filepath)==NULL)
    {
      fprintf (stderr, "%s: %u: could not find real path of file '%s'\n",
	       dir->file, dir->line, dir->args);
      exit(EXIT_FAILURE);
    }
  if (filepath[0] == '\0')
    {
      DebugLog(DF_MOD, DS_ERROR, "mod_textfile: file name is empty.\n");
      return;
    }
  if (stat(filepath, &sbuf))
    {
      DebugLog(DF_MOD, DS_ERROR, "mod_textfile: file '%s' does not exist.\n", filepath);
      return;
    }

  if (S_ISSOCK(sbuf.st_mode)) // warning, S_ISSOCK() is not POSIX...
    { // hope for a AF_UNIX, SOCK_STREAM socket
      DebugLog(DF_MOD, DS_INFO, "adding socket '%s' to selected inputs.\n", filepath);

      struct sockaddr_un sunx;
      int fd;
      textsock_t *f;
      size_t len;

      f = gc_base_malloc(ctx->gc_ctx, sizeof (textsock_t));
      f->buf_sz = 2*MAX_LINE_SZ+1; // should be at least 2*MAX_LINE_SZ
      f->buf = gc_base_malloc(ctx->gc_ctx, f->buf_sz);
      f->buf[0] = '\0';
      f->flags = TEXTSOCK_ISSOCK;
      f->line = 0;
      f->read_off = f->write_off = 0;
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
	  fprintf (stderr, "%s: %u: could not open Unix socket '%s'\n",
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
	  fprintf (stderr, "%s: %u: could not connect to Unix socket '%s': errno=%d: %s",
		   dir->file, dir->line, filepath, errno, strerror(errno));
	  exit (EXIT_FAILURE);
	}

      f->fd = fd;
      add_input_descriptor(ctx, mod, textsocket_callback, fd, (void *)f);
    }
  else if (S_ISFIFO(sbuf.st_mode))
    {
      DebugLog(DF_MOD, DS_INFO, "adding named pipe '%s' to selected inputs.\n", filepath);

      int fd;
      textsock_t *f;
      size_t len;

      f = gc_base_malloc(ctx->gc_ctx, sizeof (textsock_t));
      f->buf_sz = 2*MAX_LINE_SZ+1; // should be at least 2*MAX_LINE_SZ
      f->buf = gc_base_malloc(ctx->gc_ctx, f->buf_sz);
      f->buf[0] = '\0';
      f->flags = 0;
      f->line = 0;
      f->read_off = f->write_off = 0;
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
	  fprintf (stderr, "%s: %u: could not open file '%s' for reading\n",
		   dir->file, dir->line, filepath);
	  exit (EXIT_FAILURE);
	}

      f->fd = fd;
      add_input_descriptor(ctx, mod, textsocket_callback, fd, (void *)f);
    }
  else
    { // We assume we read from a regular file (or a pipe) here.
      textfile_t *f;
      size_t len;

      f = gc_base_malloc(ctx->gc_ctx, sizeof (textfile_t));
      f->next = NULL;

      len = strlen(dir->args);
      GC_TOUCH (ctx->gc_ctx, f->file_name = ovm_str_new (ctx->gc_ctx, len+1));
      /* allocate string, keeping the final '\0' */
      strcpy (STR(f->file_name), dir->args);
      STRLEN(f->file_name) = len;
      gc_add_root (ctx->gc_ctx, (gc_header_t **)&f->file_name);

      f->fd = fopen(filepath, "r");
      if (f==NULL)
	{
	  fprintf (stderr, "%s: %u: could not open file '%s' for reading\n",
		   dir->file, dir->line, filepath);
	  exit (EXIT_FAILURE);
	}
      Xfstat(fileno(f->fd), &f->file_stat);

      f->line = 0;
      f->eof = 0;

      DebugLog(DF_MOD, DS_INFO, "adding next file to polled inputs (fp=%p fd=%i).\n",
	       f->fd, fileno(f->fd));

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
}


static void textfile_set_process_all(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  int flag;

  DebugLog(DF_MOD, DS_INFO, "setting ProceedAll to %s\n", dir->args);

  flag = strtol(dir->args, (char **)NULL, 10);
  ((textfile_config_t *)mod->config)->process_all_data = flag;
}

static void textfile_setexit_process_all(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  int flag;

  DebugLog(DF_MOD, DS_INFO, "setting ExitAfterProoceedAll to %s\n", dir->args);

  flag = strtol(dir->args, (char **)NULL, 10);
  ((textfile_config_t *)mod->config)->exit_process_all_data = flag;
}


static void textfile_setpoll_period(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  int value;

  DebugLog(DF_MOD, DS_INFO, "setting PollPeriod to %s\n", dir->args);

  value = strtol(dir->args, (char **)NULL, 10);
  if (value < 0) {
    value = 0;
  }

  ((textfile_config_t *)mod->config)->poll_period = value;
}


static int rtaction_read_textfiles(orchids_t *ctx, heap_entry_t *he)
{
  mod_entry_t *mod;
  textfile_config_t *cfg;
  char	      eof;

  /* DebugLog(DF_MOD, DS_TRACE,
	              "Real-time action: Checking files...\n"); */

  mod = (mod_entry_t *)he->data;
  cfg = (textfile_config_t *)mod->config;

  eof = textfile_callback(ctx, mod, NULL);

  he->date = ctx->cur_loop_time;
  if (eof)
    he->date.tv_sec += cfg->poll_period;
  register_rtaction(ctx, he);
  return 0;
}


static mod_cfg_cmd_t textfile_dir[] =
{
  { "AddInputFile", add_input_textfile, "Add a text file as input source" },
  { "ProcessAll", textfile_set_process_all, "Process all lines from start" },
  { "ExitAfterProcessAll", textfile_setexit_process_all, "Exit after processing all files" },
  { "SetPollPeriod", textfile_setpoll_period, "Set poll period in second for files" },
  { "INPUT", add_input_textfile, "Add a text file as input source" },
  { NULL, NULL }
};


input_module_t mod_textfile = {
  MOD_MAGIC,
  ORCHIDS_VERSION,
  MODULE_DISSECTABLE,
  "textfile",
  "CeCILL2",
  NULL,
  textfile_dir,
  textfile_preconfig,
  textfile_postconfig,
  textfile_postcompil,
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
