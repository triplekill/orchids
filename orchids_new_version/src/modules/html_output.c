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
#include <netdb.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <limits.h>
#ifndef PATH_MAX
#define PATH_MAX 8192
/* PATH_MAX is undefined on systems without a limit of filename length,
   such as GNU/Hurd.  Also, defining _XOPEN_SOURCE on Linux will make
   PATH_MAX undefined.
*/
#endif

#include <fcntl.h>
#include <errno.h>
#include <unistd.h> // for link()

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

static int do_html_output(orchids_t *ctx, html_output_cfg_t  *cfg);

static void convert_filename(char *file);

static void fprintf_html_cfg_tree(FILE *fp, config_directive_t *root);

static void fprintf_html_cfg_tree_sub(FILE *fp,
				      config_directive_t *section,
				      int depth,
				      unsigned long mask,
				      int *counter);

static int generate_html_report(orchids_t *ctx,
				mod_entry_t *mod,
				void *data,
				state_instance_t *state);


static htmloutput_list_t  *htmloutput_list_head_g = NULL;
static htmloutput_list_t **htmloutput_list_tail_g = NULL;

static char *report_prefix_g;
static char *report_ext_g;


void html_output_add_menu_entry(orchids_t *ctx,
				mod_entry_t *mod,
				mod_htmloutput_t outputfunc)
{
  htmloutput_list_t *elmt;

  elmt = gc_base_malloc(ctx->gc_ctx, sizeof (htmloutput_list_t));
  elmt->output = outputfunc;
  elmt->mod = mod;
  elmt->next = NULL;

  if (htmloutput_list_tail_g!=NULL)
    {
      *htmloutput_list_tail_g = elmt;
    }
  else
    {
      htmloutput_list_head_g = elmt;
    }
  htmloutput_list_tail_g = &elmt->next;
}



int rtaction_html_cache_cleanup(orchids_t *ctx, heap_entry_t *he)
{
  DebugLog(DF_MOD, DS_TRACE,
           "HTML cache cleanup...\n");

  html_output_cache_cleanup(he->data);
  he->date.tv_sec += 60; /* XXX: use a customizable parameter */
  register_rtaction(ctx, he);
  return 0;
}


int rtaction_html_regeneration(orchids_t *ctx, heap_entry_t *he)
{
  DebugLog(DF_MOD, DS_TRACE,
           "HTML periodic regeneration...\n");

  html_output(ctx, he->data);
  he->date.tv_sec += 20; /* XXX: use a customizable parameter */
  register_rtaction(ctx, he);
  return 0;
}



void html_output_preconfig(orchids_t *ctx)
{
  int ret;

  /* XXX: TODO: Check that lockdir is accessible */

  /* if an old lockfile exists, try to remove it */
  ret = unlink(DEFAULT_OUTPUTHTML_LOCKFILE);
  if (ret == -1 && errno != ENOENT)
    {
      fprintf(stderr,
	      "unlink(pathname=\"%s\"): error %i: %s\n",
	      DEFAULT_OUTPUTHTML_LOCKFILE, errno, strerror(errno));
      exit(EXIT_FAILURE);
    }
}

void html_output(orchids_t *ctx, html_output_cfg_t *cfg)
{
#ifndef ORCHIDS_DEBUG
  pid_t pid;
#endif

  if (cfg->html_output_dir == NULL)
    {
      DebugLog(DF_MOD, DS_ERROR, "HTML Output isn't set. Aborting.\n");
      return;
    }

  /* Update parent process rusage now.
   * So, the child process can access to its parent rusage. */
  getrusage(RUSAGE_SELF, &ctx->ru);

#ifdef ORCHIDS_DEBUG
  do_html_output(ctx, cfg);
#else /* ORCHIDS_DEBUG */
  pid = fork();
  if (pid == 0)
    {
      int i;

      for (i = 3; i < 64; i++)
	close(i);
      do_html_output(ctx, cfg);
      exit(EXIT_SUCCESS);
    }
  else if (pid < 0)
    {
      perror("fork()");
      exit(EXIT_FAILURE);
    }
#endif /* ORCHIDS_DEBUG */
}


FILE *create_html_file(html_output_cfg_t  *cfg, char *file, int cache)
{
  char absolute_dir[PATH_MAX];
  char absolute_filepath[2*PATH_MAX+1];
  FILE *fp;

  if (realpath(cfg->html_output_dir, absolute_dir)==NULL)
    return NULL;
  snprintf(absolute_filepath, 2*PATH_MAX+1, "%s/%s", absolute_dir, file);

  if (cache)
    fp = fopen_cached(absolute_filepath);
  else
    fp = fopen(absolute_filepath, "w");
  return fp;
}


int cached_html_file(html_output_cfg_t  *cfg, char *file)
{
  char absolute_dir[PATH_MAX];
  char absolute_filepath[2*PATH_MAX+1];

  if (realpath(cfg->html_output_dir, absolute_dir)==NULL)
    return 0;
  snprintf(absolute_filepath, 2*PATH_MAX+1, "%s/%s", absolute_dir, file);

  return cached_file(absolute_filepath);
}


void fprintf_html_header(FILE *fp, char *title /*, int refresh_delay */)
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


void fprintf_html_trailer(FILE *fp)
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


int generate_htmlfile_hardlink(html_output_cfg_t  *cfg,
			       char *file, char *linkfile)
{
  char abs_dir[PATH_MAX];
  char abs_file[2*PATH_MAX+1];
  char abs_link[2*PATH_MAX+1];

  if (realpath(cfg->html_output_dir, abs_dir)==NULL)
    {
      DebugLog (DF_MOD, DS_ERROR, strerror(errno));
      return -1;
    }
  snprintf(abs_file, 2*PATH_MAX+1, "%s/%s", abs_dir, file);
  snprintf(abs_link, 2*PATH_MAX+1, "%s/%s", abs_dir, linkfile);

  DebugLog(DF_MOD, DS_INFO, "link \"%s\" to \"%s\"\n", file, linkfile);

  unlink(abs_link);
  return link(abs_file, abs_link);
}


static int generate_html_orchids_modules(orchids_t *ctx,
					 html_output_cfg_t  *cfg)
{
  FILE *fp;
  int m;
  int p = 0;
  char file[PATH_MAX];
  unsigned long ntpl, ntph;

  Timer_to_NTP(&ctx->cur_loop_time, ntph, ntpl);
  snprintf(file, sizeof (file), "orchids-modules-%08lx-%08lx.html", ntph, ntpl);
  fp = create_html_file(cfg, file, USE_CACHE);
  if (fp == CACHE_HIT || fp == NULL)
    {
      return generate_htmlfile_hardlink(cfg, file, "orchids-modules.html");
    }
  fprintf_html_header(fp, "Orchids Modules");

  fprintf(fp, "<center><h1>Loaded Orchids Modules<h1></center>\n");

  fprintf(fp, "<center>\n");
  fprintf(fp, "<table border=\"0\" cellpadding=\"3\" width=\"600\">\n");
  fprintf(fp, "<tr class=\"h\"><th colspan=\"4\">Loaded Modules</th></tr>\n");
  fprintf(fp,
          "<tr class=\"h\"><th>ID</th><th>Address</th>"
          "<th>Posted Events</th><th>Module Name</th></tr>\n");
  for (m = 0; m < ctx->loaded_modules; m++)
    {
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
  (void) fclose(fp);

  return generate_htmlfile_hardlink(cfg, file, "orchids-modules.html");
}

#ifdef GENERATE_HTML_FIELDS_OBSOLETE
static int generate_html_orchids_fields(orchids_t *ctx,
					html_output_cfg_t  *cfg)
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
  if (fp == CACHE_HIT || fp == NULL)
    {
      return generate_htmlfile_hardlink(cfg, file, "orchids-fields.html");
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

  for (m = 0, gfid = 0; m < ctx->loaded_modules; m++)
    {
      fprintf(fp,
	      "  <tr class=\"hh\"> <th colspan=\"5\"> Module %s </th> </tr>\n",
	      ctx->mods[m].mod->name);
      if (ctx->mods[m].num_fields)
	{
	  for (f = 0; f < ctx->mods[m].num_fields; f++, gfid++)
	    {
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
		      p, ctx->global_fields->fields[base + f].name,
		      p, ctx->global_fields->fields[base + f].type->name,
		      p, (ctx->global_fields->fields[base + f].desc ?
			  ctx->global_fields->fields[base + f].desc : ""));
	    }
	}
      else
	{
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
  (void) fclose(fp);

  return generate_htmlfile_hardlink(cfg, file, "orchids-fields.html");
}
#endif


#ifdef GENERATE_HTML_FUNCTIONS_OBSOLETE
static int generate_html_orchids_functions(orchids_t *ctx,
					   html_output_cfg_t  *cfg)
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
  if (fp == CACHE_HIT || fp == NULL)
    {
      return generate_htmlfile_hardlink(cfg, file, "orchids-functions.html");
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
  for (fid = 0; fid < ctx->vm_func_tbl_sz; fid++)
    {
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
  (void) fclose(fp);

  return generate_htmlfile_hardlink(cfg, file, "orchids-functions.html");
}
#endif

#ifdef GENERATE_HTML_DATATYPES_OBSOLETE
static int generate_html_orchids_datatypes(orchids_t *ctx,
					   html_output_cfg_t  *cfg)
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
  if (fp == CACHE_HIT || fp == NULL)
    {
      return generate_htmlfile_hardlink(cfg, file, "orchids-datatypes.html");
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
  for (tid = 0, t = issdlgettypes(); t->name ; tid++, t++)
    {
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
  (void) fclose(fp);

  return generate_htmlfile_hardlink(cfg, file, "orchids-datatypes.html");
}
#endif

static int generate_html_orchids_stats(orchids_t *ctx,
				       html_output_cfg_t  *cfg)
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
  if (fp == CACHE_HIT || fp == NULL)
    {
      return generate_htmlfile_hardlink(cfg, file, "orchids-stats.html");
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
  fprintf_linux_process_html_summary(fp, &pinfo, ctx->verbose);
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
          "<td class=\"v1\"> %zd </td> "
          "</tr>\n",
          ctx->global_fields->num_fields);
  fprintf(fp,
          "  <tr> "
          "<td class=\"e0\"> injected events </td> "
          "<td class=\"v0\"> %zu </td> "
          "</tr>\n",
          ctx->events);
  fprintf(fp,
          "  <tr> "
          "<td class=\"e0\"> threads </td> "
          "<td class=\"v0\"> %zu </td> "
          "</tr>\n",
          ctx->thread_queue->nelts);
  fprintf(fp,
          "  <tr> "
          "<td class=\"e1\"> reports </td> "
          "<td class=\"v1\"> %zu </td> "
          "</tr>\n",
          ctx->reports);
  fprintf(fp,
          "  <tr> "
          "<td class=\"e0\"> ovm stack size </td> "
          "<td class=\"v0\"> %i </td> "
          "</tr>\n",
          ctx->ovm_stack->n);
#ifdef GENERATE_HTML_FUNCTIONS_OBSOLETE
  fprintf(fp,
          "  <tr> "
          "<td class=\"e1\"> registered functions </td> "
          "<td class=\"v1\"> %u </td> "
          "</tr>\n",
          ctx->vm_func_tbl_sz);
#endif

  fprintf(fp, "</table>\n");
  fprintf(fp, "</center>\n");

  fprintf_html_trailer(fp);
  (void) fclose(fp);

  return generate_htmlfile_hardlink(cfg, file, "orchids-stats.html");
}


static int generate_html_rule_list(orchids_t *ctx, html_output_cfg_t  *cfg)
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
  if (fp == CACHE_HIT || fp == NULL)
    {
      return generate_htmlfile_hardlink(cfg, file, "orchids-rules.html");
    }

  fprintf_html_header(fp, "Orchids rules");

  fprintf(fp, "<center><h1>Orchids rules<h1></center>\n");

  if (ctx->rule_compiler->first_rule == NULL) {
    /* This case should never happens, since Orchids doesn't start if
       these is no rule... Just in case... */
    fprintf(fp, "<center> No rules. </center>\n");
    fprintf_html_trailer(fp);
    return fclose(fp);
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
  for (r = ctx->rule_compiler->first_rule, i = 0; r!=NULL; r = r->next, i++)
    {
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
	      "<td class=\"v%i\"> %zu </td> "
	      "<td class=\"v%i\"> %zu </td> "
	      "<td class=\"v%i\"> %zu </td> "
	      "<td class=\"v%i\"> %i </td> "
	      "<td class=\"v%i\"> %zu </td> "
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
  (void) fclose(fp);

  return generate_htmlfile_hardlink(cfg, file, "orchids-rules.html");
}


static void fprintf_file(FILE *fp, const char *filename)
{
  FILE *in;
  int c;

  in = Xfopen(filename, "r");
  while ( (c = fgetc(in)) != EOF )
    fputc(c, fp);
  Xfclose(in);
}


static int generate_html_rules(orchids_t *ctx, html_output_cfg_t  *cfg)
{
  FILE *fp;
  size_t j;
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

  if (ctx->rule_compiler->first_rule == NULL)
    return -1;

  if (realpath(cfg->html_output_dir, absolute_dir)==NULL)
    {
      DebugLog(DF_MOD, DS_ERROR, strerror (errno));
      return -1;
    }

  for (r = ctx->rule_compiler->first_rule, i = 0; r!=NULL; r = r->next, i++)
    {
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

      if (fp != CACHE_HIT && fp != NULL)
	{
	  DebugLog(DF_MOD, DS_INFO, "dot file (%s).\n", absolute_filepath);

	  fprintf_rule_dot(fp, r);
	  (void) fclose(fp);

	}

    /* create html entry */
    snprintf(absolute_filepath, PATH_MAX, "%s.html", base_name);
    fp = fopen_cached(absolute_filepath);
    if (fp != CACHE_HIT && fp != NULL)
      {
	fprintf_html_header(fp, "Rule detail");

	fprintf(fp, "<center><h1>Rule %i: %s</h1><br/>\n",i , r->name);

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
		"<td class=\"v0\"> %zu </td> "
		"</tr>\n",
		r->state_nb);
	fprintf(fp,
		"  <tr> "
		"<td class=\"e1\"> Transitions </td> "
		"<td class=\"v1\"> %zu </td> "
		"</tr>\n",
		r->trans_nb);
	fprintf(fp,
		"  <tr> "
		"<td class=\"e0\"> Static env size </td> "
		"<td class=\"v0\"> %zu </td> "
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
	for (s = 0; s < r->state_nb; s++)
	  {
	    p = s % 2;
	    fprintf(fp,
		    "<tr> "
		    "<td class=\"e%i\"> %i </td> "
		    "<td class=\"v%i\"> %s </td> "
		    "<td class=\"v%i\"> %i </td> "
		    "<td class=\"v%i\"> %zu </td> ",
		    p, s,
		    p, r->state[s].name,
		    p, r->state[s].line,
		    p, r->state[s].trans_nb);

	    fprintf(fp, "<td class=\"v%i\"> ", p);
	    if (r->state[s].action)
	      {
		fprintf(fp, "<pre>");
		fprintf_bytecode_short(fp, r->state[s].action);
		fprintf(fp, "</pre> ");
	      }
	    else
	      {
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
	for (s = 0; s < r->state_nb; s++)
	  {
	    for ( t = 0 ; t < r->state[s].trans_nb ; t++)
	      {
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
		if (r->state[s].trans[t].eval_code!=NULL)
		  {
		    fprintf(fp, "<pre>");
		    fprintf_bytecode_short(fp, r->state[s].trans[t].eval_code);
		    fprintf(fp, "</pre> ");
		  }
		else
		  {
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
	for (j = 0 ; j < r->static_env_sz; j++)
	  {
	    p = j % 2;
	    fprintf(fp,
		    "<tr> <td class=\"e%i\"> %zu </td> <td class=\"v%i\"> %s </td>",
		    p, j,
		    p, str_issdltype(TYPE(r->static_env[j])));
	    fprintf(fp, "<td class=\"v%i\"> ", p);
	    fprintf_ovm_var(fp, r->static_env[j]);
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
	for (e = 0 ; e < r->dynamic_env_sz; e++)
	  {
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
	for (e = 0 ; e < r->sync_vars_sz; e++)
	  {
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
	(void) fclose(fp);
      }
    }
  return 0;
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


void generate_html_report_cb(orchids_t	*ctx,
			     mod_entry_t	*mod,
			     void		*data,
			     state_instance_t *state)
{
#ifndef ORCHIDS_DEBUG
  pid_t pid;
#endif


#ifdef ORCHIDS_DEBUG
  (void) generate_html_report(ctx, mod, data, state);
#else /* ORCHIDS_DEBUG */

  pid = fork();
  if (pid == 0)
    {
      int i;

      for (i = 3; i < 64; i++)
	(void) close(i);
      (void) generate_html_report(ctx, mod, data, state);
      exit(EXIT_SUCCESS);
    }
  else if (pid < 0)
    {
      perror("fork()");
      exit(EXIT_FAILURE);
    }
#endif /* ORCHIDS_DEBUG */
}

void fprint_html_s_len (FILE *fp, char *s, size_t len)
{
  size_t i;
  int c;

  for (i=0; i<len; i++)
    {
      c = s[i];
      if (isprint (c))
	switch (c)
	  {
	  case '&': fputs ("&amp;", fp); break;
	  case '"': fputs ("&quot;", fp); break;
	  case '\'': fputs ("&apos;", fp); break;
	  case '<': fputs ("&lt;", fp); break;
	  case '>': fputs ("&gt;", fp); break;
	  default:
	    fputc (c, fp);
	    break;
	  }
      else fputc ('.', fp);
    }
}

void fprint_html_s (FILE *fp, char *s)
{
  fprint_html_s_len (fp, s, strlen(s));
}

void fprintf_html_var(FILE *fp, ovm_var_t *val) /* imitated from fprintf_ovm_var() in lang.c */
{
  char asc_time[32]; /* for date conversions */
  struct hostent *hptr; /* for IPV4ADDR */
  char **pptr; /* for IPV4ADDR */
  char dst[INET6_ADDRSTRLEN];

  if (val==NULL)
    fprintf(fp, "(null)\n");
  /* display data */
  else switch (val->gc.type)
	 {
	 case T_INT:
	   fprintf(fp, "%li", INT(val));
	   break;
	 case T_UINT:
	   fprintf(fp, "%lu", UINT(val));
	   break;
	 case T_BSTR:
	   fprintf (fp, "\"");
	   fprint_html_s_len (fp, (char *)BSTR(val), BSTRLEN(val));
	   fprintf (fp, "\"");
	   break;
	 case T_VBSTR:
	   fprintf (fp, "\"");
	   fprint_html_s_len (fp, (char *)VBSTR(val), VBSTRLEN(val));
	   fprintf (fp, "\"");
	   break;
	 case T_STR:
	   fprintf (fp, "\"");
	   fprint_html_s_len (fp, STR(val), STRLEN(val));
	   fprintf (fp, "\"");
	   break;
	 case T_VSTR:
	   fprintf (fp, "\"");
	   fprint_html_s_len (fp, VSTR(val), VSTRLEN(val));
	   fprintf (fp, "\"");
	   break;
	 case T_CTIME:
	   strftime(asc_time, 32,
		    "%a %b %d %H:%M:%S %Y", gmtime(&CTIME(val)));
	   fprintf(fp, "%s", asc_time);
	   break;
	 case T_IPV4:
	   fprintf(fp, "%s", inet_ntoa(IPV4(val)));
	   hptr = gethostbyaddr((char *) &IPV4(val),
				sizeof (struct in_addr), AF_INET);
	   if (hptr == NULL) {
	     break;
	   } else if (hptr->h_name != NULL) {
	     fprintf(fp, " (");
	     fprint_html_s (fp, hptr->h_name);
	   } else {
	     break;
	   }
	   for (pptr = hptr->h_aliases; *pptr != NULL; pptr++)
	     {
	       fprintf(fp, ", ");
	       fprint_html_s (fp, *pptr);
	     }
	   fprintf(fp, ")");
	   break;
	 case T_IPV6:
           inet_ntop (AF_INET6, &IPV6(val), dst, sizeof(dst));
	   fprintf(fp, "%s", dst);
	   hptr = gethostbyaddr((char *) &IPV6(val),
				sizeof (struct in6_addr), AF_INET6);
	   if (hptr == NULL) {
	     break;
	   } else if (hptr->h_name != NULL) {
	     fprintf(fp, " (");
	     fprint_html_s (fp, hptr->h_name);
	   } else {
	     break;
	   }
	   for (pptr = hptr->h_aliases; *pptr != NULL; pptr++)
	     {
	       fprintf(fp, ", ");
	       fprint_html_s (fp, *pptr);
	     }
	   fprintf(fp, ")");
	   break;
	 case T_TIMEVAL:
	   strftime(asc_time, 32, "%a %b %d %H:%M:%S %Y",
		    gmtime(&TIMEVAL(val).tv_sec));
	   fprintf(fp, "%s +%li us",
		   asc_time, (long)TIMEVAL(val).tv_usec);
	   break;
	 case T_REGEX:
	   if (REGEXSTR(val)!=NULL)
	     {
	       fprintf (fp, "\"");
	       fprint_html_s (fp, REGEXSTR(val));
	       fprintf (fp, "\"");
	     }	       
	   break;
	 case T_FLOAT:
	   fprintf(fp, "%f", FLOAT(val));
	   break;
	 default:
	   fprintf(fp, "<type %i>", val->gc.type);
	   break;
	 }
}

static int generate_html_report(orchids_t		*ctx,
				mod_entry_t	*mod,
				void		*data,
				state_instance_t	*state)
{
  html_output_cfg_t  *cfg = data;
  char report[PATH_MAX];
  struct timeval tv;
  FILE *fp;
  rule_t *rule;
  ovm_var_t *env;
  ovm_var_t *val;
  int i, n, p;
  unsigned long ntpl, ntph;

  gettimeofday(&tv, NULL);
  Timer_to_NTP(&tv, ntph, ntpl);
  snprintf(report, sizeof (report),
           "%s/reports/%s%08lx-%08lx%s",
           cfg->html_output_dir, cfg->report_prefix, ntph, ntpl, ".html");

  fp = fopen(report, "w");
  if (fp==NULL)
    return -1;

  /* build reversed backtrace list */
  fprintf_html_header(fp, "Orchids report");
  fprintf(fp, "<center><h1>Report for rule: %s</h1></center>\n",
	  state->pid->rule->name);

  DebugLog(DF_ENG, DS_INFO, "Generating html report\n");

  fprintf(fp, "<table border=\"0\" cellpadding=\"3\" width=\"600\">\n");
  fprintf(fp, "  <tr class=\"h\"> <th colspan=\"2\"> Rule %s, state %s "
	  "</th> </tr>\n", state->pid->rule->name, state->q->name);
  fprintf(fp, "  <tr class=\"hh\"> <th> Variable </th> <th> Value </th> </tr>\n\n");
  rule = state->pid->rule;
  env = state->env;
  n = rule->dynamic_env_sz;
  p = 0;
  for (i=0; i<n; i++)
    {
      val = ovm_read_value (env, i);
      if (val!=NULL)
	{
	  fprintf (fp, "  <tr> <td class=\"e%i\">", p);
	  fprint_html_s (fp, rule->var_name[i]);
	  fprintf (fp, " <td class=\"v%i\"> ", p);
	  fprintf_html_var (fp, val);
	  fprintf (fp, " </td></tr>\n");
	  p ^= 1;
	}
    }
  fprintf(fp, "</table> <br>\n\n");
  fprintf(fp, "</center>\n");
  return fclose(fp);
}

static int generate_html_report_list(orchids_t *ctx, html_output_cfg_t  *cfg)
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
  if (stat(reportdir, &s)!=0)
    {
      DebugLog(DF_MOD, DS_ERROR, strerror(errno));
      return -1;
    }
  snprintf(file, sizeof (file), "orchids-reports-%08lx.html", s.st_mtime);

  fp = create_html_file(cfg, file, USE_CACHE);
  if (fp == CACHE_HIT || fp == NULL)
    {
      return generate_htmlfile_hardlink(cfg, file, "orchids-reports.html");
    }
  fprintf_html_header(fp, "Orchids reports");

  fprintf(fp, "<center><h1>Orchids reports<h1></center>\n");

  report_prefix_g = cfg->report_prefix;
  report_ext_g = cfg->report_ext;
  n = scandir(reportdir, &namelist, select_report, compar_report);

  if (n < 0)
    {
      DebugLog(DF_MOD, DS_ERROR, strerror(errno));
      return -1;
    }
  else
    {
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

      for (i = 0; i < n; i++)
	{
	  snprintf(reportfile, sizeof (reportfile), "%s/%s",
		   reportdir, namelist[i]->d_name);
	  if (stat(reportfile, &s)!=0)
	    {
	      DebugLog (DF_MOD, DS_ERROR, strerror(errno));
	    }
	  else
	    {
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
	    }
	  free(namelist[i]);
	}
      free(namelist);
      fprintf(fp, "</table>\n");
      fprintf(fp, "</center>\n");
    }

  fprintf_html_trailer(fp);
  (void) fclose(fp);

  return generate_htmlfile_hardlink(cfg, file, "orchids-reports.html");
}


/* XXX: Rewrite this */
static int generate_html_config(orchids_t *ctx, html_output_cfg_t  *cfg)
{
  FILE *fp;

  fp = create_html_file(cfg, "orchids-config.html", NO_CACHE);
  if (fp==NULL)
    return -1;

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
  return fclose(fp);
}


static void fprintf_html_cfg_tree(FILE *fp, config_directive_t *root)
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


static void fprintf_html_cfg_tree_sub(FILE *fp,
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
  for (s = 0; section!=NULL; section = section->next, s++)
    {
      if (section->next == NULL)
	{
	  last = 1;
	  mask |= 1 << depth;
	}
      fprintf(fp, "<p>");
      for (i = 0, d = 1; i < depth; i++, d <<= 1)
	{
	  if (d & mask)
	    {
	      fprintf(fp,
		      "<img src=\"icon_blank.png\" alt=\"_\" width=16 height=22>");
	    }
	  else
	    {
	      fprintf(fp,
		      "<img src=\"icon_vertline.png\" alt=\"|\" width=16 height=22>");
	    }
	}
      if (section->first_child!=NULL)
	{
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
      else
	{
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
static int generate_html_menu_timestamps(orchids_t *ctx,
					 html_output_cfg_t  *cfg)
{
  FILE *fp;
  unsigned long ntpl, ntph;
  htmloutput_list_t *m;

  fp = create_html_file(cfg, "orchids-menu.html", NO_CACHE);
  if (fp==NULL)
    return -1;

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
#ifdef GENERATE_HTML_FIELDS_OBSOLETE
  fprintf(fp,
          "<a href=\"orchids-fields-%08lx-%08lx.html\" "
          "target=\"main\">Fields</a><br/>\n",
          ntph, ntpl);
#endif
  fprintf(fp,
          "<a href=\"orchids-stats.html\" "
          "target=\"main\">Statistics</a><br/>\n");

  Timer_to_NTP(&ctx->last_rule_act, ntph, ntpl);
  fprintf(fp,
          "<a href=\"orchids-rules-%08lx-%08lx.html\" "
          "target=\"main\">Rules</a><br/>\n",
          ntph, ntpl);

  fprintf(fp, "<a href=\"orchids-reports.html\" target=\"main\">Reports</a><br/>\n");

  /* execute registered outputs */
  if (htmloutput_list_head_g) {
    fprintf(fp, "<hr/>\n");
    fprintf(fp, "Modules:<br/>\n");
    for (m = htmloutput_list_head_g; m!=NULL; m = m->next)
      (void) (*m->output) (ctx, m->mod, fp);
  }

  fprintf(fp, "<hr/>\n");
  fprintf(fp, "<a href=\"index-help.html\" target=\"main\">Help</a><br/>\n");
  fprintf(fp, "<a href=\"index-about.html\" target=\"main\">About</a><br/>\n");
  fprintf(fp, "</center>\n");
  fprintf(fp, "</body>\n");
  fprintf(fp, "</html>\n");

  return fclose(fp);
}
#endif


static int generate_html_menu(orchids_t *ctx, html_output_cfg_t  *cfg)
{
  FILE *fp;
  htmloutput_list_t *m;

  fp = create_html_file(cfg, "orchids-menu.html", NO_CACHE);
  if (fp==NULL)
    return -1;

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
#ifdef GENERATE_HTML_FIELDS_OBSOLETE
  fprintf(fp,
          "<a href=\"orchids-fields.html\" "
          "target=\"main\">Fields</a><br/>\n");
#endif
#ifdef GENERATE_HTML_FUNCTIONS_OBSOLETE
  fprintf(fp,
          "<a href=\"orchids-functions.html\" "
          "target=\"main\">Functions</a><br/>\n");
#endif
  fprintf(fp,
          "<a href=\"orchids-stats.html\" "
          "target=\"main\">Statistics</a><br/>\n");
#ifdef GENERATE_HTML_DATATYPES_OBSOLETE
  fprintf(fp,
          "<a href=\"orchids-datatypes.html\" "
          "target=\"main\">Data Types</a><br/>\n");
#endif
  fprintf(fp,
          "<a href=\"orchids-rules.html\" "
          "target=\"main\">Rules</a><br/>\n");

  fprintf(fp,
          "<a href=\"orchids-reports.html\" "
          "target=\"main\">Reports</a><br/>\n");

  /* Execute registered outputs */
  if (htmloutput_list_head_g!=NULL)
    {
      fprintf(fp, "<hr/>\n");
      fprintf(fp, "Modules:<br/>\n");
      for (m = htmloutput_list_head_g; m!=NULL; m = m->next)
	(void) (*m->output)(ctx, m->mod, fp, cfg);
    }

  fprintf(fp, "<hr/>\n");
  fprintf(fp, "<a href=\"index-help.html\" target=\"main\">Help</a><br/>\n");
  fprintf(fp, "<a href=\"index-about.html\" target=\"main\">About</a><br/>\n");
  fprintf(fp, "</center>\n");
  fprintf(fp, "</body>\n");
  fprintf(fp, "</html>\n");

  return fclose(fp);
}


static int do_html_output(orchids_t *ctx, html_output_cfg_t  *cfg)
{
  int fd;
  int ret = 0;

  /* Acquire lock or return */
  fd = open(DEFAULT_OUTPUTHTML_LOCKFILE, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
  if (fd<0)
    return fd;
  ret = Write_lock(fd, 0, SEEK_SET, 0);
  if (ret)
    {
      if (errno == EACCES || errno == EAGAIN)
	{
	  DebugLog(DF_MOD, DS_NOTICE, "HTML already in rendering...\n");
	  return -1;
	}
      else
	{
	  perror("fcntl()");
	  exit(EXIT_FAILURE);
	}
    }

  ret |= generate_html_orchids_modules(ctx, cfg);
#ifdef GENERATE_HTML_FIELDS_OBSOLETE
  ret |= generate_html_orchids_fields(ctx, cfg);
#endif
#ifdef GENERATE_HTML_FUNCTIONS_OBSOLETE
  ret |= generate_html_orchids_functions(ctx, cfg);
#endif
  ret |= generate_html_orchids_stats(ctx, cfg);
#ifdef GENERATE_HTML_DATATYPES_OBSOLETE
  ret |= generate_html_orchids_datatypes(ctx, cfg);
#endif
  ret |= generate_html_rules(ctx, cfg);
  ret |= generate_html_rule_list(ctx, cfg);
  ret |= generate_html_report_list(ctx, cfg);
  ret |= generate_html_config(ctx, cfg);

  ret |= generate_html_menu(ctx, cfg);

  /* Lock will be released by child-process exit */
  /* but in DEBUG mode, no child is created. */
  ret |= close(fd);
  return ret;
}


static void convert_filename(char *file)
{
  int c;

  for (c = *file; c!=0; c = *++file)
    {
      if (!((c >= 'a' && c <= 'z') ||
	    (c >= 'A' && c <= 'Z') ||
	    (c >= '0' && c <= '9')))
	*file = '-';
    }
}


void html_output_cache_cleanup(html_output_cfg_t  *cfg)
{
  char base_dir[PATH_MAX];
  char dir[PATH_MAX];

  Xrealpath(cfg->html_output_dir, base_dir);

  cache_gc(base_dir, "orchids-modules-",  4, 128 * 1024, 24 * 3600);
#ifdef GENERATE_HTML_FIELDS_OBSOLETE
  cache_gc(base_dir, "orchids-fields-",   4, 512 * 1024, 24 * 3600);
#endif
  cache_gc(base_dir, "orchids-stats-",    4, 128 * 1024, 24 * 3600);
  cache_gc(base_dir, "orchids-rules-", 4, 128 * 1024, 24 * 3600);

  snprintf(dir, sizeof (dir), "%s/rules/", base_dir);
  cache_gc(dir, "orchids-rule-",     50000, 100 * 1024 * 1024, 24 * 3600);

  cache_gc(base_dir, "orchids-reports-", 4, 10 * 1024 * 1024, 48 * 3600);
}


void html_output_cache_flush(html_output_cfg_t  *cfg)
{
  char base_dir[PATH_MAX];
  char dir[2*PATH_MAX+2];

  Xrealpath(cfg->html_output_dir, base_dir);

  cache_gc(base_dir, "orchids-modules-",  0, 0, 0);
#ifdef GENERATE_HTML_FIELDS_OBSOLETE
  cache_gc(base_dir, "orchids-fields-",   0, 0, 0);
#endif
  cache_gc(base_dir, "orchids-stats-",    0, 0, 0);
  cache_gc(base_dir, "orchids-rules-", 0, 0, 0);

  snprintf(dir, sizeof (dir), "%s/rules/", base_dir);
  cache_gc(dir, "orchids-rule-", 0, 0, 0);

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
