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
#include <libxml/hash.h>
#include <time.h>
#include <errno.h>

#include "orchids.h"
#include "lang.h"
#include "ovm.h"
#include "mod_utils.h"
#include "mod_xml.h"


input_module_t mod_xml;

static char *xml_description = "xml document";

char *xml_desc(void)
{
  return xml_description;
}

static void *xml_copy (gc_t *gc_ctx, void *obj)
{
  xmlXPathContextPtr xpath_ctx = obj;
  xmlDocPtr doc = xpath_ctx->doc;

  doc = xmlCopyDoc (obj, 1);
  if (doc==NULL)
    return NULL;
  return xmlXPathNewContext(doc);
}

static void xml_free (void *obj)
{
  xmlXPathContextPtr xpath_ctx = obj;
  xmlDocPtr doc;

  if (xpath_ctx!=NULL)
    {
      doc = xpath_ctx->doc;
      if (doc!=NULL)
	xmlFreeDoc (doc);
      xmlXPathFreeContext (xpath_ctx);
    }
}

static void xml_hash_count_scanner (void *payload, void *data, xmlChar *name)
{
  size_t *np = data;

  ++*np;
}

static void xml_hash_save_scanner (void *payload, void *data, xmlChar *name)
{
  save_ctx_t *sctx = data;
  int err;

  err = save_string (sctx, (char *)name);
  if (err) { errno = err; return; }
  err = save_string (sctx, (char *)payload);
  if (err) { errno = err; return; }
}

static int xml_save (save_ctx_t *sctx, void *ptr)
{
  xmlXPathContextPtr xpath_ctx = ptr;
  xmlDocPtr doc;
  size_t n;
  int err;

  if (xpath_ctx==NULL)
    return -2; /* !!! should we need to save empty xml docs as well? */
  doc = xpath_ctx->doc;
  if (doc==NULL)
    return -2;
  err = save_string (sctx, doc->name);
  if (err) return err;
  err = xmlDocDump (sctx->f, doc);
  if (err) return err;
  xmlHashScan (xpath_ctx->nsHash, xml_hash_count_scanner, &n);
  err = save_size_t (sctx, n);
  if (err) return err;
  errno = 0;
  xmlHashScan (xpath_ctx->nsHash, xml_hash_save_scanner, sctx);
  return errno;
}

static int xml_restore_input_callback (void *context, char *buffer, int len)
{
  int i, c;
  FILE *f = context;

  for (i=0; i<len; i++)
    {
      c = getc (f);
      if (c==EOF) return c;
      buffer[i] = (char)c;
    }
  return 0;
}

static int xml_restore_close_callback (void *context)
{
  return 0;
}

static void *xml_restore (restore_ctx_t *rctx)
{
  xmlDocPtr doc;
  xmlXPathContextPtr xpath_ctx;
  size_t i, ns_n;
  char *name, *url, *baseurl;
  int err;

  err = restore_string (rctx, &baseurl);
  if (err) { errno = err; return NULL; }
  doc = xmlReadIO (xml_restore_input_callback,
		   xml_restore_close_callback,
		   rctx->f,
		   baseurl, /* not too sure about this (see xml_save()) */
		   NULL, XML_PARSE_NOWARNING);
  if (baseurl!=NULL)
    gc_base_free (baseurl);
  if (doc==NULL)
    { errno = -1; return NULL; }
  xpath_ctx = xmlXPathNewContext(doc);
  if (xpath_ctx==NULL)
    {
      xmlFreeDoc(doc);
      errno = -1; return NULL;
    }
  err = restore_size_t (rctx, &ns_n); /* get number of registered namespaces */
  if (err) { err_freexpath:
    errno = err; xmlXPathFreeContext (xpath_ctx); xmlFreeDoc (doc); return NULL; }
  for (i=0; i<ns_n; i++)
    {
      err = restore_string (rctx, &name);
      if (err) goto err_freexpath;
      if (name==NULL) { err = -2; goto err_freexpath; }
      err = restore_string (rctx, &url);
      if (err) { gc_base_free (name); goto err_freexpath; }
      if (url==NULL) { err = -2; goto err_freexpath; }
      xmlXPathRegisterNs (xpath_ctx, BAD_CAST(name), BAD_CAST(url));
      gc_base_free (name);
    }
  return xpath_ctx;
}

static ovm_extern_class_t xml_xclass = {
  "xmldoc",
  xml_copy,
  xml_free,
  xml_save,
  xml_restore
};

uint16_t ovm_xml_new(gc_t *gc_ctx, state_instance_t *si,
		     xmlXPathContextPtr xpath_ctx,
		     char *description)
{
  uint16_t handle;
  ovm_var_t *xmldoc;

  GC_START(gc_ctx, 1);
  xmldoc = ovm_extern_new (gc_ctx, xpath_ctx, &xml_xclass);
  GC_UPDATE(gc_ctx, 0, xmldoc);
  handle = create_fresh_handle (gc_ctx, si, xmldoc);
  GC_END(gc_ctx);
  return handle;
}

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

xmlNode *new_xs_datetime_node(xmlNode *parent,
			      const char *name,
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
char *xml_get_string(xmlXPathContextPtr xpath_ctx,
		     char *path)
{
  xmlChar	*text;
  xmlXPathObjectPtr	xpathObj = NULL;
  xmlNodePtr		node = NULL;

  xpathObj = xmlXPathEvalExpression(BAD_CAST path, xpath_ctx);
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
  ovm_var_t	*var1, *var2, *var3, *doc1;
  char *str2, *str3;
  xmlXPathContextPtr xpath_ctx;
  xmlDocPtr doc;
  xmlNode	*node = NULL;
  xmlChar	*content;

  var1 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 3);
  if (var1==NULL || TYPE(var1)!=T_UINT) /* an uint handle to an xmlXPathContextPtr */
    {
      DebugLog(DF_MOD, DS_ERROR, "first parameter not an xml handle\n");
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

  doc1 = handle_get_wr (ctx->gc_ctx, state, UINT(var1));
  xpath_ctx = EXTPTR(doc1);
  if (xpath_ctx==NULL)
    doc = NULL;
  else
    doc = xpath_ctx->doc;
  if (doc==NULL)
    {
      DebugLog(DF_MOD, DS_ERROR, "error : xmldoc is null\n");
      STACK_DROP(ctx->ovm_stack, 3);
      PUSH_RETURN_FALSE(ctx);
      return;
    }

  str2 = ovm_strdup (ctx->gc_ctx, var2);
  if (!(node = xml_walk_and_create(xpath_ctx, BAD_CAST str2)))
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
  ovm_var_t	*var1, *var2, *var3, *var4, *doc1;
  char *str2, *str3, *str4;
  xmlXPathContextPtr xpath_ctx;
  xmlDocPtr doc;
  xmlNode	*node = NULL;
  xmlChar	*prop_name;
  xmlChar	*prop_value;

  var1 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 4);
  if (var1==NULL || TYPE(var1)!=T_UINT) /* an uint handle to an xmlXPathContextPtr */
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

  doc1 = handle_get_wr (ctx->gc_ctx, state, UINT(var1));
  xpath_ctx = EXTPTR(doc1);
  if (xpath_ctx==NULL)
    doc = NULL;
  else
    doc = xpath_ctx->doc;
  if (doc==NULL)
    {
      DebugLog(DF_MOD, DS_ERROR, "error : xmldoc is null\n");
      STACK_DROP(ctx->ovm_stack, 4);
      PUSH_RETURN_FALSE(ctx);
      return;
    }

  str2 = ovm_strdup (ctx->gc_ctx, var2);
  if (!(node = xml_walk_and_create(xpath_ctx, BAD_CAST str2)))
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
  ovm_var_t *doc1;
  xmlXPathContextPtr xpath_ctx;
  xmlDocPtr doc;
  char *str2;
  xmlXPathObjectPtr	xpathObj = NULL;
  xmlNodePtr		node = NULL;
  ovm_var_t	*res;
  size_t	res_len;
  xmlChar	*text;

  var1 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  if (var1==NULL || TYPE(var1)!=T_UINT) /* an uint handle to an xmlXPathContextPtr */
    {
      DebugLog(DF_MOD, DS_ERROR, "first parameter not an xml handle\n");
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

  doc1 = handle_get_wr (ctx->gc_ctx, state, UINT(var1));
  xpath_ctx = EXTPTR(doc1);
  if (xpath_ctx==NULL)
    doc = NULL;
  else
    doc = xpath_ctx->doc;
  if (doc==NULL)
    {
      DebugLog(DF_MOD, DS_ERROR, "error : xmldoc is null\n");
      STACK_DROP(ctx->ovm_stack, 2);
      PUSH_RETURN_FALSE(ctx);
      return;
    }

  str2 = ovm_strdup (ctx->gc_ctx, var2);
  xpathObj = xmlXPathEvalExpression(BAD_CAST str2, xpath_ctx);
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

  /* This code is completely wrong: take this into account if
     you decide to uncomment. */
  var1 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack; 1);
  xmlDocFormatDump(stdout, ((xml_doc_t*)EXTPTR(var1))->doc, 1);
  STACK_DROP(ctx->ovm_stack);
}
#endif

static type_t t_xmldoc = { "xmldoc", T_UINT };

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

struct node_expr_s;
monotony m_xml_set (rule_compiler_t *ctx, struct node_expr_s *e, monotony m[])
{
  m[0] |= MONO_THRASH; /* thrashes the first argument (xml document) */
  return MONO_UNKNOWN | MONO_THRASH;
}

static void *mod_xml_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  DebugLog(DF_MOD, DS_INFO, "load() mod_xml@%p\n", (void *) &mod_xml);

  register_extern_class (ctx, &xml_xclass);
#if 0
  register_lang_function(ctx,
                         issdl_dump_xml,
                         "xml_dump",
			 1, xml_dump_sigs,
			 m_random_thrash,
                         "dump xml doc on stderr");
#endif

  register_lang_function(ctx,
                         issdl_xml_get_string,
                         "xml_get_str",
			 2, xml_get_str_sigs,
			 m_unknown_2,
                         "get a node content following an xpath request");

  register_lang_function(ctx,
                         issdl_xml_set_string,
                         "xml_set_str",
			 3, xml_set_str_sigs,
			 m_xml_set,
                         "set a node content following an xpath request");

  register_lang_function(ctx,
                         issdl_xml_set_attr_string,
                         "xml_set_prop",
			 4, xml_set_prop_sigs,
			 m_xml_set,
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
