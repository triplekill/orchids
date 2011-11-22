/**
 ** @file lang.h
 ** Intrusion scenario signature definitions language (issdl).
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup engine
 **
 ** @date  Started on: Mon Jan 27 16:54:31 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef LANG_H
#define LANG_H

#include <unistd.h>     /* for std types, size_t, etc... */
#include <sys/time.h>   /* for struct timeval */
#include <netinet/in.h> /* for struct addr */

# ifdef PCREPOSIX
#  include <pcreposix.h>
# else
#  include <regex.h>
#endif
#include "hash.h"

#include "orchids.h"

/* Minimal natives types */
#define T_NULL        0
#define T_FNCT        1
#define T_INT         2
#define T_BSTR        3
#define T_VBSTR       4
#define T_STR         5
#define T_VSTR        6
#define T_ARRAY       7
#define T_HASH        8

/* Extended types */
#define T_CTIME       9
#define T_IPV4       10
#define T_TIMEVAL    11
#define T_COUNTER    12
#define T_REGEX      13
#define T_SREGEX     14
#define T_PTR32      15
#define T_UINT       16
#define T_SNMPOID    17
#define T_FLOAT      18
#define T_DOUBLE     19

#define T_EXTERNAL	 20

/* ToDo -- coming soon */
#define T_NTPTIMESTAMP 0
#define T_TCPPORT 0
#define T_IPV4PEER 0#define T_IPV6ADDR 0
#define T_IPV6PEER 0

/* types flags mask -- this allow 2^16 types and 16 bits for flags */
#define TYPE_FLAGS 0xFFFF0000
#define TYPE_MASK  0x0000FFFF

/* monotony flags */
#define MONOTONY_MASK 3
#define TYPE_UNKNOWN  0
#define TYPE_MONO     1
#define TYPE_ANTI     2
#define TYPE_CONST    (TYPE_MONO|TYPE_ANTI)

/* Named variable or anonymous value.
** Used for intermediate temporary values in expressions.
** Intermediate values must be freed, but value bound to a variable
** must be kept. */
#define TYPE_CANFREE     (1 << 2)
#define TYPE_NOTBOUND    (1 << 3)

#define CAN_FREE_VAR(x) ((x)->flags & TYPE_CANFREE)
#define IS_NOT_BOUND(x) \
     (((x)->flags & (TYPE_CANFREE|TYPE_NOTBOUND)) == (TYPE_CANFREE|TYPE_NOTBOUND))

#define FREE_VAR(x)			\
  do {						\
    if (x)			\
   issdl_free(x);	\
  } while (0)

#define FREE_IF_NEEDED(x) \
  do { \
    if (x && IS_NOT_BOUND(x)) { \
      issdl_free(x); \
     } \
  } while (0)


#define STR_PAD_LEN 4

#define OVM_VAR(var)   ((ovm_var_t *)(var))

/* data accessors */
#define       TYPE(var)      (((ovm_var_t *)(var))->type)
#define    STRTYPE(var)     (str_issdltype(TYPE((ovm_var_t *)(var))))
#define      FLAGS(var)      (((ovm_var_t *)(var))->flags)
#define      ERRNO(var)     (((ovm_null_t *)(var))->err_no)
#define        INT(var)      (((ovm_int_t *)(var))->val)
#define       UINT(var)     (((ovm_uint_t *)(var))->val)
#define        STR(var)      (((ovm_str_t *)(var))->str)
#define     STRLEN(var)      (((ovm_str_t *)(var))->len)
#define       VSTR(var)     (((ovm_vstr_t *)(var))->str)
#define    VSTRLEN(var)     (((ovm_vstr_t *)(var))->len)
#define    TIMEVAL(var)  (((ovm_timeval_t *)(var))->time)
#define      CTIME(var)    (((ovm_ctime_t *)(var))->time)
#define       BSTR(var)     (((ovm_bstr_t *)(var))->str)
#define    BSTRLEN(var)     (((ovm_bstr_t *)(var))->len)
#define      VBSTR(var)    (((ovm_vbstr_t *)(var))->str)
#define   VBSTRLEN(var)    (((ovm_vbstr_t *)(var))->len)
#define       IPV4(var)     (((ovm_ipv4_t *)(var))->ipv4addr)
#define    COUNTER(var)  (((ovm_counter_t *)(var))->val)
#define      REGEX(var)    (((ovm_regex_t *)(var))->regex)
#define   REGEXSTR(var)    (((ovm_regex_t *)(var))->regex_str)
#define     SREGEX(var)   (((ovm_sregex_t *)(var))->regex)
#define  SREGEXNUM(var)   (((ovm_sregex_t *)(var))->splits)
#define  SREGEXSTR(var)   (((ovm_sregex_t *)(var))->regex_str)
#define      PTR32(var)    (((ovm_ptr32_t *)(var))->addr)
#define SNMPOIDLEN(var)  (((ovm_snmpoid_t *)(var))->len)
#define    SNMPOID(var)  (((ovm_snmpoid_t *)(var))->objoid)
#define      FLOAT(var)    (((ovm_float_t *)(var))->val)
#define     DOUBLE(var)   (((ovm_double_t *)(var))->val)
#define     EXTPTR(var)    (((ovm_extern_t *)(var))->ptr)
#define    EXTDESC(var)    (((ovm_extern_t *)(var))->desc)
#define    EXTFREE(var)    (((ovm_extern_t *)(var))->free)

#define IS_NULL(x) (!x || (TYPE(x) == T_NULL))

/* ERRNO values */
#define ERRNO_UNDEFINED		1
#define ERRNO_PARAMETER_ERROR	2
#define ERRNO_REGEX_ERROR	3

#define STATIC_VAR(ctx, si, var)			       \
   ((si)->state->rule->static_env[	       \
    (ctx)->rule_compiler->var				       \
    ])


#define ISSDL_RETURN_TRUE(ctx, si)			       \
  do { \
  stack_push((ctx)->ovm_stack, STATIC_VAR(ctx, si, static_1_res_id));	\
  } while (0)

#define ISSDL_RETURN_FALSE(ctx, si)		\
  do { \
  stack_push((ctx)->ovm_stack, STATIC_VAR(ctx, si, static_0_res_id));	\
  } while (0)

#define ISSDL_RETURN_NULL(ctx, si)		\
  do { \
  stack_push((ctx)->ovm_stack, STATIC_VAR(ctx, si, static_null_res_id));	\
  } while (0)

#define ISSDL_RETURN_PARAM_ERROR(ctx, si)	\
  do { \
  stack_push((ctx)->ovm_stack, STATIC_VAR(ctx, si, static_param_error_res_id));	\
  } while (0)

/**
 ** @struct ovm_var_s
 **   The generic universal data type for ISSDL.
 **/
/**   @var ovm_var_s::type
 **     Data type identifier.
 **/
/**   @var ovm_var_s::flags
 **     Data information flags. Actually, flags are TYPE_UNKNOWN,
 **     TYPE_MONO, TYPE_ANTI, TYPE_CONST and TYPE_TEMP.
 **/
/**   @var ovm_var_s::data
 **     Generic binary data.
 **     It should be a zero-length array... but it use STR_PAD_LEN to be ISO-C
 **     compliant (ISO-C doesn't allow zero length arrays)
 **     and to keep alignment...
 **/
typedef struct ovm_var_s ovm_var_t;
struct ovm_var_s
{
  uint32_t type;
  uint32_t flags;
  uint8_t  data[STR_PAD_LEN];
};



typedef void *(*issdl_getdata_t)(ovm_var_t *data);
typedef size_t (*issdl_getdata_len_t)(ovm_var_t *data);

typedef int (*var_cmp_t)(ovm_var_t *var1, ovm_var_t *var2);
typedef ovm_var_t *(*var_add_t)(ovm_var_t *var1, ovm_var_t *var2);
typedef ovm_var_t *(*var_sub_t)(ovm_var_t *var1, ovm_var_t *var2);
typedef ovm_var_t *(*var_mul_t)(ovm_var_t *var1, ovm_var_t *var2);
typedef ovm_var_t *(*var_div_t)(ovm_var_t *var1, ovm_var_t *var2);
typedef ovm_var_t *(*var_mod_t)(ovm_var_t *var1, ovm_var_t *var2);
typedef ovm_var_t *(*var_clone_t)(ovm_var_t *var);
typedef void	   (*var_destruct_t)(ovm_var_t *var);

/**
 ** @struct issdl_type_s
 **   ISSDL type definition
 **/
/**   @var issdl_type_s::name
 **     Type name.
 **/
/**   @var issdl_type_s::flags
 **     Type flags.
 **/
/**   @var issdl_type_s::get_data
 **     A binary data accessor.
 **/
/**   @var issdl_type_s::get_data_len
 **     Return the binary data length.
 **/
/**   @var issdl_type_s::cmp
 **     Comparison function handler.
 **/
/**   @var issdl_type_s::add
 **     Addition function handler.
 **/
/**   @var issdl_type_s::sub
 **     Subtraction function handler.
 **/
/**   @var issdl_type_s::mul
 **     Multiplication function handler.
 **/
/**   @var issdl_type_s::div
 **     Division function handler.
 **/
/**   @var issdl_type_s::mod
 **     Modulo function handler.
 **/
/**   @var issdl_type_s::clone
 **     Cloning function handler.
 **/
/**   @var issdl_type_s::destruct
 **     Destructor function handler.
 **/
/**   @var issdl_type_s::desc
 **     A shot text description of the type.
 **/
typedef struct issdl_type_s issdl_type_t;
struct issdl_type_s
{
  char                *name;
  unsigned long        flags;
  issdl_getdata_t      get_data;
  issdl_getdata_len_t  get_data_len;
  var_cmp_t            cmp;
  var_add_t            add;
  var_sub_t            sub;
  var_mul_t            mul;
  var_div_t            div;
  var_mod_t            mod;
  var_clone_t          clone;
  var_destruct_t       destruct;
  /* XXX add extension here */
  char                *desc;
};

/**
 ** @struct ovm_null_s
 **   ISSDL null datatype.
 **   Used for uninitialized data and/or error/exception reporting
 **   (divide by 0, not allocated, etc... etc...).
 **/
/**   @var ovm_null_s::type
 **     Data type identifier: T_NULL.
 **/
/**   @var ovm_null_s::flags
 **     Data access flags.
 **/
/**   @var ovm_null_s::err_no
 **     Error/exception identifer.
 **/
typedef struct ovm_null_s ovm_null_t;
struct ovm_null_s
{
  uint32_t  type;
  uint32_t  flags;
  int       err_no;
};

/**
 ** @struct @ovm_int_s
 **   ISSDL integer data type. Contain a 'C signed int'.
 **/
/**   @var ovm_int_s::type
 **     Data type identifier: T_INT.
 **/
/**   @var ovm_int_s::flags
 **     Data access flags.
 **/
/**   @var ovm_int_s::val
 **     Integer value.
 **/
typedef struct ovm_int_s ovm_int_t;
struct ovm_int_s
{
  uint32_t  type;
  uint32_t  flags;
  long   val;
};

/**
 ** @struct @ovm_uint_s
 **   ISSDL unsigned integer data type. Contain a 'C unsigned long'.
 **/
/**   @var ovm_int_s::type
 **     Data type identifier: T_UINT.
 **/
/**   @var ovm_int_s::flags
 **     Data access flags.
 **/
/**   @var ovm_int_s::val
 **     Integer value.
 **/
typedef struct ovm_uint_s ovm_uint_t;
struct ovm_uint_s
{
  uint32_t      type;
  uint32_t      flags;
  unsigned long val;
};


/**
 ** @struct ovm_str_s
 **   ISSDL string data type. (note: memory is contiguous)
 **/
/**   @var ovm_str_s::type
 **     Data type identifier: T_STR.
 **/
/**   @var ovm_str_s::flags
 **     Data access flags.
 **/
/**   @var ovm_str_s::len
 **     String length.
 **/
/**   @var ovm_str_s::str
 **     Dynamically allocated string memory.
 **     This dummy size is used to be ISO-C compliant
 **     and keep memory alignment.
 **/
typedef struct ovm_str_s ovm_str_t;
struct ovm_str_s
{
  uint32_t  type;
  uint32_t  flags;
  size_t    len;
  char      str[STR_PAD_LEN];
};




/**
 ** @struct ovm_bstr_s
 **   ISSDL binary string type. (note: memory is contiguous)
 **/
/**   @var ovm_bstr_s::type
 **     Data type identifier: T_BINSTR.
 **/
/**   @var ovm_bstr_s::flags
 **     Data access flags.
 **/
/**   @var ovm_bstr_s::len
 **     Binary string length.
 **/
/**   @var ovm_bstr_s::str
 **     Dynamically allcoated memory.
 **  This dummy size is used to be ISO-C compliant and keep
 ** memory alignement.
 **/
typedef struct ovm_bstr_s ovm_bstr_t;
struct ovm_bstr_s
{
  uint32_t  type;
  uint32_t  flags;
  size_t    len;
  uint8_t   str[STR_PAD_LEN];
};


/**
 ** @struct ovm_vstr_s
 **   ISSDL virtual string data type. Points to a allocated string, elsewhere.
 **   Pointed string memory have to be managed manually.
 **/
/**   @var ovm_vstr_s::type
 **     Data type identifier: T_VSTR.
 **/
/**   @var ovm_vstr_s::flags
 **     Data access flags.
 **/
/**   @var ovm_vstr_s::len
 **     String length.
 **/
/**   @var ovm_vstr_s::str
 **     A pointer to the string.
 **/
typedef struct ovm_vstr_s ovm_vstr_t;
struct ovm_vstr_s
{
  uint32_t  type;
  uint32_t  flags;
  size_t    len;
  char     *str;
};


/**
 ** @struct ovm_vbstr_s
 **   ISSDL virtual binary string data type. Points to a allocated
 **   binary string, elsewhere. Pointed binary string memory have to be managed
 **   manually.
 **/
/**   @var ovm_vbstr_s::type
 **     Data type identifier: T_VBINSTR.
 **/
/**   @var ovm_vbstr_s::flags
 **     Data access flags.
 **/
/**   @var ovm_vbstr_s::len
 **     Binary string length.
 **/
/**   @var ovm_vbstr_s::str
 **     A pointer to the binary string.
 **/
typedef struct ovm_vbstr_s ovm_vbstr_t;
struct ovm_vbstr_s
{
  uint32_t  type;
  uint32_t  flags;
  size_t    len;
  uint8_t  *str;
};

/**
 ** @struct ovm_ctime_s
 **   ISSDL ctime data type. It contains an unix c-time (time_t) value.
 **   Time precision is second.
 **/
/**   @var ovm_ctime_s::type
 **     Data type identifier: T_CTIME.
 **/
/**   @var ovm_ctime_s::flags
 **     Data access flags.
 **/
/**   @var ovm_ctime_s::time
 **     Time value.
 **/
typedef struct ovm_ctime_s ovm_ctime_t;
struct ovm_ctime_s
{
  uint32_t  type;
  uint32_t  flags;
  time_t    time;
};


/**
 ** @struct ovm_ipv4_s
 **   ISSDL ipv4addr data type. It contains an unix in_addr structure
 **   (as defined in <netinet/in.h> header file).
 **/
/**   @var ovm_ipv4_s::type
 **     Data type identifier: T_IPV4ADDR.
 **/
/**   @var ovm_ipv4_s::flags
 **     Data access flags.
 **/
/**   @var ovm_ipv4_s::ipv4addr
 **     IP address value.
 **/
typedef struct ovm_ipv4_s ovm_ipv4_t;
struct ovm_ipv4_s
{
  uint32_t        type;
  uint32_t        flags;
  struct in_addr  ipv4addr;
};


/**
 ** @struct ovm_timeval_s
 **   ISSDL timeval data type. It contains an unix timeval structure
 **   (as defined in <sys/timeval.h> header file).
 **   Time precision is micro-second.
 **/
/**   @var ovm_timeval_s::type
 **     Data type identifier: T_TIMEVAL.
 **/
/**   @var ovm_timeval_s::flags
 **     Data access flags.
 **/
/**   @var ovm_timeval_s::time
 **     Time value.
 **/
typedef struct ovm_timeval_s ovm_timeval_t;
struct ovm_timeval_s
{
  uint32_t        type;
  uint32_t        flags;
  struct timeval  time;
};


/**
 ** @struct @ovm_counter_s
 **   ISSDL counter data type. Contain a 'C uint32_t'.
 **/
/**   @var ovm_counter_s::type
 **     Data type identifier: T_COUNTER.
 **/
/**   @var ovm_counter_s::flags
 **     Data access flags.
 **/
/**   @var ovm_counter_s::val
 **     Counter value.
 **/
typedef struct ovm_counter_s ovm_counter_t;
struct ovm_counter_s
{
  uint32_t  type;
  uint32_t  flags;
  unsigned long  val;
};


/**
 ** @struct @ovm_regex_s
 **   ISSDL compiled regular expression data type.
 **/
/**   @var ovm_regex_s::type
 **     Data type identifier: T_REGEX.
 **/
/**   @var ovm_regex_s::flags
 **     Data access flags.
 **/
/**   @var ovm_regex_s::regex_str
 **     Regular expression string.
 **/
/**   @var ovm_regex_s::regex
 **     Compiled regular expression.
 **/
typedef struct ovm_regex_s ovm_regex_t;
struct ovm_regex_s
{
  uint32_t  type;
  uint32_t  flags;
  char     *regex_str;
  regex_t   regex;
};


/**
 ** @struct @ovm_sregex_s
 **   ISSDL compiled splitting regular expression data type.
 **/
/**   @var ovm_sregex_s::type
 **     Data type identifier: T_REGEX.
 **/
/**   @var ovm_sregex_s::flags
 **     Data access flags.
 **/
/**   @var ovm_sregex_s::regex_str
 **     Regular expression string.
 **/
/**   @var ovm_sregex_s::regex
 **     Compiled regular expression.
 **/
/**   @var ovm_sregex_s::splits
 **     Number of splits.
 **/

typedef struct ovm_sregex_s ovm_sregex_t;
struct ovm_sregex_s
{
  uint32_t  type;
  uint32_t  flags;
  char     *regex_str;
  regex_t   regex;
  int       splits;
};


/**
 ** @struct @ovm_ptr32_s
 **   ISSDL address data type. Contain a 'void * ptr'.
 **/
/**   @var ovm_ptr32_s::type
 **     Data type identifier: T_ADDRESS.
 **/
/**   @var ovm_ptr32_s::flags
 **     Data access flags.
 **/
/**   @var ovm_ptr32_s::addr
 **     Address value.
 **/
typedef struct ovm_ptr32_s ovm_ptr32_t;
struct ovm_ptr32_s
{
  uint32_t  type;
  uint32_t  flags;
  void     *addr;
};


#if 0 /* HAVE_NETSNMP */
typedef oid oid_t;
#else
typedef unsigned long oid_t;
#endif


/**
 ** @struct ovm_snmpoid_s
 **   ISSDL SNMP Object Identifier. (note: memory is contiguous)
 **/
/**   @var ovm_snmpoid_s::type
 **     Data type identifier: T_SNMPOID.
 **/
/**   @var ovm_snmpoid_s::flags
 **     Data access flags.
 **/
/**   @var ovm_snmpoid_s::len
 **     SNMP OID length.
 **/
/**   @var ovm_snmpoid_s::objoid
 **     Dynamically allocated oid memory.
 **     This dummy size is used to be ISO-C compliant
 **     and keep memory alignment.
 **/
typedef struct ovm_snmpoid_s ovm_snmpoid_t;
struct ovm_snmpoid_s
{
  uint32_t  type;
  uint32_t  flags;
  size_t    len;
  oid_t     objoid[STR_PAD_LEN];
};


/**
 ** @struct ovm_float_s
 **   ISSDL 32-bits floating point value.
 **/
/**   @var ovm_float_s::type
 **     Data type identifier: T_FLOAT.
 **/
/**   @var ovm_float_s::flags
 **     Data access flags.
 **/
/**   @var ovm_float_s::val
 **     Floating point value.
 **/
typedef struct ovm_float_s ovm_float_t;
struct ovm_float_s
{
  uint32_t  type;
  uint32_t  flags;
  float     val;
};

/**
 ** @struct ovm_double_s
 **   ISSDL 64-bits floating point value.
 **/
/**   @var ovm_double_s::type
 **     Data type identifier: T_DOUBLE.
 **/
/**   @var ovm_double_s::flags
 **     Data access flags.
 **/
/**   @var ovm_double_s::val
 **     Floating point value.
 **/
typedef struct ovm_double_s ovm_double_t;
struct ovm_double_s
{
  uint32_t  type;
  uint32_t  flags;
  double    val;
};

/**
 ** @struct @ovm_extern_s
 **   ISSDL external data type. Contains a 'void * ptr',
 **   the data description, and the desctructor
 **/
/**   @var ovm_extern_s::type
 **     Data type identifier: T_ADDRESS.
 **/
/**   @var ovm_extern_s::flags
 **     Data access flags.
 **/
/**   @var ovm_extern_s::addr
 **     Address value.
 **/
typedef void (*freefct)(void *ptr);
typedef struct ovm_extern_s ovm_extern_t;
struct ovm_extern_s
{
  uint32_t  type;
  uint32_t  flags;
  void     *ptr;
  char	   *desc;
  void (*free)(void *ptr);
};



/*----------------------------------------------------------------------------*
** function prototypes                                                       **
*----------------------------------------------------------------------------*/

/**
 ** Accessor to the global type table.
 ** @return  A pointer to the type table.
 **/
issdl_type_t *
issdlgettypes(void);


/**
 ** Test a variable
 ** @return Return TRUE or FALSE
 **/
int
issdl_test(ovm_var_t *var);

/**
 ** Free a variable and its contents if needed.
 **/
void
issdl_free(ovm_var_t	*var);


/**
 ** Comparison function of the Orchids language.  This functions is used in
 ** the implementation of comparison opcode (equal, less than, greater
 ** than...).  This function is a dispatcher which use the comparison
 ** function implemented in the type of var1.  If var1 and var2 are of
 ** different types, the comparison function of var1 must be aware of
 ** the existence of the type of the var2 (or the test will fail
 ** otherwise).
 ** @param var1  The first variable to compare.
 ** @param var2  The second variable to compare.
 ** @return      An integer equal to 0 if var1 is equal to var2, less than
 **              0 if var1 is less than var2, or greater than 0 if var1 is
 **              greater than var2.
 **/
int
issdl_cmp(ovm_var_t *var1, ovm_var_t *var2);


/**
 ** Addition function of the Orchids language.
 ** @param var1  The first operand.
 ** @param var2  The second operand.
 ** @return      A new allocated variable containing the result.
 **/
ovm_var_t *
issdl_add(ovm_var_t *var1, ovm_var_t *var2);


/**
 ** Subtraction function of the Orchids language.
 ** @param var1  The first operand.
 ** @param var2  The second operand.
 ** @return      A new allocated variable containing the result.
 **/
ovm_var_t *
issdl_sub(ovm_var_t *var1, ovm_var_t *var2);


/**
 ** Multiplication function of the Orchids language.
 ** @param var1  The first operand.
 ** @param var2  The second operand.
 ** @return      A new allocated variable containing the result.
 **/
ovm_var_t *
issdl_mul(ovm_var_t *var1, ovm_var_t *var2);


/**
 ** Division function of the Orchids language.
 ** @param var1  The first operand.
 ** @param var2  The second operand.
 ** @return      A new allocated variable containing the result.
 **/
ovm_var_t *
issdl_div(ovm_var_t *var1, ovm_var_t *var2);


/**
 ** Modulo function of the Orchids language.
 ** @param var1  The first operand.
 ** @param var2  The second operand.
 ** @return      A new allocated variable containing the result.
 **/
ovm_var_t *
issdl_mod(ovm_var_t *var1, ovm_var_t *var2);


/**
 ** Variable cloning function.  This function not necessarily clone
 ** the whole memory representation of the object.  Some constant part
 ** may be shared during the cloning operation.
 ** @param var  The variable to be cloned.
 ** @return     A new allocated copy.
 **/
ovm_var_t *
issdl_clone(ovm_var_t *var);


/**
 ** Return the type name string given a type id.
 ** @param type  The type id.
 ** @return      The type name.
 **/
char *
str_issdltype(int type);


ovm_var_t *
ovm_null_new(void);

ovm_var_t *
ovm_sregex_new(void);


void *
issdl_get_data(ovm_var_t *val);

size_t
issdl_get_data_len(ovm_var_t *val);


void
fprintf_issdl_val(FILE *fp, ovm_var_t *val);

void
set_ip_resolution(int value);

void
ovm_null_fprintf(FILE *fp, ovm_null_t *val);


ovm_var_t *
ovm_int_new(void);

void
ovm_int_fprintf(FILE *fp, ovm_int_t *val);


ovm_var_t *
ovm_str_new(size_t size);

void
ovm_str_fprintf(FILE *fp, ovm_str_t *str);


ovm_var_t *
ovm_vstr_new(void);

void
ovm_vstr_fprintf(FILE *f, ovm_vstr_t *vstr);


ovm_var_t *
ovm_counter_new(void);

void
ovm_counter_fprintf(FILE *fp, ovm_counter_t *c);


ovm_var_t *
ovm_regex_new(void);

void
ovm_regex_fprintf(FILE *fp, ovm_regex_t *regex);


ovm_var_t *
ovm_ptr32_new(void);

void
ovm_ptr32_fprintf(FILE *fp, ovm_ptr32_t *val);


ovm_var_t *
ovm_ctime_new(void);

void
ovm_ctime_fprintf(FILE *fp, ovm_ctime_t *time);


ovm_var_t *
ovm_ipv4_new(void);

void
ovm_ipv4_fprintf(FILE *fp, ovm_ipv4_t *addr);


ovm_var_t *
ovm_timeval_new(void);

void
ovm_timeval_fprintf(FILE *fp, ovm_timeval_t *time);


ovm_var_t *
ovm_bstr_new(size_t size);

void
ovm_bstr_fprintf(FILE *fp, ovm_bstr_t *bstr);

ovm_var_t *
ovm_vbstr_new(void);

void
ovm_vbstr_fprintf(FILE *fp, ovm_vbstr_t *str);

ovm_var_t *
ovm_uint_new(void);

ovm_var_t *
ovm_extern_new(void);

void
ovm_uint_fprintf(FILE *fp, ovm_uint_t *val);

/**
 ** Print a variable on a stream.
 ** @param fp  The stream on which the variable will be printed.
 ** @param val The variable to print.
 **/
void
fprintf_ovm_var(FILE *fp, ovm_var_t *val);

/**
 ** Print a variable in a buffer with a fixed size.
 ** @param buff	The buffer to fill.
 ** @param size The buffer size.
 ** @param val The variable to print.
 ** @return The number of printed characters in the buffer.
 **/
int
snprintf_ovm_var(char *buff, unsigned int size, ovm_var_t *val);


ovm_var_t *
ovm_float_new(void);
void
ovm_float_fprintf(FILE *fp, ovm_float_t *val);

ovm_var_t *
ovm_snmpoid_new(size_t len);


/**
 ** Duplicate a string of the orchids language, and convert it to a C
 ** string (null terminated).
 ** @param str  A virtual machine variable.
 ** @return     A newly allocated C string.
 **/
char *
ovm_strdup(ovm_var_t *str);


#endif /* LANG_H */


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
