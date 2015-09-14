/**
 ** @file mod_idmef.c
 ** Parse IDMEF alerts
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#define _XOPEN_SOURCE 600 /* (ugly) for strptime(), included from <time.h>
			     on Linux/glibc2 only if this is defined */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h> // for PATH_MAX
#ifndef PATH_MAX
#define PATH_MAX 8192
/* PATH_MAX is undefined on systems without a limit of filename length,
   such as GNU/Hurd.  Also, defining _XOPEN_SOURCE on Linux will make
   PATH_MAX undefined.
*/
#endif

#include "ovm.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/xmlschemas.h>

#include "mod_mgr.h"
#include "mod_idmef.h"
#include "mod_utils.h"
#include "mod_xml.h"

input_module_t mod_idmef;

static type_t t_idmef = { "xmldoc", T_UINT }; /* convertible with xmldoc type */

field_t idmef_fields[MAX_IDMEF_FIELDS] = {
#ifdef OBSOLETE
  { "idmef.ptr", &t_idmef, MONO_UNKNOWN, "idmef xml doc"   }
#endif
};

static ovm_var_t* parse_idmef_datetime(gc_t *gc_ctx, char *datetime)
{
  struct tm tm;
  char	*datetime_ = datetime;
  int	h;
  int	m;
  ovm_var_t	*res;

  memset(&tm, 0, sizeof(tm));
  tm.tm_isdst = -1;

  datetime = strptime(datetime, "%Y-%m-%dT%H:%M:%S", &tm);

  if (datetime==NULL)
  {
    DebugLog(DF_MOD, DS_ERROR, "Malformed idmef datetime (%s)\n", datetime_);
    return NULL;
  }

  if ((*datetime == '.') ||
      (*datetime == ','))
  {
    // skip fraction seconds
    datetime++;
    while (isdigit(*datetime))
      datetime++;
  }

  if ((*datetime == '+') ||
      (*datetime == '-'))
  {
    if (sscanf(datetime + 1, "%2u:%2u", &h, &m) != 2)
      DebugLog(DF_MOD, DS_ERROR, "Malformed idmef datetime (%s)\n", datetime_);
    if (*datetime == '+')
    {
      tm.tm_min -= m;
      tm.tm_hour -= h;
    }
    else
    {
      tm.tm_min += m;
      tm.tm_hour += h;
    }
  }

  res = ovm_ctime_new(gc_ctx, mktime(&tm));
  return res;
}

static int load_idmef_xmlDoc(orchids_t *ctx,
			     mod_entry_t *mod,
			     const char	*txt_line,
			     const size_t txt_len,
			     int dissector_level)
{
  idmef_cfg_t	*cfg;
  xmlDocPtr	doc = NULL;
  xmlXPathContextPtr	xpath_ctx = NULL;
  int		c;
  gc_t *gc_ctx = ctx->gc_ctx;
  ovm_var_t *val;

  cfg = mod->config;
  doc = xmlReadMemory(txt_line, txt_len, "idmef", NULL, 0);
  if (doc==NULL)
    {
      DebugLog(DF_MOD, DS_ERROR, "Error loading IDMEF xml doc\n");
      return 0;
    }
  xpath_ctx = xmlXPathNewContext(doc);
  if (xpath_ctx==NULL)
    {
      xmlFreeDoc(doc);
      DebugLog(DF_MOD, DS_ERROR, "Error creating XPath context\n");
      return 0;
    }
  xmlXPathRegisterNs(xpath_ctx, BAD_CAST ("idmef"),
		     BAD_CAST ("http://iana.org/idmef"));

  GC_START(gc_ctx, MAX_IDMEF_FIELDS+1);
#ifdef OBSOLETE
  val = ovm_xml_new (gc_ctx, doc, xpath_ctx, xml_desc());
  GC_UPDATE (gc_ctx, F_PTR, val);
#endif

  for (c = 0 /* OBSOLETE: 1 */; c < cfg->nb_fields; c++)
  {
    char	*text;
    int		text_len;

    if ((text = xml_get_string(xpath_ctx, cfg->field_xpath[c])) != NULL)
    {
      switch (idmef_fields[c].type->tag)
      {
	case T_STR :
	case T_VSTR :
	  text_len = strlen(text);
	  val = ovm_str_new(gc_ctx, text_len);
	  memcpy (STR(val), text, text_len);
	  GC_UPDATE (gc_ctx, c, val);
	  break;
	case T_CTIME :
	  val = parse_idmef_datetime(gc_ctx, text);
	  GC_UPDATE (gc_ctx, c, val);
	  break;
	case T_IPV4 :
	  val = ovm_ipv4_new(gc_ctx);
	  IPV4(val).s_addr = inet_addr(text);
	  GC_UPDATE (gc_ctx, c, val);
	  break;
	case T_INT :
	  {
	    long i;

	    sscanf(text, "%li", &i);
	    val = ovm_int_new(gc_ctx, i);
	    GC_UPDATE (gc_ctx, c, val);
	    break;
	  }
	case T_UINT :
	  {
	    unsigned long i;

	    sscanf(text, "%lu", &i);
	    val = ovm_uint_new(gc_ctx, i);
	    GC_UPDATE (gc_ctx, c, val);
	    break;
	  }
      }
      xmlFree (text);
    }
  }

  REGISTER_EVENTS(ctx, mod, MAX_IDMEF_FIELDS, dissector_level);
  GC_END(gc_ctx);
  return 1;
}

#ifdef OBSOLETE
static char *my_strnstr (char *text, size_t len, char *pattern)
{
  size_t patlen = strlen(pattern);
  char *end = text + len - patlen;
  int res;

  while (text <= end)
    {
      res = memcmp(text, pattern, patlen);
      if (res==0)
	return text;
      text++;
    }
  return NULL;
}
#endif

static const unsigned char idmef_end_marker[] = "</idmef:IDMEF-Message>";

static size_t idmef_compute_length (unsigned char *first_bytes,
				    size_t n_first_bytes,
				    size_t available_bytes,
				    /* available_bytes is ignored here */
				    int *state,
				    void *sd_data)
{
  int i;
  
  switch (*state)
    {
    case BLOX_INIT:
      /* n_first_bytes==1 */
    case BLOX_NOT_ALIGNED: /* should not happen */
    case BLOX_FINAL: /* should not happen */
      if (first_bytes[0]==idmef_end_marker[0])
	*state = BLOX_NSTATES+1; /* already matched first character of
				    idmef_end_marker */
      else *state = BLOX_NSTATES;
      return n_first_bytes+1;
    default: /* in all cases, *states==BLOX_NSTATES+i,
		where i is the length of the prefix of idmef_end_marker
		that we have recognized */
      i = *state - BLOX_NSTATES;
      if (first_bytes[n_first_bytes-1]==idmef_end_marker[i])
	{
	  i++;
	  if (idmef_end_marker[i]=='\0')
	    {
	      *state = BLOX_FINAL; /* we have found a match */
	      return n_first_bytes;
	    }
	  *state = BLOX_NSTATES+i;
	  return n_first_bytes+1; /* read one more character */
	}
      else
	{ /* no match: since idmef_end_marker only contains one '<' at
	     the beginning, return to BLOX_NSTATES state (or BLOX_NSTATES+1
	     if last character added is '<'). For more complicated
	     strings, one should use Simon's automaton construction. */
	  if (first_bytes[n_first_bytes-1]==idmef_end_marker[0])
	    *state = BLOX_NSTATES+1; /* already matched first character of
					idmef_end_marker */
	  else *state = BLOX_NSTATES;
	  return n_first_bytes+1;
	}
    }
}

static void idmef_subdissect (orchids_t *ctx, mod_entry_t *mod,
			      event_t *event,
			      ovm_var_t *delegate,
			      unsigned char *stream, size_t stream_len,
			      void *sd_data,
			      int dissector_level)
{
  load_idmef_xmlDoc(ctx, mod, (char *)stream, stream_len, dissector_level);
}

static int idmef_dissect (orchids_t *ctx, mod_entry_t *mod,
			  event_t *event, void *data, int dissector_level)
{
  return blox_dissect (ctx, mod, event, data, dissector_level);
}

static void *idmef_predissect(orchids_t *ctx, mod_entry_t *mod,
			      char *parent_modname,
			      char *cond_param_str,
			      int cond_param_size)
{
  blox_hook_t *hook;
  idmef_cfg_t *cfg = (idmef_cfg_t *)mod->config;

  hook = init_blox_hook (ctx, cfg->bcfg, cond_param_str, cond_param_size);
  return hook;
}

static int idmef_save (save_ctx_t *sctx, mod_entry_t *mod, void *data)
{
  idmef_cfg_t *cfg = (idmef_cfg_t *)data;
  
  return blox_save (sctx, cfg->bcfg);
}

static int idmef_restore (restore_ctx_t *rctx, mod_entry_t *mod, void *data)
{
  idmef_cfg_t *cfg = (idmef_cfg_t *)data;
  
  return blox_restore (rctx, cfg->bcfg);
}

static void add_analyzer_node(xmlNode *alert_root,
			      idmef_cfg_t *cfg)
{
  xmlNode *ana_node = NULL;
  xmlNode *node_node = NULL;
  xmlChar *xmlEncStr = NULL;

  ana_node = xmlNewChild(alert_root, alert_root->ns, BAD_CAST "Analyzer", NULL);
  xmlNewProp(ana_node, BAD_CAST "analyzerid", BAD_CAST cfg->analyzer_id);
  xmlNewProp(ana_node, BAD_CAST "name", BAD_CAST cfg->analyzer_name);
  node_node = xmlNewChild(ana_node, alert_root->ns, BAD_CAST "Node", NULL);
  if (cfg->analyzer_node_location)
  {
    xmlEncStr = xmlEncodeEntitiesReentrant(alert_root->doc,
					   BAD_CAST cfg->analyzer_node_location);
    xmlNewChild(node_node, alert_root->ns, BAD_CAST "location", xmlEncStr);
    xmlFree(xmlEncStr);
  }
  if (cfg->analyzer_node_name)
  {
    xmlEncStr = xmlEncodeEntitiesReentrant(alert_root->doc,
					   BAD_CAST cfg->analyzer_node_name);
    xmlNewChild(node_node, alert_root->ns, BAD_CAST "name", xmlEncStr);
    xmlFree(xmlEncStr);
  }

  if (cfg->analyzer_node_address)
  {
    xmlNode	*addr_node = NULL;

    addr_node = xmlNewChild(node_node, alert_root->ns, BAD_CAST "Address", NULL);
    xmlNewProp(addr_node, BAD_CAST "category", BAD_CAST "ipv4-addr");
    xmlEncStr = xmlEncodeEntitiesReentrant(alert_root->doc,
					   BAD_CAST cfg->analyzer_node_address);
    xmlNewChild(addr_node, alert_root->ns, BAD_CAST "address", xmlEncStr);
    xmlFree(xmlEncStr);
  }
}

ovm_var_t *idmef_generate_alert(orchids_t	*ctx,
				mod_entry_t	*mod,
				state_instance_t *state)
{
  xmlDoc	*alert_doc = NULL;
  xmlNode	*alert_root = NULL;
  xmlNode	*cur_node = NULL;
  xmlXPathContext *alert_ctx = NULL;
  char		buff[PATH_MAX];
  xmlNs		*ns;
  struct timeval tv;
  unsigned long ntph;
  unsigned long ntpl;
  idmef_cfg_t	*cfg = mod->config;
  uint16_t handle;


  gettimeofday(&tv, NULL);
  Timer_to_NTP(&tv, ntph, ntpl);

  // Create xml document and root node
  alert_doc = xmlNewDoc(BAD_CAST "1.0");
  alert_ctx = xmlXPathNewContext(alert_doc);
  xmlXPathRegisterNs(alert_ctx, BAD_CAST ("idmef"),
		     BAD_CAST ("http://iana.org/idmef"));
  alert_root = xmlNewNode(NULL, BAD_CAST "IDMEF-Message");
  ns = xmlNewNs(alert_root,
		BAD_CAST "http://iana.org/idmef", BAD_CAST "idmef");
  xmlSetNs(alert_root, ns);

  xmlDocSetRootElement(alert_doc,alert_root);

  cur_node = xmlNewChild(alert_root, alert_root->ns, BAD_CAST "Alert", NULL);
  snprintf(buff, sizeof(buff), "%s-%08lx-%08lx", cfg->analyzer_id, ntph, ntpl);
  xmlNewProp(cur_node, BAD_CAST "messageid",
	     xmlEncodeEntities(alert_root->doc,
			       BAD_CAST buff));

  add_analyzer_node(cur_node, mod->config);

  snprintf(buff, sizeof(buff), "%08lx.%08lx", ntph, ntpl);
  // Create DetectTime node
  cur_node = new_xs_datetime_node(cur_node, "CreateTime",
				  localtime(&tv.tv_sec));
  xmlNewProp(cur_node, BAD_CAST "ntpstamp",
	     xmlEncodeEntities(alert_root->doc,
			       BAD_CAST buff));

  // Create DetectTime node
  cur_node = new_xs_datetime_node(cur_node->parent, "DetectTime",
				  localtime(&tv.tv_sec));

  xmlNewProp(cur_node, BAD_CAST "ntpstamp",
	     xmlEncodeEntities(alert_root->doc,
			       BAD_CAST buff));

  handle = ovm_xml_new (ctx->gc_ctx, state, alert_ctx, xml_desc());
  return ovm_uint_new (ctx->gc_ctx, (unsigned long)handle);
}


static void issdl_idmef_new_alert(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t	*res;
  static mod_entry_t	*mod_entry = NULL;

  if (mod_entry==NULL)
    mod_entry = find_module_entry(ctx, "idmef");
  res = idmef_generate_alert (ctx, mod_entry, state);
  PUSH_VALUE(ctx, res);
}

static void issdl_idmef_write_alert(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t	*var, *doc1;
  xmlXPathContextPtr xpath_ctx;
  xmlDocPtr doc;
  static mod_entry_t	*mod_entry = NULL;
  idmef_cfg_t*	cfg;
  char		buff[PATH_MAX];
  struct timeval tv;
  unsigned long ntph;
  unsigned long ntpl;
  FILE*		fp;

  if (!mod_entry)
    mod_entry = find_module_entry(ctx, "idmef");

  cfg = (idmef_cfg_t *)mod_entry->config;
  var = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  if (var==NULL || TYPE(var)!=T_UINT) /* an uint handle to an xmlXPathContextPtr */
    {
      DebugLog(DF_MOD, DS_ERROR, "parameter error\n");
      STACK_DROP(ctx->ovm_stack, 1);
      PUSH_RETURN_FALSE(ctx);
    }
  else if (cfg->report_dir==NULL)
    {
      DebugLog(DF_MOD, DS_ERROR, "Report Directory isn't set. Aborting.\n");
      STACK_DROP(ctx->ovm_stack, 1);
      PUSH_RETURN_FALSE(ctx);
    }
  else
    {
      doc1 = handle_get_rd (ctx->gc_ctx, state, UINT(var));
      xpath_ctx = EXTPTR(doc1);
      if (xpath_ctx==NULL)
	doc = NULL;
      else
	doc = xpath_ctx->doc;

      if (doc!=NULL)
	{
	  gettimeofday(&tv, NULL);
	  Timer_to_NTP(&tv, ntph, ntpl);

	  // Write the report into the reports dir
	  snprintf(buff, sizeof (buff), "%s/%s%08lx-%08lx%s",
		   cfg->report_dir, "report-", ntph, ntpl, ".xml");
	  fp = fopen(buff, "w");
	  if (fp==NULL)
	    {
	      DebugLog(DF_MOD, DS_ERROR, "Cannot open file '%s' for writing, %s.\n",
		       buff, strerror(errno));
	      STACK_DROP(ctx->ovm_stack, 1);
	      PUSH_RETURN_FALSE(ctx);
	    }
	  else
	    {
	      xmlDocFormatDump(fp, doc, 1);
	      (void) fclose(fp);
	      STACK_DROP(ctx->ovm_stack, 1);
	      PUSH_RETURN_TRUE(ctx);
	    }
	}
    }
}

static const type_t *idmef_new_alert_sig[] = { &t_idmef };
static const type_t **idmef_new_alert_sigs[] = { idmef_new_alert_sig, NULL };

static const type_t *idmef_write_alert_sig[] = { &t_int, &t_idmef };
static const type_t **idmef_write_alert_sigs[] = { idmef_write_alert_sig, NULL };

static void *idmef_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  idmef_cfg_t *cfg;
  blox_config_t *bcfg;
  int i;

  DebugLog(DF_MOD, DS_INFO, "load() idmef@%p\n", (void *) &mod_idmef);

  bcfg = init_blox_config (ctx, mod, 1,
			   idmef_compute_length,
			   idmef_subdissect,
			   NULL);

  cfg = gc_base_malloc(ctx->gc_ctx, sizeof (idmef_cfg_t));
  cfg->nb_fields = 0; /* OBSOLETE: 1; */
  for (i=0; i<MAX_IDMEF_FIELDS; i++)
    cfg->field_xpath[i] = NULL;
  cfg->bcfg = bcfg;
  cfg->analyzer_id = 0;
  cfg->analyzer_name = NULL;
  cfg->analyzer_node_address = NULL;
  cfg->analyzer_node_location = NULL;
  cfg->analyzer_node_name = NULL;
  cfg->report_dir = NULL;

  register_lang_function(ctx,
			 issdl_idmef_new_alert,
			 "idmef_new_alert",
			 0, idmef_new_alert_sigs,
			 m_random,
			 "generate an idmef alert");

  register_lang_function(ctx,
			 issdl_idmef_write_alert,
			 "idmef_write_alert",
			 1, idmef_write_alert_sigs,
			 m_random_thrash,
			 "write idmef alert into the report folder");

  return cfg;
}

static void idmef_postconfig(orchids_t *ctx, mod_entry_t *mod)
{
  DebugLog(DF_MOD, DS_INFO, "load() idmef %i\n",
	   ((idmef_cfg_t *)mod->config)->nb_fields );

  register_fields(ctx, mod, idmef_fields,
		  ((idmef_cfg_t*)mod->config)->nb_fields);
}

static void add_field(orchids_t *ctx, char *field_name, char *path,
		      idmef_cfg_t *cfg, type_t *type)
{
  if (cfg->nb_fields == MAX_IDMEF_FIELDS)
  {
    DebugLog(DF_MOD, DS_WARN,
	     "Max number of idmef fields reached, cannot add %s %s\n",
	     field_name, path);
    return;
  }

  {
    char *s;

    s = gc_base_malloc(ctx->gc_ctx, 7 + strlen(field_name));
    idmef_fields[cfg->nb_fields].name = s;
    memcpy (s, "idmef.", 6);
    strcpy (s+6, field_name);

    idmef_fields[cfg->nb_fields].type = type;
    idmef_fields[cfg->nb_fields].desc = path;

    DebugLog(DF_MOD, DS_INFO, "add new field %i (%s) : %s\n",
	     cfg->nb_fields,
	     idmef_fields[cfg->nb_fields].name , path);
    cfg->field_xpath[cfg->nb_fields++] = path;
  }
}

static void dir_add_field(orchids_t *ctx, mod_entry_t *mod,
			  config_directive_t *dir)
{
  char* pos;

  for (pos = dir->args; *pos!='\0' && !isblank(*pos); pos++)
    continue;
  if (!pos)
  {
    DebugLog(DF_MOD, DS_ERROR, "Error when parsing directive field\n");
    return;
  }
  *pos = '\0';

  if (!strcmp(dir->directive, "str_field"))
    add_field(ctx, dir->args, pos + 1, mod->config, &t_str);
  else if (!strcmp(dir->directive, "time_field"))
    add_field(ctx, dir->args, pos + 1, mod->config, &t_ctime);
  else if (!strcmp(dir->directive, "ipv4_field"))
    add_field(ctx, dir->args, pos + 1, mod->config, &t_ipv4);
  else if (!strcmp(dir->directive, "int_field"))
    add_field(ctx, dir->args, pos + 1, mod->config, &t_int);
}

static void dir_set_analyzer_info(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  idmef_cfg_t*	cfg = mod->config;

  if (!strcmp(dir->directive, "AnalyzerId"))
    cfg->analyzer_id = dir->args;
  else if (!strcmp(dir->directive, "AnalyzerName"))
    cfg->analyzer_name = dir->args;
  else if (!strcmp(dir->directive, "AnalyzerNodeLocation"))
    cfg->analyzer_node_location = dir->args;
  else if (!strcmp(dir->directive, "AnalyzerNodeAddress"))
    cfg->analyzer_node_address = dir->args;
  else if (!strcmp(dir->directive, "AnalyzerNodeName"))
    cfg->analyzer_node_name = dir->args;
  DebugLog(DF_MOD, DS_INFO, "%s set to %s\n", dir->directive, dir->args);
}

static void dir_set_report_dir(orchids_t *ctx, mod_entry_t *mod,
			       config_directive_t *dir)
{
  idmef_cfg_t*	cfg = mod->config;
  struct stat	s;

  Xstat(dir->args, &s);

  if (!(s.st_mode & S_IFDIR))
  {
    DebugLog(DF_MOD, DS_ERROR, "%s is not a directory\n", dir->args);
    exit(EXIT_FAILURE);
  }
  cfg->report_dir = dir->args;
}

static mod_cfg_cmd_t idmef_cfgcmds[] = {
  { "str_field", dir_add_field, "add a new field corresponding to a XPath query" },
  { "time_field", dir_add_field, "add a new field corresponding to a XPath query" },
  { "ipv4_field", dir_add_field, "add a new field corresponding to a XPath query" },
  { "int_field", dir_add_field, "add a new field corresponding to a XPath query" },

  { "AnalyzerId", dir_set_analyzer_info, "set analyzer id" },
  { "AnalyzerName", dir_set_analyzer_info, "set analyzer name" },
  { "AnalyzerNodeLocation", dir_set_analyzer_info, "set analyzer location" },
  { "AnalyzerNodeAddress", dir_set_analyzer_info, "set analyzer address" },
  { "AnalyzerNodeName", dir_set_analyzer_info, "set analyzer node name" },

  { "IDMEFOutputDir", dir_set_report_dir, "write IDMEF reports into given directory" },
  { NULL, NULL, NULL }
};

char *idmef_deps[] = { "xml", NULL };

input_module_t mod_idmef = {
  MOD_MAGIC,
  ORCHIDS_VERSION,
  0,			    /* flags */
  "idmef",
  "CeCILL2",
  idmef_deps,
  idmef_cfgcmds,
  idmef_preconfig,
  idmef_postconfig,
  NULL,
  idmef_predissect,
  idmef_dissect,
  &t_str,		    /* type of fields it expects to dissect */
  idmef_save,
  idmef_restore,
};

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
