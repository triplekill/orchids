/**
 ** @file mod_iodef.c
 ** Generation of IODEF reports.
 **
 ** @author Baptiste GOURDIN <baptiste.gourdin@inria.fr>
 ** @author Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
 **
 ** @version 0.2
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
#include <netdb.h>
#include <arpa/inet.h>
#ifndef PATH_MAX
#define PATH_MAX 8192
/* PATH_MAX is undefined on systems without a limit of filename length,
   such as GNU/Hurd.  Also, defining _XOPEN_SOURCE on Linux will make
   PATH_MAX undefined.
*/
#endif

#include <time.h>
#include <ctype.h>
#include <errno.h>

#include "orchids.h"
#include "orchids_api.h"
#include "ovm.h"
#include "evt_mgr.h"
#include "mod_mgr.h"
#include "util/hash.h"
#include "mod_utils.h"
#include "mod_iodef.h"
#include "file_cache.h"
#include "html_output.h"

// Buff used for variable expansion
#define BUFF_SIZE 1024

input_module_t mod_iodef;
static iodef_cfg_t *iodef_cfg;

static char *iodef_description = "iodef report";

static xmlDocPtr parse_iodef_template (iodef_cfg_t* cfg, char *fname, size_t len)
{
  struct stat st;
  size_t off;
  char file_path[PATH_MAX];
  xmlDocPtr doc;
  char *s;

  if (cfg->iodef_conf_dir==NULL)
    {
      DebugLog(DF_MOD, DS_ERROR, "IODEFTemplatesDir not set");
      return NULL;
    }
  if (stat(cfg->iodef_conf_dir, &st)!=0)
    {
      DebugLog(DF_MOD, DS_ERROR, strerror(errno));
      return NULL;
    }

  for (s=fname+len; --s>=fname;) /* remove slashes, and keep only final file name */
    if (s[0]=='/')
      {
	s++;
	len = fname+len-s;
	fname = s;
	break;
      }
  // Verify that the path with the new extension will fit in PATH_MAX
  off = cfg->iodef_conf_dir_len;
  if (len + off + 8 >= PATH_MAX) /* = one '/', a final ".iodef", and a NUL byte */
  {
    DebugLog(DF_MOD, DS_ERROR, "file path too long\n");
    return NULL;
  }

  memcpy (file_path, cfg->iodef_conf_dir, off);
  file_path[off] = '/';
  off++;
  memcpy (file_path+off, fname, len);
  off += len;
  strcpy (file_path+off, ".iodef");


  /* parse the file and get the DOM */
  doc = xmlReadFile(file_path, NULL, XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
  /* The flags given avoid seeing messages such as:
     I/O warning : failed to load external entity "/tmp/orchids/etc/orchids/iodef/pidtrack (2).iodef"
     Error reporting is done through DebugLog() below.
  */

  if (doc == NULL)
  {
    DebugLog(DF_MOD, DS_ERROR, "error parsing the iodef template file '%s'\n", file_path);
    return NULL;
  }

  return doc;
}

typedef struct xml_buf_s xml_buf_t;
struct xml_buf_s {
  gc_t *gc_ctx;
  char *txt;
  size_t end, sz;
} xml_buf_s;

void snprint_xml_verbatim (xml_buf_t *buf, char *s, size_t len)
{
  size_t newsz;

  newsz = buf->end+len;
  if (newsz > buf->sz)
    {
      if ((buf->sz <<= 1) < newsz)
	buf->sz = newsz;
      buf->txt = gc_base_realloc (buf->gc_ctx, buf->txt, buf->sz);
    }
  memcpy (buf->txt+buf->end, s, len);
  buf->end = newsz;
}

void snprint_xml_s_len (xml_buf_t *buf, char *s, size_t len)
{
  size_t i;
  int c;
  char *from;
  char escaped[8];

  for (i=0, from=s; i<len; i++)
    {
      c = s[i];
      if (isprint(c))
	switch (c)
	  {
	  case '<':
	    snprint_xml_verbatim (buf, from, s+i-from);
	    snprint_xml_verbatim (buf, "&lt;", 4);
	    from = s+1;
	    break;
	  case '>':
	    snprint_xml_verbatim (buf, from, s+i-from);
	    snprint_xml_verbatim (buf, "&gt;", 4);
	    from = s+1;
	    break;
	  case '&':
	    snprint_xml_verbatim (buf, from, s+i-from);
	    snprint_xml_verbatim (buf, "&amp;", 5);
	    from = s+1;
	    break;
	  case '"':
	    snprint_xml_verbatim (buf, from, s+i-from);
	    snprint_xml_verbatim (buf, "&quot;", 6);
	    from = s+1;
	    break;
	  case '\'':
	    snprint_xml_verbatim (buf, from, s+i-from);
	    snprint_xml_verbatim (buf, "&apos;", 6);
	    from = s+1;
	    break;
	  default:
	    break;
	  }
      else
	{
	  snprint_xml_verbatim (buf, from, s+i-from);
	  sprintf (escaped, "&#%d;", (int)c);
	  snprint_xml_verbatim (buf, escaped, strlen(escaped));
	  from = s+1;
	}
    }
  snprint_xml_verbatim (buf, from, s+len-from);
}

void snprint_xml_s (xml_buf_t *buf, char *s)
{
  snprint_xml_s_len (buf, s, strlen(s));
}

void snprint_ovm_var (xml_buf_t *buf, ovm_var_t *val)
/* imitated from fprintf_html_var() in html_output.c */
{
  char asc_time[32]; /* for date conversions; also integers, floats, ipv4 and ipv6,
			and unknown types */
  char dst[INET6_ADDRSTRLEN];

  if (val==NULL)
    {
      snprint_xml_s(buf, "(null)\n");
    }
  /* display data */
  else switch (val->gc.type)
	 {
	 case T_INT:
	   sprintf (asc_time, "%li", INT(val));
	   snprint_xml_s (buf, asc_time);
	   break;
	 case T_UINT:
	   sprintf (asc_time, "%lu", UINT(val));
	   snprint_xml_s (buf, asc_time);
	   break;
	 case T_BSTR: /* Do not print enclosing quotes */
	   snprint_xml_s_len (buf, (char *)BSTR(val), BSTRLEN(val));
	   break;
	 case T_VBSTR:
	   snprint_xml_s_len (buf, (char *)VBSTR(val), VBSTRLEN(val));
	   break;
	 case T_STR:
	   snprint_xml_s_len (buf, (char *)STR(val), STRLEN(val));
	   break;
	 case T_VSTR:
	   snprint_xml_s_len (buf, (char *)VSTR(val), VSTRLEN(val));
	   break;
	 case T_CTIME:
	   strftime(asc_time, 32,
		    "%a %b %d %H:%M:%S %Y", gmtime(&CTIME(val)));
	   snprint_xml_s(buf, asc_time);
	   break;
	 case T_IPV4:
	   sprintf(asc_time, "%s", inet_ntoa(IPV4(val)));
	   snprint_xml_s (buf, asc_time);
	   break;
	 case T_IPV6:
	   inet_ntop (AF_INET6, &IPV6(val), dst, sizeof(dst));
	   sprintf(asc_time, "%s", dst);
	   snprint_xml_s (buf, asc_time);
	   break;
	 case T_TIMEVAL:
	   strftime(asc_time, 32, "%a %b %d %H:%M:%S %Y",
		    gmtime(&TIMEVAL(val).tv_sec));
	   snprint_xml_s (buf, asc_time);
	   sprintf(asc_time, " +%li us", (long)TIMEVAL(val).tv_usec);
	   snprint_xml_s (buf, asc_time);
	   break;
	 case T_REGEX:
	   if (REGEXSTR(val)!=NULL)
	     {
	       snprint_xml_s (buf, "\"");
	       snprint_xml_s (buf, REGEXSTR(val));
	       snprint_xml_s (buf, "\"");
	     }
	   else snprint_xml_s (buf, "<regex>");
	   break;
	 case T_FLOAT:
	   sprintf(asc_time, "%f", FLOAT(val));
	   snprint_xml_s (buf, asc_time);
	   break;
	 default:
	   sprintf(asc_time, "<type %i>", val->gc.type);
	   snprint_xml_s (buf, asc_time);
	   break;
	 }
}

static void xml_expand_var (xml_buf_t *buf, state_instance_t *state, char *name, size_t namelen)
{ /* Variable name is given in name (length=namelen), with initial dollar sign included,
     e.g., $pid
  */
  int32_t v;
  char *vname;
  ovm_var_t *val;
  rule_t *rule = state->pid->rule;
  
  for (v = 0; v < rule->dynamic_env_sz; v++)
    {
      vname = rule->var_name[v];
      if (strncmp(name, vname, namelen)==0
	  && vname[namelen]=='\0')
	{
	  val = ovm_read_value (state->env, v);
	  if (val!=NULL)
	    snprint_ovm_var(buf, val);
	  break;
	}
    }
}

static char *xml_expand_vars (gc_t *gc_ctx, char *text, state_instance_t *state)
{
  char *var, *vp, c;
  xml_buf_t buf;

  if (text==NULL)
    return NULL;
  var = strchr (text, '$');
  if (var==NULL)
    return NULL;
  buf.gc_ctx = gc_ctx;
  buf.txt = gc_base_malloc (gc_ctx, buf.sz = 128);
  buf.end = 0;
  do {
    snprint_xml_verbatim (&buf, text, var-text);
    /* Here var[0]=='$', and we start a variable name */
    vp = var+1;
    while (1)
      {
	c = *vp;
	if (isalnum(c) || c=='_' || c=='.')
	  vp++;
	else break;
      }
    xml_expand_var (&buf, state, var, vp-var);
    text = vp;
    var = strchr (text, '$');
  } while (var!=NULL);
  snprint_xml_verbatim (&buf, text, strlen(text));
  snprint_xml_verbatim (&buf, "", 1); /* print final NUL byte */
  return buf.txt;
}

static void expand_iodef_template (orchids_t *ctx,
				   xmlNodePtr cur_node,
				   state_instance_t *state)
{
  xmlAttrPtr cur_attr = NULL;
  char *expanded;

  if (!xmlFirstElementChild(cur_node))
    {
      expanded = xml_expand_vars (ctx->gc_ctx, (char*)xmlNodeGetContent(cur_node), state);
      if (expanded!=NULL)
	{
	  xmlNodeSetContent(cur_node,
			    xmlEncodeEntitiesReentrant (cur_node->doc, BAD_CAST expanded));
	  gc_base_free (expanded);
	}
  }
  for (cur_attr = cur_node->properties; cur_attr; cur_attr = cur_attr->next)
    {
      expanded = xml_expand_vars (ctx->gc_ctx, (char*)xmlGetProp(cur_node, cur_attr->name), state);
      if (expanded!=NULL)
	{
	  xmlSetProp(cur_node, cur_attr->name,
		     xmlEncodeEntitiesReentrant (cur_node->doc, BAD_CAST expanded));
	  gc_base_free (expanded);
	}
    }
  // For each state node in the .iodef file
  for (cur_node = xmlFirstElementChild(cur_node); cur_node!=NULL;
       cur_node = xmlNextElementSibling(cur_node))
    expand_iodef_template(ctx, cur_node, state);
}

static void rXmlSetNs(xmlNode	*node,
		      xmlNs	*ns)
{
  if (node->type != XML_ELEMENT_NODE)
    return;
  node->ns = ns;

  for (node = xmlFirstElementChild(node); node;
       node = xmlNextElementSibling(node))
    rXmlSetNs(node, ns);
}

static void insert_template_report(orchids_t	*ctx,
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
    if (state!=NULL)
      expand_iodef_template(ctx, new_node, state);
    xmlAddChild(report_node, new_node);
  }
}

/**
 * Copy the node from the iodef template to the iodef report.
 * Expand all variables
 *
 * @param ctx		orchids context
 * @param report_ctx	report xpath context
 * @param template_node idoef template root node
 */
static void process_iodef_template (orchids_t		*ctx,
				    xmlXPathContext	*report_ctx,
				    xmlNode		*template_node,
				    state_instance_t	*report_events
				    )
{
  xmlNode	*insert_node = NULL;
#ifdef OBSOLETE
  xmlChar	*state_name = NULL;
#endif
  xmlChar	*xpath = NULL;

  // For each insert node in the state node
  for (insert_node = xmlFirstElementChild(template_node); insert_node!=NULL;
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

#ifdef OBSOLETE
    state_name = xmlGetProp(insert_node, BAD_CAST "state");
    if (state_name==NULL || strcmp ((char *)state_name, report_events->q->name)==0)
#endif
	insert_template_report(ctx, report_ctx, insert_node, xpath, report_events);
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
static void generate_and_write_report (orchids_t	*ctx,
				       mod_entry_t	*mod,
				       void		*data,
				       state_instance_t *state)
{
  xmlXPathContextPtr xpath_ctx;
  xmlDocPtr doc;
  iodef_cfg_t*	cfg;
  char		buff[PATH_MAX];
  struct timeval tv;
  unsigned long ntph;
  unsigned long ntpl;
  FILE*		fp;
  char *rulename, *s;

  cfg = (iodef_cfg_t *)mod->config;
  if (cfg->report_dir == NULL) {
    DebugLog(DF_MOD, DS_ERROR, "Output Dir isn't set. Aborting.\n");
    return ;
  }

  rulename = state->pid->rule->name;
  /* rulename may be of the form 'name' or 'name$n' (see rule_restore());
     in the latter case, we remove everything after and including the '$' */
  for (s=rulename; *s!=0 && *s!='$'; s++);
  xpath_ctx = iodef_generate_report(ctx, mod, state, rulename, s-rulename);
  if (xpath_ctx!=NULL)
    {
      doc = xpath_ctx->doc;
      gettimeofday(&tv, NULL);
      Timer_to_NTP(&tv, ntph, ntpl);

      // Write the report into the reports dir
      snprintf(buff, sizeof (buff), "%s/%s%08lx-%08lx%s",
	       cfg->report_dir, "report-", ntph, ntpl, ".xml");
    reopen:
      fp = fopen(buff, "w");
      if (fp==NULL)
	{
	  if (errno==EINTR) goto reopen;
	  DebugLog(DF_MOD, DS_ERROR, "Cannot open file '%s' for writing, %s.\n",
		   buff, strerror(errno));
	}
      else
	{
	  xmlDocFormatDump(fp, doc, 1);
	  (void) fclose(fp);
	}
      if (doc!=NULL)
	xmlFreeDoc (doc);
      xmlXPathFreeContext (xpath_ctx);      
    }
}

xmlXPathContextPtr iodef_generate_report(orchids_t	*ctx,
					 mod_entry_t	*mod,
					 state_instance_t *state,
					 char *fname,
					 size_t len)
{
  xmlDoc	*report_doc = NULL;
  xmlDoc	*iodef_doc = NULL;
  xmlNode	*report_root = NULL;
  xmlNode	*report_cur_node = NULL;
  xmlXPathContext *report_ctx = NULL;

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

  // Create ReportTime node (must be set at sending time)
  new_xs_datetime_node(report_cur_node->parent, "ReportTime",
  		       localtime(&tv.tv_sec));

  // Parse the iodef template and process it
  if (cfg->iodef_conf_dir && ((iodef_doc = parse_iodef_template (cfg, fname, len))
			      != NULL))
  {
    process_iodef_template (ctx, report_ctx,
  			    xmlDocGetRootElement(iodef_doc),
  			    state);
    xmlFreeDoc(iodef_doc);
  }

  return report_ctx;
}

static void issdl_generate_report(orchids_t *ctx, state_instance_t *state, void *data)
{
  xmlXPathContextPtr xpath_ctx;
  ovm_var_t *res, *arg;
  uint16_t handle;
  char *fname;
  size_t len;
  mod_entry_t *mod_entry = (mod_entry_t *)data;

#ifdef OBSOLETE
  if (!mod_entry)
    mod_entry = find_module_entry(ctx, "iodef");
#endif
  arg = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  fname = NULL;
  len = 0;
  if (arg!=NULL)
    switch (TYPE(arg))
      {
      case T_STR: fname = STR(arg); len = STRLEN(arg);
	break;
      case T_VSTR: fname = VSTR(arg); len = VSTRLEN(arg);
	break;
      default:
	DebugLog(DF_MOD, DS_DEBUG, "event value not a string\n");
	STACK_DROP(ctx->ovm_stack, 1);
	PUSH_RETURN_FALSE(ctx);
	return;
      }

  xpath_ctx = iodef_generate_report (ctx, mod_entry, state, fname, len);
  handle = ovm_xml_new (ctx->gc_ctx, state, xpath_ctx, iodef_description);
  res = ovm_uint_new (ctx->gc_ctx, handle);
  STACK_DROP(ctx->ovm_stack, 1);
  PUSH_VALUE(ctx, res);
}


static void issdl_iodef_write_report(orchids_t *ctx, state_instance_t *state, void *data)
{
  ovm_var_t	*var, *doc1;
  xmlXPathContextPtr xpath_ctx;
  xmlDocPtr doc;
  mod_entry_t *mod_entry = (mod_entry_t *)data;
  iodef_cfg_t*	cfg;
  char		buff[PATH_MAX];
  struct timeval tv;
  unsigned long ntph;
  unsigned long ntpl;
  FILE*		fp;

#ifdef OBSOLETE
  if (!mod_entry)
    mod_entry = find_module_entry(ctx, "iodef");
#endif
  cfg = (iodef_cfg_t *)mod_entry->config;

  var = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  if (var==NULL || TYPE(var)!=T_UINT)  /* an uint handle to an xmlXPathContextPtr */
    {
      DebugLog(DF_MOD, DS_ERROR, "first parameter is not an XML handle\n");
      STACK_DROP(ctx->ovm_stack, 1);
      PUSH_RETURN_FALSE(ctx);
    }

  if (cfg->report_dir == NULL)
    {
      DebugLog(DF_CORE, DS_ERROR, "Report Directory isn't set. Aborting.\n");
      STACK_DROP(ctx->ovm_stack, 1);
      PUSH_RETURN_FALSE(ctx);
      return ;
    }

  doc1 = handle_get_rd (ctx->gc_ctx, state, UINT(var));
  xpath_ctx = EXTPTR(doc1);
  doc = xpath_ctx->doc;

  gettimeofday(&tv, NULL);
  Timer_to_NTP(&tv, ntph, ntpl);

  // Write the report in the reports dir
  snprintf(buff, sizeof (buff), "%s/%s%08lx-%08lx%s",
	   cfg->report_dir, "report-", ntph, ntpl, ".xml");
 reopen:
  fp = fopen(buff, "w");
  if (fp==NULL)
    {
      if (errno==EINTR) goto reopen;
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

static int iodef_htmloutput(orchids_t *ctx, mod_entry_t *mod, FILE *menufp,
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
  if (cfg->report_dir == NULL)
    {
      DebugLog(DF_MOD, DS_ERROR, "IODEF Output isn't set. Aborting.\n");
      return 0;
    }
  DebugLog(DF_MOD, DS_INFO, "IODEF Output OK.\n");

  if (stat(cfg->report_dir, &s)!=0)
    {
      DebugLog(DF_MOD, DS_ERROR, strerror(errno));
      return 0;
    }
  fp = create_html_file (ctx, htmlcfg, "orchids-iodef.html", NO_CACHE);
  if (fp==NULL)
    {
      DebugLog(DF_MOD, DS_ERROR, "Cannot open file orchids-iodef.html\n");
      return 0;
    }

  fprintf_html_header(fp, "Orchids IODEF reports");
  fprintf(fp, "<center><h1>Orchids IODEF reports<h1></center>\n");
  n = scandir(cfg->report_dir, &namelist, NULL, alphasort);
  if (n < 0)
    {
      DebugLog (DF_MOD, DS_ERROR, strerror(errno));
    }
  else
    {
      fprintf(fp, "<center>\n"						\
	      "<div id=\"display_report\" style=\"position:absolute; "	\
	      "z-index:42;\"> </div>\n"					\
	      "<table border=\"0\" cellpadding=\"3\" width=\"600\">\n"	\
	      "  <tr class=\"h\"><th> Report list </th></tr>\n");

    for (i = 0; i < n; i++)
      {
	if ((!strcmp(namelist[i]->d_name, ".")) ||
	    (!strcmp(namelist[i]->d_name, "..")))
	  continue;
	snprintf(reportfile, sizeof (reportfile), "%s/%s",
		 cfg->report_dir , namelist[i]->d_name);
	if (stat(reportfile, &s)==0)
	  {
	    strftime(asc_time, sizeof (asc_time), "%a %b %d %H:%M:%S %Y",
		     localtime(&s.st_mtime));
	    fprintf(fp, "<tr><td class=\"e%i\"><a href=\"iodef/%s\">%s -> %s </a>" \
		    "<div style=\"display:none\" id=\"report_%i\"> </div></td></tr>\n",
		    i % 2, namelist[i]->d_name, asc_time, namelist[i]->d_name, i);
	  }
	free(namelist[i]);
    }
    free(namelist);
    fprintf(fp, "</table></center>\n");
  }

  fprintf_html_trailer(fp);

  (void) fclose(fp);

  fprintf(menufp,
  	  "<a href=\"orchids-iodef.html\" target=\"main\">IODEF</a><br/>\n");

  return 1;
}

static void set_iodef_conf_dir(orchids_t *ctx, mod_entry_t *mod,
			       config_directive_t *dir)
{
  iodef_cfg_t *cfg = (iodef_cfg_t *)mod->config;
  struct stat s;
  char *path;

  path = adjust_path (ctx, dir->args);
  if (stat(path, &s) < 0)
    {
      fprintf (stderr, "IODEFTemplatesDir: cannot find '%s'.\n", path);
      fflush (stderr);
      exit(EXIT_FAILURE);
    }
  if (!(s.st_mode & S_IFDIR))
    {
      fprintf (stderr, "IODEFTemplatesDir: '%s' is not a directory.\n", path);
      fflush (stderr);
      exit(EXIT_FAILURE);
    }
  cfg->iodef_conf_dir = path;
  cfg->iodef_conf_dir_len = strlen(cfg->iodef_conf_dir);
}

static void set_report_dir(orchids_t *ctx, mod_entry_t *mod,
			   config_directive_t *dir)
{
  iodef_cfg_t *cfg = (iodef_cfg_t *)mod->config;
  struct stat s;
  char *path;

  path = adjust_path (ctx, dir->args);
  if (stat(path, &s) < 0)
    {
      fprintf (stderr, "IODEFOutputDir: cannot find '%s'.\n", path);
      fflush (stderr);
      exit(EXIT_FAILURE);
    }
  if (!(s.st_mode & S_IFDIR))
    {
      fprintf (stderr, "IODEFOutputDir: '%s' is not a directory.\n", path);
      fflush (stderr);
      exit(EXIT_FAILURE);
    }
  cfg->report_dir = path;
}

static void set_csirt_name(orchids_t *ctx, mod_entry_t *mod,
			   config_directive_t *dir)
{
  iodef_cfg_t *cfg;

  cfg = (iodef_cfg_t *)mod->config;
  cfg->CSIRT_name = dir->args;
}

static type_t t_iodef = { "xmldoc", T_UINT }; /* convertible with xmldoc type */

static const type_t *iodef_new_report_sig[] = { &t_iodef, &t_str };
static const type_t **iodef_new_report_sigs[] = { iodef_new_report_sig, NULL };

static const type_t *iodef_write_report_sig[] = { &t_int, &t_iodef };
static const type_t **iodef_write_report_sigs[] = { iodef_write_report_sig, NULL };

static void *iodef_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  iodef_cfg_t *mod_cfg;

  DebugLog(DF_MOD, DS_INFO, "load() iodef@%p\n", (void *) &mod_iodef);

  /* Set IODEF as report output module */
  register_report_output(ctx, mod, generate_and_write_report, NULL);

  /* register html output */
  html_output_add_menu_entry(ctx, mod, iodef_htmloutput);

  register_lang_function(ctx,
			 issdl_generate_report,
			 "iodef_new_report",
			 1, iodef_new_report_sigs,
			 m_random,
			 "generate a report using the iodef template",
			 mod);

  register_lang_function(ctx,
			 issdl_iodef_write_report,
			 "iodef_write_report",
			 1, iodef_write_report_sigs,
			 m_random_thrash,
			 "write iodef report into the report folder",
			 mod);


  mod_cfg = gc_base_malloc(ctx->gc_ctx, sizeof (iodef_cfg_t));
  mod_cfg->CSIRT_name = NULL;
  mod_cfg->report_dir = NULL;
  mod_cfg->iodef_conf_dir = NULL;
  mod_cfg->iodef_conf_dir_len = 0;
  mod_cfg->schema = NULL;
    // XXX: Not implemented  => Use a contact database (xml file provided)
  mod_cfg->contacts = NULL;
#ifdef IODEF_FULLDUMP
  mod_cfg->full_dump = 0;
#endif
  iodef_cfg = mod_cfg;

  return (mod_cfg);
}

#ifdef IODEF_FULLDUMP
static void set_full_dump(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  int flag;

  DebugLog(DF_MOD, DS_INFO, "setting FullDump to %s\n", dir->args);

  flag = strtol(dir->args, (char **)NULL, 10);
  if (flag)
    ((iodef_cfg_t *)mod->config)->full_dump = 1;
}
#endif

static mod_cfg_cmd_t iodef_dir[] =
{
  { "IODEFTemplatesDir", set_iodef_conf_dir, "set IODEF templates directory" },
  { "IODEFOutputDir", set_report_dir, "write IODEF reports in a directory" },
  { "CSIRTName", set_csirt_name, "set CSIRT name." },
#ifdef IODEF_FULLDUMP
  { "FullDump", set_full_dump, "will output full dumps if non-zero" },
#endif
  { NULL, NULL }
};

char *iodef_deps[] = { "htmlstate", "xml", NULL };

input_module_t mod_iodef = {
  MOD_MAGIC,
  ORCHIDS_VERSION,
  0,			    /* flags */
  "iodef",
  "CeCILL2",
  iodef_deps,
  iodef_dir,
  iodef_preconfig,
  NULL, /* postconfig */
  NULL, /* postcompil */
  NULL, /* predissect */
  NULL, /* dissect */
  NULL, /* dissect type */
  NULL, /* save */
  NULL, /* restore */
};


/*
** Copyright (c) 2011 by Baptiste GOURDIN, Laboratoire Spécification
** et Vérification (LSV), CNRS UMR 8643 & ENS Cachan.
** Copyright (c) 2013-2016 by Jean GOUBAULT-LARRECQ, Laboratoire Spécification
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
