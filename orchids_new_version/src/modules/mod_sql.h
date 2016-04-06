/**
 ** @file mod_sql.h
 ** SQL database load module headers.
 **
 ** @author Pierre-Arnaud SENTUCQ <pierre-arnaud.sentucq@lsv.ens-cachan.fr>
 ** @author Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
 **
 ** @version 0.2
 ** @ingroup modules
 **
 ** @date  Started on: Wed Apr  1 17:04:27 CEST 2015
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef MOD_SQL_H
#define MOD_SQL_H

#include "orchids.h"
#include "db.h"


typedef struct db_from_sqlite3_data db_from_sqlite3_data;
struct db_from_sqlite3_data
{
  db_map **db;
  int nfields;
  int error;
  orchids_t *ctx;
};

typedef struct list_node node;
struct list_node
{
  char *name;
  db_map *db;
  struct list_node *prev;
  struct list_node *next;
};

typedef struct double_list dlist;
struct double_list
{
  size_t length;
  struct list_node *head;
  struct list_node *tail;
};

typedef struct sql_config_s sql_config_t;
struct sql_config_s
{
  long max_db;
  struct double_list *dl;
};

/* create a new list */
dlist *dlist_new(gc_t *gc_ctx);

/* return the length of the list */
size_t dlist_length(dlist *dl);

/* return the node containing the value name */
node *dlist_find(dlist *dl, char *name);

/* add a new node at the beginning of the list */
dlist *dlist_prepend(gc_t *gc_ctx, dlist *dl, char *name, db_map *db);

/* remove the last node of the list */
dlist *dlist_remove_last(dlist *dl);

/* move the node n at the beginning of the list */
dlist *dlist_move_into_lead(dlist *dl, node *n);


static char *str_from_stack_arg(orchids_t *ctx, int arg_n, int args_total_n);

static void sql_set_max_db(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir);

static void *mod_sql_preconfig(orchids_t *ctx, mod_entry_t *mod);

#endif /* MOD_SQL_H */

/*
** Copyright (c) 2015,2016 by Pierre-Arnaud SENTUCQ, Laboratoire Spécification
** et Vérification (LSV), CNRS UMR 8643 & ENS Cachan.
** Copyright (c) 2016 by Jean GOUBAULT-LARRECQ, Laboratoire Spécification
** et Vérification (LSV), CNRS UMR 8643 & ENS Cachan.
**
** Pierre-Arnaud SENTUCQ <pierre-arnaud.sentucq@lsv.ens-cachan.fr>
** Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
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
