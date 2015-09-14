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
#include <errno.h>
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

#ifdef DEBUG_RTACTION
static int rtaction_count_data (heap_t *rt, char *data)
{
  if (rt==NULL)
    return 0;
  return ((rt->entry->data==data)?1:0) + rtaction_count_data (rt->left, data)
    + rtaction_count_data (rt->right, data);
}

static void rtaction_check_unique_data_1 (heap_t *rt, char *data, int limit)
{
  int n;

  n = rtaction_count_data (rt, data);
  if (n>limit)
    {
      fprintf (stderr, "non unique data 0x%p (count=%d)\n", data, n);
      fflush (stderr);
      abort ();
    }
}

static int rtaction_further_blox(orchids_t *ctx, heap_entry_t *he);

static void rtaction_check_unique_data (orchids_t *ctx, heap_t *rt)
{
  if (rt==NULL)
    return;
  if (rt->entry->cb==rtaction_further_blox)
    rtaction_check_unique_data_1 (ctx->rtactionlist, rt->entry->data, 1);
  rtaction_check_unique_data (ctx, rt->left);
  rtaction_check_unique_data (ctx, rt->right);
}

#define rtaction_check(ctx) rtaction_check_unique_data(ctx, ctx->rtactionlist)

void rtaction_print (heap_t *rt, int lmargin)
{
  int i;

  for (i=0; i<lmargin; i++)
    fputc (' ', stderr);
  if (rt==NULL)
    fputs ("- 0\n", stderr);
  else
    {
      fprintf (stderr, "- [%ld:%d]", rt->entry->date.tv_sec, rt->entry->date.tv_usec);
      if (rt->entry->cb==rtaction_further_blox)
	fputs (" blox", stderr);
      fprintf (stderr, " 0x%p\n", rt->entry->data);
      rtaction_print (rt->left, lmargin+2);
      rtaction_print (rt->right, lmargin+2);
    }
}
#endif

static int rtaction_further_blox(orchids_t *ctx, heap_entry_t *he)
{
  blox_hook_t *hook = (blox_hook_t *)he->data;
  blox_config_t *bcfg = hook->bcfg;
  mod_entry_t *mod = bcfg->mod;
  event_t *event = (event_t *)((struct blox_he_data_s *)he->gc_data);
  gc_t *gc_ctx = ctx->gc_ctx;
  unsigned char *stream;
  size_t len, reclen;

#ifdef DEBUG_RTACTION
  fprintf (stderr, "* entering rtaction_further_blox: he->data=0x%p\n", he->data);
  rtaction_print (ctx->rtactionlist, 0);
  fflush (stderr);
  rtaction_check (ctx);
  rtaction_check_unique_data_1 (ctx->rtactionlist, he->data, 0);
#endif
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
      reclen = bcfg->n_first_bytes;
      goto again;
    default:
      reclen = (*bcfg->compute_length) (stream, reclen, len,
					&hook->state,
					bcfg->sd_data);
      goto again;
    }

  (*bcfg->subdissect) (ctx, mod, event, hook->remaining,
		       stream, reclen, bcfg->sd_data, hook->dissector_level);
  blox_skip (gc_ctx, hook, reclen);
  hook->state = BLOX_INIT;
  hook->n_bytes = bcfg->n_first_bytes;  

  if (reclen < len) /* call ourselves back, later */
    {
      hook->flags &= ~BH_WAITING_FOR_INPUT;
      register_rtaction (ctx, he);
#ifdef DEBUG_RTACTION
      fprintf (stderr, "* calling register_rtaction at end of rtaction_further_blox:\n");
      rtaction_print (ctx->rtactionlist, 0);
      fflush (stderr);
      rtaction_check (ctx);
#endif
      return 0;
    }
  /* else fall back to the no_further_blox case: */
 no_further_blox:
#ifdef DEBUG_RTACTION
  fprintf (stderr, "* freeing he at end of rtaction_further_blox:\n");
  rtaction_print (ctx->rtactionlist, 0);
  fflush (stderr);
  rtaction_check (ctx);
#endif
  hook->flags |= BH_WAITING_FOR_INPUT;
  gc_base_free (he);
  return 0;
}

int blox_dissect(orchids_t *ctx, mod_entry_t *mod, event_t *event,
		 void *data, int dissector_level)
{
  blox_hook_t *hook = data;
  blox_config_t *bcfg = hook->bcfg;
  unsigned char *stream, *remstream;
  size_t len, remlen;
  gc_t *gc_ctx = ctx->gc_ctx;
  size_t reclen;
  ovm_var_t *concat;

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
    { /* in which case we reuse event->value, suitably copied to
	 a virtual binary string, which will get updated in blox_skip() */
      concat = ovm_vbstr_new (gc_ctx, event->value);
      VBSTR(concat) = stream;
      VBSTRLEN(concat) = len;
      GC_TOUCH (gc_ctx, hook->remaining = concat);
    }
  else if (len==0) /* or unless no new character is added */
    {
      stream = remstream;
      len = remlen;
    }
  else /* default case */
    {
      concat = ovm_bstr_new (gc_ctx, remlen+len);
      memcpy (BSTR(concat), remstream, remlen);
      memcpy (BSTR(concat)+remlen, stream, len);
      GC_TOUCH (gc_ctx, hook->remaining = concat);
      stream = BSTR(concat);
      len += remlen;
    }
  /* If hook->flags has the BH_WAITING_FOR_INPUT flag set, then
     we just have new input: try to dissect it, and register
     rtaction_further_blox to schedule reading further data from
     that input.
     Otherwise, we just concatenate the additional input (stream)
     into hook->remaining: there is already a rtaction_further_blox
     action scheduled, which will read our data later.
  */
  if (!(hook->flags & BH_WAITING_FOR_INPUT))
    { /* As we just said, we just concatenate; this has just been done,
	 so there is nothing remaining to do */
      return 0;
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
      reclen = bcfg->n_first_bytes;
      goto again;
    default:
      reclen = (*bcfg->compute_length) (stream, reclen, len,
					&hook->state,
					bcfg->sd_data);
      goto again;
    }

  (*bcfg->subdissect) (ctx, mod, event, hook->remaining,
		       stream, reclen,
		       bcfg->sd_data, dissector_level);
  blox_skip (gc_ctx, hook, reclen);
  hook->state = BLOX_INIT;
  hook->n_bytes = bcfg->n_first_bytes;
  hook->dissector_level = dissector_level;

  /* If some bytes remain, register a real time callback
     to feed further records to the subdissector */
  if (reclen < len)
    {
      hook->flags &= ~BH_WAITING_FOR_INPUT;
      register_rtcallback (ctx,
			   rtaction_further_blox,
			   (gc_header_t *)event,
			   (void *)hook,
			   0,
			   hook->dissector_level * 128);
    }
  return 0;
}

blox_config_t *init_blox_config(orchids_t *ctx,
				mod_entry_t *mod,
				size_t n_first_bytes,
				compute_length_fun compute_length,
				subdissect_fun subdissect,
				void *sd_data
				)
{
  gc_t *gc_ctx = ctx->gc_ctx;
  blox_config_t *cfg;

  cfg = gc_base_malloc (gc_ctx, sizeof(blox_config_t));
  if (cfg!=NULL)
    {
      cfg->n_first_bytes = n_first_bytes;
      cfg->compute_length = compute_length;
      cfg->subdissect = subdissect;
      cfg->mod = mod;
      cfg->hooks = NULL;
      cfg->sd_data = sd_data;
    }
  return cfg;
}

blox_hook_t *init_blox_hook(orchids_t *ctx,
			    blox_config_t *bcfg,
			    char *tag,
			    size_t taglen
			    )
{
  gc_t *gc_ctx = ctx->gc_ctx;
  blox_hook_t *hook;

  hook = gc_base_malloc (gc_ctx, sizeof(blox_hook_t));
  if (hook!=NULL)
    {
      hook->next = bcfg->hooks;
      bcfg->hooks = hook;
      hook->bcfg = bcfg;
      hook->tag = tag; /* XXX should we copy this? This comes from directives,
			  which are never freed, as far as I know. */
      hook->taglen = taglen;
      hook->n_bytes = bcfg->n_first_bytes;
      hook->dissector_level = 0; /* by default */
      hook->state = BLOX_INIT;
      hook->flags = BH_WAITING_FOR_INPUT;
      /* The BH_WAITING_FOR_INPUT flag should be set exactly when
	 there is no pending rtaction_further_blox action */
      hook->remaining = NULL;
      gc_add_root (gc_ctx, (gc_header_t **)&hook->remaining);
      hook->event = NULL;
      gc_add_root(gc_ctx, (gc_header_t **)&hook->event);
    }
  return hook;
}

int blox_save (save_ctx_t *sctx, blox_config_t *bcfg, int manage_sharing)
{
  struct blox_hook_s *hook;
  size_t n;
  int c;
  size_t j, m;
  int err;

  for (n=0, hook=bcfg->hooks; hook!=NULL; hook=hook->next) n++;
  err = save_size_t (sctx, n);
  if (err) return err;
  if (manage_sharing)
    for (n=0, hook=bcfg->hooks; hook!=NULL; hook=hook->next)
      {
	estimate_sharing (sctx->gc_ctx, (gc_header_t *)hook->remaining);
	estimate_sharing (sctx->gc_ctx, (gc_header_t *)hook->event);
      }
  for (hook=bcfg->hooks; hook!=NULL; hook=hook->next)
    {
      m = hook->taglen;
      err = save_size_t (sctx, m);
      if (err) goto end;
      for (j=0; j<m; j++)
	{
	  c = hook->tag[j];
	  if (putc (c, sctx->f) < 0) { err = errno; goto end; }
	}
      err = save_size_t (sctx, hook->n_bytes);
      if (err) goto end;
      err = save_int (sctx, hook->state);
      if (err) goto end;
      err = save_int (sctx, hook->flags);
      if (err) goto end;
      err = save_gc_struct (sctx, (gc_header_t *)hook->remaining);
      if (err) goto end;
      err = save_gc_struct (sctx, (gc_header_t *)hook->event);
      if (err) goto end;
    }
 end:
  if (manage_sharing)
    for (n=0, hook=bcfg->hooks; hook!=NULL; hook=hook->next)
      {
	reset_sharing (sctx->gc_ctx, (gc_header_t *)hook->remaining);
	reset_sharing (sctx->gc_ctx, (gc_header_t *)hook->event);
      }
  return err;
}

int blox_restore (restore_ctx_t *rctx, blox_config_t *bcfg)
{
  struct blox_hook_s *hook;
  size_t i, n;
  int c;
  size_t j, m;
  char *tag;
  size_t n_bytes;
  int state, flags;
  ovm_var_t *remaining;
  event_t *event;
  int err = 0;

  GC_START (rctx->gc_ctx, 2);
  err = restore_size_t (rctx, &n);
  if (err) goto end;
  for (i=0; i<n; i++)
    {
      err = restore_size_t (rctx, &m);
      if (err) goto end;
      tag = gc_base_malloc (rctx->gc_ctx, m);
      for (j=0; j<m; j++)
	{
	  c = getc (rctx->f);
	  if (c==EOF)
	    {
	      err = c;
	    err_freetag:
	      gc_base_free (tag);
	      goto end;
	    }
	}
      err = restore_size_t (rctx, &n_bytes);
      if (err) goto err_freetag;
      err = restore_int (rctx, &state);
      if (err) goto err_freetag;
      err = restore_int (rctx, &flags);
      if (err) goto err_freetag;
      remaining = (ovm_var_t *)restore_gc_struct (rctx);
      if (remaining==NULL && errno!=0) { err = errno; goto err_freetag; }
      if (remaining==NULL || (TYPE(remaining)!=T_BSTR && TYPE(remaining)!=T_VBSTR))
	{ err = -2; goto err_freetag; }
      GC_UPDATE (rctx->gc_ctx, 0, remaining);
      event = (event_t *)restore_gc_struct (rctx);
      if (event==NULL && errno!=0) { err = errno; goto err_freetag; }
      if (event!=NULL && TYPE(event)!=T_EVENT) { err = -2; goto err_freetag; }
      GC_UPDATE (rctx->gc_ctx, 1, event);
      for (hook=bcfg->hooks; hook!=NULL; hook=hook->next)
	{ /* linear search through all hooks: this is inefficient,
	     but there should not be many hooks here;
	     otherwise we might use the module's sub_dissectors hash table,
	     but that would not be reliable, since a module that uses
	     the blox facilities is not forced to use only them. */
	  if (hook->taglen==m && memcmp (hook->tag, tag, m)==0)
	    { /* found the blox hook to modify */
	      hook->n_bytes = n_bytes;
	      hook->state = state;
	      hook->flags = flags;
	      GC_TOUCH (rctx->gc_ctx, hook->remaining = remaining);
	      GC_TOUCH (rctx->gc_ctx, hook->event = event);
	      break;
	    }
	  /* The blox API usually has hook->remaining be a vstring
	     pointed into the middle of hook->event->value,
	     but the save/restore mechanism does not keep this sharing
	     (even if we used the estimate_sharing()/reset_sharing()
	     mechanism).  On restoring, we may therefore use a bit more
	     memory than stricly necessary, but that should not cause
	     any other problem.
	  */
	}
      gc_base_free (tag);
    }
 end:
  GC_END (rctx->gc_ctx);
  return err;
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
      int32_t field_num;
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
    {
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
	  DebugLog(DF_CORE,DS_ERROR,
		   "field name '%s' has a prefix that was already registered as a field name\n",
		   ap->name);
	  return;
	}
      i = (int)(unsigned int)(unsigned char)c;
      atpp = &atp->what.proceed[i];
    }
  if (*atpp!=NULL)
    {
      DebugLog(DF_CORE,DS_ERROR,
	       "field name '%s' is equal to or a prefix of an already registered field name\n",
	       ap->name);
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

  atp = NULL;
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
		       int32_t field_num)
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
			int32_t field_num)
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
			    int32_t field_num)
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
		       int32_t field_num)
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
		      int32_t field_num)
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
			  int32_t field_num)
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
		      int32_t field_num)
{
  gc_t *gc_ctx = octx->ctx->gc_ctx;
  ovm_var_t *addr;
  struct addrinfo *sa;
  int status;
  char t[INET6_ADDRSTRLEN];
  GC_START(gc_ctx, 1);

  t[0] = '\0';  
  while (s<end && end-s<INET6_ADDRSTRLEN && (isxdigit(*s) || *s=='.' || *s==':'))
  {
    /* not needed to add '\0' to the string t when the loop ends.
       strncat do it for us. */
    strncat(t, s++, 1); 
  }

  if ((status = getaddrinfo(t, NULL, NULL, &sa)) != 0)
  {
    DebugLog(DF_CORE, DS_ERROR, "getaddrinfo, %s\n", gai_strerror(status));
    addr = NULL;
  }
  else if(sa->ai_family == AF_INET)
  { 
    addr = ovm_ipv4_new(gc_ctx);
    IPV4(addr) = ((struct sockaddr_in *)(sa->ai_addr))->sin_addr;
    freeaddrinfo(sa);
  }
  else
  {
    addr = ovm_ipv6_new(gc_ctx);
    IPV6(addr) = ((struct sockaddr_in6 *)(sa->ai_addr))->sin6_addr;
    freeaddrinfo(sa);
  }
 
  GC_UPDATE(gc_ctx, 0, addr);
  FILL_EVENT(octx, field_num, 1);
  GC_END(gc_ctx); 
  return s;
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
