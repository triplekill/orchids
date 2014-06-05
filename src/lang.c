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

#include "orchids.h"
#include "hash.h"
#include "safelib.h"
#include "dlist.h"
#include "timer.h"
#include "graph_output.h"
#include "orchids_api.h"

#include "ovm.h"
#include "lang.h"
#include "lang_priv.h"

/**
 ** Table of data type natively recognized in the Orchids language.
 **/
static struct issdl_type_s issdl_types_g[] = {
  { "null",    0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "Null type for error/exception managmnent" },
  { "func",    0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "Function reference" },
  { "int",     0, int_get_data, int_get_data_len, int_cmp, int_add, int_sub, int_mul, int_div, int_mod, int_clone, NULL, "Integer numbers (32-bits signed int)" },
  { "bstr",    0, bytestr_get_data, bytestr_get_data_len, NULL, NULL, NULL, NULL, NULL, NULL, bstr_clone, NULL, "Binary string, allocated, (unsigned char *)" },
  { "vbstr",   0, vbstr_get_data, vbstr_get_data_len, NULL, NULL, NULL, NULL, NULL, NULL, vbstr_clone, NULL, "Virtual binary string, not allocated, only pointer/offset reference" },
  { "str",     0, string_get_data, string_get_data_len, str_cmp, str_add, NULL, NULL, NULL, NULL, string_clone, NULL, "Character string, allocated, (char *)" },
  { "vstr",    0, vstring_get_data, vstring_get_data_len, vstr_cmp, vstr_add, NULL, NULL, NULL, NULL, vstr_clone, NULL, "Virtual string, not allocated, only pointer/offset reference" },
  { "array",   0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "Array" },
  { "hash",    0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "Hash table" },
  { "ctime",   0, ctime_get_data, ctime_get_data_len, ctime_cmp, ctime_add, ctime_sub, ctime_mul, ctime_div, ctime_mod, ctime_clone, NULL, "C Time, seconds since Epoch (Jan. 1, 1970, 00:00 GMT), (time_t)" },
  { "ipv4",    0, ipv4_get_data, ipv4_get_data_len, ipv4_cmp, NULL, NULL, NULL, NULL, NULL, ipv4_clone, NULL, "IPv4 address (struct in_addr)" },
  { "timeval", 0, timeval_get_data, timeval_get_data_len, timeval_cmp, timeval_add, timeval_sub, timeval_mul, timeval_div, timeval_mod, timeval_clone, NULL, "Seconds and microseconds since Epoch, (struct timeval)" },
  { "counter", 0, counter_get_data, counter_get_data_len, counter_cmp, counter_add, NULL, counter_mul, NULL, NULL, counter_clone, NULL, "Counter (a monotonic integer)" },
  { "regex",   0, regex_get_data, regex_get_data_len, NULL, NULL, NULL, NULL, NULL, NULL, NULL, regex_destruct, "Posix Extended Regular Expression, without substring addressing" },
  { "sregex",  0, splitregex_get_data, splitregex_get_data_len, NULL, NULL, NULL, NULL, NULL, NULL, NULL, splitregex_destruct, "Posix Extended Regular Expression, with substring addressing" },
  { "ptr32",   0, address_get_data, address_get_data_len, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "32-bit memory pointer" },
  { "uint",    0, uint_get_data, uint_get_data_len, uint_cmp, uint_add, uint_sub, uint_mul, uint_div, uint_mod, uint_clone, NULL, "Non negative integer (32-bits unsigned int)" },
  { "snmpoid", 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "SNMP Object Identifier" },
  { "float",   0, float_get_data, float_get_data_len, float_cmp, float_add, float_sub, float_mul, float_div, NULL, float_clone, NULL, "IEEE 32-bit floating point number (float)" },
  { "double",  0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "IEEE 64-bit floating point number (double)" },
  { "extern",  0, extern_get_data, extern_get_data_len, NULL, NULL, NULL, NULL, NULL, NULL, NULL, extern_destruct, "External data (provided by a plugin)" },
  { NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "" }
};

static int resolve_ipv4_g = 0;

char *
str_issdltype(int type)
{
  return (issdl_types_g[type].name);
}

issdl_type_t *
issdlgettypes(void)
{
  return (issdl_types_g);
}

void
issdl_free(ovm_var_t	*var)
{
  if (!var)
    return;

  if (issdl_types_g[TYPE(var)].destruct)
    issdl_types_g[TYPE(var)].destruct(var);

  Xfree(var);
}

/*
** Null
** Type null is used in issdl mainly to report native function errors
*/

ovm_var_t *
ovm_null_new(void)
{
  ovm_null_t *n;

  n = Xmalloc(sizeof (ovm_null_t));
  n->type = T_NULL;
  n->flags = 0;
  n->err_no = 1;

  return ( OVM_VAR(n) );
}

void
ovm_null_fprintf(FILE *fp, ovm_null_t *val)
{
  if (val->type != T_NULL)
    {
      fprintf(fp, "Wrong object type.\n");
      return ;
    }

  /* Add errno explication */
  fprintf(fp, "null (%i)\n", ERRNO(val));
}

/*
** Integer type
** store an 'signed int' value.
*/

ovm_var_t *
ovm_int_new(void)
{
  ovm_int_t *n;

  n = Xmalloc(sizeof (ovm_int_t));
  n->type = T_INT;
  n->flags = 0;
  n->val = 0;

  return ( OVM_VAR(n) );
}

static void *
int_get_data(ovm_var_t *i)
{
  return ( &((ovm_int_t *)i)->val );
}

static size_t
int_get_data_len(ovm_var_t *i)
{
  return (sizeof (long));
}

void
ovm_int_fprintf(FILE *fp, ovm_int_t *val)
{
  if (val->type != T_INT)
    {
      fprintf(fp, "Wrong object type.\n");
      return ;
    }

  fprintf(fp, "int : %li\n", val->val);
}

static int
int_cmp(ovm_var_t *var1, ovm_var_t *var2)
{
  if (TYPE(var2) != T_INT)
    return (TYPE(var1) - TYPE(var2));

  return (INT(var1) - INT(var2));
}

static ovm_var_t *
int_add(ovm_var_t *var1, ovm_var_t *var2)
{
  ovm_var_t *res;

  if (var2->type != T_INT)
    return (NULL);

  res = ovm_int_new();
  INT(res) = INT(var1) + INT(var2);
  FLAGS(res) |= TYPE_CANFREE | TYPE_NOTBOUND;

  return (res);
}

static ovm_var_t *
int_sub(ovm_var_t *var1, ovm_var_t *var2)
{
  ovm_var_t *res;

  if (var2->type != T_INT)
    return (NULL);

  res = ovm_int_new();
  INT(res) = INT(var1) - INT(var2);
  FLAGS(res) |= TYPE_CANFREE | TYPE_NOTBOUND;

  return (res);
}

static ovm_var_t *
int_mul(ovm_var_t *var1, ovm_var_t *var2)
{
  ovm_var_t *res;

  if (var2->type != T_INT)
    return (NULL);

  res = ovm_int_new();
  INT(res) = INT(var1) * INT(var2);
  FLAGS(res) |= TYPE_CANFREE | TYPE_NOTBOUND;

  return (res);
}

static ovm_var_t *
int_div(ovm_var_t *var1, ovm_var_t *var2)
{
  ovm_var_t *res;

  if (var2->type != T_INT)
    return (NULL);

  if (INT(var2) == 0)
    return (NULL);

  res = ovm_int_new();
  INT(res) = INT(var1) / INT(var2);
  FLAGS(res) |= TYPE_CANFREE | TYPE_NOTBOUND;

  return (res);
}

static ovm_var_t *
int_mod(ovm_var_t *var1, ovm_var_t *var2)
{
  ovm_var_t *res;

  if (var2->type != T_INT)
    return (NULL);

  if (INT(var2) == 0)
    return (NULL);

  res = ovm_int_new();
  INT(res) = INT(var1) % INT(var2);
  FLAGS(res) |= TYPE_CANFREE | TYPE_NOTBOUND;

  return (res);
}



static ovm_var_t *
int_clone(ovm_var_t *var)
{
  ovm_var_t *res;

  if (var->type != T_INT)
    return (NULL);

  res = ovm_int_new();
  INT(res) = INT(var);
  FLAGS(res) |= TYPE_CANFREE | TYPE_NOTBOUND;

  return (res);
}


/*
** Unsigned integer type
** store an 'uint32_t' value.
*/

ovm_var_t *
ovm_uint_new(void)
{
  ovm_uint_t *n;

  n = Xmalloc(sizeof (ovm_uint_t));
  n->type = T_UINT;
  n->flags = 0;
  n->val = 0;

  return ( OVM_VAR(n) );
}

static void *
uint_get_data(ovm_var_t *i)
{
  return ( &((ovm_uint_t *)i)->val );
}

static size_t
uint_get_data_len(ovm_var_t *i)
{
  return (sizeof (unsigned long));
}

void
ovm_uint_fprintf(FILE *fp, ovm_uint_t *val)
{
  if (val->type != T_UINT)
    {
      fprintf(fp, "Wrong object type.\n");
      return ;
    }

  fprintf(fp, "uint : %lu\n", val->val);
}

static int
uint_cmp(ovm_var_t *var1, ovm_var_t *var2)
{
  if (TYPE(var2) != T_UINT)
    return (TYPE(var1) - TYPE(var2));

  return (UINT(var1) - UINT(var2));
}

static ovm_var_t *
uint_add(ovm_var_t *var1, ovm_var_t *var2)
{
  ovm_var_t *res;

  if (var2->type != T_UINT)
    return (NULL);

  res = ovm_uint_new();
  UINT(res) = UINT(var1) + UINT(var2);
  FLAGS(res) |= TYPE_CANFREE | TYPE_NOTBOUND;

  return (res);
}

static ovm_var_t *
uint_sub(ovm_var_t *var1, ovm_var_t *var2)
{
  ovm_var_t *res;

  if (var2->type != T_UINT && UINT(var1) > UINT(var2))
    return (NULL);

  res = ovm_uint_new();
  UINT(res) = UINT(var1) - UINT(var2);
  FLAGS(res) |= TYPE_CANFREE | TYPE_NOTBOUND;

  return (res);
}

static ovm_var_t *
uint_mul(ovm_var_t *var1, ovm_var_t *var2)
{
  ovm_var_t *res;

  if (var2->type != T_UINT)
    return (NULL);

  res = ovm_uint_new();
  UINT(res) = UINT(var1) * UINT(var2);
  FLAGS(res) |= TYPE_CANFREE | TYPE_NOTBOUND;

  return (res);
}

static ovm_var_t *
uint_div(ovm_var_t *var1, ovm_var_t *var2)
{
  ovm_var_t *res;

  if (var2->type != T_UINT && UINT(var2) > 0)
    return (NULL);

  res = ovm_uint_new();
  UINT(res) = UINT(var1) / UINT(var2);
  FLAGS(res) |= TYPE_CANFREE | TYPE_NOTBOUND;

  return (res);
}

static ovm_var_t *
uint_mod(ovm_var_t *var1, ovm_var_t *var2)
{
  ovm_var_t *res;

  if (TYPE(var2) != T_UINT && UINT(var2) > 0)
    return (NULL);

  res = ovm_uint_new();
  UINT(res) = UINT(var1) % UINT(var2);
  FLAGS(res) |= TYPE_CANFREE | TYPE_NOTBOUND;

  return (res);
}

static ovm_var_t *
uint_clone(ovm_var_t *var)
{
  ovm_var_t *res;

  if (TYPE(var) != T_UINT)
    return (NULL);

  res = ovm_uint_new();
  UINT(res) = UINT(var);
  FLAGS(res) |= TYPE_CANFREE | TYPE_NOTBOUND;

  return (res);
}



/*
** Binary String
** used to store binary objects.
*/

ovm_var_t *
ovm_bstr_new(size_t size)
{
  ovm_bstr_t *bstr;

  bstr = Xzmalloc(sizeof (ovm_bstr_t) - STR_PAD_LEN + size);
  bstr->type = T_BSTR;
  bstr->flags = 0;
  bstr->len = size;

  return ( OVM_VAR(bstr) );
}

static ovm_var_t *
bstr_clone(ovm_var_t *var)
{
  ovm_var_t *res;

  if (var->type != T_BSTR)
    return (NULL);

  res = ovm_bstr_new( BSTRLEN(var) );
  memcpy( BSTR(res), BSTR(var), BSTRLEN(var));
  FLAGS(res) |= TYPE_CANFREE | TYPE_NOTBOUND;

  return (res);
}


static void *
bytestr_get_data(ovm_var_t *str)
{
  return ( ((ovm_bstr_t *)str)->str );
}

static size_t
bytestr_get_data_len(ovm_var_t *str)
{
  return (((ovm_bstr_t *)str)->len);
}

void
ovm_bstr_fprintf(FILE *fp, ovm_bstr_t *str)
{
  int i;

  if (str->type != T_BSTR)
    {
      fprintf(fp, "Wrong object type.\n");
      return ;
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

ovm_var_t *
ovm_vbstr_new(void)
{
  ovm_vbstr_t *vbstr;

  vbstr = Xzmalloc(sizeof (ovm_vbstr_t));
  vbstr->type = T_VBSTR;

  return ( OVM_VAR(vbstr) );
}

static ovm_var_t *
vbstr_clone(ovm_var_t *var)
{
  ovm_var_t *res;

  if (var->type != T_VBSTR)
    return (NULL);

  res = ovm_vbstr_new();
  VBSTR(res) = VBSTR(var);
  VBSTRLEN(res) = VBSTRLEN(var);
  FLAGS(res) |= TYPE_CANFREE | TYPE_NOTBOUND;

  return (res);
}


static void *
vbstr_get_data(ovm_var_t *vbstr)
{
  return ( VBSTR(vbstr) );
}

static size_t
vbstr_get_data_len(ovm_var_t *vbstr)
{
  return ( VBSTRLEN(vbstr) );
}

void
ovm_vbstr_fprintf(FILE *fp, ovm_vbstr_t *str)
{
  int i;

  if (str->type != T_VBSTR) {
    fprintf(fp, "Wrong object type.\n");
    return ;
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

ovm_var_t *
ovm_str_new(size_t size)
{
  ovm_str_t *str;

  /* remove STR_PAD_LEN for padding, add 1 for '\0' */
  str = Xzmalloc(sizeof (ovm_str_t) - STR_PAD_LEN + 1 + size);
  str->type = T_STR;
  str->flags = 0;
  str->len = size;

  return ( OVM_VAR(str) );
}

static void *
string_get_data(ovm_var_t *str)
{
  return ( ((ovm_str_t *)str)->str );
}

static size_t
string_get_data_len(ovm_var_t *str)
{
  return (((ovm_str_t *)str)->len);
}

static ovm_var_t *
string_clone(ovm_var_t *var)
{
  ovm_var_t *res;

  if (var->type != T_STR)
    return (NULL);

  res = ovm_str_new( STRLEN(var) );
  memcpy( STR(res), STR(var), STRLEN(var));
  FLAGS(res) |= TYPE_CANFREE | TYPE_NOTBOUND;

  return (res);
}

static ovm_var_t *
str_add(ovm_var_t *var1, ovm_var_t *var2)
{
  ovm_var_t *res;

  if (var2->type == T_STR) {
    res = ovm_str_new( STRLEN(var1) + STRLEN(var2) );
    memcpy(STR(res), STR(var1), STRLEN(var1));
    memcpy(STR(res) + STRLEN(var1), STR(var2), STRLEN(var2));
    FLAGS(res) |= TYPE_CANFREE | TYPE_NOTBOUND;

    return (res);
  }
  else if (var2->type == T_VSTR) {
    res = ovm_str_new( STRLEN(var1) + VSTRLEN(var2) );
    memcpy(STR(res), STR(var1), STRLEN(var1));
    memcpy(STR(res) + STRLEN(var1), VSTR(var2), VSTRLEN(var2));
    FLAGS(res) |= TYPE_CANFREE | TYPE_NOTBOUND;

    return (res);
  }

  DebugLog(DF_OVM, DS_DEBUG, "Type error\n");

  return (NULL);
}

static int
str_cmp(ovm_var_t *var1, ovm_var_t *var2)
{

  if (TYPE(var2) == T_STR)
    return ( memcmp(STR(var1), STR(var2), STRLEN(var1)) );
  else if (TYPE(var2) == T_VSTR)
    return ( memcmp(STR(var1), VSTR(var2), STRLEN(var1)) );

  DebugLog(DF_OVM, DS_DEBUG, "Type error\n");

  return (TYPE(var1) - TYPE(var2));
}

void
ovm_str_fprintf(FILE *fp, ovm_str_t *str)
{
  int i;

  if (str->type != T_STR)
    {
      fprintf(fp, "Wrong object type.\n");
      return ;
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

ovm_var_t *
ovm_vstr_new(void)
{
  ovm_vstr_t *vstr;

  vstr = Xmalloc(sizeof (ovm_vstr_t));
  vstr->type = T_VSTR;
  vstr->flags = 0;
  vstr->str = NULL;
  vstr->len = 0;

  return ( OVM_VAR(vstr) );
}

static void *
vstring_get_data(ovm_var_t *str)
{
  return ( ((ovm_vstr_t *)str)->str);
}

static size_t
vstring_get_data_len(ovm_var_t *str)
{
  return (((ovm_vstr_t *)str)->len);
}

static ovm_var_t *
vstr_clone(ovm_var_t *var)
{
  ovm_var_t *res;

  if (var->type != T_VSTR)
    return (NULL);

  res = ovm_vstr_new();
  VSTR(res) = VSTR(var);
  VSTRLEN(res) = VSTRLEN(var);
  FLAGS(res) |= TYPE_CANFREE | TYPE_NOTBOUND;

  return (res);
}

static ovm_var_t *
vstr_add(ovm_var_t *var1, ovm_var_t *var2)
{
  ovm_var_t *res;

  if (var2->type == T_STR) {
    res = ovm_str_new( VSTRLEN(var1) + STRLEN(var2) );
    memcpy(STR(res), VSTR(var1), VSTRLEN(var1));
    memcpy(STR(res) + VSTRLEN(var1), STR(var2), STRLEN(var2));
    FLAGS(res) |= TYPE_CANFREE | TYPE_NOTBOUND;

    return (res);
  }
  else if (var2->type == T_VSTR) {
    res = ovm_str_new( VSTRLEN(var1) + VSTRLEN(var2) );
    memcpy(VSTR(res), VSTR(var1), VSTRLEN(var1));
    memcpy(VSTR(res) + VSTRLEN(var1), VSTR(var2), VSTRLEN(var2));
    FLAGS(res) |= TYPE_CANFREE | TYPE_NOTBOUND;

    return (res);
  }

  DebugLog(DF_OVM, DS_DEBUG, "Type error\n");

  return (NULL);
}

static int
vstr_cmp(ovm_var_t *var1, ovm_var_t *var2)
{

  if (TYPE(var2) == T_STR)
    return ( memcmp(VSTR(var1), STR(var2), VSTRLEN(var1)) );
  else if (TYPE(var2) == T_VSTR)
    return ( memcmp(VSTR(var1), VSTR(var2), VSTRLEN(var1)) );

  DebugLog(DF_OVM, DS_DEBUG, "Type error\n");

  return (TYPE(var1) - TYPE(var2));
}


void
ovm_vstr_fprintf(FILE *fp, ovm_vstr_t *vstr)
{
  int i;

  if (vstr->type != T_VSTR)
    {
      fprintf(fp, "Wrong object type.\n");
      return ;
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

ovm_var_t *
ovm_ctime_new(void)
{
  ovm_ctime_t *time;

  time = Xmalloc(sizeof (ovm_ctime_t));
  time->type = T_CTIME;
  time->flags = 0;
  time->time = 0;

  return ( OVM_VAR(time) );
}

static void *
ctime_get_data(ovm_var_t *t)
{
  return ( &CTIME(t) );
}

static size_t
ctime_get_data_len(ovm_var_t *i)
{
  return (sizeof (time_t));
}

static int
ctime_cmp(ovm_var_t *var1, ovm_var_t *var2)
{
  if (TYPE(var2) != T_CTIME)
    return (TYPE(var1) - TYPE(var2));

  return (CTIME(var1) - CTIME(var2));
}


static ovm_var_t *
ctime_add(ovm_var_t *var1, ovm_var_t *var2)
{
  ovm_var_t *res;

  if (TYPE(var2) == T_CTIME) {
    res = ovm_ctime_new();
    CTIME(res) = CTIME(var1) + CTIME(var2);
  }
  else if (TYPE(var2) == T_INT) {
    res = ovm_ctime_new();
    CTIME(res) = CTIME(var1) + INT(var2);
  }
  else {
    return (NULL);
  }

  FLAGS(res) |= TYPE_CANFREE | TYPE_NOTBOUND;

  return (res);
}

static ovm_var_t *
ctime_sub(ovm_var_t *var1, ovm_var_t *var2)
{
  ovm_var_t *res;

  /* XXX: check for overflows */

  if (TYPE(var2) == T_CTIME) {
    res = ovm_ctime_new();
    CTIME(res) = CTIME(var1) - CTIME(var2);
  }
  else if (TYPE(var2) == T_INT) {
    res = ovm_ctime_new();
    CTIME(res) = CTIME(var1) - INT(var2);
  }
  else {
    return (NULL);
  }

  FLAGS(res) |= TYPE_CANFREE | TYPE_NOTBOUND;

  return (res);
}

static ovm_var_t *
ctime_mul(ovm_var_t *var1, ovm_var_t *var2)
{
  ovm_var_t *res;

  /* XXX: check for overflows */

  if (TYPE(var2) == T_CTIME) {
    res = ovm_ctime_new();
    CTIME(res) = CTIME(var1) * CTIME(var2);
  }
  else if (TYPE(var2) == T_INT) {
    res = ovm_ctime_new();
    CTIME(res) = CTIME(var1) * INT(var2);
  }
  else {
    return (NULL);
  }

  FLAGS(res) |= TYPE_CANFREE | TYPE_NOTBOUND;

  return (res);
}

static ovm_var_t *
ctime_div(ovm_var_t *var1, ovm_var_t *var2)
{
  ovm_var_t *res;

  /* XXX: check for overflows */

  if (TYPE(var2) == T_CTIME && CTIME(var2) != 0) {
    res = ovm_ctime_new();
    CTIME(res) = CTIME(var1) / CTIME(var2);
  }
  else if (TYPE(var2) == T_INT && INT(var2) != 0) {
    res = ovm_ctime_new();
    CTIME(res) = CTIME(var1) / INT(var2);
  }
  else {
    return (NULL);
  }

  FLAGS(res) |= TYPE_CANFREE | TYPE_NOTBOUND;

  return (res);
}

static ovm_var_t *
ctime_mod(ovm_var_t *var1, ovm_var_t *var2)
{
  ovm_var_t *res;

  /* XXX: check for overflows */

  if (TYPE(var2) == T_CTIME && CTIME(var2) != 0) {
    res = ovm_ctime_new();
    CTIME(res) = CTIME(var1) % CTIME(var2);
  }
  else if (TYPE(var2) == T_INT && INT(var2) != 0) {
    res = ovm_ctime_new();
    CTIME(res) = CTIME(var1) % INT(var2);
  }
  else {
    return (NULL);
  }

  FLAGS(res) |= TYPE_CANFREE | TYPE_NOTBOUND;

  return (res);
}

static ovm_var_t *
ctime_clone(ovm_var_t *var)
{
  ovm_var_t *res;

  if (var->type != T_CTIME)
    return (NULL);

  res = ovm_ctime_new();
  CTIME(res) = CTIME(var);
  FLAGS(res) |= TYPE_CANFREE | TYPE_NOTBOUND;

  return (res);
}


void
ovm_ctime_fprintf(FILE *fp, ovm_ctime_t *time)
{
  char asc_time[32];

  if (time->type != T_CTIME)
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

ovm_var_t *
ovm_ipv4_new(void)
{
  ovm_ipv4_t *addr;

  addr = Xzmalloc(sizeof (ovm_ipv4_t));
  addr->type = T_IPV4;

  return ( OVM_VAR(addr) );
}

static int
ipv4_cmp(ovm_var_t *var1, ovm_var_t *var2)
{
  if (TYPE(var2) != T_IPV4)
    return (TYPE(var1) - TYPE(var2));

  return (IPV4(var1).s_addr - IPV4(var2).s_addr);
}

static void *
ipv4_get_data(ovm_var_t *addr)
{
  return ( &IPV4(addr) );
}

static size_t
ipv4_get_data_len(ovm_var_t *addr)
{
  return ( sizeof (struct in_addr));
}

static ovm_var_t *
ipv4_clone(ovm_var_t *var)
{
  ovm_var_t *res;

  if (var->type != T_IPV4)
    return (NULL);

  res = ovm_ipv4_new();
  IPV4(res) = IPV4(var);
  FLAGS(res) |= TYPE_CANFREE | TYPE_NOTBOUND;

  return (res);
}

void
ovm_ipv4_fprintf(FILE *fp, ovm_ipv4_t *addr)
{
  struct hostent *hptr;
  char **pptr;

  if (addr->type != T_IPV4)
    {
      fprintf(fp, "Wrong object type.\n");
      return ;
    }

  fprintf(fp, "ipv4 : %s", inet_ntoa(addr->ipv4addr));

  /* address resolution */
  hptr = gethostbyaddr((char *) &addr->ipv4addr,
                       sizeof (struct in_addr), AF_INET);
  if (hptr == NULL)
    {
      fprintf(fp, "\n");
      return ;
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

ovm_var_t *
ovm_timeval_new(void)
{
  ovm_timeval_t *time;

  time = Xmalloc(sizeof (ovm_timeval_t));
  time->type = T_TIMEVAL;
  time->flags = 0;
  time->time.tv_sec = 0;
  time->time.tv_usec = 0;

  return ( OVM_VAR(time) );
}

static void *
timeval_get_data(ovm_var_t *str)
{
  return ( &TIMEVAL(str) );
}

static size_t
timeval_get_data_len(ovm_var_t *str)
{
  return ( sizeof (struct timeval) );
}

static ovm_var_t *
timeval_add(ovm_var_t *var1, ovm_var_t *var2)
{
  ovm_var_t *res;

  if ( TYPE(var2) == T_TIMEVAL ) {
    res = ovm_timeval_new();
    Timer_Add( &TIMEVAL(res) , &TIMEVAL(var1) , &TIMEVAL(var2) );
  }
  else if ( TYPE(var2) == T_CTIME ) {
    res = timeval_clone(var1);
    TIMEVAL(res).tv_sec += CTIME(var2);
  }
  else if ( TYPE(var2) == T_INT ) {
    res = timeval_clone(var1);
    TIMEVAL(res).tv_sec += INT(var2);
  }
  else {
    return (NULL);
  }

  FLAGS(res) |= TYPE_CANFREE | TYPE_NOTBOUND;

  return (res);
}

static ovm_var_t *
timeval_sub(ovm_var_t *var1, ovm_var_t *var2)
{
  ovm_var_t *res;

  if ( TYPE(var2) == T_TIMEVAL ) {
    res = ovm_timeval_new();
    Timer_Sub( &TIMEVAL(res) , &TIMEVAL(var1) , &TIMEVAL(var2) );
  }
  else if ( TYPE(var2) == T_CTIME ) {
    res = timeval_clone(var1);
    TIMEVAL(res).tv_sec -= CTIME(var2);
  }
  else if ( TYPE(var2) == T_INT ) {
    res = timeval_clone(var1);
    TIMEVAL(res).tv_sec -= INT(var2);
  }
  else {
    return (NULL);
  }

  FLAGS(res) |= TYPE_CANFREE | TYPE_NOTBOUND;

  return (res);
}

static ovm_var_t *
timeval_mul(ovm_var_t *var1, ovm_var_t *var2)
{
  ovm_var_t *res;

  if ( TYPE(var2) == T_CTIME ) {
    res = timeval_clone(var1);
    TIMEVAL(res).tv_sec *= CTIME(var2);
  }
  else if ( TYPE(var2) == T_INT ) {
    res = timeval_clone(var1);
    TIMEVAL(res).tv_sec *= INT(var2);
  }
  else {
    return (NULL);
  }

  FLAGS(res) |= TYPE_CANFREE | TYPE_NOTBOUND;

  return (res);
}

static ovm_var_t *
timeval_div(ovm_var_t *var1, ovm_var_t *var2)
{
  ovm_var_t *res;

  if ( TYPE(var2) == T_CTIME && CTIME(var2) != 0) {
    res = timeval_clone(var1);
    TIMEVAL(res).tv_sec /= CTIME(var2);
  }
  else if ( TYPE(var2) == T_INT && INT(var2) != 0) {
    res = timeval_clone(var1);
    TIMEVAL(res).tv_sec /= INT(var2);
  }
  else {
    return (NULL);
  }

  FLAGS(res) |= TYPE_CANFREE | TYPE_NOTBOUND;

  return (res);
}

static ovm_var_t *
timeval_mod(ovm_var_t *var1, ovm_var_t *var2)
{
  ovm_var_t *res;

  if ( TYPE(var2) == T_CTIME && CTIME(var2) != 0) {
    res = timeval_clone(var1);
    TIMEVAL(res).tv_sec %= CTIME(var2);
  }
  else if ( TYPE(var2) == T_INT && INT(var2) != 0) {
    res = timeval_clone(var1);
    TIMEVAL(res).tv_sec %= INT(var2);
  }
  else {
    return (NULL);
  }

  FLAGS(res) |= TYPE_CANFREE | TYPE_NOTBOUND;

  return (res);
}


static int
timeval_cmp(ovm_var_t *var1, ovm_var_t *var2)
{
  if ( TYPE(var2) != T_TIMEVAL )
    return (TYPE(var1) - TYPE(var2));

  if ( timercmp( &TIMEVAL(var1) , &TIMEVAL(var2) , < ) )
    return (-1);

  if ( timercmp( &TIMEVAL(var1) , &TIMEVAL(var2) , > ) )
    return (1);

  /* if ( timercmp( &TIMEVAL(var1) , &TIMEVAL(var2) , = ) ) */
  return (0);
}


static ovm_var_t *
timeval_clone(ovm_var_t *var)
{
  ovm_var_t *res;

  if (var->type != T_TIMEVAL)
    return (NULL);

  res = ovm_timeval_new();
  TIMEVAL(res) = TIMEVAL(var);
  FLAGS(res) |= TYPE_CANFREE | TYPE_NOTBOUND;

  return (res);
}


void
ovm_timeval_fprintf(FILE *fp, ovm_timeval_t *time)
{
  char asc_time[32];

  if (time->type != T_TIMEVAL)
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

ovm_var_t *
ovm_regex_new(void)
{
  ovm_regex_t *regex;

  regex = Xzmalloc(sizeof (ovm_regex_t));
  regex->type = T_REGEX;
  regex->flags = 0;

  return ( OVM_VAR(regex) );
}

void
regex_destruct(ovm_var_t *regex)
{
  regfree(&(REGEX(regex)));
  Xfree(REGEXSTR(regex));
}

static void *regex_get_data(ovm_var_t *regex)
{
  return (REGEXSTR(regex));
}

static size_t regex_get_data_len(ovm_var_t *regex)
{
  return (strlen(REGEXSTR(regex)));
}

void
ovm_regex_fprintf(FILE *fp, ovm_regex_t *regex)
{
  if (regex->type != T_REGEX)
    {
      fprintf(fp, "Wrong object type.\n");
      return ;
    }

  fprintf(fp, "regex[%zd] : \"%s\"\n",
          strlen(REGEXSTR(regex)), REGEXSTR(regex));
}

/*
** Split Regex
*/

ovm_var_t *
ovm_sregex_new(void)
{
  ovm_var_t *sregex;

  sregex = Xzmalloc(sizeof (ovm_sregex_t));
  sregex->type = T_SREGEX;
  sregex->flags = 0;

  return ( OVM_VAR(sregex) );
}

void
splitregex_destruct(ovm_var_t *regex)
{
  regfree(&(SREGEX(regex)));
  Xfree(SREGEXSTR(regex));
}

static void *splitregex_get_data(ovm_var_t *regex)
{
  return (SREGEXSTR(regex));
}

static size_t splitregex_get_data_len(ovm_var_t *regex)
{
  return (strlen(SREGEXSTR(regex)));
}


/*
** Address type
** store an 'void *' pointer.
*/

ovm_var_t *
ovm_ptr32_new(void)
{
  ovm_ptr32_t *addr;

  addr = Xmalloc(sizeof (ovm_ptr32_t));
  addr->type = T_PTR32;
  addr->flags = 0;
  addr->addr = 0;

  return ( OVM_VAR(addr) );
}

static void *
address_get_data(ovm_var_t *i)
{
  return ( &((ovm_ptr32_t *)i)->addr );
}

static size_t
address_get_data_len(ovm_var_t *i)
{
  return (sizeof (void *));
}

void
ovm_ptr32_fprintf(FILE *fp, ovm_ptr32_t *val)
{
  if (val->type != T_PTR32)
    {
      fprintf(fp, "Wrong object type.\n");
      return ;
    }

  fprintf(fp, "ptr32 : %p\n", val->addr);
}


/*
** Float type
** store a 'float' value.
*/

ovm_var_t *
ovm_float_new(void)
{
  ovm_float_t *n;

  n = Xmalloc(sizeof (ovm_float_t));
  n->type = T_FLOAT;
  n->flags = 0;
  n->val = 0;

  return ( OVM_VAR(n) );
}

static void *
float_get_data(ovm_var_t *i)
{
  return ( &((ovm_float_t *)i)->val );
}

static size_t
float_get_data_len(ovm_var_t *i)
{
  return (sizeof (float));
}

void
ovm_float_fprintf(FILE *fp, ovm_float_t *val)
{
  if (val->type != T_INT) {
    fprintf(fp, "Wrong object type.\n");
    return ;
  }

  fprintf(fp, "float : %f\n", val->val);
}

static int
float_cmp(ovm_var_t *var1, ovm_var_t *var2)
{
  float diff;

  if (TYPE(var2) != T_FLOAT)
    return (TYPE(var1) - TYPE(var2));

  diff = FLOAT(var1) - FLOAT(var2);

  if (diff > 0.0)
    return (1);

  if (diff < 0.0)
    return (-1);

  return (0);
}

static ovm_var_t *
float_add(ovm_var_t *var1, ovm_var_t *var2)
{
  ovm_var_t *res;

  if (var2->type != T_FLOAT)
    return (NULL);

  res = ovm_float_new();
  FLOAT(res) = FLOAT(var1) + FLOAT(var2);
  FLAGS(res) |= TYPE_CANFREE | TYPE_NOTBOUND;

  return (res);
}

static ovm_var_t *
float_sub(ovm_var_t *var1, ovm_var_t *var2)
{
  ovm_var_t *res;

  if (var2->type != T_FLOAT)
    return (NULL);

  res = ovm_float_new();
  FLOAT(res) = FLOAT(var1) - FLOAT(var2);
  FLAGS(res) |= TYPE_CANFREE | TYPE_NOTBOUND;

  return (res);
}

static ovm_var_t *
float_mul(ovm_var_t *var1, ovm_var_t *var2)
{
  ovm_var_t *res;

  if (var2->type != T_FLOAT)
    return (NULL);

  res = ovm_float_new();
  FLOAT(res) = FLOAT(var1) * FLOAT(var2);
  FLAGS(res) |= TYPE_CANFREE | TYPE_NOTBOUND;

  return (res);
}

static ovm_var_t *
float_div(ovm_var_t *var1, ovm_var_t *var2)
{
  ovm_var_t *res;

  if (var2->type != T_FLOAT)
    return (NULL);

  if (FLOAT(var2) == 0.0)
    return (NULL);

  res = ovm_float_new();
  FLOAT(res) = FLOAT(var1) / FLOAT(var2);
  FLAGS(res) |= TYPE_CANFREE | TYPE_NOTBOUND;

  return (res);
}

static ovm_var_t *
float_clone(ovm_var_t *var)
{
  ovm_var_t *res;

  if (var->type != T_FLOAT)
    return (NULL);

  res = ovm_float_new();
  FLOAT(res) = FLOAT(var);
  FLAGS(res) |= TYPE_CANFREE | TYPE_NOTBOUND;

  return (res);
}


/*
** snmp oid
*/

ovm_var_t *
ovm_snmpoid_new(size_t len)
{
  ovm_snmpoid_t *o;

  o = Xzmalloc(sizeof (ovm_snmpoid_t) - STR_PAD_LEN + (len * sizeof (oid_t)));
  o->type = T_SNMPOID;
  o->len = len;

  return ( OVM_VAR(o) );
}

/* raw data support */

void *
issdl_get_data(ovm_var_t *val)
{
  if ( issdl_types_g[ val->type ].get_data )
    return ( issdl_types_g[ val->type ].get_data(val) );

  DebugLog(DF_OVM, DS_WARN,
	   "issdl_get_data(): get_data doesn't apply for type '%s'\n",
	   STRTYPE(val));

  return (NULL);
}

size_t
issdl_get_data_len(ovm_var_t *val)
{
  if (issdl_types_g[val->type].get_data_len)
    return (issdl_types_g[val->type].get_data_len(val));

  DebugLog(DF_OVM, DS_WARN,
	   "issdl_get_data_len(): get_data_len doesn't apply for type '%s'\n",
	   STRTYPE(val));

  return (0);
}

/*
** External data type
** store an 'void *' pointer.
*/

ovm_var_t *
ovm_extern_new(void)
{
  ovm_extern_t *addr;

  addr = Xmalloc(sizeof (ovm_extern_t));
  addr->type = T_EXTERNAL;
  addr->flags = 0;
  addr->ptr = 0;
  addr->desc = 0;
  addr->free = 0;

  return ( OVM_VAR(addr) );
}

void
extern_destruct(ovm_var_t *i)
{
  if (EXTFREE(i) != NULL)
    EXTFREE(i)(EXTPTR(i));
}

static void *
extern_get_data(ovm_var_t *i)
{
  return ( &((ovm_extern_t *)i)->ptr );
}

static size_t
extern_get_data_len(ovm_var_t *i)
{
  return (sizeof (void *));
}


/* type specific actions */


int
issdl_test(ovm_var_t *var)
{
  if (!var)
    return 0;
  switch (var->type)
  {
    case T_NULL :
      return 0;
    case T_INT :
      return (INT(var));
    case T_UINT :
      return (UINT(var));
    default :
      return (1);
  }
}



int
issdl_cmp(ovm_var_t *var1, ovm_var_t *var2)
{
  if (issdl_types_g[var1->type].cmp)
    return (issdl_types_g[var1->type].cmp(var1, var2));

  DebugLog(DF_OVM, DS_WARN,
	   "issdl_cmp(): comparison doesn't apply for type '%s'\n",
	   STRTYPE(var1));

  return (-1);
}


ovm_var_t *
issdl_add(ovm_var_t *var1, ovm_var_t *var2)
{
  if (issdl_types_g[ var1->type ].add)
    return ( issdl_types_g[ var1->type ].add(var1, var2) );

  DebugLog(DF_OVM, DS_WARN,
	   "issdl_add(): addition doesn't apply for type '%s'\n",
	   STRTYPE(var1));

  return (NULL);
}

ovm_var_t *
issdl_sub(ovm_var_t *var1, ovm_var_t *var2)
{
  if (issdl_types_g[ var1->type ].sub)
    return ( issdl_types_g[ var1->type ].sub(var1, var2) );

  DebugLog(DF_OVM, DS_WARN,
	   "issdl_sub(): substraction doesn't apply for type '%s'\n",
	   STRTYPE(var1));

  return (NULL);
}

ovm_var_t *
issdl_mul(ovm_var_t *var1, ovm_var_t *var2)
{
  if (issdl_types_g[ var1->type ].mul)
    return ( issdl_types_g[ var1->type ].mul(var1, var2) );

  DebugLog(DF_OVM, DS_WARN,
	   "issdl_mul(): multiplication doesn't apply for type '%s'\n",
	   STRTYPE(var1));

  return (NULL);
}

ovm_var_t *
issdl_div(ovm_var_t *var1, ovm_var_t *var2)
{
  if (issdl_types_g[ var1->type ].div)
    return ( issdl_types_g[ var1->type ].div(var1, var2) );

  DebugLog(DF_OVM, DS_WARN,
	   "issdl_div(): division doesn't apply for type '%s'\n",
	   STRTYPE(var1));

  return (NULL);
}

ovm_var_t *
issdl_mod(ovm_var_t *var1, ovm_var_t *var2)
{
  if (issdl_types_g[ var1->type ].mod)
    return ( issdl_types_g[ var1->type ].mod(var1, var2) );

  DebugLog(DF_OVM, DS_WARN,
	   "issdl_mod(): modulo doesn't apply for type '%s'\n",
	   STRTYPE(var1));

  return (NULL);
}

ovm_var_t *
issdl_clone(ovm_var_t *var)
{
  if (issdl_types_g[ var->type ].clone)
    return ( issdl_types_g[ var->type ].clone(var) );

  DebugLog(DF_OVM, DS_WARN,
	   "issdl_clone(): cloning doesn't apply for type '%s'\n",
	   STRTYPE(var));

  return (NULL);
}


/*
** Counter type
** store an 'signed int' value.
*/

ovm_var_t *
ovm_counter_new(void)
{
  ovm_counter_t *c;

  c = Xmalloc(sizeof (ovm_counter_t));
  c->type = T_COUNTER;
  c->flags = TYPE_MONO;
  c->val = 0;

  return ( OVM_VAR(c) );
}

static void *
counter_get_data(ovm_var_t *c)
{
  return ( &((ovm_counter_t *)c)->val );
}

static size_t
counter_get_data_len(ovm_var_t *c)
{
  return (sizeof (unsigned long));
}

static int
counter_cmp(ovm_var_t *var1, ovm_var_t *var2)
{
  if (TYPE(var2) != T_COUNTER)
    return (TYPE(var1) - TYPE(var2));

  return (COUNTER(var1) - COUNTER(var2));
}

static ovm_var_t *
counter_add(ovm_var_t *var1, ovm_var_t *var2)
{
  ovm_var_t *res;

  if (var2->type == T_INT) {
    res = ovm_counter_new();
    COUNTER(res) = COUNTER(var1) + INT(var2);
  }
  else if (TYPE(var2) == T_COUNTER) {
    res = ovm_counter_new();
    COUNTER(res) = COUNTER(var1) + COUNTER(var2);
  }
  else {
    return (NULL);
  }

  FLAGS(res) |= TYPE_CANFREE | TYPE_NOTBOUND;

  return (res);
}

static ovm_var_t *
counter_mul(ovm_var_t *var1, ovm_var_t *var2)
{
  ovm_var_t *res;

  if (TYPE(var2) == T_INT && INT(var2) > 0) {
    res = ovm_counter_new();
    COUNTER(res) = COUNTER(var1) * INT(var2);
  }
  else if (TYPE(var2) == T_COUNTER && COUNTER(var2) > 0) {
    res = ovm_counter_new();
    COUNTER(res) = COUNTER(var1) * COUNTER(var2);
  }
  else {
    return (NULL);
  }

  FLAGS(res) |= TYPE_CANFREE | TYPE_NOTBOUND;

  return (res);
}

static ovm_var_t *
counter_clone(ovm_var_t *var)
{
  ovm_var_t *res;

  if (var->type != T_COUNTER)
    return (NULL);

  res = ovm_counter_new();
  COUNTER(res) = COUNTER(var);
  FLAGS(res) |= TYPE_CANFREE | TYPE_NOTBOUND;

  return (res);
}


void
ovm_counter_fprintf(FILE *fp, ovm_counter_t *val)
{
  if (val->type != T_COUNTER)
    {
      fprintf(fp, "Wrong object type.\n");
      return ;
    }

  fprintf(fp, "counter : %lu\n", val->val);
}


/* Display function */


int
snprintf_ovm_var(char *buff, unsigned int buff_length, ovm_var_t *val)
{
  int i; /* for STRINGs */
  char asc_time[32]; /* for dates conversions */
  struct hostent *hptr; /* for IPV4ADDR */
  char **pptr; /* for IPV4ADDR */
  int offset = 0; /* for chars */

  /* display data */
  switch (val->type) {

  case T_NULL:
    return snprintf(buff, buff_length, "(null %i)\n", ERRNO(val));

  case T_INT:
    return snprintf(buff, buff_length, "%li", INT(val));

  case T_BSTR:
    for (i = 0; i < BSTRLEN(val); i++) {
      if (isprint(BSTR(val)[i]))
        offset += snprintf(buff + offset, buff_length - offset, "%c", BSTR(val)[i]);
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
       offset += snprintf(buff + offset, buff_length - offset, "%c", VSTR(val)[i]);
    return offset;

  case T_CTIME:
    strftime(asc_time, 32, "%a %b %d %H:%M:%S %Y", localtime(&CTIME(val)));
    return snprintf(buff, buff_length, "%s (%li)", asc_time, CTIME(val));

  case T_IPV4:
    offset += snprintf(buff, buff_length, "%s", inet_ntoa(IPV4(val)));
    hptr = gethostbyaddr((char *) &IPV4(val),
                         sizeof (struct in_addr), AF_INET);
    if (hptr == NULL) {
      return offset;
    } else if (hptr->h_name != NULL) {
      offset += snprintf(buff + offset, buff_length - offset, " (%s", hptr->h_name);
    } else {
      return offset;
    }
    for (pptr = hptr->h_aliases; *pptr != NULL; pptr++)
      offset += snprintf(buff + offset, buff_length - offset, ", %s", *pptr);
    offset += snprintf(buff + offset, buff_length - offset, ")");
    return offset;

  case T_TIMEVAL:
    strftime(asc_time, 32, "%a %b %d %H:%M:%S %Y",
             localtime(&TIMEVAL(val).tv_sec));
    return snprintf(buff, buff_length, "%s +%li us (%li.%06li)",
		    asc_time, (long)TIMEVAL(val).tv_usec,
		    TIMEVAL(val).tv_sec, (long)TIMEVAL(val).tv_usec);

  case T_COUNTER:
    return snprintf(buff, buff_length, "%lu", COUNTER(val));

  case T_REGEX:
    return snprintf(buff, buff_length, "%s", REGEXSTR(val));

  case T_SREGEX:
    return snprintf(buff, buff_length, "%s", SREGEXSTR(val));

  case T_PTR32:
    return snprintf(buff, buff_length, "%p", PTR32(val));

  case T_FLOAT:
    return snprintf(buff, buff_length, "%f", FLOAT(val));

  default:
    return snprintf(buff, buff_length, "type %i doesn't support display\n", val->type);
  }
}



void
fprintf_ovm_var(FILE *fp, ovm_var_t *val)
{
  int i; /* for STRINGs */
  char asc_time[32]; /* for dates conversions */
  struct hostent *hptr; /* for IPV4ADDR */
  char **pptr; /* for IPV4ADDR */

  /* display data */
  switch (val->type) {

  case T_NULL:
    fprintf(fp, "(null %i)\n", ERRNO(val));
    break;

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
    strftime(asc_time, 32, "%a %b %d %H:%M:%S %Y", localtime(&CTIME(val)));
    fprintf(fp, "%s (%li)", asc_time, CTIME(val));
    break;

  case T_IPV4:
    fprintf(fp, "%s", inet_ntoa(IPV4(val)));
    hptr = gethostbyaddr((char *) &IPV4(val),
                         sizeof (struct in_addr), AF_INET);
    if (hptr == NULL) {
      return ;
    } else if (hptr->h_name != NULL) {
      fprintf(fp, " (%s", hptr->h_name);
    } else {
      return ;
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

  case T_COUNTER:
    fprintf(fp, "%lu", COUNTER(val));
    break;

  case T_REGEX:
    fprintf(fp, "%s", REGEXSTR(val));
    break;

  case T_SREGEX:
    fprintf(fp, "%s", SREGEXSTR(val));
    break;

  case T_PTR32:
    fprintf(fp, "%p", PTR32(val));
    break;

  case T_FLOAT:
    fprintf(fp, "%f", FLOAT(val));
    break;

  default:
    fprintf(fp, "type %i doesn't support display\n", val->type);
  }
}


void
fprintf_issdl_val(FILE *fp, ovm_var_t *val)
{
  int i; /* for STRINGs */
  char asc_time[32]; /* for dates conversions */
  struct hostent *hptr; /* for IPV4ADDR */
  char **pptr; /* for IPV4ADDR */

  /* Display variable info: affected to a named variable, or temp result */
  if (FLAGS(val) & TYPE_CANFREE)
    fputc('F', fp);
  else
    fputc('K', fp);

  /* display nomotony info */
  switch (val->flags & MONOTONY_MASK) {

  case TYPE_UNKNOWN:
    fputc('U', fp);
    break;

  case TYPE_MONO:
    fputc('M', fp);
    break;

  case TYPE_ANTI:
    fputc('A', fp);
    break;

  case TYPE_CONST:
    fputc('C', fp);
    break;
  }
  fputc(' ', fp);

  /* Display data */
  switch (val->type) {

    case T_NULL:
      fprintf(fp, "null (%i)\n", ERRNO(val));
      break;

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
      strftime(asc_time, 32, "%a %b %d %H:%M:%S %Y", localtime(&CTIME(val)));
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

    case T_TIMEVAL:
      strftime(asc_time, 32, "%a %b %d %H:%M:%S %Y",
               localtime(&TIMEVAL(val).tv_sec));
      fprintf(fp, "timeval: (%li.%06li) = %s (+%li us)\n",
              TIMEVAL(val).tv_sec, (long)TIMEVAL(val).tv_usec,
              asc_time, (long)TIMEVAL(val).tv_usec);
      break;

    case T_COUNTER:
      fprintf(fp, "counter: %lu\n", COUNTER(val));
      break;

    case T_REGEX:
      fprintf(fp, "regex[%zd]: \"%s\"\n",
              strlen(REGEXSTR(val)), REGEXSTR(val));
      break;

    case T_SREGEX:
      fprintf(fp, "sregex[%zd]: \"%s\"\n",
              strlen(SREGEXSTR(val)), SREGEXSTR(val));
      break;

    case T_PTR32:
      fprintf(fp, "ptr32: %p\n", PTR32(val));
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

    default:
      fprintf(fp, "type %i doesn't support display\n", val->type);
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
  param = stack_pop(ctx->ovm_stack);
  if (param)
    fprintf_issdl_val(stdout, param);
  PUSH_RETURN_TRUE(ctx, state)
}

static void
issdl_dumpstack(orchids_t *ctx, state_instance_t *state)
{
  DebugLog(DF_OVM, DS_DEBUG, "issdl_dumpstack()\n");

  if (state->rule_instance == NULL)
    {
      PUSH_RETURN_FALSE(ctx,state);
      return ;
    }

  // push before state == NULL
  PUSH_RETURN_TRUE(ctx, state)

  fprintf(stdout, ">>>> rule: %s <<<<<\n", state->rule_instance->rule->name);
  while (state)
    {
      fprintf(stdout, "***** state: %s *****\n", state->state->name);
      fprintf_state_env(stdout, state);
      if (state->event)
        fprintf_event(stdout, ctx, state->event->event);
      else
        fprintf(stdout, "no event.\n");
      state = state->parent;
    }

}

static void
issdl_printevent(orchids_t *ctx, state_instance_t *state)
{
  DebugLog(DF_OVM, DS_DEBUG, "issdl_printevent()\n");
  PUSH_RETURN_TRUE(ctx, state)
  for ( ; state && state->event == NULL; state = state->parent)
    ;
  if (state && state->event)
    fprintf_event(stdout, ctx, state->event->event);
  else
    fprintf(stdout, "no event to display.\n");
}

static void
issdl_shutdown(orchids_t *ctx, state_instance_t *state)
{
  DebugLog(DF_OVM, DS_DEBUG, "issdl_shutdown()\n");
  fprintf(stdout, "explicit shutdown on '%s:%s' request\n",
          state->rule_instance->rule->name, state->state->name);
  exit(EXIT_SUCCESS);
}

static void
issdl_dumppathtree(orchids_t *ctx, state_instance_t *state)
{
  DebugLog(DF_OVM, DS_DEBUG, "issdl_dumppathtree()\n");
  fprintf_rule_instance_dot(stdout, state->rule_instance,
                            DOT_RETRIGLIST, ctx->new_qh, 100);
  /* XXX: hard-coded limit */
  PUSH_RETURN_TRUE(ctx, state)
}

static void
issdl_random(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *random;

  DebugLog(DF_OVM, DS_DEBUG, "issdl_random()\n");

  random = ovm_int_new();
  INT(random) = rand();
  stack_push(ctx->ovm_stack, random);
}

static void
issdl_system(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t  *param;
  char       *cmd_line;

  param = stack_pop(ctx->ovm_stack);
  if (param && param->type == T_STR) {
    /* copy value, and append '\0' XXX should be replaced by ovm_strdup() */
    cmd_line = Xmalloc(STRLEN(param) + 1);
    memcpy(cmd_line, STR(param), STRLEN(param));
    cmd_line[ STRLEN(param) ] = '\0';
    DebugLog(DF_OVM, DS_DEBUG, "issdl_system( \"%s\" )\n", cmd_line);
    system(cmd_line);
    Xfree(cmd_line);
  }
  else {
    DebugLog(DF_OVM, DS_DEBUG, "issdl_system(): param error\n");
  }
  PUSH_RETURN_TRUE(ctx, state)
}

static void
issdl_stats(orchids_t *ctx, state_instance_t *state)
{
  fprintf_orchids_stats(stdout, ctx);
  PUSH_RETURN_TRUE(ctx, state)
}

static void
issdl_str_from_int(orchids_t *ctx, state_instance_t *state)
{
  char buff[32];
  ovm_var_t *str;
  ovm_var_t *i;

  i = stack_pop(ctx->ovm_stack);
  if (i->type == T_INT) {
    snprintf(buff, sizeof(buff), "%li", INT(i));
    str = ovm_str_new( strlen(buff) );
    FLAGS(str) |= TYPE_CANFREE | TYPE_NOTBOUND;
    memcpy(STR(str), buff, strlen(buff));
    stack_push(ctx->ovm_stack, str);
  }
  else {
    DebugLog(DF_OVM, DS_DEBUG, "issdl_str_from_int(): param error\n");
    str = ovm_str_new(0);
    FLAGS(str) |= TYPE_CANFREE | TYPE_NOTBOUND;
    stack_push(ctx->ovm_stack, str);
  }
  if ( IS_NOT_BOUND(i) ) {
    DebugLog(DF_OVM, DS_DEBUG, "issdl_str_from_int(): free temp var\n");
    Xfree(i);
  }
}

static void
issdl_str_from_ipv4(orchids_t *ctx, state_instance_t *state)
{
  char buff[17];
  ovm_var_t *str;
  ovm_var_t *i;

  i = stack_pop(ctx->ovm_stack);
  if (i->type == T_IPV4) {
    snprintf(buff, sizeof(buff), "%s", inet_ntoa(IPV4(i)));
    str = ovm_str_new( strlen(buff) );
    FLAGS(str) |= TYPE_CANFREE | TYPE_NOTBOUND;
    memcpy(STR(str), buff, strlen(buff));
    stack_push(ctx->ovm_stack, str);
    if ( IS_NOT_BOUND(i) ) {
      DebugLog(DF_OVM, DS_DEBUG, "issdl_str_from_int(): free temp var\n");
      Xfree(i);
    }
  }
  else {
    DebugLog(DF_OVM, DS_DEBUG, "issdl_str_from_int(): param error\n");
    PUSH_RETURN_FALSE(ctx, state)
  }
}


static void
issdl_int_from_str(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *str;
  ovm_var_t *i;

  str = stack_pop(ctx->ovm_stack);
  if (str->type == T_STR) {
    i = ovm_int_new();
    INT(i) = atoi( STR(str) );
    stack_push(ctx->ovm_stack, i);
    if ( IS_NOT_BOUND(str) ) {
      DebugLog(DF_OVM, DS_DEBUG, "issdl_int_from_str(): free temp var\n");
      Xfree(str);
    }
  } else {
    DebugLog(DF_OVM, DS_DEBUG, "issdl_str_from_int(): param error\n");
    PUSH_RETURN_FALSE(ctx, state)
  }
}

static void
issdl_kill_threads(orchids_t *ctx, state_instance_t *state)
{
  wait_thread_t *t;

  for (t = ctx->cur_retrig_qh; t; t = t->next) {
    if (t->state_instance->rule_instance == state->rule_instance) {
      DebugLog(DF_ENG, DS_TRACE, "Marking thread %p as KILLED\n", t);
      t->flags |= THREAD_KILLED;
    }
  }
  PUSH_RETURN_TRUE(ctx, state)
}

static void
do_recursive_cut(state_instance_t *state)
{
  wait_thread_t *t;

  state->flags |= SF_PRUNED;

  /* Cut current state instance threads */
  for (t = state->thread_list; t; t = t->next_in_state_instance) {
    if ( !(t->flags & THREAD_ONLYONCE) ) {
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

static void
issdl_cut(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *str;
  state_instance_t *si;

  /* First poor implementation of cuts:
   * The cut should be included in the language, and should also be resolved
   * at compilation-time to apply syntactic restrictions and avoid problems
   * (ONE CUT per state MAX! And ensure at the compilation time that the
   * destination exists and is reachable) */

  str = stack_pop(ctx->ovm_stack);

  if (TYPE(str) != T_STR) {
    DebugLog(DF_ENG, DS_ERROR, "parameter type error\n");
    return ;
  }

  /* Find the destination of the cut */
  for (si = state;
       si->parent && strcmp(STR(str), si->state->name);
       si = si->parent)
    ;

  DebugLog(DF_ENG, DS_INFO, "found cut dest @ %s:%p\n", si->state->name, si);

  do_recursive_cut(si);

  PUSH_RETURN_TRUE(ctx, state)
}


char *
ovm_strdup(ovm_var_t *str)
{
  char *s;

  s = Xmalloc(STRLEN(str) + 1);
  if (TYPE(str) == T_STR) {
    strncpy(s, STR(str), STRLEN(str));
    s[ STRLEN(str) ] = '\0';
  }
  else if (TYPE(str) == T_VSTR) {
    strncpy(s, VSTR(str), VSTRLEN(str));
    s[ VSTRLEN(str) ] = '\0';
  }
  else {
    DebugLog(DF_ENG, DS_ERROR, "ovm_strdup type error\n");
    return (NULL);
  }

  return (s);
}




static void
issdl_report(orchids_t *ctx, state_instance_t *state)
{
  reportmod_t* r;

  DebugLog(DF_ENG, DS_INFO, "Generating report\n");

  if (state->rule_instance == NULL)
  {
    PUSH_RETURN_FALSE(ctx,state);
    return;
  }

  SLIST_FOREACH(r, &ctx->reportmod_list, list) {
    r->cb(ctx, r->mod, r->data, state);
  }
  ctx->reports++;
  PUSH_RETURN_TRUE(ctx, state)
}


static void
issdl_noop(orchids_t *ctx, state_instance_t *state)
{
  DebugLog(DF_ENG, DS_INFO, "No-Operation called\n");
}

/*
 * implement as a language keywork, and add an opcode
 * PASTVAL [id] [n] ... => ... , value
 * push the n-th past value on the stack
 */
static void
issdl_pastval(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *varname;
  ovm_var_t *n;
  state_instance_t *si;
  char **var_array;
  int var_array_sz;
  int i;

  DebugLog(DF_ENG, DS_INFO, "Past value\n");

  si = state;

  varname = stack_pop(ctx->ovm_stack);
  n = stack_pop(ctx->ovm_stack);

  /* Lookup var id */
  var_array = state->rule_instance->rule->var_name;
  var_array_sz = state->rule_instance->rule->dynamic_env_sz;
  for (i = 0;
       i < var_array_sz &&
         strncmp( STR(varname) , var_array[i] , STRLEN(varname) );
       i++)
    ;
  if (i == var_array_sz) {
    DebugLog(DF_ENG, DS_ERROR, "variable not found (%s)\n", STR(varname));
    return ;
  }

  if (si->parent == NULL) {
    DebugLog(DF_ENG, DS_ERROR, "no parent state -> no past value\n");
    return ;
  }

  if (si->current_env[i]) {
    n--;
  }

  for (si = state->parent;
       si && si->current_env[i] && n;
       si = si->parent, n--)
    ;

  if (si == NULL) {
    DebugLog(DF_ENG, DS_ERROR, "past value not found (%s)\n", STR(varname));
    return ;
  }
  else {
    DebugLog(DF_ENG, DS_ERROR, "past value FOUND (%s)\n", STR(varname));
  }

/* XXX: Clone the value, set the correct flags (if required) and push */
/*   stack_push(ctx->ovm_stack, si->current_env[i]); */
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
  char *to_str;
  int i;

  from = stack_pop(ctx->ovm_stack);
  to = stack_pop(ctx->ovm_stack);
  subject = stack_pop(ctx->ovm_stack);
  body = stack_pop(ctx->ovm_stack);

  DebugLog(DF_ENG, DS_INFO,
           "Sendmail from:%s to:%s subject:%s\n",
           STR(from), STR(to), STR(subject));

  pid = fork();
  if (pid == 0) {
    strcpy(tmpfile, ORCHIDS_SENDMAIL_TEMPFILE );
    tmp_fd = mkstemp(tmpfile);
    ftmp = Xfdopen(tmp_fd, "w+");
    Xunlink(tmpfile);

    to_str = ovm_strdup(to);
    fputs("To: ", ftmp);
    fputs(to_str, ftmp);
    fputs("\n", ftmp);
    fputs("From: ", ftmp);
    for (i = 0; i < STRLEN(from); i++)
      fputc(STR(from)[i], ftmp);
    fputs("\n", ftmp);
    fputs("Subject: ", ftmp);
    for (i = 0; i < STRLEN(subject); i++)
      fputc(STR(subject)[i], ftmp);
    fputs("\n\n", ftmp);
    for (i = 0; i < STRLEN(body); i++)
      fputc(STR(body)[i], ftmp);

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
  }

  /* parent returns */
  PUSH_RETURN_TRUE(ctx, state)
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
  char *to_str;
  int i;
  state_instance_t *report_events;

  from = stack_pop(ctx->ovm_stack);
  to = stack_pop(ctx->ovm_stack);
  subject = stack_pop(ctx->ovm_stack);
  body = stack_pop(ctx->ovm_stack);

  DebugLog(DF_ENG, DS_INFO,
           "Sendmail from:%s to:%s subject:%s\n",
           STR(from), STR(to), STR(subject));

    strcpy(tmpfile, ORCHIDS_SENDMAIL_TEMPFILE );
    tmp_fd = mkstemp(tmpfile);
    ftmp = Xfdopen(tmp_fd, "w+");
    Xunlink(tmpfile);

    to_str = ovm_strdup(to);
    fputs("To: ", ftmp);
    fputs(to_str, ftmp);
    fputs("\n", ftmp);
    fputs("From: ", ftmp);
    for (i = 0; i < STRLEN(from); i++)
      fputc(STR(from)[i], ftmp);
    fputs("\n", ftmp);
    fputs("Subject: ", ftmp);
    for (i = 0; i < STRLEN(subject); i++)
      fputc(STR(subject)[i], ftmp);
    fputs("\n\n", ftmp);
    for (i = 0; i < STRLEN(body); i++)
      fputc(STR(body)[i], ftmp);


    /* report */
    for (report_events = NULL; state; state = state->parent) {
      state->next_report_elmt = report_events;
      report_events = state;
    }

    fprintf(ftmp, "Report for rule: %s\n\n",
            report_events->rule_instance->rule->name);

    for ( ; report_events ; report_events = report_events->next_report_elmt) {
      fprintf(ftmp, "State: %s\n", report_events->state->name);
      if (report_events->event)
        fprintf_event(ftmp, ctx, report_events->event->event);
      else
        fprintf(ftmp, "no event.\n");
      fprintf_state_env(ftmp, report_events);
    }

    rewind(ftmp);

  pid = fork();
  if (pid == 0) {

    Xdup2(fileno(ftmp), fileno(stdin));
    execl(PATH_TO_SENDMAIL, "sendmail", "-odi", to_str, NULL);

    DebugLog(DF_ENG, DS_ERROR, "execl(): error %i: %s\n",
             errno, strerror(errno));

    exit(EXIT_FAILURE);
  }
  else if (pid < 0) {
    DebugLog(DF_ENG, DS_ERROR, "fork(): error %i: %s\n",
             errno, strerror(errno));
  }

  /* parent returns */
  PUSH_RETURN_TRUE(ctx, state)
}

static void
issdl_drop_event(orchids_t *ctx, state_instance_t *state)
{
  if (state->event == NULL) {
    DebugLog(DF_ENG, DS_ERROR, "error: state instance does not have event reference\n");
    PUSH_RETURN_FALSE(ctx, state);
    return ;
  }

  state->event_level = -1;
  PUSH_RETURN_TRUE(ctx, state)
}

static void
issdl_set_event_level(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *level;

  level = stack_pop(ctx->ovm_stack);

  if (state->event == NULL) {
    DebugLog(DF_ENG, DS_ERROR, "state instance does not have event reference\n");
    PUSH_RETURN_FALSE(ctx,state);
    return ;
  }

  if (TYPE(level) != T_INT) {
    DebugLog(DF_ENG, DS_ERROR, "parameter type error\n");
    PUSH_RETURN_FALSE(ctx,state);
    return ;
  }

  state->event_level = INT(level);
  PUSH_RETURN_TRUE(ctx, state)
}


/* Compute the number of different bit between two variables of
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

  param1 = stack_pop(ctx->ovm_stack);
  param2 = stack_pop(ctx->ovm_stack);
  len = issdl_get_data_len(param1);
  len2 = issdl_get_data_len(param2);

  if (TYPE(param1) != TYPE(param2)) {
    DebugLog(DF_OVM, DS_DEBUG, "Type error\n");
  }
  else if (len != len2) {
    DebugLog(DF_OVM, DS_DEBUG, "Size error\n");
  }
  else {
    dist = 0;
    data1 = issdl_get_data(param1);
    data2 = issdl_get_data(param2);
    for ( ; len > 0; len--) {
      int x = *data1++ ^ *data2++;
      dist += bitcnt_tbl[x];
    }

    d = ovm_int_new();
    INT(d) = dist;
    FLAGS(d) |= TYPE_CANFREE | TYPE_NOTBOUND;
    stack_push(ctx->ovm_stack, d);
    DebugLog(DF_OVM, DS_DEBUG, "Computed bit distance %i\n", dist);
  }

  FREE_IF_NEEDED(param1);
  FREE_IF_NEEDED(param2);
}

static void
issdl_bytedist(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *param1;
  unsigned char *data1;
  size_t len;
  ovm_var_t *param2;
  unsigned char *data2;
  size_t len2;
  int dist;
  ovm_var_t *d;

  param1 = stack_pop(ctx->ovm_stack);
  param2 = stack_pop(ctx->ovm_stack);
  len = issdl_get_data_len(param1);
  len2 = issdl_get_data_len(param2);

  if (TYPE(param1) != TYPE(param2)) {
    DebugLog(DF_OVM, DS_DEBUG, "Type error\n");
  }
  else if (len != len2) {
    DebugLog(DF_OVM, DS_DEBUG, "Size error\n");
  }
  else {
    dist = 0;
    data1 = issdl_get_data(param1);
    data2 = issdl_get_data(param2);
    for ( ; len > 0; len--) {
      if (*data1++ != *data2++)
	dist++;
    }

    d = ovm_int_new();
    INT(d) = dist;
    FLAGS(d) |= TYPE_CANFREE | TYPE_NOTBOUND;
    stack_push(ctx->ovm_stack, d);
    DebugLog(DF_OVM, DS_DEBUG, "Computed byte distance %i\n", dist);
  }

  FREE_IF_NEEDED(param1);
  FREE_IF_NEEDED(param2);
}

static void
issdl_vstr_from_regex(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *vstr;
  ovm_var_t *regex;

  regex = stack_pop(ctx->ovm_stack);
  if (TYPE(regex) == T_REGEX) {
    vstr = ovm_vstr_new();
    VSTR(vstr) = REGEXSTR(regex);
    VSTRLEN(vstr) = strlen(REGEXSTR(regex));
    stack_push(ctx->ovm_stack, vstr);
    FREE_IF_NEEDED(regex);
  } else {
    DebugLog(DF_OVM, DS_ERROR, "issdl_vstr_from_regex(): type error\n");
  }
}

static void
issdl_str_from_regex(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *str;
  ovm_var_t *regex;

  regex = stack_pop(ctx->ovm_stack);
  if (TYPE(regex) == T_REGEX) {
    size_t s;
    s = strlen(REGEXSTR(regex));
    str = ovm_str_new(s);
    memcpy(STR(str), REGEXSTR(regex), s);
    STRLEN(str) = s;
    stack_push(ctx->ovm_stack, str);
    FREE_IF_NEEDED(regex);
  } else {
    DebugLog(DF_OVM, DS_ERROR, "issdl_str_from_regex(): param error\n");
  }
}

static void
issdl_regex_from_str(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *str;
  static char *regexstr = "(string not available: was compiled dynamically)";

  str = stack_pop(ctx->ovm_stack);
  if (TYPE(str) == T_STR || TYPE(str) == T_VSTR) {
    int ret;
    char *s;
    ovm_var_t *regex;

    s = ovm_strdup(str);
    regex = ovm_regex_new();
    ret = regcomp(&(REGEX(regex)), s, REG_EXTENDED | REG_NOSUB);
    if (ret) {
      char err_buf[64];
      regerror(ret, &(REGEX(regex)), err_buf, sizeof (err_buf));
      Xfree(regex);
      DebugLog(DF_OVM, DS_ERROR,
	       "issdl_regex_from_str(): "
	       "compilation error in \"%s\": %s\n", s, err_buf);
    }
    else {
      /* XXX: BUG regexstring should be \0 ended ! */
      //REGEXSTR(regex) = (TYPE(str) == T_STR) ? STR(str) : VSTR(str);
      REGEXSTR(regex) = regexstr;
      stack_push(ctx->ovm_stack, regex);
    }
  }
  else {
    DebugLog(DF_OVM, DS_ERROR, "issdl_str_from_regex(): param error\n");
  }
}

static void
issdl_defined(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *field;

  field = stack_pop(ctx->ovm_stack);

  if (IS_NULL(field))
    ISSDL_RETURN_FALSE(ctx, state);
  else
    ISSDL_RETURN_TRUE(ctx, state);

}


static void
issdl_difftime(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *t1, *t2, *res;

  t1 = stack_pop(ctx->ovm_stack);
  t2 = stack_pop(ctx->ovm_stack);

  if ((TYPE(t1) & TYPE(t2)) == T_CTIME)
  {
    res = ovm_int_new();
    INT(res) = difftime(CTIME(t1), CTIME(t2));
    stack_push(ctx->ovm_stack, res);
  }
  else
    ISSDL_RETURN_FALSE(ctx, state);
}

static void
issdl_str_from_time(orchids_t *ctx, state_instance_t *state)
{
  char buff[128];
  ovm_var_t *str;
  ovm_var_t *t;
  struct tm time;
  int len;

  t = stack_pop(ctx->ovm_stack);
  if (t->type == T_CTIME) {
    time = *localtime (&(CTIME(t)));
    len = strftime(buff, 128 * sizeof (char), "%Y-%m-%dT%H:%M:%S%z", &time);
    str = ovm_str_new(len);
    FLAGS(str) |= TYPE_CANFREE | TYPE_NOTBOUND;
    memcpy(STR(str), buff, len);
    stack_push(ctx->ovm_stack, str);
    if ( IS_NOT_BOUND(t) ) {
      DebugLog(DF_OVM, DS_DEBUG, "issdl_str_from_time(): free temp var\n");
      Xfree(t);
    }
  }
  else {
    DebugLog(DF_OVM, DS_DEBUG, "issdl_str_from_time(): param error\n");
  }
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
  { issdl_printevent, 3, "print_event", 0, "print the event associated with state" },
  { issdl_dumppathtree, 4, "dump_dot_pathtree", 0, "dump the rule instance path tree in the GraphViz Dot format"},
  { issdl_drop_event, 5, "drop_event", 0, "Drop event" },
  { issdl_set_event_level, 6, "set_event_level", 1, "Set event level" },
  { issdl_report, 7, "report", 0, "generate report" },

  { issdl_shutdown, 8, "shutdown", 0, "shutdown orchids" },

  { issdl_random, 9, "random", 0, "return a random number" },
  { issdl_system, 10, "system", 0, "execute a system command" },
  { issdl_stats, 11, "show_stats", 0, "show orchids internal statistics" },
  { issdl_str_from_int, 12, "str_from_int", 0, "convert an integer to a string" },
  { issdl_int_from_str, 13, "int_from_str", 0, "convert a string to an integer" },
  { issdl_str_from_ipv4, 14, "str_from_ipv4", 0, "convert an ipv4 address to a string" },
  { issdl_kill_threads, 15, "kill_threads", 0, "kill threads of a rule instance" },
  { issdl_cut, 16, "cut", 1, "special cut" },
  { issdl_pastval, 17, "pastval", 2, "Past value of a variable" },
  { issdl_sendmail, 18, "sendmail", 4, "Send a mail" },
  { issdl_sendmail_report, 19, "sendmail_report", 4, "Send a report by mail" },
  { issdl_bindist, 20, "bitdist", 2, "Number of different bits" },
  { issdl_bytedist, 21, "bytedist", 2, "Number of different bytes" },
  { issdl_vstr_from_regex, 22, "vstr_from_regex", 1, "Return the source virtual string of a compiled regex" },
  { issdl_str_from_regex, 23, "str_from_regex", 1, "Return the source string of a compiled regex" },
  { issdl_regex_from_str, 24, "regex_from_str", 1, "Compile a regex from a string" },
  { issdl_defined, 25, "defined", 1, "Return if a field is defined" },
  { issdl_difftime, 26, "difftime", 1, "The difftime() function shall return the difference expressed in seconds as a type int."},
  { issdl_str_from_time, 12, "str_from_time", 0, "convert an time to a string" },
  { NULL, 0, NULL, 0, NULL }
};

void
register_core_functions(orchids_t *ctx)
{
  issdl_function_t *f;

  for (f = issdl_function_g; f->func; f++)
    register_lang_function(ctx, f->func, f->name, f->args_nb, f->desc);
}

void
register_lang_function(orchids_t *ctx,
                       ovm_func_t func,
                       const char *name,
                       int arity,
                       const char *desc)
{
  issdl_function_t *f;
  size_t array_size;

  DebugLog(DF_ENG, DS_INFO,
           "Registering language function %s/%i @ %p\n",
           name, arity, func);

  array_size = (ctx->vm_func_tbl_sz + 1) * sizeof (issdl_function_t);
  ctx->vm_func_tbl = Xrealloc(ctx->vm_func_tbl, array_size);

  f = &ctx->vm_func_tbl[ ctx->vm_func_tbl_sz ];
  f->func = func;
  f->id = ctx->vm_func_tbl_sz;
  f->name = strdup(name);
  f->args_nb = arity;
  f->desc = strdup(desc);

  ctx->vm_func_tbl_sz++;
}

issdl_function_t *
get_issdl_functions(void)
{
  return (issdl_function_g);
}

void
fprintf_issdl_functions(FILE *fp, orchids_t *ctx)
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


void
set_ip_resolution(int value)
{
  resolve_ipv4_g = value ? TRUE : FALSE;
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
