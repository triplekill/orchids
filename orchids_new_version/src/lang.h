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

/* Minimal native types */
#define T_NULL            0 /* Type of things not handled by the ovm, or
			       for which we don't care about the type    */
#define T_FNCT            1 /* Unused */
#define T_INT             2
#define T_BSTR            3
#define T_VBSTR           4
#define T_STR             5
#define T_VSTR            6
#define T_ARRAY           7
#define T_HASH            8

/* Extended types */
#define T_CTIME           9
#define T_IPV4            10
#define T_IPV6            11
#define T_TIMEVAL         12
#define T_REGEX           13
#define T_UINT            14
#define T_SNMPOID         15
#define T_FLOAT           16
#define T_EVENT           17
#define T_STATE_INSTANCE  18
#define T_DB_EMPTY        19
#define T_DB_MAP          20
#define T_DB_SINGLETON    21

#define T_EXTERNAL	  22


/* ToDo -- coming soon */
#define T_NTPTIMESTAMP 0
#define T_TCPPORT 0
#define T_IPV4PEER 0
#define T_IPV6PEER 0

/* Extra types, not used for constant values */
#define T_NOTHING 128
/* T_NOTHING is not used; only as a tag in functions save_gc_struct()
   and related */
#define T_SHARED_DEF 127
/* not used, only as a tag in functions save_gc_struct() and related */
#define T_SHARED_USE 126
/* not used, only as a tag in functions save_gc_struct() and related */
#define T_SHARED_DEF_FORWARD 125
/* not used, only as a tag in functions save_gc_struct() and related */
#define T_SHARED_USE_FORWARD 124
/* not used, only as a tag in functions save_gc_struct() and related */

#define T_BIND 255
/* T_BIND is for environments; see env_bind_t in orchids.h */
#define T_SPLIT 254
/* T_SPLIT is for environments; see env_split_t in orchids.h */
#define T_HEAP 253
/* T_HEAP is for skew heaps, as used in register_rtaction() and others in evt_mgr.c */
#define T_DB_SMALL_TABLE 252
/* T_DB_SMALL_TABLE is for collision lists (db_small_table) in databases, see db.c */
#define T_THREAD_QUEUE 251
/* T_THREAD_QUEUE is for thread queues (thread_queue_t) in engine.c */
#define T_THREAD_QUEUE_ELT 250
/* T_THREAD_QUEUE_ELT is for thread queue elements (thread_queue_elt_t)
   in engine.c */
#define T_THREAD_GROUP 249
/* T_THREAD_GROUP is for thread groups (thread_group_t) in engine.c */
#define T_FIELD_RECORD_TABLE 248
/* T_FIELD_RECORD_TABLE is for the unique table of field records
   (field_record_table_t) in orchids_api.c */
#define T_FIELD_TABLE 247
/* T_FIELD_TABLE is for field tables (field_table_t) in orchids_api.c */
#define T_RULE_COMPILER 246
/* T_RULE_COMPILER is for the unique rule compiler context (rule_compiler_t)
   in rule_compiler.c */
#define T_VARSET 245
/* T_VARSET is for sets of variables (varset_t) in rule_compiler.c */
#define T_NODE_STATE 244
/* T_NODE_STATE is for state nodes (node_state_t) in rule_compiler.c */
#define T_NODE_IFSTMT 243
/* T_NODE_IFSTMT is for if statement nodes (node_expr_if_t) in rule_compiler.c */
#define T_NODE_TRANS 242
/* T_NODE_TRANS is for transition nodes (node_trans_t) in rule_compiler.c */
#define T_NODE_RULE 241
/* T_NODE_RULE is for rule nodes (node_rule_t) in rule_compiler.c */
#define T_TYPE_HEAP 240
/* T_TYPE_HEAP is for heaps of type equations (type_heap_t) in rule_compiler.c */
#define T_NODE_REGSPLIT 239
/* T_NODE_REGSPLIT is for regexp split expression nodes (node_expr_regsplit_t,
   of class node_expr_regsplit_class) in rule_compiler.c */
#define T_NODE_BINOP 238
/* T_NODE_BINOP is for binary operator expression nodes (node_expr_bin_t,
   of class node_expr_bin_class) in rule_compiler.c */
#define T_NODE_MONOP 237
/* T_NODE_MONOP is for unary operator expression nodes (node_expr_mon_t,
   of class node_expr_mon_class) in rule_compiler.c */
#define T_NODE_RETURN 236
/* T_NODE_RETURN is for return expression nodes (node_expr_mon_t,
   of class node_expr_mon_class --- as for T_NODE_MONOP) in rule_compiler.c */
#define T_NODE_BREAK 235
/* T_NODE_BREAK is for break expression nodes (node_expr_mon_t,
   of class node_expr_mon_class --- as for T_NODE_MONOP) in rule_compiler.c */
#define T_NODE_CONS 234
/* T_NODE_CONS is for cons nodes (node_expr_bin_t, of class node_expr_bin_class
   --- as for T_NODE_BINOP) in rule_compiler.c */
#define T_NODE_COND 233
/* T_NODE_COND is for logical operator nodes (node_expr_bin_t,
   of class node_expr_bin_class --- as for T_NODE_BINOP) in rule_compiler.c */
#define T_NODE_ASSOC 232
/* T_NODE_ASSOC is for assignment nodes (node_expr_bin_t,
   of class node_expr_bin_class --- as for T_NODE_BINOP) in rule_compiler.c */
#define T_NODE_EVENT 231
/* T_NODE_EVENT is for event enrichment nodes (node_expr_bin_t,
   of class node_expr_bin_class --- as for T_NODE_BINOP) in rule_compiler.c */
#define T_NODE_DB_PATTERN 230
/* T_NODE_DB_PATTERN is for database pattern nodes (node_expr_bin_t,
   of class node_expr_bin_class --- as for T_NODE_BINOP) in rule_compiler.c */
#define T_NODE_DB_COLLECT 229
/* T_NODE_DB_COLLECT is for database 'collect' nodes (node_expr_bin_t,
   of class node_expr_bin_class --- as for T_NODE_BINOP) in rule_compiler.c */
#define T_NODE_DB_SINGLETON 228
/* T_NODE_DB_SINGLETON is for database singleton nodes (node_expr_mon_t,
   of class node_expr_mon_class --- as for T_NODE_MONOP) in rule_compiler.c */
#define T_NODE_CALL 227
/* T_NODE_CALL is for primitive call nodes (node_expr_call_t)
   in rule_compiler.c */
#define T_NODE_FIELD 226
/* T_NODE_FIELD is for field name nodes (node_expr_symbol_t)
   in rule_compiler.c */
#define T_NODE_VARIABLE 225
/* T_NODE_VARIABLE is for variable nodes (node_expr_symbol_t
   --- as for T_NODE_FIELD) in rule_compiler.c */
#define T_NODE_CONST 224
/* T_NODE_CONST is for constant value nodes (node_expr_term_t)
   in rule_compiler.c */
#define T_RULE 223
/* T_RULE is for OrchIDS rules (rule_t) in rule_compiler.c */

/* Types, for the type checker.
   Not to be confused with the run-time type tags T_*
*/
typedef struct type_s type_t;
struct type_s {
  char *name;
  unsigned char tag;
};

extern type_t t_int, t_uint, t_float,
  t_bstr, t_str, t_ctime, t_timeval, t_ipv4, t_ipv6,
  t_regex, t_snmpoid, t_event, t_mark, t_db_any,
  t_void, t_any;

#define STR_PAD_LEN 4

#define OVM_VAR(var)   ((ovm_var_t *)(var))

/* data accessors */
#define       TYPE(var)      ((uint32_t)((ovm_var_t *)(var))->gc.type)
#define    STRTYPE(var)     (str_issdltype(TYPE((ovm_var_t *)(var))))
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
#define       IPV6(var)     (((ovm_ipv6_t *)(var))->ipv6addr)
#define      REGEX(var)    (((ovm_regex_t *)(var))->regex)
#define   REGEXSTR(var)    (((ovm_regex_t *)(var))->regex_str)
#define   REGEXNUM(var)   (((ovm_regex_t *)(var))->splits)
#define SNMPOIDLEN(var)  (((ovm_snmpoid_t *)(var))->len)
#define    SNMPOID(var)  (((ovm_snmpoid_t *)(var))->objoid)
#define      FLOAT(var)    (((ovm_float_t *)(var))->val)
#define     EXTPTR(var)    (((ovm_extern_t *)(var))->ptr)
#define   EXTCLASS(var)   (((ovm_extern_t *)(var))->class)
#define    EXTDESC(var)    (((ovm_extern_t *)(var))->class->desc)
#define    EXTCOPY(var)    (((ovm_extern_t *)(var))->class->copy)
#define    EXTFREE(var)    (((ovm_extern_t *)(var))->class->free)
#define    EXTSAVE(var)    (((ovm_extern_t *)(var))->class->save)
#define EXTRESTORE(var) (((ovm_extern_t *)(var))->class->restore)

#define IS_NULL(var) ((var)==NULL)

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
  gc_header_t gc;
  uint8_t  data[STR_PAD_LEN];
};



typedef void *(*issdl_getdata_t)(ovm_var_t *data);
typedef size_t (*issdl_getdata_len_t)(ovm_var_t *data);

typedef int (*var_cmp_t)(ovm_var_t *var1, ovm_var_t *var2, int dir);
typedef ovm_var_t *(*var_add_t)(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2);
typedef ovm_var_t *(*var_sub_t)(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2);
typedef ovm_var_t *(*var_opp_t)(gc_t *gc_ctx, ovm_var_t *var);
typedef ovm_var_t *(*var_mul_t)(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2);
typedef ovm_var_t *(*var_div_t)(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2);
typedef ovm_var_t *(*var_mod_t)(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2);
typedef ovm_var_t *(*var_clone_t)(gc_t *gc_ctx, ovm_var_t *var);
typedef ovm_var_t *(*var_and_t)(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2);
typedef ovm_var_t *(*var_or_t)(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2);
typedef ovm_var_t *(*var_xor_t)(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2);
typedef ovm_var_t *(*var_not_t)(gc_t *gc_ctx, ovm_var_t *var);
typedef ovm_var_t *(*var_plus_t)(gc_t *gc_ctx, ovm_var_t *var);

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
/**   @var issdl_type_s::opp
 **     Opposite function handler.
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
/**   @var issdl_type_s::add
 **     Bitwise add function handler.
 **/
/**   @var issdl_type_s::or
 **     Bistwise inclusive or function handler.
 **/
/**   @var issdl_type_s::xor
 **     Biteise exclusive or function handler.
 **/
/**   @var issdl_type_s::not
 **     Bitwise complement function handler.
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
  var_opp_t            opp;
  var_mul_t            mul;
  var_div_t            div;
  var_mod_t            mod;
  var_clone_t          clone;
  var_and_t            and;
  var_or_t             or;
  var_xor_t            xor;
  var_not_t            not;
  var_plus_t	       plus;
  /* XXX add extension here */
  char                *desc;
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
  gc_header_t gc;
  long val;
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
  gc_header_t gc;
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
  gc_header_t gc;
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
  gc_header_t gc;
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
  gc_header_t gc;
  ovm_var_t *delegate;
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
  gc_header_t gc;
  ovm_var_t *delegate;
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
  gc_header_t gc;
  time_t    time;
};


/**
 ** @struct ovm_ipv4_s
 **   ISSDL ipv4addr data type. It contains an unix in_addr structure
 **   (as defined in <netinet/in.h> header file).
 **/
/**   @var ovm_ipv4_s::type
 **     Data type identifier: T_IPV4.
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
  gc_header_t gc;
  struct in_addr ipv4addr;
};


/**
 ** @struct ovm_ipv6_s
 **   ISSDL ipv6addr data type. It contains an unix in6_addr structure
 **   (as defined in <netinet/in.h> header file).
 **/
/**   @var ovm_ipv6_s::type
 **     Data type identifier: T_IPV6.
 **/
/**   @var ovm_ipv6_s::flags
 **     Data access flags.
 **/
/**   @var ovm_ipv6_s::ipv6addr
 **     IP address value.
 **/
typedef struct ovm_ipv6_s ovm_ipv6_t;
struct ovm_ipv6_s
{
  gc_header_t gc;
  struct in6_addr ipv6addr;
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
  gc_header_t gc;
  struct timeval time;
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
/**   @var ovm_regex_s::splits
 **     Number of splits.
 **/
typedef struct ovm_regex_s ovm_regex_t;
struct ovm_regex_s
{
  gc_header_t gc;
  char     *regex_str;
  regex_t   regex;
  int splits;
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
  gc_header_t gc;
  size_t    len;
  oid_t     objoid[1]; /* actually objoid[len] */
};


/**
 ** @struct ovm_float_s
 **   ISSDL (64-bit) floating point value.
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
  gc_header_t gc;
  double     val;
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
typedef struct ovm_extern_class_s ovm_extern_class_t;
struct ovm_extern_class_s {
  char	   *desc;
  void *(*copy)(gc_t *gc_ctx, void *ptr);
  void (*free)(void *ptr);
  int (*save)(save_ctx_t *sctx, void *ptr);
  void *(*restore)(restore_ctx_t *rctx);
};

typedef struct ovm_extern_s ovm_extern_t;
struct ovm_extern_s
{
  gc_header_t gc;
  void     *ptr;
  struct ovm_extern_class_s *class;
};



/*----------------------------------------------------------------------------*
** function prototypes                                                       **
*----------------------------------------------------------------------------*/

/**
 ** Accessor to the global type table.
 ** @return  A pointer to the type table.
 **/
issdl_type_t *issdlgettypes(void);


/**
 ** Test a variable
 ** @return Return TRUE or FALSE
 **/
int
issdl_test(ovm_var_t *var);


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
 ** @param dir   A flag
 ** @return      An integer:
 **              If dir & CMP_LEQ_MASK, result has CMP_LEQ_MASK set
 **              iff var1<=var2, unset otherwise (i.e., unset if var1>var2
 **              on totally ordered types;
 **              databases are *not* totally ordered)
 **              If dir & CMP_GEQ_MASK, result has CMP_GEQ_MASK set
 **              iff var1>=var2, unset otherwise (same comment as above).
 **              In particular, if dir==CMP_LEQ_MASK | CMP_GEQ_MASK,
 **              you will get CMP_LEQ_MASK if var1<var2,
 **              CMP_GEQ_MASK if var1>var2,
 **              CMP_LEQ_MASK | CMP_GEQ_MASK if var1==var2,
 **              0 if var1 and var2 are incomparable (including cases of
 **              type error).
 **/
#define CMP_LEQ_MASK 0x2
#define CMP_GEQ_MASK 0x1
#define CMP_LT CMP_LEQ_MASK
#define CMP_GT CMP_GEQ_MASK
#define CMP_EQ (CMP_LEQ_MASK | CMP_GEQ_MASK)
#define CMP_ERROR (0)
#define CMP_EQUAL(res) ((res)==(CMP_LEQ_MASK | CMP_GEQ_MASK))
#define CMP_DIFFERENT(res) (!CMP_EQUAL(res))
#define CMP_LEQ(res) (((res) & CMP_LEQ_MASK) != 0)
#define CMP_GEQ(res) (((res) & CMP_GEQ_MASK) != 0)
#define CMP_LESS(res) ((res)==CMP_LEQ_MASK)
#define CMP_GREATER(res) ((res)==CMP_GEQ_MASK)
#define CMP_INCOMPARABLE(res) ((res)==0)
int issdl_cmp(ovm_var_t *var1, ovm_var_t *var2, int dir);


/**
 ** Addition function of the Orchids language.
 ** @param var1  The first operand.
 ** @param var2  The second operand.
 ** @return      A new allocated variable containing the result.
 **/
ovm_var_t *issdl_add(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2);


/**
 ** Subtraction function of the Orchids language.
 ** @param var1  The first operand.
 ** @param var2  The second operand.
 ** @return      A new allocated variable containing the result.
 **/
ovm_var_t *issdl_sub(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2);

/**
 ** Opposite function of the Orchids language (- operator).
 ** @param var   The operand.
 ** @return      A new allocated variable containing the result.
 **/
ovm_var_t *issdl_opp(gc_t *gc_ctx, ovm_var_t *var1);

/**
 ** Same sign conversion (+ operator)
 ** @param var   The operand.
 ** @return      A new allocated variable containing the result.
 **/
ovm_var_t *issdl_plus(gc_t *gc_ctx, ovm_var_t *var1);


/**
 ** Multiplication function of the Orchids language.
 ** @param var1  The first operand.
 ** @param var2  The second operand.
 ** @return      A new allocated variable containing the result.
 **/
ovm_var_t *issdl_mul(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2);


/**
 ** Division function of the Orchids language.
 ** @param var1  The first operand.
 ** @param var2  The second operand.
 ** @return      A new allocated variable containing the result.
 **/
ovm_var_t *issdl_div(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2);


/**
 ** Modulo function of the Orchids language.
 ** @param var1  The first operand.
 ** @param var2  The second operand.
 ** @return      A new allocated variable containing the result.
 **/
ovm_var_t *issdl_mod(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2);


/**
 ** Variable cloning function.  This function not necessarily clone
 ** the whole memory representation of the object.  Some constant part
 ** may be shared during the cloning operation.
 ** @param var  The variable to be cloned.
 ** @return     A new allocated copy.
 **/
ovm_var_t *issdl_clone(gc_t *gc_ctx, ovm_var_t *var);

/**
 ** Bitwise logical and.
 ** @param var1  The first operand.
 ** @param var2  The second operand.
 ** @return      A new allocated variable containing the result.
 **/
ovm_var_t *issdl_and(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2);

/**
 ** Bitwise inclusive or.
 ** @param var1  The first operand.
 ** @param var2  The second operand.
 ** @return      A new allocated variable containing the result.
 **/
ovm_var_t *issdl_or(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2);

/**
 ** Bitwise exclusive or.
 ** @param var1  The first operand.
 ** @param var2  The second operand.
 ** @return      A new allocated variable containing the result.
 **/
ovm_var_t *issdl_xor(gc_t *gc_ctx, ovm_var_t *var1, ovm_var_t *var2);

/**
 ** Bitwise complement.
 ** @param var1  The first operand.
 ** @param var2  The second operand.
 ** @return      A new allocated variable containing the result.
 **/
ovm_var_t *issdl_not(gc_t *gc_ctx, ovm_var_t *var);

/**
 ** Return the type name string given a type id.
 ** @param type  The type id.
 ** @return      The type name.
 **/
char *str_issdltype(int type);

void *issdl_get_data(ovm_var_t *val);
size_t issdl_get_data_len(ovm_var_t *val);

void set_ip_resolution(int value);

ovm_var_t *ovm_int_new(gc_t *gc_ctx, long val);
void ovm_int_fprintf(FILE *fp, ovm_int_t *val);

ovm_var_t *ovm_str_new(gc_t *gc_ctx, size_t size);
void ovm_str_fprintf (FILE *fp, ovm_str_t *str);

ovm_var_t *ovm_vstr_new(gc_t *gc_ctx, ovm_var_t *str);
void ovm_vstr_fprintf(FILE *f, ovm_vstr_t *vstr);

ovm_var_t *ovm_regex_new(gc_t *gc_ctx);
void ovm_regex_fprintf(FILE *fp, ovm_regex_t *regex);

ovm_var_t *ovm_ctime_new(gc_t *gc_ctx, time_t tm);
void ovm_ctime_fprintf(FILE *fp, ovm_ctime_t *time);

ovm_var_t *ovm_ipv4_new(gc_t *gc_ctx);
void ovm_ipv4_fprintf(FILE *fp, ovm_ipv4_t *addr);

ovm_var_t *ovm_ipv6_new(gc_t *gc_ctx);
void ovm_ipv6_fprintf(FILE *fp, ovm_ipv6_t *addr);

ovm_var_t *ovm_timeval_new(gc_t *gc_ctx);
void ovm_timeval_fprintf(FILE *fp, ovm_timeval_t *time);

ovm_var_t *ovm_bstr_new (gc_t *gc_ctx, size_t size);
void ovm_bstr_fprintf(FILE *fp, ovm_bstr_t *bstr);

ovm_var_t *ovm_vbstr_new(gc_t *gc_ctx, ovm_var_t *delegate);
void ovm_vbstr_fprintf(FILE *fp, ovm_vbstr_t *str);

ovm_var_t *ovm_uint_new (gc_t *gc_ctx, unsigned long val);

ovm_var_t *ovm_extern_new(gc_t *gc_ctx, void *ptr, ovm_extern_class_t *xclass);

void ovm_uint_fprintf (FILE *fp, ovm_uint_t *val);


size_t add_slashes_compute_length (char *s, size_t len);
void add_slashes (char *s, size_t len, char *t);
/* implement addslashes(), a la PHP; t should contain enough bytes to hold the result:
   consider calling add_slashes_compute_length (s, len) before
   allocating t. */

/**
 ** Print a variable on a stream.
 ** @param fp  The stream on which the variable will be printed.
 ** @param val The variable to print.
 **/
void fprintf_ovm_var(FILE *fp, ovm_var_t *val);

ovm_var_t *ovm_float_new(gc_t *gc_ctx, double val);
void ovm_float_fprintf(FILE *fp, ovm_float_t *val);

ovm_var_t *ovm_snmpoid_new(gc_t *gc_ctx, size_t len);


/**
 ** Duplicate a string of the orchids language, and convert it to a C
 ** string (null terminated).
 ** @param gc_ctx The garbage collector context.
 ** @param str  A virtual machine variable.
 ** @return     A newly allocated C string.
 **/
char *ovm_strdup(gc_t *gc_ctx, ovm_var_t *str);

ovm_var_t *ovm_read_value (ovm_var_t *env, unsigned long var); /* returns value, or NULL */

ovm_var_t *ovm_write_value (gc_t *gc_ctx, ovm_var_t *env, unsigned long var, ovm_var_t *val);
/* returns new environment (env is not modified) */

ovm_var_t *ovm_release_value (gc_t *gc_ctx, ovm_var_t *env, unsigned long var);
/* removes binding for variable number var,
   returns new environment (env is not modified) */

int save_gc_struct (save_ctx_t *sctx, gc_header_t *p);
gc_header_t *restore_gc_struct (restore_ctx_t *rctx);

/* To handle cycles in save/restore: */
int save_postpone (save_ctx_t *sctx, postponed_save f, void *p);
int save_flush (save_ctx_t *sctx);
int restore_forward (restore_ctx_t *rctx, postponed_restore f, void *data);
void restore_forward_free (void *e); /* passed to free_inthash() and clear_inthash on forward_hash */
int restore_flush (restore_ctx_t *rctx);

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
