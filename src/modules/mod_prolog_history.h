/**
 ** @file mod_prolog_history.h
 ** Definitions for mod_prolog_history.h
 **
 ** @author Baptiste GOURDIN <gourdin@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 ** @date  Started on: Mon Mar 14 16:15:13 CET 2011
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */
#ifndef MOD_PROLOG_HISTORY_H
# define MOD_PROLOG_HISTORY_H

#include "orchids_types.h"
#include "orchids.h"
#include "mod_prolog.h"

typedef struct prolog_history_s prolog_history_t;
struct prolog_history_s {
  time_t	time;
  char		*odbc_DSN;
  SLIST_ENTRY(prolog_history_t)	next;
};

typedef struct prolog_history_cfg_s prolog_history_cfg_t;
struct prolog_history_cfg_s {
  SLIST_HEAD(prolog_history, prolog_history_t) history;
  char*    time_fields_name[256];
  int32_t	time_fields_ids[256];
  int32_t	time_fields_nb;
  prolog_cfg_t	*prolog_cfg;
};


#endif /* !MOD_PROLOG_HISTORY_H */
