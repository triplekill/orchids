/**
 ** @file mod_prelude.h
 ** Definitions for mod_prelude.c
 **
 ** @author Baptiste GOURDIN <gourdin@lsv.ens-cachan.fr>
 ** @author Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
 **
 ** @version 0.2
 ** @ingroup modules
 **
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef MOD_PRELUDE_H_
# define MOD_PRELUDE_H_

#include <libprelude/prelude.h>
#include <libprelude/prelude-client.h>
#include <libprelude/idmef.h>
#include <libprelude/idmef-message-print.h>
#include <libpreludedb/preludedb.h>
#include <libpreludedb/preludedb-sql.h>

#include "orchids.h"

#include "orchids_api.h"
#include "evt_mgr.h"

#define INITIAL_MODPRELUDE_POLL_DELAY  1
#define DEFAULT_MODPRELUDE_POLL_PERIOD 1

#define MAX_PRELUDE_FIELDS	128

#ifdef OBSOLETE
#define F_PTR		0
#endif

enum prelude_mode_e {
  PRELUDE_MODE_SENSOR,
  PRELUDE_MODE_ANALYZER,
  PRELUDE_MODE_PREWIKKA
};

typedef struct modprelude_s modprelude_t;
struct modprelude_s
{
    prelude_client_t *	client;
    prelude_io_t	*prelude_io;
    preludedb_t		*db;
    enum prelude_mode_e	mode;
    const char*		profile;
    const char*		prelude_db_settings;
    int			poll_period;
    size_t       	nb_fields;
    const char*		field_xpath[MAX_PRELUDE_FIELDS];
};

#endif /* !MOD_PRELUDE_H_ */

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
