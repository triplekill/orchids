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
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

#include "orchids.h"
#include "orchids_api.h"
#include "mod_mgr.h"
#include "evt_mgr.h"
#include "mod_utils.h"

/*** Functions to help splice and dice binary chunks of binary data
 *** used in dissection modules such as openbsm which take binary input
 ***/

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
      reclen = (*hook->compute_length) (stream, reclen, len,
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
      reclen = (*hook->compute_length) (stream, reclen, len,
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

/*** Quick parser functions
 *** can parse sequences of <keyword><data>, as used e.g. in mod_newauditd.c
 ***/

struct action_tree_s {
#define TAG_PROCEED 1
#define TAG_END 2
  int tag;
  union {
    struct action_tree_s *proceed[256];
    struct {
      action_doer action_do;
      int field_num;
      int dummy; /* dummy... only there to know how much to allocate,
		    see action_insert() */
    } code;
  } what;
};

static void action_insert (gc_t *gc_ctx, action_tree_t **atpp,
			   action_t *ap)
{
  char *s, c;
  action_tree_t *atp = *atpp;
  int i;

  for (s=ap->name; (c = *s++)!=0; )
    { /* atp should always be non-NULL here. */
      i = (int)(unsigned int)(unsigned char)c;
      atpp = &atp->what.proceed[i];
      atp = *atpp;
      if (atp==NULL)
	{
	  atp = *atpp = gc_base_malloc (gc_ctx,
					sizeof (action_tree_t));
	  atp->tag = TAG_PROCEED;
	  for (i=0; i<256; i++)
	    atp->what.proceed[i] = NULL;
	}
      else if (atp->tag==TAG_END)
	{
	  DebugLog(DF_CORE,DS_ERROR,"field name has a prefix that was already registered as a field name\n");
	  return;
	}
    }
  if (atp!=NULL)
    {
      DebugLog(DF_CORE,DS_ERROR,"field name is equal to or a prefix of an already registered field name\n");
      return;
    }
  atp = *atpp = gc_base_malloc (gc_ctx,
				offsetof(action_tree_t, what.code.dummy));
  atp->tag = TAG_END;
  atp->what.code.action_do = ap->action_do;
  atp->what.code.field_num = ap->field_num;
}

action_tree_t *compile_actions(gc_t *gc_ctx, action_t *actions)
{
  action_tree_t *atp;
  action_t *ap;
  int i;

  atp = gc_base_malloc (gc_ctx, sizeof (action_tree_t));
  atp->tag = TAG_PROCEED;
  for (i=0; i<256; i++)
    atp->what.proceed[i] = NULL;

  for (ap=actions; ap->name!=NULL; ap++)
    action_insert(gc_ctx, &atp, ap);
  return atp;
}

void action_parse_event (action_orchids_ctx_t *octx, char *data, char *end)
{
  char *s;
  char c;
  int i;
  action_tree_t *atp;

  atp = octx->atree; /* NULL or with tag=TAG_PROCEED */
  if (atp==NULL)
    return;
  /* For the rest of the procedure, atp is always non-NULL,
     or with tag=TAG_PROCEED.
     This assumes that otcx->atree does not just store
     the empty word.
  */
  for (s = data; s<end; )
    {
      c = *s++;
      /* skip spaces */
      if (isspace (c))
	continue;
      /* Try to recognize keyword=value (or audit(...)). */
      goto again;
    start:
      if (s>=end)
	break;
      c = *s++;
    again:
      i = (int)(unsigned int)(unsigned char)c;
      atp = atp->what.proceed[i];
      if (atp==NULL) /* Foiled: no such keyword. */
	atp = octx->atree; /* Ignore this illegal sequence. */
      else if (atp->tag==TAG_END) /* We found something to do! */
	{
	  s = (*atp->what.code.action_do) (octx, s, end,
					   atp->what.code.field_num);
	  atp = octx->atree;
	}
      else goto start;
    }
}

#define FILL_EVENT(octx,n,len) add_fields_to_event_stride(octx->ctx, octx->mod, octx->out_event, (ovm_var_t **)GC_DATA(), n, n+len)

char *orchids_atoui_hex (char *s, char *end, unsigned long *ip)
{
  unsigned long i = 0;
  unsigned long j;
  char c;

  while (s<end && (c = *s, isxdigit (c)))
    {
      if (isdigit (c))
	j = ((int)c) - '0';
      else j = (((int)c) - 'A' + 10) & 0x1f;
      i = 16*i + j;
      s++;
    }
  *ip = i;
  return s;
}

char *action_doer_int (action_orchids_ctx_t *octx, char *s, char *end,
		       int field_num)
{
  long i;
  ovm_var_t *v;
  char *t;
  gc_t *gc_ctx = octx->ctx->gc_ctx;
  GC_START(gc_ctx, 1);

  t = orchids_atoi (s, end-s, &i);
  v = ovm_int_new(gc_ctx, i);
  GC_UPDATE(gc_ctx, 0, v);
  FILL_EVENT(octx, field_num, 1);
  GC_END(gc_ctx);
  return t;
}

char *action_doer_uint (action_orchids_ctx_t *octx, char *s, char *end,
			int field_num)
{
  unsigned long i;
  ovm_var_t *v;
  char *t;
  gc_t *gc_ctx = octx->ctx->gc_ctx;
  GC_START(gc_ctx, 1);

  t = orchids_atoui (s, end-s, &i);
  v = ovm_uint_new(gc_ctx, i);
  GC_UPDATE(gc_ctx, 0, v);
  FILL_EVENT(octx, field_num, 1);
  GC_END(gc_ctx);
  return t;
}

char *action_doer_uint_hex (action_orchids_ctx_t *octx, char *s, char *end,
			    int field_num)
{
  unsigned long i;
  ovm_var_t *v;
  char *t;
  gc_t *gc_ctx = octx->ctx->gc_ctx;
  GC_START(gc_ctx, 1);

  t = orchids_atoui_hex (s, end, &i);
  v = ovm_uint_new(gc_ctx, i);
  GC_UPDATE(gc_ctx, 0, v);
  FILL_EVENT(octx, field_num, 1);
  GC_END(gc_ctx);
  return t;
}

char *action_doer_dev (action_orchids_ctx_t *octx, char *s, char *end,
		       int field_num)
{
  int major=0, minor=0;
  unsigned long i;
  ovm_var_t *v;
  char c;
  gc_t *gc_ctx = octx->ctx->gc_ctx;
  GC_START(gc_ctx, 1);

  while (s<end && (c = *s, isdigit (c)))
    {
      major = 10*major + (((int)c) - '0');
      s++;
    }
  if (s<end && *s==':')
    {
       s++;
       while (s<end && (c = *s, isdigit (c)))
         {
            minor = 10*minor + (((int)c) - '0');
            s++;
         }
    }
  i = (major << 6) | minor;
  v = ovm_uint_new(gc_ctx, i);
  GC_UPDATE(gc_ctx, 0, v);
  FILL_EVENT(octx, field_num, 1);
  GC_END(gc_ctx);
  return s;
}

char *action_doer_id (action_orchids_ctx_t *octx, char *s, char *end,
		      int field_num)
{
  char c, *t;
  ovm_var_t *v;
  gc_t *gc_ctx = octx->ctx->gc_ctx;
  GC_START(gc_ctx, 1);

  v = ovm_vstr_new(gc_ctx, octx->in_event->value);
  VSTR(v) = s;
  for (t=s; t<end && (c = *t, c!=0 && !isspace (c)); t++);
  VSTRLEN(v) = t-s;
  FILL_EVENT(octx, field_num, 1);
  GC_END(gc_ctx);
  return t;
}

char *action_doer_string (action_orchids_ctx_t *octx, char *s, char *end,
			  int field_num)
{
  gc_t *gc_ctx = octx->ctx->gc_ctx;
  char c;
  char *from;
  char *to;
  ovm_var_t *v;
  char *str = NULL;
  size_t str_sz = 0;

  if (s<end && *s != '"')
    return action_doer_id (octx, s, end, field_num);
  from = s++;
  /* We try to allocate the string as a VSTR, except
     if we find backslashed characters, in which case we allocate a STR
     instead.
  */
  GC_START(gc_ctx, 1);
  while (s < end)
    switch (c = *s++)
      {
      case 0: case '"':
	goto build_vstr;
      case '\\': /* cannot keep this as a VSTR: */
	goto scan_str;
      default:
	break;
      }
 build_vstr:
  v = ovm_vstr_new (gc_ctx, octx->in_event->value);
  VSTR(v) = from;
  VSTRLEN(v) = s-1-from;
  goto end;

 scan_str:
  str_sz = end-from; /* string cannot be larger than this */
  v = ovm_str_new (gc_ctx, str_sz);
  str = STR(v);
  s--; /* back up right before the backslash */
  memcpy (str, from, s-from); /* copy what we have seen so far */
  to = str + (s - from);

  while (s<end)
    {
      switch (c = *s++)
	{
	case 0: case '"': goto build_str;
	case '\\':
	  if (s>=end)
	    goto build_str; // malformed string, but who cares
	  c = *s++;
	  switch (c)
	    {
	    case 0: case '"':
	      goto build_str; // malformed string, but who cares
	    case 'n': *to++ = '\n'; break;
	    case 'r': *to++ = '\r'; break;
	    case 't': *to++ = '\t'; break;
	    case '\\': *to++ = '\\'; break;
	    case '0': case '1': case '2': case '3':
	    case '4': case '5': case '6': case '7':
	      {
		/* start of octal number */
		int i = (c - '0');

		if (s>=end)
		  goto build_str; // malformed string, but who cares
		c = *s++;
		switch (c)
		  {
		  case '0': case '1': case '2': case '3':
		  case '4': case '5': case '6': case '7':
		    i = 8*i + (c - '0');
		    if (s>=end)
		      goto build_str; // malformed string, but who cares
		    c = *s++;
		    switch (c)
		      {
		      case '0': case '1': case '2': case '3':
		      case '4': case '5': case '6': case '7':
			i = 8*i + (c - '0');
			break;
		      default: s--; break;
		      }
		    break;
		  default: s--; break;
		  }
		*to++ = (char)i;
	      }
	      break;
	    default: *to++ = c; break;
	    }
	  break;
	default:
	  *to++ = c;
	  break;
	}
    }
 build_str:
  STRLEN(v) = to-str;
  /* goto end; */

 end:
  GC_UPDATE(gc_ctx, 0, v);
  FILL_EVENT(octx, field_num, 1);
  GC_END(gc_ctx);
  return s;
}

char *action_doer_ip (action_orchids_ctx_t *octx, char *s, char *end,
		      int field_num)
{
  gc_t *gc_ctx = octx->ctx->gc_ctx;
  ovm_var_t *addr;
  struct addrinfo *sa;
  int status;
  char *t;
  size_t len;
  GC_START(gc_ctx, 1);  
 
  t = gc_base_malloc(gc_ctx, INET6_ADDRSTRLEN * sizeof(*t));
  len = end - s;
  if (len >= INET6_ADDRSTRLEN)
    len = INET6_ADDRSTRLEN-1;
  memcpy(t, s, len);
  t[len] = "\0";
 

  if ((status = getaddrinfo(t, NULL, NULL, &sa)) != 0)
  {
    DebugLog(DF_CORE, DS_ERROR, "getaddrinfo, %s\n", gai_strerror(status));
    addr = NULL;
  }
  else if(sa->ai_family == AF_INET)
  { 
    addr = ovm_ipv4_new(gc_ctx);
    IVP4(addr) = ((struct sockadrr_in *)(sa->ai_addr))->sin_addr;
    freeaddrinfo(sa);
  }
  else
  {
    addr = ovm_ipv6_new(gc_ctx);
    IPV6(addr) = ((struct sockaddr_in6 *)(sa->ai_addr))->sin6_addr;
    freeaddrinfo(sa);
  }
 
  GC_UPDATE(gc_ctx, 0 , addr);
  FILL_EVENT(octx, field_num, 1);
  GC_END(gc_ctx); 
  return t;
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
