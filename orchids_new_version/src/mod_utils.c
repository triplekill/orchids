/**
 ** @file mod_utils.c
 ** Utility functions for use in modules
 **
 ** @author Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup core
 **
 ** @date  Started on: Mer  1 oct 2014 09:28:22 UTC
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

#include "orchids_api.h"
#include "mod_mgr.h"
#include "evt_mgr.h"
#include "mod_utils.h"

static void blox_skip(gc_t *gc_ctx, blox_hook_t *hook, long skip)
{
  ovm_var_t *vbstr;

  if (hook->remaining==NULL)
    return; /* should not happen */
  switch (TYPE(hook->remaining))
    {
    case T_BSTR:
      vbstr = ovm_vbstr_new (gc_ctx, hook->remaining);
      VBSTR(vbstr) = BSTR(hook->remaining)+skip;
      VBSTRLEN(vbstr) = BSTRLEN(hook->remaining)-skip;
      GC_TOUCH (gc_ctx, hook->remaining = vbstr);
      break;
    case T_VBSTR:
      vbstr = hook->remaining;
      VBSTR(vbstr) += skip;
      VBSTRLEN(vbstr) -= skip;
      break;
    default: /* should not happen */
      break;
    }
}

static int rtaction_further_blox(orchids_t *ctx, heap_entry_t *he)
{
  blox_hook_t *hook = (blox_hook_t *)he->data;
  mod_entry_t *mod = hook->mod;
  event_t *event = (event_t *)((struct blox_he_data_s *)he->gc_data);
  gc_t *gc_ctx = ctx->gc_ctx;
  unsigned char *stream;
  size_t len, reclen;

  switch (TYPE(hook->remaining))
    {
    case T_BSTR: stream = BSTR(hook->remaining);
      len = BSTRLEN(hook->remaining);
      break;
    case T_VBSTR: stream = VBSTR(hook->remaining);
      len = VBSTRLEN(hook->remaining);
      break;
    default:
      stream = NULL;
      len = 0;
      break;
    }
  reclen = hook->n_bytes;
 again:
  if (len < reclen)
    {
      hook->n_bytes = reclen;
      goto no_further_blox;
    }
  switch (hook->state)
    {
    case BLOX_FINAL:
      /* fine, we have got the desired length of bytes to read in reclen,
       and we managed to read len >= reclen bytes */
      break;
    case BLOX_NOT_ALIGNED:
      /* not a beginning of a record: skip reclen characters */
      blox_skip (gc_ctx, hook, reclen);
      stream += reclen;
      len -= reclen;
      hook->state = BLOX_INIT;
      reclen = hook->n_first_bytes;
      goto again;
    default:
      reclen = (*hook->compute_length) (stream, reclen,
					&hook->state,
					hook->sd_data);
      goto again;
    }

  (*hook->subdissect) (ctx, mod, event, hook->remaining,
		       stream, reclen, hook->sd_data);
  blox_skip (gc_ctx, hook, reclen);
  hook->state = BLOX_INIT;
  hook->n_bytes = hook->n_first_bytes;  

  if (reclen < len) /* call ourselves back, later */
    {
      register_rtaction (ctx, he);
      return 0;
    }
  /* else fall back to the no_further_blox case: */
 no_further_blox:
  gc_base_free (he->data);
  gc_base_free (he);
  return 0;
}

int blox_dissect(orchids_t *ctx, mod_entry_t *mod, event_t *event,
		 void *data)
{
  blox_hook_t *hook = data;
  unsigned char *stream, *remstream;
  size_t len, remlen;
  gc_t *gc_ctx = ctx->gc_ctx;
  size_t reclen;

  if (event->value==NULL)
    {
      DebugLog(DF_MOD, DS_DEBUG, "NULL event value\n");
      return -1;
    }
  switch (TYPE(event->value))
    {
    case T_BSTR: stream = BSTR(event->value); len = BSTRLEN(event->value);
      break;
    case T_VBSTR: stream = VBSTR(event->value); len = VBSTRLEN(event->value);
      break;
    default:
      DebugLog(DF_MOD, DS_DEBUG, "event value not a binary string\n");
      return -1;
    }
  remstream = NULL;
  remlen = 0;
  if (hook->remaining!=NULL)
    {
      switch (TYPE(hook->remaining))
        {
        case T_BSTR: remstream = BSTR(hook->remaining);
	  remlen = BSTRLEN(hook->remaining);
	  break;
        case T_VBSTR: remstream = VBSTR(hook->remaining);
	  remlen = VBSTRLEN(hook->remaining);
	  break;
        }
    }
  /* By default, concatenate hook->remaining and stream. */
  if (remlen==0) /* unless no character remained */
    ;
  else if (len==0) /* or unless no new character is added */
    {
      stream = remstream;
      len = remlen;
    }
  else /* default case */
    {
      ovm_var_t *concat;

      concat = ovm_bstr_new (gc_ctx, remlen+len);
      memcpy (BSTR(concat), remstream, remlen);
      memcpy (BSTR(concat)+remlen, stream, len);
      GC_TOUCH (gc_ctx, hook->remaining = concat);
      stream = BSTR(concat);
      len += remlen;
    }
  /* Now compute record length */
  reclen = hook->n_bytes;
 again:
  if (len < reclen)
    {
      DebugLog(DF_MOD, DS_DEBUG, "Not enough characters yet\n");
      hook->n_bytes = reclen;
      return -1;
    }
  switch (hook->state)
    {
    case BLOX_FINAL:
      /* fine, we have got the desired length of bytes to read in reclen,
       and we managed to read len >= reclen bytes */
      break;
    case BLOX_NOT_ALIGNED:
      /* not a beginning of a record: skip reclen characters */
      blox_skip (gc_ctx, hook, reclen);
      stream += reclen;
      len -= reclen;
      hook->state = BLOX_INIT;
      reclen = hook->n_first_bytes;
      goto again;
    default:
      reclen = (*hook->compute_length) (stream, reclen,
					&hook->state,
					hook->sd_data);
      goto again;
    }

  (*hook->subdissect) (ctx, mod, event, hook->remaining,
		       stream, reclen,
		       hook->sd_data);
  blox_skip (gc_ctx, hook, reclen);
  hook->state = BLOX_INIT;
  hook->n_bytes = hook->n_first_bytes;

  /* If some bytes remain, register a real time callback
     to feed further records to the subdissector */
  if (reclen < len)
    register_rtcallback (ctx,
			 rtaction_further_blox,
			 (gc_header_t *)event,
			 (void *)hook,
			 0);
  return 0;
}

blox_hook_t *init_blox_hook(orchids_t *ctx,
			    mod_entry_t *mod,
			    size_t n_first_bytes,
			    compute_length_fun compute_length,
			    subdissect_fun subdissect,
			    void *sd_data
			    )
{
  gc_t *gc_ctx = ctx->gc_ctx;
  blox_hook_t *hook;

  hook = gc_base_malloc (gc_ctx, sizeof(blox_hook_t));
  if (hook!=NULL)
    {
      hook->n_first_bytes = n_first_bytes;
      hook->n_bytes = n_first_bytes;
      hook->state = BLOX_INIT;
      hook->compute_length = compute_length;
      hook->subdissect = subdissect;
      hook->remaining = NULL;
      gc_add_root (gc_ctx, (gc_header_t **)&hook->remaining);
      hook->event = NULL;
      gc_add_root(gc_ctx, (gc_header_t **)&hook->event);
      hook->mod = mod;
      hook->sd_data = sd_data;
    }
  return hook;
}

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
