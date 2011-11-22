/**
 ** @file mod_iodef.h
 ** Definitions for mod_iodef.c
 **
 ** @author Baptiste GOURDIN <baptiste.gourdin@inria.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 ** @date  Started on: Wed Jan 15 17:07:26 2003
 ** @date Last update: Thu Aug  2 23:38:46 2007
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */


#ifndef MOD_IODEF_H_
# define MOD_IODEF_H_

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/xmlschemas.h>

#include "mod_xml.h"

typedef struct iodef_cfg_s iodef_cfg_t;
struct iodef_cfg_s {
    char*	CSIRT_name;
    char*	report_dir;
    char*	iodef_conf_dir;
    int		iodef_conf_dir_len;
    xmlSchemaPtr	schema;
    // XXX: Not implemented  => Use a contact database (xml file provided)
    xmlNodePtr	contacts;
    int		full_dump;
};


xml_doc_t*
generate_report(orchids_t	*ctx,
		mod_entry_t	*mod,
		state_instance_t *state);

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


#endif /* !MOD_IODEF_H_ */
