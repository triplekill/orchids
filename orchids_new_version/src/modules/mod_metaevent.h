/**
 ** @file mod_metaevent.h
 ** Definitions for mod_metaevent.c
 **
 ** @author Baptiste GOURDIN <gourdin@lsv.ens-cachan.fr>
 ** @author Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
 **
 ** @version 0.2
 ** @ingroup modules
 **
 **
 ** @date  Started on: Fri Feb  7 11:07:42 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef MOD_METAEVENT_H
#define MOD_METAEVENT_H

#include "stailq.h"
#include "orchids.h"



typedef struct meta_field_s meta_field_t;
struct meta_field_s
{
  STAILQ_ENTRY(meta_field_t) fields;
  STAILQ_ENTRY(meta_field_t) globfields;
  int field_id;
  type_t *type;
  char *name;
  char *description;
};

typedef struct meta_vmod_s meta_vmod_t;
struct meta_vmod_s
{
  STAILQ_ENTRY(meta_vmod_t) vmods;
  STAILQ_ENTRY(meta_vmod_t) globvmods;
  /* match list */
  STAILQ_HEAD(fields, meta_field_t) field_list;
  STAILQ_HEAD(globfields, meta_field_t) field_globlist;
  char *name;
  int mod_id;
  input_module_t mod_entry;
  field_t *field_array;
  int nfields;
  field_table_t *fields;
  strhash_t *field_hash;
};

typedef struct eventlist_s eventlist_t;
struct eventlist_s
{
    event_t	*event;
    STAILQ_ENTRY(eventlist_t) events;
};

typedef struct metaevent_config_s metaevent_config_t;
struct metaevent_config_s
{
    STAILQ_HEAD(vmods, meta_vmod_t) vmod_list;
    strhash_t *mod_hash;
    int mods;
};



static void * metaevent_preconfig(orchids_t *ctx, mod_entry_t *mod);

static void metaevent_postconfig(orchids_t *ctx, mod_entry_t *mod);

static int rtaction_inject_event(orchids_t *ctx, heap_entry_t *he);

#endif /* MOD_METAEVENT_H */

/*
** Copyright (c) 2011 by Baptiste GOURDIN, Laboratoire Spécification
** et Vérification (LSV), CNRS UMR 8643 & ENS Cachan.
** Copyright (c) 2013-2015 by Jean GOUBAULT-LARRECQ, Laboratoire Spécification
** et Vérification (LSV), CNRS UMR 8643 & ENS Cachan.
**
** Baptiste GOURDIN <gourdin@lsv.ens-cachan.fr>
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
