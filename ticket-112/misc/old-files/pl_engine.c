/**
 ** @file pl_engine.c
 ** SWI-Prolog wrapper for Orchids.
 ** 
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 ** 
 ** @version 0.1.0
 ** 
 ** @date  Started on: Tue May 10 12:00:22 2005
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
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

int
pl_init(char *name)
{
  int plac;
  char *plav[10];

  plac = 0;
  plav[plac++] = name;
  plav[plac++] = "-nosignals";
  plav[plac++] = "-nodebug";
  plav[plac] = NULL;

  if ( !PL_initialise(plac, plav) ) {
    PL_cleanup(1);
    return (FALSE);
  }

  pl_execute("set_prolog_flag(verbose,silent). ");
  pl_execute("set_prolog_flag(generate_debug_info,false). ");
  pl_execute("set_prolog_flag(debug_on_error,false). ");
  pl_execute("set_prolog_flag(report_error,false). ");
  /* pl_execute("set_prolog_flag(abort_with_exception,false). "); */

  return (TRUE);
}

int
pl_execute(const char *goal)
{
  fid_t fid;
  term_t g;
  int retval;

  fid = PL_open_foreign_frame();
  g = PL_new_term_ref();

  if ( PL_chars_to_term(goal, g) )
    retval = PL_call(g, NULL);
  else
    retval = 0;

  PL_discard_foreign_frame(fid);

  return (retval);
}

char *
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
        return (val_str);
      }
    }
  }

  PL_discard_foreign_frame(fid);

  return (NULL);
}


int
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
