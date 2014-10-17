/**
 ** @file mod_sockunix.c
 ** Listen to an unix local socket...
 **
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 ** @date  Started on: Mon Jan 27 16:56:40 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <time.h> /* for strftime() and localtime() */
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include <errno.h>

#include "orchids.h"

#include "orchids_api.h"

#include "mod_sockunix.h"

input_module_t mod_sockunix;


static int create_sockunix_socket(const char *path)
{
  struct sockaddr_un sunx;
  int fd;
  int ret;
  size_t len;

  if (path[0] == '\0')
    return -1;

  ret = unlink(path);
  if (ret == -1)
    {
      fprintf(stderr, "%s:%i:unlink(path=%s): errno=%i: %s\n",
	      __FILE__, __LINE__, path, errno, strerror(errno));
      if (errno != ENOENT)
	exit(EXIT_FAILURE);
    }

  fd = Xsocket(AF_UNIX, SOCK_DGRAM, 0);

  // The following added by jgl, 30 mai 2012
  memset(&sunx, 0, sizeof(sunx));
  sunx.sun_family = AF_UNIX;
  strncpy(sunx.sun_path, path, sizeof(sunx.sun_path));
  sunx.sun_path[sizeof(sunx.sun_path)-1] = '\0';
  len = SUN_LEN(&sunx);
  // in replacement of:
  /*
  memset(&sunx, 0, sizeof(sunx));
  sunx.sun_family = AF_UNIX;
  strncpy(sunx.sun_path, path, sizeof(sunx.sun_path));
  */

  Xbind(fd, (struct sockaddr *) &sunx, len);
        // was: sizeof(sunx.sun_family) + strlen(sunx.sun_path)); // jgl, 30 mai 2012

  Xchmod(path, 0666);
  // Xconnect(fd, (struct sockaddr *)&sunx, len); // was added by jgl, but does not seen necessary

  return fd;
}

#define MAXSOCKLEN 8192

static int sockunix_callback(orchids_t *ctx, mod_entry_t *mod, int fd, void *data)
{
  gc_t *gc_ctx = ctx->gc_ctx;
  socklen_t sz;
  struct sockaddr_un from;
  socklen_t len;
  ovm_var_t *var;

  GC_START(gc_ctx,SOCKUNIX_FIELDS+1);
  /* SOCKUNIX_FIELDS fields, plus the event to be built */

  DebugLog(DF_MOD, DS_TRACE, "sockunix_callback()\n");

  var = ovm_timeval_new (gc_ctx);
  gettimeofday(&TIMEVAL(var) , NULL);
  GC_UPDATE(gc_ctx, F_TIME, var);

  var = ovm_int_new (gc_ctx, (long) mod->posts);
  GC_UPDATE (gc_ctx, F_EVENT, var);

  var = ovm_bstr_new (gc_ctx, MAXSOCKLEN); /* why +2? */
  GC_UPDATE (gc_ctx, F_MSG, var);
  sz = Xrecvfrom (fd, BSTR(var), MAXSOCKLEN, 0, NULL, NULL);
  BSTRLEN(var) = sz;

  len = sizeof (struct sockaddr_un);
  memset(&from, 0, sizeof (struct sockaddr_un));
  getsockname(fd, (struct sockaddr *)&from, &len);
  len = strlen(from.sun_path);
  var = ovm_str_new (gc_ctx, len);
  memcpy(STR(var), from.sun_path, len);
  GC_UPDATE (gc_ctx, F_SOCKET, var);

  REGISTER_EVENTS(ctx, mod, SOCKUNIX_FIELDS);

  GC_END(gc_ctx);
  return 0;
}

static field_t sockunix_fields[] = {
  { "sockunix.event",    &t_int,      "event number"        },
  { "sockunix.time",     &t_timeval,  "reception time"      },
  { "sockunix.socket",   &t_str,      "unix socket name"    },
  { "sockunix.msg",      &t_bstr,     "message"             }
};

static void * sockunix_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  DebugLog(DF_MOD, DS_DEBUG, "load() sockunix@%p\n", (void *) &mod_sockunix);

  register_fields(ctx, mod, sockunix_fields, SOCKUNIX_FIELDS);
  return NULL;
}

static void add_unix_socket(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  ovm_var_t *var;
  size_t len;
  int sd;

  DebugLog(DF_MOD, DS_INFO, "Add unix socket port [%s]\n", dir->args);

  sd = create_sockunix_socket(dir->args);
  GC_START(ctx->gc_ctx, 1);
  len = strlen(dir->args);
  var = ovm_str_new (ctx->gc_ctx, len);
  memcpy (STR(var), dir->args, len);
  GC_UPDATE(ctx->gc_ctx, 0, var);
  add_input_descriptor(ctx, mod, sockunix_callback, sd, (void *)var);
  GC_END(ctx->gc_ctx);
}

static mod_cfg_cmd_t sockunix_dir[] =
{
  { "AddUnixSocket", add_unix_socket, "Add a unix socket" },
  { "INPUT", add_unix_socket, "Add a unix socket" },
  { NULL, NULL, NULL }
};

input_module_t mod_sockunix = {
  MOD_MAGIC,
  ORCHIDS_VERSION,
  "sockunix",
  "CeCILL2",
  NULL,
  sockunix_dir,
  sockunix_preconfig,
  NULL,
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
