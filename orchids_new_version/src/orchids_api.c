/**
 ** @file orchids_api.c
 ** Common program maintenance functions.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 1.0
 ** @ingroup core
 **
 ** @date  Started on: Wed Jan 22 16:31:59 2003
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
#include <limits.h>

#include <sys/time.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <sys/select.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <fcntl.h>
#include <errno.h>

#ifdef linux
#include "linux_process_info.h"
#endif /* linux */

#include "orchids.h"
#include "orchids_defaults.h"

#include "engine.h"
#include "mod_mgr.h"
#include "rule_compiler.h"

#include "orchids_api.h"

int
lock_reg(int fd, int cmd, int type, off_t offset, int whence, off_t len)
{
  struct flock lock;

  lock.l_type = type;
  lock.l_start = offset;
  lock.l_whence = whence;
  lock.l_len = len;

  return ( fcntl(fd, cmd, &lock) );
}


pid_t
lock_test(int fd, int type, off_t offset, int whence, off_t len)
{
  struct flock lock;

  lock.l_type = type;
  lock.l_start = offset;
  lock.l_whence = whence;
  lock.l_len = len;

  if (fcntl(fd, F_GETLK, &lock) < 0) {
    perror("fcntl()");
    exit(EXIT_FAILURE);
  }

  if (lock.l_type == F_UNLCK)
    return (0);

  return (lock.l_pid);
}


void
orchids_lock(const char *lockfile)
{
  int fd;
  pid_t pid;
  int ret;

  fd = Xopen(lockfile, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);

  pid = lock_test(fd, F_WRLCK, 0, SEEK_SET, 0);
  if (pid) {
    fprintf(stderr,
            "An Orchids process (pid=%i) already running... Exiting\n",
            pid);
    exit(EXIT_FAILURE);
  }

  ret = Write_lock(fd, 0, SEEK_SET, 0);
  if (ret) {
    if (errno == EACCES || errno == EAGAIN) {
      fprintf(stderr,
              "Another Orchids process already running (race condition).\n");
      perror("fcntl()");
      exit(EXIT_FAILURE);
    }
    else {
      perror("fcntl()");
      exit(EXIT_FAILURE);
    }
  }
}

static void field_record_table_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  size_t i, n;
  field_record_table_t *frt = (field_record_table_t *)p;

  if (frt->fields!=NULL)
    for (i=0, n=frt->num_fields; i<n; i++)
      GC_TOUCH (gc_ctx, frt->fields[i].val);
}

static void field_record_table_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  field_record_table_t *frt = (field_record_table_t *)p;

  if (frt->fields!=NULL)
    gc_base_free (frt->fields);
  /* Nothing else to free; notably, all name and desc fields are
     static. */
}

static int field_record_table_traverse (gc_traverse_ctx_t *gtc,
					gc_header_t *p,
					void *data)
{
  size_t i, n;
  field_record_table_t *frt = (field_record_table_t *)p;
  int err = 0;

  if (frt->fields!=NULL)
    for (i=0, n=frt->num_fields; i<n; i++)
      {
	err = (*gtc->do_subfield) (gtc, (gc_header_t *)frt->fields[i].val,
				   data);
	if (err)
	  return err;
      }
  return err;
}

static gc_class_t field_record_table_class = {
  GC_ID('f','r','c','t'),
  field_record_table_mark_subfields,
  field_record_table_finalize,
  field_record_table_traverse
};

orchids_t *
new_orchids_context(void)
{
  orchids_t *ctx;

  ctx = Xmalloc(sizeof (orchids_t));
  ctx->gc_ctx = gc_init();
  GC_TOUCH(ctx->gc_ctx, ctx->one = ovm_int_new(ctx->gc_ctx, 1));
  gc_add_root(ctx->gc_ctx, (gc_header_t **)&ctx->one);
  GC_TOUCH(ctx->gc_ctx, ctx->zero = ovm_int_new(ctx->gc_ctx, 0));
  gc_add_root(ctx->gc_ctx, (gc_header_t **)&ctx->zero);
  GC_TOUCH(ctx->gc_ctx, ctx->minusone = ovm_int_new(ctx->gc_ctx, -1));
  gc_add_root(ctx->gc_ctx, (gc_header_t **)&ctx->minusone);
  GC_TOUCH(ctx->gc_ctx, ctx->empty_string = ovm_str_new(ctx->gc_ctx, 0));
  gc_add_root(ctx->gc_ctx, (gc_header_t **)&ctx->empty_string);

  /* do default initialisation here */
  gettimeofday(&ctx->start_time, NULL);
  ctx->config_file = DEFAULT_CONFIG_FILE;
  ctx->loaded_modules = 0;
  ctx->last_poll = 0;
  ctx->realtime_handler_list = NULL;
  ctx->maxfd = 0;
  FD_ZERO(&ctx->fds);
  ctx->cfg_tree = NULL;
  ctx->global_fields = NULL; /* for now, will be replaced below */
  ctx->events = 0;
  ctx->rule_compiler = NULL; /* for now, will be replaced below */
  ctx->poll_period.tv_sec = DEFAULT_IN_PERIOD;
  ctx->poll_period.tv_usec = 0;
  ctx->rulefile_list = NULL;
  ctx->last_rulefile = NULL;
#ifdef OBSOLETE
  ctx->first_rule_instance = NULL;
  ctx->last_rule_instance = NULL;
  ctx->retrig_list = NULL;
#endif
  ctx->ovm_stack = NULL; /* for now, will be replaced below */
  ctx->vm_func_tbl = NULL;
  ctx->vm_func_tbl_sz = 0;
  ctx->off_line_mode = 0;
  ctx->off_line_input_file = NULL;
  ctx->daemon = 0;
  ctx->verbose = 0;
  ctx->actmon = 0;
#ifdef OBSOLETE
  ctx->current_tail = NULL;
  gc_add_root(ctx->gc_ctx, (gc_header_t **)&ctx->current_tail);
  ctx->cur_retrig_qh = NULL;
  gc_add_root(ctx->gc_ctx, (gc_header_t **)&ctx->cur_retrig_qh);
  ctx->cur_retrig_qt = NULL;
  gc_add_root(ctx->gc_ctx, (gc_header_t **)&ctx->cur_retrig_qt);
  ctx->new_qh = NULL;
  gc_add_root(ctx->gc_ctx, (gc_header_t **)&ctx->new_qh);
  ctx->new_qt = NULL;
  gc_add_root(ctx->gc_ctx, (gc_header_t **)&ctx->new_qt);
  ctx->retrig_qh = NULL;
  gc_add_root(ctx->gc_ctx, (gc_header_t **)&ctx->retrig_qh);
  ctx->retrig_qt = NULL;
  gc_add_root(ctx->gc_ctx, (gc_header_t **)&ctx->retrig_qt);
  ctx->active_event_head = NULL;
  gc_add_root(ctx->gc_ctx, (gc_header_t **)&ctx->active_event_head);
  ctx->active_event_tail = NULL;
  gc_add_root(ctx->gc_ctx, (gc_header_t **)&ctx->active_event_tail);
  ctx->active_event_cur = NULL;
  gc_add_root(ctx->gc_ctx, (gc_header_t **)&ctx->active_event_cur);
#endif
  ctx->thread_queue = NULL;
  gc_add_root(ctx->gc_ctx, (gc_header_t **)&ctx->thread_queue);
  ctx->current_event = NULL;
  gc_add_root(ctx->gc_ctx, (gc_header_t **)&ctx->current_event);
  ctx->preconfig_time.tv_sec = 0;
  ctx->preconfig_time.tv_usec = 0;
  ctx->postconfig_time.tv_sec = 0;
  ctx->postconfig_time.tv_usec = 0;
  ctx->compil_time.tv_sec = 0;
  ctx->compil_time.tv_usec = 0;
  ctx->postcompil_time.tv_sec = 0;
  ctx->postcompil_time.tv_usec = 0;
#ifdef OBSOLETE
  ctx->evt_fb_fp = NULL;
#endif
  ctx->temporal = NULL; /* for now, will be replaced below */
  ctx->runtime_user = NULL;
  /* ctx->ru not initialized */
  ctx->pid = getpid();
  ctx->reports = 0;
  ctx->last_evt_act = ctx->start_time;
  ctx->last_mod_act.tv_sec = ctx->last_mod_act.tv_usec = 0;
  ctx->last_rule_act.tv_sec = ctx->last_rule_act.tv_usec = 0;
  ctx->last_ruleinst_act = ctx->start_time;
  ctx->rtactionlist = NULL;
  gc_add_root(ctx->gc_ctx, (gc_header_t **)&ctx->rtactionlist);
#ifdef ENABLE_PREPROC
  ctx->default_preproc_cmd = DEFAULT_PREPROC_CMD;
#endif
  ctx->cur_loop_time = ctx->start_time;
  ctx->modules_dir = DEFAULT_MODULES_DIR;
  ctx->lockfile = DEFAULT_ORCHIDS_LOCKFILE;
  SLIST_INIT(&ctx->pre_evt_hook_list);
  SLIST_INIT(&ctx->post_evt_hook_list);
  SLIST_INIT(&ctx->reportmod_list);
  /* End of default and non-memory allocating initializations */

  GC_TOUCH(ctx->gc_ctx,
	   ctx->global_fields = gc_alloc (ctx->gc_ctx,
					  sizeof(field_record_table_t),
					  &field_record_table_class));
  ctx->global_fields->gc.type = T_NULL;
  ctx->global_fields->num_fields = 0;
  ctx->global_fields->fields = NULL;
  gc_add_root(ctx->gc_ctx, (gc_header_t **)&ctx->global_fields);
  /* initialise a rule compiler context */
  GC_TOUCH (ctx->gc_ctx,
	    ctx->rule_compiler = new_rule_compiler_ctx(ctx->gc_ctx));
  gc_add_root(ctx->gc_ctx, (gc_header_t **)&ctx->rule_compiler);

  /* initialise OVM stack */
  GC_TOUCH(ctx->gc_ctx, ctx->ovm_stack = new_stack(ctx->gc_ctx, 128));
  gc_add_root(ctx->gc_ctx, (gc_header_t **)&ctx->ovm_stack);

  /* Register core VM functions */
  register_core_functions(ctx);

  /* initialise other stuff here... */
  //set_lexer_context(ctx->rule_compiler);
  //set_yaccer_context(ctx->rule_compiler);

  ctx->temporal = new_strhash(ctx->gc_ctx, 65537);
  return ctx;
}


void del_input_descriptor(orchids_t *ctx, int fd)
{
  realtime_input_t *rti;
  realtime_input_t *prev;

  prev = NULL;
  for (rti = ctx->realtime_handler_list; rti!=NULL; rti = rti->next)
    {
      if (rti->fd == fd)
	{
	  if (prev!=NULL)
	    prev->next = rti->next;
	  else
	    ctx->realtime_handler_list = rti->next;
	  FD_CLR(fd, &ctx->fds);
	  gc_base_free(rti);
	  return;
	}
      prev = rti;
    }
}


void add_input_descriptor(orchids_t *ctx,
			  mod_entry_t *mod,
			  realtime_callback_t cb,
			  int fd,
			  void *data)
{
  realtime_input_t *rti;

  DebugLog(DF_CORE, DS_INFO, "Adding input descriptor (%i)...\n", fd);

  /* Input fd MUST be a socket (and a fifo ???) XXX - Put sanity checks */

  rti = gc_base_malloc(ctx->gc_ctx, sizeof (realtime_input_t));
  rti->fd = fd;
  rti->data = data;
  rti->mod_id = mod->mod_id;
  rti->cb = cb;
  rti->next = ctx->realtime_handler_list;
  ctx->realtime_handler_list = rti;

  /* Add descriptor to global set */
  if (fd > ctx->maxfd)
    ctx->maxfd = fd;
  FD_SET(fd, &ctx->fds);
}

void
substitute_fd(orchids_t *ctx, int oldfd, int newfd)
{
  realtime_input_t *rti;

  if (oldfd==newfd)
    return;
  for (rti = ctx->realtime_handler_list; rti!=NULL; rti = rti->next) {
      if (rti->fd==oldfd)
        rti->fd = newfd;
    }
}

void reincarnate_fd(orchids_t *ctx, int oldfd, int newfd)
{
  substitute_fd(ctx,oldfd,newfd);
  FD_SET(newfd, &ctx->fds);
}

void register_dissector(orchids_t *ctx,
			mod_entry_t *mod,
			char *parent_modname,
			dissect_t dissect,
			void *data)
{
  mod_entry_t *parent_mod;

  DebugLog(DF_CORE, DS_INFO, "Register unconditional dissector...\n");

  parent_mod = find_module_entry(ctx, parent_modname);
  if (!parent_mod) {
    DebugLog(DF_CORE, DS_FATAL,
             "parent module [%s] not found\n", parent_modname);
    exit(EXIT_FAILURE);
  }

  if (parent_mod->sub_dissectors) {
    DebugLog(DF_CORE, DS_FATAL, "conditional sub dissector(s) present.\n");
    exit(EXIT_FAILURE);
  }

  if (parent_mod->dissect) {
    DebugLog(DF_CORE, DS_FATAL,
             "another module already registered this hook.\n");
    exit(EXIT_FAILURE);
  }

  parent_mod->dissect = dissect;
  parent_mod->dissect_mod = mod;
  parent_mod->data = data;
}


void register_conditional_dissector(orchids_t *ctx,
				    mod_entry_t *m_dissect,
				    char *mod_source_name,
				    /*void *key,*/
				    /*size_t keylen,*/
				    /*dissect_t dissect,*/
				    char *cond_param_str, /*new*/
				    int cond_param_size, /*in case cond_param_str is a bstr */
				    void *data,
				    const char *file, /*new*/
				    uint32_t line) /*new*/
{
  mod_entry_t *m_source;
  conditional_dissector_record_t *cond_dissect;
  dissect_t dissect_func;
  type_t *given_type;
  void	*cond_param;
  char errbuf[32];

  DebugLog(DF_CORE, DS_INFO, "register conditional dissector...\n");

  m_source = find_module_entry(ctx, mod_source_name);
  if (m_source==NULL)
    {
      fprintf (stderr, "%s:%u: unknown source module %s in DISSECT directive.\n",
	       file, line, mod_source_name);
      fflush (stderr);
      exit(EXIT_FAILURE);
    }
  if (m_source->dissect!=NULL)
    {
      fprintf (stderr, "%s:%u: source module %s already has an unconditional dissector.\n",
	       file, line,
	       mod_source_name);
      fflush (stderr);
      exit(EXIT_FAILURE);
    }
  if ((m_source->mod->flags & MODULE_DISSECTABLE)==0)
    {
      fprintf (stderr, "%s:%u: source module %s is not dissectable.\n",
	       file, line,
	       mod_source_name);
      fflush (stderr);
      exit(EXIT_FAILURE);
    }
  dissect_func = m_dissect->mod->dissect_fun;
  if (dissect_func==NULL) // || m_source->num_fields < 2)
    {
      fprintf (stderr, "%s:%u: module %s is not a dissection module.\n",
	       file, line,
	       m_dissect->mod->name);
      fflush (stderr);
      exit(EXIT_FAILURE);
    }
  given_type = ctx->global_fields->fields[m_source->first_field_pos + m_source->num_fields - 1].type;
  if (m_dissect->mod->dissect_type==NULL ||
      strcmp(m_dissect->mod->dissect_type->name, given_type->name))
    {
      fprintf (stderr, "%s:%u: source module %s provides a %s, but dissection module %s requires a %s.\n",
	       file, line,
	       mod_source_name, given_type->name,
	       m_dissect->mod->name, m_dissect->mod->dissect_type->name);
      fflush (stderr);
      //exit(EXIT_FAILURE);
    }
  given_type = ctx->global_fields->fields[m_source->first_field_pos + m_source->num_fields - 2].type;
  switch ((int)(unsigned int)given_type->tag)
    {
    case T_STR:
    case T_VSTR: // subsumed by T_STR, actually
      cond_param = cond_param_str;
      cond_param_size = strlen(cond_param_str);
      break;
    case T_BSTR:
    case T_VBSTR: // subsumed by T_BSTR, actually
      cond_param = cond_param_str;
      /* cond_param_size is given */
      {
	char *s;
	int i, j, len;

	strcpy (errbuf, "<bstr:");
	i = strlen(errbuf);
	s = errbuf+i;
	for (j=0; j<cond_param_size && i+6 < sizeof(errbuf); j++)
	  {
	    len = sprintf (s, "0x%02x", ((char *)cond_param)[j]);
	    i += len;
	    s += len;
	  }
	if (j<cond_param_size) /* overflow */
	  {
	    if (s > errbuf+sizeof(errbuf)-5)
	      s = errbuf+sizeof(errbuf)-5;
	    strcpy (s, "...>");
	  }
	else strcpy (s, ">");
	cond_param_str = errbuf; /* for error reporting */
      }
      break;
    case T_UINT:
      cond_param = gc_base_malloc (ctx->gc_ctx, sizeof (unsigned long));
      *(unsigned long *)cond_param = strtoul(cond_param_str, (char **)NULL, 10);
      cond_param_size = sizeof (unsigned long);
      break;
    case T_INT:
      cond_param = gc_base_malloc (ctx->gc_ctx, sizeof (long));
      *(long *)cond_param = strtol(cond_param_str, (char **)NULL, 10);
      cond_param_size = sizeof (long);
      break;
    case T_IPV4:
      cond_param = gc_base_malloc (ctx->gc_ctx, sizeof (in_addr_t));
      if (inet_pton (AF_INET, cond_param_str, cond_param) != 1)
	{
	  fprintf (stderr,
		   "%s:%u: cannot read %s as ipv4 address.\n",
		   file, line,
		   cond_param_str);
	  fflush (stderr);
	  exit(EXIT_FAILURE);
	}
      cond_param_size = sizeof (in_addr_t);
      break;
    case T_IPV6:
      cond_param = gc_base_malloc (ctx->gc_ctx, sizeof (struct in6_addr));
      /* inet_addr is not IPv6 aware. Use inet_pton instead */
      if (inet_pton (AF_INET6, cond_param_str, cond_param) != 1)
	{
	  fprintf (stderr,
		   "%s:%u: cannot read %s as ipv6 address.\n",
		   file, line,
		   cond_param_str);
	  fflush (stderr);
	  exit(EXIT_FAILURE);
	}
      cond_param_size = sizeof (struct in6_addr);
      break;
    default:
      fprintf (stderr, "%s:%u: source module %s provides dissection key of unrecognized type %s.\n",
		   file, line,
		   mod_source_name, given_type->name);
      fflush (stderr);
      exit(EXIT_FAILURE);
      break;
    }

  if (m_source->sub_dissectors==NULL)
    {
      /* XXX Hard-coded hash size... */
      m_source->sub_dissectors = new_hash(ctx->gc_ctx, 31);
    }
  else if (hash_get(m_source->sub_dissectors, cond_param, cond_param_size))
    {
      fprintf (stderr, "%s:%u: dissection module %s already has a subdissector for key %s",
	       file, line, m_dissect->mod->name, cond_param_str);
      exit(EXIT_FAILURE);
    }

  cond_dissect = gc_base_malloc(ctx->gc_ctx, sizeof (conditional_dissector_record_t));
  cond_dissect->dissect = dissect_func;
  cond_dissect->data = data;
  cond_dissect->mod = m_dissect;

  hash_add(ctx->gc_ctx, m_source->sub_dissectors, cond_dissect, cond_param, cond_param_size);
}


void register_pre_inject_hook(orchids_t *ctx,
			      mod_entry_t *mod,
			      hook_cb_t cb,
			      void *data)
{
  hook_list_elmt_t *e;

  DebugLog(DF_CORE, DS_INFO, "register pre event injection hook...\n");

  e = gc_base_malloc(ctx->gc_ctx, sizeof (hook_list_elmt_t));
  e->cb = cb;
  e->mod = mod;
  e->data = data;
  SLIST_NEXT(e, hooklist) = NULL;
  SLIST_INSERT_HEAD(&ctx->pre_evt_hook_list, e, hooklist);
}


void register_post_inject_hook(orchids_t *ctx,
			       mod_entry_t *mod,
			       hook_cb_t cb,
			       void *data)
{
  hook_list_elmt_t *e;

  DebugLog(DF_CORE, DS_INFO, "register post event injection hook...\n");

  e = gc_base_malloc(ctx->gc_ctx, sizeof (hook_list_elmt_t));
  e->cb = cb;
  e->mod = mod;
  e->data = data;
  SLIST_NEXT(e, hooklist) = NULL;
  SLIST_INSERT_HEAD(&ctx->post_evt_hook_list, e, hooklist);
}

reportmod_t * register_report_output(orchids_t *ctx, mod_entry_t *mod_entry,
				     report_cb_t cb, void *data)
{
  reportmod_t *mod;

  mod = gc_base_malloc(ctx->gc_ctx, sizeof (reportmod_t));
  mod->mod = mod_entry;
  mod->cb = cb;
  mod->data = data;
  SLIST_NEXT(mod, list) = NULL;
  SLIST_INSERT_HEAD(&(ctx->reportmod_list), mod, list);
  return mod;
}

static void event_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  event_t *e = (event_t *)p;

  GC_TOUCH (gc_ctx, e->value);
  GC_TOUCH (gc_ctx, e->next);
}

static void event_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  return;
}

static int event_traverse (gc_traverse_ctx_t *gtc, gc_header_t *p,
			   void *data)
{
  event_t *e = (event_t *)p;
  int err = 0;

  err = (*gtc->do_subfield) (gtc, (gc_header_t *)e->value, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *)e->next, data);
  return err;
}

static gc_class_t event_class = {
  GC_ID('e','v','n','t'),
  event_mark_subfields,
  event_finalize,
  event_traverse
};

void
execute_pre_inject_hooks(orchids_t *ctx, event_t *event)
{
  hook_list_elmt_t *e;

  SLIST_FOREACH(e, &ctx->pre_evt_hook_list, hooklist) {
    (*e->cb) (ctx, e->mod, e->data, event);
  }
}

void
execute_post_inject_hooks(orchids_t *ctx, event_t *event)
{
  hook_list_elmt_t *e;

  SLIST_FOREACH(e, &ctx->post_evt_hook_list, hooklist) {
    (*e->cb)(ctx, e->mod, e->data, event);
  }
}

#if 0
static void field_record_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  field_record_t *fr = (field_record_t *)p;

  GC_TOUCH (gc_ctx, fr->val);
}

static void field_record_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  return; /* nothing to do: name and desc are pointers to static data */
}

static int field_record_traverse (gc_traverse_ctx_t *gtc, gc_header_t *p,
				  void *data)
{
  field_record_t *fr = (field_record_t *)p;
  int err = 0;

  err = (*gtc->do_subfield) (gtc, (gc_header_t *)fr->val, data);
  return err;
}

static gc_class_t field_record_class = {
  GC_ID('f','r','e','c'),
  field_record_mark_subfields,
  field_record_finalize,
  field_record_traverse
};
#endif


void register_fields(orchids_t *ctx, mod_entry_t *mod, field_t *field_tab, size_t sz)
{
  int i, n;
  field_record_t *new_fields;
  gc_t *gc_ctx = ctx->gc_ctx;

  //gc_check(ctx->gc_ctx);
  DebugLog(DF_CORE, DS_TRACE, "registering fields for module '%s'.\n",
           mod->mod->name);

  //gc_check(ctx->gc_ctx);
  /* 1 - Allocate some memory in global field list */
  /* Is it the first allocation ?? */
  if (ctx->global_fields->fields == NULL) {
    ctx->global_fields->fields =
      gc_base_malloc (gc_ctx, sz * sizeof(field_record_t));
    //gc_check(ctx->gc_ctx);
  }
  else {
    ctx->global_fields->fields =
      gc_base_realloc (gc_ctx, ctx->global_fields->fields,
		       (ctx->global_fields->num_fields + sz)
		       * sizeof (field_record_t));
    //gc_check(ctx->gc_ctx);
  }

  /* 2 - index of first module field in global array
  ** (need an offset, because a pointer could be relocated by realloc() ) */
  mod->first_field_pos = n = ctx->global_fields->num_fields;

  /* 3 - Add entries in global list and update counters */
  new_fields = &ctx->global_fields->fields[n];
  for (i = 0; i < sz; ++i) {
    new_fields[i].active = DEFAULT_FIELD_ACTIVATION;
    new_fields[i].name = field_tab[i].name;
    new_fields[i].type = field_tab[i].type;
    new_fields[i].desc = field_tab[i].desc;
    new_fields[i].id   = n + i;
    new_fields[i].mono = field_tab[i].mono;
    new_fields[i].val = NULL;
  }
  mod->num_fields = sz;
  ctx->global_fields->num_fields += sz;
  //gc_check(ctx->gc_ctx);
}


event_t *new_event (gc_t *gc_ctx, int32_t field_id, ovm_var_t *val,
		    event_t *event)
{
  event_t *new_evt;

  new_evt = gc_alloc (gc_ctx, sizeof (event_t), &event_class);
  new_evt->gc.type = T_EVENT;
  new_evt->field_id = field_id;
  GC_TOUCH (gc_ctx, new_evt->value = val);
  GC_TOUCH (gc_ctx, new_evt->next = event);
  return new_evt;
}

void add_fields_to_event_stride(orchids_t *ctx, mod_entry_t *mod,
				event_t **event, ovm_var_t **tbl_event,
				size_t from, size_t to)
{
  int i, j;
  ovm_var_t *evt;
  gc_t *gc_ctx = ctx->gc_ctx;

  for (i = from; i < to; ++i) {
    /* Handle filled fields */
    j = i - from;
    evt = tbl_event[j];
    if ((evt != NULL) && (evt != F_NOT_NEEDED)) {
      /* This field is activated, so add it to current event */
      /* XXX: add checks here ??? (if module is well coded
         no need to check) */
      if (ctx->global_fields->fields[ mod->first_field_pos + i].active)
	{
	  event_t *new_evt;

	  new_evt = new_event (gc_ctx, mod->first_field_pos + i,
			       evt,
			       *event);
	  GC_TOUCH (gc_ctx, *event = new_evt);
	}
    }
  }
}

void add_fields_to_event(orchids_t *ctx, mod_entry_t *mod,
			 event_t **event, ovm_var_t **tbl_event, size_t sz)
{
  add_fields_to_event_stride(ctx,mod,event,tbl_event,0,sz);
}


void fprintf_event(FILE *fp, const orchids_t *ctx, const event_t *event)
{
  fprintf(fp, "-------------------------[ event id: %p ]------------------\n",
	  (void *) event);
  fprintf(fp,
          " fid |"
          "         attribute        |"
          "         value\n");
  fprintf(fp,
          "-----+"
          "--------------------------+"
          "---------------------------------\n");
  for ( ; event!=NULL; event = event->next) {
    fprintf(fp, "%4i | %-24s | ",
            event->field_id,
	    ctx->global_fields->fields[ event->field_id ].name);
    fprintf_issdl_val(fp, ctx, event->value);
  }
  fprintf(fp,
          "-----+"
          "--------------------------+"
          "---------------------------------\n");
}


void post_event(orchids_t *ctx, mod_entry_t *sender, event_t *event, int dissection_level)
{
  int ret;
  conditional_dissector_record_t *cond_dissect;

  DebugLog(DF_CORE, DS_INFO,
           "post_event() -- sender->mod_id = %i\n", sender->mod_id);

  /* XXX: add some stats here (correct/incorrect events, posts, injections) */
  sender->posts++;

  if (sender->dissect!=NULL)
    {
      /* check for unconditional dissector */
      DebugLog(DF_CORE, DS_DEBUG, "Call unconditional sub-dissector.\n");
      ret = (*sender->dissect) (ctx, sender->dissect_mod, event,
				sender->data, dissection_level+1);
      /* Free and optionally inject event if sub-dissector failed */
      if (ret)
	{
	  DebugLog(DF_CORE, DS_WARN, "dissection failed.\n");
	  inject_event(ctx, event);
	}
    }
  else if (sender->sub_dissectors!=NULL)
    {
      /* Check for conditional dissectors */
      DebugLog(DF_CORE, DS_DEBUG, "Resolve conditional dissector\n");
      /* XXX: Fix this / check if event is present
       JGL: should be corrected now, since typing checks whether
       plugged modules fit with each other now
      */
      cond_dissect = hash_get(sender->sub_dissectors,
			      issdl_get_data(event->next->value),
			      issdl_get_data_len(event->next->value));
      if (cond_dissect!=NULL)
	{
	  DebugLog(DF_CORE, DS_DEBUG, "call found conditional sub-dissector.\n");
	  ret = (*cond_dissect->dissect) (ctx,
					  cond_dissect->mod,
					  event,
					  cond_dissect->data,
					  dissection_level+1);
	  /* Free and optionally inject event if sub-dissector fail */
	  if (ret)
	    {
	      DebugLog(DF_CORE, DS_DEBUG, "dissection failed.\n");
	      inject_event(ctx, event);
	    }
	}
      else
	{
	  DebugLog(DF_CORE, DS_TRACE, "no sub-dissectors found. --> inject -->\n");
	  inject_event(ctx, event);
	}
    }
  else
    {
      /* If no sub-dissectors is found, then inject into the analysis engine */
      DebugLog(DF_CORE, DS_TRACE, "--> Injection into analysis engine -->\n");
      inject_event(ctx, event);
    }
}


void
fprintf_orchids_stats(FILE *fp, const orchids_t *ctx)
{
  struct timeval diff_time;
  struct timeval curr_time;
  struct rusage ru;
  float usage;
  struct utsname un;
  char uptime_buf[64];
#ifdef linux
  linux_process_info_t pinfo;
#endif

  gettimeofday(&curr_time, NULL);
  getrusage(RUSAGE_SELF, &ru);

  fprintf(fp, "\n");
  fprintf(fp,
          "-----------------------------[ "
          "orchids statistics"
          " ]-------------------------\n");
  fprintf(fp,
          "           property |"
          "  value\n");
  fprintf(fp,
          "--------------------+"
          "-------------------------------------------------------\n");
  Xuname(&un);
  fprintf(fp, "             System : %s %s %s %s\n",
          un.sysname, un.nodename, un.release, un.machine);
  snprintf_uptime(uptime_buf, sizeof (uptime_buf),
                  curr_time.tv_sec - ctx->start_time.tv_sec);
  fprintf(fp, "        Real uptime : %lis (%s)\n",
          curr_time.tv_sec - ctx->start_time.tv_sec,
          uptime_buf);
  fprintf(fp, "          User time : %li.%03li s\n",
          ru.ru_utime.tv_sec, ru.ru_utime.tv_usec / 1000L);
  fprintf(fp, "        System time : %li.%03li s\n",
          ru.ru_stime.tv_sec, ru.ru_stime.tv_usec / 1000L);
  usage = (float)(ru.ru_stime.tv_sec + ru.ru_utime.tv_sec);
  usage += (float)((ru.ru_stime.tv_usec + ru.ru_utime.tv_usec) / 1000000);
  usage +=((ru.ru_stime.tv_usec + ru.ru_utime.tv_usec) % 1000000) / 1000000.0;
  fprintf(fp, "     Total CPU time : %5.3f s\n", usage);
  usage /= (float)(curr_time.tv_sec - ctx->start_time.tv_sec);
  usage *= 100.0;
  fprintf(fp, "   Average CPU load : %4.2f %%\n", usage);

  Timer_Sub(&diff_time, &ctx->preconfig_time, &ctx->start_time);
  fprintf(fp, "   Pre-config. time : %li.%03li ms\n",
          diff_time.tv_sec * 1000L + diff_time.tv_usec / 1000L,
          diff_time.tv_usec % 1000L);

  Timer_Sub(&diff_time, &ctx->postconfig_time, &ctx->preconfig_time);
  fprintf(fp, "  Post-config. time : %li.%03li ms\n",
          diff_time.tv_sec * 1000L + diff_time.tv_usec / 1000L,
          diff_time.tv_usec % 1000L);

  Timer_Sub(&diff_time, &ctx->compil_time, &ctx->postconfig_time);
  fprintf(fp, " Rules compil. time : %li.%03li ms\n",
          diff_time.tv_sec * 1000L + diff_time.tv_usec / 1000L,
          diff_time.tv_usec % 1000L);

  Timer_Sub(&diff_time, &ctx->postcompil_time, &ctx->compil_time);
  fprintf(fp, "  Post-compil. time : %li.%03li ms\n",
          diff_time.tv_sec * 1000L + diff_time.tv_usec / 1000L,
          diff_time.tv_usec % 1000L);


#ifdef linux
  fprintf(fp,
          "- - - - - - - - - - + - - - - -[ "
          "linux specific"
          " ]- - - - - - - - - - - - - -\n");
  get_linux_process_info(&pinfo, getpid());
  fprintf_linux_process_summary(fp, &pinfo, ctx->verbose);
  fprintf(fp, "- - - - - - - - - - + - - - - - - - - - - - - - - - - - - - - - - - - - - - \n");
#endif /* linux */

  fprintf(fp, "current config file : '%s'\n", ctx->config_file);
  fprintf(fp, "     loaded modules : %i\n", ctx->loaded_modules);
  fprintf(fp, "  registered fields : %zi\n", ctx->global_fields->num_fields);
  fprintf(fp, "    injected events : %lu\n", ctx->events);
  fprintf(fp, "            reports : %zu\n", ctx->reports);
  fprintf(fp,
          "--------------------+"
          "-------------------------------------------------------\n");
}

void
fprintf_hexdump(FILE *fp, const void *data, size_t n)
{
  size_t i;

  for (i = 0; i < n; ++i)
    fprintf(fp, "0x%02X ", ((unsigned char *)data)[i]);
  fprintf(stdout, "\n");
}


void
fprintf_fields(FILE *fp, const orchids_t *ctx)
{
  int base;
  int mod;
  int field;
  int gfid;

  fprintf(fp,
          "---------------------------[ "
          "registered fields"
          " ]-----------------------\n");
  fprintf(fp, "gfid|mfid|      field name      | type     | description\n");
  gfid = 0;
  for (mod = 0; mod < ctx->loaded_modules; mod++) {
    fprintf(fp,
            "----+----+------------[ "
            "module %-16s "
            "]-------------------------\n",
            ctx->mods[mod].mod->name);
    if (ctx->mods[mod].num_fields) {
      for (field = 0; field < ctx->mods[mod].num_fields; ++field) {
        base = ctx->mods[mod].first_field_pos;
        fprintf(fp, "%3i | %2i | %-20s | %-8s | %-32s\n",
                gfid,
                field,
                ctx->global_fields->fields[base + field].name,
                ctx->global_fields->fields[base + field].type->name,
                ctx->global_fields->fields[base + field].desc);
        gfid++;
      }
    }
    else {
      fprintf(fp, " No field.\n");
    }
  }
  fprintf(fp,
          "-----+"
          "--------------------------+"
          "------------------+"
          "-------------------\n");
}


void fprintf_state_env(FILE *fp, const orchids_t *ctx,
		       const state_instance_t *state)
{
  int i;
  ovm_var_t *val;

  for (i = 0; i < state->pid->rule->dynamic_env_sz; ++i) {
    val = ovm_read_value (state->env, i);
    if (val!=NULL)
      {
	fprintf(fp, "    env[%i]: ($%s) ",
		i, state->pid->rule->var_name[i]);
	fprintf_issdl_val(fp, ctx, val);
      }
    else
      {
	fprintf(fp, "            env[%i]: ($%s) nil\n",
		i, state->pid->rule->var_name[i]);
      }
  }
}

static void field_table_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  size_t i, n;
  field_table_t *gft = (field_table_t *)p;

  if (gft->field_values!=NULL)
    for (i=0, n=gft->nfields; i<n; i++)
      GC_TOUCH (gc_ctx, gft->field_values[i]);
}

static void field_table_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  field_table_t *gft = (field_table_t *)p;

  if (gft->field_values!=NULL)
    gc_base_free (gft->field_values);
}

static int field_table_traverse (gc_traverse_ctx_t *gtc,
					gc_header_t *p,
					void *data)
{
  size_t i, n;
  field_table_t *gft = (field_table_t *)p;
  int err = 0;

  if (gft->field_values!=NULL)
    for (i=0, n=gft->nfields; i<n; i++)
      {
	err = (*gtc->do_subfield) (gtc, (gc_header_t *)gft->field_values[i],
				   data);
	if (err)
	  return err;
      }
  return err;
}

static gc_class_t field_table_class = {
  GC_ID('g','f','l','t'),
  field_table_mark_subfields,
  field_table_finalize,
  field_table_traverse
};

field_table_t *new_field_table(gc_t *gc_ctx, size_t nfields)
{
  ovm_var_t **fvals;
  size_t i;
  field_table_t *fields;

  fvals = gc_base_malloc (gc_ctx, nfields*sizeof (ovm_var_t *));
  for (i=0; i<nfields; i++)
    fvals[i] = NULL;
  fields = gc_alloc (gc_ctx, sizeof(field_table_t),
		     &field_table_class);
  fields->gc.type = T_NULL;
  fields->nfields = nfields;
  fields->field_values = fvals;
  return fields;
}

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
