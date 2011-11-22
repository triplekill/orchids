/**
 ** @file mod_consoles.c
 ** Multiples log consoles.
 ** Used to test/debug/orchids internals at runtime.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 ** @date  Started on: Mon Jan 27 17:32:49 2003
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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "orchids.h"

#include "orchids_api.h"

#include "mod_consoles.h"

input_module_t mod_consoles;

static conscfg_t *conscfg_g;


static void
issdl_console_msg(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *str;
  ovm_var_t *con;
  char *s;
  char *c;

  con = stack_pop(ctx->ovm_stack);
  if (TYPE(con) != T_STR) {
    DebugLog(DF_ENG, DS_ERROR, "parameter type error (%i)\n", TYPE(con));
    ISSDL_RETURN_PARAM_ERROR(ctx, state);
    return ;
  }

  str = stack_pop(ctx->ovm_stack);
  if (TYPE(str) != T_STR) {
    DebugLog(DF_ENG, DS_ERROR, "parameter type error (%i)\n", TYPE(str));
    ISSDL_RETURN_PARAM_ERROR(ctx, state);
    return ;
  }

  c = ovm_strdup(con);
  s = ovm_strdup(str);

  output_console_msg(c, s);
  ISSDL_RETURN_TRUE(ctx, state);
}


static void
issdl_console_evt(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *con;
  char *c;

  con = stack_pop(ctx->ovm_stack);
  if (TYPE(con) != T_STR) {
    DebugLog(DF_ENG, DS_ERROR, "parameter type error (%i)\n", TYPE(con));
    ISSDL_RETURN_PARAM_ERROR(ctx, state);
    return ;
  }

  c = ovm_strdup(con);

  output_console_evt(ctx, c, state);
  ISSDL_RETURN_TRUE(ctx, state);
}


static void *
cons_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  conscfg_t *mod_cfg;

  DebugLog(DF_MOD, DS_INFO,
           "loading consoles module @ %p\n", (void *) &mod_consoles);

  register_lang_function(ctx, issdl_console_msg,
                         "console_msg", 2, "Console message output");

  register_lang_function(ctx, issdl_console_evt,
                         "console_evt", 1, "Console event output");

  mod_cfg = Xzmalloc(sizeof (conscfg_t));
  mod_cfg->consoles = new_strhash(1021);

  conscfg_g = mod_cfg;

  return (mod_cfg);
}


static FILE *
create_udp_socket(const char *host, const int port)
{
  struct sockaddr_in srvaddr;
  struct hostent *he;
  int sd;
  FILE *sp;

  he = gethostbyname(host);
  if (he == NULL) {
    DebugLog(DF_MOD, DS_ERROR, "Unknown hostname '%s'.\n", host);
    exit(EXIT_FAILURE);
  }

  memset(&srvaddr, 0, sizeof (struct sockaddr_in));
  memcpy(&srvaddr.sin_addr, he->h_addr, he->h_length);
  srvaddr.sin_family = he->h_addrtype;
  srvaddr.sin_port = htons(port);

  sd = Xsocket(PF_INET, SOCK_DGRAM, 0);
  Xconnect(sd, (struct sockaddr *) &srvaddr, sizeof (struct sockaddr));
  sp = Xfdopen(sd, "w");

  return (sp);
}


static void
add_console(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  char console[64];
  char hostname[64];
  int port;
  int ret;
  console_t *con;

  ret = sscanf(dir->args, "%64s %64s %u", console, hostname, &port);
  if (ret != 3) {
    DebugLog(DF_MOD, DS_ERROR, "Error in console module configuration.\n");
    return ;
  }

  DebugLog(DF_MOD, DS_INFO, "Creating new console '%s' to %s:%i\n",
           console, hostname, port);

  if (strhash_get( ((conscfg_t *)mod->config)->consoles, console) != NULL) {
    DebugLog(DF_MOD, DS_ERROR, "console '%s' already defined\n", console);
    return ;
  }

  con = Xzmalloc(sizeof (console_t));
  con->name = strdup(console);
  con->host = strdup(hostname);
  con->port = port;
  con->fp = create_udp_socket(con->host, con->port);

  strhash_add(((conscfg_t *)mod->config)->consoles, con, con->name);
}


static void
output_console_msg(char *console, char *msg)
{
  console_t *con;

  DebugLog(DF_MOD, DS_INFO, "Output msg '%s' to console '%s'\n",
           msg, console);

  con = strhash_get(conscfg_g->consoles, console);
  if (con == NULL) {
    DebugLog(DF_MOD, DS_ERROR, "console '%s' not defined\n", console);
    return ;
  }

  fprintf(con->fp, "%s\n", msg);
  fflush(con->fp);
}


static void
output_console_evt(orchids_t *ctx, char *console, state_instance_t *state)
{
  console_t *con;

  DebugLog(DF_MOD, DS_INFO, "Output msg event to console '%s'\n", console);

  con = strhash_get(conscfg_g->consoles, console);
  if (con == NULL) {
    DebugLog(DF_MOD, DS_ERROR, "console '%s' not defined\n", console);
    return ;
  }

  for ( ; state && state->event == NULL; state = state->parent)
    ;
  if (state && state->event)
    fprintf_event(con->fp, ctx, state->event->event);
  else
    fprintf(con->fp, "No event to display.\n");
  fflush(con->fp);
}

static mod_cfg_cmd_t cons_dir[] = 
{
  { "AddConsole", add_console, "Add a new console (args <name> <host> <udp_port>)" },
  { NULL, NULL }
};

input_module_t mod_consoles = {
  MOD_MAGIC,
  ORCHIDS_VERSION,
  "consoles",
  "CeCILL2",
  NULL,
  cons_dir,
  cons_preconfig,
  NULL, /* cons_postconfig */
  NULL  /* cons_postcompil */
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
