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
#include <errno.h>

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
  VSTRLEN(attr[F_FILE]) = tf->filename_len;

  attr[F_LINE] = ovm_str_new(strlen(buf));
  memcpy (STR(attr[F_LINE]), buf,STRLEN(attr[F_LINE]));

  event = NULL;
  add_fields_to_event(ctx, mod, &event, attr, TF_FIELDS);

  post_event(ctx, mod, event);
}


static int
process_new_lines(orchids_t *ctx, mod_entry_t *mod, textfile_t *tf)
{
  char s_buff[BUFF_SZ];
  char* d_buff = NULL;
  char* buff = NULL;
  unsigned int buff_sz = BUFF_SZ;
  int	eof = 0;
  /* read and process new lines */

  buff_sz = BUFF_SZ;
  eof = (Xfgets(s_buff, BUFF_SZ, tf->fd) == NULL);
  if (!eof)
  {
    if (s_buff[strlen (s_buff) - 1] != '\n')
    {
      d_buff = Xzmalloc (buff_sz);
      do {
	buff_sz += BUFF_SZ;
	if (buff_sz < MAX_LINE_SZ)
	{
	  d_buff = Xrealloc(d_buff, buff_sz);
	  strcat (d_buff, s_buff);
	}
	Xfgets(s_buff, BUFF_SZ, tf->fd);
      } while (s_buff[strlen (s_buff) - 1] != '\n');

      buff_sz += BUFF_SZ;
      d_buff = Xrealloc(d_buff, buff_sz);
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
	Xfree(buff);
    }
    else
    {

      /* update line # */
      tf->line++;
      /* update crc */
      *(unsigned int *)&tf->hash =
	crc32( *(unsigned int *)&tf->hash, buff, strlen(buff));
      DebugLog(DF_MOD, DS_DEBUG,
	       "read line %i crc(0x%08X) %s",
	       tf->line, *(unsigned int *)&tf->hash, buff);

      textfile_buildevent(ctx, mod, tf, buff);
      if (buff == d_buff)
	Xfree(buff);
    }
  }

  tf->eof = eof;

  return eof;
}

static int
textfile_callback(orchids_t *ctx, mod_entry_t *mod, void *dummy)
{
  textfile_config_t *cfg;
  textfile_t *tf;
  struct stat st;
  char		eof = 1;

  /*  DebugLog(DF_MOD, DS_TRACE, "textfile_callback();\n"); */

  cfg = (textfile_config_t *)mod->config;
  for (tf = cfg->file_list; tf; tf = tf->next) {
    if (tf->eof)
    {
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
	/* read and process new lines */
	eof &= process_new_lines(ctx, mod, tf);
      }
#ifdef ORCHIDS_DEBUG
      else if (st.st_mtime < tf->file_stat.st_mtime) {
	DebugLog(DF_MOD, DS_DEBUG, "mtime older for [%s] ?.\n", tf->filename);
      } else if (st.st_mtime == tf->file_stat.st_mtime) {
	DebugLog(DF_MOD, DS_DEBUG, "no changes for [%s]\n", tf->filename);
      }
#endif
    }
    else
      eof &= process_new_lines(ctx, mod, tf);
  }

  if (cfg->exit_process_all_data && eof)
    exit(EXIT_SUCCESS);

  return (eof);
}

static int
textsocket_try_reconnect(orchids_t *ctx, rtaction_t *e)
{
  textsock_t *f = (textsock_t *)e->data;
  struct sockaddr_un sunx;
  int fd, n;

  fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd<0)
    { // cannot even (re)create socket
      DebugLog(DF_MOD, DS_DEBUG, "textsocket_try_reconnect: cannot recreate socket, errno=%d.\n", errno);
      e->date.tv_sec += 5; // XXX hard-coded: 5 seconds into the future
      register_rtaction(ctx,e);
      return 1;
    }
  memset(&sunx, 0, sizeof(sunx));
  sunx.sun_family = AF_UNIX;
  strncpy(sunx.sun_path, f->filename, f->filename_len);
  sunx.sun_path[sizeof(sunx.sun_path)-1] = '\0';
  n = connect(fd, (struct sockaddr *)&sunx, SUN_LEN(&sunx));
  if (n)
    {
       close(fd);
       DebugLog(DF_MOD, DS_DEBUG, "textsocket_try_reconnect: cannot connect, errno=%d.\n", errno);
       e->date.tv_sec += 1; // XXX hard-coded: 1 second into the future
       register_rtaction(ctx,e);
       return 2;
    }
  reincarnate_fd(ctx,f->fd,fd);
  f->fd = fd;
  free(e);
/*
  reincarnate_fd(ctx, f->fd, f->fd);
  free(e);
*/
  return 0;
}

static int
textsocket_callback(orchids_t *ctx, mod_entry_t *mod, int fd, void *data)
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

      rtaction_t *e;

      e = Xzmalloc(sizeof(struct rtaction_s));
      e->date = ctx->cur_loop_time;
      e->date.tv_sec += cfg->poll_period;
	 // try to reconnect later
      e->cb = textsocket_try_reconnect;
      e->data = data;
      register_rtaction(ctx,e);

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
	      ovm_var_t *attr[TF_FIELDS];
	      event_t *event;
	      size_t len;

	      memset(attr, 0, sizeof(attr));

	      attr[F_LINE_NUM] = ovm_int_new();
	      attr[F_LINE_NUM]->flags |= TYPE_MONO;
	      INT(attr[F_LINE_NUM]) = f->line;

	      attr[F_FILE] = ovm_vstr_new();
	      VSTR(attr[F_FILE]) = f->filename;
	      VSTRLEN(attr[F_FILE]) = f->filename_len;

	      len = p-pstart;
	      attr[F_LINE] = ovm_str_new(len);
	      memcpy (STR(attr[F_LINE]), pstart, len);
	      STR(attr[F_LINE])[len] = '\0';

	      DebugLog(DF_MOD, DS_TRACE, "Dissecting: %s", STR(attr[F_LINE]));

	      event = NULL;
	      add_fields_to_event(ctx, mod, &event, attr, TF_FIELDS);

	      post_event(ctx, mod, event);
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
  DebugLog(DF_MOD, DS_INFO, "Activating file polling\n");
  register_rtcallback(ctx,
		      rtaction_read_files,
		      mod,
		      INITIAL_MODTEXT_POLL_DELAY);
}


static void
textfile_postcompil(orchids_t *ctx, mod_entry_t *mod)
{
  textfile_config_t *cfg;
  textfile_t *tf;

  DebugLog(DF_MOD, DS_TRACE,
           "textfile_postcompil() -- reading initial file content...\n");

  cfg = (textfile_config_t *)mod->config;

  /* if we don't have to compute hashes, just fseek to the end-of-file */
  for (tf = cfg->file_list; tf; tf = tf->next) {
    DebugLog(DF_MOD, DS_DEBUG, "process file : %s\n", tf->filename);
    /* read and process new lines */
    process_new_lines(ctx, mod, tf);
  }
}


static void
add_input_file(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  char filepath[PATH_MAX];
  textfile_config_t *cfg;
  struct stat sbuf;

  cfg = (textfile_config_t *)mod->config;
  Xrealpath(dir->args, filepath);
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

      f = Xzmalloc(sizeof (textsock_t));
      f->buf_sz = 2*MAX_LINE_SZ+1; // should be at least 2*MAX_LINE_SZ
      f->buf = Xzmalloc(f->buf_sz);
      f->read_off = f->write_off = 0;
      f->flags = TEXTSOCK_ISSOCK;
      f->line = 0;
      f->filename = strdup(dir->args);
      f->filename_len = strlen(dir->args);


      fd = Xsocket(AF_UNIX, SOCK_STREAM, 0);
      /*
      Xbind(fd, (struct sockaddr *) &sunx,
            sizeof(sunx.sun_family) + strlen(sunx.sun_path));
      */
      memset(&sunx, 0, sizeof(sunx));
      sunx.sun_family = AF_UNIX;
      strncpy(sunx.sun_path, filepath, sizeof(sunx.sun_path));
      sunx.sun_path[sizeof(sunx.sun_path)-1] = '\0';
      len = SUN_LEN(&sunx);
      Xconnect(fd, (struct sockaddr *)&sunx, len);

      f->fd = fd;
      add_input_descriptor(ctx, mod, textsocket_callback, fd, (void *)f);
    }
  else if (S_ISFIFO(sbuf.st_mode))
    {
      DebugLog(DF_MOD, DS_INFO, "adding named pipe '%s' to selected inputs.\n", filepath);

      int fd;
      textsock_t *f;

      f = Xzmalloc(sizeof (textsock_t));
      f->buf_sz = 2*MAX_LINE_SZ+1; // should be at least 2*MAX_LINE_SZ
      f->buf = Xzmalloc(f->buf_sz);
      f->read_off = f->write_off = 0;
      f->flags = 0;
      f->line = 0;
      f->filename = strdup(dir->args);
      f->filename_len = strlen(dir->args);

      fd = Xopen(filepath, O_RDWR, 0); // not O_RDONLY, otherwise open() may block

      f->fd = fd;
      add_input_descriptor(ctx, mod, textsocket_callback, fd, (void *)f);
    }
  else
    { // We assume we read from a regular file (or a pipe) here.
      textfile_t *f;

      f = Xzmalloc(sizeof (textfile_t));
      f->filename = strdup(dir->args);
      f->filename_len = strlen(dir->args);
      f->fd = Xfopen(filepath, "r");
      Xfstat(fileno(f->fd), &f->file_stat);

      DebugLog(DF_MOD, DS_INFO, "adding file '%s' to polled inputs (fp=%p fd=%i).\n",
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
}


static void
set_process_all(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  int flag;

  DebugLog(DF_MOD, DS_INFO, "setting ProoceedAll to %s\n", dir->args);

  flag = atoi(dir->args);
  if (flag)
    ((textfile_config_t *)mod->config)->process_all_data = 1;
}

static void
set_exit_process_all(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  int flag;

  DebugLog(DF_MOD, DS_INFO, "setting ExitAfterProoceedAll to %s\n", dir->args);

  flag = atoi(dir->args);
  if (flag)
    ((textfile_config_t *)mod->config)->exit_process_all_data = 1;
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
  char	      eof;

  /* DebugLog(DF_MOD, DS_TRACE,
	              "Real-time action: Checking files...\n"); */

  mod = (mod_entry_t *)(e->data);
  cfg = (textfile_config_t *)(mod->config);

  eof = textfile_callback(ctx, mod, NULL);

  e->date = ctx->cur_loop_time;
  if (eof)
    e->date.tv_sec += cfg->poll_period;
  register_rtaction(ctx, e);

  return (0);
}


static mod_cfg_cmd_t textfile_dir[] =
{
  { "AddInputFile", add_input_file, "Add a file as input source" },
  { "ProcessAll", set_process_all, "Process all lines from start" },
  { "ExitAfterProcessAll", set_exit_process_all, "Exit after processing all files" },
  { "SetPollPeriod", set_poll_period, "Set poll period in second for files" },
  { "INPUT", add_input_file, "Add a file as input source" },
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
