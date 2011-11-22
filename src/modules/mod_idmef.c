/**
 ** @file mod_idmef.c
 ** Parse IDMEF alerts
 **
 ** @author Baptiste GOURDIN <gourdin@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <ctype.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/xmlschemas.h>

#include "mod_mgr.h"
#include "mod_idmef.h"
#include "mod_xml.h"

input_module_t mod_idmef;

field_t idmef_fields[MAX_IDMEF_FIELDS] = {
  { "idmef.ptr",		    T_EXTERNAL,"idmef xml doc"   }
};

static ovm_var_t*
parse_idmef_datetime(char	*datetime)
{
  struct tm tm;
  char	*datetime_ = datetime;
  int	h;
  int	m;
  ovm_var_t	*res;

  memset(&tm, 0, sizeof(tm));
  tm.tm_isdst = -1;

  datetime = strptime(datetime, "%Y-%m-%dT%H:%M:%S", &tm);

  if (!datetime)
  {
    DebugLog(DF_MOD, DS_ERROR, "Malformated idmef datetime (%s)\n", datetime_);
    return NULL;
  }

  if ((*datetime == '.') ||
      (*datetime == ','))
  {
    // skip fraction seconds
    datetime++;
    while ( isdigit(*datetime) )
      datetime++;
  }

  if ((*datetime == '+') ||
      (*datetime == '-'))
  {
    if (sscanf(datetime + 1, "%2u:%2u", &h, &m) != 2)
      DebugLog(DF_MOD, DS_ERROR, "Malformated idmef datetime (%s)\n", datetime_);
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

  res = ovm_ctime_new();
  CTIME(res) = mktime(&tm);
  return res;
}

static int
load_idmef_xmlDoc(orchids_t	*ctx,
		  mod_entry_t	*mod,
		  const char	*txt_line,
		  const size_t	txt_len)
{
  idmef_cfg_t	*cfg;
  xmlDocPtr	doc = NULL;
  ovm_var_t	*attr[MAX_IDMEF_FIELDS];
  event_t	*event;
  xmlXPathContextPtr	xpath_ctx = NULL;
  xml_doc_t	*xml_doc = NULL;
  int		c;

  cfg = mod->config;
  if (((doc = xmlReadMemory(txt_line, txt_len, "idmef", NULL, 0)) == NULL)
      || ((xpath_ctx = xmlXPathNewContext(doc)) == NULL))
  {
    fprintf(stdout, "Error loading IDMEF xml doc \n");
    return (0);
  }

  xml_doc = Xzmalloc(sizeof (xml_doc_t));
  xml_doc->doc = doc;
  xml_doc->xpath_ctx = xpath_ctx;

  xmlXPathRegisterNs(xpath_ctx, BAD_CAST ("idmef"),
		     BAD_CAST ("http://iana.org/idmef"));

  memset(attr, 0, sizeof(attr));
  attr[F_PTR] = ovm_extern_new();
  EXTPTR(attr[F_PTR]) = xml_doc;
  EXTFREE(attr[F_PTR]) = free_xml_doc;

  for (c = 1; c < cfg->nb_fields; c++)
  {
    char	*text;
    int		text_len;
    ovm_var_t	*res;

    if ((text = xml_get_string(xml_doc, cfg->field_xpath[c])) != NULL)
    {
      switch (idmef_fields[c].type)
      {
	case T_STR :
	case T_VSTR :
	  text_len = strlen(text);
	  res = ovm_str_new(text_len);
	  memcpy (STR(res), text, text_len);
	  attr[c] = res;
	  break;
	case T_CTIME :
	{
	  attr[c] = parse_idmef_datetime(text);
	  break;
	}
	case T_IPV4 :
	  attr[c] = ovm_ipv4_new();
	  IPV4(attr[c]).s_addr = inet_addr(text);
	  break;
	case T_INT :
	  attr[c] = ovm_int_new();
	  sscanf(text, "%li", &(INT(attr[c])));
	  break;
	case T_UINT :
	  attr[c] = ovm_uint_new();
	  sscanf(text, "%lu", &(UINT(attr[c])));
	  break;
      }
      xmlFree (text);
    }
  }

  event = NULL;
  add_fields_to_event(ctx, mod, &event, attr, cfg->nb_fields);
  post_event(ctx, mod, event);

  return (1);
}

static int
dissect_idmef(orchids_t		*ctx,
	      mod_entry_t	*mod,
	      event_t		*event,
	      void		*data)
{
  char		*line;
  char		*end;
  idmef_cfg_t	*cfg;

  cfg = mod->config;
  line = STR(event->value);

  while ((end = strstr(line, "</idmef:IDMEF-Message>")) != NULL)
  {
    if (cfg->buff_len +  end - line + 22 >= MAX_IDMEF_SIZE)
      DebugLog(DF_MOD, DS_ERROR, "idmef message too big\n");
    else
    {
      strncat (cfg->buff, line, end - line + 22);
      load_idmef_xmlDoc(ctx, mod, cfg->buff, strlen(cfg->buff));
    }
    cfg->buff[0] = 0;
    cfg->buff_len = 0;
    line = end + 22;
    while (*line && (*line != '<'))
      line ++;

  }
  strcat (cfg->buff, line);

  return (1);
}

int
generic_dissect(orchids_t *ctx, mod_entry_t *mod, event_t *event, void *data)
{
  return dissect_idmef(ctx, mod, event, data);
}

static void
add_analyzer_node(xmlNode	*alert_root,
		  idmef_cfg_t	*cfg)
{
  xmlNode	*ana_node = NULL;
  xmlNode	*node_node = NULL;
  xmlChar	*xmlEncStr = NULL;

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

xml_doc_t*
generate_alert(orchids_t	*ctx,
	       mod_entry_t	*mod,
	       state_instance_t *state)
{
  xmlDoc	*alert_doc = NULL;
  xmlNode	*alert_root = NULL;
  xmlNode	*cur_node = NULL;
  xmlXPathContext *alert_ctx = NULL;
  xml_doc_t	*xml_doc = NULL;
  char		buff[PATH_MAX];

  xmlNs		*ns;
  struct timeval tv;
  unsigned long ntph;
  unsigned long ntpl;
  idmef_cfg_t	*cfg = mod->config;


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


  xml_doc = Xzmalloc(sizeof(xml_doc_t));
  xml_doc->doc = alert_doc;
  xml_doc->xpath_ctx = alert_ctx;

  return (xml_doc);
}


static void
issdl_idmef_new_alert(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t	*res;
  static mod_entry_t	*mod_entry = NULL;

  if (!mod_entry)
    mod_entry = find_module_entry(ctx, "idmef");

  res = ovm_xml_new ();
  EXTPTR(res) = generate_alert(ctx, mod_entry, state);

  stack_push(ctx->ovm_stack, res);
}

static void
issdl_idmef_write_alert(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t	*var;
  xml_doc_t	*report;
  static mod_entry_t	*mod_entry = NULL;
  idmef_cfg_t*	cfg;
  char		buff[PATH_MAX];
  struct timeval tv;
  unsigned long ntph;
  unsigned long ntpl;
  FILE*		fp;

  if (!mod_entry)
    mod_entry = find_module_entry(ctx, "idmef");

  cfg = (idmef_cfg_t*)mod_entry->config;

  var = stack_pop(ctx->ovm_stack);
  if (!var || (TYPE(var) != T_EXTERNAL) || !EXTPTR(var))
  {
    DebugLog(DF_ENG, DS_ERROR, "parameter error\n");
    ISSDL_RETURN_FALSE(ctx, state);
  }

  if (cfg->report_dir == NULL) {
    DebugLog(DF_CORE, DS_ERROR, "Report Directory isn't set. Aborting.\n");
    return ;
  }

  report = EXTPTR(var);

  gettimeofday(&tv, NULL);
  Timer_to_NTP(&tv, ntph, ntpl);

  // Write the report in the reports dir
  snprintf(buff, sizeof (buff), "%s/%s%08lx-%08lx%s",
	   cfg->report_dir, "report-", ntph, ntpl, ".xml");
  fp = Xfopen(buff, "w");
  xmlDocFormatDump(fp, report->doc, 1);
  Xfclose(fp);

  ISSDL_RETURN_TRUE(ctx, state);
}

static void *
idmef_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  idmef_cfg_t*	cfg;

  DebugLog(DF_MOD, DS_INFO, "load() idmef@%p\n", (void *) &mod_idmef);

  cfg = Xzmalloc(sizeof (idmef_cfg_t));
  cfg->nb_fields = 1;

  register_lang_function(ctx,
			 issdl_idmef_new_alert,
			 "idmef_new_alert", 0,
			 "generate an idmef alert");

  register_lang_function(ctx,
			 issdl_idmef_write_alert,
			 "idmef_write_alert", 0,
			 "write idmef alert in the report folder");

  return (cfg);
}

static void
idmef_postconfig(orchids_t *ctx, mod_entry_t *mod)
{
  DebugLog(DF_MOD, DS_INFO, "load() idmef %i\n", ((idmef_cfg_t*)mod->config)->nb_fields );

  register_fields(ctx, mod, idmef_fields, ((idmef_cfg_t*)mod->config)->nb_fields);
}

static void
add_field(char* field_name, char* path, idmef_cfg_t*	cfg, int type)
{
  if (cfg->nb_fields == MAX_IDMEF_FIELDS)
  {
    DebugLog(DF_MOD, DS_WARN, "Max number of idmef fields reached, cannot add %s %s\n", field_name , path);
    return;
  }

  idmef_fields[cfg->nb_fields].name = Xzmalloc((9 + strlen(field_name))
							  * sizeof(char));
  strcat(idmef_fields[cfg->nb_fields].name, "idmef.");
  strcat(idmef_fields[cfg->nb_fields].name, field_name);

  idmef_fields[cfg->nb_fields].type = type;
  idmef_fields[cfg->nb_fields].desc = path;

  DebugLog(DF_MOD, DS_INFO, "add new field %i (%s) : %s\n",
	   cfg->nb_fields,
	   idmef_fields[cfg->nb_fields].name , path);
  cfg->field_xpath[cfg->nb_fields++] = path;
}

static void
dir_add_field(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  char* pos;

  for (pos = dir->args; *pos && !isblank(*pos); pos++)
    continue;
  if (!pos)
  {
    DebugLog(DF_MOD, DS_ERROR, "Error when parsing directive field\n");
    return;
  }
  *pos = '\0';

  if (!strcmp(dir->directive, "str_field"))
    add_field(dir->args, pos + 1, mod->config, T_STR);
  else if (!strcmp(dir->directive, "time_field"))
    add_field(dir->args, pos + 1, mod->config, T_CTIME);
  else if (!strcmp(dir->directive, "ipv4_field"))
    add_field(dir->args, pos + 1, mod->config, T_IPV4);
  else if (!strcmp(dir->directive, "int_field"))
  add_field(dir->args, pos + 1, mod->config, T_INT);
}

static void
dir_set_analyzer_info(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
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
static void
dir_set_report_dir(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
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
  { "str_field", dir_add_field, "Add a new field corresponding to a XPath query."},
  { "time_field", dir_add_field, "Add a new field corresponding to a XPath query."},
  { "ipv4_field", dir_add_field, "Add a new field corresponding to a XPath query."},
  { "int_field", dir_add_field, "Add a new field corresponding to a XPath query."},

  { "AnalyzerId", dir_set_analyzer_info, "Set analyzer id."},
  { "AnalyzerName", dir_set_analyzer_info, "Set analyzer name."},
  { "AnalyzerNodeLocation", dir_set_analyzer_info, "Set analyzer location."},
  { "AnalyzerNodeAddress", dir_set_analyzer_info, "Set analyzer address."},
  { "AnalyzerNodeName", dir_set_analyzer_info, "Set analyzer node name."},

  { "IDMEFOutputDir", dir_set_report_dir, "Write IDMEF reports in the directory."},
  { NULL, NULL, NULL }
};


static char *idmef_deps[] = {
  "udp",
  "textfile",
  NULL
};


input_module_t mod_idmef = {
  MOD_MAGIC,
  ORCHIDS_VERSION,
  "idmef",
  "CeCILL2",
  idmef_deps,
  idmef_cfgcmds,
  idmef_preconfig,
  idmef_postconfig,
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
