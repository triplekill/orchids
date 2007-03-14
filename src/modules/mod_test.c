/**
 ** @file mod_test.c
 ** A module for testing the analysis engine.
 **
 ** input syntax :
 ** ^seq_num: [0-9]+ event: [0-9]+$
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 ** @date  Started on: Wed May 21 14:03:49 2003
 ** @date Last update: Tue Nov 29 10:54:27 2005
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

#include "orchids.h"

input_module_t mod_test;

#define TEST_FIELDS 2
#define F_SEQNUM  0
#define F_EVENTTYPE 1

static int
test_dissector(orchids_t *ctx, mod_entry_t *mod, event_t *e, void *data)
{
  ovm_var_t *attr[TEST_FIELDS];
  char   *msg;
  int  msg_len;
  size_t tok_sz;

  DPRINTF( ("test_dissector()\n") );

  memset(attr, 0, sizeof(attr));
  msg     = VSTR(e->value);
  msg_len = VSTRLEN(e->value);

  /* fast syntax check */
  if (strncmp("seq_num: ", msg, 9))
    {
      DPRINTF( ("syntax error\n") );
      return (1);
    }
  msg_len -= 9;
  if (msg_len <= 0)
    {
      DPRINTF( ("syntax error\n") );
      return (1);
    }
  msg += 9; /* 9 = strlen("seq_num: ") */

  /* get the event sequence number */
  attr[F_SEQNUM] = ovm_int_new();
  tok_sz = get_next_int(msg, &(INT(attr[F_SEQNUM])), msg_len);
  if (tok_sz == 0)
    {
      DPRINTF( ("syntax error\n") );
      return (1);
    }
  msg_len -= tok_sz;
  if (msg_len <= 0)
    {
      DPRINTF( ("syntax error\n") );
      return (1);
    }
  msg += tok_sz;
  
  /* fast syntax check */
  if (strncmp(msg, " event: ", 8))
    {
      DPRINTF( ("syntax error\n") );
      return (1);
    }
  msg_len -= 8;
  if (msg_len <= 0)
    {
      DPRINTF( ("syntax error\n") );
      return (1);
    }
  msg += 8; /* 8 = strlen(" event: ") */

  /* get the event type number */
  attr[F_EVENTTYPE] = ovm_int_new();
  tok_sz = get_next_int(msg, &(INT(attr[F_EVENTTYPE])), msg_len);
  if (tok_sz == 0)
    {
      DPRINTF( ("syntax error\n") );
      return (1);
    }
  msg_len -= tok_sz;
  if (msg_len != 0)
    {
      DPRINTF( ("syntax error\n") );
      return (1);
    }


  /* add values to event */
  add_fields_to_event(ctx, mod, &e, attr, TEST_FIELDS);
  /* then, post resulting event */
  post_event(ctx, mod, e);

  return (0);
}

static field_t test_fields[] = {
  { "test.seqnum", T_INT,  "event sequence number"   },
  { "test.eventtype", T_INT, "event type" },
};

static void *
test_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  DPRINTF( ("load() test@%p\n", (void *) &mod_test) );

  register_fields(ctx, mod, test_fields, TEST_FIELDS);
  register_conditional_dissector(ctx, mod, "syslog", (void *)"event-gen", 9,
                                 test_dissector, NULL);

  return (NULL);
}

static char *test_dependencies[] = {
  "syslog",
  NULL
};

input_module_t mod_test = {
  MOD_MAGIC,                /* Magic number */
  ORCHIDS_VERSION,          /* Module version */
  "test",                   /* module name */
  "CeCILL2",                /* module license */
  test_dependencies,        /* module dependencies */
  NULL,                     /* module configuration commands,
                               for core config parser */
  test_preconfig,           /* called just after module registration */
  NULL,                      /* called after all mods preconfig,
                               and after all module configuration*/
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
