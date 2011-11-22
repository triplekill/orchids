/**
 ** @file mod_xml.c
 ** libxml2 module (See http://xmlsoft.org)
 **
 ** @author Baptiste GOURDIN <gourdin@lsv.ens-cachan.fr>
 ** @version 0.1
 ** @ingroup modules
 **
 **
 ** @date  Started on:  Tue Apr 12 11:20:38 CEST 2011
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif


#include <string.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/xmlschemas.h>
#include <time.h>

#include "orchids.h"
#include "lang.h"
#include "mod_xml.h"


input_module_t mod_xml;

/**
 * Return a node for a XPath request, create nodes automatically
 *
 * @param ctx	xmlXPath context
 * @param path  XPath string
 *
 * @return NULL if an error occured, else the node is returned.
 */
xmlNodePtr
xml_walk_and_create (xmlXPathContextPtr ctx,
		     xmlChar	*path)
{
  xmlXPathObjectPtr	xpathObj = NULL;
  xmlNodePtr		node = NULL;
  unsigned int		path_len = 0;
  unsigned int		new_len = 0;
  xmlChar		*off = NULL;

  path_len = xmlStrlen(path);

  do {
    if (xpathObj)
      xmlXPathFreeObject(xpathObj);
    xpathObj = xmlXPathEvalExpression(path, ctx);
    if ((xpathObj == NULL) || (xpathObj->nodesetval == 0)
	|| (xpathObj->nodesetval->nodeNr == 0))
    {
      off = (xmlChar*)strrchr((char*)path, '/');
      if (off != NULL)
	*off = 0;
    }
  } while (((xpathObj == NULL) || (xpathObj->nodesetval == 0)
	    || (xpathObj->nodesetval->nodeNr == 0)) && (off != path));

  if (off != path)
    node = xpathObj->nodesetval->nodeTab[0];
  else
  {
    DebugLog(DF_MOD, DS_ERROR,
	     "Error: wrong root node \"%s\"\n", path);
    return NULL;
  }

  new_len = xmlStrlen (path);
  while  (new_len != path_len)
  {
    const xmlChar	*node_name;

    path[new_len] = '/';
    node_name = xmlStrchr(path + new_len + 1, ':');
    if (node_name != NULL)
      node_name += 1;
    else
      node_name = path + new_len + 1;

    node = xmlNewChild(node, node->ns, node_name, NULL);
    new_len = xmlStrlen (path);
  }

  xmlXPathFreeObject(xpathObj);

  return node;
}

xmlNode*
new_xs_datetime_node(xmlNode	*parent,
		     const char	*name,
		     struct tm *time)
{
  char		buff[128];
  size_t	len;

  len = strftime(buff, 128 * sizeof (char), "%Y-%m-%dT%H:%M:%S%z", time);

  if ((buff[len - 5] == '+') ||
      (buff[len - 5] == '-'))
  {
    buff[len + 1] = 0;
    buff[len] = buff[len - 1];
    buff[len - 1] = buff[len - 2];
    buff[len - 2] = ':';
  }

  return xmlNewChild(parent, parent->ns, BAD_CAST name,
		     BAD_CAST buff);
}

/**
 * Get an xml element, node or attribute using xpath request
 *
 * @param xml_doc Xml document
 * @param xpath	  XPatch request
 *
 * @return return the element if found, else NULL.
 */
char*
xml_get_string(xml_doc_t	*xml_doc,
	       char		*path)
{
  xmlChar	*text;
  xmlXPathObjectPtr	xpathObj = NULL;
  xmlNodePtr		node = NULL;

  xpathObj = xmlXPathEvalExpression(BAD_CAST path, xml_doc->xpath_ctx);
  if ((xpathObj == NULL) || (xpathObj->nodesetval == 0)
      || (xpathObj->nodesetval->nodeNr == 0))
  {
    DebugLog(DF_ENG, DS_ERROR, "error : no nodes for path %s\n", path);
    return (NULL);
  }
  else
  {
    node = xpathObj->nodesetval->nodeTab[0];
    text = xmlNodeGetContent(node);
    xmlXPathFreeObject(xpathObj);
    return ((char*)text);
  }
}

/**
 * Set a value in the xml tree, create intermediate nodes if necessary
 * Push the result on the stack
 *
 * @param ctx	Orchids context
 * @param state Current state instance
 */
static void
issdl_xml_set_string(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t	*var1, *var2, *var3;
  xml_doc_t	*xml_doc;
  xmlNode	*node = NULL;
  xmlChar	*content;

  var1 = stack_pop(ctx->ovm_stack);
  var2 = stack_pop(ctx->ovm_stack);
  var3 = stack_pop(ctx->ovm_stack);

  if (!var1 || !var2 || !var3
      || (TYPE(var1) != T_EXTERNAL)
      || (TYPE(var2) != T_STR && TYPE(var2) != T_VSTR)
      || (TYPE(var3) != T_STR && TYPE(var3) != T_VSTR)
      || (!EXTPTR(var1)))
  {
    DebugLog(DF_ENG, DS_ERROR, "parameter error\n");
    ISSDL_RETURN_FALSE(ctx, state);
  }

  xml_doc = EXTPTR(var1);
  if (!(node = xml_walk_and_create(xml_doc->xpath_ctx,
				   BAD_CAST STR(var2))))
    return;

  if (TYPE(var3) == T_STR)
    content = xmlEncodeEntitiesReentrant(xml_doc->doc, BAD_CAST STR(var3));
  else
  {
    char *str;
    str = strndup(VSTR(var3), VSTRLEN(var3));
    content = xmlEncodeEntitiesReentrant(xml_doc->doc, BAD_CAST str);
    Xfree(str);
  }
  xmlNodeSetContent(node, content);
  xmlFree(content);

  FREE_IF_NEEDED(var2);
  FREE_IF_NEEDED(var3);
}

/**
 * Set an attribute in the xml tree, create nodes if necessary
 * Push the result on the stack
 *
 * @param ctx	Orchids context
 * @param state Current state instance
 */
static void
issdl_xml_set_attr_string(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t	*var1, *var2, *var3, *var4;
  xml_doc_t	*xml_doc;
  xmlNode	*node = NULL;
  xmlChar	*prop_name;
  xmlChar	*prop_value;

  var1 = stack_pop(ctx->ovm_stack);
  var2 = stack_pop(ctx->ovm_stack);
  var3 = stack_pop(ctx->ovm_stack);
  var4 = stack_pop(ctx->ovm_stack);

  if (!var1 || !var2 || !var3
      || (TYPE(var1) != T_EXTERNAL) || (TYPE(var2) != T_STR) || (TYPE(var3) != T_STR)
      || (TYPE(var4) != T_STR)
      || (!EXTPTR(var1)))
  {
    DebugLog(DF_ENG, DS_ERROR, "parameter error\n");
    ISSDL_RETURN_FALSE(ctx, state);
  }

  xml_doc = EXTPTR(var1);
  if (!(node = xml_walk_and_create(xml_doc->xpath_ctx, BAD_CAST STR(var2))))
    return;

  prop_name = xmlEncodeEntitiesReentrant(xml_doc->doc, BAD_CAST STR(var3));
  prop_value = xmlEncodeEntitiesReentrant(xml_doc->doc, BAD_CAST STR(var4));
  xmlSetProp(node, prop_name, prop_value);
  xmlFree(prop_name);
  xmlFree(prop_value);

  FREE_IF_NEEDED(var2);
  FREE_IF_NEEDED(var3);
  FREE_IF_NEEDED(var4);
}



/**
 * Get an xml element, node or attribute using xpath request
 * Push the result on the stack
 *
 * @param ctx	Orchids context
 * @param state Current state instance
 */
static void
issdl_xml_get_string(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t	*var1;
  ovm_var_t	*var2;
  xml_doc_t	*xml_doc;
  xmlXPathObjectPtr	xpathObj = NULL;
  xmlNodePtr		node = NULL;
  ovm_var_t	*res;
  size_t	res_len;
  xmlChar	*text;

  var1 = stack_pop(ctx->ovm_stack);
  var2 = stack_pop(ctx->ovm_stack);

  if (!var1 || !var2 || (TYPE(var1) != T_EXTERNAL) || (TYPE(var2) != T_STR)) {
    DebugLog(DF_ENG, DS_ERROR, "parameter error\n");
    ISSDL_RETURN_FALSE(ctx, state);
  }

  if ((xml_doc = EXTPTR(var1)) == NULL)  {
    DebugLog(DF_ENG, DS_ERROR, "error : pointer is null\n");
    ISSDL_RETURN_FALSE(ctx, state);
  }

  xpathObj = xmlXPathEvalExpression(BAD_CAST STR(var2), xml_doc->xpath_ctx);
  if ((xpathObj == NULL) || (xpathObj->nodesetval == 0)
      || (xpathObj->nodesetval->nodeNr == 0))
  {
    DebugLog(DF_ENG, DS_ERROR, "error : no nodes for path %s\n", STR(var2));
    stack_push(ctx->ovm_stack, ovm_str_new(0));
  }
  else
  {
    node = xpathObj->nodesetval->nodeTab[0];
    if ((text = xmlNodeGetContent(node)) == NULL)
      stack_push(ctx->ovm_stack, ovm_str_new(0));
    else
    {
      res_len = strlen((char*)text);
      res = ovm_str_new(res_len);
      memcpy (STR(res), text, res_len);
      stack_push(ctx->ovm_stack, res);
    }
  }
}



/**
 * Free the xml document
 *
 * @param doc XML Document to free
 */
void
free_xml_doc(void	*ptr)
{
  xml_doc_t	*doc = ptr;

  xmlFreeDoc(doc->doc);
  xmlXPathFreeContext(doc->xpath_ctx);
  Xfree(doc);
  return;
}

/**
 * Create a new extern variable for xml documents
 */
ovm_var_t*
ovm_xml_new()
{
  ovm_var_t*	res;

  res = ovm_extern_new ();
  EXTFREE(res) = free_xml_doc;
  EXTDESC(res) = "Xml Document";
  return res;
}

static void
issdl_free_xml_doc(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t	*var1;

  var1 = stack_pop(ctx->ovm_stack);
  FREE_VAR(var1);
}

/**
 * Dump the xml doc on stderr
 *
 * @param ctx	Orchids context
 * @param state Current state instance
 */
static void
issdl_dump_xml(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t	*var1;

  var1 = stack_pop(ctx->ovm_stack);
  xmlDocFormatDump(stdout, ((xml_doc_t*)EXTPTR(var1))->doc, 1);
}

static void *
mod_xml_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  DebugLog(DF_MOD, DS_INFO, "load() mod_xml@%p\n", (void *) &mod_xml);

  register_lang_function(ctx,
                         issdl_dump_xml,
                         "xml_dump", 1,
                         "dump xml doc on stderr");

  register_lang_function(ctx,
                         issdl_xml_get_string,
                         "xml_get_str", 2,
                         "get a node content following an xpath request");

  register_lang_function(ctx,
                         issdl_xml_set_string,
                         "xml_set_str", 3,
                         "set a node content following an xpath request");

  register_lang_function(ctx,
                         issdl_xml_set_attr_string,
                         "xml_set_prop", 4,
                         "set a node property following an xpath request");

  register_lang_function(ctx,
                         issdl_free_xml_doc,
                         "xml_free", 3,
                         "set a string from an xml doc using an xpath request");



  return (NULL);
}

input_module_t mod_xml = {
  MOD_MAGIC,
  ORCHIDS_VERSION,
  "xml",
  "CeCILL2",
  NULL,
  NULL,
  mod_xml_preconfig,
  NULL,
  NULL
};

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
