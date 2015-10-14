/**
 ** @file lang.c
 ** The intrusion scenario signature definition language.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 ** @author Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
 **
 ** @version 1.1
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
#include <math.h>
#include <regex.h>

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

#define TIME_MAX ((time_t)((unsigned long)(1L<<(sizeof(time_t)*8-1))-1))
#define TIME_MIN ((time_t)((unsigned long)(1L<<(sizeof(time_t)*8-1))))

/**
 ** Table of data types natively recognized in the Orchids language.
 **/
static struct issdl_type_s issdl_types_g[] = {
  { "null",    0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "Null type for error/exception management" },
  { "func",    0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "Function reference" },
  { "int",     0, int_get_data, int_get_data_len, int_cmp, int_add, int_sub, int_opp, int_mul, int_div, int_mod, int_clone, int_and, int_or, int_xor, int_not, int_plus,
    "Integer numbers (32-bits signed int)" },
  { "bstr",    0, bytestr_get_data, bytestr_get_data_len, NULL, NULL, NULL, NULL, NULL, NULL, NULL, bstr_clone, NULL, NULL, NULL, NULL, NULL,
    "Binary string, allocated, (unsigned char *)" },
  { "vbstr",   0, vbstr_get_data, vbstr_get_data_len, NULL, NULL, NULL, NULL, NULL, NULL, NULL, vbstr_clone, NULL, NULL, NULL, NULL, NULL,
    "Virtual binary string, not allocated, only pointer/offset reference" },
  { "str",     0, string_get_data, string_get_data_len, str_cmp, str_add, NULL, NULL, NULL, NULL, NULL, string_clone, NULL, NULL, NULL, NULL, NULL,
    "Character string, allocated, (char *)" },
  { "vstr",    0, vstring_get_data, vstring_get_data_len, vstr_cmp, vstr_add, NULL, NULL, NULL, NULL, NULL, vstr_clone, NULL, NULL, NULL, NULL, NULL,
    "Virtual string, not allocated, only pointer/offset reference" },
  { "array",   0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "Array" },
  { "hash",    0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "Hash table" },
  { "ctime",   0, ctime_get_data, ctime_get_data_len, ctime_cmp, ctime_add, ctime_sub, NULL, NULL, NULL, NULL, ctime_clone, NULL, NULL, NULL, NULL, NULL,
    "C Time, seconds since Epoch (Jan. 1, 1970, 00:00 GMT), (time_t)" },
  { "ipv4",    0, ipv4_get_data, ipv4_get_data_len, ipv4_cmp, NULL, NULL, NULL, NULL, NULL, NULL, ipv4_clone, ipv4_and, ipv4_or, ipv4_xor, ipv4_not, NULL,
    "IPv4 address (struct in_addr)" },
  { "ipv6",    0, ipv6_get_data, ipv6_get_data_len, ipv6_cmp, NULL, NULL, NULL, NULL, NULL, NULL, NULL, ipv6_and, ipv6_or, ipv6_xor, ipv6_not, NULL,
    "IPv6 address (struct in6_addr)" },
  { "timeval", 0, timeval_get_data, timeval_get_data_len, timeval_cmp, timeval_add, timeval_sub, NULL, NULL, NULL, NULL, timeval_clone, NULL, NULL, NULL, NULL, NULL,
    "Seconds and microseconds since Epoch, (struct timeval)" },
  { "regex",   0, regex_get_data, regex_get_data_len, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "Posix Extended Regular Expression, with substring addressing" },
  { "uint",    0, uint_get_data, uint_get_data_len, uint_cmp, uint_add, uint_sub, uint_opp, uint_mul, uint_div, uint_mod, uint_clone, uint_and, uint_or, uint_xor, uint_not, uint_plus,
    "Non negative integer (32-bits unsigned int)" },
  { "snmpoid", 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "SNMP Object Identifier" },
  { "float",   0, float_get_data, float_get_data_len, float_cmp, float_add, float_sub, float_opp, float_mul, float_div, NULL, float_clone, NULL, NULL, NULL, NULL, float_plus,
    "IEEE 64-bit floating point number (float)" },
  { "event", 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "Meta event" },
  { "state_instance", 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "State instance/cut mark" },
  { "db", 0, NULL, NULL, db_cmp, db_add, db_sub, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "Empty database" },
  { "db", 0, NULL, NULL, db_cmp, db_add, db_sub, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "Singleton database" },
  { "db", 0, NULL, NULL, db_cmp, db_add, db_sub, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "General database" },
  { "extern",  0, extern_get_data, extern_get_data_len, NULL, NULL, NULL, NULL, NULL, NULL, NULL, extern_clone, NULL, NULL, NULL, NULL, NULL,
    "External data (provided by a plugin)" },
  { NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "" }
};

type_t t_int = { "int", T_INT };
type_t t_uint = { "uint", T_UINT };
type_t t_float = { "float", T_FLOAT };
type_t t_bstr = { "bstr", T_BSTR };
type_t t_str = { "str", T_STR };
type_t t_ctime = { "ctime", T_CTIME };
type_t t_ipv4 = { "ipv4", T_IPV4 };
type_t t_ipv6 = { "ipv6", T_IPV6 };
type_t t_timeval = { "timeval", T_TIMEVAL };
type_t t_regex = { "regex", T_REGEX };
type_t t_snmpoid = { "snmpoid", T_SNMPOID };
type_t t_event = { "event", T_EVENT };
type_t t_mark = { "mark", T_STATE_INSTANCE };
type_t t_db_any = { "db[*]", T_DB_EMPTY };

/* Special types */
type_t t_void = { "void", T_NULL };
type_t t_any = { "*", 128 };

static int resolve_ipv4_g = 0;
static int resolve_ipv6_g = 0;

char *str_issdltype(int type)
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

static int int_save (save_ctx_t *sctx, gc_header_t *p)
{
  return save_long (sctx, INT(p));
}

static gc_class_t int_class;

static gc_header_t *int_restore (restore_ctx_t *rctx)
{
  long n;
  ovm_int_t *p;
  int err;

  err = restore_long (rctx, &n);
  if (err) { errno = err; return NULL; }
  p = gc_alloc (rctx->gc_ctx, sizeof (ovm_int_t), &int_class);
  p->gc.type = T_INT;
  p->val = n;
  return (gc_header_t *)p;
}

static gc_class_t int_class = {
  GC_ID('i','n','t',' '),
  NULL,
  NULL,
  NULL,
  int_save,
  int_restore
};

ovm_var_t *ovm_int_new(gc_t *gc_ctx, long val)
{
  ovm_int_t *n;

  n = gc_alloc (gc_ctx, sizeof (ovm_int_t), &int_class);
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

static int int_cmp(ovm_var_t *var1, ovm_var_t *var2, int dir)
{
  int res;

  if (var2==NULL)
    return CMP_ERROR;
  if (TYPE(var2) != T_INT) /* should not happen (because of typing) */
    return CMP_ERROR;
  res = INT(var1) - INT(var2);
  if (res>0)
    return CMP_GT;
  if (res<0)
    return CMP_LT;
  return CMP_EQ;
}

static ovm_var_t *int_add (gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  long m, n, res;

  if (var2==NULL || var2->gc.type != T_INT)
    return NULL;
  m = INT(var1);
  n = INT(var2);
  res = m+n;
  /* There is an overflow if:
     - m>=0, n>=0, and res<0
     - m<0, n<0, and res>=0
     In case this happens, we return the max (or min) possible value,
     so as to preserve monotonicity.
     There is never any overflow if m and n have opposite signs.
  */
  if (m>=0 && n>=0 && res<0)
    return ovm_int_new (gc_ctx, LONG_MAX);
  if (m<0 && n<0 && res>=0)
    return ovm_int_new (gc_ctx, LONG_MIN);
  return ovm_int_new (gc_ctx, res);
}

static ovm_var_t *int_sub (gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  long m, n, res;

  if (var2==NULL || var2->gc.type != T_INT)
    return NULL;
  /* There is an overflow if:
     - m>=0, n<0, and res<0
     - m<0, n>=0, and res>=0
     In case this happens, we return the max (or min) possible value,
     so as to preserve monotonicity.
     There is never any overflow if m and n have the same sign.
     (Note that 0 - INT_MAX does not overflow in the negative
     [this is equal to INT_MIN+1, in two's complement]
     and (-1) - INT_MIN does not overflow in the positive
     [this is equal to INT_MAX, in two's complement]
   */
  m = INT(var1);
  n = INT(var2);
  res = m-n;
  if (m>=0 && n<0 && res<0)
    return ovm_int_new (gc_ctx, LONG_MAX);
  if (m<0 && n>=0 && res>=0)
    return ovm_int_new (gc_ctx, LONG_MIN);
  return ovm_int_new (gc_ctx, res);
}

static ovm_var_t *int_opp (gc_t *gc_ctx, ovm_var_t *var)
{
  long m = INT(var);

  /* Despite all appearances, opposite is not antitonic.
     The naughty case is when m==LONG_MIN, in which case -m==LONG_MIN as well.
     To force monotonicity, -LONG_MIN (==LONG_MAX+1) is chopped at LONG_MAX.
  */
  if (m==LONG_MIN)
    return ovm_int_new (gc_ctx, LONG_MAX);
  return ovm_int_new (gc_ctx, -m);
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

static ovm_var_t *int_plus(gc_t *gc_ctx, ovm_var_t *var)
{
  return var;
}

/*
** Unsigned integer type
** store an 'uint32_t' value.
*/

static int uint_save (save_ctx_t *sctx, gc_header_t *p)
{
  return save_ulong (sctx, UINT(p));
}

static gc_class_t uint_class;

static gc_header_t *uint_restore (restore_ctx_t *rctx)
{
  unsigned long n;
  ovm_uint_t *p;
  int err;

  err = restore_ulong (rctx, &n);
  if (err) { errno = err; return NULL; }
  p = gc_alloc (rctx->gc_ctx, sizeof (ovm_uint_t), &uint_class);
  p->gc.type = T_UINT;
  p->val = n;
  return (gc_header_t *)p;
}

static gc_class_t uint_class = {
  GC_ID('u','i','n','t'),
  NULL,
  NULL,
  NULL,
  uint_save,
  uint_restore
};

ovm_var_t *ovm_uint_new(gc_t *gc_ctx, unsigned long val)
{
  ovm_uint_t *n;

  n = gc_alloc (gc_ctx, sizeof (ovm_uint_t), &uint_class);
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

static int uint_cmp (ovm_var_t *var1, ovm_var_t *var2, int dir)
{
  int res;

  if (var2==NULL)
    return CMP_ERROR;
  if (TYPE(var2) != T_UINT) /* should not happen (because of typing) */
    return CMP_ERROR;
  res = UINT(var1) - UINT(var2);
  if (res>0)
    return CMP_GT;
  if (res<0)
    return CMP_LT;
  return CMP_EQ;
}

static ovm_var_t *uint_add (gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  unsigned long m, n, res;

  if (var2==NULL || var2->gc.type != T_UINT)
    return NULL;
  m = UINT(var1);
  n = UINT(var2);
  res = m+n;
  /* There is an overflow if res<m (or equivalently, res<n)
     In case this happens, we return the max possible value,
     so as to preserve monotonicity.
   */
  if (res<m)
    return ovm_uint_new (gc_ctx, ULONG_MAX);
  return ovm_uint_new (gc_ctx, res);
}

static ovm_var_t *uint_sub (gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  if (var2==NULL || var2->gc.type != T_UINT)
    return NULL;
  return ovm_uint_new (gc_ctx,
		       (UINT(var1)>=UINT(var2))?
		       (UINT(var1) - UINT(var2)):0);
}

static ovm_var_t *uint_opp (gc_t *gc_ctx, ovm_var_t *var)
{
  unsigned long n;
  long res;

  n = UINT(var);
  if (n+LONG_MIN<n) /* overflow, i.e., when n+LONG_MIN<0, mathematically;
		       equivalently, n<-LONG_MIN: we truncate to LONG_MIN */
    res = LONG_MIN;
  else res = -(long)n;
  return ovm_int_new (gc_ctx, res);
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
  return ovm_uint_new (gc_ctx, ~UINT(var));
}

static ovm_var_t *uint_plus(gc_t *gc_ctx, ovm_var_t *var)
{
  unsigned long n;
  long res;
  
  n = UINT(var);
  if (n>LONG_MAX)
    res = LONG_MAX;
  else res = (long)n;
  return ovm_int_new (gc_ctx, res);
}

/*
** Binary String
** used to store binary objects.
*/

static int bstr_save (save_ctx_t *sctx, gc_header_t *p)
{
  FILE *f = sctx->f;
  ovm_bstr_t *str = (ovm_bstr_t *)p;
  uint8_t *s;
  size_t i, n;
  int c, err;

  err = save_size_t (sctx, BSTRLEN(str));
  if (err) return err;
  for (i=0, n=BSTRLEN(str), s=BSTR(str); i<n; i++)
    {
      c = s[i];
      if (putc_unlocked (c, f) < 0) return errno;
    }
  return 0;
}

static gc_class_t bstr_class;

static gc_header_t *bstr_restore (restore_ctx_t *rctx)
{
  FILE *f = rctx->f;
  ovm_bstr_t *bstr;
  uint8_t *s;
  int err=0;
  int c;
  size_t i, sz;

  /* We have already read the first byte, and it was T_BSTR. */
  err = restore_size_t (rctx, &sz);
  if (err) goto errlab;
  bstr = gc_alloc (rctx->gc_ctx, offsetof(ovm_bstr_t,str[sz]), &bstr_class);
  bstr->gc.type = T_BSTR;
  bstr->len = sz;
  for (i=0, s=bstr->str; i<sz; i++)
    {
      c = getc_unlocked (f);
      if (c==EOF)
	{ err = c; goto errlab; }
      s[i] = c;
    }
  goto normal;
 errlab:
  bstr = NULL;
  errno = err;
 normal:
  return (gc_header_t *)bstr;
}

static gc_class_t bstr_class = {
  GC_ID('b','s','t','r'),
  NULL,
  NULL,
  NULL,
  bstr_save,
  bstr_restore
};


ovm_var_t *ovm_bstr_new (gc_t *gc_ctx, size_t size)
{
  ovm_bstr_t *bstr;

  bstr = gc_alloc (gc_ctx, offsetof(ovm_bstr_t,str[size]), &bstr_class);
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

static int vbstr_save (save_ctx_t *sctx, gc_header_t *p)
{
  FILE *f = sctx->f;
  ovm_vbstr_t *str = (ovm_vbstr_t *)p;
  uint8_t *s;
  size_t i, n;
  int c, err;

  /*  if (putc_unlocked (TYPE(p), f) < 0) return errno;
      // TYPE(p) will already have been saved.
      */
  err = save_size_t (sctx, VBSTRLEN(str));
  if (err) return err;
  for (i=0, n=VBSTRLEN(str), s=VBSTR(str); i<n; i++)
    {
      c = s[i];
      if (putc_unlocked (c, f) < 0) return errno;
    }
  return 0;
}

static gc_class_t vbstr_class;
static gc_class_t bstr_class;

static gc_header_t *vbstr_restore (restore_ctx_t *rctx)
{
  gc_t *gc_ctx = rctx->gc_ctx;
  FILE *f = rctx->f;
  ovm_vbstr_t *vbstr;
  ovm_bstr_t *bstr;
  uint8_t *s;
  int err=0;
  int c;
  size_t i, sz;

  GC_START (gc_ctx, 1);
  vbstr = gc_alloc (gc_ctx, sizeof (ovm_vbstr_t), &vbstr_class);
  vbstr->gc.type = T_VBSTR;
  vbstr->delegate = NULL;
  GC_UPDATE (gc_ctx, 0, vbstr);
  /* We have already read the first byte, and it was T_VBSTR. */
  err = restore_size_t (rctx, &sz);
  if (err) goto errlab;
  VBSTRLEN(vbstr) = sz;
  bstr = gc_alloc (gc_ctx, offsetof(ovm_bstr_t,str[sz+1]), &bstr_class);
  bstr->gc.type = T_BSTR;
  bstr->len = sz;
  for (i=0, s=bstr->str; i<sz; i++)
    {
      c = getc_unlocked (f);
      if (c==EOF)
	{ err = c; goto errlab; }
      s[i] = c;
    }
  s[sz] = '\0'; /* some functions (e.g., error reporting in rule_compiler.c)
		   assume a final NUL character */
  GC_TOUCH (gc_ctx, vbstr->delegate = (ovm_var_t *)bstr);
  VBSTR(vbstr) = BSTR(bstr);
  goto normal;
 errlab:
  vbstr = NULL;
  errno = err;
 normal:
  GC_END (gc_ctx);
  return (gc_header_t *)vbstr;
}

static gc_class_t vbstr_class = {
  GC_ID('v','b','s','t'),
  vbstr_mark_subfields,
  NULL,
  vbstr_traverse,
  vbstr_save,
  vbstr_restore
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

static int str_save (save_ctx_t *sctx, gc_header_t *p)
{
  FILE *f = sctx->f;
  ovm_str_t *str = (ovm_str_t *)p;
  char *s;
  size_t i, n;
  int c, err;

  err = save_size_t (sctx, STRLEN(str));
  if (err) return err;
  for (i=0, n=STRLEN(str), s=STR(str); i<n; i++)
    {
      c = s[i];
      if (putc_unlocked (c, f) < 0) return errno;
    }
  return 0;
}

static gc_class_t str_class;

static gc_header_t *str_restore (restore_ctx_t *rctx)
{
  FILE *f = rctx->f;
  ovm_str_t *str;
  char *s;
  int err=0;
  int c;
  size_t i, sz;

  /* We have already read the first byte, and it was T_STR. */
  err = restore_size_t (rctx, &sz);
  if (err) goto errlab;
  str = gc_alloc (rctx->gc_ctx, offsetof(ovm_str_t,str[sz]), &str_class);
  str->gc.type = T_STR;
  str->len = sz;
  for (i=0, s=str->str; i<sz; i++)
    {
      c = getc_unlocked (f);
      if (c==EOF)
	{ err = c; goto errlab; }
      s[i] = c;
    }
  goto normal;
 errlab:
  str = NULL;
  errno = err;
 normal:
  return (gc_header_t *)str;
}

static gc_class_t str_class = {
  GC_ID('s','t','r',' '),
  NULL,
  NULL,
  NULL,
  str_save,
  str_restore
};

ovm_var_t *ovm_str_new(gc_t *gc_ctx, size_t size)
{
  ovm_str_t *str;

  str = gc_alloc (gc_ctx, offsetof(ovm_str_t,str[size]), &str_class);
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

static int str_cmp(ovm_var_t *var1, ovm_var_t *var2, int dir)
{
  int n;

  if (var2==NULL)
    return CMP_ERROR;
  if (TYPE(var2) == T_STR)
    {
      n = STRLEN(var1) - STRLEN(var2);
      if (n==0)
	n = memcmp(STR(var1), STR(var2), STRLEN(var1));
    }
  else if (TYPE(var2) == T_VSTR)
    {
      n = STRLEN(var1) - VSTRLEN(var2);
      if (n==0)
	n = memcmp(STR(var1), VSTR(var2), STRLEN(var1));
    }
  else
    {
      DebugLog(DF_OVM, DS_DEBUG, "Type error\n");
      return CMP_ERROR;
    }
  if (n>0)
    return CMP_GT;
  if (n<0)
    return CMP_LT;
  return CMP_EQ;
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

static int vstr_save (save_ctx_t *sctx, gc_header_t *p)
{
  FILE *f = sctx->f;
  ovm_vstr_t *str = (ovm_vstr_t *)p;
  char *s;
  size_t i, n;
  int c, err;

  /*  if (putc_unlocked (TYPE(p), f) < 0) return errno;
      // TYPE(p) will already have been saved.
      */
  err = save_size_t (sctx, VSTRLEN(str));
  if (err) return err;
  for (i=0, n=VSTRLEN(str), s=VSTR(str); i<n; i++)
    {
      c = s[i];
      if (putc_unlocked (c, f) < 0) return errno;
    }
  return 0;
}

static gc_class_t vstr_class;

static gc_header_t *vstr_restore (restore_ctx_t *rctx)
{
  gc_t *gc_ctx = rctx->gc_ctx;
  FILE *f = rctx->f;
  ovm_vstr_t *vstr;
  ovm_str_t *str;
  char *s;
  int err=0;
  int c;
  size_t i, sz;

  GC_START (gc_ctx, 1);
  vstr = gc_alloc (gc_ctx, sizeof (ovm_vstr_t), &vstr_class);
  vstr->gc.type = T_VSTR;
  vstr->delegate = NULL;
  GC_UPDATE (gc_ctx, 0, vstr);
  /* We have already read the first byte, and it was T_VSTR. */
  err = restore_size_t (rctx, &sz);
  if (err) goto errlab;
  VSTRLEN(vstr) = sz;
  str = gc_alloc (gc_ctx, offsetof(ovm_str_t,str[sz+1]), &str_class);
  str->gc.type = T_STR;
  str->len = sz;
  for (i=0, s=str->str; i<sz; i++)
    {
      c = getc_unlocked (f);
      if (c==EOF)
	{ err = c; goto errlab; }
      s[i] = c;
    }
  s[sz] = '\0'; /* some functions (e.g., error reporting in rule_compiler.c)
		   assume a final NUL character */
  GC_TOUCH (gc_ctx, vstr->delegate = (ovm_var_t *)str);
  VSTR(vstr) = STR(str);
  goto normal;
 errlab:
  vstr = NULL;
  errno = err;
 normal:
  GC_END (gc_ctx);
  return (gc_header_t *)vstr;
}

static gc_class_t vstr_class = {
  GC_ID('v','s','t', 'r'),
  vstr_mark_subfields,
  NULL,
  vstr_traverse,
  vstr_save,
  vstr_restore
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

static int vstr_cmp(ovm_var_t *var1, ovm_var_t *var2, int dir)
{
  int n;

  if (var2==NULL)
    return CMP_ERROR;
  if (TYPE(var2) == T_STR)
    {
      n = VSTRLEN(var1) - STRLEN(var2);
      if (n==0)
	n = memcmp(VSTR(var1), STR(var2), VSTRLEN(var1));
    }
  else if (TYPE(var2) == T_VSTR)
    {
      n = VSTRLEN(var1) - VSTRLEN(var2);
      if (n==0)
	n = memcmp(VSTR(var1), VSTR(var2), VSTRLEN(var1));
    }
  else
    {
      DebugLog(DF_OVM, DS_DEBUG, "Type error\n");
      return CMP_ERROR;
    }
  if (n>0)
    return CMP_GT;
  if (n<0)
    return CMP_LT;
  return CMP_EQ;
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

static int ctime_save (save_ctx_t *sctx, gc_header_t *p)
{
  return save_ctime (sctx, CTIME(p));
}

static gc_class_t ctime_class;

static gc_header_t *ctime_restore (restore_ctx_t *rctx)
{
  time_t tm;
  ovm_ctime_t *p;
  int err;

  err = restore_ctime (rctx, &tm);
  if (err) { errno = err; return NULL; }
  p = gc_alloc (rctx->gc_ctx, sizeof (ovm_ctime_t), &ctime_class);
  p->gc.type = T_CTIME;
  p->time = tm;
  return (gc_header_t *)p;
}

static gc_class_t ctime_class = {
  GC_ID('t','i','m','e'),
  NULL,
  NULL,
  NULL,
  ctime_save,
  ctime_restore
};

ovm_var_t *ovm_ctime_new(gc_t *gc_ctx, time_t tm)
{
  ovm_ctime_t *time;

  time = gc_alloc (gc_ctx, sizeof (ovm_ctime_t), &ctime_class);
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

static int ctime_cmp(ovm_var_t *var1, ovm_var_t *var2, int dir)
{
  long res;

  if (var2==NULL)
    return CMP_ERROR;
  if (TYPE(var2) != T_CTIME) /* should not happen (because of typing) */
    return CMP_ERROR;
  res = CTIME(var1) - CTIME(var2);
  if (res>0)
    return CMP_GT;
  if (res<0)
    return CMP_LT;
  return CMP_EQ;
}

static ovm_var_t *ctime_add(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  if (var2==NULL)
    return NULL;
  switch (TYPE(var2))
    {
    case T_CTIME:
      {
	time_t m, n, res;

	/* For overflow rules, see int_add(); we don't know much about time_t,
	 except that it must be signed. I am assuming two's complement to obtain
	 the min and max values. */
	m = CTIME(var1);
	n = CTIME(var2);
	res = m+n;
	if (m>=0 && n>=0 && res<0)
	  return ovm_ctime_new (gc_ctx, TIME_MAX);
	if (m<0 && n<0 && res>=0)
	  return ovm_ctime_new (gc_ctx, TIME_MIN);
	return ovm_ctime_new (gc_ctx, res);
      }
    case T_INT:
      {
	time_t m, res;
	long n;

	/* For overflow rules, see int_add(); we don't know much about time_t,
	 except that it must be signed. I am assuming two's complement to obtain
	 the min and max values. */
	m = CTIME(var1);
	n = CTIME(var2);
	res = m+n;
	if (m>=0 && n>=0 && res<0)
	  return ovm_ctime_new (gc_ctx, TIME_MAX);
	if (m<0 && n<0 && res>=0)
	  return ovm_ctime_new (gc_ctx, TIME_MIN);
	return ovm_ctime_new (gc_ctx, res);
      }
    case T_TIMEVAL:
      {
	long tv_sec, tv_res;
	time_t n;
	ovm_var_t *res;

	/* For overflow rules, see int_add(); we don't know much about time_t,
	 except that it must be signed. I am assuming two's complement to obtain
	 the min and max values. */
	tv_sec = TIMEVAL(var2).tv_sec;
	n = CTIME(var1);
	tv_res = tv_sec+n;
	res = ovm_timeval_new (gc_ctx);
	if (tv_sec>=0 && n>=0 && tv_res<0)
	  {
	    TIMEVAL(res).tv_sec = LONG_MAX;
	    TIMEVAL(res).tv_usec = 999999;
	  }
	else if (tv_sec<0 && n<0 && tv_res>=0)
	  {
	    TIMEVAL(res).tv_sec = LONG_MIN;
	    TIMEVAL(res).tv_usec = 0;
	  }
	else
	  {
	    TIMEVAL(res).tv_sec = tv_sec;
	    TIMEVAL(res).tv_usec = TIMEVAL(var2).tv_usec;
	  }
	return res;
      }
    default:
      return NULL;
    }
}

static ovm_var_t *ctime_sub(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  time_t m, res;

  if (var2==NULL)
    return NULL;
  m = CTIME(var1);
  /* For overflow cases, see int_sub() */
  switch (TYPE(var2))
    {
    case T_CTIME:
      {
	time_t n = CTIME(var2);

	res = m-n;
	if (m>=0 && n<0 && res<0)
	  return ovm_ctime_new (gc_ctx, TIME_MAX);
	if (m<0 && n>=0 && res>=0)
	  return ovm_ctime_new (gc_ctx, TIME_MIN);
	return ovm_ctime_new (gc_ctx, res);
      }
    case T_INT:
      {
	long n = INT(var2);

	res = m-n;
	if (m>=0 && n<0 && res<0)
	  return ovm_ctime_new (gc_ctx, TIME_MAX);
	if (m<0 && n>=0 && res>=0)
	  return ovm_ctime_new (gc_ctx, TIME_MIN);
	return ovm_ctime_new (gc_ctx, res);
      }
    default:
      return NULL;
    }
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

static int ipv4_save (save_ctx_t *sctx, gc_header_t *p)
{
  ovm_ipv4_t *addr = (ovm_ipv4_t *)p;
  in_addr_t a = IPV4(addr).s_addr;

  return save_uint32 (sctx, (uint32_t)a);
}

static gc_class_t ipv4_class;

static gc_header_t *ipv4_restore (restore_ctx_t *rctx)
{
  uint32_t a;
  ovm_ipv4_t *addr;
  int err;

  err = restore_uint32 (rctx, &a);
  if (err) { errno = err; return NULL; }
  addr = gc_alloc (rctx->gc_ctx, sizeof (ovm_ipv4_t), &ipv4_class);
  addr->gc.type = T_IPV4;
  IPV4(addr).s_addr = (in_addr_t)a;
  return (gc_header_t *)addr;
}

static gc_class_t ipv4_class = {
  GC_ID('i','p','v','4'),
  NULL,
  NULL,
  NULL,
  ipv4_save,
  ipv4_restore
};

ovm_var_t *ovm_ipv4_new(gc_t* gc_ctx)
{
  ovm_ipv4_t *addr;

  addr = gc_alloc (gc_ctx, sizeof (ovm_ipv4_t), &ipv4_class);
  addr->gc.type = T_IPV4;
  return OVM_VAR(addr);
}

static int ipv4_cmp(ovm_var_t *var1, ovm_var_t *var2, int dir)
{
  in_addr_t a1, a2;

  if (var2==NULL)
    return CMP_ERROR;
  if (TYPE(var2) != T_IPV4) /* should not happen (because of typing) */
    return CMP_ERROR;
  a1 = IPV4(var1).s_addr;
  a2 =  IPV4(var2).s_addr;
  if (a1>a2)
    return CMP_GT;
  if (a1<a2)
    return CMP_LT;
  return CMP_EQ;
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
  fprintf(fp, "ipv4 : %s", inet_ntoa(IPV4(addr)));
  /* address resolution */
  hptr = gethostbyaddr((char *)&IPV4(addr),
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

static int ipv6_save (save_ctx_t *sctx, gc_header_t *p)
{
  ovm_ipv6_t *addr = (ovm_ipv6_t *)p;
  FILE *f = sctx->f;
  int i, c;

  for (i = 0 ; i < 16 ; i++)
    {
      c = (char)IPV6(addr).s6_addr[i];
      if (putc_unlocked (c, f) < 0) return errno;
    }
  return 0;
}

static gc_class_t ipv6_class;

static gc_header_t *ipv6_restore (restore_ctx_t *rctx)
{
  FILE *f = rctx->f;
  ovm_ipv6_t *addr;
  int err=0;
  int c;
  int i;

  addr = gc_alloc (rctx->gc_ctx, sizeof(ovm_ipv6_t), &ipv6_class);
  addr->gc.type = T_IPV6;
  for (i=0; i<16; i++)
    {
      c = getc_unlocked (f);
      if (c==EOF)
	{ err = c; goto errlab; }
      IPV6(addr).s6_addr[i] = (unsigned char)c;
    }
  goto normal;
 errlab:
  addr = NULL;
  errno = err;
 normal:
  return (gc_header_t *)addr;
}

static gc_class_t ipv6_class = {
  GC_ID('i','p','v','6'),
  NULL,
  NULL,
  NULL,
  ipv6_save,
  ipv6_restore
};

ovm_var_t *ovm_ipv6_new(gc_t* gc_ctx)
{
  ovm_ipv6_t *addr;

  addr = gc_alloc (gc_ctx, sizeof (ovm_ipv6_t), &ipv6_class);
  addr->gc.type = T_IPV6;
  return OVM_VAR(addr);
}

static int ipv6_cmp(ovm_var_t *var1, ovm_var_t *var2, int dir)
{
  int i;
  unsigned char a1, a2;

  if (var2 == NULL || TYPE(var2) != T_IPV6)
    return CMP_ERROR;
  for (i = 0 ; i < 16 ; i++)
    {
      a1 = IPV6(var1).s6_addr[i];
      a2 = IPV6(var2).s6_addr[i];
      if (a1!=a2)
	{
	  if (a1 > a2)
	    return CMP_GT;
	  return CMP_LT;
	}
    }
  return CMP_EQ;
}

static void *ipv6_get_data(ovm_var_t *addr)
{
  return &IPV6(addr);
}

static size_t ipv6_get_data_len(ovm_var_t *addr)
{
  return sizeof (struct in6_addr);
}

static ovm_var_t *ipv6_and (gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  ovm_var_t *res;

  if ((var2 == NULL) || (TYPE(var2) != T_IPV6))
    return NULL;

  res = ovm_ipv6_new(gc_ctx);
  
  int i = 0;
  for (i = 0 ; i < 16 ; i++)
    IPV6(res).s6_addr[i] = IPV6(var1).s6_addr[i] & IPV6(var2).s6_addr[i];
  return res;
}

static ovm_var_t *ipv6_or (gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  ovm_var_t *res;

  if ((var2 == NULL) || (TYPE(var2) != T_IPV6))
    return NULL;

  res = ovm_ipv6_new(gc_ctx);
  
  int i = 0;
  for (i = 0 ; i < 16 ; i++)
    IPV6(res).s6_addr[i] = IPV6(var1).s6_addr[i] | IPV6(var2).s6_addr[i];
  return res;
}

static ovm_var_t *ipv6_xor (gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  ovm_var_t *res;

  if ((var2 == NULL) || (TYPE(var2) != T_IPV6))
    return NULL;

  res = ovm_ipv6_new(gc_ctx);

  int i = 0;
  for (i = 0 ; i < 16 ; i++)
    IPV6(res).s6_addr[i] = IPV6(var1).s6_addr[i] ^ IPV6(var2).s6_addr[i];
  return res;
}

static ovm_var_t *ipv6_not(gc_t *gc_ctx, ovm_var_t *var)
{
  ovm_var_t *res;

  if (var==NULL || TYPE(var) != T_IPV6)
    return NULL;

  res = ovm_ipv6_new(gc_ctx);

  int i = 0;
  for (i = 0 ; i < 16 ; i++)
    IPV6(res).s6_addr[i] = ~ IPV6(var).s6_addr[i];
  return res;
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
  (void) inet_ntop (AF_INET6, &IPV6(addr), dst, sizeof(dst));
  fprintf(fp, "ipv6 : %s", dst);

  /* address resolution */
  hptr = gethostbyaddr((char *)&IPV6(addr),
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

static int timeval_save (save_ctx_t *sctx, gc_header_t *p)
{
  ovm_timeval_t *time = (ovm_timeval_t *)p;
  int err;

  err = save_long (sctx, time->time.tv_sec);
  if (err) return err;
  err = save_long (sctx, time->time.tv_usec);
  return err;
}

static gc_class_t timeval_class;

static gc_header_t *timeval_restore (restore_ctx_t *rctx)
{
  ovm_timeval_t *time;
  long sec, usec;
  int err;

  err = restore_long (rctx, &sec);
  if (err) { errno = err; return NULL; }
  err = restore_long (rctx, &usec);
  if (err) { errno = err; return NULL; }
  time = gc_alloc (rctx->gc_ctx, sizeof (ovm_timeval_t), &timeval_class);
  time->gc.type = T_TIMEVAL;
  time->time.tv_sec = sec;
  time->time.tv_usec = usec;
  return (gc_header_t *)time;
}

static gc_class_t timeval_class = {
  GC_ID('t','v','a','l'),
  NULL,
  NULL,
  NULL,
  timeval_save,
  timeval_restore
};

ovm_var_t *ovm_timeval_new(gc_t *gc_ctx)
{
  ovm_timeval_t *time;

  time = gc_alloc (gc_ctx, sizeof (ovm_timeval_t), &timeval_class);
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
  /* For overflow rules, see int_add() */
  switch (TYPE(var2))
    {
    case T_TIMEVAL:
      res = ovm_timeval_new(gc_ctx);
      Timer_Add(&TIMEVAL(res) , &TIMEVAL(var1) , &TIMEVAL(var2));
      if (TIMEVAL(var1).tv_sec>=0 && TIMEVAL(var2).tv_sec>=0 && TIMEVAL(res).tv_sec<0)
	{
	  TIMEVAL(res).tv_sec = LONG_MAX;
	  TIMEVAL(res).tv_usec = 999999;
	}
      else if (TIMEVAL(var1).tv_sec<0 && TIMEVAL(var2).tv_sec<0 && TIMEVAL(res).tv_sec>=0)
	{
	  TIMEVAL(res).tv_sec = LONG_MIN;
	  TIMEVAL(res).tv_usec = 0;
	}
      return res;
    case T_CTIME:
      {
	long tv_sec, tv_res;
	time_t n;
	ovm_var_t *res;

	tv_sec = TIMEVAL(var1).tv_sec;
	n = CTIME(var2);
	tv_res = tv_sec+n;
	res = ovm_timeval_new (gc_ctx);
	if (tv_sec>=0 && n>=0 && tv_res<0)
	  {
	    TIMEVAL(res).tv_sec = LONG_MAX;
	    TIMEVAL(res).tv_usec = 999999;
	  }
	else if (tv_sec<0 && n<0 && tv_res>=0)
	  {
	    TIMEVAL(res).tv_sec = LONG_MIN;
	    TIMEVAL(res).tv_usec = 0;
	  }
	else
	  {
	    TIMEVAL(res).tv_sec = tv_sec;
	    TIMEVAL(res).tv_usec = TIMEVAL(var2).tv_usec;
	  }
	return res;
      }
    case T_INT:
      {
	long tv_sec, tv_res;
	long n;
	ovm_var_t *res;

	tv_sec = TIMEVAL(var1).tv_sec;
	n = INT(var2);
	tv_res = tv_sec+n;
	res = ovm_timeval_new (gc_ctx);
	if (tv_sec>=0 && n>=0 && tv_res<0)
	  {
	    TIMEVAL(res).tv_sec = LONG_MAX;
	    TIMEVAL(res).tv_usec = 999999;
	  }
	else if (tv_sec<0 && n<0 && tv_res>=0)
	  {
	    TIMEVAL(res).tv_sec = LONG_MIN;
	    TIMEVAL(res).tv_usec = 0;
	  }
	else
	  {
	    TIMEVAL(res).tv_sec = tv_sec;
	    TIMEVAL(res).tv_usec = TIMEVAL(var2).tv_usec;
	  }
	return res;
      }
    default:
      return NULL;
    }
}

static ovm_var_t *timeval_sub(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  ovm_var_t *res;

  if (var2==NULL)
    return NULL;
  /* For overflow cases, see int_sub() */
  switch (TYPE(var2))
    {
    case T_TIMEVAL:
      res = ovm_timeval_new(gc_ctx);
      Timer_Sub(&TIMEVAL(res) , &TIMEVAL(var1) , &TIMEVAL(var2));
      if (TIMEVAL(var1).tv_sec>=0 && TIMEVAL(var2).tv_sec<0 && TIMEVAL(res).tv_sec<0)
	{
	  TIMEVAL(res).tv_sec = LONG_MAX;
	  TIMEVAL(res).tv_usec = 999999;
	}
      else if (TIMEVAL(var1).tv_sec<0 && TIMEVAL(var2).tv_sec>=0 && TIMEVAL(res).tv_sec>=0)
	{
	  TIMEVAL(res).tv_sec = LONG_MIN;
	  TIMEVAL(res).tv_usec = 0;
	}
      return res;
    case T_CTIME:
      {
	time_t n = CTIME(var2);

	res = timeval_clone(gc_ctx, var1);
	TIMEVAL(res).tv_sec -= n;
	if (TIMEVAL(var1).tv_sec>=0 && n<0 && TIMEVAL(res).tv_sec<0)
	  {
	    TIMEVAL(res).tv_sec = LONG_MAX;
	    TIMEVAL(res).tv_usec = 999999;
	  }
	else if (TIMEVAL(var1).tv_sec<0 && n>=0 && TIMEVAL(res).tv_sec<0)
	  {
	    TIMEVAL(res).tv_sec = LONG_MIN;
	    TIMEVAL(res).tv_usec = 0;
	  }
	return res;
      }
    case T_INT:
      {
	long n = INT(var2);

	res = timeval_clone(gc_ctx, var1);
	TIMEVAL(res).tv_sec -= n;
	if (TIMEVAL(var1).tv_sec>=0 && n<0 && TIMEVAL(res).tv_sec<0)
	  {
	    TIMEVAL(res).tv_sec = LONG_MAX;
	    TIMEVAL(res).tv_usec = 999999;
	  }
	else if (TIMEVAL(var1).tv_sec<0 && n>=0 && TIMEVAL(res).tv_sec<0)
	  {
	    TIMEVAL(res).tv_sec = LONG_MIN;
	    TIMEVAL(res).tv_usec = 0;
	  }
	return res;
      }
    default:
      return NULL;
    }
}

static int timeval_cmp(ovm_var_t *var1, ovm_var_t *var2, int dir)
{
  int res;

  if (var2==NULL)
    return CMP_ERROR;
  if (TYPE(var2) != T_TIMEVAL) /* should not happen (because of typing) */
    return CMP_ERROR;
  res = 0;
  if (dir & CMP_LEQ_MASK)
    {
      if (timercmp (&TIMEVAL(var1), &TIMEVAL(var2), <=))
	res |= CMP_LEQ_MASK;
    }
  if (dir & CMP_GEQ_MASK)
    {
      if (timercmp (&TIMEVAL(var1), &TIMEVAL(var2), >=))
	res |= CMP_GEQ_MASK;
    }
  return res;
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

static void regex_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  ovm_regex_t *regex = (ovm_regex_t *)p;

  //if (REGEX(regex).re_magic!=0)
    regfree(&(REGEX(regex)));
  if (REGEXSTR(regex)!=NULL)
    gc_base_free(REGEXSTR(regex));
}

static int regex_save (save_ctx_t *sctx, gc_header_t *p)
{
  ovm_regex_t *regex = (ovm_regex_t *)p;

  return save_string (sctx, REGEXSTR(regex));
}

static gc_class_t regex_class;

static gc_header_t *regex_restore (restore_ctx_t *rctx)
{
  ovm_regex_t *regex;
  char *str;
  int err;

  /* We have already read the first byte, and it was T_REGEX. */
  err = restore_string (rctx, &str);
  if (err) goto errlab;
  regex = gc_alloc (rctx->gc_ctx, sizeof(ovm_regex_t), &regex_class);
  regex->gc.type = T_REGEX;
  regex->regex_str = str;
  //regex->regex.re_magic = 0;
  regex->splits = 0;
  if (str==NULL) { errno = -2; goto errlab2; }
  err = regcomp(&REGEX(regex), str, REG_EXTENDED);
  if (err) goto errlab;
  REGEXNUM(regex) = REGEX(regex).re_nsub;
  goto normal;
 errlab:
  errno = err;
 errlab2:
  regex = NULL;
 normal:
  return (gc_header_t *)regex;
}

static gc_class_t regex_class = {
  GC_ID('r','e','g','x'),
  NULL,
  regex_finalize,
  NULL,
  regex_save,
  regex_restore
};

ovm_var_t *ovm_regex_new(gc_t *gc_ctx)
{
  ovm_regex_t *regex;

  regex = gc_alloc (gc_ctx, sizeof (ovm_regex_t), &regex_class);
  regex->gc.type = T_REGEX;
  regex->regex_str = NULL;
  //regex->regex.re_magic = 0;
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

static int float_save (save_ctx_t *sctx, gc_header_t *p)
{
  return save_double (sctx, FLOAT(p));
}

static gc_class_t float_class;

static gc_header_t *float_restore (restore_ctx_t *rctx)
{
  double x;
  ovm_float_t *p;
  int err;

  err = restore_double (rctx, &x);
  if (err) { errno = err; return NULL; }
  p = gc_alloc (rctx->gc_ctx, sizeof (ovm_float_t), &float_class);
  p->gc.type = T_FLOAT;
  p->val = x;
  return (gc_header_t *)p;
}

static gc_class_t float_class = {
  GC_ID('f','l','t',' '),
  NULL,
  NULL,
  NULL,
  float_save,
  float_restore
};

ovm_var_t *ovm_float_new(gc_t *gc_ctx, double val)
{
  ovm_float_t *n;

  n = gc_alloc (gc_ctx, sizeof (ovm_float_t), &float_class);
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

static int float_cmp(ovm_var_t *var1, ovm_var_t *var2, int dir)
{
  double x1, x2;
  int res;

  if (var2==NULL)
    return CMP_ERROR;
  if (TYPE(var2) != T_FLOAT) /* should not happen (because of typing) */
    return CMP_ERROR;
  x1 = FLOAT(var1);
  x2 = FLOAT(var2);
  res = 0;
  if (dir & CMP_LEQ_MASK)
    {
      if (islessequal(x1,x2))
	res |= CMP_LEQ_MASK;
    }
  if (dir & CMP_GEQ_MASK)
    {
      if (isgreaterequal(x1,x2))
	res |= CMP_GEQ_MASK;
    }
  return res;
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

static ovm_var_t *float_plus(gc_t *gc_ctx, ovm_var_t *var)
{
  return var;
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

static int snmpoid_save (save_ctx_t *sctx, gc_header_t *p)
{
  ovm_snmpoid_t *oid = (ovm_snmpoid_t *)p;
  int err;
  size_t i, n;

  n = SNMPOIDLEN(oid);
  err = save_size_t (sctx, n);
  if (err) return err;
  for (i=0; i<n; i++)
    {
      err = save_ulong (sctx, SNMPOID(oid)[i]);
      if (err) return err;
    }
  return 0;
}

static gc_class_t snmpoid_class;

static gc_header_t *snmpoid_restore (restore_ctx_t *rctx)
{
  ovm_snmpoid_t *oid;
  int err;
  size_t i, n;

  err = restore_size_t (rctx, &n);
  if (err) { errno = err; return NULL; }
  oid = gc_alloc (rctx->gc_ctx, offsetof(ovm_snmpoid_t,objoid[n]), &snmpoid_class);
  oid->gc.type = T_SNMPOID;
  oid->len = n;
  for (i=0; i<n; i++)
    {
      err = restore_size_t (rctx, &oid->objoid[i]);
      if (err) { errno = err; return NULL; }
    }
  return (gc_header_t *)oid;
}

static gc_class_t snmpoid_class = {
  GC_ID('s','n','m','p'),
  NULL,
  NULL,
  NULL,
  snmpoid_save,
  snmpoid_restore
};

ovm_var_t *ovm_snmpoid_new(gc_t *gc_ctx, size_t len)
{
  ovm_snmpoid_t *o;

  o = gc_alloc (gc_ctx, offsetof(ovm_snmpoid_t,objoid[len]), &snmpoid_class);
  o->gc.type = T_SNMPOID;
  o->len = len;
  return OVM_VAR(o);
}

/* raw data support */

void *issdl_get_data(ovm_var_t *val)
{
  if (val==NULL)
    return NULL;
  if ( issdl_types_g[ val->gc.type ].get_data )
    return (*issdl_types_g[ val->gc.type ].get_data) (val);
  DebugLog(DF_OVM, DS_WARN,
	   "issdl_get_data(): get_data doesn't apply for type '%s'\n",
	   STRTYPE(val));
  return NULL;
}

size_t issdl_get_data_len(ovm_var_t *val)
{
  if (val==NULL)
    return 0;
  if (issdl_types_g[val->gc.type].get_data_len)
    return (*issdl_types_g[val->gc.type].get_data_len) (val);
  DebugLog(DF_OVM, DS_WARN,
	   "issdl_get_data_len(): get_data_len doesn't apply for type '%s'\n",
	   STRTYPE(val));
  return 0;
}

/* Databases
 */

static int db_cmp (ovm_var_t *db1, ovm_var_t *db2, int dir)
{
  int res;

  if (db2==NULL)
    return CMP_ERROR;
  res = 0;
  if (dir & CMP_LEQ_MASK)
    {
      if (db_included_lazy ((db_map *)db1, (db_map *)db2))
	res |= CMP_LEQ_MASK;
    }
  if (dir & CMP_GEQ_MASK)
    {
      if (db_included_lazy ((db_map *)db2, (db_map *)db1))
	res |= CMP_GEQ_MASK;
    }
  return res;
}

static ovm_var_t *db_add(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  if (var2==NULL)
    return NULL;
  return (ovm_var_t *)db_union_lazy (gc_ctx, (db_map *)var1, (db_map *)var2);
}

static ovm_var_t *db_sub(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  if (var2==NULL)
    return NULL;
  return (ovm_var_t *)db_diff_lazy (gc_ctx, (db_map *)var1, (db_map *)var2);
}

/*
** External data type
** store an 'void *' pointer.
*/

static void extern_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  (*EXTFREE(p)) (EXTPTR(p));
}

static int extern_save (save_ctx_t *sctx, gc_header_t *p)
{
  ovm_extern_t *a = (ovm_extern_t *)p;
  char *desc;
  int err;

  desc = EXTDESC(a);
  err = save_string (sctx, desc);
  if (err) return err;
  return (*EXTSAVE(a)) (sctx, EXTPTR(a));
}

static gc_class_t extern_class;

static gc_header_t *extern_restore (restore_ctx_t *rctx)
{
  gc_t *gc_ctx = rctx->gc_ctx;
  int err;
  ovm_extern_t *a;
  char *desc;
  struct ovm_extern_class_s *class;
  void *ptr;

  a = NULL;
  err = restore_string (rctx, &desc);
  if (err) { errno = err; goto end; }
  if (desc==NULL) { errno = -2; goto end; }
  class = strhash_get (rctx->externs, desc);
  if (class==NULL)
    { errno = -2; goto end; }
  ptr = (*class->restore) (rctx);
  if (ptr==NULL && errno!=0)
    goto end;
  a = gc_alloc (gc_ctx, sizeof (ovm_extern_t), &extern_class);
  a->gc.type = T_EXTERNAL;
  a->ptr = ptr;
  a->class = class;
 end:
  if (desc!=NULL)
    gc_base_free (desc);
  return (gc_header_t *)a;
}

static gc_class_t extern_class = {
  GC_ID('x','t','r','n'),
  NULL,
  extern_finalize,
  NULL,
  extern_save,
  extern_restore
};

ovm_var_t *ovm_extern_new(gc_t *gc_ctx, void *ptr, ovm_extern_class_t *xclass)
{
  ovm_extern_t *addr;

  addr = gc_alloc (gc_ctx, sizeof (ovm_extern_t), &extern_class);
  addr->gc.type = T_EXTERNAL;
  addr->ptr = ptr;
  addr->class = xclass;
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

static ovm_var_t *extern_clone (gc_t *gc_ctx, ovm_var_t *var)
{
  void *ptr;

  if (var==NULL || var->gc.type!=T_EXTERNAL)
    return NULL;
  ptr = (*EXTCOPY(var)) (gc_ctx, EXTPTR(var));
  return ovm_extern_new (gc_ctx, ptr, EXTCLASS(var));
}

void register_extern_class (orchids_t *ctx, ovm_extern_class_t *xclass)
{
  strhash_add(ctx->gc_ctx, ctx->xclasses, xclass, xclass->desc);
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

int issdl_cmp(ovm_var_t *var1, ovm_var_t *var2, int dir)
{
  if (var1!=NULL && issdl_types_g[var1->gc.type].cmp!=NULL)
    return (*issdl_types_g[var1->gc.type].cmp) (var1, var2, dir);
  DebugLog(DF_OVM, DS_WARN,
	   "issdl_cmp(): comparison doesn't apply for type '%s'\n",
	   var1==NULL?"null":STRTYPE(var1));
  return CMP_ERROR;
}

ovm_var_t *issdl_add(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  if (var1!=NULL && issdl_types_g[ var1->gc.type ].add!=NULL)
    return (*issdl_types_g[ var1->gc.type ].add) (gc_ctx, var1, var2);
  DebugLog(DF_OVM, DS_WARN,
	   "issdl_add(): addition doesn't apply for type '%s'\n",
	   var1==NULL?"null":STRTYPE(var1));
  return NULL;
}

ovm_var_t *issdl_sub(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  if (var1!=NULL && issdl_types_g[var1->gc.type].sub!=NULL)
    return (*issdl_types_g[ var1->gc.type ].sub) (gc_ctx, var1, var2);
  DebugLog(DF_OVM, DS_WARN,
	   "issdl_sub(): subtraction doesn't apply for type '%s'\n",
	   var1==NULL?"null":STRTYPE(var1));
  return NULL;
}

ovm_var_t *issdl_opp(gc_t *gc_ctx, ovm_var_t *var)
{
  if (var!=NULL && issdl_types_g[var->gc.type].opp!=NULL)
    return (*issdl_types_g[var->gc.type].opp) (gc_ctx, var);
  DebugLog(DF_OVM, DS_WARN,
	   "issdl_opp(): opposite doesn't apply for type '%s'\n",
	   var==NULL?"null":STRTYPE(var));
  return NULL;
}

ovm_var_t *issdl_plus(gc_t *gc_ctx, ovm_var_t *var)
{
  if (var!=NULL && issdl_types_g[var->gc.type].plus!=NULL)
    return (*issdl_types_g[var->gc.type].plus) (gc_ctx, var);
  DebugLog(DF_OVM, DS_WARN,
	   "issdl_plus(): same sign conversion doesn't apply for type '%s'\n",
	   var==NULL?"null":STRTYPE(var));
  return NULL;
}

ovm_var_t *issdl_mul(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  if (var1!=NULL && issdl_types_g[ var1->gc.type ].mul!=NULL)
    return (*issdl_types_g[ var1->gc.type ].mul) (gc_ctx, var1, var2);
  DebugLog(DF_OVM, DS_WARN,
	   "issdl_mul(): multiplication doesn't apply for type '%s'\n",
	   var1==NULL?"null":STRTYPE(var1));
  return NULL;
}

ovm_var_t *issdl_div(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  if (var1!=NULL && issdl_types_g[ var1->gc.type ].div!=NULL)
    return (*issdl_types_g[ var1->gc.type ].div) (gc_ctx, var1, var2);
  DebugLog(DF_OVM, DS_WARN,
	   "issdl_div(): division doesn't apply for type '%s'\n",
	   var1==NULL?"null":STRTYPE(var1));
  return NULL;
}

ovm_var_t *issdl_mod(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  if (var1!=NULL && issdl_types_g[ var1->gc.type ].mod!=NULL)
    return (*issdl_types_g[ var1->gc.type ].mod) (gc_ctx, var1, var2);
  DebugLog(DF_OVM, DS_WARN,
	   "issdl_mod(): modulo doesn't apply for type '%s'\n",
	   var1==NULL?"null":STRTYPE(var1));
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
  return var;
}

ovm_var_t *issdl_and(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  if (var1!=NULL && issdl_types_g[ var1->gc.type ].and!=NULL)
    return (*issdl_types_g[ var1->gc.type ].and) (gc_ctx, var1, var2);
  DebugLog(DF_OVM, DS_WARN,
	   "issdl_and(): bitwise and doesn't apply for type '%s'\n",
	   var1==NULL?"null":STRTYPE(var1));
  return NULL;
}

ovm_var_t *issdl_or(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  if (var1!=NULL && issdl_types_g[ var1->gc.type ].or!=NULL)
    return (*issdl_types_g[ var1->gc.type ].or) (gc_ctx, var1, var2);
  DebugLog(DF_OVM, DS_WARN,
	   "issdl_and(): bitwise inclusive or doesn't apply for type '%s'\n",
	   var1==NULL?"null":STRTYPE(var1));
  return NULL;
}

ovm_var_t *issdl_xor(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2)
{
  if (var1!=NULL && issdl_types_g[ var1->gc.type ].xor!=NULL)
    return (*issdl_types_g[ var1->gc.type ].xor) (gc_ctx, var1, var2);
  DebugLog(DF_OVM, DS_WARN,
	   "issdl_and(): bitwise exclusive or doesn't apply for type '%s'\n",
	   var1==NULL?"null":STRTYPE(var1));
  return NULL;
}

ovm_var_t *issdl_not(gc_t *gc_ctx, ovm_var_t *var)
{
  if (var!=NULL && issdl_types_g[var->gc.type].not!=NULL)
    return (*issdl_types_g[var->gc.type].not) (gc_ctx, var);
  DebugLog(DF_OVM, DS_WARN,
	   "issdl_not(): bitwise complement doesn't apply for type '%s'\n",
	   var==NULL?"null":STRTYPE(var));
  return NULL;
}

/* Display functions */

void fprintf_ovm_var(FILE *fp, ovm_var_t *val)
{
  int i; /* for STRINGs */
  char asc_time[32]; /* for date conversions */
  struct hostent *hptr; /* for IPV4ADDR */
  char **pptr; /* for IPV4ADDR */
  char dst[INET6_ADDRSTRLEN];
  int c;

  if (val==NULL)
    fprintf(fp, "(null)\n");
  /* display data */
  else switch (val->gc.type)
	 {
	 case T_INT:
	   fprintf(fp, "%li", INT(val));
	   break;
	 case T_UINT:
	   fprintf(fp, "%lu", UINT(val));
	   break;
	 case T_BSTR:
	   flockfile (fp);
	   for (i = 0; i < BSTRLEN(val); i++) {
	     c = BSTR(val)[i];
	     if (isprint(c))
	       putc_unlocked (c, fp);
	     else
	       putc_unlocked ('.', fp);
	   }
	   funlockfile (fp);
	   break;
	 case T_VBSTR:
	   flockfile (fp);
	   for (i = 0; i < VBSTRLEN(val); i++) {
	     c = VBSTR(val)[i];
	     if (isprint(c))
	       putc_unlocked (c, fp);
	     else
	       putc_unlocked ('.', fp);
	   }
	   funlockfile (fp);
	   break;
	 case T_STR:
	   flockfile (fp);
	   for (i = 0; i < STRLEN(val); i++) {
	     c = STR(val)[i];
	     if (isprint(c))
	       putc_unlocked (c, fp);
	     else
	       putc_unlocked ('.', fp);
	   }
	   funlockfile (fp);
	   break;
	 case T_VSTR:
	   flockfile (fp);
	   for (i = 0; i < VSTRLEN(val); i++) {
	     c = VSTR(val)[i];
	     if (isprint(c))
	       putc_unlocked (c, fp);
	     else
	       putc_unlocked ('.', fp);
	   }
	   funlockfile (fp);
	   break;
	 case T_CTIME:
	   strftime(asc_time, 32,
		    "%a %b %d %H:%M:%S %Y", gmtime(&CTIME(val)));
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
           (void) inet_ntop (AF_INET6, &IPV6(val), dst, sizeof(dst));
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
		    gmtime(&TIMEVAL(val).tv_sec));
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
	   fprintf(fp, "<type %i> doesn't support display", val->gc.type);
	   break;
	 }
}

void fprintf_env (FILE *fp, const orchids_t *ctx, rule_t *rule, ovm_var_t *env)
{
  ovm_var_t *val;
  int i, n;

  n = rule->dynamic_env_sz;
  for (i=0; i<n; i++)
    {
      val = ovm_read_value (env, i);
      if (val!=NULL)
	{
	  fprintf (fp, "* %s = ", rule->var_name[i]);
	  fprintf_ovm_var (fp, val);
	  fprintf (fp, "\n");
	}
    }
}

void fprintf_issdl_val(FILE *fp, const orchids_t *ctx, ovm_var_t *val)
{
  int i; /* for STRINGs */
  char asc_time[32]; /* for date conversions */
  struct hostent *hptr; /* for IPV4ADDR */
  char **pptr; /* for IPV4ADDR */
  char dst[INET6_ADDRSTRLEN];
  int c;

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
      flockfile (fp);
      for (i = 0; i < BSTRLEN(val); i++) {
	c = BSTR(val)[i];
        if (isprint(c))
          putc_unlocked(c, fp);
        else
          putc_unlocked('.', fp);
      }
      funlockfile (fp);
      fputs("\"\n", fp);
      break;
    case T_VBSTR:
      fprintf(fp, "vbstr[%zd]: \"", VBSTRLEN(val));
      flockfile (fp);
      for (i = 0; i < VBSTRLEN(val); i++) {
	c = VBSTR(val)[i];
        if (isprint(c))
          putc_unlocked (c, fp);
        else
          putc_unlocked ('.', fp);
      }
      funlockfile (fp);
      fputs("\"\n", fp);
      break;
    case T_STR:
      fprintf(fp, "str[%zd]: \"", STRLEN(val));
      flockfile (fp);
      for (i = 0; i < STRLEN(val); i++)
	{
	  c = STR(val)[i];
	  putc_unlocked (c, fp);
	}
      funlockfile (fp);
      fputs("\"\n", fp);
      break;
    case T_VSTR:
      fprintf(fp, "vstr[%zd]: \"", VSTRLEN(val));
      flockfile (fp);
      for (i = 0; i < VSTRLEN(val); i++)
	{
	  c = VSTR(val)[i];
	  putc_unlocked (c, fp);
	}
      funlockfile (fp);
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
      (void) inet_ntop (AF_INET6, &IPV6(val), dst, sizeof(dst));
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

static void issdl_print_string (orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *param;
  char *s;
  size_t len, i;
  int c;

  DebugLog(DF_OVM, DS_DEBUG, "issdl_print_string()\n");
  param = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  s = NULL;
  len = 0;
  if (param!=NULL)
    switch (TYPE(param))
      {
      case T_STR:
	s = STR(param); len = STRLEN(param); break;
      case T_VSTR:
	s = VSTR(param); len = VSTRLEN(param); break;
      default:
	DebugLog(DF_OVM, DS_DEBUG, "parameter error\n");
	break;
      }
  flockfile (stdout);
  for (i = 0; i < len; i++)
    {
      c = s[i];
      putc_unlocked(c, stdout);
    }
  funlockfile (stdout);
  fflush (stdout);
  STACK_DROP(ctx->ovm_stack, 1);
  PUSH_RETURN_TRUE(ctx);
}


#ifdef OBSOLETE
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

  fprintf(stdout, ">>>> rule: %s <<<<<\n", state->pid->rule->name);
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
#endif

#if 0
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
#endif

static void issdl_shutdown(orchids_t *ctx, state_instance_t *state)
{
  DebugLog(DF_OVM, DS_DEBUG, "issdl_shutdown()\n");
  fprintf(stdout, "explicit shutdown on '%s:%s' request\n",
          state->pid->rule->name, state->q->name);
  exit(EXIT_SUCCESS);
}

#ifdef OBSOLETE
static void issdl_dumppathtree(orchids_t *ctx, state_instance_t *state)
{
  DebugLog(DF_OVM, DS_DEBUG, "issdl_dumppathtree()\n");
  fprintf_rule_instance_dot(stdout, state->rule_instance,
                            DOT_RETRIGLIST, ctx->new_qh, 100);
  /* XXX: hard-coded limit */
  PUSH_RETURN_TRUE(ctx);
}
#endif

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

#ifdef OBSOLETE
static void issdl_stats(orchids_t *ctx, state_instance_t *state)
{
  fprintf_orchids_stats(stdout, ctx);
  PUSH_RETURN_TRUE(ctx);
}
#endif

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

static void issdl_str_from_uint(orchids_t *ctx, state_instance_t *state)
{
  char buff[64];
  ovm_var_t *str;
  ovm_var_t *i;
  size_t len;

  i = POP_VALUE(ctx);
  if (i!=NULL && TYPE(i)==T_UINT)
    {
      snprintf(buff, sizeof(buff), "%lu", UINT(i));
      len = strlen(buff);
      str = ovm_str_new(ctx->gc_ctx, len);
      memcpy(STR(str), buff, len);
      PUSH_VALUE(ctx,str);
    }
  else
    {
      DebugLog(DF_OVM, DS_DEBUG, "issdl_str_from_uint(): param error\n");
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
    (void) inet_ntop(AF_INET6, &IPV6(i), buff, sizeof(buff));
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

static void issdl_ipv4_from_ipv6(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *addr4;
  ovm_var_t *addr6;
  size_t len = sizeof(in_addr_t); 
  uint8_t buff[len];
  
  addr6  = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);

  if ((addr6 == NULL) || (addr6->gc.type != T_IPV6))
    {
      DebugLog(DF_OVM, DS_DEBUG, "issdl_ipv4_from_ipv6(): param error\n");
      STACK_DROP(ctx->ovm_stack, 1);
      PUSH_VALUE(ctx, NULL);
    }
  else
    { 
      int index = sizeof(struct in6_addr) - len;
      int i;
      for (i = 0; i < len ; i++)
        buff[i] = *(IPV6(addr6).s6_addr + index + i); 

      addr4 = ovm_ipv4_new(ctx->gc_ctx);
      memcpy(&IPV4(addr4), buff, len);
      STACK_DROP(ctx->ovm_stack, 1);
      PUSH_VALUE(ctx, addr4);
    }
}



static void issdl_ipv6_from_ipv4(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *addr4;
  ovm_var_t *addr6;
  size_t len = sizeof(struct in6_addr); 
  uint8_t buff[len];

  addr4  = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);

  if ((addr4 == NULL) || (addr4->gc.type != T_IPV4))
    {
      DebugLog(DF_OVM, DS_DEBUG, "issdl_ipv6_from_ipv4(): param error\n");
      STACK_DROP(ctx->ovm_stack, 1);
      PUSH_VALUE(ctx, NULL);
    }
  else
    {
      memset(buff, 0, 10);
      memset(&buff[10], 0xff, 2);
      memcpy(&buff[12], &IPV4(addr4), 4); 

      addr6 = ovm_ipv6_new(ctx->gc_ctx); 
      memcpy(&IPV6(addr6), buff, len);
      STACK_DROP(ctx->ovm_stack, 1);
      PUSH_VALUE(ctx, addr6);
    }
}

char *orchids_atoui (char *s, size_t len, unsigned long *result)
{
  char *end = s + len;
  unsigned int i = 0;
  char c;

  while ((s < end) && isspace(*s))
    s++;

  c = *s;
  if (c=='0') // octal or hex
    {
      c = *++s;
      if (c=='x') // hex
        {
          int j;

	  s++;
          while (s<end && (c = *s, isxdigit (c)))
            {
              if (isdigit (c))
        	j = ((int)c) - '0';
              else j = (((int)c) - 'A' + 10) & 0x1f;
              i = 16*i + j;
              s++;
            }
        }
      else // octal
        while (s<end && (c = *s, isdigit (c) && c<'8'))
          {
            i = 8*i + (((int)c) - '0');
            s++;
          }
    }
  else while (s<end && (c = *s, isdigit (c)))
	 {
	   i = 10*i + (((int)c) - '0');
	   s++;
	 }
  *result = i;
  return s;
}

char *orchids_atoi (char *str, size_t len, long *result)
{
  char *end = str + len;
  char *s;
  int negate = 0;
  unsigned long ures;

  while ((str < end) && isspace(*str)) 
    str++;
  if (str<end)
    switch (*str)
      {
      case '-': negate = 1; /*FALLTHROUGH*/
      case '+': str++; break;
      }
  s = orchids_atoui(str, len, &ures);
  *result = negate?-(long)ures:(long)ures;
  return s;
}

static void issdl_int_from_str(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *str;
  ovm_var_t *i;
  long n;

  str = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  if (str!=NULL && str->gc.type == T_STR)
    {
      (void) orchids_atoi(STR(str), STRLEN(str), &n);
      i = ovm_int_new(ctx->gc_ctx, n);
      STACK_DROP(ctx->ovm_stack, 1);
      PUSH_VALUE(ctx,i);
    }
  else if (str && str->gc.type == T_VSTR)
    {
      (void) orchids_atoi(VSTR(str), VSTRLEN(str), &n);
      i = ovm_int_new(ctx->gc_ctx, n);
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

static void issdl_uint_from_str(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *str;
  ovm_var_t *i;
  unsigned long n;

  str = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  if (str!=NULL && str->gc.type == T_STR)
    {
      (void) orchids_atoui(STR(str), STRLEN(str), &n);
      i = ovm_uint_new(ctx->gc_ctx, n);
      STACK_DROP(ctx->ovm_stack, 1);
      PUSH_VALUE(ctx,i);
    }
  else if (str && str->gc.type == T_VSTR)
    {
      (void) orchids_atoui(VSTR(str), VSTRLEN(str), &n);
      i = ovm_uint_new(ctx->gc_ctx, n);
      STACK_DROP(ctx->ovm_stack, 1);
      PUSH_VALUE(ctx,i);
    }
  else
    {
      DebugLog(DF_OVM, DS_DEBUG, "issdl_uint_from_str(): param error\n");
      STACK_DROP(ctx->ovm_stack, 1);
      PUSH_VALUE(ctx, NULL);
    }
}

static void issdl_int_from_uint(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *ui;
  ovm_var_t *i;
  long n;

  ui = POP_VALUE(ctx);
  if (ui != NULL && TYPE(ui) == T_UINT)
    {
      if (UINT(ui) <= LONG_MAX)
	n = UINT(ui);
      else n = LONG_MAX;
    
      i = ovm_int_new(ctx->gc_ctx, n);
      PUSH_VALUE(ctx, i);  
    }
  else
    {
      DebugLog(DF_OVM, DS_DEBUG, "issdl_int_from_uint(): param error\n");
      PUSH_VALUE(ctx, NULL);
    }
}

static void issdl_uint_from_int(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *i;
  ovm_var_t *ui;
  unsigned long n;

  i = POP_VALUE(ctx);
  if (i != NULL && TYPE(i) == T_INT)
    {
      if (INT(i) >= 0)
	n = INT(i);
      else n = 0;

      ui = ovm_uint_new(ctx->gc_ctx, n);
      PUSH_VALUE(ctx, ui);   
    }
  else
    {
      DebugLog(DF_OVM, DS_DEBUG, "issdl_uint_from_int(): param error\n");
      PUSH_VALUE(ctx, NULL);
    }
}

static void issdl_int_from_float(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *x;
  ovm_var_t *i;
  long n;

  x = POP_VALUE(ctx);
  if (x!=NULL && TYPE(x)==T_FLOAT)
    {
      if (FLOAT(x) >= (double)LONG_MAX)
	n = LONG_MAX;
      else if (FLOAT(x) <= (double)LONG_MIN)
	n = LONG_MIN;
      else n = (long)FLOAT(x);

      i = ovm_int_new(ctx->gc_ctx, n);
      PUSH_VALUE(ctx, i);
    }
  else
    {
      DebugLog(DF_OVM, DS_DEBUG, "issdl_int_from_float(): param error\n");
      PUSH_VALUE(ctx, NULL);
    }
}

static void issdl_uint_from_float(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *x;
  ovm_var_t *ui;
  unsigned long n;

  x = POP_VALUE(ctx);
  if (x!=NULL && TYPE(x)==T_FLOAT)
    {
      if (FLOAT(x) >= (double)ULONG_MAX)
	n = ULONG_MAX;
      else if (FLOAT(x) <= 0.0)
	n = 0;
      else n = (unsigned long)FLOAT(x);

      ui = ovm_uint_new(ctx->gc_ctx, n);
      PUSH_VALUE(ctx, ui);
    }
  else
    {
      DebugLog(DF_OVM, DS_DEBUG, "issdl_uint_from_float(): param error\n");
      PUSH_VALUE(ctx, NULL);
    }
}

static void issdl_float_from_int(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *i;
  ovm_var_t *x;

  i = POP_VALUE(ctx);
  if (i!=NULL && TYPE(i)==T_INT)
    {
      x = ovm_float_new(ctx->gc_ctx, (double)INT(i));
      PUSH_VALUE(ctx, x);
    }
  else
    {
      DebugLog(DF_OVM, DS_DEBUG, "issdl_float_from_int(): param error\n");
      PUSH_VALUE(ctx, NULL);
    } 
}

static void issdl_float_from_uint(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *ui;
  ovm_var_t *x;

  ui = POP_VALUE(ctx);
  if (ui!=NULL && TYPE(ui)==T_UINT)
    {
      x = ovm_float_new(ctx->gc_ctx, (double)UINT(ui));
      PUSH_VALUE(ctx, x);
    }
  else
    {
      DebugLog(DF_OVM, DS_DEBUG, "issdl_float_from_uint(): param error\n");
      PUSH_VALUE(ctx, NULL);
    }
}

char *time_convert_idmef(char *str, char *end, time_t *res)
{ /* IDMEF time format is %Y-%m-%dT%H:%M:%S%z
     parse it in a fault-tolerant way.
  */
  struct tm tm = { 0, };
  unsigned long i;
  int negate = 0;
  int looks_ok = 0;
  int looks_wrong = 0;
  char *str0 = str;

  str = orchids_atoui (str, end-str, &i);
  tm.tm_year = i - 1900; /* tm_year is number of years since 1900 */
  while (str < end && isspace(*str)) str++; /* skip spaces */
  if (str < end && str[0]=='-')
    looks_ok++;
  while (str < end && !isdigit(*str)) str++; /* skip '-' */
  str = orchids_atoui (str, end-str, &i);
  tm.tm_mon = i-1; /* should be between 0 and 11, while %m month is between 1 and 12 */
  if (i==0 || i>12)
    looks_wrong++;
  while (str < end && isspace(*str)) str++; /* skip spaces */
  if (str < end && str[0]=='-')
    looks_ok++;
  while (str < end && isspace(*str)) str++; /* skip spaces */
  if (str < end && str[0]=='-')
    looks_ok++;
  while (str < end && !isdigit(*str)) str++; /* skip '-' */
  str = orchids_atoui (str, end-str, &i);
  tm.tm_mday = i; /* should be between 1 and 31 */
  if (i==0 || i>31)
    looks_wrong++;
  while (str < end && isspace(*str)) str++; /* skip spaces */
  if (str < end && str[0]=='T')
    looks_ok++;
  while (str < end && !isdigit(*str)) str++; /* skip 'T' */
  str = orchids_atoui (str, end-str, &i);
  tm.tm_hour = i; /* should be between 0 and 23 */
  if (i>23)
    looks_wrong++;
  while (str < end && isspace(*str)) str++; /* skip spaces */
  if (str < end && str[0]==':')
    looks_ok++;
  while (str < end && !isdigit(*str)) str++; /* skip ':' */
  str = orchids_atoui (str, end-str, &i);
  tm.tm_min = i; /* should be between 0 and 59 */
  if (i>59)
    looks_wrong++;
  while (str < end && isspace(*str)) str++; /* skip spaces */
  if (str < end && str[0]==':')
    looks_ok++;
  while (str < end && !isdigit(*str)) str++; /* skip ':' */
  str = orchids_atoui (str, end-str, &i);
  tm.tm_sec = i; /* should be between 0 and 60 */
  if (i>60)
    looks_wrong++;
  while (str < end && isspace(*str)) str++;
  if (str < end)
    switch (*str)
      {
      case '-': negate = 1; /*FALLTHROUGH*/
      case '+': str++;
	/* expect timezone, as HHMM */
	if (str+4>end)
	  break;
	if (!isdigit(str[0]) || !isdigit(str[1]) ||
	    !isdigit(str[2]) || !isdigit(str[3]))
	  break;
	i = 60* (
		 60*(10*(str[0]-'0') + (str[1]-'0')) +
		 10*(str[2]-'0') + (str[3]-'0')
		 );
	tm.tm_gmtoff = negate?(-i):i;
	break;
      }
  if (looks_wrong)
    return str0;
  if (!looks_ok)
    return str0;
  *res = timegm(&tm);
  return str;
}

char *time_convert(char *str, char *end, struct timeval *tv)
{
  long sec = 0;
  double d = 0.0;
  double mask = 1.0;
  char c;
  char *newstr;
  time_t t;

  newstr = time_convert_idmef (str, end, &t);
  if (newstr!=str) /* should be an IDMEF time */
    {
      tv->tv_sec = t;
      tv->tv_usec = 0;
      return newstr;
    }
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

char *orchids_atof(char *str, size_t str_sz, double *result)
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
    {
      *result = x;
      return end;
    }
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
  *result = neg?(-x):x;
  return s;
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
  s = orchids_atoi (s, end-s, &expo);
  x = ldexp10 (x, expo);
  goto neg;
}


static void issdl_float_from_str(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *str;
  ovm_var_t *i;
  double x;

  str = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  if (str!=NULL && TYPE(str) == T_STR)
    {
      (void) orchids_atof(STR(str), STRLEN(str), &x);
      i = ovm_float_new(ctx->gc_ctx, x);
      STACK_DROP(ctx->ovm_stack, 1);
      PUSH_VALUE(ctx,i);
    }
  else if (str && TYPE(str) == T_VSTR)
    {
      (void) orchids_atof(VSTR(str), VSTRLEN(str), &x);
      i = ovm_float_new(ctx->gc_ctx, x);
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

#ifdef OBSOLETE
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
#endif

#ifdef OBSOLETE
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
#endif

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

	flockfile (ftmp);
	for (i=0; i<body_len; i++)
	  {
	    putc_unlocked (body_str[i], ftmp);
	  }
	funlockfile (ftmp);
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


static void issdl_sendmail_report(orchids_t *ctx, state_instance_t *state)
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
	DebugLog(DF_OVM, DS_DEBUG, "issdl_sendmail_report(): param error: from\n");
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
	DebugLog(DF_OVM, DS_DEBUG, "issdl_sendmail_report(): param error: to\n");
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
	DebugLog(DF_OVM, DS_DEBUG, "issdl_sendmail_report(): param error: subject\n");
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

      flockfile (ftmp);
      for (i=0; i<body_len; i++)
	{
	  putc_unlocked (body_str[i], ftmp);
	}
      funlockfile (ftmp);
    }
  /* report */
  fprintf(ftmp, "Report for rule %s, at state %s:\n\n", state->pid->rule->name,
	  state->q->name);
  fprintf_env (ftmp, ctx, state->pid->rule, state->env);
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

#if 0
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
#endif

/* Compute the number of different bits between two variables of
   the same type and size (binary distance). */
static void
issdl_bindist(orchids_t *ctx, state_instance_t *state)
{
  static unsigned long bitcnt_tbl[] = { /* Precomputed table of 1-bit in each bytes */
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
  unsigned long dist;
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
      d = ovm_uint_new(ctx->gc_ctx, dist);
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
  unsigned long dist;
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
      d = ovm_uint_new(ctx->gc_ctx, dist);
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
  if (str!=NULL && (TYPE(str) == T_STR || TYPE(str) == T_VSTR))
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
  ovm_var_t *val;

  val = POP_VALUE(ctx);
  if (IS_NULL(val))
    PUSH_RETURN_FALSE(ctx);
  else
    PUSH_RETURN_TRUE(ctx);
}

#ifdef OBSOLETE
// superseded by plain old '-'
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
#endif

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


static void env_bind_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  env_bind_t *bind = (env_bind_t *)p;

  GC_TOUCH (gc_ctx, bind->val);
}

static int env_bind_traverse (gc_traverse_ctx_t *gtc, gc_header_t *p,
			      void *data)
{
  env_bind_t *bind = (env_bind_t *)p;
  int err = 0;

  err = (*gtc->do_subfield) (gtc, (gc_header_t *)bind->val, data);
  return err;
}

static int env_bind_save (save_ctx_t *sctx, gc_header_t *p)
{
  env_bind_t *bind = (env_bind_t *)p;
  int err;

  err = save_int32 (sctx, bind->var);
  if (err) return err;
  err = save_gc_struct (sctx, (gc_header_t *)bind->val);
  return err;
}

static gc_class_t env_bind_class;

static gc_header_t *env_bind_restore (restore_ctx_t *rctx)
{
  int err;
  int32_t var;
  ovm_var_t *val;
  env_bind_t *bind;

  GC_START(rctx->gc_ctx, 1);
  err = restore_int32 (rctx, &var);
  if (err) goto errlab;
  val = (ovm_var_t *)restore_gc_struct (rctx);
  if (val==NULL && errno!=0)
    goto errlab;
  GC_UPDATE(rctx->gc_ctx, 0, val);
  bind = gc_alloc (rctx->gc_ctx, sizeof(env_bind_t), &env_bind_class);
  bind->gc.type = T_BIND;
  bind->var = var;
  GC_TOUCH (rctx->gc_ctx, bind->val = val);
  goto normal;
 errlab:
  errno = err;
  bind = NULL;
 normal:
  GC_END(rctx->gc_ctx);
  return (gc_header_t *)bind;
}

static gc_class_t env_bind_class = {
  GC_ID('b','i','n','d'),
  env_bind_mark_subfields,
  NULL,
  env_bind_traverse,
  env_bind_save,
  env_bind_restore
};

static void env_split_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  env_split_t *split = (env_split_t *)p;

  GC_TOUCH (gc_ctx, split->left);
  GC_TOUCH (gc_ctx, split->right);
}

static int env_split_traverse (gc_traverse_ctx_t *gtc, gc_header_t *p,
			      void *data)
{
  env_split_t *split = (env_split_t *)p;
  int err = 0;

  err = (*gtc->do_subfield) (gtc, (gc_header_t *)split->left, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *)split->right, data);
  return err;
}

static int env_split_save (save_ctx_t *sctx, gc_header_t *p)
{
  env_split_t *split = (env_split_t *)p;
  int err;

  err = save_gc_struct (sctx, (gc_header_t *)split->left);
  if (err) return err;
  err = save_gc_struct (sctx, (gc_header_t *)split->right);
  return err;
}

static gc_class_t env_split_class;

static gc_header_t *env_split_restore (restore_ctx_t *rctx)
{
  gc_t *gc_ctx = rctx->gc_ctx;
  ovm_var_t *left, *right;
  env_split_t *split;

  GC_START (gc_ctx, 2);
  left = (ovm_var_t *)restore_gc_struct (rctx);
  if (left==NULL && errno!=0)
    goto errlab;
  GC_UPDATE (gc_ctx, 0, left);
  right = (ovm_var_t *)restore_gc_struct (rctx);
  if (right==NULL && errno!=0)
    goto errlab;
  GC_UPDATE (gc_ctx, 1, right);
  split = gc_alloc (gc_ctx, sizeof(env_split_t), &env_split_class);
  split->gc.type = T_SPLIT;
  GC_TOUCH (gc_ctx, split->left = left);
  GC_TOUCH (gc_ctx, split->right = right);
  goto normal;
 errlab:
  split = NULL;
 normal:
  GC_END (gc_ctx);
  return (gc_header_t *)split;
}

static gc_class_t env_split_class = {
  GC_ID('s','p','l','t'),
  env_split_mark_subfields,
  NULL,
  env_split_traverse,
  env_split_save,
  env_split_restore
};

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

ovm_var_t *ovm_write_value (gc_t *gc_ctx, ovm_var_t *env, unsigned long var, ovm_var_t *val)
{
  ovm_var_t *branch[32]; /*depth of environment is at most 32,
			   since indexed by int32_t's in general
			   (here by a bytecode_t, but see
			   dynamic_env_sz in rule_s, orchids.h) */
  ovm_var_t **branchp;
  bytecode_t mask, var2;
  ovm_var_t *env_left, *env_right;

  for (mask = 1L, branchp=branch; env!=NULL; mask <<= 1)
    if (TYPE(env)==T_BIND)
      break;
    else if (var & mask)
      {
	*branchp++ = ((env_split_t *)env)->left;
	env = ((env_split_t *)env)->right;
      }
    else
      {
	*branchp++ = ((env_split_t *)env)->right;
	env = ((env_split_t *)env)->left;
      }
  if (env!=NULL)
    {
      var2 = ((env_bind_t *)env)->var;
      if (var2!=var)
	for (;; mask<<=1)
	  {
	    if ((var2 & mask)==(var & mask))
	      *branchp++ = NULL;
	    else
	      {
		*branchp++ = env;
		mask <<= 1;
		break;
	      }
	  }
    }
  GC_START(gc_ctx, 1);
  env = gc_alloc (gc_ctx, sizeof(env_bind_t), &env_bind_class);
  env->gc.type = T_BIND;
  ((env_bind_t *)env)->var = var;
  GC_TOUCH (gc_ctx, ((env_bind_t *)env)->val = val);
  GC_UPDATE(gc_ctx, 0, env);
  for (; branchp > branch; )
    {
      mask >>= 1;
      if (var & mask)
	{
	  env_left = *--branchp;
	  env_right = env;
	}
      else
	{
	  env_left = env;
	  env_right = *--branchp;
	}
      env = gc_alloc (gc_ctx, sizeof(env_split_t), &env_split_class);
      env->gc.type = T_SPLIT;
      GC_TOUCH (gc_ctx, ((env_split_t *)env)->left = env_left);
      GC_TOUCH (gc_ctx, ((env_split_t *)env)->right = env_right);
      GC_UPDATE(gc_ctx, 0, env);
    }
  GC_END(gc_ctx);
  return env;
}

ovm_var_t *ovm_release_value (gc_t *gc_ctx, ovm_var_t *env, unsigned long var)
{
  ovm_var_t *branch[32]; /*depth of environment is at most 32,
			   since indexed by int32_t's in general
			   (here by a bytecode_t, but see
			   dynamic_env_sz in rule_s, orchids.h) */
  ovm_var_t **branchp;
  bytecode_t mask;
  ovm_var_t *env_left, *env_right, *env0, *otherenv;

  GC_START(gc_ctx, 1);
  env0 = env;
  for (mask = 1L, branchp=branch; env!=NULL; mask <<= 1)
    if (TYPE(env)==T_BIND)
      break;
    else if (var & mask)
      {
	*branchp++ = ((env_split_t *)env)->left;
	env = ((env_split_t *)env)->right;
      }
    else
      {
	*branchp++ = ((env_split_t *)env)->right;
	env = ((env_split_t *)env)->left;
      }
  if (env==NULL || ((env_bind_t *)env)->var!=var)
    env = env0;
  else 
    {
  env = NULL;
  for (; branchp > branch; )
    {
      mask >>= 1;
      otherenv = *--branchp;
      if (otherenv==NULL)
	{
	  if (env==NULL || TYPE(env)==T_BIND)
	    continue;
	}
      else if (env==NULL)
	{
	  if (TYPE (otherenv)==T_BIND)
	    continue;
	}
      if (var & mask)
	{
	  env_left = otherenv;
	  env_right = env;
	}
      else
	{
	  env_left = env;
	  env_right = otherenv;
	}
      env = gc_alloc (gc_ctx, sizeof(env_split_t), &env_split_class);
      env->gc.type = T_SPLIT;
      GC_TOUCH (gc_ctx, ((env_split_t *)env)->left = env_left);
      GC_TOUCH (gc_ctx, ((env_split_t *)env)->right = env_right);
      GC_UPDATE(gc_ctx, 0, env);
    }
  } 
  GC_END(gc_ctx);
  return env;
}

/**
 ** Table of built-in function of the Orchids language.  This table is
 ** used at startup by the function register_core_functions() to
 ** register them.
 **/
static type_t *null_sig[] = { NULL };
static type_t **null_sigs[] = { null_sig, NULL };

static type_t *int_sig[] = { &t_int };
static type_t **int_sigs[] = { int_sig, NULL };

static type_t *int_of_any_sig[] = { &t_int, &t_any };
static type_t **int_of_any_sigs[] = { int_of_any_sig, NULL };

#if 0
static type_t *int_of_int_sig[] = { &t_int, &t_int };
static type_t **int_of_int_sigs[] = { int_of_int_sig, NULL };
#endif

static type_t *int_of_str_sig[] = { &t_int, &t_str };
static type_t **int_of_str_sigs[] = { int_of_str_sig, NULL };

static type_t *uint_of_str_sig[] = { &t_uint, &t_str };
static type_t **uint_of_str_sigs[] = { uint_of_str_sig, NULL };

static type_t *str_of_int_sig[] = { &t_str, &t_int };
static type_t **str_of_int_sigs[] = { str_of_int_sig, NULL };

static type_t *str_of_uint_sig[] = { &t_str, &t_uint };
static type_t **str_of_uint_sigs[] = { str_of_uint_sig, NULL };

static type_t *float_of_str_sig[] = { &t_float, &t_str };
static type_t **float_of_str_sigs[] = { float_of_str_sig, NULL };

static type_t *str_of_float_sig[] = { &t_str, &t_float };
static type_t **str_of_float_sigs[] = { str_of_float_sig, NULL };

static type_t *str_of_ipv4_sig[] = { &t_str, &t_ipv4 };
static type_t **str_of_ipv4_sigs[] = { str_of_ipv4_sig, NULL };

static type_t *str_of_ipv6_sig[] = { &t_str, &t_ipv6 };
static type_t **str_of_ipv6_sigs[] = { str_of_ipv6_sig, NULL };

static type_t *sendmail_sig[] = { &t_int, &t_str, &t_str, &t_str, &t_str };
static type_t **sendmail_sigs[] = { sendmail_sig, NULL };

static type_t *int_cmp_sig[] = { &t_uint, &t_int, &t_int };
static type_t *uint_cmp_sig[] = { &t_uint, &t_uint, &t_uint };
static type_t *ipv4_cmp_sig[] = { &t_uint, &t_ipv4, &t_ipv4 };
static type_t *ipv6_cmp_sig[] = { &t_uint, &t_ipv6, &t_ipv6 };
static type_t *str_cmp_sig[] = { &t_uint, &t_str, &t_str };
static type_t *bstr_cmp_sig[] = { &t_uint, &t_bstr, &t_bstr };
static type_t *ctime_cmp_sig[] = { &t_uint, &t_ctime, &t_ctime };
static type_t *timeval_cmp_sig[] = { &t_uint, &t_timeval, &t_timeval };
static type_t *float_cmp_sig[] = { &t_uint, &t_float, &t_float };
static type_t **cmp_sigs[] = {
  int_cmp_sig, uint_cmp_sig, ipv4_cmp_sig, ipv6_cmp_sig,
  str_cmp_sig, bstr_cmp_sig, ctime_cmp_sig, timeval_cmp_sig,
  float_cmp_sig, NULL
};

static type_t *regex_of_str_sig[] = { &t_regex, &t_str };
static type_t **regex_of_str_sigs[] = { regex_of_str_sig, NULL };

static type_t *str_of_regex_sig[] = { &t_str, &t_regex };
static type_t **str_of_regex_sigs[] = { str_of_regex_sig, NULL };

#ifdef OBSOLETE
static type_t *difftime_sig[] = { &t_int, &t_ctime, &t_ctime };
static type_t **difftime_sigs[] = { difftime_sig, NULL };
#endif

static type_t *str_of_ctime_sig[] = { &t_str, &t_ctime };
static type_t **str_of_ctime_sigs[] = { str_of_ctime_sig, NULL };

static type_t *str_of_timeval_sig[] = { &t_str, &t_timeval };
static type_t **str_of_timeval_sigs[] = { str_of_timeval_sig, NULL };

static type_t *ctime_of_str_sig[] = { &t_ctime, &t_str };
static type_t **ctime_of_str_sigs[] = { ctime_of_str_sig, NULL };

static type_t *timeval_of_str_sig[] = { &t_timeval, &t_str };
static type_t **timeval_of_str_sigs[] = { timeval_of_str_sig, NULL };

static type_t *ipv4_of_ipv6_sig[] = { &t_ipv4, &t_ipv6 };
static type_t **ipv4_of_ipv6_sigs[] = { ipv4_of_ipv6_sig, NULL };

static type_t *ipv6_of_ipv4_sig[] = { &t_ipv6, &t_ipv4 };
static type_t **ipv6_of_ipv4_sigs[] = { ipv6_of_ipv4_sig, NULL };

static type_t *int_of_uint_sig[] = { &t_int, &t_uint };
static type_t **int_of_uint_sigs[] = { int_of_uint_sig, NULL };

static type_t *uint_of_int_sig[] = { &t_uint, &t_int };
static type_t **uint_of_int_sigs[] = { uint_of_int_sig, NULL };

static type_t *int_of_float_sig[] = { &t_int, &t_float };
static type_t **int_of_float_sigs[] = { int_of_float_sig, NULL };

static type_t *uint_of_float_sig[] = { &t_uint, &t_float };
static type_t **uint_of_float_sigs[] = { uint_of_float_sig, NULL };

static type_t *float_of_int_sig[] = { &t_float, &t_int };
static type_t **float_of_int_sigs[] = { float_of_int_sig, NULL };

static type_t *float_of_uint_sig[] = { &t_float, &t_uint };
static type_t **float_of_uint_sigs[] = { float_of_uint_sig, NULL };

static issdl_function_t issdl_function_g[] = {
  { issdl_noop, 0, "null",
    0, null_sigs,
    m_const,
    "returns null (the undefined object)" },
  { issdl_print, 1, "print",
    1, int_of_any_sigs, /* always returns true, in fact */
    m_const_thrash,
    "display a string (TEST FUNCTION)" },
#ifdef OBSOLETE
  { issdl_dumpstack, 2, "dump_stack",
    0, int_sigs, /* returns 0 or 1, in fact */
    m_random_thrash,
    "dump the stack of the current rule" },
#endif
  { issdl_print_string, 3, "print_string",
    1, int_of_str_sigs,  /* always returns true, in fact */
    m_const_thrash,
    "print a string verbatim" },
#ifdef OBSOLETE
  { issdl_dumppathtree, 4, "dump_dot_pathtree",
    0, int_sigs, /* always returns true, in fact */
    m_const_thrash,
    "dump the rule instance path tree in the GraphViz Dot format"},
#endif
#if 0
  { issdl_drop_event, 5, "drop_event",
    0, int_sigs, /* returns 0 or 1, in fact */
    m_random_thrash,
    "Drop event" },
  { issdl_set_event_level, 6, "set_event_level",
    1, int_of_int_sigs, /* returns 0 or 1, in fact */
    m_random_thrash,
    "Set event level" },
#endif
  { issdl_report, 7, "report",
    0, int_sigs, /* returns 0 or 1, in fact */
    m_random_thrash,
    "generate report" },
  { issdl_shutdown, 8, "shutdown",
    0, int_sigs, /* does not return */
    m_const_thrash,
    "shutdown orchids" },
  { issdl_random, 9, "random",
    0, int_sigs,
    m_random,
    "return a random number" },
  { issdl_system, 10, "system",
    1, int_of_str_sigs,
    m_random_thrash,
    "execute a system command" },
#ifdef OBSOLETE
  { issdl_statsx, 11, "show_stats",
    0, int_sigs, /* always returns true, in fact */
    m_const_thrash,
    "show orchids internal statistics" },
#endif
  { issdl_str_from_int, 12, "str_from_int",
    1, str_of_int_sigs,
    m_unknown_1,
    "convert an integer to a string" },
  { issdl_int_from_str, 13, "int_from_str",
    1, int_of_str_sigs,
    m_unknown_1,
    "convert a string to an integer" },
  { issdl_float_from_str, 14, "float_from_str",
    1, float_of_str_sigs,
    m_unknown_1,
    "convert a string to a float" },
  { issdl_str_from_float, 15, "str_from_float",
    1, str_of_float_sigs,
    m_unknown_1,
    "convert a float to a string" },
  { issdl_str_from_ipv4, 16, "str_from_ipv4",
    1, str_of_ipv4_sigs,
    m_unknown_1,
    "convert an ipv4 address to a string" },
#ifdef OBSOLETE
  { issdl_kill_threads, 17, "kill_threads",
    0, int_sigs, /* always returns true, in fact */
    m_const_thrash,
    "kill threads of a rule instance" },
#endif
#ifdef OBSOLETE
  { issdl_cut, 18, "cut",
    1, int_of_str_sigs, /* returns 0 or 1, in fact */
    m_unknown_1_thrash,
    "special cut" },
#endif
  { issdl_sendmail, 19, "sendmail",
    4, sendmail_sigs,
    /* sendmail (from, to, subject, body), returns 0 or 1, in fact */
    m_random_thrash,
    "Send an email" },
  { issdl_sendmail_report, 20, "sendmail_report",
    4, sendmail_sigs,
    m_random_thrash,
    /* sendmail_report (from, to, subject, body), returns 0 or 1, in fact */
    "Send a report by email" },
  { issdl_bindist, 21, "bitdist",
    2, cmp_sigs,
    m_unknown_2,
    "Number of different bits" },
  { issdl_bytedist, 22, "bytedist",
    2, cmp_sigs,
    m_unknown_2,
    "Number of different bytes" },
  { issdl_vstr_from_regex, 23, "str_from_regex",
    1, str_of_regex_sigs,
    m_unknown_1,
    "Return the source string of a compiled regex" },
  { issdl_regex_from_str, 24, "regex_from_str",
    1, regex_of_str_sigs,
    m_unknown_1,
    "Compile a regex from a string" },
  { issdl_defined, 25, "defined",
    1, int_of_any_sigs, /* returns 0 or 1, in fact */
    m_unknown_1,
    "Return whether a value is defined, i.e., different from NULL" },
#ifdef OBSOLETE
  { issdl_difftime, 26, "difftime",
    2, difftime_sigs,
    "difference expressed in seconds as an int"},
#endif
  { issdl_str_from_time, 27, "str_from_ctime",
    1, str_of_ctime_sigs,
    m_unknown_1,
    "convert a time to a string" },
  { issdl_str_from_timeval, 28, "str_from_timeval",
    1, str_of_timeval_sigs,
    m_unknown_1,
    "convert a timeval to a string" },
  { issdl_time_from_str, 29, "ctime_from_str",
    1, ctime_of_str_sigs,
    m_unknown_1,
    "convert a string to a time" },
  { issdl_timeval_from_str, 30, "timeval_from_str",
    1, timeval_of_str_sigs,
    m_unknown_1,
    "convert a string to a timeval" },
  { issdl_str_from_ipv6, 31, "str_from_ipv6",
    1, str_of_ipv6_sigs,
    m_unknown_1,
    "convert an ipv6 address to a string" },
  { issdl_ipv4_from_ipv6, 32, "ipv4_from_ipv6",
    1, ipv4_of_ipv6_sigs,
    m_unknown_1,
    "convert an ipv6 address to an ipv4 address" },
  { issdl_ipv6_from_ipv4, 33, "ipv6_from_ipv4",
    1, ipv6_of_ipv4_sigs,
    m_unknown_1,
    "convert an ipv4 address to an ipv6 address" },
  { issdl_str_from_uint, 34, "str_from_uint",
    1, str_of_uint_sigs,
    m_unknown_1,
    "convert an unsigned integer to a string" },
  { issdl_uint_from_str, 35, "uint_from_str",
    1, uint_of_str_sigs,
    m_unknown_1,
    "convert a string to an unsigned integer" },
  { issdl_int_from_uint, 36, "int_from_uint",
    1, int_of_uint_sigs,
    m_unknown_1,
    "convert an unsigned integer to an integer"},
  { issdl_uint_from_int, 37, "uint_from_int",
    1, uint_of_int_sigs,
    m_unknown_1,
    "convert an integer to an unsigned integer"},
  { issdl_int_from_float, 38, "int_from_float",
    1, int_of_float_sigs,
    m_unknown_1,
    "convert a float to an integer"},
  { issdl_uint_from_float, 39, "uint_from_float",
    1, uint_of_float_sigs,
    m_unknown_1,
    "convert a float to an unsigned integer"},
  { issdl_float_from_int, 40, "float_from_int",
    1, float_of_int_sigs,
    m_unknown_1,
    "convert an integer to a float"},
  { issdl_float_from_uint, 41, "float_from_uint",
    1, float_of_uint_sigs,
    m_unknown_1,
    "convert an unsigned integer to a float"},
  { NULL, 0, NULL, 0, NULL, NULL }
};

void register_core_functions(orchids_t *ctx)
{
  issdl_function_t *f;

  for (f = issdl_function_g + 1; f->func!=NULL; f++) /* +1: do not register "null" */
    register_lang_function(ctx, f->func, f->name,
			   f->args_nb, (const type_t ***)f->sigs,
			   f->cm, f->desc);
}

void register_lang_function(orchids_t *ctx,
			    ovm_func_t func,
			    const char *name,
			    int arity,
			    const type_t ***sigs,
			    monotony_apply cm,
			    const char *desc)
{
  issdl_function_t *f;
  size_t array_size;

  DebugLog(DF_ENG, DS_INFO,
           "Registering language function %s/%i @ %p\n",
           name, arity, func);
  array_size = (ctx->vm_func_tbl_sz + 1) * sizeof (issdl_function_t);
  ctx->vm_func_tbl = gc_base_realloc (ctx->gc_ctx,
				      ctx->vm_func_tbl, array_size);
  f = &ctx->vm_func_tbl[ctx->vm_func_tbl_sz];
  f->func = func;
  f->id = ctx->vm_func_tbl_sz;
  f->name = (char *)name;
  f->args_nb = arity;
  f->sigs = (type_t ***)sigs;
  f->cm = cm;
  f->desc = (char *)desc;
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

/*********
	  Saving and restoring
 *********/

/* In db.c: */
extern gc_class_t empty_db_class;
extern gc_class_t db_small_table_class;
extern gc_class_t db_tuples_class;
extern gc_class_t db_branch_class;
/* In engine.c: */
extern gc_class_t thread_group_class;
extern gc_class_t state_instance_class;
extern gc_class_t thread_queue_elt_class;
extern gc_class_t thread_queue_class;
/* In orchids_api.c: */
extern gc_class_t field_record_table_class;
extern gc_class_t event_class;
extern gc_class_t field_table_class;
/* In rule_compiler.c: */
extern gc_class_t node_state_class;
extern gc_class_t varset_class;
extern gc_class_t node_trans_class;
extern gc_class_t node_rule_class;
extern gc_class_t node_expr_term_class;
extern gc_class_t node_expr_var_class;
extern gc_class_t node_expr_field_class;
extern gc_class_t node_expr_bin_class;
extern gc_class_t node_expr_mon_class;
extern gc_class_t node_expr_call_class;
extern gc_class_t node_expr_ifstmt_class;
extern gc_class_t node_expr_regsplit_class;
extern gc_class_t type_heap_class;
extern gc_class_t rule_class;

static gc_header_t *nothing_restore (restore_ctx_t *rctx)
{
  return NULL;
}

static gc_class_t nothing_class = {
  GC_ID(' ',' ',' ',' '),
  NULL,
  NULL,
  NULL,
  NULL,
  nothing_restore
};

static gc_header_t *shared_def_restore (restore_ctx_t *rctx)
{
  unsigned long id;
  gc_header_t *p;
  int err;

  GC_START (rctx->gc_ctx, 1);
  err = restore_ulong (rctx, &id);
  if (err) { errno = err; p = NULL; }
  else
    {
      p = hash_get (rctx->shared_hash, &id, sizeof(unsigned long));
      if (p!=NULL) { errno = -2; p = NULL; }
      else
	{
	  p = restore_gc_struct (rctx);
	  if (p==NULL && errno!=0)
	    ;
	  else
	    {
	      GC_UPDATE (rctx->gc_ctx, 0, p);
	      hash_add (rctx->gc_ctx, rctx->shared_hash, p, &id, sizeof(unsigned long));
	    }
	}
    }
  GC_END (rctx->gc_ctx);
  return p;
}

static gc_class_t shared_def_class = {
  GC_ID('s','h','r','d'),
  NULL,
  NULL,
  NULL,
  NULL,
  shared_def_restore
};

static gc_header_t *shared_use_restore (restore_ctx_t *rctx)
{
  unsigned long id;
  gc_header_t *p;
  int err;

  err = restore_ulong (rctx, &id);
  if (err) { errno = err; return NULL; }
  p = hash_get (rctx->shared_hash, &id, sizeof(unsigned long));
  if (p==NULL) { errno = -2; }
  return p;
}

static gc_class_t shared_use_class = {
  GC_ID('s','h','r','u'),
  NULL,
  NULL,
  NULL,
  NULL,
  shared_use_restore
};

static gc_class_t *gc_classes[256] = {
  /* 00 */ NULL, NULL, &int_class, &bstr_class,
  /* 04 */ &vbstr_class, &str_class, &vstr_class, NULL,
  /* 08 */ NULL, &ctime_class, &ipv4_class, &ipv6_class,
  /* 0c */ &timeval_class, &regex_class, &uint_class, &snmpoid_class,
  /* 10 */ &float_class, &event_class, &state_instance_class, &empty_db_class, 
  /* 14 */ &db_branch_class, &db_tuples_class, &extern_class, NULL,
  /* 18 */ NULL, NULL, NULL, NULL,
  /* 1c */ NULL, NULL, NULL, NULL,
  /* 20 */ NULL, NULL, NULL, NULL,
  /* 24 */ NULL, NULL, NULL, NULL,
  /* 28 */ NULL, NULL, NULL, NULL,
  /* 2c */ NULL, NULL, NULL, NULL,
  /* 30 */ NULL, NULL, NULL, NULL,
  /* 34 */ NULL, NULL, NULL, NULL,
  /* 38 */ NULL, NULL, NULL, NULL,
  /* 3c */ NULL, NULL, NULL, NULL,
  /* 40 */ NULL, NULL, NULL, NULL,
  /* 44 */ NULL, NULL, NULL, NULL,
  /* 48 */ NULL, NULL, NULL, NULL,
  /* 4c */ NULL, NULL, NULL, NULL,
  /* 50 */ NULL, NULL, NULL, NULL,
  /* 54 */ NULL, NULL, NULL, NULL,
  /* 58 */ NULL, NULL, NULL, NULL,
  /* 5c */ NULL, NULL, NULL, NULL,
  /* 60 */ NULL, NULL, NULL, NULL,
  /* 64 */ NULL, NULL, NULL, NULL,
  /* 68 */ NULL, NULL, NULL, NULL,
  /* 6c */ NULL, NULL, NULL, NULL,
  /* 70 */ NULL, NULL, NULL, NULL,
  /* 74 */ NULL, NULL, NULL, NULL,
  /* 78 */ NULL, NULL, NULL, NULL,
  /* 7c */ NULL, NULL, &shared_use_class, &shared_def_class,
  /* 80 */ &nothing_class, NULL, NULL, NULL,
  /* 84 */ NULL, NULL, NULL, NULL,
  /* 88 */ NULL, NULL, NULL, NULL,
  /* 8c */ NULL, NULL, NULL, NULL,
  /* 90 */ NULL, NULL, NULL, NULL,
  /* 94 */ NULL, NULL, NULL, NULL,
  /* 98 */ NULL, NULL, NULL, NULL,
  /* 9c */ NULL, NULL, NULL, NULL,
  /* a0 */ NULL, NULL, NULL, NULL,
  /* a4 */ NULL, NULL, NULL, NULL,
  /* a8 */ NULL, NULL, NULL, NULL,
  /* ac */ NULL, NULL, NULL, NULL,
  /* b0 */ NULL, NULL, NULL, NULL,
  /* b4 */ NULL, NULL, NULL, NULL,
  /* b8 */ NULL, NULL, NULL, NULL,
  /* bc */ NULL, NULL, NULL, NULL,
  /* c0 */ NULL, NULL, NULL, NULL,
  /* c4 */ NULL, NULL, NULL, NULL,
  /* c8 */ NULL, NULL, NULL, NULL,
  /* cc */ NULL, NULL, NULL, NULL,
  /* d0 */ NULL, NULL, NULL, NULL,
  /* d4 */ NULL, NULL, NULL, NULL,
  /* d8 */ NULL, NULL, NULL, NULL,
  /* dc */ NULL, NULL, NULL, &rule_class,
  /* e0 */ &node_expr_term_class, &node_expr_var_class, &node_expr_field_class, &node_expr_call_class,
  /* e4 */ &node_expr_mon_class, &node_expr_bin_class, &node_expr_bin_class, &node_expr_bin_class,
  /* e8 */ &node_expr_bin_class, &node_expr_bin_class, &node_expr_bin_class, &node_expr_mon_class,
  /* ec */ &node_expr_mon_class, &node_expr_mon_class, &node_expr_bin_class, &node_expr_regsplit_class,
  /* f0 */ &type_heap_class, &node_rule_class, &node_trans_class, &node_expr_ifstmt_class,
  /* f4 */ &node_state_class, &varset_class, NULL, &field_table_class,
  /* f8 */ &field_record_table_class, &thread_group_class, &thread_queue_elt_class, &thread_queue_class,
  /* fc */ &db_small_table_class, NULL, &env_split_class, &env_bind_class
};

int save_gc_struct (save_ctx_t *sctx, gc_header_t *p)
{
  int c, err;
  unsigned long id;
  gc_class_t *gccl;

  if (p==NULL)
    {
      if (putc_unlocked (T_NOTHING, sctx->f) < 0)
	err = errno;
      else err = 0;
    }
  else switch ((int)(unsigned int)(p->flags &
				   (GC_FLAGS_EST_SEEN | GC_FLAGS_EST_SHARED)))
    {
    case GC_FLAGS_EST_SEEN | GC_FLAGS_EST_SHARED:
      /* ah, this object is shared, and it is the first time we try to save it; */
      p->flags &= ~GC_FLAGS_EST_SEEN; /* mark it by resetting the *_SEEN
					 flag, so that next time we see it,
					 we shall enter the 'case GC_FLAGS_EST_SHARED'
					 branch. */
      if (putc_unlocked (T_SHARED_DEF, sctx->f) < 0) { err = errno; goto end; }
      /* compute unique identifier for pointer p;
	 I assume an unsigned long will hold enough of the bits of p for that.
	 We do not want to reveal too much of the memory layout, so we
	 xor it with some (fixed) magic value, sctx->fuzz.  This is not meant as
	 a serious security measure.
      */
      id = ((unsigned long)p) ^ sctx->fuzz;
      err = save_ulong (sctx, id);
      if (err) goto end;
      /* now save the object itself */
      /*FALLTHROUGH*/
    case GC_FLAGS_EST_SEEN: /* normal case: object is reachable, but not shared */
      c = TYPE(p);
      if (putc_unlocked (c, sctx->f) < 0) { err = errno; goto end; }
      gccl = gc_classes[c];
      if (gccl==NULL) { err = -2; goto end; }
      err = (*gccl->save) (sctx, p);
      break;
    case GC_FLAGS_EST_SHARED: /* this one is shared, but we have already saved
				 it at a previous iteration */
      if (putc_unlocked (T_SHARED_USE, sctx->f) < 0) { err = errno; goto end; }
      id = ((unsigned long)p) ^ sctx->fuzz;
      err = save_ulong (sctx, id);
      break;
    default: /* namely, 0: we are trying to save an unreachable object...
		something's gone wrong. */
      err = -2;
      break;
    }
 end:
  return err;
}

/* Before calling restore_gc_struct(), set errno=0,
   get back ctx->xclasses and put it into rctx.
*/
gc_header_t *restore_gc_struct (restore_ctx_t *rctx)
{
  int c;
  gc_class_t *gccl;

  c = getc_unlocked (rctx->f);
  if (c==EOF) { errno = c; return NULL; }
  if (c<0 || c>=256 || (gccl = gc_classes[c])==NULL || gccl->restore==NULL)
    { errno = -2; return NULL; }
  return (*gccl->restore) (rctx);
}

/*
** Copyright (c) 2002-2005 by Julien OLIVAIN, Laboratoire Spécification
** et Vérification (LSV), CNRS UMR 8643 & ENS Cachan.
** Copyright (c) 2014-2015 by Jean GOUBAULT-LARRECQ, Laboratoire Spécification
** et Vérification (LSV), CNRS UMR 8643 & ENS Cachan.
**
** Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
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
