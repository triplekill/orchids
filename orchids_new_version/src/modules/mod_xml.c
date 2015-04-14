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
#include "ovm.h"
#include "mod_utils.h"
#include "mod_xml.h"


input_module_t mod_xml;


char * xml_description = "xml document";

/**
 * Return a node for a XPath request, create nodes automatically
 *
 * @param ctx	xmlXPath context
 * @param path  XPath string
 *
 * @return NULL if an error occured, else the node is returned.
 */
xmlNodePtr xml_walk_and_create (xmlXPathContextPtr ctx,
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

xmlNode* new_xs_datetime_node(xmlNode	*parent,
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
char *xml_get_string(xml_doc_t *xml_doc,
		     char *path)
{
  xmlChar	*text;
  xmlXPathObjectPtr	xpathObj = NULL;
  xmlNodePtr		node = NULL;

  xpathObj = xmlXPathEvalExpression(BAD_CAST path, xml_doc->xpath_ctx);
  if ((xpathObj == NULL) || (xpathObj->nodesetval == 0)
      || (xpathObj->nodesetval->nodeNr == 0))
  {
    DebugLog(DF_MOD, DS_ERROR, "error : no nodes for path %s\n", path);
    return (NULL);
  }
  else
  {
    node = xpathObj->nodesetval->nodeTab[0];
    text = xmlNodeGetContent(node);
    xmlXPathFreeObject(xpathObj);
    return ((char *)text);
  }
}

/**
 * Set a value in the xml tree, create intermediate nodes if necessary
 * Push the result on the stack
 *
 * @param ctx	Orchids context
 * @param state Current state instance
 */
static void issdl_xml_set_string(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t	*var1, *var2, *var3;
  char *str2, *str3;
  xmlDocPtr doc;
  xmlNode	*node = NULL;
  xmlChar	*content;

  var1 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 3);
  if (var1==NULL || TYPE(var1)!=T_EXTERNAL || EXTDESC(var1)!=xml_description)
    {
      DebugLog(DF_MOD, DS_ERROR, "first parameter not an xml document\n");
      STACK_DROP(ctx->ovm_stack, 3);
      PUSH_RETURN_FALSE(ctx);
      return;
    }
  var2 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  if (var2==NULL || (TYPE(var2)!=T_STR && TYPE(var2)!=T_VSTR))
    {
      DebugLog(DF_MOD, DS_ERROR, "second parameter not a string\n");
      STACK_DROP(ctx->ovm_stack, 3);
      PUSH_RETURN_FALSE(ctx);
      return;
    }
  var3 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  if (var3==NULL || (TYPE(var3)!=T_STR && TYPE(var3)!=T_VSTR))
    {
      DebugLog(DF_MOD, DS_ERROR, "third parameter not a string\n");
      STACK_DROP(ctx->ovm_stack, 3);
      PUSH_RETURN_FALSE(ctx);
      return;
    }

  doc = XMLDOC_RW(ctx->gc_ctx, state, var1);
  if (doc==NULL)
    {
      DebugLog(DF_MOD, DS_ERROR, "error : xmldoc is null\n");
      STACK_DROP(ctx->ovm_stack, 3);
      PUSH_RETURN_FALSE(ctx);
      return;
    }

  str2 = ovm_strdup (ctx->gc_ctx, var2);
  if (!(node = xml_walk_and_create(XMLPATHCTX(var1),
				   BAD_CAST str2)))
    {
      gc_base_free (str2);
      STACK_DROP(ctx->ovm_stack, 3);
      PUSH_RETURN_FALSE(ctx);
      return;
    }

  str3 = ovm_strdup (ctx->gc_ctx, var3);
  content = xmlEncodeEntitiesReentrant(doc, BAD_CAST str3);
  xmlNodeSetContent(node, content);
  xmlFree(content);

  gc_base_free (str2);
  gc_base_free (str3);
  STACK_DROP(ctx->ovm_stack, 3);
  PUSH_RETURN_TRUE(ctx);
}

/**
 * Set an attribute in the xml tree, create nodes if necessary
 * Push the result on the stack
 *
 * @param ctx	Orchids context
 * @param state Current state instance
 */
static void issdl_xml_set_attr_string(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t	*var1, *var2, *var3, *var4;
  char *str2, *str3, *str4;
  xmlDocPtr doc;
  xmlNode	*node = NULL;
  xmlChar	*prop_name;
  xmlChar	*prop_value;

  var1 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 4);
  if (var1==NULL || TYPE(var1)!=T_EXTERNAL || EXTDESC(var1)!=xml_description)
    {
      DebugLog(DF_MOD, DS_ERROR, "first parameter not an xml document\n");
      STACK_DROP(ctx->ovm_stack, 4);
      PUSH_RETURN_FALSE(ctx);
      return;
    }
  var2 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 3);
  if (var2==NULL || (TYPE(var2)!=T_STR && TYPE(var2)!=T_VSTR))
    {
      DebugLog(DF_MOD, DS_ERROR, "second parameter not a string\n");
      STACK_DROP(ctx->ovm_stack, 4);
      PUSH_RETURN_FALSE(ctx);
      return;
    }
  var3 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  if (var3==NULL || (TYPE(var3)!=T_STR && TYPE(var3)!=T_VSTR))
    {
      DebugLog(DF_MOD, DS_ERROR, "third parameter not a string\n");
      STACK_DROP(ctx->ovm_stack, 4);
      PUSH_RETURN_FALSE(ctx);
      return;
    }
  var4 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  if (var4==NULL || (TYPE(var4)!=T_STR && TYPE(var4)!=T_VSTR))
    {
      DebugLog(DF_MOD, DS_ERROR, "fourth parameter not a string\n");
      STACK_DROP(ctx->ovm_stack, 4);
      PUSH_RETURN_FALSE(ctx);
      return;
    }

  doc = XMLDOC_RW(ctx->gc_ctx, state, var1);
  if (doc==NULL)
    {
      DebugLog(DF_MOD, DS_ERROR, "error : xmldoc is null\n");
      STACK_DROP(ctx->ovm_stack, 4);
      PUSH_RETURN_FALSE(ctx);
      return;
    }

  str2 = ovm_strdup (ctx->gc_ctx, var2);
  if (!(node = xml_walk_and_create(XMLPATHCTX(var1),
				   BAD_CAST str2)))
    {
      gc_base_free (str2);
      STACK_DROP(ctx->ovm_stack, 4);
      PUSH_RETURN_FALSE(ctx);
      return;
    }

  str3 = ovm_strdup (ctx->gc_ctx, var3);
  prop_name = xmlEncodeEntitiesReentrant(doc, BAD_CAST str3);
  str4 = ovm_strdup (ctx->gc_ctx, var4);
  prop_value = xmlEncodeEntitiesReentrant(doc, BAD_CAST str4);
  xmlSetProp(node, prop_name, prop_value);
  gc_base_free (str2);
  gc_base_free (str3);
  gc_base_free (str4);
  xmlFree(prop_name);
  xmlFree(prop_value);
  STACK_DROP(ctx->ovm_stack, 4);
  PUSH_RETURN_TRUE(ctx);
}



/**
 * Get an xml element, node or attribute using xpath request
 * Push the result on the stack
 *
 * @param ctx	Orchids context
 * @param state Current state instance
 */
static void issdl_xml_get_string(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t	*var1;
  ovm_var_t	*var2;
  char *str2;
  xml_doc_t	*xml_doc;
  xmlXPathObjectPtr	xpathObj = NULL;
  xmlNodePtr		node = NULL;
  ovm_var_t	*res;
  size_t	res_len;
  xmlChar	*text;

  var1 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  if (var1==NULL || TYPE(var1)!=T_EXTERNAL || EXTDESC(var1)!=xml_description)
    {
      DebugLog(DF_MOD, DS_ERROR, "first parameter not an xml document\n");
      STACK_DROP(ctx->ovm_stack, 2);
      PUSH_RETURN_FALSE(ctx);
      return;
    }
  var2 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  if (var2==NULL || (TYPE(var2)!=T_STR && TYPE(var2)!=T_VSTR))
    {
      DebugLog(DF_MOD, DS_ERROR, "second parameter not a string\n");
      STACK_DROP(ctx->ovm_stack, 2);
      PUSH_RETURN_FALSE(ctx);
      return;
    }

  if ((xml_doc = EXTPTR(var1)) == NULL)
    {
      DebugLog(DF_MOD, DS_ERROR, "error : xmldoc is null\n");
      STACK_DROP(ctx->ovm_stack, 2);
      PUSH_RETURN_FALSE(ctx);
      return;
    }

  str2 = ovm_strdup (ctx->gc_ctx, var2);
  xpathObj = xmlXPathEvalExpression(BAD_CAST str2, xml_doc->xpath_ctx);
  if ((xpathObj == NULL) || (xpathObj->nodesetval == 0)
      || (xpathObj->nodesetval->nodeNr == 0))
    {
      DebugLog(DF_MOD, DS_ERROR, "error : no node for path %s\n", str2);
      STACK_DROP(ctx->ovm_stack, 2);
      PUSH_VALUE(ctx, NULL);
    }
  else
    {
      node = xpathObj->nodesetval->nodeTab[0];
      if ((text = xmlNodeGetContent(node)) == NULL)
	{
	  STACK_DROP(ctx->ovm_stack, 2);
	  PUSH_VALUE(ctx, NULL);
	}
      else
	{
	  res_len = strlen((char*)text);
	  res = ovm_str_new(ctx->gc_ctx, res_len);
	  memcpy (STR(res), text, res_len);
	  STACK_DROP(ctx->ovm_stack, 2);
	  PUSH_VALUE(ctx, res);
	}
    }
  gc_base_free (str2);
}



/**
 * Free the xml document
 *
 * @param doc XML Document to free
 */

void free_xml_doc(void *ptr)
{
  xml_doc_t *doc = ptr;

  if (doc!=NULL)
    {
      free_thread_local_obj (doc->tl);
      xmlXPathFreeContext(doc->xpath_ctx);
      gc_base_free(doc);
    }
}

/**
 * Create a new extern variable for xml documents
 */
static void xml_free (void *obj)
{
  xmlFreeDoc (obj);
}

static void *xml_copy (void *obj)
{
  return xmlCopyDoc (obj, 1);
}

static thread_local_class_t xml_thread_local_class = {
  xml_free,
  xml_copy
};

ovm_var_t *ovm_xml_new(gc_t *gc_ctx, xmlDocPtr doc, xmlXPathContextPtr xpath_ctx,
		       char *description)
{
  ovm_var_t *res;
  xml_doc_t *xdoc;

  xdoc = gc_base_malloc(gc_ctx, sizeof(xml_doc_t));
  xdoc->tl = new_thread_local_obj (gc_ctx, &xml_thread_local_class, doc);
  xdoc->xpath_ctx = xpath_ctx;
  res = ovm_extern_new (gc_ctx, xdoc, description, free_xml_doc);
  return res;
}

#if 0
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

  var1 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack; 1);
  xmlDocFormatDump(stdout, ((xml_doc_t*)EXTPTR(var1))->doc, 1);
  STACK_DROP(ctx->ovm_stack);
}
#endif

static type_t t_xmldoc = { "xmldoc", T_EXTERNAL };

static const type_t *xml_set_str_sig[] = { &t_int, &t_xmldoc, &t_str, &t_str }; /* returns 0 or 1, in fact */
static const type_t **xml_set_str_sigs[] = { xml_set_str_sig, NULL };

static const type_t *xml_get_str_sig[] = { &t_str, &t_xmldoc, &t_str };
static const type_t **xml_get_str_sigs[] = { xml_get_str_sig, NULL };

static const type_t *xml_set_prop_sig[] = { &t_int, &t_xmldoc, &t_str, &t_str, &t_str }; /* returns 0 or 1, in fact */
static const type_t **xml_set_prop_sigs[] = { xml_set_prop_sig, NULL };

#if 0
static const type_t *xml_dump_sig[] = { &t_int, &t_xmldoc }; /* returns 0 or 1, in fact */
static const type_t **xml_dump_sigs[] = { xml_dump_sig, NULL };
#endif

static void *mod_xml_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  DebugLog(DF_MOD, DS_INFO, "load() mod_xml@%p\n", (void *) &mod_xml);

#if 0
  register_lang_function(ctx,
                         issdl_dump_xml,
                         "xml_dump",
			 1, xml_dump_sigs,
			 m_random,
                         "dump xml doc on stderr");
#endif

  register_lang_function(ctx,
                         issdl_xml_get_string,
                         "xml_get_str",
			 2, xml_get_str_sigs,
			 m_random,
                         "get a node content following an xpath request");

  register_lang_function(ctx,
                         issdl_xml_set_string,
                         "xml_set_str",
			 3, xml_set_str_sigs,
			 m_random,
                         "set a node content following an xpath request");

  register_lang_function(ctx,
                         issdl_xml_set_attr_string,
                         "xml_set_prop",
			 4, xml_set_prop_sigs,
			 m_random,
                         "set a node property following an xpath request");

  return NULL;
}

input_module_t mod_xml = {
  MOD_MAGIC,
  ORCHIDS_VERSION,
  0,			    /* flags */
  "xml",
  "CeCILL2",
  NULL,
  NULL,
  mod_xml_preconfig,
  NULL,
  NULL,
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
