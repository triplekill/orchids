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
#include <unistd.h>

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


orchids_t *
new_orchids_context(void)
{
  orchids_t *ctx;

  ctx = Xzmalloc(sizeof (orchids_t));

  /* do default initialisation here */
  ctx->default_preproc_cmd = DEFAULT_PREPROC_CMD;
  ctx->config_file = DEFAULT_CONFIG_FILE;
  ctx->poll_period.tv_sec = DEFAULT_IN_PERIOD;
  ctx->poll_period.tv_usec = 0;
  gettimeofday(&ctx->start_time, NULL);
  ctx->last_ruleinst_act = ctx->start_time;
  ctx->last_evt_act = ctx->start_time;
  ctx->cur_loop_time = ctx->start_time;
  ctx->lockfile = DEFAULT_ORCHIDS_LOCKFILE;

  /* initialise a rule compiler context -- XXX: This is not thread safe !
   * but we don't usually want to parse file in parallel */
  ctx->rule_compiler = new_rule_compiler_ctx();

  /* initialise OVM stack */
  ctx->ovm_stack = new_stack(128, 128);

  /* Register core VM functions */
  register_core_functions(ctx);

  /* initialise other stuffs here... */
  set_lexer_context(ctx->rule_compiler);
  set_yaccer_context(ctx->rule_compiler);

  ctx->temporal = new_strhash(65537);

  ctx->pid = getpid();

  ctx->modules_dir = DEFAULT_MODULES_DIR;

  return (ctx);
}


void
add_polled_input_callback(orchids_t *ctx,
                          mod_entry_t *mod,
                          poll_callback_t cb,
                          void *data)
{
  polled_input_t *pi;

  pi = Xzmalloc(sizeof (polled_input_t));
  pi->cb = cb;
  pi->mod = mod;
  pi->data = data;
  pi->next = ctx->poll_handler_list;
  ctx->poll_handler_list = pi;
}


void
del_input_descriptor(orchids_t *ctx, int fd)
{
  realtime_input_t *rti;
  realtime_input_t *prev;

  prev = NULL;
  for (rti = ctx->realtime_handler_list; rti; rti = rti->next) {
    if (rti->fd == fd) {
      if (prev)
        prev->next = rti->next;
      else
        ctx->realtime_handler_list = rti->next;
      FD_CLR(fd, &ctx->fds);
      Xfree(rti);
      return ;
    }
    prev = rti;
  }
}


void
add_input_descriptor(orchids_t *ctx,
                     mod_entry_t *mod,
		     realtime_callback_t cb,
                     int fd,
                     void *data)
{
  realtime_input_t *rti;

  DebugLog(DF_CORE, DS_INFO, "Adding input descriptor (%i)...\n", fd);

  /* Input fd MUST be a socket (and a fifo ???) XXX - Put sanity checks */

  rti = Xzmalloc(sizeof (realtime_input_t));
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

void
register_dissector(orchids_t *ctx,
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


void
register_conditional_dissector(orchids_t *ctx,
                               mod_entry_t *mod,
                               char *parent_modname,
                               void *key,
                               size_t keylen,
                               dissect_t dissect,
                               void *data)
{
  mod_entry_t *parent_mod;
  conditional_dissector_record_t *cond_dissect;

  DebugLog(DF_CORE, DS_INFO, "register conditional dissector...\n");

  parent_mod = find_module_entry(ctx, parent_modname);
  if (!parent_mod) {
    DebugLog(DF_CORE, DS_FATAL,
             "parent module [%s] not found\n", parent_modname);
    exit(EXIT_FAILURE);
  }

  if (parent_mod->dissect) {
    DebugLog(DF_CORE, DS_FATAL, "unconditional sub dissector present.\n");
    exit(EXIT_FAILURE);
  }

  if (!parent_mod->sub_dissectors) {
    /* XXX Hard-coded hash size... */
    parent_mod->sub_dissectors = new_hash(31);
  } else if (hash_get(parent_mod->sub_dissectors, key, keylen)) {
    DebugLog(DF_CORE, DS_FATAL, "another module registered this value.\n");
    exit(EXIT_FAILURE);
  }

  cond_dissect = Xmalloc(sizeof (conditional_dissector_record_t));
  cond_dissect->dissect = dissect;
  cond_dissect->data = data;
  cond_dissect->mod = mod;

  hash_add(parent_mod->sub_dissectors, cond_dissect, key, keylen);
}


void
register_pre_inject_hook(orchids_t *ctx,
			 mod_entry_t *mod,
			 hook_cb_t cb,
			 void *data)
{
  hook_list_elmt_t *e;

  DebugLog(DF_CORE, DS_INFO, "register pre event injection hook...\n");

  e = Xzmalloc( sizeof (hook_list_elmt_t) );
  e->cb = cb;
  e->mod = mod;
  e->data = data;

  SLIST_INSERT_HEAD(&ctx->pre_evt_hook_list, e, hooklist);
}


void
register_post_inject_hook(orchids_t *ctx,
                          mod_entry_t *mod,
                          hook_cb_t cb,
                          void *data)
{
  hook_list_elmt_t *e;

  DebugLog(DF_CORE, DS_INFO, "register post event injection hook...\n");

  e = Xzmalloc( sizeof (hook_list_elmt_t) );
  e->cb = cb;
  e->mod = mod;
  e->data = data;

  SLIST_INSERT_HEAD(&ctx->post_evt_hook_list, e, hooklist);
}

reportmod_t *
register_report_output(orchids_t *ctx, mod_entry_t *mod_entry, report_cb_t cb, void *data)
{
  reportmod_t *mod;

  mod = Xzmalloc(sizeof (reportmod_t));
  mod->mod = mod_entry;
  mod->cb = cb;
  mod->data = data;

  SLIST_INSERT_HEAD(&(ctx->reportmod_list), mod, list);

  return (mod);
}

void
execute_pre_inject_hooks(orchids_t *ctx, event_t *event)
{
  hook_list_elmt_t *e;

  SLIST_FOREACH(e, &ctx->pre_evt_hook_list, hooklist) {
    e->cb(ctx, e->mod, e->data, event);
  }
}

void
execute_post_inject_hooks(orchids_t *ctx, event_t *event)
{
  hook_list_elmt_t *e;

  SLIST_FOREACH(e, &ctx->post_evt_hook_list, hooklist) {
    e->cb(ctx, e->mod, e->data, event);
  }
}


void
register_fields(orchids_t *ctx, mod_entry_t *mod, field_t *field_tab, size_t sz)
{
  int i;
  field_record_t *new_fields;

  DebugLog(DF_CORE, DS_TRACE, "registering fields for module '%s'.\n",
           mod->mod->name);

  /* 1 - Allocate some memory in global field list */
  /* Is it the first allocation ?? */
  if (ctx->global_fields == NULL) {
    ctx->global_fields =
      Xmalloc(sz * sizeof(field_record_t));
  }
  else {
    ctx->global_fields =
      Xrealloc(ctx->global_fields,
               (ctx->num_fields + sz) * sizeof (field_record_t));
  }

  /* 2 - index of first module field in global array
  ** (need an offset, because a pointer could be relocated by realloc() ) */
  mod->first_field_pos = ctx->num_fields;

  /* 3 - Add entries in global list and update counters */
  new_fields = &ctx->global_fields[ ctx->num_fields ];
  for (i = 0; i < sz; ++i) {
    new_fields[i].active = DEFAULT_FIELD_ACTIVATION;
    new_fields[i].name = field_tab[i].name;
    new_fields[i].type = field_tab[i].type;
    new_fields[i].desc = field_tab[i].desc;
    new_fields[i].id   = ctx->num_fields + i;
  }
  mod->num_fields = sz;
  ctx->num_fields += sz;
}


void
free_fields(ovm_var_t **tbl_event, size_t s)
{
  int i;

  for (i = 0; i < s; ++i)
    if ((tbl_event[i] != NULL) && (tbl_event[i] != F_NOT_NEEDED))
      Xfree(tbl_event[i]);
}


void
add_fields_to_event_stride(orchids_t *ctx, mod_entry_t *mod,
			   event_t **event, ovm_var_t **tbl_event,
			   size_t from, size_t to)
{
  int i, j;

  for (i = from; i < to; ++i) {
    /* Handle filled fields */
    j = i - from;
    if ((tbl_event[j] != NULL) && (tbl_event[j] != F_NOT_NEEDED)) {
      /* This field is activated, so add it to current event */
      /* XXX: add checks here ??? (if module is well coded
         no need to check) */
      if (ctx->global_fields[ mod->first_field_pos + i].active) {
        event_t *new_evt;

        new_evt = Xmalloc(sizeof (event_t));
        new_evt->field_id = mod->first_field_pos + i;
        new_evt->value = tbl_event[j];
        new_evt->next = *event;
        *event = new_evt;
      } else { /* drop data */
        DebugLog(DF_CORE, DS_TRACE, "free disabled attribute.\n");
        Xfree(tbl_event[j]);
      }
    }
  }
}

void
add_fields_to_event(orchids_t *ctx, mod_entry_t *mod,
                    event_t **event, ovm_var_t **tbl_event, size_t sz)
{
  add_fields_to_event_stride(ctx,mod,event,tbl_event,0,sz);
}


void
fprintf_event(FILE *fp, const orchids_t *ctx, const event_t *event)
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
  for ( ; event; event = event->next) {
    fprintf(fp, "%4i | %-24s | ",
            event->field_id, ctx->global_fields[ event->field_id ].name);
    fprintf_issdl_val(fp, event->value);
  }
  fprintf(fp,
          "-----+"
          "--------------------------+"
          "---------------------------------\n");
}


void
free_event(event_t *event)
{
  event_t *e;

  while (event) {
    e = event->next;
    FREE_VAR(event->value);
    Xfree(event);
    event = e;
  }
}


void
post_event(orchids_t *ctx, mod_entry_t *sender, event_t *event)
{
  int ret;
  conditional_dissector_record_t *cond_dissect;

  DebugLog(DF_CORE, DS_INFO,
           "post_event() -- sender->mod_id = %i\n", sender->mod_id);

  /* XXX: add some stats here (correct/incorrect events, posts, injections) */
  sender->posts++;

  if (sender->dissect) {
    /* check for unconditional dissector */
    DebugLog(DF_CORE, DS_DEBUG, "Call unconditional sub-dissector.\n");
    ret = sender->dissect(ctx, sender->dissect_mod, event, NULL);
    /* Free and optionally inject event if sub-dissector fail */
    if (ret) {
      DebugLog(DF_CORE, DS_WARN, "dissection failed.\n");
      inject_event(ctx, event);
    }
  } else if (sender->sub_dissectors) {
    /* Check for conditional dissectors */
    DebugLog(DF_CORE, DS_DEBUG, "Resolve conditional dissector\n");
    /* XXX: Fix this / check if event is present */
    cond_dissect = hash_get(sender->sub_dissectors,
                            issdl_get_data(event->next->value),
                            issdl_get_data_len(event->next->value));
    if (cond_dissect) {
      DebugLog(DF_CORE, DS_DEBUG, "call found conditional sub-dissector.\n");
      ret = cond_dissect->dissect(ctx,
                                  cond_dissect->mod,
                                  event,
                                  cond_dissect->data);
      /* Free and optionally inject event if sub-dissector fail */
      if (ret) {
        DebugLog(DF_CORE, DS_DEBUG, "dissection failed.\n");
        inject_event(ctx, event);
      }
    } else {
      DebugLog(DF_CORE, DS_TRACE, "no sub-dissectors found. --> inject -->\n");
      inject_event(ctx, event);
    }
  } else {
    /* If no sub-dissectors is found, then inject into the analysis engine */
    DebugLog(DF_CORE, DS_TRACE, "--> Injection into analysis engine -->\n");
    inject_event(ctx, event);
  }
}

#ifndef ORCHIDS_DEMO

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
          diff_time.tv_sec * 1000 + diff_time.tv_usec / 1000,
          diff_time.tv_usec % 1000L);

  Timer_Sub(&diff_time, &ctx->postconfig_time, &ctx->preconfig_time);
  fprintf(fp, "  Post-config. time : %li.%03li ms\n",
          diff_time.tv_sec * 1000 + diff_time.tv_usec / 1000,
          diff_time.tv_usec % 1000L);

  Timer_Sub(&diff_time, &ctx->compil_time, &ctx->postconfig_time);
  fprintf(fp, " Rules compil. time : %li.%03li ms\n",
          diff_time.tv_sec * 1000 + diff_time.tv_usec / 1000,
          diff_time.tv_usec % 1000L);

  Timer_Sub(&diff_time, &ctx->postcompil_time, &ctx->compil_time);
  fprintf(fp, "  Post-compil. time : %li.%03li ms\n",
          diff_time.tv_sec * 1000 + diff_time.tv_usec / 1000,
          diff_time.tv_usec % 1000L);


#ifdef linux
  fprintf(fp,
          "- - - - - - - - - - + - - - - -[ "
          "linux specific"
          " ]- - - - - - - - - - - - - -\n");
  get_linux_process_info(&pinfo, getpid());
  fprintf_linux_process_summary(fp, &pinfo);
  fprintf(fp,
          "- - - - - - - - - - +"
          " - - - - - - - - - - - - - - - - - - - - - - - - - - - \n");
#endif /* linux */

  fprintf(fp, "current config file : '%s'\n", ctx->config_file);
  fprintf(fp, "     loaded modules : %i\n", ctx->loaded_modules);
  fprintf(fp, "current poll period : %li\n", ctx->poll_period.tv_sec);
  fprintf(fp, "  registered fields : %i\n", ctx->num_fields);
  fprintf(fp, "    injected events : %u\n", ctx->events);
  fprintf(fp, "      active events : %u\n", ctx->active_events);
  fprintf(fp, "     rule instances : %u\n", ctx->rule_instances);
  fprintf(fp, "    state instances : %u\n", ctx->state_instances);
  fprintf(fp, "     active threads : %u\n", ctx->threads);
  fprintf(fp, "     ovm stack size : %zd\n", ctx->ovm_stack->size);
  fprintf(fp, "            reports : %u\n", ctx->reports);
  fprintf(fp,
          "--------------------+"
          "-------------------------------------------------------\n");
}

#else /* #ifndef ORCHIDS_DEMO */

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
          ru.ru_utime.tv_sec, ru.ru_utime.tv_usec / 1000);
  fprintf(fp, "        System time : %li.%03li s\n",
          ru.ru_stime.tv_sec, ru.ru_stime.tv_usec / 1000);
  usage = (float)(ru.ru_stime.tv_sec + ru.ru_utime.tv_sec);
  usage += (float)((ru.ru_stime.tv_usec + ru.ru_utime.tv_usec) / 1000000);
  usage +=((ru.ru_stime.tv_usec + ru.ru_utime.tv_usec) % 1000000) / 1000000.0;
  fprintf(fp, "     Total CPU time : %5.3f s\n", usage);
  usage /= (float)(curr_time.tv_sec - ctx->start_time.tv_sec);
  usage *= 100.0;
  fprintf(fp, "   Average CPU load : %4.2f %%\n", usage);

  Timer_Sub(&diff_time, &ctx->preconfig_time, &ctx->start_time);
  fprintf(fp, "   Pre-config. time : %li.%03li ms\n",
          diff_time.tv_sec * 1000 + diff_time.tv_usec / 1000,
          diff_time.tv_usec % 1000);

  Timer_Sub(&diff_time, &ctx->postconfig_time, &ctx->preconfig_time);
  fprintf(fp, "  Post-config. time : %li.%03li ms\n",
          diff_time.tv_sec * 1000 + diff_time.tv_usec / 1000,
          diff_time.tv_usec % 1000);

  Timer_Sub(&diff_time, &ctx->compil_time, &ctx->postconfig_time);
  fprintf(fp, " Rules compil. time : %li.%03li ms\n",
          diff_time.tv_sec * 1000 + diff_time.tv_usec / 1000,
          diff_time.tv_usec % 1000);

  Timer_Sub(&diff_time, &ctx->postcompil_time, &ctx->compil_time);
  fprintf(fp, "  Post-compil. time : %li.%03li ms\n",
          diff_time.tv_sec * 1000 + diff_time.tv_usec / 1000,
          diff_time.tv_usec % 1000);


#ifdef linux
  fprintf(fp,
          "- - - - - - - - - - + - - - - -[ "
          "linux specific"
          " ]- - - - - - - - - - - - - -\n");
  get_linux_process_info(&pinfo, getpid());
  fprintf_linux_process_summary(fp, &pinfo);
  fprintf(fp, "- - - - - - - - - - + - - - - - - - - - - - - - - - - - - - - - - - - - - - \n");
#endif /* linux */

  fprintf(fp, "current config file : '%s'\n", ctx->config_file);
  fprintf(fp, "     loaded modules : %i\n", ctx->loaded_modules);
  fprintf(fp, "  registered fields : %i\n", ctx->num_fields);
  fprintf(fp, "    injected events : %lu\n", ctx->events);
  fprintf(fp, "      active events : %lu\n", ctx->active_events);
  fprintf(fp, "     rule instances : %lu\n", ctx->rule_instances);
  fprintf(fp, "    state instances : %lu\n", ctx->state_instances);
  fprintf(fp, "     active threads : %lu\n", ctx->threads);
  fprintf(fp, "            reports : %lu\n", ctx->reports);
  fprintf(fp,
          "--------------------+"
          "-------------------------------------------------------\n");
}

#endif /* ORCHIDS_DEMO */

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
                ctx->global_fields[base + field].name,
                str_issdltype(ctx->global_fields[base + field].type),
                ctx->global_fields[base + field].desc);
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


void
fprintf_state_env(FILE *fp, const state_instance_t *state)
{
  int i;

  for (i = 0; i < state->rule_instance->rule->dynamic_env_sz; ++i) {
    if (state->current_env[i]) {
      fprintf(fp, "    current_env[%i]: ($%s) ",
              i, state->rule_instance->rule->var_name[i]);
      fprintf_issdl_val(fp, state->current_env[i]);
    }
    else if (state->inherit_env && state->inherit_env[i]) {
      fprintf(fp, "  inherited_env[%i]: ($%s) ",
              i, state->rule_instance->rule->var_name[i]);
      fprintf_issdl_val(fp, state->inherit_env[i]);
    }
    else {
      fprintf(fp, "            env[%i]: ($%s) nil\n",
              i, state->rule_instance->rule->var_name[i]);
    }
  }
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
