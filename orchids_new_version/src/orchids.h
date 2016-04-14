/**
 ** @file orchids.h
 ** Main Orchids header.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 ** @author Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
 **
 ** @version 1.1
 ** @ingroup core
 **
 ** @date  Started on: Web Jan 22 16:47:31 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

/**
 ** @defgroup core Orchids Core
 **/
/**
 ** @defgroup output Orchids Output
 **/

#ifndef ORCHIDS_H
#define ORCHIDS_H

#define find_module orchids_find_module

#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

#include "gc.h"

#include "orchids_types.h"
#include "orchids_defaults.h"

#include "safelib.h"
#include "hash.h"
#include "strhash.h"
#include "objhash.h"
#include "inthash.h"
#include "stack.h"
#include "lang.h"
#include "queue.h"
#include "debuglog.h"
#include "timer.h"
#include "slist.h"

#ifdef DMALLOC
#include <dmalloc.h>
extern unsigned long dmalloc_orchids;
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define RETURN_SUCCESS 0

/* should go to orchids_cfg.h */
#define ERR_CFG_PEOF 1
#define ERR_CFG_SECT 2
#define CONFIG_IGNORE_LINE 1

/* orchids module magic number ('ORCH' in an int) */
#define MOD_MAGIC 0x4F524348 /* (0x4843524F with byte ordering) */

#define ORCHIDS_MAJOR_VERSION 0
#define ORCHIDS_MINOR_VERSION 1
#define ORCHIDS_VERSION (ORCHIDS_MAJOR_VERSION | ORCHIDS_MINOR_VERSION)

#define ORCHIDS_MODULE MOD_MAGIC, ORCHIDS_VERSION, -1, NULL, NULL

/** Orchids text banner */
#define ORCHIDS_BANNER \
"                                   _     _     _\n" \
"      __/\\_______    ___  _ __ ___| |__ (_) __| |___   _______/\\__\n" \
" _____\\    /_____|  / _ \\| '__/ __| '_ \\| |/ _` / __| |_____\\    /_____\n" \
"|_____/_  _\\_____| | (_) | | | (__| | | | | (_| \\__ \\ |_____/_  _\\_____|\n" \
"        \\/          \\___/|_|  \\___|_| |_|_|\\__,_|___/         \\/\n"
#define MODNAME_MAX 32
#define MAX_MODULES 256

/* this is a trick to allow the optimizer to
   enable or disable fields in modules (not used yet) */
#define F_NOT_NEEDED ((void *)-1)

#define MODE_ONLINE 0
#define MODE_SYSLOG 1
#define MODE_SNARE  2


/**
 ** @struct config_directive_s
 **   Used for building the configuration tree from the configuration file
 **   (tree represents pseudo-xml file structure).
 **/
/**   @var config_directive_s::directive
 **     A pointer to the directive name.
 **/
/**   @var config_directive_s::args
 **     A pointer to the directive arguments.
 **/
/**   @var config_directive_s::next
 **     A pointer to the next argument. (sibling argument).
 **/
/**   @var config_directive_s::first_child
 **     A pointer to the first child contained by this directive.
 **/
/**   @var config_directive_s::parent
 **     A pointer to the parent directive container.
 **/
/**   @var config_directive_s::file
 **     A pointer to the file name containing the directive.
 **/
/**   @var config_directive_s::line
 **     Line number where directive appear in configuration file
 **/
typedef struct config_directive_s config_directive_t;
struct config_directive_s
{
  char               *directive;
  char               *args;
  config_directive_t *next;
  config_directive_t *first_child;
  config_directive_t *parent;
  const char         *file;
  uint32_t            line;
};

/**
 ** @struct event_s
 **   Type of event built by dissection modules an injected in engine.
 **   (field_id once needed to be sorted in decreasing order, but
 **    apparently no longer needs to).
 **/
/**   @var event_s::field_id
 **     Field identifier in the global field record table.
 **/
/**   @var event_s::value
 **     Value associated to the field.
 **/
/**   @var event_s::next
 **     Next field of event.
 **/
typedef struct event_s event_t;
struct event_s
{
  gc_header_t gc;
  int32_t    field_id;
  ovm_var_t *value;
  event_t   *next;
};

typedef struct orchids_s orchids_t;


/* monotony flags */
typedef int monotony;
#define MONOTONY_MASK 3
#define MONO_UNKNOWN  0
#define MONO_MONO     1
#define MONO_ANTI     2
#define MONO_CONST    (MONO_MONO|MONO_ANTI)
#define MONO_THRASH   4

/**
 ** @struct field_record_s
 **   Used for building global field record array.
 **/
/**   @var field_record_s::active
 **     Activation flag set by the compiler-optimizer.
 **/
/**   @var field_record_s::name
 **     Full field name (ex. ".module.fieldname").
 **/
/**   @var field_record_s::type
 **     static data type (@see lang.h).
 **/
/**   @var field_record_s::desc
 **     Description text field.
 **/
/**   @var field_record_s::id
 **     Field identifier. Used for reverse field name lookup from hash tables.
 **/
/**   @var field_record_s::mono
 **     Monotonicity info for the field.
 **/
/**   @var field_record_s::val
 **     Current field value resolution is not thread-safe.
 **/
typedef struct field_record_s field_record_t;
struct field_record_s
{
  int32_t     active;
  char       *name;
  type_t     *type;
  char       *desc;
  int32_t     id;
  monotony    mono;
  ovm_var_t  *val;
};

typedef struct field_record_table_s field_record_table_t;
struct field_record_table_s
{
  gc_header_t gc;
  size_t num_fields;
  field_record_t *fields;
};


/* Field arrays: used in some modules (generic, timeout, for example) */
typedef struct field_table_s field_table_t;
struct field_table_s
{
  gc_header_t gc;
  size_t nfields;
  ovm_var_t **field_values;
};

field_table_t *new_field_table(gc_t *gc_ctx, size_t nfields);






/*---------------------------------------------*
** data structure for automata representation **
*---------------------------------------------*/

typedef struct transition_s transition_t;
typedef struct state_s state_t;
typedef struct rule_s rule_t;
typedef struct rule_instance_s rule_instance_t;
typedef struct state_instance_s state_instance_t;
typedef unsigned long bytecode_t;

typedef struct input_module_s input_module_t;
typedef struct realtime_input_s realtime_input_t;
typedef struct rule_compiler_s rule_compiler_t;

/**
 ** @struct transition_s
 **   Transition structure.
 **/
/**   @var transition_s::dest
 **     Reference to destination state.
 **/
/**   @var transition_s::required_fields_nb
 **     Number of required fields.
 **/
/**   @var transition_s::required_fields
 **     Static conditions (required fields).
 **/
/**   @var transition_s::eval_code
 **     Evaluation byte code.
 **/
/**   @var transition_s::id
 **     Transition identifier in state.
 **/
/**   @var transition_s::global_id
 **     Transition identifier in rule.
 **/
struct transition_s
{
  state_t *dest;
  size_t   required_fields_nb;
  int32_t *required_fields;
  size_t eval_code_length;
  bytecode_t *eval_code;
  int32_t id;
  int32_t global_id;
  uint32_t flags;
#define TRANS_NO_WAIT 0x1
};


/**
 ** @struct state_s
 **   State structure.
 **/
/**   @var state_s::name
 **     State name, as defined in source file.
 **/
/**   @var state_s::line
 **     Line number in the rule source file.
 **/
/**   @var state_s::action
 **     Actions byte code.
 **/
/**   @var state_s::trans_nb
 **     Transitions array size.
 **/
/**   @var state_s::trans
 **     Transitions array.
 **/
/**   @var state_s::rule
 **     A reference to the rule containing this state.
 **/
/**   @var state_s::flags
 **     Optional flags.
 **/
/**   @var state_s::id
 **     State identifier.
 **/
struct state_s
{
  char         *name;
  int32_t       line;
  size_t actionlength;
  bytecode_t   *action;
  size_t        trans_nb;
  transition_t *trans;
  rule_t       *rule;
  uint32_t      flags;
  /* STATE_COMMIT is set if once we enter the state, we are sure
     that this is the right one, and no other transition is worth
     going through (see engine.c) */
#define STATE_COMMIT 0x1
  /* STATE_EPSILON is set if all outgoing transitions
     are epsilon transitions; otherwise, all outgoing transitions
     are 'expect' transitions, which wait on a future event. */
#define STATE_EPSILON 0x2
  /* STATE_SYNCVARS_ALL_SYNC is set exactly of those states
     where one should check whether to synchronize. */
#define STATE_SYNCVARS_ALL_SYNC 0x4
  int32_t       id;
};

/**
 ** @struct rule_s
 **   Rule structure.
 **/
/**   @var rule_s::filename
 **     File name which contain definition of this rule.
 **/
/**   @var rule_s::lineno
 **     Line number in source file.
 **/
/**   @var rule_s::name
 **     Name of the rule, as defined in source file.
 **/
/**   @var rule_s::state_nb
 **     State array size.
 **/
/**   @var rule_s::state
 **     State array.
 **/
/**   @var rule_s::trans_nb
 **     Total transition number.
 **/
/**   @var rule_s::static_env
 **     Static environment definition (static constant data in rule file).
 **/
/**   @var rule_s::static_env_sz
 **     Static environment size.
 **/
/**   @var rule_s::dynamic_env_sz
 **     Size of dynamic environment. Used for state instance creation.
 **/
/**   @var rule_s::var_name
 **     Dynamic environment variable name.
 **/
/**   @var rule_s::next
 **     Next rule in list (used for complete enumeration).
 **/
/**   @var rule_s::id
 **     Rule identifier.
 **/
/**   @var rule_s::complexity_degree
 **     Degree of the polynomial representation the complexity of the rule,
 **     or ULONG_MAX if exponential.
 **/
struct rule_s
{
  gc_header_t       gc;
  char             *filename;
  time_t	    file_mtime;
  int32_t           lineno;
  char             *name;
  size_t            state_nb;
  state_t          *state;
  size_t            trans_nb;
  ovm_var_t       **static_env;
  size_t            static_env_sz;
  int32_t           dynamic_env_sz; /* do not change type to, e.g., size_t:
				      ovm_write_value(), ovm_release_value() all depend on this
				      to hold at most 32 bits---that should
				      be enough anyway */
  char            **var_name;
  rule_t           *next;
  int32_t           id;
  unsigned long complexity_degree;
  int32_t          *sync_vars;
  size_t           sync_vars_sz;

  /* The following are used at runtime */
  int               flags;
  /* The following two flags are taken from gc.h, and
     are only used at restore() time */
#define RULE_RESTORE_UNKNOWN_FIELD_NAME RESTORE_UNKNOWN_FIELD_NAME
#define RULE_RESTORE_UNKNOWN_PRIMITIVE RESTORE_UNKNOWN_PRIMITIVE
  /* The next one is used for rules whose initial state has
     the STATE_COMMIT flag set */
#define RULE_INITIAL_ALREADY_LAUNCHED 0x80

  objhash_t        *sync_lock;
  struct thread_queue_s *thread_queue;
};

typedef struct env_bind_s env_bind_t;
struct env_bind_s {
  gc_header_t gc;
  int32_t var;
  ovm_var_t *val;
};

typedef struct env_split_s env_split_t;
struct env_split_s {
  gc_header_t gc;
  ovm_var_t *left;
  ovm_var_t *right;
};

typedef struct heap_s heap_t;
typedef struct heap_entry_s heap_entry_t;

/**
 ** @typedef rtaction_cb_t
 **   Real-time action callback function prototype.
 **/
typedef int (*rtaction_cb_t)(orchids_t *ctx, heap_entry_t *he);

struct heap_entry_s {
  timeval_t date;
  int pri;
  rtaction_cb_t cb;
  gc_header_t *gc_data;
  void *data;
};

struct heap_s {
  gc_header_t gc;
  heap_entry_t *entry;
  heap_t *left;
  heap_t *right;
};

struct thread_group_s {
  gc_header_t gc;
  rule_t *rule; /* the 'formula F' in the spec, except it is not a formula,
		   it is a rule */
  size_t nsi; /* number of live state instances that have this pid;
	       The state instances (having this pid) that are live are defined as
	       those whose age is equal to the thread group's age.
	      */
  union {
    uint32_t i;
    unsigned char c[4];
#define THREAD_AGE(pid) (pid)->flags.c[0]
#define THREAD_FLAGS(pid) (pid)->flags.c[1]
#define THREAD_LOCK 0x1
  } flags;
};
typedef struct thread_group_s thread_group_t;

/* A state_instance_t is what is called a thread (I know, it's
   confusing) in the spec.
*/

#define MAX_HANDLES (32)
typedef uint32_t handle_bitmask_t;
#define HANDLE_FAKE_VAR(k) (ULONG_MAX-(k))

struct state_instance_s {
  gc_header_t gc;
  struct thread_group_s *pid;
  state_t *q;  /* pointer to some state
		  in pid->rule->state[] array */
  transition_t *t;
  ovm_var_t *env; /* 'rho' in the spec */
  handle_bitmask_t owned_handles; /* bitmask of handles I do own;
				     the value of handle k is obtained by looking
				     for fake variable HANDLE_FAKE_VAR(k) in env; no value there
				     means the handle in fact does not exist. */
  uint16_t nhandles; /* between 0 and MAX_HANDLES-1: copied lazily;
		      above MAX_HANDLES: copied at each creation of a state_instance_t
		      (costly, but is not meant to happen) */
  unsigned char age;
#define SI_AGE(si) (si)->age
  unsigned char dummy;
  /* The 'lock' component of the spec will be found in rule->sync_lock,
     rule->sync_vars, rule->sync_vars_sz
  */
};

struct thread_queue_elt_s {
  gc_header_t gc;
  struct thread_queue_elt_s *next;
  struct state_instance_s *thread;
  /* thread can be NULL, in which case it is interpreted as BUMP,
     the funny curved arrow symbol in the spec (see top of p.18)
  */
};
typedef struct thread_queue_elt_s thread_queue_elt_t;

struct thread_queue_s {
  gc_header_t gc;
  size_t nelts; /* number of elements, not counting instances of BUMP (NULL) */
  struct thread_queue_elt_s *first;
  struct thread_queue_elt_s *last; /* or NULL if queue is empty */
};
typedef struct thread_queue_s thread_queue_t;

/* --- in engine.c: --- */

/* Create a fresh handle to a mutable data val.
   Return handle number (between 0 and MAX_HANDLES), or TOO_MANY_HANDLES
   if we ran out of handles.
*/
uint16_t create_fresh_handle (gc_t *gc_ctx, state_instance_t *si, ovm_var_t *val);

/* Free a handle for reuse. */
int release_handle (gc_t *gc_ctx, state_instance_t *si, uint16_t k);

/* Get object referenced by handle --- for reading only */
ovm_var_t *handle_get_rd (gc_t *gc_ctx, state_instance_t *si, uint16_t k);

/* Get object referenced by handle --- we may write into the object */
ovm_var_t *handle_get_wr (gc_t *gc_ctx, state_instance_t *si, uint16_t k);

thread_queue_t *new_thread_queue (gc_t *gc_ctx);
void thread_enqueue (gc_t *gc_ctx, thread_queue_t *tq, state_instance_t *si);
void thread_enqueue_all (gc_t *gc_ctx, thread_queue_t *tq, thread_queue_t *all);
 /* thread_enqueue_all(): concatenate the 'all' queue to the end of tq;
    'all' is destroyed after that;
    if last element of tq is BUMP and first element of 'all' is BUMP, remove one of
    them */
state_instance_t *thread_dequeue (gc_t *gc_ctx, thread_queue_t *tq);

/* ------------------------------------------------------ */


/**
 ** @struct rulefile_s
 **   Rule file list for the rule compiler.
 **/
/**   @var rulefile_s::name
 **     Filename.
 **/
/**   @var rulefile_s::next
 **     Next rule file.
 **/
typedef struct rulefile_s rulefile_t;
struct rulefile_s
{
  char       *name;
  rulefile_t *next;
};

struct filestack_s;

/*
** Since the maxline limit is usually 4096 and quoted string can't
** be defined in multiple lines, we can't have a quoted string
** bigger than 4096.
*/
#define QSTRMAX 4096

struct type_heap_s; /* defined in rule_compiler.h */

/**
 ** @struct rule_compiler_s
 ** The main rule compiler context.
 **/
/**   @var rule_compiler_s::gc_ctx
 **     The garbage collector context.
 **/
/**   @var rule_compiler_s::currfile
 **     Current file in compilation.
 **/
/**   @var rule_compiler_s::rules
 **     Total number of rule.
 **/
/**   @var rule_compiler_s::rulenames_hash
 **     ISSDL build-in functions hash table (see lang.c and lang.h).
 **/
/**   @var rule_compiler_s::fields_hash
 **     Field name hash table.
 **/
/**   @var rule_compiler_s::rulenames_hash
 **     Rule name hash table.
 **/
/**   @var rule_compiler_s::statenames_hash
 **     State names hash table for current rule in compilation.
 **     (reseted at rule commit by compiler_reset()).
 **/
/**   @var rule_compiler_s::rule_env
 **     Rule environment hash table for current rule in compilation.
 **     (reseted at rule commit by compiler_reset()).
 **/
/**   @var rule_compiler_s::statics
 **     Static resources array for current rule in compilation.
 **     (reseted at rule commit by compiler_reset()).
 **/
/**   @var rule_compiler_s::statics_nb
 **     Number of elements in 'statics' array.
 **/
/**   @var rule_compiler_s::statics_sz
 **     Allocated size of the 'statics' array.
 **/
/**   @var rule_compiler_s::dyn_var_name
 **     Dynamic environment variable for current rule in compilation.
 **     (reseted at rule commit by compiler_reset()).
 **/
/**   @var rule_compiler_s::dyn_var_name_nb
 **     Number of elements in 'dyn_var_name' array.
 **/
/**   @var rule_compiler_s::dyn_var_name_sz
 **     Allocated size of the 'dyn_var_name' array.
 **/
/**   @var rule_compiler_s::first_rule
 **     First rule in the list.
 **/
/**   @var rule_compiler_s::last_rule
 **     Last rule in the list
 **/
/**   @var rule_compiler_s::returns
 **     Stack (list) of lists of 'return expr' expression nodes
 **     Each level of the stack is for a different scope.
 **/
/**   @var rule_compiler_s::type_stack
 **     Stack of nodes whose type we do not know yet
 **/
struct rule_compiler_s
{
  gc_header_t       gc;
  gc_t             *gc_ctx;
  size_t            nprotected;
  size_t            maxprotected;
  gc_header_t      **protected;
  ovm_var_t *currfile; /* really ovm_str_t * */
  /* For the lex scanner: */
  void             *scanner;
  unsigned int      issdllineno;
  char             *issdlcurrentfile; /* the current file; should be distinguished
					 from STR(...->currfile).  E.g., in file
					 "a.rule", preprocessed through cpp
					 and after a line command '# 1 "b.rule"',
					 issdlcurrentfile would be a.rule,
					 with STR(...->currfile) would be b.rule. */
  FILE             *issdlin;
  SLIST_HEAD(filestack, struct filestack_s) issdlfilestack;
  char              qstr_buf[QSTRMAX];
  char             *qstr_ptr;
  size_t            qstr_sz;
  /* */
  int32_t           rules;
  strhash_t        *functions_hash;
  strhash_t        *fields_hash;
  strhash_t        *rulenames_hash;
  strhash_t        *statenames_hash;
  strhash_t        *rule_env;
  ovm_var_t       **statics;
  int32_t           statics_nb;
  int32_t           statics_sz;
  char            **dyn_var_name;
  int32_t           dyn_var_name_nb;
  int32_t           dyn_var_name_sz;
  rule_t           *first_rule;
  rule_t           *last_rule;
  struct node_expr_s *returns;
  /* Type-checking: */
  struct type_heap_s *type_stack;
  int nerrors;
  int verbose;
};


/**
 ** @typedef ovm_func_t
 ** ISSDL built-in function type declaration
 **/
typedef void (*ovm_func_t)(orchids_t *ctx, state_instance_t *state, void *data);


/**
 ** @struct issdl_function_s
 **   ISSDL built-in functions records
 **/
/**   @var issdl_function_s::func
 **     A pointer to the function implementation.
 **/
/**   @var issdl_function_s::id
 **     The code identifier of function (used in byte code).
 **     XXX: should be declared dynamically.
 **/
/**   @var issdl_function_s::name
 **     Name of the function. (symbol name for the compiler).
 **/
/**   @var issdl_function_s::args_nb
 **     Number of argument (for a little type/prototype checking).
 **/
/**   @var issdl_function_s::sigs
 **     List of typing signatures, ending in NULL
 **     Each signature is a table [return-type, arg-type1, ..., arg-typen]
 **     where n = arg_nb
 **     Types are NULL, &t_int, &t_bstr, &t_str, etc.,
 **     plus &t_any (wildcard arg type)
 **/
/**   @var issdl_function_s::cm
 **     Auxiliary function meant to compute monotonicity status of expression e.
 **     Can be left NULL if result is always MONO_UNKNOWN.
 **     e is a NODE_CALL.
 **     args[] is a table of monotonicity info, one for each argument.
 **/
/**   @var issdl_function_s::desc
 **     Function description (for a little help).
 **/
/**   @var issdl_function_s::data
 **     Closure data.
 **/
struct node_expr_s;

typedef monotony (*monotony_apply) (rule_compiler_t *ctx,
				    struct node_expr_s *e,
				    monotony args[]);
typedef struct issdl_function_s issdl_function_t;
struct issdl_function_s
{
  ovm_func_t func;
  int32_t    id;
  char      *name;
  int32_t    args_nb;
  type_t  ***sigs;
  monotony_apply cm;
  char      *desc;
  void      *data;
};

/* Possible values for compute_monotony field
   (non-exclusive): */
monotony m_const (rule_compiler_t *ctx, struct node_expr_s *e,
		  monotony args[]); /* for maps returning a constant, no side-effect */
monotony m_const_thrash (rule_compiler_t *ctx, struct node_expr_s *e,
			 monotony args[]); /* for maps returning a constant,
					      but may have side-effects (printing, exiting,
					      etc.) */
/* The m_unknown_<n> functions are for maps of <n> arguments,
   returning a value that may increase or decrease, we do not know;
   but at least if you call it twice on the same argument, you get
   the same result (no side-effect) */
monotony m_unknown_1 (rule_compiler_t *ctx, struct node_expr_s *e,
		      monotony args[]);
monotony m_unknown_2 (rule_compiler_t *ctx, struct node_expr_s *e,
		      monotony args[]);
/* The m_unknown_<n>_thrash functions are for maps of <n> arguments,
   returning a value that may increase or decrease, we do not know,
   and with possible side-effects; */
monotony m_unknown_1_thrash (rule_compiler_t *ctx, struct node_expr_s *e,
			     monotony args[]);
monotony m_unknown_2_thrash (rule_compiler_t *ctx, struct node_expr_s *e,
			     monotony args[]);
monotony m_random (rule_compiler_t *ctx, struct node_expr_s *e,
		   monotony args[]); /* for maps returning a value
					that may increase or decrease,
					we do not know; additional,
					calling them twice may return
					different results; but they do not have
					side-effects */
monotony m_random_thrash (rule_compiler_t *ctx, struct node_expr_s *e,
			  monotony args[]); /* for maps returning a value
					       that may increase or decrease,
					       we do not know; additional,
					       calling them twice may return
					       different results; and they may
					       have side-effects */

typedef struct mod_entry_s mod_entry_t;

/**
 ** @typedef dissect_t
 **   Dissector function pointer implemented in modules and registered
 **   into the module manager.
 **/
typedef int (*dissect_t)(orchids_t *ctx, mod_entry_t *mod, struct event_s *e, void *data,
			 int dissection_level);

/** @typedef pre_dissect_t
 ** Preconfiguration function for dissectors.
 **/
typedef void *(*pre_dissect_t)(orchids_t *ctx, mod_entry_t *mod,
			       char *parent_modname,
			       char *cond_param_str,
			       int cond_param_size);

/** @typedef save_t
 ** Function meant to save the current state of a module.
 **/
typedef int (*save_t)(save_ctx_t *sctx, mod_entry_t *mod, void *data);

/** @typedef restore_t
 ** Function meant to restore the current state of a module.
 **/
typedef int (*restore_t)(restore_ctx_t *rctx, mod_entry_t *mod, void *data);

/**
 ** @struct conditional_dissector_record_s
 **   This structure is used to wrap a conditional dissector,
 **   the pointer to module responsible for the dissection and
 **   the abstract data for the dissector.  This structure was added
 **   during the modularization of Orchids, more precisely when Orchids
 **   started to build modules as shared objects.  A module configuration was
 **   initially held in the ::input_module_s module structure.  This was
 **   incompatible with security measures in newer OSes.  See for example:
 **   http://www.akkadia.org/drepper/selinux-mem.html
 **/
/** @var conditional_dissector_record_s::dissect
 **   A pointer to the dissector.
 **/
/** @var conditional_dissector_record_s::mod
 **   A pointer to the module which requested the dissector registration.
 **/
/** @var conditional_dissector_record_s::data
 **   Abstract data which will be passed to the dissector.
 **/
typedef struct conditional_dissector_record_s conditional_dissector_record_t;
struct conditional_dissector_record_s {
  dissect_t  dissect;
  mod_entry_t *mod;
  void     *data;
};

/**
 ** @struct mod_entry_s
 **   A module entry registered in the module manager.
 **/
/**   @var mod_entry_s::num_fields
 **     Number of fields registered by this module.
 **/
/**   @var mod_entry_s::first_field_pos
 **     First fields position in global table.
 **/
/**   @var mod_entry_s::config
 **     A void pointer to module configuration structure.
 **/
/**   @var mod_entry_s::sub_dissectors
 **     Conditional, pattern-driven, sub-dissectors.
 **/
/**   @var mod_entry_s::dissect
 **     Unconditional registered sub-dissectors for this module.
 **/
/**   @var mod_entry_s::data
 **     Data passed to the unconditional sub-dissector.
 **/
/**   @var mod_entry_s::mod
 **     A reference to the loaded module
 **/
/**   @var mod_entry_s::posts
 **     Some values for statistics
 **/
/**   @var mod_entry_s::mod_id
 **     Module identifier.
 **/
/**   @var mod_entry_s::dlhandle
 **     The handle returned by dlopen().
 **/
struct mod_entry_s
{
  int32_t                num_fields;
  int32_t                first_field_pos;
  void                  *config;
  hash_t                *sub_dissectors;
  dissect_t              dissect;
  mod_entry_t           *dissect_mod;
  void                  *data;
  input_module_t        *mod;
  unsigned long          posts;
  int32_t                mod_id;
  void                  *dlhandle;
};


/**
 ** @typedef hook_cb_t
 **   Hook callback function type.
 **/
typedef int (*hook_cb_t)(orchids_t *ctx,
                         mod_entry_t *mod,
                         void *data,
                         event_t *e);


/**
 ** @struct hook_list_elmt_s
 **   Hook list element structure.  This structure is used
 ** to register callback function of type ::hook_cb_t in a list.
 ** This type was introduced to add the post event hook
 ** orchids_s::post_evt_hook_list .
 **/
typedef struct hook_list_elmt_s hook_list_elmt_t;
struct hook_list_elmt_s {
  hook_cb_t cb;
  mod_entry_t *mod;
  void *data;
  SLIST_ENTRY(hook_list_elmt_t) hooklist;
};


/**
 ** @struct preproc_cmd_s
 **   Preprocessor command bounded to a file suffix.
 **/
typedef struct preproc_cmd_s preproc_cmd_t;
struct preproc_cmd_s {
  SLIST_ENTRY(preproc_cmd_t)  preproclist;
  char *suffix;
  char *cmd;
};


typedef void (*report_cb_t)(orchids_t *ctx, mod_entry_t *mod, void *data, state_instance_t *state);

typedef struct reportmod_s reportmod_t;

/**
 ** @struct reportmod_s
 ** Report module structure, used to dispatch a report event
 **/
/**   @var reportmod_s::cb
 **     The callback to execute.
 **/
/**   @var reportmod_s::mod
 **     The module entry who registered this callback.
 **/
/**   @var reportmod_s::data
 **     Arbitraty data that may be used by the callback.
 **/
/**   @var reportmod_s::list
 **     The list elements.
 **/
struct reportmod_s {
  report_cb_t cb;
  mod_entry_t *mod;
  void *data;
  SLIST_ENTRY(reportmod_t) list;
};


/**
 ** @struct orchids_s
 **   Main program context structure.
 **   Contain all needed data for global operation.
 **/
/**   @var orchids_s::gc_ctx
 **     The garbage collection context.  Needed when calling
 **     any function that may allocate memory using gc_alloc().
 **/
/**   @var orchids_s::one
 **     The global constant '1'.  This is used frequently by the
 **     virtual machine, and should not be reallocated each time.
 **/
/**   @var orchids_s::zero
 **     The global constant '0'.  This is used frequently by the
 **     virtual machine, and should not be reallocated each time.
 **/
/**   @var orchids_s::empty_string
 **     The empty string, as a global constant.
 **     This is used frequently by the
 **     virtual machine, and should not be reallocated each time.
 **/
/**   @var orchids_s::start_time
 **     Start time of the program (for uptime and cpu usage statistics).
 **/
/**   @var orchids_s::config_file
 **     Configuration file used for this context.
 **/
/**   @var orchids_s::mods
 **     Input module array (should replace mod_list).
 **/
/**   @var orchids_s::loaded_modules
 **     Number of loaded modules.
 **/
/**   @var orchids_s::last_poll
 **     Date of the last poll.
 **/
/**   @var orchids_s::realtime_handler_list
 **     List of real-time handlers. (and associated file/socket descriptor).
 **/
/**   @var orchids_s::maxfd
 **     Highest file descriptor, for the select() call in
 **     event_dispatcher_main_loop().
 **/
/**   @var orchids_s::fds
 **     Descriptor set, for the select() call in
 **     event_dispatcher_main_loop().
 **/
/**   @var orchids_s::cfg_tree
 **     Configuration tree root.
 **/
/**   @var orchids_s::num_fields
 **     Number of registered field.
 **/
/**   @var orchids_s::global_fields
 **     Global field array.
 **/
/**   @var orchids_s::events
 **     Total number of injected events.
 **/
/**   @var orchids_s::rule_compiler
 **     Rule compiler context.
 **/
/**   @var orchids_s::poll_period
 **     Period for polling handlers.
 **/
/**   @var orchids_s::rulefile_list
 **     First rule file in linked-list.
 **/
/**   @var orchids_s::last_rulefile
 **     Last rule file in linked-list.
 **/
/**   @var orchids_s::first_rule_instance
 **     First rule instance in linked-list.
 **/
/**   @var orchids_s::last_rule_instance
 **     Last rule instance in linked-list.
 **/
/**   @var orchids_s::retrig_list
 **     Global retrig list : list of all active state instance.
 **/
/**   @var orchids_s::ovm_stack
 **     Orchids virtual machine stack.
 **/
/**   @var orchids_s::vm_func_tbl
 **     ISSDL built-in functions array.
 **/
/**   @var orchids_s::vm_func_tbl_sz
 **     ISSDL built-in functions array size.
 **/
/**   @var orchids_s::off_line_mode
 **     Off-line input file type.  Possible values are:
 **       - #MODE_ONLINE: default online analysis
 **       - #MODE_SYSLOG: default when -f ar -r is used
 **                        (register textfile and syslog modules)
 **       - #MODE_SNARE:  register textfile and snare module.
 **/
/**   @var orchids_s::off_line_input_file
 **     Off-line input file.
 **/
/**   @var orchids_s::current_tail
 **     A pointer to the current thread being processed.  By following
 **     the linked list of threads, it correspond to the all remaining
 **     threads to be processed in the queue, for the current event
 **     loop (this exclude the newly created threads).
 **/
/**   @var orchids_s::cur_retrig_qh
 **     Head of the current waiting threads queue to re-trigger.
 **/
/**   @var orchids_s::cur_retrig_qt
 **     Tail of the current waiting threads queue to re-trigger.
 **/
/**   @var orchids_s::new_qh
 **     New thread queue head.
 **/
/**   @var orchids_s::new_qt
 **     New thread queue tail.
 **/
/**   @var orchids_s::retrig_qh
 **     Queue head of threads to re-trig.
 **/
/**   @var orchids_s::retrig_qt
 **     Queue tail of threads to re-trig.
 **/
/**   @var orchids_s::active_event_head
 **     Active event queue head.
 **/
/**   @var orchids_s::active_event_tail
 **     Active event queue queue.
 **/
/**   @var orchids_s::preconfig_time
 **     Pre-configuration end time.
 **/
/**   @var orchids_s::compil_time
 **     Compilation end time.
 **/
/**   @var orchids_s::postconfig_time
 **     Post-configuration end time.
 **/
/**   @var orchids_s::post_evt_hook_list
 **     Post event injection hook list.
 **/
/**   @var orchids_s::daemon
 **     A boolean value for requesting the daemonization or not.
 **/
/**   @var orchids_s::postcompil_time
 **     Post-compilation end time.
 **/
/**   @var orchids_s::evt_fb_fp
 **     Event feedback stream.  Every processed event by orchids are printed
 **     on this stream (if not NULL).  This is for debugging / instrumentation
 **     purposes.
 **/
/**   @var orchids_s::runtime_user
 **     The user in which Orchids will drop its privileges, after its
 **     configuration complete (and if it was started as root).
 **/
/**   @var orchids_s::reports
 **     Total number of report generated since startup.
 **/
/**   @var orchids_s::last_rule_act
 **     Last rule activity: the last time when a rule was inserted or removed.
 **/
/**   @var orchids_s::rtactionlist
 **     The list of real-time actions.  These actions are registered and
 **     will be executed at the scheduled time.
 **/
/**   @var orchids_s::cur_loop_time
 **     The time of the current loop.  This was variable is used to
 **     "cache" the time because gettimeofday() system call may me
 **     slow on some systems.  Since there is exactly one loop per
 **     event processed, this time may be used to time-stamp the
 **     event; there is no possible confusion.
 **/
/**   @var orchids_s::modules_dir
 **     The directory where Orchids modules are installed.
 **/
/**   @var orchids_s::lockfile
 **     The Orchids daemon lock file.  This is used to prevent
 **     accidental multiple instance of the daemon.
 **/
struct orchids_s
{
  gc_t *gc_ctx;
  ovm_var_t *one;
  ovm_var_t *zero;
  ovm_var_t *minusone;
  ovm_var_t *empty_string;
  char *orchids_app_path;
  timeval_t    start_time;
  char        *config_file;
  mod_entry_t  mods[MAX_MODULES];
  int32_t      loaded_modules;
  time_t              last_poll;
  realtime_input_t   *realtime_handler_list;
  int                 maxfd;
  fd_set_t            fds;
  config_directive_t *cfg_tree;
  /*int32_t             num_fields; suppressed: now global_fields->num_fields */
  /* field_record_t     *global_fields; replaced by the following: */
  field_record_table_t *global_fields;
  size_t             events;
  rule_compiler_t    *rule_compiler;
  timeval_t           poll_period;
  rulefile_t         *rulefile_list;
  rulefile_t         *last_rulefile;
  char               *save_file;
  timeval_t          save_interval;
#ifdef OBSOLETE
  rule_instance_t    *first_rule_instance;
  rule_instance_t    *last_rule_instance;
  state_instance_t   *retrig_list;
#endif
  lifostack_t        *ovm_stack;
  issdl_function_t   *vm_func_tbl;
  int32_t             vm_func_tbl_sz;
  int32_t             off_line_mode;
  char               *off_line_input_file;
  int                 verbose;
  bool_t              daemon;
  bool_t              actmon;

  event_t *current_event;

  timeval_t  preconfig_time;
  timeval_t  postconfig_time;
  timeval_t  compil_time;
  timeval_t  postcompil_time;

#ifdef OBSOLETE
  FILE *evt_fb_fp;
#endif

  /* global temporal information container */
  /* XXX: Move this into mod_period */
  strhash_t        *temporal;
  strhash_t        *xclasses;

  char *runtime_user;

  rusage_t ru;
  pid_t pid;

  size_t reports;

  timeval_t last_rule_act;

  heap_t *rtactionlist;

#ifdef ENABLE_PREPROC
  char *default_preproc_cmd;
  SLIST_HEAD(preproclist, preproc_cmd_t) preproclist;
#endif /* ENABLE_PREPROC */

  timeval_t cur_loop_time;

  char *modules_dir;
  char *lockfile;

  SLIST_HEAD(preevthooklist, hook_list_elmt_t) pre_evt_hook_list;
  SLIST_HEAD(postevthooklist, hook_list_elmt_t) post_evt_hook_list;
  SLIST_HEAD(list, reportmod_t) reportmod_list;
};


/**
 ** @typedef dir_handler_t
 **   Directive handler function pointer type.
 **/
typedef void (*dir_handler_t)(orchids_t *ctx, mod_entry_t *mod,
			      config_directive_t *dir);


/**
 ** @struct mod_cfg_cmd_s
 **   Module configuration command definition.
 **/
/**   @var mod_cfg_cmd_s::name
 **     Name of the command. (in the configuration file).
 **/
/**   @var mod_cfg_cmd_s::cmd
 **     Command handler.
 **/
/**   @var mod_cfg_cmd_s::help
 **     Command description.
 **/
typedef struct mod_cfg_cmd_s mod_cfg_cmd_t;
struct mod_cfg_cmd_s
{
  char          *name;
  dir_handler_t  cmd;
  char          *help;
};

/**
 ** @typedef pre_config_t
 **   Pre-configuration handler function pointer type.
 **/
typedef void *(*pre_config_t)(orchids_t *ctx, mod_entry_t *mod);


/**
 ** @typedef post_config_t
 **   Post-configuration handler function pointer type.
 **/
typedef void (*post_config_t)(orchids_t *ctx, mod_entry_t *mod);

/**
 ** @typedef post_compil_t
 **   Post-compilation handler function pointer type.
 **/
typedef void (*post_compil_t)(orchids_t *ctx, mod_entry_t *mod);


/**
 ** @struct input_module_s
 **   Input Module Structure.
 **   These modules extract useful info, pre-process and prepare them
 **   to be proceeded by the analysis engine.
 **/
/**   @var input_module_s::magic
 **     Magic number. Useful for dynamic shared objects checking.
 **     See MOD_MAGIC.
 **/
/**   @var input_module_s::version
 **     Version of Orchids. See ORCHIDS_VERSION.
 **/
/**   @var input_module_s::flags
 **     Input module flags
 **     only MODULE_DISSECTABLE provided for now, means
 **     that the last two fields defined by the module
 **     are: (next-to-last) a string, used as a key for
 **     conditional dissection; (last) a string, uint, int,
 **     ipv4, or ipv6 field to be dissected.
 **/
/**   @var input_module_s::name
 **     Module name.
 **/
/**   @var input_module_s::license
 **     The license of the module.
 **/
/**   @var input_module_s::dependencies
 **     Module dependencies.
 **/
/**   @var input_module_s::cfg_cmds
 **     List of configuration directives for module specific parameters.
 **/
/**   @var input_module_s::pre_config
 **     Generic function for module initialisation/post-registration.
 **     should allocate some memory for 'config' ptr,
 **     should register hooks ans callbacks,
 **     allocate some memory if needed,
 **     pre-compute some stuffs...
 **     and finally, return the module configuration structure.
 **/
/**   @var input_module_s::post_config
 **     Post-configuration function.
 **/
/**   @var input_module_s::post_compil
 **     Function to be executed after the rule compilation.
 **     This field may be NULL if there is nothing to execute.
 **/
/**   @var input_module_s::pre_dissect
 **     Function to be executed at each time a new conditional
 **     dissector is registered that uses this module as dissector.
 **     This field may be NULL if there is nothing to execute.
 **/
/**   @var input_module_s::dissect_fun
 **     Dissection function for module; can be NULL if no dissector
 **/
/**   @var input_module_s::dissect_type
 **     Expected input type for dissection function
 **/
struct input_module_s
{
  uint32_t               magic;
  uint32_t               version;
  uint32_t               flags;
#define MODULE_DISSECTABLE 0x1
  char                  *name;
  char                  *license;
  char                 **dependencies;
  mod_cfg_cmd_t         *cfg_cmds;
  pre_config_t           pre_config;
  post_config_t          post_config;
  post_compil_t          post_compil;
  pre_dissect_t          pre_dissect;
  dissect_t		 dissect_fun;
  type_t                *dissect_type;
  save_t                 save_fun;
  restore_t              restore_fun;
};




/**
 ** @typedef realtime_callback_t
 **   Real-time callback function pointer type. This callback will be triggered
 **   when there is activity on associated descriptor.
 **/
typedef int (*realtime_callback_t)(orchids_t *ctx, mod_entry_t *mod, int fd, void *data);


/**
 ** @struct realtime_input_s
 **   List of registered callback for real-time event creation
 **/
/**   @var realtime_input_s::next
 **     Next real-time input.
 **/
/**   @var realtime_input_s::data
 **     Data passed to the callback.
 **/
/**   @var realtime_input_s::cb
 **     The callback itself.
 **/
/**   @var realtime_input_s::fd
 **     File/socket descriptor used for real-time input.
 **/
/**   @var realtime_input_s::mod_id
 **     This unique identifier of this registered module.
 **/
struct realtime_input_s
{
  realtime_input_t    *next;
  void                *data;
  realtime_callback_t  cb;
  int                  fd;
  int32_t              mod_id;
};


/**
 ** @typedef poll_callback_t
 **   Callback for polled inputs.
 **/
typedef int (*poll_callback_t)(orchids_t *ctx, mod_entry_t *mod, void *data);


/**
 ** @struct field_s
 **   Used for field declaration in modules
 **/
/**   @var field_s::name
 **     Full field name. (ex. ".module.field").
 **/
/**   @var field_s::type
 **     Field data type. (ISSDL type, @see lang.h).
 **/
/**   @var field_s::mono
 **     Monotonicity information.
 **/
/**   @var field_s::desc
 **     Field description.
 **/
typedef struct field_s field_t;
struct field_s
{
  char    *name;
  type_t  *type;
  monotony mono;
  char    *desc;
};



/**
 ** @typedef mod_func_t
 **   Function prototype for optional inter-module function calls.
 **/
typedef int (*mod_func_t)(orchids_t *ctx, mod_entry_t *mod, void *params);



/*
** DOT DISPLAY flags.
*/

#define DOT_RETRIGLIST   1

/*----------------------------------------------------------------------------*
** functions                                                                 **
*----------------------------------------------------------------------------*/

/* orchids_cfg.c */

/**
 ** Display a configuration tree on a stream.
 **
 ** @param fp    The output stream.
 ** @param root  The root configuration directive.
 **/
void
fprintf_cfg_tree(FILE *fp, config_directive_t *root);

/**
 ** Proceed to the pre-configuration.
 ** Pre-configuration build the configuration tree from the configuration file,
 ** and interprets it by executing registered directive handlers.
 **
 ** @param ctx  Orchids application context.
 **/
void
proceed_pre_config(orchids_t *ctx);

/**
 ** Proceed to the post-configuration.
 ** Post-configuration execute post-config handler of registered modules.
 ** Actions are executed AFTER the configuration file but BEFORE
 ** rule compilation.  For example, a module can register new fields
 ** into the compiler (e.g. virtual modules).
 **
 ** @param ctx  Orchids application context.
 **/
void
proceed_post_config(orchids_t *ctx);

/**
 ** Proceed to the post-compilation actions.
 ** Post-compilation execute post-compilation handler of registered modules.
 ** Actions are executed AFTER the rule compilation.
 **
 ** @param ctx  Orchids application context.
 **/
void
proceed_post_compil(orchids_t *ctx);

char *dir_parse_id (orchids_t *ctx, const char *file, uint32_t line,
		    char *argstring, char **value);
char *dir_parse_string (orchids_t *ctx, const char *file, uint32_t line,
			char *argstring, char **value);


void
fprintf_cfg_mib(FILE *fp, config_directive_t *section);

/**
 ** Display the list of all known directives, given the registered modules.
 **
 ** @param fp   The output stream.
 ** @param ctx  Orchids application context.
 **/
void
fprintf_directives(FILE *fp, orchids_t *ctx);


/* crc32.c */

/**
 ** Compute the 32-bits Cyclic Redundancy Checksum (CRC-32) of data.
 **
 ** @param crc  Last computed crc. used to compute crc from a read buffer.
 ** @param buf  The buffer that contain the source data.
 ** @param len  The buffer length.
 **
 ** @return The new computed CRC.
 **/
unsigned int crc32(unsigned int crc, char *buf, int len);

/* string_util.c */
size_t my_strspn(const char *pos, const char *eot, size_t n);
size_t my_strcspn(const char *pos, const char *eot, size_t n);
size_t get_next_uint(const char *istr, unsigned long *i, size_t n);
size_t get_next_token(const char *pos, int c, size_t n);


/* issdl.l */
/*
void
set_lexer_context(rule_compiler_t *ctx);
*/

/**
 ** ISSDL lexer entry-point.
 **/
union YYSTYPE;
int issdllex(union YYSTYPE *yylval, rule_compiler_t *ctx, void *yyscanner); // yylex()

void issdlerror(rule_compiler_t *ctx, char *s); // yyerror()

/* issdl.y */
/*
void
set_yaccer_context(rule_compiler_t *ctx);
*/

/**
 ** ISSDL parser (yaccer) entry-point.
 **
 ** @return The error code.
 **/
int issdlparse(void *__compiler_ctx_g); /* really of type rule_compiler_t * */

/* rule_compiler.c */

type_t *stype_from_string (gc_t *gc_ctx, char *type_name, int forcenew,
			   unsigned char tag);
objhash_t *create_sync_lock_if_needed (gc_t *gc_ctx, rule_t *rule);
rule_t *install_restored_rule (rule_compiler_t *ctx, rule_t *rule);

/* ovm.h */

/**
 ** Orchids virtual machine entry point.
 **
 ** @param ctx       Orchids application context.
 ** @param s         State instance for the execution context.
 ** @param bytecode  Byte code to execute.
 **
 ** @return  value computed (bytecode must return an object of type T_INT)
 **/

int ovm_exec_trans_cond (orchids_t *ctx, state_instance_t *s, bytecode_t *bytecode);
#ifdef OBSOLETE
int ovm_exec_expr (orchids_t *ctx, state_instance_t *s, bytecode_t *bytecode);
#endif

/**
 ** Orchids virtual machine entry point.
 **
 ** @param ctx       Orchids application context.
 ** @param s         State instance for the execution context.
 ** @param bytecode  Byte code to execute.
 **
 ** @return  nothing
 **/
void
ovm_exec_stmt(orchids_t *ctx, state_instance_t *s, bytecode_t *bytecode);

/**
 ** Convert an ovm opcode into the mnemonic name.
 **
 ** @param opcode  The numeric opcode to convert.
 **
 ** @return  The corresponding mnemonic name string.
 **/
const char *get_opcode_name(bytecode_t opcode);

/**
 ** Disassemble and display an ovm byte code.
 ** @param fp        Output stream.
 ** @param bytecode  Byte code to disassemble.
 ** @param length    Length of byte code to disassemble.
 **/
void fprintf_bytecode(FILE *fp, bytecode_t *bytecode, size_t length);
void fprintf_bytecode_short(FILE *fp, bytecode_t *bytecode, size_t length);


/**
 ** Display a raw byte code dump in hexadecimal.
 **
 ** @param fp    Output stream.
 ** @param code  Byte code to display.
 **/
void fprintf_bytecode_dump(FILE *fp, bytecode_t *code);

/**
 ** Generic bytecode processor
 **
 ** @param bytecode            Byte code to process.
 ** @param length              Length of byte code to process.
 ** @param review              Function called on each instruction;
 **                this function takes a pointer bc to an instruction,
 **                the number of byte codes the instruction takes,
 **                the address where all byte codes start (equal to
 **                the bytecode parameter to review_bytecode()),
 **                and a pointer data to user-defined data.
 ** @param data                User-defined data, passed to review().
 **/
int review_bytecode(bytecode_t *bytecode, size_t length,
		    int (*review) (bytecode_t *bc, size_t sz,
				   bytecode_t *start,
				   void *data),
		    void *data);


/*
  misc.c
 */

void
tvdiff(struct timeval *dst, const struct timeval *ref1, const struct timeval *ref2);
void
fprintf_uptime(FILE *fp, time_t uptime);
int
snprintf_uptime(char *str, size_t size, time_t uptime);
int
snprintf_uptime_short(char *str, size_t size, time_t uptime);



/* Some lock stuffs */


/* lang.h */
/* XXX: separate structure and prototype definitions and
   move this in lang files */
/**
 ** Accessor to the global built-in Orchids function table.
 ** @return  A pointer to the function table.
 **/
issdl_function_t *
get_issdl_functions(void);

/**
 ** Print an Orchids value.
 ** @param fp  The file on which printing must occur
 ** @param ctx The global Orchids context
 ** @param val The value to print
 **/
void fprintf_issdl_val(FILE *fp, const orchids_t *ctx, ovm_var_t *val);

/**
 ** Register a new function in the Orchids language.
 ** This function may be used by loadable module to register
 ** new functions.
 ** @param ctx   A pointer to the Orchids application context.
 ** @param func  A pointer to the C function which handle this
 **              Orchids function.
 ** @param name  The name of the function in the Orchids language.
 ** @param arity The number of arguments of the function.
 ** @param desc  A short description of the function.
 ** @param data  Closure data.
 **/
void register_lang_function(orchids_t *ctx,
			    ovm_func_t func,
			    const char *name,
			    int arity,
			    const type_t ***sigs,
			    monotony_apply cm,
			    const char *desc,
			    void *data);

/**
 ** Print the table of all registered functions on a stream.
 ** @param fp   The output stream.
 ** @param ctx  A pointer to the Orchids application context.
 **/
void
fprintf_issdl_functions(FILE *fp, orchids_t *ctx);


/**
 ** Register built-in functions of the Orchids language.
 ** @param ctx  A pointer to the Orchids application context.
 **/
void
register_core_functions(orchids_t *ctx);



/*
** Debugging stuff
******/

#ifdef ORCHIDS_DEBUG

#define DPRINTF(args) \
  do { \
      printf("%24s:%-4d: ", __FILE__, __LINE__); \
      printf args ; \
  } while (0)

#define DFPRINTF(args) fprintf args

#else /* DEBUG */
# define DPRINTF(args)
# define DFPRINTF(args)
#endif /* DEBUG */

/* In orchids_api.c: */
char *adjust_path (orchids_t *ctx, char *path); /* expand initial ':' if necessary */
char *orchids_realpath (orchids_t *ctx, char *file_name,
			char *resolved_name);
event_t *new_event (gc_t *gc_ctx, int32_t field_id, ovm_var_t *val,
		    event_t *event);

/* In lang.c: */
struct ovm_extern_class_s;
void register_extern_class (orchids_t *ctx, struct ovm_extern_class_s *xclass);

char *time_convert(char *str, char *end, struct timeval *tv);
char *orchids_atoi (char *str, size_t len, long *result);
char *orchids_atoui (char *str, size_t len, unsigned long *result);
char *orchids_atof (char *str, size_t len, double *result);

/* In db.c: */
void db_init (gc_t *gc_ctx);

struct db_map;
typedef struct db_map db_map;
db_map *db_union (gc_t *gc_ctx, int nfields, db_map *m1, db_map *m2);
db_map *db_diff (gc_t *gc_ctx, int nfields, db_map *m1, db_map *m2);
int db_included (int nfields, db_map *m1, db_map *m2);

/* slightly slower functions, not requiring an nfields argument: */
db_map *db_union_lazy (gc_t *gc_ctx, db_map *m1, db_map *m2);
db_map *db_diff_lazy (gc_t *gc_ctx, db_map *m1, db_map *m2);
int db_included_lazy (db_map *m1, db_map *m2);

unsigned long issdl_hash (ovm_var_t *var);

struct db_proj_spec;
db_map *db_proj (gc_t *gc_ctx, struct db_proj_spec *spec, db_map *m);

struct db_filter_spec;
db_map *db_filter (gc_t *gc_ctx, struct db_filter_spec *spec, db_map *m);

struct db_join_spec;
db_map *db_join (gc_t *gc_ctx, struct db_join_spec *spec,
		 db_map *m1, db_map *m2);

db_map *db_empty (gc_t *gc_ctx);
db_map *db_singleton (gc_t *gc_ctx, ovm_var_t **tuple, int nfields);

int db_sweep (gc_t *gc_ctx, int nfields, db_map *m,
	      int (*do_sweep) (gc_t *gc_ctx, int nfields, db_map *singleton, void *data),
	      void *data);
db_map *db_collect (gc_t *gc_ctx, int nfields_res, int nfields, db_map *m,
		    db_map *(*do_collect) (gc_t *gc_ctx, int nfields_res, int nfields,
					   db_map *singleton, void *data),
		    void *data);
db_map *db_collect_lazy (gc_t *gc_ctx, int nfields_res, db_map *m,
			 db_map *(*do_collect) (gc_t *gc_ctx,
						int nfields_res,
						int nfields,
						db_map *singleton,
						void *data),
			 void *data);
void *db_transitive_closure (gc_t *gc_ctx, int nfields, int nio,
			     unsigned long *inp_fields, unsigned long *out_fields,
			     db_map *m);

/* In mod_utils.c */
char *orchids_atoui_hex (char *s, char *end, unsigned long *ip);
void remove_thread_local_entries (state_instance_t *state);

/* In mod_mgr.c */
int begin_save_module (save_ctx_t *sctx, char *modname, off_t *startpos);
int end_save_module (save_ctx_t *sctx, off_t startpos);

/* In orchids_api.c */
/* return error codes when restore()ing. */
int restore_end_of_file (void);
int restore_badly_formatted_data (void);
int restore_bad_size (void);
int restore_bad_number_of_columns (void);
int restore_bad_magic (void);
int restore_bad_version (void);
int restore_unknown_record_field_name (void);
int restore_unknown_primitive (void);
int restore_bad_integer_size (void);

#endif /* ORCHID_H */



/*
** Copyright (c) 2002-2005 by Julien OLIVAIN, Laboratoire Spécification
** et Vérification (LSV), CNRS UMR 8643 & ENS Cachan.
** Copyright (c) 2013-2015 by Jean GOUBAULT-LARRECQ, Laboratoire Spécification
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
