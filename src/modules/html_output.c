/**
 ** @file html_output.c
 ** HTML Output
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1.0
 ** @ingroup output
 **
 ** @date  Started on: Fri Mar 12 10:44:59 2004
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#define HTML_AUTOREFRESH 20

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <limits.h>

#include <fcntl.h>
#include <errno.h>

#include <string.h>
#include <dirent.h>

#ifdef linux
#include "linux_process_info.h"
#endif /* linux */

#include "orchids.h"

#include "evt_mgr.h"
#include "graph_output.h"
#include "orchids_api.h"
#include "file_cache.h"

#include "html_output.h"


static void
do_html_output(orchids_t *ctx, html_output_cfg_t  *cfg);

static void
convert_filename(char *file);

static void
fprintf_html_cfg_tree(FILE *fp, config_directive_t *root);

static void
fprintf_html_cfg_tree_sub(FILE *fp,
                          config_directive_t *section,
                          int depth,
                          unsigned long mask,
                          int *counter);

static void
generate_html_report(orchids_t		*ctx,
		     mod_entry_t	*mod,
		     void		*data,
		     state_instance_t	*state);


static htmloutput_list_t  *htmloutput_list_head_g = NULL;
static htmloutput_list_t **htmloutput_list_tail_g = NULL;

static char *report_prefix_g;
static char *report_ext_g;


void
html_output_add_menu_entry(orchids_t *ctx,
			   mod_entry_t *mod,
			   mod_htmloutput_t outputfunc)
{
  htmloutput_list_t *elmt;

  elmt = Xmalloc(sizeof (htmloutput_list_t));
  elmt->output = outputfunc;
  elmt->mod = mod;
  elmt->next = NULL;

  if (htmloutput_list_tail_g) {
    *htmloutput_list_tail_g = elmt;
  }
  else {
    htmloutput_list_head_g = elmt;
  }
  htmloutput_list_tail_g = &elmt->next;
}



int
rtaction_html_cache_cleanup(orchids_t *ctx, rtaction_t *e)
{
  DebugLog(DF_MOD, DS_TRACE,
           "HTML cache cleanup...\n");

  html_output_cache_cleanup(e->data);

  e->date.tv_sec += 60; /* XXX: use a customizable parameter */

  register_rtaction(ctx, e);

  return (0);
}


int
rtaction_html_regeneration(orchids_t *ctx, rtaction_t *e)
{
  DebugLog(DF_MOD, DS_TRACE,
           "HTML periodic regeneration...\n");

  html_output(ctx, e->data);

  e->date.tv_sec += 20; /* XXX: use a customizable parameter */

  register_rtaction(ctx, e);

  return (0);
}



void
html_output_preconfig(orchids_t *ctx)
{
  int ret;

  /* XXX: TODO: Check that lockdir is accessible */

  /* if an old lockfile exists, try to remove it */
  ret = unlink(DEFAULT_OUTPUTHTML_LOCKFILE);
  if (ret == -1 && errno != ENOENT) {
    fprintf(stderr,
            "unlink(pathname=\"%s\"): error %i: %s\n",
            DEFAULT_OUTPUTHTML_LOCKFILE, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }

}

void
html_output(orchids_t *ctx, html_output_cfg_t *cfg)
{
#ifndef ORCHIDS_DEBUG
  pid_t pid;
#endif

  if (cfg->html_output_dir == NULL) {
    DebugLog(DF_MOD, DS_ERROR, "HTML Output isn't set. Aborting.\n");
    return ;
  }

  /* Update parent process rusage now.
   * So, the child process can access to its parent rusage. */
  getrusage(RUSAGE_SELF, &ctx->ru);

#ifdef ORCHIDS_DEBUG

  do_html_output(ctx, cfg);

#else /* ORCHIDS_DEBUG */

  pid = fork();
  if (pid == 0) {
    int i;
    for (i = 3; i < 64; i++)
      close(i);
    do_html_output(ctx, cfg);
    exit(EXIT_SUCCESS);
  } else if (pid < 0) {
    perror("fork()");
    exit(EXIT_FAILURE);
  }

#endif /* ORCHIDS_DEBUG */

}


FILE *
create_html_file(html_output_cfg_t  *cfg, char *file, int cache)
{
  char absolute_dir[PATH_MAX];
  char absolute_filepath[PATH_MAX];
  FILE *fp;

  Xrealpath(cfg->html_output_dir, absolute_dir);
  snprintf(absolute_filepath, PATH_MAX, "%s/%s", absolute_dir, file);

  if (cache) {
    fp = fopen_cached(absolute_filepath);
  }
  else {
    fp = Xfopen(absolute_filepath, "w");
  }

  return (fp);
}


int
cached_html_file(html_output_cfg_t  *cfg, char *file)
{
  char absolute_dir[PATH_MAX];
  char absolute_filepath[PATH_MAX];
  int ret;

  Xrealpath(cfg->html_output_dir, absolute_dir);
  snprintf(absolute_filepath, PATH_MAX, "%s/%s", absolute_dir, file);

  ret = cached_file(absolute_filepath);

  return (ret);
}


void
fprintf_html_header(FILE *fp, char *title /*, int refresh_delay */)
{
  fprintf(fp,
          "<!DOCTYPE html PUBLIC \"-//W3C//"
          "DTD HTML 4.01 Transitional//EN\">\n");
  fprintf(fp, "<html>\n");
  fprintf(fp, "<head>\n");
  fprintf(fp,
          "  <link rel=\"stylesheet\" "
                 "type=\"text/css\" "
                 "href=\"style.css\">\n");
  fprintf(fp, "  <title>%s</title>\n", title);
  fprintf(fp,
          "  <meta http-equiv=\"content-type\" "
                      "content=\"text/html; "
                      "charset=ISO-8859-1\">\n");
#ifndef USE_HTTP_11
  fprintf(fp, "  <meta http-equiv=\"pragma\" content=\"no-cache\">\n");
#else /* USE_HTTP_11 */
  fprintf(fp, "  <meta http-equiv=\"cache-control\" content=\"no-cache\">\n");
#endif /* USE_HTTP_11 */
  /* if (refresh_delay)*/
  fprintf(fp,
          "  <meta http-equiv=\"refresh\" content=\"%i\">\n",
          HTML_AUTOREFRESH);
  fprintf(fp, "</head>\n");
  fprintf(fp, "<body>\n");
}

/*

for http/1.0:  <meta http-equiv="pragma" content="no-cache">
for http/1.1:  <meta http-equiv="cache-control" content="no-cache">

auto-reload

<meta http-equiv="refresh" content="20">

*/


void
fprintf_html_trailer(FILE *fp)
{
  time_t t;
  char asc_time[32];

  t = time(NULL);
  strftime(asc_time, 32, "%a %b %d %H:%M:%S %Y",
           localtime(&t));

  fprintf(fp,
          "<br/> <p align=\"right\"> <small> "
          "Generated by "
          "<a href=\"http://www.lsv.ens-cachan.fr/orchids/\">Orchids</a>. "
          " %s </small> </p>\n", asc_time);
  fprintf(fp, "</body>\n");
  fprintf(fp, "</html>\n");
}


void
generate_htmlfile_hardlink(html_output_cfg_t  *cfg, char *file, char *link)
{
  char abs_dir[PATH_MAX];
  char abs_file[PATH_MAX];
  char abs_link[PATH_MAX];

  Xrealpath(cfg->html_output_dir, abs_dir);
  snprintf(abs_file, PATH_MAX, "%s/%s", abs_dir, file);
  snprintf(abs_link, PATH_MAX, "%s/%s", abs_dir, link);

  DebugLog(DF_MOD, DS_INFO, "link \"%s\" to \"%s\"\n", file, link);

  unlink(abs_link);

  Xlink(abs_file, abs_link);
}


static void
generate_html_orchids_modules(orchids_t *ctx, html_output_cfg_t  *cfg)
{
  FILE *fp;
  int m;
  int p = 0;
  char file[PATH_MAX];
  unsigned long ntpl, ntph;

  Timer_to_NTP(&ctx->cur_loop_time, ntph, ntpl);
  snprintf(file, sizeof (file), "orchids-modules-%08lx-%08lx.html", ntph, ntpl);
  fp = create_html_file(cfg, file, USE_CACHE);
  if ((fp == CACHE_HIT) || (fp == NULL)) {
    generate_htmlfile_hardlink(cfg, file, "orchids-modules.html");
    return ;
  }
  fprintf_html_header(fp, "Orchids Modules");

  fprintf(fp, "<center><h1>Loaded Orchids Modules<h1></center>\n");

  fprintf(fp, "<center>\n");
  fprintf(fp, "<table border=\"0\" cellpadding=\"3\" width=\"600\">\n");
  fprintf(fp, "<tr class=\"h\"><th colspan=\"4\">Loaded Modules</th></tr>\n");
  fprintf(fp,
          "<tr class=\"h\"><th>ID</th><th>Address</th>"
          "<th>Posted Events</th><th>Module Name</th></tr>\n");
  for (m = 0; m < ctx->loaded_modules; m++) {
    p = m % 2;
    fprintf(fp,
            "<tr><td class=\"e%i\">%d</td><td class=\"v%i\">0x%08lx</td>"
            "<td class=\"v%i\">%li</td><td class=\"v%i\">%s</td></tr>\n",
            p, m,
            p, (unsigned long) ctx->mods[m].mod,
            p, ctx->mods[m].posts,
	    p, ctx->mods[m].mod->name);
  }
  fprintf(fp, "</table>\n");
  fprintf(fp, "</center>\n");

  fprintf_html_trailer(fp);
  Xfclose(fp);

  generate_htmlfile_hardlink(cfg, file, "orchids-modules.html");
}


static void
generate_html_orchids_fields(orchids_t *ctx, html_output_cfg_t  *cfg)
{
  FILE *fp;
  int m;
  int p = 0;
  int gfid;
  int f;
  int base;
  char file[PATH_MAX];
  unsigned long ntpl, ntph;

  Timer_to_NTP(&ctx->start_time, ntph, ntpl);
  snprintf(file, sizeof (file), "orchids-fields-%08lx-%08lx.html", ntph, ntpl);
  fp = create_html_file(cfg, file, USE_CACHE);
  if ((fp == CACHE_HIT) || (fp == NULL)) {
    generate_htmlfile_hardlink(cfg, file, "orchids-fields.html");
    return ;
  }

  fprintf_html_header(fp, "Orchids Data Fields");

  fprintf(fp, "<center><h1>Orchids Data Fields<h1></center>\n");

  fprintf(fp, "<center>\n");
  fprintf(fp, "<table border=\"0\" cellpadding=\"3\" width=\"600\">\n");
  fprintf(fp,
          "  <tr class=\"h\">"
          "<th colspan=\"5\"> Registered Fields </th>"
          "</tr>\n");
  fprintf(fp,
          "  <tr class=\"h\"> "
          "<th> Glob ID </th> <th> Mod ID </th> "
          "<th> Field Name </th> <th> Type </th> "
          "<th> Description </th> </tr>\n\n");

  for (m = 0, gfid = 0; m < ctx->loaded_modules; m++) {
    fprintf(fp,
            "  <tr class=\"hh\"> <th colspan=\"5\"> Module %s </th> </tr>\n",
            ctx->mods[m].mod->name);
    if (ctx->mods[m].num_fields) {
      for (f = 0; f < ctx->mods[m].num_fields; f++, gfid++) {
        p = gfid % 2;
        base = ctx->mods[m].first_field_pos;
        fprintf(fp,
                "  <tr> "
                "<td class=\"e%i\"> %i </td> "
                "<td class=\"e%i\"> %i </td> "
                "<td class=\"v%i\"> %s </td> "
                "<td class=\"v%i\"> %s </td> "
                "<td class=\"v%i\"> %s </td> "
                "</tr>\n",
                p, gfid,
                p, f,
                p, ctx->global_fields[base + f].name,
                p, str_issdltype(ctx->global_fields[base + f].type),
                p, (ctx->global_fields[base + f].desc ?
                    ctx->global_fields[base + f].desc : ""));
      }
    } else {
      fprintf(fp,
              "  <tr class=\"v%i\"> "
              "<th colspan=\"5\"> No field. </th> "
              "</tr>\n",
              p);
    }
  }
  fprintf(fp, "</table>\n");
  fprintf(fp, "</center>\n");

  fprintf_html_trailer(fp);
  Xfclose(fp);

  generate_htmlfile_hardlink(cfg, file, "orchids-fields.html");
}


static void
generate_html_orchids_functions(orchids_t *ctx, html_output_cfg_t  *cfg)
{
  FILE *fp;
  int fid;
  int p = 0;
  char file[PATH_MAX];
  unsigned long ntpl, ntph;
  issdl_function_t *f;

  Timer_to_NTP(&ctx->start_time, ntph, ntpl);
  snprintf(file, sizeof (file),
           "orchids-functions-%08lx-%08lx.html",
           ntph, ntpl);
  fp = create_html_file(cfg, file, USE_CACHE);
  if ((fp == CACHE_HIT) || (fp == NULL)) {
    generate_htmlfile_hardlink(cfg, file, "orchids-functions.html");
    return ;
  }
  fprintf_html_header(fp, "Orchids Registered Language Functions");

  fprintf(fp, "<center><h1>Registered Language Functions<h1></center>\n");

  fprintf(fp, "<center>\n");
  fprintf(fp, "<table border=\"0\" cellpadding=\"3\" width=\"600\">\n");
  fprintf(fp, "<tr class=\"h\"><th colspan=\"5\">Functions</th></tr>\n");
  fprintf(fp,
          "<tr class=\"h\">"
          "<th>ID</th><th>Name</th><th>Address</th>"
          "<th>Arity</th><th>Description</th>"
          "</tr>\n");
  for (fid = 0; fid < ctx->vm_func_tbl_sz; fid++) {
    f = &ctx->vm_func_tbl[ fid ];
    p = fid % 2;
    fprintf(fp,
            "<tr>"
            "<td class=\"e%i\">%d</td>"
            "<td class=\"v%i\">%s</td>"
            "<td class=\"v%i\">0x%08lx</td>"
            "<td class=\"v%i\">%i</td>"
            "<td class=\"v%i\">%s</td>"
            "</tr>\n",
            p, fid,
            p, f->name,
            p, (unsigned long)f->func,
	    p, f->args_nb,
	    p, f->desc);
  }
  fprintf(fp, "</table>\n");
  fprintf(fp, "</center>\n");

  fprintf_html_trailer(fp);
  Xfclose(fp);

  generate_htmlfile_hardlink(cfg, file, "orchids-functions.html");
}


static void
generate_html_orchids_datatypes(orchids_t *ctx, html_output_cfg_t  *cfg)
{
  FILE *fp;
  int tid;
  int p = 0;
  char file[PATH_MAX];
  unsigned long ntpl, ntph;
  issdl_type_t *t;

  Timer_to_NTP(&ctx->start_time, ntph, ntpl);
  snprintf(file, sizeof (file),
           "orchids-datatypes-%08lx-%08lx.html",
           ntph, ntpl);
  fp = create_html_file(cfg, file, USE_CACHE);
  if ((fp == CACHE_HIT) || (fp == NULL)) {
    generate_htmlfile_hardlink(cfg, file, "orchids-datatypes.html");
    return ;
  }
  fprintf_html_header(fp, "Orchids Registered Data Types");

  fprintf(fp, "<center><h1>Registered Data Types<h1></center>\n");

  fprintf(fp, "<center>\n");
  fprintf(fp, "<table border=\"0\" cellpadding=\"3\" width=\"600\">\n");
  fprintf(fp, "<tr class=\"h\"><th colspan=\"3\">Data Types</th></tr>\n");
  fprintf(fp,
          "<tr class=\"h\">"
          "<th>ID</th><th>Name</th><th>Description</th>"
          "</tr>\n");
  for (tid = 0, t = issdlgettypes(); t->name ; tid++, t++) {
    p = tid % 2;
    fprintf(fp,
            "<tr>"
            "<td class=\"e%i\">%d</td>"
            "<td class=\"v%i\">%s</td>"
            "<td class=\"v%i\">%s</td>"
            "</tr>\n",
            p, tid,
            p, t->name,
	    p, t->desc);
  }
  fprintf(fp, "</table>\n");
  fprintf(fp, "</center>\n");

  fprintf_html_trailer(fp);
  Xfclose(fp);

  generate_htmlfile_hardlink(cfg, file, "orchids-datatypes.html");
}


static void
generate_html_orchids_stats(orchids_t *ctx, html_output_cfg_t  *cfg)
{
  FILE *fp;
  struct timeval diff_time;
  struct timeval curr_time;
/*   struct rusage ru; */
  float usage;
  struct utsname un;
#ifdef linux
  linux_process_info_t pinfo;
#endif
  char file[PATH_MAX];
  unsigned long ntpl, ntph;

  gettimeofday(&curr_time, NULL);

  Timer_to_NTP(&curr_time, ntph, ntpl);
  snprintf(file, sizeof (file), "orchids-stats-%08lx-%08lx.html", ntph, ntpl);
  /* cache never hit here... just add USE_CACHE for backlogging statistics */
  fp = create_html_file(cfg, file, USE_CACHE);
  if ((fp == CACHE_HIT) || (fp == NULL)) {
    generate_htmlfile_hardlink(cfg, file, "orchids-stats.html");
    return ;
  }

  fprintf_html_header(fp, "Orchids statistics");

  fprintf(fp, "<center><h1>Orchids statistics<h1></center>\n");

  fprintf(fp, "<center>\n");
  fprintf(fp, "<table border=\"0\" cellpadding=\"3\" width=\"600\">\n");
  fprintf(fp, "  <tr class=\"h\"> <th colspan=\"2\"> Statistics </th> </tr>\n");
  fprintf(fp,
          "  <tr class=\"hh\"> <th> Property </th> <th> Value </th> </tr>\n\n");

  Xuname(&un);
  fprintf(fp,
          "  <tr> "
          "<td class=\"e0\"> System </td> "
          "<td class=\"v0\"> %s %s %s %s %s </td> "
          "</tr>\n",
          un.sysname, un.nodename, un.release, un.version, un.machine);

  fprintf(fp,
          "  <tr> <td class=\"e1\"> Real uptime </td> <td class=\"v1\"> %lis (",
          curr_time.tv_sec - ctx->start_time.tv_sec);
  fprintf_uptime(fp, curr_time.tv_sec - ctx->start_time.tv_sec);
  fprintf(fp, ") </td> </tr>\n");

  fprintf(fp,
          "  <tr> "
          "<td class=\"e0\"> User time </td> "
          "<td class=\"v0\"> %li.%03li s </td> "
          "</tr>\n",
          ctx->ru.ru_utime.tv_sec, ctx->ru.ru_utime.tv_usec / 1000L);

  fprintf(fp,
          "  <tr> "
          "<td class=\"e1\"> System time </td> "
          "<td class=\"v1\"> %li.%03li s </td> "
          "</tr>\n",
          ctx->ru.ru_stime.tv_sec, ctx->ru.ru_stime.tv_usec / 1000L);

  usage = (float)(ctx->ru.ru_stime.tv_sec + ctx->ru.ru_utime.tv_sec);
  usage += (float)((ctx->ru.ru_stime.tv_usec + ctx->ru.ru_utime.tv_usec) / 1000000);
  usage +=((ctx->ru.ru_stime.tv_usec + ctx->ru.ru_utime.tv_usec) % 1000000) / 1000000.0;
  fprintf(fp,
          "  <tr> "
          "<td class=\"e0\"> Total CPU time </td> "
          "<td class=\"v0\"> %5.3f s </td> "
          "</tr>\n",
          usage);

  usage /= (float)(curr_time.tv_sec - ctx->start_time.tv_sec);
  usage *= 100.0;
  fprintf(fp,
          "  <tr> "
          "<td class=\"e1\"> Average CPU load </td> "
          "<td class=\"v1\"> %5.2f %% </td> "
          "</tr>\n",
          usage);

  Timer_Sub(&diff_time, &ctx->preconfig_time, &ctx->start_time);
  fprintf(fp,
          "  <tr> "
          "<td class=\"e0\"> Pre-configuration time </td> "
          "<td class=\"v0\"> %li.%03li ms </td> "
          "</tr>\n",
          diff_time.tv_sec * 1000 + diff_time.tv_usec / 1000,
          diff_time.tv_usec % 1000L);

  Timer_Sub(&diff_time, &ctx->postconfig_time, &ctx->preconfig_time);
  fprintf(fp,
          "  <tr> "
          "<td class=\"e1\"> Post-configuration time </td> "
          "<td class=\"v1\"> %li.%03li ms </td> "
          "</tr>\n",
          diff_time.tv_sec * 1000 + diff_time.tv_usec / 1000,
          diff_time.tv_usec % 1000L);

  Timer_Sub(&diff_time, &ctx->compil_time, &ctx->postconfig_time);
  fprintf(fp,
          "  <tr> "
          "<td class=\"e0\"> Rules compilation time </td> "
          "<td class=\"v0\"> %li.%03li ms </td> "
          "</tr>\n",
          diff_time.tv_sec * 1000 + diff_time.tv_usec / 1000,
          diff_time.tv_usec % 1000L);

  Timer_Sub(&diff_time, &ctx->postcompil_time, &ctx->compil_time);
  fprintf(fp,
          "  <tr> "
          "<td class=\"e1\"> Post compilation time </td> "
          "<td class=\"v1\"> %li.%03li ms </td> "
          "</tr>\n",
          diff_time.tv_sec * 1000 + diff_time.tv_usec / 1000,
          diff_time.tv_usec % 1000L);

#ifdef linux
  fprintf(fp,
          "  <tr class=\"hh\"> "
          "<th colspan=\"2\"> Linux specific statistics </th> "
          "</tr>\n");
  get_linux_process_info(&pinfo, ctx->pid);
  fprintf_linux_process_html_summary(fp, &pinfo);
  fprintf(fp,
          "  <tr class=\"hh\"> "
          "<th colspan=\"2\"> End of linux specific statistics </th> "
          "</tr>\n");
#endif /* linux */

  fprintf(fp,
          "  <tr> "
          "<td class=\"e0\"> current config file </td> "
          "<td class=\"v0\"> '%s' </td> "
          "</tr>\n",
          ctx->config_file);
  fprintf(fp,
          "  <tr> "
          "<td class=\"e1\"> loaded modules </td> "
          "<td class=\"v1\"> %i </td> "
          "</tr>\n",
          ctx->loaded_modules);
  fprintf(fp,
          "  <tr> "
          "<td class=\"e0\"> current poll period </td> "
          "<td class=\"v0\"> %li </td> "
          "</tr>\n",
          ctx->poll_period.tv_sec);
  fprintf(fp,
          "  <tr> "
          "<td class=\"e1\"> registed fields </td> "
          "<td class=\"v1\"> %i </td> "
          "</tr>\n",
          ctx->num_fields);
  fprintf(fp,
          "  <tr> "
          "<td class=\"e0\"> injected events </td> "
          "<td class=\"v0\"> %u </td> "
          "</tr>\n",
          ctx->events);
  fprintf(fp,
          "  <tr> "
          "<td class=\"e1\"> active events </td> "
          "<td class=\"v1\"> %u </td> "
          "</tr>\n",
          ctx->active_events);
  fprintf(fp,
          "  <tr> "
          "<td class=\"e0\"> rule instances </td> "
          "<td class=\"v0\"> %u </td> "
          "</tr>\n",
          ctx->rule_instances);
  fprintf(fp,
          "  <tr> "
          "<td class=\"e1\"> state instances </td> "
          "<td class=\"v1\"> %u </td> "
          "</tr>\n",
          ctx->state_instances);
  fprintf(fp,
          "  <tr> "
          "<td class=\"e0\"> active threads </td> "
          "<td class=\"v0\"> %u </td> "
          "</tr>\n",
          ctx->threads);
  fprintf(fp,
          "  <tr> "
          "<td class=\"e1\"> reports </td> "
          "<td class=\"v1\"> %u </td> "
          "</tr>\n",
          ctx->reports);
  fprintf(fp,
          "  <tr> "
          "<td class=\"e0\"> ovm stack size </td> "
          "<td class=\"v0\"> %zu </td> "
          "</tr>\n",
          ctx->ovm_stack->size);
  fprintf(fp,
          "  <tr> "
          "<td class=\"e1\"> registered functions </td> "
          "<td class=\"v1\"> %u </td> "
          "</tr>\n",
          ctx->vm_func_tbl_sz);

  fprintf(fp, "</table>\n");
  fprintf(fp, "</center>\n");

  fprintf_html_trailer(fp);
  Xfclose(fp);

  generate_htmlfile_hardlink(cfg, file, "orchids-stats.html");
}


static void
generate_html_rule_list(orchids_t *ctx, html_output_cfg_t  *cfg)
{
  FILE *fp;
  rule_t *r;
  int i;
  int p;
  char file[PATH_MAX];
  char rulefile[PATH_MAX];
  char basefile[PATH_MAX];
  unsigned long ntpl, ntph;

  Timer_to_NTP(&ctx->last_rule_act, ntph, ntpl);
  snprintf(file, sizeof (file), "orchids-rules-%08lx-%08lx.html", ntph, ntpl);
  fp = create_html_file(cfg, file, USE_CACHE);
  if ((fp == CACHE_HIT) || (fp == NULL)) {
    generate_htmlfile_hardlink(cfg, file, "orchids-rules.html");
    return ;
  }

  fprintf_html_header(fp, "Orchids rules");

  fprintf(fp, "<center><h1>Orchids rules<h1></center>\n");

  if (ctx->rule_compiler->first_rule == NULL) {
    /* This case should never happens, since Orchids doesn't start if
       these is no rule... Just in case... */
    fprintf(fp, "<center> No rules. </center>\n");
    fprintf_html_trailer(fp);
    Xfclose(fp);
    return ;
  }

  fprintf(fp, "<center>\n");
  fprintf(fp, "<table border=\"0\" cellpadding=\"3\" width=\"600\">\n");
  fprintf(fp,
          "  <tr class=\"h\"> <th colspan=\"9\"> Rule instances </th> </tr>\n");
  fprintf(fp,
          "  <tr class=\"hh\"> "
          "<th> ID </th> <th> Rule name </th> <th> States </th> "
          "<th> Trans. </th> <th> Static env. sz </th> "
          "<th> Dyn. env. sz </th> <th> Inst. </th> "
          "<th> File:line </th> <th> Detail </th> "
          "</tr>\n\n");
  for (r = ctx->rule_compiler->first_rule, i = 0; r; r = r->next, i++) {

    strncpy(rulefile, r->filename, sizeof (rulefile));
    rulefile[sizeof(rulefile)-1] = '\0';
    convert_filename(rulefile);
    snprintf(basefile, PATH_MAX,
	     "orchids-rule-%s-%s-%08lx",
	     r->name, rulefile, r->file_mtime);
    p = i % 2;
    fprintf(fp,
            "  <tr> "
            "<td class=\"e%i\"> <a href=\"rules/%s.html\"> %i </a> </td> "
            "<td class=\"v%i\"> <a href=\"rules/%s.html\"> %s </a> </td> "
            "<td class=\"v%i\"> %i </td> "
            "<td class=\"v%i\"> %i </td> "
            "<td class=\"v%i\"> %i </td> "
            "<td class=\"v%i\"> %i </td> "
            "<td class=\"v%i\"> %i </td> "
            "<td class=\"v%i\"> %s:%i </td> "
            "<td class=\"e%i\"> [<a href=\"rules/%s.html\">detail</a>]</td> "
            "</tr>\n",
            p, basefile, i,
            p, basefile, r->name,
            p, r->state_nb,
            p, r->trans_nb,
            p, r->static_env_sz,
            p, r->dynamic_env_sz,
            p, r->instances,
            p, r->filename, r->lineno,
            p, basefile);
  }
  fprintf(fp, "</table>\n");
  fprintf(fp, "</center>\n");

  fprintf_html_trailer(fp);
  Xfclose(fp);

  generate_htmlfile_hardlink(cfg, file, "orchids-rules.html");
}


static void
fprintf_file(FILE *fp, const char *filename)
{
  FILE *in;
  int c;

  in = Xfopen(filename, "r");
  while ( (c = fgetc(in)) != EOF ) {
    fputc(c, fp);
  }
}


static void
generate_html_rules(orchids_t *ctx, html_output_cfg_t  *cfg)
{
  FILE *fp;
  int i;
  int p;
  int e;
  int s;
  int t;
  rule_t *r;
  char absolute_dir[PATH_MAX];
  char absolute_filepath[PATH_MAX];
  char base_name[PATH_MAX];
  char basefile[PATH_MAX];
  char rulefile[PATH_MAX];
  char cmdline[2048];

  if (ctx->rule_compiler->first_rule == NULL) {
    return ;
  }

  for (r = ctx->rule_compiler->first_rule, i = 0; r; r = r->next, i++) {
    Xrealpath(cfg->html_output_dir, absolute_dir);
    strncpy(rulefile, r->filename, sizeof (rulefile));
    rulefile[sizeof(rulefile)-1] = '\0';
    convert_filename(rulefile);
    snprintf(basefile, PATH_MAX,
             "orchids-rule-%s-%s-%08lx",
             r->name, rulefile, r->file_mtime);
    snprintf(base_name, PATH_MAX,
             "%s/rules/%s",
             absolute_dir, basefile);
    snprintf(absolute_filepath, PATH_MAX, "%s.dot", base_name);
    fp = fopen_cached(absolute_filepath);

    if (fp != CACHE_HIT && fp != NULL) {
      DebugLog(DF_MOD, DS_INFO, "dot file (%s).\n", absolute_filepath);

      fprintf_rule_dot(fp, r);
      Xfclose(fp);

      /* convert in various formats (eps/pdf/jpeg) */
      snprintf(cmdline, sizeof(cmdline),
               COMMAND_PREFIX PATH_TO_DOT
               " -Tps -Grankdir=LR \"%s\" -o \"%s.eps\"",
               absolute_filepath, base_name);
      DebugLog(DF_MOD, DS_DEBUG, "executing cmdline: %s\n", cmdline);
      system(cmdline);

      snprintf(cmdline, sizeof(cmdline),
               COMMAND_PREFIX PATH_TO_DOT
               " -Tps -Grankdir=LR \"%s\" -Gsize=8,8 -o \"%s.thumb.eps\"",
               absolute_filepath, base_name);
      DebugLog(DF_MOD, DS_DEBUG, "executing cmdline: %s\n", cmdline);
      system(cmdline);

#ifndef ORCHIDS_DEMO
      snprintf(cmdline, sizeof(cmdline),
               COMMAND_PREFIX PATH_TO_EPSTOPDF " \"%s.eps\"", base_name);
      DebugLog(DF_MOD, DS_DEBUG, "executing cmdline: %s\n", cmdline);
      system(cmdline);
#endif

      snprintf(cmdline, sizeof(cmdline),
               COMMAND_PREFIX PATH_TO_CONVERT
               " \"%s.eps\" \"%s.jpg\"",
               base_name, base_name);
      DebugLog(DF_MOD, DS_DEBUG, "executing cmdline: %s\n", cmdline);
      system(cmdline);

      snprintf(cmdline, sizeof(cmdline),
               COMMAND_PREFIX PATH_TO_CONVERT
               " \"%s.thumb.eps\" \"%s.thumb.jpg\"",
               base_name, base_name);
      DebugLog(DF_MOD, DS_DEBUG, "executing cmdline: %s\n", cmdline);
      system(cmdline);
    }

    /* create html entry */
    snprintf(absolute_filepath, PATH_MAX, "%s.html", base_name);
    fp = fopen_cached(absolute_filepath);
    if (fp != CACHE_HIT && fp != NULL) {
      fprintf_html_header(fp, "Rule detail");

      fprintf(fp, "<center><h1>Rule %i: %s</h1><br/>\n",i , r->name);

      fprintf(fp,
              "<br/>Preview: <br/> <img src=\"%s.thumb.jpg\"><br/>\n",
              basefile);
      fprintf(fp,
              "[<a href=\"%s.jpg\">Full Size</a>] "
              "[<a href=\"%s.dot\">dot</a>] "
              "[<a href=\"%s.eps\">eps</a>] "
              "[<a href=\"%s.pdf\">pdf</a>]<br/><br/><br/>",
              basefile, basefile, basefile, basefile);

      fprintf(fp, "<table border=\"0\" cellpadding=\"3\" width=\"300\">\n");
      fprintf(fp,
              "  <tr class=\"h\">"
              "<th colspan=\"2\"> Informations  </th> "
              "</tr>\n");
      fprintf(fp,
              "  <tr class=\"hh\"><th>Property</th><th>Value</th></tr>\n\n");
      fprintf(fp,
              "  <tr> "
              "<td class=\"e0\"> States </td> "
              "<td class=\"v0\"> %i </td> "
              "</tr>\n",
              r->state_nb);
      fprintf(fp,
              "  <tr> "
              "<td class=\"e1\"> Transitions </td> "
              "<td class=\"v1\"> %i </td> "
              "</tr>\n",
              r->trans_nb);
      fprintf(fp,
              "  <tr> "
              "<td class=\"e0\"> Static env size </td> "
              "<td class=\"v0\"> %i </td> "
              "</tr>\n",
              r->static_env_sz);
      fprintf(fp,
              "  <tr> "
              "<td class=\"e1\"> Dynamic env size </td> "
              "<td class=\"v1\"> %i </td> "
              "</tr>\n",
              r->dynamic_env_sz);
      fprintf(fp,
              "  <tr> "
              "<td class=\"e0\"> Source </td> "
              "<td class=\"v0\"> %s:%i </td> "
              "</tr>\n",
              r->filename, r->lineno);
      fprintf(fp, "</table><br/><br/>\n");

      fprintf(fp, "<table border=\"0\" cellpadding=\"3\" width=\"600\">\n");
      fprintf(fp,
              "  <tr class=\"h\"> <th colspan=\"5\"> State list </th> </tr>\n");
      fprintf(fp,
              "  <tr class=\"hh\"> "
              "<th> ID </th> <th> Name </th> <th> Line </th> "
              "<th> Trans. </th> <th> Action code </th> "
              "</tr>\n\n");
      for (s = 0; s < r->state_nb; s++) {
        p = s % 2;
        fprintf(fp,
                "<tr> "
                "<td class=\"e%i\"> %i </td> "
                "<td class=\"v%i\"> %s </td> "
                "<td class=\"v%i\"> %i </td> "
                "<td class=\"v%i\"> %i </td> ",
                p, s,
                p, r->state[s].name,
                p, r->state[s].line,
                p, r->state[s].trans_nb);

        fprintf(fp, "<td class=\"v%i\"> ", p);
        if (r->state[s].action) {
          fprintf(fp, "<pre>");
          fprintf_bytecode_short(fp, r->state[s].action);
          fprintf(fp, "</pre> ");
        } else {
          fprintf(fp, "No action. ");
        }
        fprintf(fp, "</tr> </tr>\n");
      }
      fprintf(fp, "</table><br/><br/>\n");

      fprintf(fp, "<table border=\"0\" cellpadding=\"3\" width=\"600\">\n");
      fprintf(fp,
              "  <tr class=\"h\"> "
              "<th colspan=\"4\"> Transision list </th> "
              "</tr>\n");
      fprintf(fp,
              "  <tr class=\"hh\"> "
              "<th> ID </th> <th> Src </th> <th> Dst </th> "
              "<th> Evaluation bytecode </th> </tr>\n\n");
      for (s = 0; s < r->state_nb; s++) {
        for ( t = 0 ; t < r->state[s].trans_nb ; t++) {
          p = r->state[s].trans[t].global_id % 2;
          fprintf(fp,
                  "<tr> "
                  "<td class=\"e%i\"> %i </td> "
                  "<td class=\"v%i\"> %s (%i) </td> "
                  "<td class=\"v%i\"> %s (%i) </td>",
                  p, r->state[s].trans[t].global_id,
                  p, r->state[s].name, r->state[s].id,
                  p, r->state[s].trans[t].dest->name,
                     r->state[s].trans[t].dest->id);
          fprintf(fp, "<td class=\"v%i\"> ", p);
          if (r->state[s].trans[t].eval_code) {
            fprintf(fp, "<pre>");
            fprintf_bytecode_short(fp, r->state[s].trans[t].eval_code);
            fprintf(fp, "</pre> ");
          } else {
            fprintf(fp, "No evaluation code. ");
          }
          fprintf(fp, "</td> </tr>\n");
        }
      }
      fprintf(fp, "</table><br/><br/>\n");

      fprintf(fp, "<table border=\"0\" cellpadding=\"3\" width=\"600\">\n");
      fprintf(fp,
              "  <tr class=\"h\"> "
              "<th colspan=\"3\"> Static environment </th> "
              "</tr>\n");
      fprintf(fp,
              "  <tr class=\"hh\"> "
              "<th> ID </th> <th> Type </th> <th> Value </th> "
              "</tr>\n\n");
      for (e = 0 ; e < r->static_env_sz; e++) {
        p = e % 2;
        fprintf(fp,
                "<tr> <td class=\"e%i\"> %i </td> <td class=\"v%i\"> %s </td>",
                p, e,
                p, str_issdltype(r->static_env[e]->type));
        fprintf(fp, "<td class=\"v%i\"> ", p);
        fprintf_ovm_var(fp, r->static_env[e]);
        fprintf(fp, "</td> </tr>\n");
      }
      fprintf(fp, "</table><br/><br/>\n");

      fprintf(fp, "<table border=\"0\" cellpadding=\"3\" width=\"600\">\n");
      fprintf(fp,
              "  <tr class=\"h\"> "
              "<th colspan=\"2\"> Dynamic environment </th> "
              "</tr>\n");
      fprintf(fp,
              "  <tr class=\"hh\"> "
              "<th> ID </th> <th> Variable name </th> "
              "</tr>\n\n");
      for (e = 0 ; e < r->dynamic_env_sz; e++) {
        p = e % 2;
        fprintf(fp,
                "<tr> "
                "<td class=\"e%i\"> %i </td> "
                "<td class=\"v%i\"> $%s </td> "
                "</tr>\n",
                p, e,
                p, r->var_name[e]);
      }
      fprintf(fp, "</table><br/><br/>\n");

      fprintf(fp, "<table border=\"0\" cellpadding=\"3\" width=\"600\">\n");
      fprintf(fp,
              "  <tr class=\"h\"> "
              "<th colspan=\"3\"> Synchronization environment </th> "
              "</tr>\n");
      fprintf(fp,
              "  <tr class=\"hh\"> "
              "<th> ID </th> <th> Dyn Var ID </th> <th> Variable name </th> "
              "</tr>\n\n");
      for (e = 0 ; e < r->sync_vars_sz; e++) {
        p = e % 2;
        fprintf(fp,
                "<tr> "
                "<td class=\"e%i\"> %i </td> "
                "<td class=\"v%i\"> %i </td> "
                "<td class=\"v%i\"> $%s </td> "
                "</tr>\n",
                p, e,
                p, r->sync_vars[e],
                p, r->var_name[ r->sync_vars[e] ]);
      }
      fprintf(fp, "</table><br/><br/>\n");

      fprintf(fp, "<table border=\"0\" cellpadding=\"3\" width=\"600\">\n");
      fprintf(fp,
              "  <tr class=\"h\"> "
              "<th colspan=\"1\"> Original source file </th> "
              "</tr>\n");
      fprintf(fp, "    <td class=\"v0\"> <pre>\n");
      fprintf_file(fp, r->filename);
      fprintf(fp, "    </pre></td>\n");
      fprintf(fp, "</table><br/><br/>\n");

      fprintf(fp, "</center>\n");

      fprintf_html_trailer(fp);
      Xfclose(fp);
    }
  }
}


static void
generate_html_rule_instances(orchids_t *ctx, html_output_cfg_t  *cfg)
{
  FILE *fp;
  int i;
  rule_instance_t *r;
  char absolute_dir[PATH_MAX];
  char absolute_filepath[PATH_MAX];
  char base_name[PATH_MAX];
  char basefile[PATH_MAX];
  char cmdline[2048];
  unsigned long ntpch, ntpcl, ntpah, ntpal;

  if (ctx->first_rule_instance == NULL) {
    return ;
  }

  for (r = ctx->first_rule_instance, i = 0; r; r = r->next, i++) {
    Xrealpath(cfg->html_output_dir, absolute_dir);
    Timer_to_NTP(&r->new_creation_date, ntpch, ntpcl);
    Timer_to_NTP(&r->new_last_act, ntpah, ntpal);
    snprintf(basefile, sizeof (basefile),
             "orchids-ruleinst-%s-%08lx-%08lx-%08lx-%08lx",
             r->rule->name, ntpch, ntpcl, ntpah, ntpal);
    snprintf(base_name, sizeof (base_name),
             "%s/rule-insts/%s", absolute_dir, basefile);
    snprintf(absolute_filepath, sizeof (absolute_filepath),
             "%s.dot", base_name);
    fp = fopen_cached(absolute_filepath);
    if (fp != CACHE_HIT && fp != NULL) {
      DebugLog(DF_MOD, DS_INFO, "dot file (%s).\n", absolute_filepath);
      fprintf_rule_instance_dot(fp,
                                r,
                                DOT_RETRIGLIST,
                                ctx->new_qh,
                                HTMLSTATE_STATE_LIMIT);
      Xfclose(fp);

      /* Convert in various formats (eps/pdf/jpeg) */
      snprintf(cmdline, sizeof (cmdline),
               COMMAND_PREFIX PATH_TO_DOT
               " -Tps -Grankdir=LR "
               "-Gnodesep=0.05 -Granksep=0.05 \"%s\" "
               "-o \"%s.eps\"",
               absolute_filepath, base_name);
      DebugLog(DF_MOD, DS_DEBUG, "executing cmdline: %s\n", cmdline);
      system(cmdline);

      /* Smaller eps, for thumbs (for better image quality) */
      snprintf(cmdline, sizeof (cmdline),
               COMMAND_PREFIX PATH_TO_DOT
               " -Tps -Grankdir=LR "
               "-Gnodesep=0.05 -Granksep=0.05 -Gsize=8,8 \"%s\" "
               "-o \"%s.thumb.eps\"",
               absolute_filepath, base_name);
      DebugLog(DF_MOD, DS_DEBUG, "executing cmdline: %s\n", cmdline);
      system(cmdline);

#ifndef ORCHIDS_DEMO
      snprintf(cmdline, sizeof (cmdline),
               COMMAND_PREFIX PATH_TO_EPSTOPDF " \"%s.eps\"", base_name);
      DebugLog(DF_MOD, DS_DEBUG, "executing cmdline: %s\n", cmdline);
      system(cmdline);
#endif

      snprintf(cmdline, sizeof (cmdline),
               COMMAND_PREFIX PATH_TO_CONVERT " \"%s.eps\" \"%s.jpg\"",
               base_name, base_name);
      DebugLog(DF_MOD, DS_DEBUG, "executing cmdline: %s\n", cmdline);
      system(cmdline);

      snprintf(cmdline, sizeof (cmdline),
               COMMAND_PREFIX PATH_TO_CONVERT
               " \"%s.thumb.eps\" \"%s.thumb.jpg\"",
               base_name, base_name);
      DebugLog(DF_MOD, DS_DEBUG, "executing cmdline: %s\n", cmdline);
      system(cmdline);
    }

    /* create html entry */
    snprintf(absolute_filepath, sizeof (absolute_filepath),
             "%s/rule-insts/%s.html",
             absolute_dir, basefile);
/*     fp = Xfopen(absolute_filepath, "w"); */
    fp = fopen_cached(absolute_filepath);
    if (fp != CACHE_HIT && fp != NULL) {
    fprintf_html_header(fp, "Rule instance detail");

    fprintf(fp,
            "<center> Preview: <br/> <img src=\"%s.thumb.jpg\"><br/>\n",
            basefile);
    fprintf(fp, "[<a href=\"%s.jpg\">Full Size</a>] "
                "[<a href=\"%s.dot\">dot</a>] "
                "[<a href=\"%s.eps\">eps</a>] "
                "[<a href=\"%s.pdf\">pdf</a>]<br/></center>",
	    basefile, basefile, basefile, basefile);

    fprintf_html_trailer(fp);
    Xfclose(fp);
    }
  }
}


static void
generate_html_rule_instance_list(orchids_t *ctx, html_output_cfg_t  *cfg)
{
  char asc_time[32];
  rule_instance_t *r;
  int i;
  int p;
  FILE *fp;
  char file[PATH_MAX];
  char rulefile[PATH_MAX];
  char baserulefile[PATH_MAX];
  char basefile[PATH_MAX];
  unsigned long ntpl, ntph;
  unsigned long ntpah, ntpal;

  Timer_to_NTP(&ctx->last_ruleinst_act, ntph, ntpl);
  snprintf(file, sizeof (file),
           "orchids-ruleinsts-%08lx-%08lx.html",
           ntph, ntpl);

  fp = create_html_file(cfg, file, USE_CACHE);
  if ((fp == CACHE_HIT) || (fp == NULL)) {
    generate_htmlfile_hardlink(cfg, file, "orchids-ruleinsts.html");
    return ;
  }
  fprintf_html_header(fp, "Orchids rules instances");

  fprintf(fp, "<center><h1>Orchids rule instances<h1></center>\n");

  if (ctx->first_rule_instance == NULL) {
    fprintf(fp, "<center> No rule instance. </center>\n");
    fprintf_html_trailer(fp);
    Xfclose(fp);
    generate_htmlfile_hardlink(cfg, file, "orchids-ruleinsts.html");
    return ;
  }

  fprintf(fp, "<center>\n");
  fprintf(fp, "<table border=\"0\" cellpadding=\"3\" width=\"600\">\n");
  fprintf(fp,
          "  <tr class=\"h\"> "
          "<th colspan=\"7\"> Rule instances </th> "
          "</tr>\n");
  fprintf(fp,
          "  <tr class=\"hh\"> "
          "<th> ID </th> <th> Rule name </th> <th> Active states </th> "
          "<th> Active threads </th> <th> Max. Depth</th> "
          "<th> Creation date </th> <th> Detail </th> "
          "</tr>\n\n");
  for (r = ctx->first_rule_instance, i = 0; r; r = r->next, i++) {
    Timer_to_NTP(&r->new_creation_date, ntph, ntpl);
    Timer_to_NTP(&r->new_last_act, ntpah, ntpal);
    snprintf(basefile, PATH_MAX,
	     "orchids-ruleinst-%s-%08lx-%08lx-%08lx-%08lx",
	     r->rule->name, ntph, ntpl, ntpah, ntpal);
    p = i % 2;
    if (r->creation_date > 0) {
      strftime(asc_time, 32, "%a %b %d %H:%M:%S %Y",
               localtime(&r->creation_date));
    }
    else {
      strcpy(asc_time, "Initial instance");
    }
    strncpy(rulefile, r->rule->filename, sizeof (rulefile));
    rulefile[sizeof(rulefile)-1] = '\0';
    convert_filename(rulefile);
    snprintf(baserulefile, PATH_MAX,
	     "orchids-rule-%s-%s-%08lx",
	     r->rule->name, rulefile, r->rule->file_mtime);

    fprintf(fp,
            "  <tr> "
            "<td class=\"e%i\"> <a href=\"rule-insts/%s.html\"> %i </a> </td> "
            "<td class=\"v%i\"><a href=\"rules/%s.html\">%s</a></td>"
            "<td class=\"v%i\"> %i </td> "
            "<td class=\"v%i\"> %i </td> "
            "<td class=\"v%i\"> %i </td> "
            "<td class=\"v%i\"> %.32s (%li) </td> "
            "<td class=\"e%i\"> "
              "[<a href=\"rule-insts/%s.html\">detail</a>] "
            "</td> "
            "</tr>\n",
            p, basefile, i,
            p, baserulefile, r->rule->name,
            p, r->state_instances,
            p, r->threads,
            p, r->max_depth,
            p, asc_time, r->creation_date,
            p, basefile);
  }
  fprintf(fp, "</table>\n");
  fprintf(fp, "</center>\n");

  fprintf_html_trailer(fp);
  Xfclose(fp);

  generate_htmlfile_hardlink(cfg, file, "orchids-ruleinsts.html");
}


static void
generate_html_thread_queue(orchids_t *ctx, html_output_cfg_t  *cfg)
{
  FILE *fp;
  int i;
  int p;
  wait_thread_t *t;
  unsigned long ntph, ntpl;
  char file[PATH_MAX];

  Timer_to_NTP(&ctx->last_ruleinst_act, ntph, ntpl);
  snprintf(file, PATH_MAX, "orchids-thread-queue-%08lx-%08lx.html", ntph, ntpl);

  fp = create_html_file(cfg, file, USE_CACHE);
  if ((fp == CACHE_HIT) || (fp == NULL)) {
    generate_htmlfile_hardlink(cfg, file, "orchids-thread-queue.html");
    return ;
  }
  fprintf_html_header(fp, "Orchids thread queue");

  fprintf(fp, "<center><h1>Orchids thread queue<h1></center>\n");

  if (ctx->new_qh == NULL) {
    fprintf(fp, "<center> No active thread. </center>\n");
    fprintf_html_trailer(fp);
    Xfclose(fp);
    generate_htmlfile_hardlink(cfg, file, "orchids-thread-queue.html");
    return ;
  }

  fprintf(fp, "<center>\n");
  fprintf(fp, "<table border=\"0\" cellpadding=\"3\" width=\"600\">\n");
  fprintf(fp,
          "  <tr class=\"h\"> <th colspan=\"8\"> Thread queue </th> </tr>\n");
  fprintf(fp,
          "  <tr class=\"hh\"> "
          "<th> ID </th> <th> Rule name </th> <th> Transition (IDs) </th> "
          "<th> Rule ID </th> <th> Transition ID </th> <th> Pass </th> "
          "<th> Bump? </th> "
          "</tr>\n\n");
  for (i = 0, t = ctx->cur_retrig_qh; t; t = t->next, i++) {
    p = i % 2;
    fprintf(fp,
            "  <tr> "
            "<td class=\"e%i\"> %u </td> "
            "<td class=\"v%i\"> %s </td> "
            "<td class=\"v%i\"> %s(%i) -> %s(%i) </td> "
            "<td class=\"v%i\"> %i </td> "
            "<td class=\"v%i\"> %i </td> "
            "<td class=\"v%i\"> %i </td> "
            "<td class=\"v%i\"> %s </td>\n",
            p, i,
            p, t->state_instance->state->rule->name,
            p, t->state_instance->state->name, t->state_instance->state->id,
               t->trans->dest->name, t->trans->dest->id,
            p, t->state_instance->state->rule->id,
            p, t->trans->id,
            p, t->pass,
            p, (t->flags & THREAD_BUMP) ? "Bump!" : "");
  }
  fprintf(fp, "</table>\n");
  fprintf(fp, "</center>\n");

  fprintf_html_trailer(fp);
  Xfclose(fp);

  generate_htmlfile_hardlink(cfg, file, "orchids-thread-queue.html");
}


static void
generate_html_events(orchids_t *ctx, html_output_cfg_t  *cfg)
{
  FILE *fp;
  active_event_t *ae;
  event_t *e;
  int i;
  int f;
  int p;
  char *mono = NULL;
  char file[PATH_MAX];
  unsigned long ntph, ntpl;

  Timer_to_NTP(&ctx->last_evt_act, ntph, ntpl);
  snprintf(file, sizeof (file), "orchids-events-%08lx-%08lx.html", ntph, ntpl);

  fp = create_html_file(cfg, file, USE_CACHE);
  if ((fp == CACHE_HIT) || (fp == NULL)) {
    generate_htmlfile_hardlink(cfg, file, "orchids-events.html");
    return;
  }
  fprintf_html_header(fp, "Orchids active events");

  fprintf(fp, "<center><h1>Orchids active events<h1></center>\n");

  if (ctx->active_event_head == NULL) {
    fprintf(fp, "<center> No active event. </center>\n");
    fprintf_html_trailer(fp);
    Xfclose(fp);
    generate_htmlfile_hardlink(cfg, file, "orchids-events.html");
    return ;
  }

  fprintf(fp, "<center>\n");
  for (i = 0, ae = ctx->active_event_head; ae; ae = ae->next, i++) {
    fprintf(fp, "<table border=\"0\" cellpadding=\"3\" width=\"600\">\n");
    fprintf(fp,
            "  <tr class=\"h\"> "
            "<th colspan=\"5\"> Event %i (id:%p %i ref) </th> "
            "</tr>\n",
            i, ae->event, ae->refs);
    fprintf(fp,
            "  <tr class=\"hh\"> "
            "<th> FID </th> <th> Field </th> <th> Type </th> "
            "<th> Monotony </th> <th> Data content </th> "
            "</tr>\n\n");

    for (e = ae->event, f = 0; e; e = e->next, f++) {
      p = f % 2;
      switch (e->value->flags & MONOTONY_MASK) {
      case TYPE_UNKNOWN:
        mono = "unkn";
      break;
      case TYPE_MONO:
        mono = "mono";
        break;
      case TYPE_ANTI:
        mono = "anti";
        break;
      case TYPE_CONST:
        mono = "const";
        break;
      }

      fprintf(fp,
              "  <tr> "
              "<td class=\"e%i\"> %i </td> "
              "<td class=\"v%i\"> %s </td> "
              "<td class=\"v%i\"> %s </td> "
              "<td class=\"v%i\"> %s </td> ",
              p, e->field_id,
              p, ctx->global_fields[ e->field_id ].name,
              p, str_issdltype(e->value->type),
              p, mono);
      fprintf(fp, "<td class=\"v%i\"> ", p);
      fprintf_ovm_var(fp, e->value);
      fprintf(fp, " </td></tr>\n");
    }
    fprintf(fp, "</table> <br/>\n\n");
  }
  fprintf(fp, "</center>\n");

  fprintf_html_trailer(fp);
  Xfclose(fp);

  generate_htmlfile_hardlink(cfg, file, "orchids-events.html");
}


static int
select_report(
#ifndef BSD_SCANDIR
	const
#endif
	 struct dirent *d)
{
  return (!strncmp(report_prefix_g,
                   d->d_name,
                   strlen(report_prefix_g) ) &&
          !strcmp(report_ext_g,
                  d->d_name + strlen(d->d_name) - strlen(report_ext_g)) );
}


static int
compar_report(
#ifndef BSD_SCANDIR
	const struct dirent **a, const struct dirent **b
#define REPA (*a)
#define REPB (*b)
#else
	const void *ap, const void *bp
#define REPA (*(const struct dirent **)ap)
#define REPB (*(const struct dirent **)ap)
#endif
	)
{
  return (-strcmp(REPA->d_name,
                  REPB->d_name));
#undef REPA
#undef REPB
}


void
generate_html_report_cb(orchids_t	*ctx,
			mod_entry_t	*mod,
			void		*data,
			state_instance_t *state)
{
#ifndef ORCHIDS_DEBUG
  pid_t pid;
#endif


#ifdef ORCHIDS_DEBUG

  generate_html_report(ctx, mod, data, state);
#else /* ORCHIDS_DEBUG */

  pid = fork();
  if (pid == 0) {
    int i;
    for (i = 3; i < 64; i++)
      close(i);
    generate_html_report(ctx, mod, data, state);
    exit(EXIT_SUCCESS);
  } else if (pid < 0) {
    perror("fork()");
    exit(EXIT_FAILURE);
  }
#endif /* ORCHIDS_DEBUG */
}

static void
generate_html_report(orchids_t		*ctx,
		     mod_entry_t	*mod,
		     void		*data,
		     state_instance_t	*state)
{
  html_output_cfg_t  *cfg = data;
  char report[PATH_MAX];
  struct timeval tv;
  unsigned long ntph;
  unsigned long ntpl;
  FILE *fp;
  state_instance_t *report_events;
  event_t *e;
  int i;
  int p;
  int f;
  char *mono = NULL;

  gettimeofday(&tv, NULL);
  Timer_to_NTP(&tv, ntph, ntpl);
  snprintf(report, sizeof (report),
           "%s/reports/%s%08lx-%08lx%s",
           cfg->html_output_dir, cfg->report_prefix, ntph, ntpl, ".html");

  fp = Xfopen(report, "w");

  /* build reversed backtrace list */
  for (report_events = NULL; state; state = state->parent) {
    state->next_report_elmt = report_events;
    report_events = state;
  }

  fprintf_html_header(fp, "Orchids report");


  fprintf(fp, "<center><h1>Report for rule: %s</h1></center>\n",
	  report_events->rule_instance->rule->name);

  DebugLog(DF_ENG, DS_INFO, "Generating html report\n");

  /* walk report list */
  fprintf(fp, "<center>\n");
  for (i = 0 ; report_events ; report_events = report_events->next_report_elmt, i++) {

    if (report_events->event) {

      fprintf(fp, "<table border=\"0\" cellpadding=\"3\" width=\"600\">\n");
      fprintf(fp, "  <tr class=\"h\"> <th colspan=\"5\"> Event %i (id:%p %i ref) " \
	      "</th> </tr>\n", i, report_events->event->event, report_events->event->refs);
      fprintf(fp, "  <tr class=\"hh\"> <th> FID </th> <th> Field </th> <th> Type </th> " \
	      "<th> Monotony </th> <th> Data content </th> </tr>\n\n");

      for (e = report_events->event->event, f = 0; e; e = e->next, f++) {
        p = f % 2;
        switch (e->value->flags & MONOTONY_MASK) {
        case TYPE_UNKNOWN:
          mono = "unkn";
          break;
        case TYPE_MONO:
          mono = "mono";
          break;
        case TYPE_ANTI:
          mono = "anti";
          break;
        case TYPE_CONST:
          mono = "const";
          break;
        }

        fprintf(fp, "  <tr> <td class=\"e%i\"> %i </td> <td class=\"v%i\"> %s </td> " \
		"<td class=\"v%i\"> %s </td> <td class=\"v%i\"> %s </td> ",
                p, e->field_id,
                p, ctx->global_fields[ e->field_id ].name,
                p, str_issdltype(e->value->type),
                p, mono);
        fprintf(fp, "<td class=\"v%i\"> ", p);
        fprintf_ovm_var(fp, e->value);
        fprintf(fp, " </td></tr>\n");
      }
      fprintf(fp, "</table> <br>\n\n");
    }
  }
  fprintf(fp, "</center>\n");
  Xfclose(fp);
}

static void
generate_html_report_list(orchids_t *ctx, html_output_cfg_t  *cfg)
{
  FILE *fp;
  struct dirent **namelist;
  int n;
  int i;
  char reportdir[PATH_MAX];
  char reportfile[1024];
  char asc_time[32];
  struct stat s;
  char file[PATH_MAX];

  snprintf(reportdir, sizeof (reportdir), "%s/reports", cfg->html_output_dir);
  Xstat(reportdir, &s);
  snprintf(file, sizeof (file), "orchids-reports-%08lx.html", s.st_mtime);

  fp = create_html_file(cfg, file, USE_CACHE);
  if ((fp == CACHE_HIT) || (fp == NULL)) {
    generate_htmlfile_hardlink(cfg, file, "orchids-reports.html");
    return ;
  }
  fprintf_html_header(fp, "Orchids reports");

  fprintf(fp, "<center><h1>Orchids reports<h1></center>\n");


  report_prefix_g = cfg->report_prefix;
  report_ext_g = cfg->report_ext;
  n = scandir(reportdir, &namelist, select_report, compar_report);

  if (n < 0)
    perror("scandir()");
  else {
    fprintf(fp, "<center>\n");
    fprintf(fp, "<table border=\"0\" cellpadding=\"3\" width=\"600\">\n");
    fprintf(fp,
            "  <tr class=\"h\"> "
            "<th colspan=\"3\"> Report list </th> "
            "</tr>\n");
    fprintf(fp,
            "  <tr class=\"hh\"> "
            "<th> Report file name </th> <th> File size </th> "
            "<th> Creation time </th> "
            "</tr>\n\n");

    for (i = 0; i < n; i++) {
      snprintf(reportfile, sizeof (reportfile), "%s/%s",
               reportdir, namelist[i]->d_name);
      Xstat(reportfile, &s);
      strftime(asc_time, sizeof (asc_time),
               "%a %b %d %H:%M:%S %Y",
               localtime(&s.st_mtime));
      fprintf(fp,
              "  <tr> "
              "<td class=\"e%i\"> <a href=\"reports/%s\"> %s </a> </td> "
              "<td class=\"v%i\"> %jd </td> "
              "<td class=\"v%i\"> %s </td> "
              "</tr>\n",
              i % 2, namelist[i]->d_name, namelist[i]->d_name,
              i % 2, (intmax_t)s.st_size,
              i % 2, asc_time);

      free(namelist[i]);
    }
    free(namelist);
    fprintf(fp, "</table>\n");
    fprintf(fp, "</center>\n");

  }

  fprintf_html_trailer(fp);
  Xfclose(fp);

  generate_htmlfile_hardlink(cfg, file, "orchids-reports.html");
}


/* XXX: Rewrite this */
static void
generate_html_config(orchids_t *ctx, html_output_cfg_t  *cfg)
{
  FILE *fp;

  fp = create_html_file(cfg, "orchids-config.html", NO_CACHE);

  fprintf(fp,
          "<!DOCTYPE html PUBLIC \"-//W3C//"
          "DTD HTML 4.01 Transitional//EN\">\n");
  fprintf(fp, "<html>\n");
  fprintf(fp, "<head>\n");
  fprintf(fp,
          "<link rel=\"stylesheet\" type=\"text/css\" href=\"style.css\">\n");
  fprintf(fp, "<title>Orchids configuration tree</title>\n");
  fprintf(fp,
          "<meta http-equiv=\"content-type\" "
                   "content=\"text/html; charset=ISO-8859-1\">\n");
#ifndef USE_HTTP_11
  fprintf(fp, "<meta http-equiv=\"pragma\" content=\"no-cache\">\n");
#else /* USE_HTTP_11 */
  fprintf(fp, "<meta http-equiv=\"cache-control\" content=\"no-cache\">\n");
#endif /* USE_HTTP_11 */

  /* Add JavaScript tree functions */
  fputs("<script type=\"text/javascript\" src=\"tree.js\"></script>",fp);

  fprintf(fp, "</head>\n");
  fprintf(fp, "<body>\n");

  fprintf(fp, "<center><h1>Configuration tree</h1></center>\n");

  fprintf_html_cfg_tree(fp, ctx->cfg_tree);

  fprintf_html_trailer(fp);
  Xfclose(fp);
}


static void
fprintf_html_cfg_tree(FILE *fp, config_directive_t *root)
{
  int counter;

  fprintf(fp, "<div class=\"directory\">");
  fprintf(fp, "<h3>Config tree</h3>\n");
  fprintf(fp, "<div style=\"display: block;\">\n");
  fprintf(fp,
          "<p>"
          "[<a onclick=\"openAll()\">open all</a>]"
          "[<a onclick=\"closeAll()\">close all</a>]\n");
  counter = 0;
  fprintf_html_cfg_tree_sub(fp, root, 0, 0, &counter);
  fprintf(fp, "</div></div>\n");
}


static void
fprintf_html_cfg_tree_sub(FILE *fp,
                          config_directive_t *section,
                          int depth,
                          unsigned long mask,
                          int *counter)
{
  int i;
  int s;
  int last;
  unsigned long d;

  last = 0;
  for (s = 0; section; section = section->next, s++) {
    if (section->next == NULL) {
      last = 1;
      mask |= 1 << depth;
    }
    fprintf(fp, "<p>");
    for (i = 0, d = 1; i < depth; i++, d <<= 1) {
      if (d & mask) {
        fprintf(fp,
                "<img src=\"icon_blank.png\" alt=\"_\" width=16 height=22>");
      }
      else {
        fprintf(fp,
                "<img src=\"icon_vertline.png\" alt=\"|\" width=16 height=22>");
      }
    }
    if (section->first_child) {
      (*counter)++;
      fprintf(fp,
              "<img "
                "src=\"icon_p%snode.png\" "
                "alt=\"*\" "
                "width=16 height=22 "
                "onclick=\"toggleFolder('section-%i', this)\">"
              "<img "
                "src=\"icon_folder_closed.png\" "
                "alt=\"+\" "
                "width=24 height=22>"
              "Section <b style=\"font-size: 10pt;\">%s</b> %s</p>\n"
              "<div id=\"section-%i\">\n",
              last ? "last" : "",
              *counter,
              section->directive+1, section->args,
              *counter);

      fprintf_html_cfg_tree_sub(fp,
                                section->first_child, depth + 1,
                                mask,
                                counter);
      fprintf(fp, "</div>\n");
    }
    else {
      fprintf(fp,
              "<img src=\"icon_%snode.png\" alt=\"o\" width=16 height=22>"
              "<img src=\"icon_doc.png\" alt=\"*\" width=24 height=22>"
              " <b>%s</b> %s</p>\n",
              last ? "last" : "",
              section->directive, section->args);
    }
  }
}


#if 0
static void
generate_html_menu_timestamps(orchids_t *ctx, html_output_cfg_t  *cfg)
{
  FILE *fp;
  unsigned long ntpl, ntph;
  htmloutput_list_t *m;

  fp = create_html_file(cfg, "orchids-menu.html", NO_CACHE);
  fprintf_html_header(fp, "Orchids Menu");

  fprintf(fp, "<center>\n");
  fprintf(fp, "<h2>- Menu -</h2>\n");
  fprintf(fp, "<a href=\"index-main.html\" target=\"main\">Main</a><br/>\n");
  fprintf(fp, "<hr/>\n");
  fprintf(fp, "Core:<br/>\n");
  fprintf(fp, "<a href=\"orchids-config.html\" target=\"main\">Config tree</a><br/>\n");
  Timer_to_NTP(&ctx->start_time, ntph, ntpl);
  fprintf(fp,
          "<a href=\"orchids-modules-%08lx-%08lx.html\" "
          "target=\"main\">Modules</a><br/>\n",
          ntph, ntpl);
  fprintf(fp,
          "<a href=\"orchids-fields-%08lx-%08lx.html\" "
          "target=\"main\">Fields</a><br/>\n",
          ntph, ntpl);
  fprintf(fp,
          "<a href=\"orchids-stats.html\" "
          "target=\"main\">Statistics</a><br/>\n");

  Timer_to_NTP(&ctx->last_rule_act, ntph, ntpl);
  fprintf(fp,
          "<a href=\"orchids-rules-%08lx-%08lx.html\" "
          "target=\"main\">Rules</a><br/>\n",
          ntph, ntpl);

  Timer_to_NTP(&ctx->last_ruleinst_act, ntph, ntpl);
  fprintf(fp,
          "<a href=\"orchids-ruleinsts-%08lx-%08lx.html\" "
          "target=\"main\">Rule instances</a><br/>\n",
          ntph, ntpl);

  fprintf(fp,
          "<a href=\"orchids-thread-queue-%08lx-%08lx.html\" "
          "target=\"main\">Thread queue</a><br/>\n",
          ntph, ntpl);

  Timer_to_NTP(&ctx->last_evt_act, ntph, ntpl);
  fprintf(fp,
          "<a href=\"orchids-events-%08lx-%08lx.html\" "
          "target=\"main\">Active events</a><br/>\n",
          ntph, ntpl);

  fprintf(fp, "<a href=\"orchids-reports.html\" target=\"main\">Reports</a><br/>\n");

  /* execute registered outputs */
  if (htmloutput_list_head_g) {
    fprintf(fp, "<hr/>\n");
    fprintf(fp, "Modules:<br/>\n");
    for (m = htmloutput_list_head_g; m; m = m->next)
      m->output(ctx, m->mod, fp);
  }

  fprintf(fp, "<hr/>\n");
  fprintf(fp, "<a href=\"index-help.html\" target=\"main\">Help</a><br/>\n");
  fprintf(fp, "<a href=\"index-about.html\" target=\"main\">About</a><br/>\n");
  fprintf(fp, "</center>\n");
  fprintf(fp, "</body>\n");
  fprintf(fp, "</html>\n");

  Xfclose(fp);
}
#endif


static void
generate_html_menu(orchids_t *ctx, html_output_cfg_t  *cfg)
{
  FILE *fp;
  htmloutput_list_t *m;

  fp = create_html_file(cfg, "orchids-menu.html", NO_CACHE);

  fprintf_html_header(fp, "Orchids Menu");

  fprintf(fp, "<center>\n");
  fprintf(fp, "<h2>- Menu -</h2>\n");
  fprintf(fp, "<a href=\"index-main.html\" target=\"main\">Main</a><br/>\n");
  fprintf(fp, "<hr/>\n");
  fprintf(fp, "Core:<br/>\n");
  fprintf(fp,
          "<a href=\"orchids-config.html\" "
          "target=\"main\">Config tree</a><br/>\n");
  fprintf(fp,
          "<a href=\"orchids-modules.html\" "
          "target=\"main\">Modules</a><br/>\n");
  fprintf(fp,
          "<a href=\"orchids-fields.html\" "
          "target=\"main\">Fields</a><br/>\n");
  fprintf(fp,
          "<a href=\"orchids-functions.html\" "
          "target=\"main\">Functions</a><br/>\n");
  fprintf(fp,
          "<a href=\"orchids-stats.html\" "
          "target=\"main\">Statistics</a><br/>\n");
  fprintf(fp,
          "<a href=\"orchids-datatypes.html\" "
          "target=\"main\">Data Types</a><br/>\n");
  fprintf(fp,
          "<a href=\"orchids-rules.html\" "
          "target=\"main\">Rules</a><br/>\n");
  fprintf(fp,
          "<a href=\"orchids-ruleinsts.html\" "
          "target=\"main\">Rule instances</a><br/>\n");
  fprintf(fp,
          "<a href=\"orchids-thread-queue.html\" "
          "target=\"main\">Thread queue</a><br/>\n");
  fprintf(fp,
          "<a href=\"orchids-events.html\" "
          "target=\"main\">Active events</a><br/>\n");

  fprintf(fp,
          "<a href=\"orchids-reports.html\" "
          "target=\"main\">Reports</a><br/>\n");

  /* Execute registered outputs */
  if (htmloutput_list_head_g) {
    fprintf(fp, "<hr/>\n");
    fprintf(fp, "Modules:<br/>\n");
    for (m = htmloutput_list_head_g; m; m = m->next)
      m->output(ctx, m->mod, fp, cfg);
  }

  fprintf(fp, "<hr/>\n");
  fprintf(fp, "<a href=\"index-help.html\" target=\"main\">Help</a><br/>\n");
  fprintf(fp, "<a href=\"index-about.html\" target=\"main\">About</a><br/>\n");
  fprintf(fp, "</center>\n");
  fprintf(fp, "</body>\n");
  fprintf(fp, "</html>\n");

  Xfclose(fp);
}


static void
do_html_output(orchids_t *ctx, html_output_cfg_t  *cfg)
{
  int fd;
  int ret;

  /* Acquire lock or return */
  fd = Xopen(DEFAULT_OUTPUTHTML_LOCKFILE, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
  ret = Write_lock(fd, 0, SEEK_SET, 0);
  if (ret) {
    if (errno == EACCES || errno == EAGAIN) {
      DebugLog(DF_MOD, DS_NOTICE, "HTML already in rendering...\n");
      return ;
    }
    else {
      perror("fcntl()");
      exit(EXIT_FAILURE);
    }
  }

  generate_html_orchids_modules(ctx, cfg);
  generate_html_orchids_fields(ctx, cfg);
  generate_html_orchids_functions(ctx, cfg);
  generate_html_orchids_stats(ctx, cfg);
  generate_html_orchids_datatypes(ctx, cfg);
  generate_html_rules(ctx, cfg);
  generate_html_rule_list(ctx, cfg);
  generate_html_rule_instances(ctx, cfg);
  generate_html_rule_instance_list(ctx,cfg);
  generate_html_thread_queue(ctx, cfg);
  generate_html_events(ctx, cfg);
  generate_html_report_list(ctx, cfg);
  generate_html_config(ctx, cfg);

  generate_html_menu(ctx, cfg);

  /* Lock will be released by child-process exit */
  /* but in DEBUG mode, no child is created. */
  close(fd);
}


static void
convert_filename(char *file)
{
  int c;

  for (c = *file; c; c = *++file) {
    if (!((c >= 'a' && c <= 'z') ||
	  (c >= 'A' && c <= 'Z') ||
	  (c >= '0' && c <= '9')))
      *file = '-';
  }
}


void
html_output_cache_cleanup(html_output_cfg_t  *cfg)
{
  char base_dir[PATH_MAX];
  char dir[PATH_MAX];

  Xrealpath(cfg->html_output_dir, base_dir);

  cache_gc(base_dir, "orchids-modules-",  4, 128 * 1024, 24 * 3600);
  cache_gc(base_dir, "orchids-fields-",   4, 512 * 1024, 24 * 3600);
  cache_gc(base_dir, "orchids-stats-",    4, 128 * 1024, 24 * 3600);
  cache_gc(base_dir, "orchids-rules-", 4, 128 * 1024, 24 * 3600);

  snprintf(dir, sizeof (dir), "%s/rules/", base_dir);
  cache_gc(dir, "orchids-rule-",     50000, 100 * 1024 * 1024, 24 * 3600);

  cache_gc(base_dir, "orchids-ruleinsts-", 4, 1024 * 1024, 48 * 3600);

  snprintf(dir, sizeof (dir), "%s/rule-insts/", base_dir);
  cache_gc(dir, "orchids-ruleinst-",     50000, 100 * 1024 * 1024, 48 * 3600);

  cache_gc(base_dir, "orchids-thread-queue-", 4, 100 * 1024, 48 * 3600);
  cache_gc(base_dir, "orchids-events-", 4, 10 * 1024 * 1024, 48 * 3600);
  cache_gc(base_dir, "orchids-reports-", 4, 10 * 1024 * 1024, 48 * 3600);
}


void
html_output_cache_flush(html_output_cfg_t  *cfg)
{
  char base_dir[PATH_MAX];
  char dir[PATH_MAX];

  Xrealpath(cfg->html_output_dir, base_dir);

  cache_gc(base_dir, "orchids-modules-",  0, 0, 0);
  cache_gc(base_dir, "orchids-fields-",   0, 0, 0);
  cache_gc(base_dir, "orchids-stats-",    0, 0, 0);
  cache_gc(base_dir, "orchids-rules-", 0, 0, 0);

  snprintf(dir, sizeof (dir), "%s/rules/", base_dir);
  cache_gc(dir, "orchids-rule-", 0, 0, 0);

  cache_gc(base_dir, "orchids-ruleinsts-", 0, 0, 0);

  snprintf(dir, sizeof (dir), "%s/rule-insts/", base_dir);
  cache_gc(dir, "orchids-ruleinst-", 0, 0, 0);

  cache_gc(base_dir, "orchids-thread-queue-", 0, 0, 0);
  cache_gc(base_dir, "orchids-events-", 0, 0, 0);
  cache_gc(base_dir, "orchids-reports-", 0, 0, 0);
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
