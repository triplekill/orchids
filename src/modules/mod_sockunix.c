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


static int
create_sockunix_socket(const char *path)
{
  struct sockaddr_un sunx;
  int fd;
  int ret;
  size_t len;

  if (path[0] == '\0')
    return (-1);

  ret = unlink(path);
  if (ret == -1) {
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

  return (fd);
}


static int
sockunix_callback(orchids_t *ctx, mod_entry_t *mod, int fd, void *data)
{
  ovm_var_t *attr[SOCKUNIX_FIELDS];
  char buf[8194];
  socklen_t sz;
  struct sockaddr_un from;
  socklen_t len;
  event_t *event;

  DebugLog(DF_MOD, DS_TRACE, "sockunix_callback()\n");

  memset(attr, 0, sizeof(attr));

  sz = Xrecvfrom(fd,buf,8192,0, NULL, NULL);
  buf[sz] = '\0';

  attr[F_TIME] = ovm_timeval_new();
  attr[F_TIME]->flags |= TYPE_MONO;
  gettimeofday( &(TIMEVAL(attr[F_TIME])) , NULL);

  attr[F_EVENT] = ovm_int_new();
  attr[F_EVENT]->flags |= TYPE_MONO;
  INT(attr[F_EVENT]) = (long) mod->posts;

  len = sizeof (struct sockaddr_un);
  memset(&from, 0, sizeof (struct sockaddr_un));
  getsockname(fd, (struct sockaddr *)&from, &len);
  len = strlen(from.sun_path);
  attr[F_SOCKET] = ovm_str_new(len);
  memcpy(STR(attr[F_SOCKET]), from.sun_path, len);

  attr[F_MSG] = ovm_bstr_new(sz);
  memcpy(BSTR(attr[F_MSG]), buf, sz);

  event = NULL;
  add_fields_to_event(ctx, mod, &event, attr, SOCKUNIX_FIELDS);

  post_event(ctx, mod, event);

  return (0);
}

static field_t sockunix_fields[] = {
  { "sockunix.event",    T_INT,      "event number"        },
  { "sockunix.time",     T_TIMEVAL,  "reception time"      },
  { "sockunix.socket",   T_STR,     "unix socket name"    },
  { "sockunix.msg",      T_BSTR,     "message"             }
};

static void *
sockunix_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  DebugLog(DF_MOD, DS_DEBUG, "load() sockunix@%p\n", (void *) &mod_sockunix);

  register_fields(ctx, mod, sockunix_fields, SOCKUNIX_FIELDS);

  return (NULL);
}

static void
add_unix_socket(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  int sd;

  DebugLog(DF_MOD, DS_INFO, "Add unix socket port [%s]\n", dir->args);

  sd = create_sockunix_socket(dir->args);
  add_input_descriptor(ctx, mod, sockunix_callback, sd, (void *)dir->args);
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
