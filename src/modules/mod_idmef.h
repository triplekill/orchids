/**
 ** @file mod_idmef.h
 ** Definitions for mod_idmef.c
 **
 ** @author Baptiste GOURDIN <gourdin@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **/

#ifndef MOD_IDMEF_H
# define MOD_IDMEF_H


#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#include <libxml/parser.h>
#include <libxml/xmlstring.h>


#include "orchids.h"

#include "orchids_api.h"

#define F_PTR	0

#define XML_ALLOC_SIZE	1024
#define XML_PATH_MAX	1024

#define MAX_IDMEF_FIELDS 32
#define MAX_IDMEF_SIZE 8192

typedef struct	idmef_cfg_t
{
    unsigned int       	nb_fields;
    char*		field_xpath[MAX_IDMEF_FIELDS];
    char		buff[MAX_IDMEF_SIZE];
    int			buff_len;
    char		too_big;
    char*		analyzer_id;
    char*		analyzer_name;
    char*		analyzer_node_address;
    char*		analyzer_node_location;
    char*		analyzer_node_name;
    char*		report_dir;
}		idmef_cfg_t;

typedef struct idmef_buf_t
{
    size_t	length;
    char*	message;
}		idmef_buf_t;

static int
dissect_idmef(orchids_t *ctx, mod_entry_t *mod, event_t *event, void *data);


static void *
idmef_preconfig(orchids_t *ctx, mod_entry_t *mod);

#endif /* !MOD_IDMEF_H */


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

