/**
 ** @file mod_xml.h
 ** Definitions for mod_xml.h
 **
 ** @author Baptiste Gourdin <gourdin@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 ** @date  Started on: Tue Apr 12 14:33:03 CEST 2011
 **/

#ifndef MOD_XML_H_
# define MOD_XML_H_

typedef struct {
    xmlDocPtr	doc;
    xmlXPathContextPtr	xpath_ctx;
}		xml_doc_t;

ovm_var_t*
ovm_xml_new();

char*
xml_get_string(xml_doc_t	*xml_doc,
	       char		*path);

xmlNodePtr
xml_walk_and_create (xmlXPathContextPtr ctx,
		     xmlChar	*path);

char
validate_xml_with_schema(xmlSchemaPtr	schema,
			 char		*doc);

xmlNode*
new_xs_datetime_node(xmlNode	*parent,
		     const char	*name,
		     struct tm *time);

xmlNodePtr
xml_walk_and_create (xmlXPathContextPtr ctx,
		     xmlChar	*path);

void
free_xml_doc(void	*ptr);


#endif /* !MOD_XML_H_ */

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
