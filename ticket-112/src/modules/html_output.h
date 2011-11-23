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

typedef struct html_output_cfg_s html_output_cfg_t;

typedef int (*mod_htmloutput_t)(orchids_t *ctx,
				mod_entry_t *mod,
				FILE *menufp,
				html_output_cfg_t *cfg);


typedef struct htmloutput_list_s htmloutput_list_t;
struct htmloutput_list_s {
  mod_htmloutput_t output;
  mod_entry_t *mod;
  htmloutput_list_t *next;
};


struct html_output_cfg_s
{
  int enable_cache;
  int fork;

  int auto_refresh_delay;
  int page_generation_delay;
  int cache_cleanup_delay;
  int rule_media; /* flags for graphs */
  int rule_limit;
  int rule_state_limit;
  int rule_instance_limit;
  int rule_instence_state_limit;
  int thread_limit;
  int event_limit;

  char* html_output_dir;

  char* report_ext;
  char* report_prefix;
};



void
html_output_add_menu_entry(orchids_t *ctx,
			   mod_entry_t *mod,
			   mod_htmloutput_t outputfunc);


int
rtaction_html_cache_cleanup(orchids_t *ctx, rtaction_t *e);


int
rtaction_html_regeneration(orchids_t *ctx, rtaction_t *e);

void
html_output_preconfig(orchids_t *ctx);

void
html_output_postconfig(orchids_t *ctx, html_output_cfg_t  *cfg);

void
html_output(orchids_t *ctx, html_output_cfg_t  *cfg);

FILE *
create_html_file(html_output_cfg_t  *cfg, char *file, int cache);

int
cached_html_file(html_output_cfg_t  *cfg, char *file);


void
fprintf_html_header(FILE *fp, char *title /*, int refresh_delay */);


void
fprintf_html_trailer(FILE *fp);


void
generate_htmlfile_hardlink(html_output_cfg_t  *cfg, char *file, char *link);


void
generate_html_report_cb(orchids_t *ctx, mod_entry_t *mod, void *data, state_instance_t *state);

void
html_output_cache_cleanup(html_output_cfg_t  *cfg);


void
html_output_cache_flush(html_output_cfg_t  *cfg);






#endif /* HTML_OUTPUT_H */
