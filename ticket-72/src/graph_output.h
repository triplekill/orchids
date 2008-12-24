/**
 ** @file graph_output.h
 ** Public definitions for graph_output.c.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 1.0
 ** @ingroup output
 **
 ** @date  Started on: Fri May 23 12:18:40 2003
 ** @date Last update: Fri Jul 27 16:00:42 2007
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef GRAPH_OUTPUT_H
#define GRAPH_OUTPUT_H

#include <stdio.h>

#include "orchids.h"

/**
 * Display a rule definition in the GraphViz dot format.
 *
 * @param fp Output stream.
 * @param rule Rule to display.
 **/
void
fprintf_rule_dot(FILE *fp, rule_t *rule);


/**
 * Display a rule instance path tree in the GraphViz dot format.
 *
 * @param fp Output stream.
 * @param rule Rule instance to display.
 * @param options Options flags for output.
 * @param tq Current thread queue.
 * option #DOT_RETRIGLIST: draw retrig list (in dotted links).
 **/
void
fprintf_rule_instance_dot(FILE *fp,
                          rule_instance_t *rule,
                          int options,
                          wait_thread_t *tq,
                          int limit);

#endif /* GRAPH_OUTPUT_H */
