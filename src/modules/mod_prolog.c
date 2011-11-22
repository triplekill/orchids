/**
 ** @file mod_prolog.c
 ** An embeded SWI-Prolog interpreter for Orchids.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 ** @date  Started on: Fri Feb  7 11:07:42 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

/*
Compile with:
plld -DORCHIDS_DEBUG -DENABLE_DEBUGLOG -DHAVE_SWIPROLOG \
     -v -g -embed-shared -I. -I./util -o modules/mod_prolog.so -nostate \
     modules/mod_prolog.c
*/

/*
  assume that the prolog bootfile is present in the module directory.
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_SWIPROLOG

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <SWI-Prolog.h>

#include "orchids.h"
#include "file_cache.h"

#include "mod_prolog.h"

input_module_t mod_prolog;

static int
pl_init(char *name, char *bootfile)
{
  int plac;
  char *plav[10];

  plac = 0;
  plav[plac++] = name;
  plav[plac++] = "-nosignals";
  plav[plac++] = "-nodebug";
  if (bootfile) {
    plav[plac++] = "-x";
    plav[plac++] = bootfile;
  }
  plav[plac] = NULL;

  putenv("SWI_HOME_DIR=/usr/lib/swi-prolog");

  if ( !PL_initialise(plac, plav) ) {
    PL_cleanup(1);
    return (FALSE);
  }

  pl_execute("set_prolog_flag(verbose,silent). ");
  pl_execute("set_prolog_flag(generate_debug_info,false). ");
  pl_execute("set_prolog_flag(debug_on_error,false). ");
  pl_execute("set_prolog_flag(report_error,false). ");
  pl_execute("set_prolog_flag(abort_with_exception,false). ");

  return (TRUE);
}

int
pl_execute(const char *goal)
{
  fid_t fid;
  term_t g;
  int retval;

  DebugLog(DF_MOD, DS_DEBUG, "prolog Executing '%s'\n", goal);

  fid = PL_open_foreign_frame();
  g = PL_new_term_ref();

  if ( PL_chars_to_term(goal, g) )
    retval = PL_call(g, NULL);
  else
    retval = 0;

  PL_discard_foreign_frame(fid);

  DebugLog(DF_MOD, DS_DEBUG, "prolog Executed '%s': %s\n", goal, retval ? "Yes" : "No");

  return (retval);
}


static char *
pl_execute_var(const char *goal_str, const char *var_name)
{
  fid_t fid;
  term_t goal_atom;
  term_t goal;
  term_t bindings;
  predicate_t pred_a2t;
  int ret;
  term_t list;
  term_t list_head;
  const char *eq_funct;
  term_t var;
  term_t val;
  int arity;
  atom_t atom;
  char *var_str;
  char *val_str;
  char *ret_str;

  fid = PL_open_foreign_frame();

  goal_atom = PL_new_term_refs(3);
  goal = goal_atom + 1;
  bindings = goal_atom + 2;

  pred_a2t = PL_predicate("atom_to_term", 3, "system");
  PL_put_atom_chars(goal_atom, goal_str);
  ret = PL_call_predicate(NULL, PL_Q_NORMAL, pred_a2t, goal_atom);
  if (ret == FALSE) {
    PL_discard_foreign_frame(fid);
    return (NULL);
  }

  ret = PL_call(goal, NULL);
  if (ret == FALSE) {
    PL_discard_foreign_frame(fid);
    return (NULL);
  }

  if ( PL_is_list(bindings) == FALSE) {
    PL_discard_foreign_frame(fid);
    return (NULL);
  }

  list_head = PL_new_term_ref();
  list = PL_copy_term_ref(bindings);
  var = PL_new_term_ref();
  val = PL_new_term_ref();

  while( PL_get_list(list, list_head, list) ) {

    PL_get_name_arity(list_head, &atom, &arity);
    eq_funct = PL_atom_chars(atom);

    if (PL_is_compound(list_head) == TRUE &&
        !strcmp("=", eq_funct) &&
        arity == 2) {
      PL_get_arg(1, list_head, var);
      PL_get_chars(var, &var_str, CVT_WRITE);
      if ( !strcmp(var_name, var_str)) {
        PL_get_arg(2, list_head, val);
        PL_get_chars(val, &val_str, CVT_WRITE);
        ret_str = strdup(val_str);
        PL_discard_foreign_frame(fid);
        return (ret_str);
      }
    }
  }

  PL_discard_foreign_frame(fid);

  return (NULL);
}


static int
set_prolog_io(const char *in, const char *out, const char *err)
{
  int ret;
  char pl_code[4096];

  snprintf(pl_code,
           sizeof (pl_code),
           "open('%s', read, StdIn), "
           "open('%s', append, StdOut), "
           "open('%s', append, StdErr), "
           "set_prolog_IO(StdIn, StdOut, StdErr). ",
           in, out, err);

  ret = pl_execute(pl_code);

  return (ret);
}


#if 0
int
pl_dump_dyndb(const char *file)
{
  int ret;
  char pl_code[4096];

  snprintf(pl_code,
           sizeof (pl_code),
           "open('%s', write,Fd), "
           "current_output(Curr_out), "
           "set_output(Fd), "
           "listing, "
           "set_output(Curr_out), "
           "close(Fd).",
           file);

  ret = pl_execute(pl_code);

  return (ret);
}
#endif


static int
pl_consult(const char *file)
{
  char pl_code[4096];
  int ret;

  snprintf(pl_code, sizeof (pl_code), "consult('%s'). ", file);

  ret = pl_execute(pl_code);

  return (ret);
}


static void
issdl_prolog(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *plq;
  ovm_var_t *var;
  ovm_var_t *val;
  char *ret;

  plq = stack_pop(ctx->ovm_stack);
  if (TYPE(plq) != T_STR) {
    DebugLog(DF_MOD, DS_ERROR, "parameter type error (%i)\n", TYPE(plq));
    return ;
  }

  var = stack_pop(ctx->ovm_stack);
  if (TYPE(var) != T_STR) {
    DebugLog(DF_MOD, DS_ERROR, "parameter type error (%i)\n", TYPE(var));
    return ;
  }

  ret = pl_execute_var(STR(plq), STR(var));
  if (ret) {
    val = ovm_str_new( strlen(ret) );
    memcpy(STR(val), ret, strlen(ret));
    Xfree(ret);
  }
  else {
    val = ovm_str_new( 2 );
    strcpy(STR(val), "No");
  }

  stack_push(ctx->ovm_stack, val);
}


static void *
prolog_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  prolog_cfg_t *cfg;
  char bootfile[PATH_MAX];

  DebugLog(DF_MOD, DS_INFO, "load() prolog@%p\n", &mod_prolog);

  cfg = Xzmalloc(sizeof (prolog_cfg_t));

  snprintf(bootfile, sizeof (bootfile), "%s/mod_prolog.prc", ctx->modules_dir);

  pl_init("orchids", bootfile);

#if 1
  set_prolog_io("/dev/null",
                LOCALSTATEDIR "/orchids/log/mod_prolog.stdout",
                LOCALSTATEDIR "/orchids/log/mod_prolog.stderr");
#else
  set_prolog_io("/dev/null", "/dev/null", "/dev/null");
#endif /* PREFIX_PATH */

  pl_execute("get_time(Time), "
             "convert_time(Time, TimeString), "
             "write(TimeString), "
             "write(': Orchids SWI Prolog module started.\n'), "
             "flush_output. ");

  /* register language functions */
  register_lang_function(ctx, issdl_prolog,
                         "prolog", 2, "query the prolog engine");

  /* if (have_module("htmloutput"))*/
  html_output_add_menu_entry(ctx, mod, prolog_htmloutput);

  gettimeofday(&cfg->last_db_update, NULL);

  return (cfg);
}

static void
execute_dir(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  pl_execute(dir->args);
}

static void
consult_plfile(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  int ret;
  prolog_cfg_t *cfg;

  DebugLog(DF_MOD, DS_INFO, "Consulting prolog file %s\n", dir->args);

  ret = pl_consult(dir->args);

  if (ret == FALSE) {
    DebugLog(DF_MOD, DS_ERROR, "Failed to consult prolog file %s\n", dir->args);
  }

  cfg = (prolog_cfg_t *)mod->config;
  gettimeofday(&cfg->last_db_update, NULL);
}

static mod_cfg_cmd_t prolog_config_commands[] =
{
  { "Consult", consult_plfile, "Consult a prolog file" },
  { "Execute", execute_dir, "Execute a prolog query at configuration time" },
  { NULL, NULL, NULL }
};

static int
prolog_htmloutput(orchids_t *ctx, mod_entry_t *mod, FILE *menufp, html_output_cfg_t *htmlcfg)
{
  FILE *fp;
  char pl_code[PATH_MAX];
  int ret;
  unsigned long ntpl, ntph;
  prolog_cfg_t *cfg;
  char bodyfile[PATH_MAX];
  char file[PATH_MAX];

  DebugLog(DF_MOD, DS_INFO, "Prolog HTML output\n");

  fprintf(menufp,
  	  "<a href=\"orchids-prolog.html\" "
          "target=\"main\">Prolog</a><br/>\n");

  cfg = (prolog_cfg_t *)mod->config;
  Timer_to_NTP(&cfg->last_db_update, ntph, ntpl);

  snprintf(bodyfile, sizeof (bodyfile),
  	   "orchids-prolog-%08lx-%08lx.html", ntph, ntpl);

  if (cached_html_file(htmlcfg, bodyfile)) {
    generate_htmlfile_hardlink(htmlcfg, bodyfile, "orchids-prolog.html");
    return (0);
  }

  /* generate header */
  fp = create_html_file(htmlcfg, bodyfile, NO_CACHE);
  fprintf_html_header(fp, "Orchids Prolog Database");
  fprintf(fp, "<center><h1>Orchids Prolog Database<h1></center>\n");
  fprintf(fp,
          "<center>"
          "<table border=\"0\" cellpadding=\"3\" width=\"600\">"
          "<tr> <td class=\"v1\">");
  Xfclose(fp);

  /* generate html prolog listing */
  snprintf(pl_code, sizeof (pl_code),
  	   "prolog_htmloutput("
  	     "'source-highlight -n -s prolog >> %s/%s'"
  	   ").",
  	   htmlcfg->html_output_dir, bodyfile);

  ret = pl_execute(pl_code);

  /* generate footer */
  snprintf(file, sizeof (file),
  	   "%s/%s", htmlcfg->html_output_dir, bodyfile);
  fp = Xfopen(file, "a");
  fprintf(fp, "</td></tr></table></center>\n");
  fprintf_html_trailer(fp);
  Xfclose(fp);

  generate_htmlfile_hardlink(htmlcfg, bodyfile, "orchids-prolog.html");

  return (0);
}

input_module_t mod_prolog = {
  MOD_MAGIC,                /* Magic number */
  ORCHIDS_VERSION,          /* Module version */
  "prolog",                 /* module name */
  "CeCILL2",                /* module license */
  NULL,                     /* module dependencies */
  prolog_config_commands,   /* module configuration commands,
                               for core config parser */
  prolog_preconfig,         /* called just after module registration */
  NULL             ,        /* called after all mods preconfig,
                               and after all module configuration*/
  NULL
};

#endif /* HAVE_SWIPROLOG */


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
