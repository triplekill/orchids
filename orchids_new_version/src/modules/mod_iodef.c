/**
 ** @file mod_iodef.c
 ** Generation of IODEF reports.
 **
 ** @author Baptiste GOURDIN <baptiste.gourdin@inria.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 ** @date  Started on: Wed Jan 15 17:07:26 2003
 ** @date Last update: Thu Aug  2 23:31:16 2007
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <ctype.h>

#include "orchids.h"
#include "orchids_api.h"
#include "evt_mgr.h"
#include "mod_mgr.h"
#include "util/hash.h"
#include "mod_iodef.h"
#include "file_cache.h"
#include "html_output.h"

// Buff used for variable expantion
#define BUFF_SIZE 1024

input_module_t mod_iodef;
static iodef_cfg_t *iodef_cfg;

static xmlDocPtr
parse_iodef_template (iodef_cfg_t* cfg, rule_t *rule)
{
  struct stat	s;
  char		file_path[PATH_MAX];
  char*		ext = NULL;
  xmlDocPtr	doc;

  Xstat(cfg->iodef_conf_dir, &s);

  // Verify that the path with the new extension will fit in PATH_MAX
  if (strlen(rule->filename) + cfg->iodef_conf_dir_len + 2 >= PATH_MAX)
  {
    DebugLog(DF_MOD, DS_ERROR, "file path too long\n");
    return NULL;
  }

  snprintf(file_path, PATH_MAX, "%s/%s", cfg->iodef_conf_dir,
	   strrchr(rule->filename, '/'));

  // Set the new extension
  ext = strrchr(file_path, '.');
  if (ext == NULL)
  {
    DebugLog(DF_MOD, DS_ERROR, "rule filename format error\n");
    return NULL;
  }
  *ext = 0;
  strcat(file_path, ".iodef");


  /* parse the file and get the DOM */
  doc = xmlReadFile(file_path, NULL, 0);

  if (doc == NULL)
  {
    DebugLog(DF_MOD, DS_ERROR, "error parsing the iodef template file\n");
    return NULL;
  }

  return doc;
}

static char
expand_var (orchids_t		*ctx,
	    char		*text,
	    state_instance_t	*state,
	    char		*buff,
	    unsigned int	buff_size)
{
  unsigned int	i;
  unsigned int	v;
  unsigned int	buff_offset = 0;
  unsigned int	text_offset = 0;
  char		c;

  if ((!text) || (!strchr(text, '$')))
    return 0;

  memset(buff, 0, buff_size);

  while (text[text_offset] != 0)
  {
   if (buff_offset >= buff_size)
      return (1);
    if (text[text_offset] != '$')
      buff[buff_offset++] = text[text_offset++];
    else
    {

      // Get the full variable name
      for (i = 1;
	   ((text[text_offset] != 0)
	    && ((text[text_offset + i] == '.')
		|| (text[text_offset + i] == '_')
		|| (isalnum(text[text_offset + i]))));
	   i++)
	continue ;

      c = text[text_offset + i];
      text[text_offset + i] = '\0';

      // Retreive the last variable value and add it the the buffer
      for (v = 0; v < state->rule_instance->rule->dynamic_env_sz; ++v)
      {
	if (!strcmp(text + text_offset + 1,
		    state->rule_instance->rule->var_name[v]))
	{
	  if (state->current_env[v])
	    buff_offset += snprintf_ovm_var(buff + buff_offset,
					    buff_size - buff_offset,
					    state->current_env[v]);
	  else if (state->inherit_env[v])
	    buff_offset += snprintf_ovm_var(buff + buff_offset,
					    buff_size - buff_offset,
					    state->inherit_env[v]);
	}
      }

      if (buff_offset >= buff_size)
	return (1);

      text_offset += i;
      buff[buff_offset++] = c;
      if (c)
	text_offset++;
    }
  }

  return (1);
}

static void
expand_iodef_template (orchids_t *ctx,
		       xmlNodePtr cur_node,
		       state_instance_t *state)
{
  xmlAttrPtr	cur_attr = NULL;
  char		buff[BUFF_SIZE];

  if (!xmlFirstElementChild(cur_node))
  {
    if (expand_var(ctx, (char*)xmlNodeGetContent(cur_node),
		   state, buff, BUFF_SIZE))
      xmlNodeSetContent(cur_node,
			xmlEncodeEntitiesReentrant (cur_node->doc, BAD_CAST buff));
  }
  for (cur_attr = cur_node->properties; cur_attr; cur_attr = cur_attr->next)
    if (expand_var(ctx, (char*)xmlGetProp(cur_node, cur_attr->name),
		  state, buff, BUFF_SIZE))
      xmlSetProp(cur_node, cur_attr->name,
		 xmlEncodeEntitiesReentrant (cur_node->doc, BAD_CAST buff));

  // For each state node in the .iodef file
  for (cur_node = xmlFirstElementChild(cur_node); cur_node;
       cur_node = xmlNextElementSibling(cur_node))
    expand_iodef_template(ctx, cur_node, state);
}

static void
rXmlSetNs(xmlNode	*node,
	  xmlNs	*ns)
{
  if (node->type != XML_ELEMENT_NODE)
    return;
  node->ns = ns;

  for (node = xmlFirstElementChild(node); node;
       node = xmlNextElementSibling(node))
    rXmlSetNs(node, ns);
}

static void
insert_template_report(orchids_t	*ctx,
		       xmlXPathContext	*report_ctx,
		       xmlNode		*insert_node,
		       xmlChar		*xpath,
		       state_instance_t	*state)
{
  xmlNode	*report_node = NULL;
  xmlNode	*copy_node = NULL;

  // Follow the path and create nodes if needed
  if (!(report_node = xml_walk_and_create(report_ctx, xpath)))
    return;

  for (copy_node = xmlFirstElementChild(insert_node); copy_node;
       copy_node = xmlNextElementSibling(copy_node))
  {
    xmlNode	*new_node = NULL;
    if (copy_node->type != XML_ELEMENT_NODE)
      continue;

    new_node = xmlCopyNode(copy_node, 1);
    rXmlSetNs(new_node, report_node->ns);
    if (state)
      expand_iodef_template(ctx, new_node, state);
    xmlAddChild(report_node, new_node);
  }
}

/**
 * Copy the node from the iodef template to the iodef report.
 * Expand all variables and fields
 *
 * @param ctx		orchids context
 * @param report_ctx	report xpath context
 * @param template_node idoef template root node
 * @param report_events	first report state instance
 */
static void
process_iodef_template (orchids_t		*ctx,
			xmlXPathContext		*report_ctx,
			xmlNode			*template_node,
			state_instance_t	*report_events)
{
  xmlNode	*insert_node = NULL;
  xmlChar	*state_name = NULL;
  xmlChar	*xpath = NULL;
  state_instance_t	*state;
  state_instance_t	*last_state = report_events;

  // For each insert node in the state node
  for (insert_node = xmlFirstElementChild(template_node); insert_node;
       insert_node = xmlNextElementSibling(insert_node))
  {
    if (insert_node->type != XML_ELEMENT_NODE)
      continue;

    xpath = xmlGetProp(insert_node, BAD_CAST "path");
    if (xpath == NULL)
    {
      DebugLog(DF_CORE, DS_ERROR,
	       "The insert node does not have a path attribute\n");
      continue;
    }

    if ((state_name = xmlGetProp(insert_node, BAD_CAST "state")) != NULL)
    {
      for (state = report_events; state ;
	   state = state->next_report_elmt)
	if (!strcmp((char*)state_name, state->state->name))
	  insert_template_report(ctx, report_ctx, insert_node, xpath, state);
    }
    else
      insert_template_report(ctx, report_ctx, insert_node, xpath, last_state);
  }
}

/**
 * Generate an iodef xml document from the report
 * and write it into the iodef reports folder
 *
 * @param ctx	Orchids context
 * @param mod	iodef module entry
 * @param data  Unused
 * @param state First report state instance
 */
static void
generate_and_write_report (orchids_t	*ctx,
			   mod_entry_t	*mod,
			   void		*data,
			   state_instance_t *state)
{
  xml_doc_t	*report;
  iodef_cfg_t*	cfg;
  char		buff[PATH_MAX];
  struct timeval tv;
  unsigned long ntph;
  unsigned long ntpl;
  FILE*		fp;

  cfg = (iodef_cfg_t *)mod->config;
  if (cfg->report_dir == NULL) {
    DebugLog(DF_CORE, DS_ERROR, "Output Dir isn't set. Aborting.\n");
    return ;
  }

  if ((report = generate_report(ctx, mod, state)))
  {
    gettimeofday(&tv, NULL);
    Timer_to_NTP(&tv, ntph, ntpl);

    // Write the report in the reports dir
    snprintf(buff, sizeof (buff), "%s/%s%08lx-%08lx%s",
	     cfg->report_dir, "report-", ntph, ntpl, ".xml");
    fp = Xfopen(buff, "w");
    xmlDocFormatDump(fp, report->doc, 1);
    Xfclose(fp);
  }
//XXX FREE REPORT
}

xml_doc_t*
generate_report(orchids_t	*ctx,
		mod_entry_t	*mod,
		state_instance_t *state)
{
  xmlDoc	*report_doc = NULL;
  xmlDoc	*iodef_doc = NULL;
  xmlNode	*report_root = NULL;
  xmlNode	*report_cur_node = NULL;
  xmlXPathContext *report_ctx = NULL;
  xml_doc_t	*xml_doc = NULL;

  char		buff[PATH_MAX];
  iodef_cfg_t*	cfg;
  struct timeval tv;
  unsigned long ntph;
  unsigned long ntpl;
  xmlNs		*ns;

  cfg = (iodef_cfg_t *)mod->config;

  gettimeofday(&tv, NULL);
  Timer_to_NTP(&tv, ntph, ntpl);

  // Create xml document and root node
  report_doc = xmlNewDoc(BAD_CAST "1.0");

  report_ctx = xmlXPathNewContext(report_doc);
  xmlXPathRegisterNs(report_ctx, BAD_CAST ("iodef"),
		     BAD_CAST ("urn:ietf:params:xml:ns:iodef-1.0"));
  report_root = xmlNewNode(NULL, BAD_CAST "IODEF-Document");


  xmlDocSetRootElement(report_doc, report_root);
  xmlNewProp(report_root, BAD_CAST "version", BAD_CAST "1.00");
  xmlNewProp(report_root, BAD_CAST "lang", BAD_CAST "en");
  xmlNewNs(report_root,
  	   BAD_CAST "urn:ietf:params:xml:ns:iodef-1.0", NULL);
  ns = xmlNewNs(report_root,
  	   BAD_CAST "urn:ietf:params:xml:ns:iodef-1.0", BAD_CAST "iodef");
  xmlNewNs(report_root,
  	   BAD_CAST "http://www.w3.org/2001/XMLSchema-instance", BAD_CAST "xsi");
  xmlNewProp(report_root,
  	     BAD_CAST "xsi:schemaLocation",
  	     BAD_CAST "urn:ietf:params:xml:schema:iodef-1.0");
  xmlSetNs(report_root, ns);

  // Create Incident node
  report_cur_node = xmlNewChild(report_root, NULL, BAD_CAST "Incident", NULL);
  xmlNewProp(report_cur_node, BAD_CAST "purpose", BAD_CAST "reporting");
  xmlSetNs(report_cur_node, ns);


  // Create IncidentID node
  snprintf(buff, sizeof(buff), "%s-%08lx-%08lx", cfg->CSIRT_name, ntph, ntpl);
  report_cur_node = xmlNewChild(report_cur_node, NULL,
				BAD_CAST "IncidentID",
				xmlEncodeEntitiesReentrant(report_doc,
							   BAD_CAST buff));
  xmlNewProp(report_cur_node, BAD_CAST "name",  BAD_CAST cfg->CSIRT_name);
  xmlNewProp(report_cur_node, BAD_CAST "restriction",  BAD_CAST ("public"));

  // Create DetectTime node
  new_xs_datetime_node(report_cur_node->parent, "DetectTime",
  		       localtime(&tv.tv_sec));

  // Create StartTime node (can be overwitten by the template)
  new_xs_datetime_node(report_cur_node->parent, "StartTime",
  		       localtime(&tv.tv_sec));

  // Create ReportTime node (Must be set at sending time)
  new_xs_datetime_node(report_cur_node->parent, "ReportTime",
  		       localtime(&tv.tv_sec));

  // Parse the iodef template and process it
  if (cfg->iodef_conf_dir && ((iodef_doc = parse_iodef_template (cfg, state->rule_instance->rule))
			      != NULL))
  {
    process_iodef_template (ctx, report_ctx,
  			    xmlDocGetRootElement(iodef_doc),
  			    state);
     xmlFreeDoc(iodef_doc);
  }

  xml_doc = Xzmalloc(sizeof(xml_doc_t));
  xml_doc->doc = report_doc;
  xml_doc->xpath_ctx = report_ctx;

  return xml_doc;
}

static void
issdl_generate_report(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t	*res;
  static mod_entry_t	*mod_entry = NULL;

  if (!mod_entry)
    mod_entry = find_module_entry(ctx, "iodef");

  res = ovm_xml_new ();
  EXTPTR(res) = generate_report(ctx, mod_entry, state);
  stack_push(ctx->ovm_stack, res);
}


static void
issdl_iodef_write_report(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t	*var;
  xml_doc_t	*report;
  static mod_entry_t	*mod_entry = NULL;
  iodef_cfg_t*	cfg;
  char		buff[PATH_MAX];
  struct timeval tv;
  unsigned long ntph;
  unsigned long ntpl;
  FILE*		fp;

  if (!mod_entry)
    mod_entry = find_module_entry(ctx, "iodef");

  cfg = (iodef_cfg_t*)mod_entry->config;

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

static int
iodef_htmloutput(orchids_t *ctx, mod_entry_t *mod, FILE *menufp,
		 html_output_cfg_t *htmlcfg)
{
  FILE *fp;
  struct dirent **namelist;
  int n;
  int i;
  char reportfile[1024];
  char asc_time[32];
  struct stat s;
  iodef_cfg_t* cfg;

  cfg = (iodef_cfg_t *)mod->config;
  if (cfg->report_dir == NULL) {
    printf("IODEF Output isn't set. Aborting.\n");
    DebugLog(DF_CORE, DS_ERROR, "IODEF Output isn't set. Aborting.\n");
    return 0;
  }
  DebugLog(DF_CORE, DS_ERROR, "IODEF Output OK.\n");

  Xstat(cfg->report_dir, &s);
  fp = create_html_file(htmlcfg, "orchids-iodef.html", NO_CACHE);
  fprintf_html_header(fp, "Orchids IODEF reports");
  fprintf(fp, "<center><h1>Orchids IODEF reports<h1></center>\n");
  n = scandir(cfg->report_dir, &namelist, NULL, alphasort);
  if (n < 0)
    perror("scandir()");
  else {
    fprintf(fp, "<center>\n" \
	    "<div id=\"display_report\" style=\"position:absolute; "	\
	    "z-index:42;\"> </div>\n"					\
	    "<table border=\"0\" cellpadding=\"3\" width=\"600\">\n"	\
	    "  <tr class=\"h\"><th> Report list </th></tr>\n");

    for (i = 0; i < n; i++) {
      if ((!strcmp(namelist[i]->d_name, ".")) ||
	  (!strcmp(namelist[i]->d_name, "..")))
	continue;
      snprintf(reportfile, sizeof (reportfile), "%s/%s",
              cfg->report_dir , namelist[i]->d_name);
      Xstat(reportfile, &s);
      strftime(asc_time, sizeof (asc_time), "%a %b %d %H:%M:%S %Y",
               localtime(&s.st_mtime));
      fprintf(fp, "<tr><td class=\"e%i\"><a href=\"iodef/%s\">%s -> %s </a>" \
	      "<div style=\"display:none\" id=\"report_%i\"> </div></td></tr>\n",
              i % 2, namelist[i]->d_name, asc_time, namelist[i]->d_name, i);
      free(namelist[i]);
    }
    free(namelist);
    fprintf(fp, "</table></center>\n");
  }

  fprintf_html_trailer(fp);

  Xfclose(fp);

  fprintf(menufp,
  	  "<a href=\"orchids-iodef.html\" target=\"main\">IODEF</a><br/>\n");

  return 1;
}

static void
set_iodef_conf_dir(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  iodef_cfg_t *cfg;

  cfg = (iodef_cfg_t *)mod->config;
  cfg->iodef_conf_dir = strdup(dir->args);
  cfg->iodef_conf_dir_len = strlen(cfg->iodef_conf_dir);
}

static void
set_report_dir(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  iodef_cfg_t *cfg;

  cfg = (iodef_cfg_t *)mod->config;
  cfg->report_dir = strdup(dir->args);
}

static void
set_csirt_name(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  iodef_cfg_t *cfg;

  cfg = (iodef_cfg_t *)mod->config;
  cfg->CSIRT_name = dir->args;
}

static void *
iodef_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  iodef_cfg_t *mod_cfg;

  DebugLog(DF_MOD, DS_INFO, "load() iodef@%p\n", (void *) &mod_iodef);

  /* Set IODEF as report output module */
  register_report_output(ctx, mod, generate_and_write_report, NULL);

  /* register html output */
  html_output_add_menu_entry(ctx, mod, iodef_htmloutput);

  register_lang_function(ctx,
			issdl_generate_report,
			"iodef_new_report", 0,
			"generate a report using the iodef template");

  register_lang_function(ctx,
			issdl_iodef_write_report,
			"iodef_write_report", 0,
			"write iodef report in the report folder");


  mod_cfg = Xzmalloc(sizeof (iodef_cfg_t));
  iodef_cfg = mod_cfg;

  return (mod_cfg);
}

static mod_cfg_cmd_t iodef_dir[] =
{
  { "IODEFTemplatesDir", set_iodef_conf_dir, "Set IODEF templates directory"},
  { "IODEFOutputDir", set_report_dir, "Write IODEF reports in a directory"},
  { "CSIRTName", set_csirt_name, "Set CSIRT name."},
  { NULL, NULL }
};

input_module_t mod_iodef = {
  MOD_MAGIC,
  ORCHIDS_VERSION,
  "iodef",
  "CeCILL2",
  NULL,
  iodef_dir,
  iodef_preconfig,
  NULL,
  NULL
};

