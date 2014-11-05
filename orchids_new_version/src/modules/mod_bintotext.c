/**
 ** @file mod_bintotext.c
 ** Converting raw binary data to lines of text.
 **
 ** @author Jean Goubault-Larrecq <goubault@lsv.ens-cachan.fr>
 ** @version 1.0
 ** @ingroup modules
 ** 
 **
 ** @date  Started on: Mer  5 nov 2014 12:59:22 UTC
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <string.h>
#include "orchids.h"
#include "orchids_api.h"
#include "mod_utils.h"
#include "mod_bintotext.h"

input_module_t mod_bintotext;

static size_t bintotext_compute_length (unsigned char *first_bytes,
				      size_t n_first_bytes,
				      size_t available_bytes,
				      /* available_bytes is ignored here */
				      int *state,
				      void *sd_data)
{
  unsigned char *fin;

  /* n_first_bytes==1, and available_bytes>=1
     is number of bytes available in first_bytes array */
  switch (*state)
    {
    case BLOX_INIT:
    case BLOX_NOT_ALIGNED: /* should not happen */
    case BLOX_FINAL: /* should not happen */
      fin = memchr (first_bytes, '\n', available_bytes);
      break;
    default: /* in all other cases, *state==BLOX_NSTATES+n,
		where n is length of prefix we know does not contain
		any newline */
      fin = memchr (first_bytes + (*state - BLOX_NSTATES), '\n',
		    available_bytes - (*state - BLOX_NSTATES));
      break;
    }
  if (fin==NULL)
    {
      *state = BLOX_NSTATES + available_bytes; /* remember how many bytes
						  we know do not contain
						  a newline yet. */
      return available_bytes+1; /* call me back when you have read
				   at least one more byte */
    }
  /* otherwise we have found a complete line */
  *state = BLOX_FINAL;
  return fin-first_bytes+1; /* this is the length of what we consume:
			       the whole line including the final newline */
}

static void bintotext_subdissect (orchids_t *ctx, mod_entry_t *mod,
				  event_t *event,
				  ovm_var_t *delegate,
				  unsigned char *stream, size_t stream_len,
				  void *sd_data)
{
  ovm_var_t *line;
  gc_t *gc_ctx = ctx->gc_ctx;

  GC_START(gc_ctx, BINTOTEXT_FIELDS+1);
  GC_UPDATE(gc_ctx, BINTOTEXT_FIELDS, event->next);
  //GC_UPDATE(gc_ctx, F_BINTOTEXT_TAG, event->next->value);
  /* When entering bintotext_subdissect(),
     event = [<some bstr>; <some tag>; ...]
     We product a new event of the form [<some line of text>; <some tag>; ...]
     where <some bstr> has been discarded.  This allows
     the next subdissector to be chosen based on the same tag
     (<some tag>) we were given. */
  line = ovm_vstr_new (gc_ctx, delegate);
  VSTR(line) = (char *)stream;
  VSTRLEN(line) = stream_len-1; /* remove the final newline */
  GC_UPDATE(gc_ctx, F_BINTOTEXT_LINE, line);
  REGISTER_EVENTS(ctx, mod, BINTOTEXT_FIELDS);
  GC_END(gc_ctx);
}

static int bintotext_dissect (orchids_t *ctx, mod_entry_t *mod,
			      event_t *event, void *data)
{
  return blox_dissect (ctx, mod, event, mod->config);
}

static field_t bintotext_fields[] = {
  { "bintotext.line", &t_str, "extracted line of text" }
};

static void *bintotext_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  blox_hook_t *hook;

  DebugLog(DF_MOD, DS_INFO, "load() bintotext@%p\n", &mod_bintotext);
  register_fields(ctx, mod, bintotext_fields, BINTOTEXT_FIELDS);
  hook = init_blox_hook (ctx, mod, 1,
			 bintotext_compute_length,
			 bintotext_subdissect,
			 NULL);
  return hook;
}

input_module_t mod_bintotext = {
  MOD_MAGIC, /* Magic number */
  ORCHIDS_VERSION, /* Module version */
  MODULE_DISSECTABLE, /* flags */
  "bintotext",
  "CeCILL2",
  NULL,
  NULL,
  bintotext_preconfig,
  NULL,
  NULL,
  bintotext_dissect,
  &t_bstr, /* type of fields it expects to dissect */
};

/*
** Copyright (c) 2014 by Jean GOUBAULT-LARRECQ, Laboratoire Spécification
** et Vérification (LSV), CNRS UMR 8643 & ENS Cachan.
**
** Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
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


