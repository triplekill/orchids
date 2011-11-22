/**
 ** @file mod_udp.c
 ** Listen to events on udp sockets.
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
#include <netinet/in.h>
#include <arpa/inet.h>

#include "orchids.h"

#include "orchids_api.h"

#include "mod_udp.h"

input_module_t mod_udp;


static int
create_udp_socket(int udp_port)
{
  int fd, on = 1;
  struct sockaddr_in sin;

  fd = Xsocket(AF_INET, SOCK_DGRAM, 0);

  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(udp_port);

  Xsetsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

  Xbind(fd, (struct sockaddr *) &sin, sizeof(sin));

  return (fd);
}


static int
udp_callback(orchids_t *ctx, mod_entry_t *mod, int fd, void *data)
{
  ovm_var_t *attr[UDP_FIELDS];
  char buf[8194];
  socklen_t sz;
  struct sockaddr_in from;
  socklen_t len;
  event_t *event;

  DebugLog(DF_MOD, DS_TRACE, "udp_callback()\n");

  memset(attr, 0, sizeof(attr));

  len = sizeof (struct sockaddr_in);
  memset(&from, 0, sizeof (struct sockaddr_in));
  sz = Xrecvfrom(fd,buf,8192,0, (struct sockaddr *)&from, &len);
  buf[sz] = '\0';

  DebugLog(DF_MOD, DS_TRACE, "read size = %i\n", sz);

  attr[F_EVENT] = ovm_int_new();
  attr[F_EVENT]->flags |= TYPE_MONO;
  INT(attr[F_EVENT]) = (long) mod->posts;

  attr[F_TIME] = ovm_timeval_new();
  attr[F_TIME]->flags |= TYPE_MONO;
  gettimeofday( &(TIMEVAL(attr[F_TIME])) , NULL);

  attr[F_SRC_ADDR] = ovm_ipv4_new();
  IPV4(attr[F_SRC_ADDR]) = from.sin_addr;

  attr[F_DST_PORT] = ovm_int_new();
  INT(attr[F_DST_PORT]) = (int) data;

  attr[F_MSG] = ovm_bstr_new(sz);
  memcpy(BSTR(attr[F_MSG]), buf, sz);

  event = NULL;
  add_fields_to_event(ctx, mod, &event, attr, UDP_FIELDS);

  post_event(ctx, mod, event);

  return (0);
}


static field_t udp_fields[] = {
  { "udp.event",    T_INT,      "event number"        },
  { "udp.time",     T_TIMEVAL,  "reception time"      },
  { "udp.src_addr", T_IPV4,     "source adress"       },
  { "udp.src_port", T_INT,      "source port"         },
  { "udp.dst_addr", T_IPV4,     "destination address" },
  { "udp.dst_port", T_INT,      "destination port"    },
  { "udp.msg",      T_BSTR,     "message"             }
};


static void *
udp_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  DebugLog(DF_MOD, DS_DEBUG, "load() udp@%p\n", (void *) &mod_udp);

  register_fields(ctx, mod, udp_fields, UDP_FIELDS);

  return (NULL);
}


static void
add_listen_port(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  int sd;
  int port;

  port = atoi(dir->args);
  DebugLog(DF_MOD, DS_INFO, "Add udp listen port %i\n", port);

  if ((port < 1) || (port > 65535)) {
    DebugLog(DF_MOD, DS_WARN, "bad port number.\n");
    return ;
  }

  sd = create_udp_socket(port);
  add_input_descriptor(ctx, mod, udp_callback, sd, (void *)port);
}


static mod_cfg_cmd_t udp_dir[] = {
  { "AddListenPort", add_listen_port, "Add a listen port for udp input" },
  { "INPUT", add_listen_port, "Add a listen port for udp input" },
  { NULL, NULL }
};


input_module_t mod_udp = {
  MOD_MAGIC,
  ORCHIDS_VERSION,
  "udp",
  "CeCILL2",
  NULL,
  udp_dir,
  udp_preconfig,
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
