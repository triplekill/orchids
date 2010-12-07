/**
 ** @file html_output.h
 ** Definitions for html_output.c.
 ** 
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 ** 
 ** @version 0.1.0
 ** @ingroup output
 ** 
 ** @date  Started on: Fri Mar 12 10:44:59 2004
 ** @date Last update: Fri Aug  3 09:31:16 2007
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef HTML_OUTPUT_H
#define HTML_OUTPUT_H

#include "orchids.h"

#define HTMLSTATE_LNK_DOT         (1 << 0)
#define HTMLSTATE_LNK_EPS         (1 << 1)
#define HTMLSTATE_LNK_JPG_THUMB   (1 << 2)
#define HTMLSTATE_LNK_JPG_FULL    (1 << 3)
#define HTMLSTATE_LNK_PDF         (1 << 4)

#define HTMLSTATE_NEED_DOT(flag)      \
  (flag & (  HTMLSTATE_LNK_DOT        \
           | HTMLSTATE_LNK_EPS        \
           | HTMLSTATE_LNK_JPG_THUMB  \
           | HTMLSTATE_LNK_JPG_FULL   \
           | HTMLSTATE_LNK_PDF))

#define HTMLSTATE_NEED_EPS_FULL(flag) \
  (flag & (  HTMLSTATE_LNK_EPS        \
           | HTMLSTATE_LNK_JPG_FULL   \
           | HTMLSTATE_LNK_PDF))

#define HTMLSTATE_NEED_EPS_THUMB(flag) \
  (flag & (HTMLSTATE_LNK_JPG_THUMB)

#define HTMLSTATE_NEED_JPG_FULL(flag) \
  (flag & ( HTMLSTATE_LNK_JPG_FULL ))

#define HTMLSTATE_NEED_JPG_THUMB(flag) \
  (flag & ( HTMLSTATE_LNK_JPG_THUMB ))

#define HTMLSTATE_NEED_PDF_FULL(flag) \
  (flag & ( HTMLSTATE_LNK_PDF_FULL ))

#define HTMLSTATE_STATE_LIMIT 50

void
html_output_add_menu_entry(orchids_t *ctx, 
			   mod_entry_t *mod, 
			   mod_htmloutput_t outputfunc);


static int
rtaction_html_cache_cleanup(orchids_t *ctx, rtaction_t *e);


static int
rtaction_html_regeneration(orchids_t *ctx, rtaction_t *e);


/* XXX: Move this in mod_htmlstate.c */
void
html_output_preconfig(orchids_t *ctx);


void
html_output(orchids_t *ctx);


FILE *
create_html_file(orchids_t *ctx, char *file, int cache);


int
cached_html_file(orchids_t *ctx, char *file);


void
fprintf_html_header(FILE *fp, char *title /*, int refresh_delay */);


void
fprintf_html_trailer(FILE *fp);


void
generate_htmlfile_hardlink(orchids_t *ctx, char *file, char *link);


static void
generate_html_orchids_modules(orchids_t *ctx);


static void
generate_html_orchids_fields(orchids_t *ctx);


static void
generate_html_orchids_functions(orchids_t *ctx);


static void
generate_html_orchids_datatypes(orchids_t *ctx);


static void
generate_html_orchids_stats(orchids_t *ctx);


static void
generate_html_rule_list(orchids_t *ctx);


static void
fprintf_file(FILE *fp, const char *filename);


static void
generate_html_rules(orchids_t *ctx);


static void
generate_html_rule_instances(orchids_t *ctx);


static void
generate_html_rule_instance_list(orchids_t *ctx);


static void
generate_html_thread_queue(orchids_t *ctx);


static void
generate_html_events(orchids_t *ctx);


static int
select_report(const struct dirent *d);


static int
compar_report(const struct dirent **a, const struct dirent **b);


static void
generate_html_report(orchids_t *ctx, const char *reportfile);


static void
generate_html_report_list(orchids_t *ctx);


static void
generate_html_config(orchids_t *ctx);


static void
fprintf_html_cfg_tree(FILE *fp, config_directive_t *root);


static void
fprintf_html_cfg_tree_sub(FILE *fp,
                          config_directive_t *section,
                          int depth,
                          unsigned long mask,
                          int *counter);


static void
generate_html_menu(orchids_t *ctx);


static void
do_html_output(orchids_t *ctx);


static void
convert_filename(char *file);


void
html_output_cache_cleanup(orchids_t *ctx);


void
html_output_cache_flush(orchids_t *ctx);






#endif /* HTML_OUTPUT_H */
