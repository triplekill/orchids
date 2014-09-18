/**
 ** @file lang.c
 ** The intrusion scenario signature definition language.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 1.0
 ** @ingroup engine
 **
 ** @date  Started on: Mon Feb  3 18:11:19 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <time.h> /* for strftime() and localtime() */
#include <ctype.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <limits.h> // for PATH_MAX
#ifndef PATH_MAX
#define PATH_MAX 8192
/* PATH_MAX is undefined on systems without a limit of filename length,
   such as GNU/Hurd.  Also, defining _XOPEN_SOURCE on Linux will make
   PATH_MAX undefined.
*/
#endif

#include "orchids.h"
#include "hash.h"
#include "safelib.h"
#include "timer.h"
#include "graph_output.h"
#include "orchids_api.h"

#include "ovm.h"
#include "lang.h"
#include "lang_priv.h"

/**
 ** Table of data types natively recognized in the Orchids language.
 **/
static struct issdl_type_s issdl_types_g[] = {
  { "null",    0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "Null type for error/exception management" },
  { "func",    0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "Function reference" },
  { "int",     0, int_get_data, int_get_data_len, int_cmp, int_add, int_sub, int_opp, int_mul, int_div, int_mod, int_clone, int_and, int_or, int_xor, int_not,
    "Integer numbers (32-bits signed int)" },
  { "bstr",    0, bytestr_get_data, bytestr_get_data_len, NULL, NULL, NULL, NULL, NULL, NULL, NULL, bstr_clone, NULL, NULL, NULL, NULL,
    "Binary string, allocated, (unsigned char *)" },
  { "vbstr",   0, vbstr_get_data, vbstr_get_data_len, NULL, NULL, NULL, NULL, NULL, NULL, NULL, vbstr_clone, NULL, NULL, NULL, NULL,
    "Virtual binary string, not allocated, only pointer/offset reference" },
  { "str",     0, string_get_data, string_get_data_len, str_cmp, str_add, NULL, NULL, NULL, NULL, NULL, string_clone, NULL, NULL, NULL, NULL,
    "Character string, allocated, (char *)" },
  { "vstr",    0, vstring_get_data, vstring_get_data_len, vstr_cmp, vstr_add, NULL, NULL, NULL, NULL, NULL, vstr_clone, NULL, NULL, NULL, NULL,
    "Virtual string, not allocated, only pointer/offset reference" },
  { "array",   0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "Array" },
  { "hash",    0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "Hash table" },
  { "ctime",   0, ctime_get_data, ctime_get_data_len, ctime_cmp, ctime_add, ctime_sub, NULL, NULL, NULL, NULL, ctime_clone, NULL, NULL, NULL, NULL,
    "C Time, seconds since Epoch (Jan. 1, 1970, 00:00 GMT), (time_t)" },
  { "ipv4",    0, ipv4_get_data, ipv4_get_data_len, ipv4_cmp, NULL, NULL, NULL, NULL, NULL, NULL, ipv4_clone, ipv4_and, ipv4_or, ipv4_xor, ipv4_not,
    "IPv4 address (struct in_addr)" },
  { "ipv6",    0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "IPv6 address (struct in6_addr)" },
  { "timeval", 0, timeval_get_data, timeval_get_data_len, timeval_cmp, timeval_add, timeval_sub, NULL, NULL, NULL, NULL, timeval_clone, NULL, NULL, NULL, NULL,
    "Seconds and microseconds since Epoch, (struct timeval)" },
  { "regex",   0, regex_get_data, regex_get_data_len, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "Posix Extended Regular Expression, with substring addressing" },
  { "uint",    0, uint_get_data, uint_get_data_len, uint_cmp, uint_add, uint_sub, NULL, uint_mul, uint_div, uint_mod, uint_clone, uint_and, uint_or, uint_xor, uint_not,
    "Non negative integer (32-bits unsigned int)" },
  { "snmpoid", 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "SNMP Object Identifier" },
  { "float",   0, float_get_data, float_get_data_len, float_cmp, float_add, float_sub, float_opp, float_mul, float_div, NULL, float_clone, NULL, NULL, NULL, NULL,
    "IEEE 32-bit floating point number (float)" },
  { "event", 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "Meta event" },
  { "state_instance", 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "State instance/cut mark" },
  { "extern",  0, extern_get_data, extern_get_data_len, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "External data (provided by a plugin)" },
  { NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "" }
};

static int resolve_ipv4_g = 0;
static int resolve_ipv6_g = 0;

char *
str_issdltype(int type)
{
  return issdl_types_g[type].name;
}

issdl_type_t *issdlgettypes(void)
{
  return issdl_types_g;
}

/*
** Integer type
** store a 'signed int' value.
*/

ovm_var_t *ovm_int_new(gc_t *gc_ctx, long val)
{
  ovm_int_t *n;

  n = gc_alloc (gc_ctx, sizeof (ovm_int_t), NULL);
  n->gc.type = T_INT;
  n->val = val;
  return OVM_VAR(n);
}

static void *int_get_data(ovm_var_t *i)
{
  return &((ovm_int_t *)i)->val;
}

static size_t int_get_data_len(ovm_var_t *i)
{
  return sizeof (long);
}

void ovm_int_fprintf (FILE *fp, ovm_int_t *val)
{
  if (val==NULL || val->gc.type != T_INT)
    {
      fprintf(fp, "Wrong object type.\n");
      return;
    }

  fprintf(fp, "int : %li\n", val->val);
}

static int int_cmp(ovm_var_t *var1, ovm_var_t *var2)
{
  if (var2==NULL)
    return INT(var1);
  if (TYPE(var2) != T_INT)
    return TYPE(var1) - TYPE(var2);
  return INT(var1) - INT(var2);
}

static ovm_var_t *int_add (gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  if (var2==NULL || var2->gc.type != T_INT)
    return NULL;
  return ovm_int_new (gc_ctx, INT(var1) + INT(var2));
}

static ovm_var_t *int_sub (gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  if (var2==NULL || var2->gc.type != T_INT)
    return NULL;
  return ovm_int_new (gc_ctx, INT(var1) - INT(var2));
}

static ovm_var_t *int_opp (gc_t *gc_ctx, ovm_var_t *var)
{
  return ovm_int_new (gc_ctx, -INT(var));
}

static ovm_var_t *int_mul (gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  if (var2==NULL || var2->gc.type != T_INT)
    return NULL;
  return ovm_int_new (gc_ctx, INT(var1) * INT(var2));
}

static ovm_var_t *int_div (gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  if (var2==NULL || var2->gc.type != T_INT || INT(var2)==0)
    return NULL;
  return ovm_int_new (gc_ctx, INT(var1) / INT(var2));
}

static ovm_var_t *int_mod (gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  if (var2==NULL || var2->gc.type != T_INT || INT(var2)==0)
    return NULL;
  return ovm_int_new (gc_ctx, INT(var1) % INT(var2));
}

static ovm_var_t *int_clone(gc_t *gc_ctx, ovm_var_t *var)
{
  if (var==NULL || var->gc.type != T_INT)
    return NULL;
  return ovm_int_new (gc_ctx, INT(var));
}

static ovm_var_t *int_and (gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  long mask;

  if (var2==NULL)
    return NULL;
  switch (TYPE(var2))
    {
    case T_INT: mask = INT(var2); break;
    case T_UINT: mask = (long)UINT(var2); break;
    case T_IPV4: mask = (long)IPV4(var2).s_addr; break;
    default: return NULL;
    }
  return ovm_int_new (gc_ctx, INT(var1) & mask);
}

static ovm_var_t *int_or (gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  long mask;

  if (var2==NULL)
    return NULL;
  switch (TYPE(var2))
    {
    case T_INT: mask = INT(var2); break;
    case T_UINT: mask = (long)UINT(var2); break;
    case T_IPV4: mask = (long)IPV4(var2).s_addr; break;
    default: return NULL;
    }
  return ovm_int_new (gc_ctx, INT(var1) | mask);
}

static ovm_var_t *int_xor (gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  long mask;

  if (var2==NULL)
    return NULL;
  switch (TYPE(var2))
    {
    case T_INT: mask = INT(var2); break;
    case T_UINT: mask = (long)UINT(var2); break;
    case T_IPV4: mask = (long)IPV4(var2).s_addr; break;
    default: return NULL;
    }
  return ovm_int_new (gc_ctx, INT(var1) ^ mask);
}

static ovm_var_t *int_not(gc_t *gc_ctx, ovm_var_t *var)
{
  if (var==NULL || TYPE(var) != T_INT)
    return NULL;
  return ovm_int_new (gc_ctx, ~INT(var));
}

/*
** Unsigned integer type
** store an 'uint32_t' value.
*/

ovm_var_t *ovm_uint_new(gc_t *gc_ctx, unsigned long val)
{
  ovm_uint_t *n;

  n = gc_alloc (gc_ctx, sizeof (ovm_uint_t), NULL);
  n->gc.type = T_UINT;
  n->val = val;
  return OVM_VAR(n);
}

static void *uint_get_data(ovm_var_t *i)
{
  return &((ovm_uint_t *)i)->val;
}

static size_t uint_get_data_len(ovm_var_t *i)
{
  return sizeof (unsigned long);
}

void ovm_uint_fprintf(FILE *fp, ovm_uint_t *val)
{
  if (val==NULL || val->gc.type != T_UINT)
    {
      fprintf(fp, "Wrong object type.\n");
      return;
    }
  fprintf(fp, "uint : %lu\n", val->val);
}

static int uint_cmp (ovm_var_t *var1, ovm_var_t *var2)
{
  if (var2==NULL)
    return UINT(var1);
  if (TYPE(var2) != T_UINT)
    return TYPE(var1) - TYPE(var2);
  return UINT(var1) - UINT(var2);
}

static ovm_var_t *uint_add (gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  if (var2==NULL || var2->gc.type != T_UINT)
    return NULL;
  return ovm_uint_new (gc_ctx, UINT(var1) + UINT(var2));
}

static ovm_var_t *uint_sub (gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  if (var2==NULL || var2->gc.type != T_UINT)
    return NULL;
  return ovm_uint_new (gc_ctx,
		       (UINT(var1)>=UINT(var2))?
		       (UINT(var1) - UINT(var2)):0);
}

static ovm_var_t *uint_mul (gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  if (var2==NULL || var2->gc.type != T_UINT)
    return NULL;
  return ovm_uint_new (gc_ctx, UINT(var1) * UINT(var2));
}

static ovm_var_t *uint_div (gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  if (var2==NULL || var2->gc.type != T_UINT || UINT(var2)==0)
    return NULL;
  return ovm_uint_new (gc_ctx, UINT(var1) / UINT(var2));
}

static ovm_var_t *uint_mod (gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  if (var2==NULL || TYPE(var2) != T_UINT || UINT(var2)==0)
    return NULL;
  return ovm_uint_new (gc_ctx, UINT(var1) % UINT(var2));
}

static ovm_var_t *uint_clone (gc_t *gc_ctx, ovm_var_t *var)
{
  if (var==NULL || TYPE(var) != T_UINT)
    return NULL;
  return ovm_uint_new (gc_ctx, UINT(var));
}

static ovm_var_t *uint_and (gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  unsigned long mask;

  if (var2==NULL)
    return NULL;
  switch (TYPE(var2))
    {
    case T_INT: mask = (unsigned long)INT(var2); break;
    case T_UINT: mask = UINT(var2); break;
    case T_IPV4: mask = (unsigned long)IPV4(var2).s_addr; break;
    default: return NULL;
    }
  return ovm_uint_new (gc_ctx, INT(var1) & mask);
}

static ovm_var_t *uint_or (gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  unsigned long mask;

  if (var2==NULL)
    return NULL;
  switch (TYPE(var2))
    {
    case T_INT: mask = (unsigned long)INT(var2); break;
    case T_UINT: mask = UINT(var2); break;
    case T_IPV4: mask = (unsigned long)IPV4(var2).s_addr; break;
    default: return NULL;
    }
  return ovm_uint_new (gc_ctx, INT(var1) | mask);
}

static ovm_var_t *uint_xor (gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  unsigned long mask;

  if (var2==NULL)
    return NULL;
  switch (TYPE(var2))
    {
    case T_INT: mask = (unsigned long)INT(var2); break;
    case T_UINT: mask = UINT(var2); break;
    case T_IPV4: mask = (unsigned long)IPV4(var2).s_addr; break;
    default: return NULL;
    }
  return ovm_uint_new (gc_ctx, INT(var1) ^ mask);
}

static ovm_var_t *uint_not(gc_t *gc_ctx, ovm_var_t *var)
{
  if (var==NULL || TYPE(var) != T_UINT)
    return NULL;
  return ovm_int_new (gc_ctx, ~UINT(var));
}

/*
** Binary String
** used to store binary objects.
*/

ovm_var_t *ovm_bstr_new (gc_t *gc_ctx, size_t size)
{
  ovm_bstr_t *bstr;

  bstr = gc_alloc (gc_ctx, offsetof(ovm_bstr_t,str[size]), NULL);
  bstr->gc.type = T_BSTR;
  bstr->len = size;
  return OVM_VAR(bstr);
}

static ovm_var_t *bstr_clone(gc_t *gc_ctx, ovm_var_t *var)
{
  ovm_var_t *res;

  if (var==NULL || var->gc.type!=T_BSTR)
    return (NULL);
  res = ovm_bstr_new (gc_ctx, BSTRLEN(var));
  memcpy (BSTR(res), BSTR(var), BSTRLEN(var));
  return res;
}

static void *bytestr_get_data(ovm_var_t *str)
{
  return ((ovm_bstr_t *)str)->str;
}

static size_t bytestr_get_data_len(ovm_var_t *str)
{
  return ((ovm_bstr_t *)str)->len;
}

void ovm_bstr_fprintf(FILE *fp, ovm_bstr_t *str)
{
  int i;

  if (str==NULL || str->gc.type != T_BSTR)
    {
      fprintf(fp, "Wrong object type.\n");
      return;
    }
  fprintf(fp, "binstr[%zd] : \"", str->len);
  for (i = 0; i < str->len; i++)
    {
      if (isprint(str->str[i]))
        fprintf(fp, "%c", str->str[i]);
      else
        fprintf(fp, ".");
    }
  fprintf(fp, "\"\n");
}

/*
** Virtual Binary String
** store references on an already allocated binary string.
*/

static void vbstr_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  ovm_vbstr_t *str = (ovm_vbstr_t *)p;

  if (str->delegate!=NULL)
    GC_TOUCH (gc_ctx, str->delegate);
}

static void vbstr_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  return;
}

static int vbstr_traverse (gc_traverse_ctx_t *gtc, gc_header_t *p,
			   void *data)
{
  ovm_vbstr_t *str = (ovm_vbstr_t *)p;
  int err = 0;

  if (str->delegate!=NULL)
    {
      err = (*gtc->do_subfield) (gtc, (gc_header_t *)str->delegate, data);
    }
  return err;
}

static gc_class_t vbstr_class = {
  GC_ID('v','b','s','t'),
  vbstr_mark_subfields,
  vbstr_finalize,
  vbstr_traverse
};

ovm_var_t *ovm_vbstr_new(gc_t *gc_ctx, ovm_var_t *bstr)
{
  ovm_vbstr_t *vbstr;

  vbstr = gc_alloc (gc_ctx, sizeof (ovm_vbstr_t), &vbstr_class);
  vbstr->gc.type = T_VBSTR;
  vbstr->delegate = bstr;
  return OVM_VAR(vbstr);
}

static ovm_var_t *vbstr_clone(gc_t *gc_ctx, ovm_var_t *var)
{
  ovm_var_t *res;

  if (var==NULL || var->gc.type != T_VBSTR)
    return NULL;
  res = ovm_vbstr_new (gc_ctx, ((ovm_vbstr_t *)var)->delegate);
  VBSTR(res) = VBSTR(var);
  VBSTRLEN(res) = VBSTRLEN(var);
  return res;
}

static void *vbstr_get_data(ovm_var_t *vbstr)
{
  return VBSTR(vbstr);
}

static size_t vbstr_get_data_len(ovm_var_t *vbstr)
{
  return VBSTRLEN(vbstr);
}

void ovm_vbstr_fprintf(FILE *fp, ovm_vbstr_t *str)
{
  int i;

  if (str==NULL || str->gc.type != T_VBSTR) {
    fprintf(fp, "Wrong object type.\n");
    return;
  }

  fprintf(fp, "vbinstr[%zd] : \"", str->len);
  for (i = 0; i < str->len; i++) {
    if (isprint(str->str[i]))
      fprintf(fp, "%c", str->str[i]);
    else
      fprintf(fp, ".");
  }
  fprintf(fp, "\"\n");
}



/*
** String
** store text string
*/

ovm_var_t *ovm_str_new(gc_t *gc_ctx, size_t size)
{
  ovm_str_t *str;

  str = gc_alloc (gc_ctx, offsetof(ovm_str_t,str[size]), NULL);
  str->gc.type = T_STR;
  str->len = size;
  return OVM_VAR(str);
}

static void *string_get_data (ovm_var_t *str)
{
  return ((ovm_str_t *)str)->str;
}

static size_t string_get_data_len (ovm_var_t *str)
{
  return ((ovm_str_t *)str)->len;
}

static ovm_var_t *string_clone(gc_t *gc_ctx, ovm_var_t *var)
{
  ovm_var_t *res;

  if (var==NULL || var->gc.type != T_STR)
    return NULL;
  res = ovm_str_new (gc_ctx, STRLEN(var));
  memcpy (STR(res), STR(var), STRLEN(var));
  return res;
}

static ovm_var_t *str_add(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  ovm_var_t *res;

  if (var2!=NULL)
    {
      if (var2->gc.type==T_STR) {
	res = ovm_str_new (gc_ctx, STRLEN(var1) + STRLEN(var2));
	memcpy(STR(res), STR(var1), STRLEN(var1));
	memcpy(STR(res) + STRLEN(var1), STR(var2), STRLEN(var2));
	return res;
      }
      else if (var2->gc.type == T_VSTR) {
	res = ovm_str_new (gc_ctx, STRLEN(var1) + VSTRLEN(var2) );
	memcpy(STR(res), STR(var1), STRLEN(var1));
	memcpy(STR(res) + STRLEN(var1), VSTR(var2), VSTRLEN(var2));
	return res;
      }
    }
  DebugLog(DF_OVM, DS_DEBUG, "Type error\n");
  return NULL;
}

static int str_cmp(ovm_var_t *var1, ovm_var_t *var2)
{
  int n;

  if (var2==NULL)
    return STRLEN(var1);
  if (TYPE(var2) == T_STR)
    {
      n = STRLEN(var1) - STRLEN(var2);
      return (n==0)?memcmp(STR(var1), STR(var2), STRLEN(var1)):n;
    }
  else if (TYPE(var2) == T_VSTR)
    {
      n = STRLEN(var1) - VSTRLEN(var2);
      return (n==0)?memcmp(STR(var1), VSTR(var2), STRLEN(var1)):n;
    }
  DebugLog(DF_OVM, DS_DEBUG, "Type error\n");
  return TYPE(var1) - TYPE(var2);
}

void ovm_str_fprintf(FILE *fp, ovm_str_t *str)
{
  int i;

  if (str==NULL || str->gc.type != T_STR)
    {
      fprintf(fp, "Wrong object type.\n");
      return;
    }
  fprintf(fp, "str[%zd] : \"", str->len);
  for (i = 0; i < str->len; i++)
    fprintf(fp, "%c", str->str[i]);
  fprintf(fp, "\"\n");
}

/*
** Virtual String
** used for store reference on text string.
*/

static void vstr_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  ovm_vstr_t *str = (ovm_vstr_t *)p;

  if (str->delegate!=NULL)
    GC_TOUCH (gc_ctx, str->delegate);
}

static void vstr_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  return;
}

static int vstr_traverse (gc_traverse_ctx_t *gtc, gc_header_t *p,
			  void *data)
{
  ovm_vstr_t *str = (ovm_vstr_t *)p;
  int err = 0;

  if (str->delegate!=NULL)
    {
      err = (*gtc->do_subfield) (gtc, (gc_header_t *)str->delegate, data);
    }
  return err;
}

static gc_class_t vstr_class = {
  GC_ID('v','s','t', 'r'),
  vstr_mark_subfields,
  vstr_finalize,
  vstr_traverse
};

ovm_var_t *ovm_vstr_new(gc_t *gc_ctx, ovm_var_t *str)
{
  ovm_vstr_t *vstr;

  vstr = gc_alloc (gc_ctx, sizeof (ovm_vstr_t), &vstr_class);
  vstr->gc.type = T_VSTR;
  vstr->delegate = str;
  return OVM_VAR(vstr);
}

static void *vstring_get_data(ovm_var_t *str)
{
  return ((ovm_vstr_t *)str)->str;
}

static size_t vstring_get_data_len(ovm_var_t *str)
{
  return ((ovm_vstr_t *)str)->len;
}

static ovm_var_t *vstr_clone(gc_t *gc_ctx, ovm_var_t *var)
{
  ovm_var_t *res;

  if (var==NULL || var->gc.type != T_VSTR)
    return NULL;
  res = ovm_vstr_new (gc_ctx, ((ovm_vstr_t *)var)->delegate);
  VSTR(res) = VSTR(var);
  VSTRLEN(res) = VSTRLEN(var);
  return res;
}

static ovm_var_t *vstr_add(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  ovm_var_t *res;

  if (var2!=NULL)
    {
      if (var2->gc.type == T_STR) {
	res = ovm_str_new (gc_ctx, VSTRLEN(var1) + STRLEN(var2));
	memcpy(STR(res), VSTR(var1), VSTRLEN(var1));
	memcpy(STR(res) + VSTRLEN(var1), STR(var2), STRLEN(var2));
	return res;
      }
      else if (var2->gc.type == T_VSTR) {
	res = ovm_str_new (gc_ctx, VSTRLEN(var1) + VSTRLEN(var2));
	memcpy(VSTR(res), VSTR(var1), VSTRLEN(var1));
	memcpy(VSTR(res) + VSTRLEN(var1), VSTR(var2), VSTRLEN(var2));
	return res;
      }
    }
  DebugLog(DF_OVM, DS_DEBUG, "Type error\n");
  return NULL;
}

static int vstr_cmp(ovm_var_t *var1, ovm_var_t *var2)
{
  int n;

  if (var2==NULL)
    return VSTRLEN(var1);
  if (TYPE(var2) == T_STR)
    {
      n = VSTRLEN(var1) - STRLEN(var2);
      return (n==0)?memcmp(VSTR(var1), STR(var2), VSTRLEN(var1)):n;
    }
  else if (TYPE(var2) == T_VSTR)
    {
      n = VSTRLEN(var1) - VSTRLEN(var2);
      return (n==0)?memcmp(VSTR(var1), VSTR(var2), VSTRLEN(var1)):n;
    }
  DebugLog(DF_OVM, DS_DEBUG, "Type error\n");
  return TYPE(var1) - TYPE(var2);
}


void ovm_vstr_fprintf(FILE *fp, ovm_vstr_t *vstr)
{
  int i;

  if (vstr==NULL || vstr->gc.type != T_VSTR)
    {
      fprintf(fp, "Wrong object type.\n");
      return;
    }

  fprintf(fp, "vstr[%zd] : \"", vstr->len);
  for (i = 0; i < vstr->len; i++)
    fprintf(fp, "%c", vstr->str[i]);
  fprintf(fp, "\"\n");
}

/*
** Array
** store an array on issdl objects (of same type ?)
*/

/* XXX: T_ARRAY -- ToDo */

/*
** Hash
** store issdl objects indexed by other issdl objects
** (constant i/o type ?)
*/

/* XXX: T_ARRAY -- ToDo */

/*
** CTime
** store a time_t value
*/

ovm_var_t *ovm_ctime_new(gc_t *gc_ctx, time_t tm)
{
  ovm_ctime_t *time;

  time = gc_alloc (gc_ctx, sizeof (ovm_ctime_t), NULL);
  time->gc.type = T_CTIME;
  time->time = tm;
  return OVM_VAR(time);
}

static void *ctime_get_data(ovm_var_t *t)
{
  return &CTIME(t);
}

static size_t ctime_get_data_len(ovm_var_t *i)
{
  return sizeof(time_t);
}

static int ctime_cmp(ovm_var_t *var1, ovm_var_t *var2)
{
  if (var2==NULL)
    return (int)CTIME(var1);
  if (TYPE(var2) != T_CTIME)
    return TYPE(var1) - TYPE(var2);
  return CTIME(var1) - CTIME(var2);
}

static ovm_var_t *ctime_add(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  ovm_var_t *res;

  if (var2==NULL)
    return NULL;
  switch (TYPE(var2))
    {
    case T_CTIME:
      return ovm_ctime_new (gc_ctx, CTIME(var1) + CTIME(var2));
    case T_INT:
      return ovm_ctime_new (gc_ctx,  CTIME(var1) + INT(var2));
    case T_TIMEVAL:
      res = timeval_clone(gc_ctx, var2);
      TIMEVAL(res).tv_sec += CTIME(var1);
      return res;
    default:
      return NULL;
    }
}

static ovm_var_t *ctime_sub(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  /* XXX: check for overflows */

  if (var2==NULL)
    return NULL;
  if (TYPE(var2) == T_CTIME)
    return ovm_ctime_new (gc_ctx, CTIME(var1) - CTIME(var2));
  else if (TYPE(var2) == T_INT)
    return ovm_ctime_new (gc_ctx, CTIME(var1) - INT(var2));
  else return NULL;
}

static ovm_var_t *ctime_clone(gc_t *gc_ctx, ovm_var_t *var)
{
  if (var==NULL || var->gc.type != T_CTIME)
    return NULL;
  return ovm_ctime_new (gc_ctx, CTIME(var));
}


void ovm_ctime_fprintf(FILE *fp, ovm_ctime_t *time)
{
  char asc_time[32];

  if (time==NULL || time->gc.type != T_CTIME)
    {
      fprintf(fp, "Wrong object type.\n");
      return ;
    }

  strftime(asc_time, 32, "%a %b %d %H:%M:%S %Y", localtime(&time->time));
  fprintf(fp, "ctime : (%li) = %s\n", time->time, asc_time);
}

/*
** IPv4
** store a sockaddr_in structure
*/

ovm_var_t *ovm_ipv4_new(gc_t* gc_ctx)
{
  ovm_ipv4_t *addr;

  addr = gc_alloc (gc_ctx, sizeof (ovm_ipv4_t), NULL);
  addr->gc.type = T_IPV4;
  return OVM_VAR(addr);
}

static int ipv4_cmp(ovm_var_t *var1, ovm_var_t *var2)
{
  if (var2==NULL)
    return (int)IPV4(var1).s_addr;
  if (TYPE(var2) != T_IPV4)
    return TYPE(var1) - TYPE(var2);
  return IPV4(var1).s_addr - IPV4(var2).s_addr;
}

static void *ipv4_get_data(ovm_var_t *addr)
{
  return &IPV4(addr);
}

static size_t ipv4_get_data_len(ovm_var_t *addr)
{
  return sizeof (struct in_addr);
}

static ovm_var_t *ipv4_clone(gc_t *gc_ctx, ovm_var_t *var)
{
  ovm_var_t *res;

  if (var==NULL || var->gc.type != T_IPV4)
    return NULL;
  res = ovm_ipv4_new(gc_ctx);
  IPV4(res) = IPV4(var);
  return res;
}

static ovm_var_t *ipv4_and (gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  unsigned long mask;
  ovm_var_t *res;

  if (var2==NULL)
    return NULL;
  switch (TYPE(var2))
    {
    case T_INT: mask = (unsigned long)INT(var2); break;
    case T_UINT: mask = UINT(var2); break;
    case T_IPV4: mask = (unsigned long)IPV4(var2).s_addr; break;
    default: return NULL;
    }
  res = ovm_ipv4_new(gc_ctx);
  IPV4(res).s_addr = IPV4(var1).s_addr & mask;
  return res;
}

static ovm_var_t *ipv4_or (gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  unsigned long mask;
  ovm_var_t *res;

  if (var2==NULL)
    return NULL;
  switch (TYPE(var2))
    {
    case T_INT: mask = (unsigned long)INT(var2); break;
    case T_UINT: mask = UINT(var2); break;
    case T_IPV4: mask = (unsigned long)IPV4(var2).s_addr; break;
    default: return NULL;
    }
  res = ovm_ipv4_new(gc_ctx);
  IPV4(res).s_addr = IPV4(var1).s_addr | mask;
  return res;
}

static ovm_var_t *ipv4_xor (gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  unsigned long mask;
  ovm_var_t *res;

  if (var2==NULL)
    return NULL;
  switch (TYPE(var2))
    {
    case T_INT: mask = (unsigned long)INT(var2); break;
    case T_UINT: mask = UINT(var2); break;
    case T_IPV4: mask = (unsigned long)IPV4(var2).s_addr; break;
    default: return NULL;
    }
  res = ovm_ipv4_new(gc_ctx);
  IPV4(res).s_addr = IPV4(var1).s_addr ^ mask;
  return res;
}

static ovm_var_t *ipv4_not(gc_t *gc_ctx, ovm_var_t *var)
{
  ovm_var_t *res;

  if (var==NULL || TYPE(var) != T_IPV4)
    return NULL;

  res = ovm_ipv4_new(gc_ctx);
  IPV4(res).s_addr = ~IPV4(var).s_addr;
  return res;
}

void ovm_ipv4_fprintf(FILE *fp, ovm_ipv4_t *addr)
{
  struct hostent *hptr;
  char **pptr;

  if (addr==NULL || addr->gc.type != T_IPV4)
    {
      fprintf(fp, "Wrong object type.\n");
      return;
    }
  fprintf(fp, "ipv4 : %s", inet_ntoa(addr->ipv4addr));
  /* address resolution */
  hptr = gethostbyaddr((char *)&addr->ipv4addr,
                       sizeof (struct in_addr), AF_INET);
  if (hptr == NULL)
    {
      fprintf(fp, "\n");
      return;
    }
  else if (hptr->h_name != NULL)
    fprintf(fp, " (name=%s", hptr->h_name);
  else
    {
      fprintf(fp, "\n");
      return ;
    }
  for (pptr = hptr->h_aliases; *pptr != NULL; pptr++)
    fprintf(fp, " alias=%s", *pptr);
  fprintf(fp, ")\n");
}

/*
** IPv6
** store a sockaddr_in6 structure
*/

ovm_var_t *ovm_ipv6_new(gc_t* gc_ctx)
{
  ovm_ipv6_t *addr;

  addr = gc_alloc (gc_ctx, sizeof (ovm_ipv6_t), NULL);
  addr->gc.type = T_IPV6;
  return OVM_VAR(addr);
}

void ovm_ipv6_fprintf(FILE *fp, ovm_ipv6_t *addr)
{
  struct hostent *hptr;
  char **pptr;
  char dst[INET6_ADDRSTRLEN];

  if (addr==NULL || addr->gc.type != T_IPV6)
    {
      fprintf(fp, "Wrong object type.\n");
      return;
    }

  /* inet_ntoa is not IPv6 aware. Use inet_ntop (defined in 
     arpa/inet.h) instead. */
  inet_ntop (AF_INET6, &addr->ipv6addr, dst, sizeof(dst));
  fprintf(fp, "ipv6 : %s", dst);

  /* address resolution */
  hptr = gethostbyaddr((char *)&addr->ipv6addr,
                       sizeof (struct in6_addr), AF_INET6);
  if (hptr == NULL)
    {
      fprintf(fp, "\n");
      return;
    }
  else if (hptr->h_name != NULL)
    fprintf(fp, " (name=%s", hptr->h_name);
  else
    {
      fprintf(fp, "\n");
      return ;
    }
  for (pptr = hptr->h_aliases; *pptr != NULL; pptr++)
    fprintf(fp, " alias=%s", *pptr);
  fprintf(fp, ")\n");
}

/*
** Timeval
** store a timeval struct
*/

ovm_var_t *ovm_timeval_new(gc_t *gc_ctx)
{
  ovm_timeval_t *time;

  time = gc_alloc (gc_ctx, sizeof (ovm_timeval_t), NULL);
  time->gc.type = T_TIMEVAL;
  time->time.tv_sec = 0;
  time->time.tv_usec = 0;
  return OVM_VAR(time);
}

static void *timeval_get_data(ovm_var_t *str)
{
  return &TIMEVAL(str);
}

static size_t timeval_get_data_len(ovm_var_t *str)
{
  return sizeof (struct timeval);
}

static ovm_var_t *timeval_add(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  ovm_var_t *res;

  if (var2==NULL)
    return NULL;
  switch (TYPE(var2))
    {
    case T_TIMEVAL:
      res = ovm_timeval_new(gc_ctx);
      Timer_Add(&TIMEVAL(res) , &TIMEVAL(var1) , &TIMEVAL(var2));
      return res;
    case T_CTIME:
      res = timeval_clone(gc_ctx, var1);
      TIMEVAL(res).tv_sec += CTIME(var2);
      return res;
    case T_INT:
      res = timeval_clone(gc_ctx, var1);
      TIMEVAL(res).tv_sec += INT(var2);
      return res;
    default:
      return NULL;
    }
}

static ovm_var_t *timeval_sub(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  ovm_var_t *res;

  if (var2==NULL)
    return NULL;
  switch (TYPE(var2))
    {
    case T_TIMEVAL:
      res = ovm_timeval_new(gc_ctx);
      Timer_Sub(&TIMEVAL(res) , &TIMEVAL(var1) , &TIMEVAL(var2));
      return res;
    case T_CTIME:
      res = timeval_clone(gc_ctx, var1);
      TIMEVAL(res).tv_sec -= CTIME(var2);
      return res;
    case T_INT:
      res = timeval_clone(gc_ctx, var1);
      TIMEVAL(res).tv_sec -= INT(var2);
      return res;
    default:
      return NULL;
    }
}

static int timeval_cmp(ovm_var_t *var1, ovm_var_t *var2)
{
  if (var2==NULL)
    return TIMEVAL(var1).tv_sec;
  if (TYPE(var2) != T_TIMEVAL)
    return (TYPE(var1) - TYPE(var2));
  if (timercmp(&TIMEVAL(var1) , &TIMEVAL(var2) , <))
    return -1;
  if (timercmp( &TIMEVAL(var1) , &TIMEVAL(var2) , >))
    return 1;
  /* if ( timercmp( &TIMEVAL(var1) , &TIMEVAL(var2) , = ) ) */
  return 0;
}

static ovm_var_t *timeval_clone(gc_t *gc_ctx, ovm_var_t *var)
{
  ovm_var_t *res;

  if (var==NULL || var->gc.type != T_TIMEVAL)
    return NULL;
  res = ovm_timeval_new(gc_ctx);
  TIMEVAL(res) = TIMEVAL(var);
  return res;
}

void ovm_timeval_fprintf(FILE *fp, ovm_timeval_t *time)
{
  char asc_time[32];

  if (time==NULL || time->gc.type != T_TIMEVAL)
    {
      fprintf(fp, "Wrong object type.\n");
      return ;
    }

  strftime(asc_time, 32, "%a %b %d %H:%M:%S %Y",
           localtime(&time->time.tv_sec));
  fprintf(fp, "timeval : (%li.%06li) = %s (+%li us)\n",
          time->time.tv_sec, (long)time->time.tv_usec,
          asc_time, (long)time->time.tv_usec);
}

/*
** Regex
*/

static void regex_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  return;
}

static void regex_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  ovm_regex_t *regex = (ovm_regex_t *)p;

  regfree(&(REGEX(regex)));
  if (regex->regex_str!=NULL)
    gc_base_free(regex->regex_str);
}

static int regex_traverse (gc_traverse_ctx_t *gtc, gc_header_t *p,
			   void *data)
{
  return 0;
}

static gc_class_t regex_class = {
  GC_ID('r','e','g','x'),
  regex_mark_subfields,
  regex_finalize,
  regex_traverse
};

ovm_var_t *ovm_regex_new(gc_t *gc_ctx)
{
  ovm_regex_t *regex;

  regex = gc_alloc (gc_ctx, sizeof (ovm_regex_t), &regex_class);
  regex->gc.type = T_REGEX;
  regex->regex_str = NULL;
  regex->splits = 0;
  return OVM_VAR(regex);
}

static void *regex_get_data(ovm_var_t *regex)
{
  return REGEXSTR(regex);
}

static size_t regex_get_data_len(ovm_var_t *regex)
{
  return strlen(REGEXSTR(regex));
}

void ovm_regex_fprintf(FILE *fp, ovm_regex_t *regex)
{
  if (regex==NULL || regex->gc.type != T_REGEX)
    {
      fprintf(fp, "Wrong object type.\n");
      return;
    }
  if (REGEXSTR(regex)==NULL)
    fprintf (fp, "regex[no description]\n");
  else fprintf(fp, "regex[%zd] : \"%s\"\n",
	       strlen(REGEXSTR(regex)), REGEXSTR(regex));
}

/*
** Float type
** store a 'float' value.
*/

ovm_var_t *ovm_float_new(gc_t *gc_ctx, double val)
{
  ovm_float_t *n;

  n = gc_alloc (gc_ctx, sizeof (ovm_float_t), NULL);
  n->gc.type = T_FLOAT;
  n->val = val;
  return OVM_VAR(n);
}

static void *float_get_data(ovm_var_t *i)
{
  return &((ovm_float_t *)i)->val;
}

static size_t float_get_data_len(ovm_var_t *i)
{
  return sizeof (double);
}

void ovm_float_fprintf(FILE *fp, ovm_float_t *val)
{
  if (val==NULL || val->gc.type != T_FLOAT) {
    fprintf(fp, "Wrong object type.\n");
    return;
  }
  fprintf(fp, "float : %f\n", val->val);
}

static int float_cmp(ovm_var_t *var1, ovm_var_t *var2)
{
  double diff;

  if (var2==NULL)
    return (int)FLOAT(var1);
  if (TYPE(var2) != T_FLOAT)
    return TYPE(var1) - TYPE(var2);
  diff = FLOAT(var1) - FLOAT(var2);
  if (diff > 0.0)
    return 1;
  if (diff < 0.0)
    return -1;
  return 0;
}

static ovm_var_t *float_add(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  if (var2==NULL || var2->gc.type != T_FLOAT)
    return NULL;
  return ovm_float_new (gc_ctx, FLOAT(var1) + FLOAT(var2));
}

static ovm_var_t *float_sub(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  if (var2==NULL || var2->gc.type != T_FLOAT)
    return NULL;
  return ovm_float_new (gc_ctx, FLOAT(var1) - FLOAT(var2));
}

static ovm_var_t *float_opp(gc_t *gc_ctx, ovm_var_t *var)
{
  return ovm_float_new (gc_ctx, -FLOAT(var));
}

static ovm_var_t *float_mul(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  if (var2==NULL || var2->gc.type != T_FLOAT)
    return NULL;
  return ovm_float_new (gc_ctx, FLOAT(var1) * FLOAT(var2));
}

static ovm_var_t *float_div(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  if (var2==NULL || var2->gc.type != T_FLOAT)
    return (NULL);
  /* Don't need to check whether FLOAT(var2) is zero, or denormalized:
     IEEE 754 knows how to handle these (with infinities, and NaNs).
  */
  return ovm_float_new (gc_ctx, FLOAT(var1) / FLOAT(var2));
}

static ovm_var_t *float_clone(gc_t *gc_ctx, ovm_var_t *var)
{
  if (var==NULL || var->gc.type != T_FLOAT)
    return NULL;
  return ovm_float_new (gc_ctx, FLOAT(var));
}


/*
** snmp oid
*/

ovm_var_t *ovm_snmpoid_new(gc_t *gc_ctx, size_t len)
{
  ovm_snmpoid_t *o;

  o = gc_alloc (gc_ctx, offsetof(ovm_snmpoid_t,objoid[len]), NULL);
  o->gc.type = T_SNMPOID;
  o->len = len;
  return OVM_VAR(o);
}

/* raw data support */

void *issdl_get_data(ovm_var_t *val)
{
  if ( issdl_types_g[ val->gc.type ].get_data )
    return (*issdl_types_g[ val->gc.type ].get_data) (val);
  DebugLog(DF_OVM, DS_WARN,
	   "issdl_get_data(): get_data doesn't apply for type '%s'\n",
	   STRTYPE(val));
  return NULL;
}

size_t issdl_get_data_len(ovm_var_t *val)
{
  if (issdl_types_g[val->gc.type].get_data_len)
    return (*issdl_types_g[val->gc.type].get_data_len) (val);
  DebugLog(DF_OVM, DS_WARN,
	   "issdl_get_data_len(): get_data_len doesn't apply for type '%s'\n",
	   STRTYPE(val));
  return 0;
}

/*
** External data type
** store an 'void *' pointer.
*/

static void extern_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  return;
}

static void extern_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  EXTFREE(p) (EXTPTR(p));
}

static int extern_traverse (gc_traverse_ctx_t *gtc, gc_header_t *p,
			   void *data)
{
  return 0;
}

static gc_class_t extern_class = {
  GC_ID('x','t','r','n'),
  extern_mark_subfields,
  extern_finalize,
  extern_traverse
};

ovm_var_t *ovm_extern_new(gc_t* gc_ctx, void *ptr, char *desc,
			  void (*free) (void *ptr))
{
  ovm_extern_t *addr;

  addr = gc_alloc (gc_ctx, sizeof (ovm_extern_t), &extern_class);
  addr->gc.type = T_EXTERNAL;
  addr->ptr = ptr;
  addr->desc = desc;
  addr->free = free;
  return OVM_VAR(addr);
}

static void *extern_get_data(ovm_var_t *i)
{
  return &((ovm_extern_t *)i)->ptr;
}

static size_t extern_get_data_len(ovm_var_t *i)
{
  return sizeof (void *);
}


/* type specific actions */


int issdl_test(ovm_var_t *var)
{
  if (var==NULL)
    return 0;
  switch (var->gc.type)
  {
    case T_INT :
      return INT(var);
    case T_UINT :
      return UINT(var);
    default :
      return (1);
  }
}

int issdl_cmp(ovm_var_t *var1, ovm_var_t *var2)
{
  if (var1!=NULL && issdl_types_g[var1->gc.type].cmp!=NULL)
    return (*issdl_types_g[var1->gc.type].cmp) (var1, var2);
  DebugLog(DF_OVM, DS_WARN,
	   "issdl_cmp(): comparison doesn't apply for type '%s'\n",
	   STRTYPE(var1));
  return -1;
}

ovm_var_t *issdl_add(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  if (var1!=NULL && issdl_types_g[ var1->gc.type ].add!=NULL)
    return (*issdl_types_g[ var1->gc.type ].add) (gc_ctx, var1, var2);
  DebugLog(DF_OVM, DS_WARN,
	   "issdl_add(): addition doesn't apply for type '%s'\n",
	   STRTYPE(var1));
  return NULL;
}

ovm_var_t *issdl_sub(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  if (var1!=NULL && issdl_types_g[var1->gc.type].sub!=NULL)
    return (*issdl_types_g[ var1->gc.type ].sub) (gc_ctx, var1, var2);
  DebugLog(DF_OVM, DS_WARN,
	   "issdl_sub(): subtraction doesn't apply for type '%s'\n",
	   STRTYPE(var1));
  return NULL;
}

ovm_var_t *issdl_opp(gc_t *gc_ctx, ovm_var_t *var)
{
  if (var!=NULL && issdl_types_g[var->gc.type].opp!=NULL)
    return (*issdl_types_g[var->gc.type].opp) (gc_ctx, var);
  DebugLog(DF_OVM, DS_WARN,
	   "issdl_opp(): substraction doesn't apply for type '%s'\n",
	   STRTYPE(var));
  return NULL;
}

ovm_var_t *issdl_mul(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  if (var1!=NULL && issdl_types_g[ var1->gc.type ].mul!=NULL)
    return (*issdl_types_g[ var1->gc.type ].mul) (gc_ctx, var1, var2);
  DebugLog(DF_OVM, DS_WARN,
	   "issdl_mul(): multiplication doesn't apply for type '%s'\n",
	   STRTYPE(var1));
  return NULL;
}

ovm_var_t *issdl_div(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  if (var1!=NULL && issdl_types_g[ var1->gc.type ].div!=NULL)
    return (*issdl_types_g[ var1->gc.type ].div) (gc_ctx, var1, var2);
  DebugLog(DF_OVM, DS_WARN,
	   "issdl_div(): division doesn't apply for type '%s'\n",
	   STRTYPE(var1));
  return NULL;
}

ovm_var_t *issdl_mod(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  if (var1!=NULL && issdl_types_g[ var1->gc.type ].mod!=NULL)
    return (*issdl_types_g[ var1->gc.type ].mod) (gc_ctx, var1, var2);
  DebugLog(DF_OVM, DS_WARN,
	   "issdl_mod(): modulo doesn't apply for type '%s'\n",
	   STRTYPE(var1));
  return NULL;
}

ovm_var_t *issdl_clone(gc_t *gc_ctx, ovm_var_t *var)
{
  if (var!=NULL)
    {
      if (issdl_types_g[ var->gc.type ].clone!=NULL)
	return (*issdl_types_g[ var->gc.type ].clone) (gc_ctx, var);
      DebugLog(DF_OVM, DS_WARN,
	       "issdl_clone(): cloning doesn't apply for type '%s'\n",
	       STRTYPE(var));
    }
  return NULL;
}

ovm_var_t *issdl_and(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  if (var1!=NULL && issdl_types_g[ var1->gc.type ].and!=NULL)
    return (*issdl_types_g[ var1->gc.type ].and) (gc_ctx, var1, var2);
  DebugLog(DF_OVM, DS_WARN,
	   "issdl_and(): bitwise and doesn't apply for type '%s'\n",
	   STRTYPE(var1));
  return NULL;
}

ovm_var_t *issdl_or(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  if (var1!=NULL && issdl_types_g[ var1->gc.type ].or!=NULL)
    return (*issdl_types_g[ var1->gc.type ].or) (gc_ctx, var1, var2);
  DebugLog(DF_OVM, DS_WARN,
	   "issdl_and(): bitwise inclusive or doesn't apply for type '%s'\n",
	   STRTYPE(var1));
  return NULL;
}

ovm_var_t *issdl_xor(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  if (var1!=NULL && issdl_types_g[ var1->gc.type ].xor!=NULL)
    return (*issdl_types_g[ var1->gc.type ].xor) (gc_ctx, var1, var2);
  DebugLog(DF_OVM, DS_WARN,
	   "issdl_and(): bitwise exclusive or doesn't apply for type '%s'\n",
	   STRTYPE(var1));
  return NULL;
}

ovm_var_t *issdl_not(gc_t *gc_ctx, ovm_var_t *var)
{
  if (var!=NULL && issdl_types_g[var->gc.type].not!=NULL)
    return (*issdl_types_g[var->gc.type].not) (gc_ctx, var);
  DebugLog(DF_OVM, DS_WARN,
	   "issdl_not(): bitwise complement doesn't apply for type '%s'\n",
	   STRTYPE(var));
  return NULL;
}

/* Display function */

int snprintf_ovm_var(char *buff, unsigned int buff_length, ovm_var_t *val)
{
  int i; /* for STRINGs */
  char asc_time[32]; /* for date conversions */
  struct hostent *hptr; /* for IPV4ADDR */
  char **pptr; /* for IPV4ADDR */
  char dst[INET6_ADDRSTRLEN]; /* for IPV6 */
  int offset = 0; /* for chars */

  /* display data */
  if (val==NULL)
    return snprintf(buff, buff_length, "(null)\n");
  switch (val->gc.type)
    {
    case T_INT:
      return snprintf(buff, buff_length, "%li", INT(val));
    case T_BSTR:
      for (i = 0; i < BSTRLEN(val); i++) {
	if (isprint(BSTR(val)[i]))
	  offset += snprintf(buff + offset, buff_length - offset,
			     "%c", BSTR(val)[i]);
	else
	  offset += snprintf(buff + offset, buff_length - offset, ". ");
      }
      return offset;
  case T_STR:
    for (i = 0; i < STRLEN(val); i++)
      offset += snprintf(buff + offset, buff_length - offset, "%c", STR(val)[i]);
    return offset;
    case T_VSTR:
      for (i = 0; i < VSTRLEN(val); i++)
	offset += snprintf(buff + offset, buff_length - offset,
			   "%c", VSTR(val)[i]);
      return offset;
    case T_CTIME:
      strftime(asc_time, 32, "%a %b %d %H:%M:%S %Y",
	       localtime(&CTIME(val)));
      return snprintf(buff, buff_length, "%s (%li)", asc_time, CTIME(val));
    case T_IPV4:
      offset += snprintf(buff, buff_length, "%s", inet_ntoa(IPV4(val)));
      hptr = gethostbyaddr((char *) &IPV4(val),
			   sizeof (struct in_addr), AF_INET);
      if (hptr == NULL) {
	return offset;
      } else if (hptr->h_name != NULL) {
	offset += snprintf(buff + offset, buff_length - offset,
			   " (%s", hptr->h_name);
      } else {
	return offset;
      }
      for (pptr = hptr->h_aliases; *pptr != NULL; pptr++)
	offset += snprintf(buff + offset, buff_length - offset,
			   ", %s", *pptr);
      offset += snprintf(buff + offset, buff_length - offset, ")");
      return offset;
    case T_IPV6:
      inet_ntop (AF_INET6, &IPV6(val), dst, sizeof(dst));
      offset += snprintf(buff, buff_length, "%s", dst);
      hptr = gethostbyaddr((char *) &IPV6(val),
			   sizeof (struct in6_addr), AF_INET6);
      if (hptr == NULL) {
	return offset;
      } else if (hptr->h_name != NULL) {
	offset += snprintf(buff + offset, buff_length - offset,
			   " (%s", hptr->h_name);
      } else {
	return offset;
      }
      for (pptr = hptr->h_aliases; *pptr != NULL; pptr++)
	offset += snprintf(buff + offset, buff_length - offset,
			   ", %s", *pptr);
      offset += snprintf(buff + offset, buff_length - offset, ")");
      return offset;
    case T_TIMEVAL:
      strftime(asc_time, 32, "%a %b %d %H:%M:%S %Y",
	       localtime(&TIMEVAL(val).tv_sec));
      return snprintf(buff, buff_length, "%s +%li us (%li.%06li)",
		      asc_time, (long)TIMEVAL(val).tv_usec,
		      TIMEVAL(val).tv_sec, (long)TIMEVAL(val).tv_usec);
    case T_REGEX:
      if (REGEXSTR(val)==NULL)
	{
	  buff[0] = '\0';
	  return 0;
	}
      return snprintf(buff, buff_length, "%s", REGEXSTR(val));
    case T_FLOAT:
      return snprintf(buff, buff_length, "%f", FLOAT(val));
    default:
      return snprintf(buff, buff_length,
		      "type %i doesn't support display\n", val->gc.type);
    }
}

void
fprintf_ovm_var(FILE *fp, ovm_var_t *val)
{
  int i; /* for STRINGs */
  char asc_time[32]; /* for dates conversions */
  struct hostent *hptr; /* for IPV4ADDR */
  char **pptr; /* for IPV4ADDR */
  char dst[INET6_ADDRSTRLEN];

  if (val==NULL)
    fprintf(fp, "(null)\n");
  /* display data */
  else switch (val->gc.type)
	 {
	 case T_INT:
	   fprintf(fp, "%li", INT(val));
	   break;
	 case T_BSTR:
	   for (i = 0; i < BSTRLEN(val); i++) {
	     if (isprint(BSTR(val)[i]))
	       fprintf(fp, "%c", BSTR(val)[i]);
	     else
	       fprintf(fp, ". ");
	   }
	   break;
	 case T_STR:
	   for (i = 0; i < STRLEN(val); i++)
	     fprintf(fp, "%c", STR(val)[i]);
	   break;
	 case T_VSTR:
	   for (i = 0; i < VSTRLEN(val); i++)
	     fprintf(fp, "%c", VSTR(val)[i]);
	   break;
	 case T_CTIME:
	   strftime(asc_time, 32,
		    "%a %b %d %H:%M:%S %Y", localtime(&CTIME(val)));
	   fprintf(fp, "%s (%li)", asc_time, CTIME(val));
	   break;
	 case T_IPV4:
	   fprintf(fp, "%s", inet_ntoa(IPV4(val)));
	   hptr = gethostbyaddr((char *) &IPV4(val),
				sizeof (struct in_addr), AF_INET);
	   if (hptr == NULL) {
	     break;
	   } else if (hptr->h_name != NULL) {
	     fprintf(fp, " (%s", hptr->h_name);
	   } else {
	     break;
	   }
	   for (pptr = hptr->h_aliases; *pptr != NULL; pptr++)
	     fprintf(fp, ", %s", *pptr);
	   fprintf(fp, ")");
	   break;
	 case T_IPV6:
           inet_ntop (AF_INET6, &IPV6(val), dst, sizeof(dst));
	   fprintf(fp, "%s", dst);
	   hptr = gethostbyaddr((char *) &IPV6(val),
				sizeof (struct in6_addr), AF_INET6);
	   if (hptr == NULL) {
	     break;
	   } else if (hptr->h_name != NULL) {
	     fprintf(fp, " (%s", hptr->h_name);
	   } else {
	     break;
	   }
	   for (pptr = hptr->h_aliases; *pptr != NULL; pptr++)
	     fprintf(fp, ", %s", *pptr);
	   fprintf(fp, ")");
	   break;
	 case T_TIMEVAL:
	   strftime(asc_time, 32, "%a %b %d %H:%M:%S %Y",
		    localtime(&TIMEVAL(val).tv_sec));
	   fprintf(fp, "%s +%li us (%li.%06li)",
		   asc_time, (long)TIMEVAL(val).tv_usec,
		   TIMEVAL(val).tv_sec, (long)TIMEVAL(val).tv_usec);
	   break;
	 case T_REGEX:
	   if (REGEXSTR(val)!=NULL)
	     fprintf(fp, "%s", REGEXSTR(val));
	   break;
	 case T_FLOAT:
	   fprintf(fp, "%f", FLOAT(val));
	   break;
	 default:
	   fprintf(fp, "type %i doesn't support display\n", val->gc.type);
	   break;
	 }
}

void fprintf_issdl_val(FILE *fp, const orchids_t *ctx, ovm_var_t *val)
{
  int i; /* for STRINGs */
  char asc_time[32]; /* for date conversions */
  struct hostent *hptr; /* for IPV4ADDR */
  char **pptr; /* for IPV4ADDR */
  char dst[INET6_ADDRSTRLEN];

  if (val==NULL)
    fprintf (fp, "null\n");
  /* Display data */
  else switch (val->gc.type) {
    case T_INT:
      fprintf(fp, "int: %li\n", INT(val));
      break;
    case T_UINT:
      fprintf(fp, "uint: %lu\n", UINT(val));
      break;
    case T_BSTR:
      fprintf(fp, "bstr[%zd]: \"", BSTRLEN(val));
      for (i = 0; i < BSTRLEN(val); i++) {
        if (isprint(BSTR(val)[i]))
          fputc(BSTR(val)[i], fp);
        else
          fputc('.', fp);
      }
      fputs("\"\n", fp);
      break;
    case T_VBSTR:
      fprintf(fp, "vbstr[%zd]: \"", VBSTRLEN(val));
      for (i = 0; i < VBSTRLEN(val); i++) {
        if (isprint(VBSTR(val)[i]))
          fputc(VBSTR(val)[i], fp);
        else
          fputc('.', fp);
      }
      fputs("\"\n", fp);
      break;
    case T_STR:
      fprintf(fp, "str[%zd]: \"", STRLEN(val));
      for (i = 0; i < STRLEN(val); i++)
        fputc(STR(val)[i], fp);
      fputs("\"\n", fp);
      break;
    case T_VSTR:
      fprintf(fp, "vstr[%zd]: \"", VSTRLEN(val));
      for (i = 0; i < VSTRLEN(val); i++)
        fputc(VSTR(val)[i], fp);
      fprintf(fp, "\"\n");
      break;
    case T_CTIME:
      strftime(asc_time, 32, "%a %b %d %H:%M:%S %Y",
	       localtime(&CTIME(val)));
      fprintf(fp, "ctime : (%li) = %s\n", CTIME(val), asc_time);
      break;
    case T_IPV4:
      fprintf(fp, "ipv4: %s", inet_ntoa(IPV4(val)));
      if (resolve_ipv4_g) {
        hptr = gethostbyaddr((char *) &IPV4(val),
                             sizeof (struct in_addr), AF_INET);
      }
      else {
        hptr = NULL;
      }
      if (hptr == NULL) {
        fputc('\n', fp);
        return ;
      }
      else if (hptr->h_name != NULL) {
        fprintf(fp, " (name=%s", hptr->h_name);
      }
      else {
        fputc('\n', fp);
        return ;
      }
      for (pptr = hptr->h_aliases; *pptr != NULL; pptr++)
        fprintf(fp, " alias=%s", *pptr);
      fputs(")\n", fp);
      break;
    case T_IPV6:
      inet_ntop (AF_INET6, &IPV6(val), dst, sizeof(dst));
      fprintf(fp, "ipv6: %s", dst);
      if (resolve_ipv6_g) {
        hptr = gethostbyaddr((char *) &IPV6(val),
                             sizeof (struct in6_addr), AF_INET6);
      }
      else {
        hptr = NULL;
      }
      if (hptr == NULL) {
        fputc('\n', fp);
        return ;
      }
      else if (hptr->h_name != NULL) {
        fprintf(fp, " (name=%s", hptr->h_name);
      }
      else {
        fputc('\n', fp);
        return ;
      }
      for (pptr = hptr->h_aliases; *pptr != NULL; pptr++)
        fprintf(fp, " alias=%s", *pptr);
      fputs(")\n", fp);
      break;
    case T_TIMEVAL:
      strftime(asc_time, 32, "%a %b %d %H:%M:%S %Y",
               localtime(&TIMEVAL(val).tv_sec));
      fprintf(fp, "timeval: (%li.%06li) = %s (+%li us)\n",
              TIMEVAL(val).tv_sec, (long)TIMEVAL(val).tv_usec,
              asc_time, (long)TIMEVAL(val).tv_usec);
      break;
    case T_REGEX:
      if (REGEXSTR(val)==NULL)
	fprintf(fp, "regex[no description]\n");
      else fprintf(fp, "regex[%zd]: \"%s\"\n",
		   strlen(REGEXSTR(val)), REGEXSTR(val));
      break;
    case T_FLOAT:
      fprintf(fp, "float: %f\n", FLOAT(val));
      break;
    case T_SNMPOID:
      fprintf(fp, "snmpoid[%zd]: ", SNMPOIDLEN(val));
      for (i=0; i < SNMPOIDLEN(val); i++) {
        fprintf(fp, "%lu.", (SNMPOID(val))[i]);
      }
      fputc('\n', fp);
      break;
    case T_EVENT:
      fprintf_event(fp, ctx, (event_t *)val);
      break;
    default:
      fprintf(fp, "type %i doesn't support display\n", val->gc.type);
    }
}

/* -------------------------------------------------------------------------- *
** Built-in functions
*/

static void
issdl_print(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *param;

  DebugLog(DF_OVM, DS_DEBUG, "issdl_print()\n");
  param = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  fprintf_issdl_val(stdout, ctx, param);
  STACK_DROP(ctx->ovm_stack, 1);
  PUSH_RETURN_TRUE(ctx);
}

static void issdl_dumpstack(orchids_t *ctx, state_instance_t *state)
{
  DebugLog(DF_OVM, DS_DEBUG, "issdl_dumpstack()\n");

  if (state->rule_instance == NULL)
    {
      PUSH_RETURN_FALSE(ctx);
      return;
    }

  // push before state == NULL
  PUSH_RETURN_TRUE(ctx);

  fprintf(stdout, ">>>> rule: %s <<<<<\n", state->rule_instance->rule->name);
  while (state)
    {
      fprintf(stdout, "***** state: %s *****\n", state->state->name);
      fprintf_state_env(stdout, ctx, state);
      if (state->event)
        fprintf_event(stdout, ctx, state->event->event);
      else
        fprintf(stdout, "no event.\n");
      state = state->parent;
    }
}

static void issdl_printevent(orchids_t *ctx, state_instance_t *state)
{
  DebugLog(DF_OVM, DS_DEBUG, "issdl_printevent()\n");
  PUSH_RETURN_TRUE(ctx);
  for ( ; state!=NULL && state->event == NULL; state = state->parent)
    ;
  if (state!=NULL && state->event!=NULL)
    fprintf_event(stdout, ctx, state->event->event);
  else
    fprintf(stdout, "no event to display.\n");
}

static void issdl_shutdown(orchids_t *ctx, state_instance_t *state)
{
  DebugLog(DF_OVM, DS_DEBUG, "issdl_shutdown()\n");
  fprintf(stdout, "explicit shutdown on '%s:%s' request\n",
          state->rule_instance->rule->name, state->state->name);
  exit(EXIT_SUCCESS);
}

static void issdl_dumppathtree(orchids_t *ctx, state_instance_t *state)
{
  DebugLog(DF_OVM, DS_DEBUG, "issdl_dumppathtree()\n");
  fprintf_rule_instance_dot(stdout, state->rule_instance,
                            DOT_RETRIGLIST, ctx->new_qh, 100);
  /* XXX: hard-coded limit */
  PUSH_RETURN_TRUE(ctx);
}

static void issdl_random(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *random;

  DebugLog(DF_OVM, DS_DEBUG, "issdl_random()\n");
  random = ovm_int_new(ctx->gc_ctx, rand ());
  PUSH_VALUE (ctx, random);
}

static void issdl_system(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t  *param;
  char       *cmd_line;
  int ret;

  ret = -1;
  param = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  if (param!=NULL &&
      (param->gc.type == T_STR || param->gc.type == T_VSTR))
    {
      cmd_line = ovm_strdup(ctx->gc_ctx, param);
      if (cmd_line!=NULL)
	{
	  DebugLog(DF_OVM, DS_DEBUG, "issdl_system( \"%s\" )\n", cmd_line);
	  ret = system(cmd_line);
	  gc_base_free(cmd_line);
	}
    }
  else
    {
      DebugLog(DF_OVM, DS_DEBUG, "issdl_system(): param error\n");
    }
  STACK_DROP(ctx->ovm_stack, 1);
  if (ret==0)
    PUSH_RETURN_FALSE(ctx); /* do not allocate 0 */
  else
    {
      ovm_var_t *val = ovm_int_new (ctx->gc_ctx, ret);
      PUSH_VALUE(ctx, val);
    }
}

static void issdl_stats(orchids_t *ctx, state_instance_t *state)
{
  fprintf_orchids_stats(stdout, ctx);
  PUSH_RETURN_TRUE(ctx);
}

static void issdl_str_from_int(orchids_t *ctx, state_instance_t *state)
{
  char buff[64];
  ovm_var_t *str;
  ovm_var_t *i;
  size_t len;

  i = POP_VALUE(ctx);
  if (i!=NULL && TYPE(i)==T_INT)
    {
      snprintf(buff, sizeof(buff), "%li", INT(i));
      len = strlen(buff);
      str = ovm_str_new(ctx->gc_ctx, len);
      memcpy(STR(str), buff, len);
      PUSH_VALUE(ctx,str);
    }
  else
    {
      DebugLog(DF_OVM, DS_DEBUG, "issdl_str_from_int(): param error\n");
      PUSH_VALUE(ctx, NULL);
    }
}

static void issdl_str_from_float(orchids_t *ctx, state_instance_t *state)
{
  char buff[64];
  ovm_var_t *str;
  ovm_var_t *x;
  size_t len;

  x = POP_VALUE(ctx);
  if (x!=NULL && TYPE(x)==T_FLOAT)
    {
      snprintf(buff, sizeof(buff), "%g", FLOAT(x));
      len = strlen(buff);
      str = ovm_str_new(ctx->gc_ctx, len);
      memcpy(STR(str), buff, len);
      PUSH_VALUE(ctx,str);
    }
  else
    {
      DebugLog(DF_OVM, DS_DEBUG, "issdl_str_from_float(): param error\n");
      PUSH_VALUE(ctx, NULL);
    }
}

static void issdl_str_from_ipv4(orchids_t *ctx, state_instance_t *state)
{
  char buff[32];
  ovm_var_t *str;
  ovm_var_t *i;
  size_t len;

  i = POP_VALUE(ctx);
  if (i!=NULL && i->gc.type == T_IPV4) {
    snprintf(buff, sizeof(buff), "%s", inet_ntoa(IPV4(i)));
    len = strlen (buff);
    str = ovm_str_new(ctx->gc_ctx, len);
    memcpy(STR(str), buff, len);
    PUSH_VALUE(ctx, str);
  }
  else {
    DebugLog(DF_OVM, DS_DEBUG, "issdl_str_from_ipv4(): param error\n");
    PUSH_VALUE(ctx, NULL);
  }
}

static void issdl_str_from_ipv6(orchids_t *ctx, state_instance_t *state)
{
  char buff[INET6_ADDRSTRLEN];
  ovm_var_t *str;
  ovm_var_t *i;
  size_t len;

  i = POP_VALUE(ctx);
  if (i!=NULL && i->gc.type == T_IPV6) {
    inet_ntop(AF_INET6, &IPV6(i), buff, sizeof(buff));
    len = strlen (buff);
    str = ovm_str_new(ctx->gc_ctx, len);
    memcpy(STR(str), buff, len);
    PUSH_VALUE(ctx, str);
  }
  else {
    DebugLog(DF_OVM, DS_DEBUG, "issdl_str_from_ipv6(): param error\n");
    PUSH_VALUE(ctx, NULL);
  }
}

unsigned long orchids_atoui (char *str, size_t len)
{
  char *end = str+len;
  unsigned long n=0;

  while (str<end && isspace(*str)) str++;
  while (str<end && isdigit(*str)) n = 10*n + ((*str++)-'0');
  return n;
}

long orchids_atoi (char *str, size_t len)
{
  char *end = str+len;
  long n=0;
  int negate=0;

  while (str<end && isspace(*str)) str++;
  if (str < end && *str=='-') { negate=1; str++; }
  while (str<end && isspace(*str)) str++;
  while (str<end && isdigit(*str)) n = 10*n + ((*str++)-'0');
  return negate?(-n):n;
}

static void issdl_int_from_str(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *str;
  ovm_var_t *i;

  str = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  if (str!=NULL && str->gc.type == T_STR)
    {
      i = ovm_int_new(ctx->gc_ctx, orchids_atoi(STR(str), STRLEN(str)));
      STACK_DROP(ctx->ovm_stack, 1);
      PUSH_VALUE(ctx,i);
    }
  else if (str && str->gc.type == T_VSTR)
    {
      i = ovm_int_new(ctx->gc_ctx, orchids_atoi(VSTR(str), VSTRLEN(str)));
      STACK_DROP(ctx->ovm_stack, 1);
      PUSH_VALUE(ctx,i);
    }
  else
    {
      DebugLog(DF_OVM, DS_DEBUG, "issdl_int_from_str(): param error\n");
      STACK_DROP(ctx->ovm_stack, 1);
      PUSH_VALUE(ctx, NULL);
    }
}

char *time_convert(char *str, char *end, struct timeval *tv)
{
  long sec = 0;
  double d = 0.0;
  double mask = 1.0;
  char c;

  while (str<end && (c = *str, isdigit(c)))
    {
      sec = 10 * sec + (((int)c) - '0');
      str++;
    }
  if (str<end && (c = *str)=='.')
    {
      str++;
      while (str<end && (c = *str, isdigit(c)))
	{
	  mask *= 0.1;
	  d += (((int)c) - '0') * mask;
	  str++;
	}
    }
  tv->tv_sec = sec;
  tv->tv_usec = (long) (1000000.0 * d);
  return str;
}

static void issdl_timeval_from_str(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *str;
  ovm_var_t *i;
  struct timeval tv;

  str = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  if (str!=NULL && TYPE(str)==T_STR)
    {
      (void) time_convert(STR(str), STR(str)+STRLEN(str), &tv);
      i = ovm_timeval_new(ctx->gc_ctx);
      TIMEVAL(i) = tv;
      STACK_DROP(ctx->ovm_stack, 1);
      PUSH_VALUE(ctx,i);
    }
  else if (str!=NULL && TYPE(str)==T_VSTR)
    {
      (void) time_convert(VSTR(str), VSTR(str)+VSTRLEN(str), &tv);
      i = ovm_timeval_new(ctx->gc_ctx);
      TIMEVAL(i) = tv;
      STACK_DROP(ctx->ovm_stack, 1);
      PUSH_VALUE(ctx,i);
    }
  else
    {
      DebugLog(DF_OVM, DS_DEBUG, "issdl_timeval_from_str(): param error\n");
      STACK_DROP(ctx->ovm_stack, 1);
      PUSH_VALUE(ctx, NULL);
    }
}

static void issdl_time_from_str(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *str;
  ovm_var_t *i;
  struct timeval tv;

  str = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  if (str!=NULL && TYPE(str)==T_STR)
    {
      (void) time_convert(STR(str), STR(str)+STRLEN(str), &tv);
      i = ovm_ctime_new(ctx->gc_ctx, tv.tv_sec);
      STACK_DROP(ctx->ovm_stack, 1);
      PUSH_VALUE(ctx,i);
    }
  else if (str!=NULL && TYPE(str)==T_VSTR)
    {
      (void) time_convert(VSTR(str), VSTR(str)+VSTRLEN(str), &tv);
      i = ovm_ctime_new(ctx->gc_ctx, tv.tv_sec);
      STACK_DROP(ctx->ovm_stack, 1);
      PUSH_VALUE(ctx,i);
    }
  else
    {
      DebugLog(DF_OVM, DS_DEBUG, "issdl_time_from_str(): param error\n");
      STACK_DROP(ctx->ovm_stack, 1);
      PUSH_VALUE(ctx, NULL);
    }
}

static double ldexp10 (double x, long y)
{
  double z;

  if (y>0)
    {
      z = 10.0;
      while (y!=0)
	{
	  if (y & 1)
	    x *= z;
	  y >>= 1;
	  z = z*z;
	}
    }
  else if (y<0)
    {
      y = -y;
      z = 0.1;
      while (y!=0)
	{
	  if (y & 1)
	    x *= z;
	  y >>= 1;
	  z = z*z;
	}
    }
  return x;
}

double orchids_atof(char *str, size_t str_sz)
{
  double x;
  int neg;
  char *s, *end;
  char c;
  double mask;
  long expo;

  s = str;
  end = s+str_sz;
  x = 0.0;
  if (s>=end)
    return x;
  c = *s;
  if (c=='-')
    {
      neg = 1;
      s++;
    }
  else neg = 0;
  while (s<end)
    {
      c = *s++;
      switch (c)
	{
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
	  x = 10.0 * x + (double)(((int)c) - '0');
	  break;
	case '.':
	  goto dot;
	case 'e': case 'E':
	  goto exp;
	default:
	  goto neg;
	}
    }
 neg:
  return neg?(-x):x;
 dot:
  mask = 1.0;
  while (s<end && (c = *s, isdigit(c)))
    {
      mask *= 0.1;
      x += (((int)c) - '0') * mask;
      s++;
    }
  if (s>=end)
    goto neg;
  c = *s++;
  if (c!='e' && c!='E')
    goto neg;
 exp:
  expo = orchids_atoi (s, end-s);
  x = ldexp10 (x, expo);
  goto neg;
}


static void issdl_float_from_str(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *str;
  ovm_var_t *i;

  str = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  if (str!=NULL && TYPE(str) == T_STR)
    {
      i = ovm_float_new(ctx->gc_ctx, orchids_atof(STR(str), STRLEN(str)));
      STACK_DROP(ctx->ovm_stack, 1);
      PUSH_VALUE(ctx,i);
    }
  else if (str && TYPE(str) == T_VSTR)
    {
      i = ovm_float_new(ctx->gc_ctx, orchids_atof(VSTR(str), VSTRLEN(str)));
      STACK_DROP(ctx->ovm_stack, 1);
      PUSH_VALUE(ctx,i);
    }
  else
    {
      DebugLog(DF_OVM, DS_DEBUG, "issdl_float_from_str(): param error\n");
      STACK_DROP(ctx->ovm_stack, 1);
      PUSH_VALUE(ctx, NULL);
    }
}


static void issdl_kill_threads(orchids_t *ctx, state_instance_t *state)
{
  wait_thread_t *t;

  for (t = ctx->cur_retrig_qh; t; t = t->next)
    {
      if (t->state_instance->rule_instance == state->rule_instance)
	{
	  DebugLog(DF_ENG, DS_TRACE, "Marking thread %p as KILLED\n", t);
	  t->flags |= THREAD_KILLED;
	}
    }
  PUSH_RETURN_TRUE(ctx);
}

static void do_recursive_cut(state_instance_t *state)
{
  wait_thread_t *t;

  state->flags |= SF_PRUNED;

  /* Cut current state instance threads */
  for (t = state->thread_list; t; t = t->next_in_state_instance)
    {
      if ( !(t->flags & THREAD_ONLYONCE) )
	{
	  DebugLog(DF_ENG, DS_TRACE, "Marking thread %p as KILLED (cut)\n", t);
	  t->flags |= THREAD_KILLED;
	}
    }

  /* Then call recursively */
  if (state->first_child)
    do_recursive_cut(state->first_child);

  if (state->next_sibling)
    do_recursive_cut(state->next_sibling);
}

static int hybrid_strcmp (char *s, char *t, size_t slen)
{
  size_t i;
  char c;

  for (i=0; i<slen; i++)
    {
      c = *t++;
      if (c=='\0')
	return -1;
      if (c != *s++)
	return 1;
    }
  return (*t == '\0');
}

static void issdl_cut(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *str;
  state_instance_t *si;
  char *s;
  size_t len;

  /* First poor implementation of cuts:
   * The cut should be included in the language, and should also be resolved
   * at compilation-time to apply syntactic restrictions and avoid problems
   * (ONE CUT per state MAX! And ensure at the compilation time that the
   * destination exists and is reachable) */

  str = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);

  if (str==NULL)
    {
      DebugLog(DF_ENG, DS_ERROR, "parameter type error\n");
      /* FREE_IF_NEEDED(str); */
      PUSH_RETURN_FALSE(ctx);
      return;
    }
  switch (TYPE(str))
    {
    case T_STR: s = STR(str); len = STRLEN(str);break;
    case T_VSTR: s = VSTR(str); len = VSTRLEN(str); break;
    default:
      DebugLog(DF_ENG, DS_ERROR, "parameter type error\n");
      STACK_DROP(ctx->ovm_stack, 1);
      PUSH_RETURN_FALSE(ctx);
      return;
    }

  /* Find the destination of the cut */
  for (si = state;
       si->parent && hybrid_strcmp(s, si->state->name, len);
       si = si->parent)
    ;
  DebugLog(DF_ENG, DS_INFO, "found cut dest @ %s:%p\n", si->state->name, si);
  do_recursive_cut(si);
  STACK_DROP(ctx->ovm_stack, 1);
  PUSH_RETURN_TRUE(ctx);
}

char *ovm_strdup(gc_t *gc_ctx, ovm_var_t *str)
{
  char *s;
  size_t len;

  if (str!=NULL && TYPE(str) == T_STR)
    {
      len = STRLEN(str);
      s = gc_base_malloc(gc_ctx, len+1);
      strncpy(s, STR(str), len);
      s[len] = '\0';
    }
  else if (str!=NULL && TYPE(str) == T_VSTR)
    {
      len = VSTRLEN(str);
      s = gc_base_malloc(gc_ctx, len+1);
      strncpy(s, VSTR(str), len);
      s[len] = '\0';
    }
  else
    {
      DebugLog(DF_ENG, DS_ERROR, "ovm_strdup type error\n");
      s = NULL;
    }
  return s;
}

static void issdl_report(orchids_t *ctx, state_instance_t *state)
{
  reportmod_t* r;

  DebugLog(DF_ENG, DS_INFO, "Generating report\n");
  if (state->rule_instance == NULL)
  {
    PUSH_RETURN_FALSE(ctx);
    return;
  }
  SLIST_FOREACH(r, &ctx->reportmod_list, list) {
    (*r->cb) (ctx, r->mod, r->data, state);
  }
  ctx->reports++;
  PUSH_RETURN_TRUE(ctx);
}


static void issdl_noop(orchids_t *ctx, state_instance_t *state)
{
  DebugLog(DF_ENG, DS_INFO, "No-Operation called\n");
  PUSH_VALUE(ctx, NULL);
}

static void
issdl_sendmail(orchids_t *ctx, state_instance_t *state)
{
  int pid;
  int tmp_fd;
  FILE *ftmp;
  char tmpfile[PATH_MAX];
  ovm_var_t *from;
  ovm_var_t *to;
  ovm_var_t *subject;
  ovm_var_t *body;
  char *from_str=NULL, *to_str=NULL, *subject_str=NULL, *body_str=NULL;
  size_t body_len;

  body = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  subject = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  to = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 3);
  from = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 4);

  if (from!=NULL)
    switch (TYPE(from))
      {
      case T_STR: case T_VSTR:
	from_str = ovm_strdup (ctx->gc_ctx, from);
	break;
      default:
	DebugLog(DF_OVM, DS_DEBUG, "issdl_sendmail(): param error: from\n");
	STACK_DROP(ctx->ovm_stack, 4);
	PUSH_RETURN_FALSE(ctx);
	return;
      }
  if (to!=NULL)
    switch (TYPE(to))
      {
      case T_STR: case T_VSTR:
	to_str = ovm_strdup (ctx->gc_ctx, to);
	break;
      default:
	DebugLog(DF_OVM, DS_DEBUG, "issdl_sendmail(): param error: to\n");
	STACK_DROP(ctx->ovm_stack, 4);
	PUSH_RETURN_FALSE(ctx);
	return;
      }
  if (subject!=NULL)
    switch (TYPE(subject))
      {
      case T_STR: case T_VSTR:
	subject_str = ovm_strdup (ctx->gc_ctx, subject);
	break;
      default:
	DebugLog(DF_OVM, DS_DEBUG, "issdl_sendmail(): param error: subject\n");
	STACK_DROP(ctx->ovm_stack, 4);
	PUSH_RETURN_FALSE(ctx);
	return;
      }
  if (body!=NULL)
    switch (TYPE(body))
      {
      case T_STR: body_str = STR(body); body_len = STRLEN(body); break;
      case T_VSTR: body_str = VSTR(body); body_len = VSTRLEN(body); break;
      default:
	DebugLog(DF_OVM, DS_DEBUG, "issdl_sendmail(): param error: body\n");
	STACK_DROP(ctx->ovm_stack, 4);
	PUSH_RETURN_FALSE(ctx);
	return;
      }
  else
    {
      body_str = NULL;
      body_len = 0;
    }
  DebugLog(DF_ENG, DS_INFO,
           "Sendmail from:%s to:%s subject:%s\n",
           (from_str==NULL)?"(null)":from_str,
	   (to_str==NULL)?"(null)":to_str,
	   (subject==NULL)?"(null)":subject_str);

  pid = fork();
  if (pid == 0) {
    strcpy(tmpfile, ORCHIDS_SENDMAIL_TEMPFILE );
    tmp_fd = mkstemp(tmpfile);
    ftmp = Xfdopen(tmp_fd, "w+");
    Xunlink(tmpfile);

    if (to_str)
      {
	fputs("To: ", ftmp);
	fputs(to_str, ftmp);
	fputs("\n", ftmp);
      }
    if (from_str)
      {
	fputs("From: ", ftmp);
	fputs(from_str, ftmp);
	fputs("\n", ftmp);
      }
    if (subject_str)
      {
	fputs("Subject: ", ftmp);
	fputs(subject_str, ftmp);
	fputs("\n", ftmp);
      }
    fputs("\n", ftmp);
    if (body_str)
      {
	size_t i;
 
	for (i=0; i<body_len; i++)
	  {
	    putc (body_str[i], ftmp);
	  }
      }
    rewind(ftmp);
    Xdup2(fileno(ftmp), fileno(stdin));
    execl(PATH_TO_SENDMAIL, "sendmail", "-odi", to_str, NULL);
    DebugLog(DF_ENG, DS_ERROR, "execl(): error %i: %s\n",
             errno, strerror(errno));
    exit(EXIT_FAILURE);
  }
  else if (pid < 0) {
    DebugLog(DF_ENG, DS_ERROR, "fork(): error %i: %s\n",
             errno, strerror(errno));
    STACK_DROP(ctx->ovm_stack, 4);
    PUSH_RETURN_FALSE(ctx);
  }
  else {
    /* parent returns */
    STACK_DROP(ctx->ovm_stack, 4);
    PUSH_RETURN_TRUE(ctx);
  }
  if (from_str!=NULL)
    gc_base_free (from_str);
  if (to_str!=NULL)
    gc_base_free (to_str);
  if (subject_str!=NULL)
    gc_base_free (subject_str);
}


static void
issdl_sendmail_report(orchids_t *ctx, state_instance_t *state)
{
  int pid;
  int tmp_fd;
  FILE *ftmp;
  char tmpfile[PATH_MAX];
  ovm_var_t *from;
  ovm_var_t *to;
  ovm_var_t *subject;
  ovm_var_t *body;
  state_instance_t *report_events;
  char *from_str=NULL, *to_str=NULL, *subject_str=NULL, *body_str=NULL;
  size_t body_len;

  body = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  subject = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  to = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 3);
  from = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 4);

  if (from!=NULL)
    switch (TYPE(from))
      {
      case T_STR: case T_VSTR:
	from_str = ovm_strdup (ctx->gc_ctx, from);
	break;
      default:
	DebugLog(DF_OVM, DS_DEBUG, "issdl_sendmail(): param error: from\n");
	STACK_DROP(ctx->ovm_stack, 4);
	PUSH_RETURN_FALSE(ctx);
	return;
      }
  if (to!=NULL)
    switch (TYPE(to))
      {
      case T_STR: case T_VSTR:
	to_str = ovm_strdup (ctx->gc_ctx, to);
	break;
      default:
	DebugLog(DF_OVM, DS_DEBUG, "issdl_sendmail(): param error: to\n");
	STACK_DROP(ctx->ovm_stack, 4);
	PUSH_RETURN_FALSE(ctx);
	return;
      }
  if (subject!=NULL)
    switch (TYPE(subject))
      {
      case T_STR: case T_VSTR:
	subject_str = ovm_strdup (ctx->gc_ctx, subject);
	break;
      default:
	DebugLog(DF_OVM, DS_DEBUG, "issdl_sendmail(): param error: subject\n");
	STACK_DROP(ctx->ovm_stack, 4);
	PUSH_RETURN_FALSE(ctx);
	return;
      }
  if (body!=NULL)
    switch (TYPE(body))
      {
      case T_STR: body_str = STR(body); body_len = STRLEN(body); break;
      case T_VSTR: body_str = VSTR(body); body_len = VSTRLEN(body); break;
      default:
	DebugLog(DF_OVM, DS_DEBUG, "issdl_sendmail_report(): param error: body\n");
	STACK_DROP(ctx->ovm_stack, 4);
	PUSH_RETURN_FALSE(ctx);
	return;
      }
  else
    {
      body_str = NULL;
      body_len = 0;
    }
  DebugLog(DF_ENG, DS_INFO,
           "Sendmail from:%s to:%s subject:%s\n",
           (from_str==NULL)?"(null)":from_str,
	   (to_str==NULL)?"(null)":to_str,
	   (subject==NULL)?"(null)":subject_str);
  strcpy(tmpfile, ORCHIDS_SENDMAIL_TEMPFILE );
  tmp_fd = mkstemp(tmpfile);
  ftmp = Xfdopen(tmp_fd, "w+");
  Xunlink(tmpfile);
  if (to_str)
    {
      fputs("To: ", ftmp);
      fputs(to_str, ftmp);
      fputs("\n", ftmp);
    }
  if (from_str)
    {
      fputs("From: ", ftmp);
      fputs(from_str, ftmp);
      fputs("\n", ftmp);
    }
  if (subject_str)
    {
      fputs("Subject: ", ftmp);
      fputs(subject_str, ftmp);
      fputs("\n", ftmp);
    }
  fputs("\n", ftmp);
  if (body_str)
    {
      size_t i;
 
      for (i=0; i<body_len; i++)
	{
	  putc (body_str[i], ftmp);
	}
    }
  /* report */
  for (report_events = NULL; state!=NULL; state = state->parent)
    {
      state->next_report_elmt = report_events;
      report_events = state;
    }
  fprintf(ftmp, "Report for rule: %s\n\n",
	  report_events->rule_instance->rule->name);
  for ( ; report_events!=NULL ;
	report_events = report_events->next_report_elmt)
    {
      fprintf(ftmp, "State: %s\n", report_events->state->name);
      if (report_events->event!=NULL)
        fprintf_event(ftmp, ctx, report_events->event->event);
      else
        fprintf(ftmp, "no event.\n");
      fprintf_state_env(ftmp, ctx, report_events);
    }
  rewind(ftmp);
  /* Now call sendmail */
  pid = fork();
  if (pid == 0)
    {
      Xdup2(fileno(ftmp), fileno(stdin));
      execl(PATH_TO_SENDMAIL, "sendmail", "-odi", to_str, NULL);
      DebugLog(DF_ENG, DS_ERROR, "execl(): error %i: %s\n",
	       errno, strerror(errno));
      exit(EXIT_FAILURE);
    }
  else if (pid < 0)
    {
      DebugLog(DF_ENG, DS_ERROR, "fork(): error %i: %s\n",
	       errno, strerror(errno));
      STACK_DROP(ctx->ovm_stack, 4);
      PUSH_RETURN_FALSE(ctx);
    }
  else
    {
      /* parent returns */
      STACK_DROP(ctx->ovm_stack, 4);
      PUSH_RETURN_TRUE(ctx);
    }
  if (from_str!=NULL)
    gc_base_free (from_str);
  if (to_str!=NULL)
    gc_base_free (to_str);
  if (subject_str!=NULL)
    gc_base_free (subject_str);
}

static void issdl_drop_event(orchids_t *ctx, state_instance_t *state)
{
  if (state->event == NULL)
    {
      DebugLog(DF_ENG, DS_ERROR, "error: state instance does not have event reference\n");
      PUSH_RETURN_FALSE(ctx);
      return;
    }
  state->event_level = -1;
  PUSH_RETURN_TRUE(ctx);
}

static void issdl_set_event_level(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *level;

  level = POP_VALUE(ctx);
  if (state->event == NULL)
    {
      DebugLog(DF_ENG, DS_ERROR, "state instance does not have event reference\n");
    fail:
      PUSH_RETURN_FALSE(ctx);
      return;
  }
  if (level==NULL || TYPE(level) != T_INT) {
    DebugLog(DF_ENG, DS_ERROR, "parameter type error\n");
    goto fail;
  }
  state->event_level = INT(level);
  PUSH_RETURN_TRUE(ctx);
}


/* Compute the number of different bits between two variables of
   the same type and size (binary distance). */
static void
issdl_bindist(orchids_t *ctx, state_instance_t *state)
{
  static int bitcnt_tbl[] = { /* Precomputed table of 1-bit in each bytes */
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
  };

  ovm_var_t *param1;
  unsigned char *data1;
  size_t len;
  ovm_var_t *param2;
  unsigned char *data2;
  size_t len2;
  int dist;
  ovm_var_t *d;

  param2 = POP_VALUE(ctx);
  param1 = POP_VALUE(ctx);
  if (param1==NULL)
    {
      DebugLog(DF_OVM, DS_DEBUG, "Null param1\n");
      PUSH_VALUE(ctx, NULL);
      return;
    }
  if (param2==NULL)
    {
      DebugLog(DF_OVM, DS_DEBUG, "Null param2\n");
      PUSH_VALUE(ctx, NULL);
      return;
    }
  len = issdl_get_data_len(param1);
  len2 = issdl_get_data_len(param2);
  if (len != len2)
    {
      DebugLog(DF_OVM, DS_DEBUG, "Size error\n");
      PUSH_VALUE(ctx, NULL);
    }
  else
    {
      dist = 0;
      data1 = issdl_get_data(param1);
      data2 = issdl_get_data(param2);
      for ( ; len > 0; len--)
	{
	  int x = *data1++ ^ *data2++;
	  dist += bitcnt_tbl[x];
	}
      d = ovm_int_new(ctx->gc_ctx, dist);
      PUSH_VALUE (ctx, d);
      DebugLog(DF_OVM, DS_DEBUG, "Computed bit distance %i\n", dist);
    }
}

static void issdl_bytedist(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *param1;
  unsigned char *data1;
  size_t len;
  ovm_var_t *param2;
  unsigned char *data2;
  size_t len2;
  int dist;
  ovm_var_t *d;

  param2 = POP_VALUE(ctx);
  param1 = POP_VALUE(ctx);
  if (param1==NULL)
    {
      DebugLog(DF_OVM, DS_DEBUG, "Null param1\n");
      PUSH_VALUE(ctx, NULL);
      return;
    }
  if (param2==NULL)
    {
      DebugLog(DF_OVM, DS_DEBUG, "Null param2\n");
      PUSH_VALUE(ctx, NULL);
      return;
    }
  len = issdl_get_data_len(param1);
  len2 = issdl_get_data_len(param2);
  if (len != len2)
    {
      DebugLog(DF_OVM, DS_DEBUG, "Size error\n");
      PUSH_VALUE(ctx, NULL);
    }
  else
    {
      dist = 0;
      data1 = issdl_get_data(param1);
      data2 = issdl_get_data(param2);
      for ( ; len > 0; len--)
	{
	  if (*data1++ != *data2++)
	    dist++;
	}
      d = ovm_int_new(ctx->gc_ctx, dist);
      PUSH_VALUE (ctx, d);
      DebugLog(DF_OVM, DS_DEBUG, "Computed byte distance %i\n", dist);
    }
}

static void issdl_vstr_from_regex(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *vstr;
  ovm_var_t *regex;

  regex = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  if (regex!=NULL && TYPE(regex) == T_REGEX)
    {
      if (REGEXSTR(regex)==NULL)
	{
	  STACK_DROP(ctx->ovm_stack, 1);
	  PUSH_VALUE(ctx, NULL);
	}
      else
	{
	  vstr = ovm_vstr_new (ctx->gc_ctx, regex);
	  VSTR(vstr) = REGEXSTR(regex);
	  VSTRLEN(vstr) = strlen(REGEXSTR(regex));
	  STACK_DROP(ctx->ovm_stack, 1);
	  PUSH_VALUE(ctx, vstr);
	}
    }
  else
    {
      DebugLog(DF_OVM, DS_ERROR, "issdl_vstr_from_regex(): type error\n");
      STACK_DROP(ctx->ovm_stack, 1);
      PUSH_VALUE(ctx, NULL);
  }
}

static void issdl_regex_from_str(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *str;
  ovm_var_t *regex;

  str = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  if (str && (TYPE(str) == T_STR || TYPE(str) == T_VSTR))
    {
      int ret;
      char *s;

      s = ovm_strdup(ctx->gc_ctx, str);
      regex = ovm_regex_new (ctx->gc_ctx);
      ret = regcomp(&(REGEX(regex)), s, REG_EXTENDED | REG_NOSUB);
      if (ret)
	{
	  char err_buf[512];

	  regerror(ret, &(REGEX(regex)), err_buf, sizeof (err_buf));
	  DebugLog(DF_OVM, DS_ERROR,
		   "issdl_regex_from_str(): "
		   "compilation error in \"%s\": %s\n", s, err_buf);
	  regex = NULL;
	  gc_base_free (s);
	}
      else
	{
	  REGEXSTR(regex) = s;
	}
    }
  else
    {
      DebugLog(DF_OVM, DS_ERROR, "issdl_regex_from_str(): param error\n");
      regex = NULL;
    }
  STACK_DROP(ctx->ovm_stack, 1);
  PUSH_VALUE(ctx, regex);
}

static void issdl_defined(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *field;

  field = POP_VALUE(ctx);
  if (IS_NULL(field))
    PUSH_RETURN_FALSE(ctx);
  else
    PUSH_RETURN_TRUE(ctx);
}

static void issdl_difftime(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *t1, *t2, *res;

  t1 = POP_VALUE(ctx);
  t2 = POP_VALUE(ctx);
  if (t1!=NULL && TYPE(t1)==T_CTIME && t2!=NULL && TYPE(t2)==T_CTIME)
  {
    res = ovm_int_new (ctx->gc_ctx, difftime(CTIME(t1), CTIME(t2)));
    PUSH_VALUE (ctx, res);
  }
  else
    PUSH_VALUE(ctx, NULL);
}

static void issdl_str_from_time(orchids_t *ctx, state_instance_t *state)
{
  char buff[256];
  ovm_var_t *str;
  ovm_var_t *t;
  struct tm time;
  int len;

  t = POP_VALUE(ctx);
  if (t!=NULL && TYPE(t) == T_CTIME)
    {
      time = *localtime (&CTIME(t));
      len = strftime(buff, sizeof(buff), "%Y-%m-%dT%H:%M:%S%z", &time);
      str = ovm_str_new(ctx->gc_ctx, len);
      memcpy(STR(str), buff, len);
      PUSH_VALUE(ctx,str);
    }
  else {
    DebugLog(DF_OVM, DS_DEBUG, "issdl_str_from_time(): param error\n");
    PUSH_VALUE(ctx, NULL);
  }
}

static void issdl_str_from_timeval(orchids_t *ctx, state_instance_t *state)
{
  char buff[256];
  ovm_var_t *str;
  ovm_var_t *t;
  struct timeval *time;
  int len;

  t = POP_VALUE(ctx);
  if (t!=NULL && TYPE(t) == T_TIMEVAL)
    {
      time = &TIMEVAL(t);
      len = snprintf(buff, sizeof(buff), "%li.%06li",
		     (long)time->tv_sec, (long)time->tv_usec);
      str = ovm_str_new(ctx->gc_ctx, len);
      memcpy(STR(str), buff, len);
      PUSH_VALUE(ctx,str);
    }
  else {
    DebugLog(DF_OVM, DS_DEBUG, "issdl_str_from_timeval(): param error\n");
    PUSH_VALUE(ctx, NULL);
  }
}


ovm_var_t *ovm_read_value (ovm_var_t *env, unsigned long var)
{
  unsigned long mask;
  ovm_var_t *res;

  res = NULL;
  for (mask = 1L; env!=NULL; mask <<= 1)
    if (TYPE(env)==T_BIND)
      {
	if (((env_bind_t *)env)->var==var)
	  res = ((env_bind_t *)env)->val;
	break;
      }
    else /* T_SPLIT */
      if (var & mask)
	env = ((env_split_t *)env)->right;
      else env = ((env_split_t *)env)->left;
  return res;
}

/**
 ** Table of built-in function of the Orchids language.  This table is
 ** used at startup by the function register_core_functions() to
 ** register them.
 **/
static issdl_function_t issdl_function_g[] = {
  { issdl_noop, 0, "noop", 0, "No Operation function" },
  { issdl_print, 1, "print", 1, "display a string (TEST FUNCTION)" },
  { issdl_dumpstack, 2, "dump_stack", 0, "dump the stack of the current rule" },
  { issdl_printevent, 3, "print_event", 0, "print the event associated with state" }, // OBSOLETE, use print(current_event())
  { issdl_dumppathtree, 4, "dump_dot_pathtree", 0, "dump the rule instance path tree in the GraphViz Dot format"},
  { issdl_drop_event, 5, "drop_event", 0, "Drop event" },
  { issdl_set_event_level, 6, "set_event_level", 1, "Set event level" },
  { issdl_report, 7, "report", 0, "generate report" },

  { issdl_shutdown, 8, "shutdown", 0, "shutdown orchids" },

  { issdl_random, 9, "random", 0, "return a random number" },
  { issdl_system, 10, "system", 1, "execute a system command" },
  { issdl_stats, 11, "show_stats", 0, "show orchids internal statistics" },
  { issdl_str_from_int, 12, "str_from_int", 1, "convert an integer to a string" },
  { issdl_int_from_str, 13, "int_from_str", 1, "convert a string to an integer" },
  { issdl_float_from_str, 14, "float_from_str", 1, "convert a string to a float" },
  { issdl_str_from_float, 15, "str_from_float", 1, "convert a float to a string" },
  { issdl_str_from_ipv4, 16, "str_from_ipv4", 1, "convert an ipv4 address to a string" },
  { issdl_kill_threads, 17, "kill_threads", 0, "kill threads of a rule instance" },
  { issdl_cut, 18, "cut", 1, "special cut" },
  { issdl_sendmail, 19, "sendmail", 4, "Send a mail" },
  { issdl_sendmail_report, 20, "sendmail_report", 4, "Send a report by mail" },
  { issdl_bindist, 21, "bitdist", 2, "Number of different bits" },
  { issdl_bytedist, 22, "bytedist", 2, "Number of different bytes" },
  { issdl_vstr_from_regex, 23, "str_from_regex", 1, "Return the source string of a compiled regex" },
  { issdl_regex_from_str, 24, "regex_from_str", 1, "Compile a regex from a string" },
  { issdl_defined, 25, "defined", 1, "Return if a field is defined" },
  { issdl_difftime, 26, "difftime", 2, "The difftime() function shall return the difference expressed in seconds as a type int."},
  { issdl_str_from_time, 27, "str_from_time", 1, "convert a time to a string" },
  { issdl_str_from_timeval, 28, "str_from_timeval", 1, "convert a timeval to a string" },
  { issdl_time_from_str, 29, "time_from_str", 1, "convert a string to a time" },
  { issdl_timeval_from_str, 30, "timeval_from_str", 1, "convert a string to a timeval" },
  { issdl_str_from_ipv6, 31, "str_from_ipv6", 1, "convert an ipv6 address to a string" },
  { NULL, 0, NULL, 0, NULL }
};

void register_core_functions(orchids_t *ctx)
{
  issdl_function_t *f;

  for (f = issdl_function_g; f->func!=NULL; f++)
    register_lang_function(ctx, f->func, f->name, f->args_nb, f->desc);
}

void register_lang_function(orchids_t *ctx,
			    ovm_func_t func,
			    const char *name,
			    int arity,
			    const char *desc)
{
  issdl_function_t *f;
  size_t array_size;
  size_t len;

  DebugLog(DF_ENG, DS_INFO,
           "Registering language function %s/%i @ %p\n",
           name, arity, func);
  array_size = (ctx->vm_func_tbl_sz + 1) * sizeof (issdl_function_t);
  ctx->vm_func_tbl = gc_base_realloc (ctx->gc_ctx,
				      ctx->vm_func_tbl, array_size);
  f = &ctx->vm_func_tbl[ctx->vm_func_tbl_sz];
  f->func = func;
  f->id = ctx->vm_func_tbl_sz;
  len = strlen (name);
  f->name = gc_base_malloc (ctx->gc_ctx, len+1);
  strcpy (f->name, name);
  f->args_nb = arity;
  len = strlen (desc);
  f->desc = gc_base_malloc (ctx->gc_ctx, len+1);
  strcpy (f->desc, desc);
  ctx->vm_func_tbl_sz++;
}

issdl_function_t *get_issdl_functions(void)
{
  return issdl_function_g;
}

void fprintf_issdl_functions(FILE *fp, orchids_t *ctx)
{
  int i;

  fprintf(fp, "-----------[ registered language functions ]---\n");
  fprintf(fp, "         function name | description \n");
  fprintf(fp, "-----------------------------------------------\n");
  for (i = 0; i < ctx->vm_func_tbl_sz; i++) {
    fprintf(fp, "%20s() | %s\n",
            ctx->vm_func_tbl[i].name,
            ctx->vm_func_tbl[i].desc);
  }
}


void set_ip_resolution(int value)
{
  resolve_ipv6_g = resolve_ipv4_g = value ? TRUE : FALSE;
}


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
