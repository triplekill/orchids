/**
 ** @file graph_output.c
 ** Rule output in the GraphViz dot format.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 1.0
 ** @ingroup output
 **
 ** @date  Started on: Fri May 23 12:18:40 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>

#include "orchids.h"

#include "graph_output.h"
#include "graph_output_priv.h"


void
fprintf_rule_dot(FILE *fp, rule_t *rule)
{
  int s;

  fprintf(fp, "\n/* Generated by orchids */\n\n");
  fprintf(fp, "digraph \"%s\" {\n", rule->name);

  /* List all states... */
  for (s = 0; s < rule->state_nb; s++) {
    int t;

    fprintf(fp, "  \"%s\" [ label=\"%s\" ]\n",
            rule->state[s].name, rule->state[s].name);

    /* ...and for each state, list all transitions */
    for (t = 0; t < rule->state[s].trans_nb; ++t) {
      fprintf(fp, "  \"%s\" -> \"%s\" [ label=\"t%i\" ]\n",
              rule->state[s].name,
              rule->state[s].trans[t].dest ?
              rule->state[s].trans[t].dest->name : NULL,
              rule->state[s].trans[t].global_id);
    }
  }
  fprintf(fp, "}\n");
}


static void
fprintf_state_instance_dot(FILE *fp, state_instance_t *state, int options, int limit)
{
  fprintf_state_instance_dot_sub(fp, state, options, &limit);
}


static void
fprintf_state_instance_dot_sub(FILE *fp, state_instance_t *state, int options, int *limit)
{
  /* Small hack: Use state pointer to identify state instance */
  fprintf(fp, "  \"%s.%p\" [ label=\"%s\" ]\n",
          state->state->name, (void *) state, state->state->name);

  if (*limit == 0)
    return ;
  else
    (void) *limit--;

  /* If the state has a parent, display transitions */
  if (state->parent) {
    fprintf(fp, "  \"%s.%p\" -> \"%s.%p\" [ label=\"%p\", URL=\"#%p\" ]\n",
            state->parent->state->name, (void *) state->parent,
            state->state->name, (void *) state, state->event, state->event);
  }

  if (state->first_child)
    fprintf_state_instance_dot_sub(fp, state->first_child, options, limit);

  if (state->next_sibling)
    fprintf_state_instance_dot_sub(fp, state->next_sibling, options, limit);
}


void
fprintf_rule_instance_dot(FILE *fp,
                          rule_instance_t *rule,
                          int options,
                          wait_thread_t *tq,
                          int limit)
{
  wait_thread_t *t;
  int local_id;
  int global_id;

  fprintf(fp, "\n/* Generated by orchids */\n\n");
  fprintf(fp, "digraph \"%s\" {\n", rule->rule->name);

  fprintf_state_instance_dot(fp, rule->first_state, options, limit);

  if (options & DOT_RETRIGLIST) {
    for (t = tq, local_id = 0, global_id = 0; t; t = t->next) {
      if (t->state_instance->rule_instance == rule) {
        fprintf(fp, "  \"%s.%p.%p\" [ style = \"invis\" ]\n",
                t->state_instance->state->name,
                (void *) t->state_instance,
                (void *) t);
        fprintf(fp, "  \"%s.%p\" -> \"%s.%p.%p\" [ color=\"#C0C0C0\", fontcolor=\"#C0C0C0\",label=\"(%i:%i)\"]\n",
                t->state_instance->state->name, (void *)t->state_instance,
                t->state_instance->state->name, (void *)t->state_instance,
                (void *) t, local_id, global_id);
        local_id++;
      }
      global_id++;
    }
  }

  fprintf(fp, "}\n");
}


/*
** Default configuration header.
** Build a graph display configuration structure to modify values
*/
#if 0
static void
fprintf_header(FILE *fp)
{
  fprintf(fp, "  graph [\n");
  fprintf(fp, "    fontsize = \"14\"\n");
  fprintf(fp, "    fontname = \"Times-Roman\"\n");
  fprintf(fp, "    fontcolor = \"black\"\n");
  fprintf(fp, "    color = \"black\"\n");
  fprintf(fp, "  ]\n");

  fprintf(fp, "  node [\n");
  fprintf(fp, "    fontsize = \"14\"\n");
  fprintf(fp, "    fontname = \"Times-Roman\"\n");
  fprintf(fp, "    fontcolor = \"black\"\n");
  fprintf(fp, "    shape = \"ellipse\"\n");
  fprintf(fp, "    color = \"black\"\n");
  fprintf(fp, "  ]\n");

  fprintf(fp, "  edge [\n");
  fprintf(fp, "    fontsize = \"14\"\n");
  fprintf(fp, "    fontname = \"Times-Roman\"\n");
  fprintf(fp, "    fontcolor = \"black\"\n");
  fprintf(fp, "    color = \"black\"\n");
  fprintf(fp, "  ]\n");
}
#endif /* 0 */


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
