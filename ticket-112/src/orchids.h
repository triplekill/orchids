/**
 ** @file orchids.h
 ** Main Orchids header.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 1.0
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

#include "orchids_types.h"
#include "orchids_defaults.h"

#include "safelib.h"
#include "hash.h"
#include "strhash.h"
#include "objhash.h"
#include "stack.h"
#include "lang.h"
#include "queue.h"
#include "debuglog.h"
#include "timer.h"
#include "slist.h"
#include "dlist.h"

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

/* this is a trick to allow optimizer enable or disable fields in modules */
#define F_NOT_NEEDED ((void *)-1)

#define MODE_ONLINE 0
#define MODE_SYSLOG 1
#define MODE_SNARE  2

#define BYTECODE_HAVE_PUSHFIELD 0x01


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
 **   (field_id MUST be sorted in decreasing order).
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
  int32_t    field_id;
  ovm_var_t *value;
  event_t   *next;
};

/**
 ** @struct active_event_s
 **   A record of a active event, referenced by a state instance.
 **   Used for the active events list and the reference count.
 **/
/**   @var active_event_s::event
 **     A pointer to the event data.
 **/
/**   @var active_event_s::next
 **     A pointer to the next active event.
 **/
/**   @var active_event_s::prev
 **     A pointer to the previous active event.
 **/
/**   @var active_event_s::refs
 **     The number of state instance that references this event
 **/
typedef struct active_event_s active_event_t;
struct active_event_s
{
  event_t        *event;
  active_event_t *next;
  active_event_t *prev;
  int32_t         refs;
};

typedef struct orchids_s orchids_t;

typedef struct rtaction_s rtaction_t;

/**
 ** @typedef rtaction_cb_t
 **   Real-time action callback function prototype.
 **/
typedef int (*rtaction_cb_t)(orchids_t *ctx, rtaction_t *e);


/**
 ** @struct rtaction_s
 **   Real time action structure, element used in the
 **   wait queue orchids_s::rtactionlist.
 **/
/**   @var rtaction_s::date
 **     Date to execute the registered action.
 **/
/**   @var rtaction_s::cb
 **     The callback to execute.
 **/
/**   @var rtaction_s::data
 **     Arbitrary data that may be used by the callback.
 **/
/**   @var rtaction_s::evtlist
 **     The list elements.
 **/
struct rtaction_s {
  timeval_t date;
  rtaction_cb_t cb;
  void *data;
  DLIST_ENTRY(rtaction_t) evtlist;
};


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
 **     ISSDL data type (@see lang.h).
 **/
/**   @var field_record_s::desc
 **     Description text field.
 **/
/**   @var field_record_s::id
 **     Field identifier. Used for reverse field name lookup from hash tables.
 **/
/**   @var field_record_s::val
 **     Current field value resolution are not thread-safe.
 **/
typedef struct field_record_s field_record_t;
struct field_record_s
{
  int32_t     active;
  char       *name;
  int32_t     type;
  char       *desc;
  int32_t     id;
  ovm_var_t  *val;
};







/*---------------------------------------------*
** data structure for automata representation **
*---------------------------------------------*/

#define SF_NOFLAGS  0x00000000
#define SF_PRUNED   0x00000002
#define SF_PATHMARK 0xF0000000

#define THREAD_BUMP     0x00000001
#define THREAD_ONLYONCE 0x00000002
#define THREAD_KILLED   0x00000004

#define RULE_INUSE     0x00000008

#define THREAD_IS_ONLYONCE(t) ((t)->flags & THREAD_ONLYONCE)
#define THREAD_IS_KILLED(t) ((t)->flags & THREAD_KILLED)

/** Mark a thread as killed.  It will be deallocated later. */
#define KILL_THREAD(ctx, t) \
      (t)->flags |= THREAD_KILLED


#define NO_MORE_THREAD(r) ((r)->threads == 0)

#define INIT_STATE_INST 0x00000001

typedef struct transition_s transition_t;
typedef struct state_s state_t;
typedef struct rule_s rule_t;
typedef struct rule_instance_s rule_instance_t;
typedef struct state_instance_s state_instance_t;
typedef unsigned long bytecode_t;

typedef struct wait_thread_s wait_thread_t;

typedef struct input_module_s input_module_t;
typedef struct polled_input_s polled_input_t;
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
  /* int state_id; */ /* XXX: NOT USED */
  int32_t  required_fields_nb;
  int32_t *required_fields;
  bytecode_t *eval_code;
  int32_t id;
  int32_t global_id;
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
  bytecode_t   *action;
  int32_t       trans_nb;
  transition_t *trans;
  rule_t       *rule;
  uint32_t      flags;
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
/**   @var rule_s::start_conds
 **     Starting conditions array.
 **/
/**   @var rule_s::start_conds_sz
 **     Starting conditions array size.
 **/
/**   @var rule_s::next
 **     Next rule in list (used for complete enumeration).
 **/
/**   @var rule_s::instances
 **     Number of active instances of this rule.
 **/
/**   @var rule_s::id
 **     Rule identifier
 **/
struct rule_s
{
  char             *filename;
  time_t	    file_mtime;
  int32_t           lineno;
  char             *name;
  int32_t           state_nb;
  state_t          *state;
  int32_t           trans_nb;
  ovm_var_t       **static_env;
  int32_t           static_env_sz;
  int32_t           dynamic_env_sz;
  char            **var_name;
  int32_t          *start_conds;
  size_t            start_conds_sz;
  rule_t           *next;
  int32_t           instances;
  int32_t           id;

  state_instance_t *init;
  wait_thread_t    *ith; /* XXX: UNUSED */
  wait_thread_t    *itt; /* XXX: UNUSED */
  int32_t           nb_init_threads; /* XXX: UNUSED */

  objhash_t        *sync_lock;
  int32_t          *sync_vars;
  int32_t           sync_vars_sz;

  /* XXX add rule stats here ??? */
};


/**
 ** @struct sync_lock_list_s
 **   Structure for the list of state instance holding a lock over
 **   synchronized variable(s).
 **/
/** @var sync_lock_list_s::next
 **   A pointer to the next element in the list.
 **/
/** @var sync_lock_list_s::state
 **   A pointer to a state instance holding a lock.
 **/
typedef struct sync_lock_list_s sync_lock_list_t;
struct sync_lock_list_s {
  sync_lock_list_t *next;
  state_instance_t *state;
};


/**
 ** @struct rule_instance_s
 **   Rule instance structure.
 **/
/**   @var rule_instance_s::rule
 **     A reference to the rule definition.
 **/
/**   @var rule_instance_s::first_state
 **     A pointer to the first state instance of this rule.
 **     This is an instance of the 'init' state of the reference rule.
 **     It is the root of the paths tree.
 **/
/**   @var rule_instance_s::next
 **     A pointer to the next rule instance in the global rule_instance list.
 **/
/**   @var rule_instance_s::state_instances
 **     Number of state instances in this rule instance.
 **/
/**   @var rule_instance_s::creation_date
 **     Rule instance creation date.
 **/
/**   @var rule_instance_s::state_list
 **     Re-trigger list for this rule instance (reverse chronological order).
 **/
/**   @var rule_instance_s::max_depth
 **     Biggest path depth. (for statistics).
 **/
/**   @var rule_instance_s::threads
 **     XXX
 **/
/**   @var rule_instance_s::flags
 **     Flags.
 **/
struct rule_instance_s
{
  rule_t *rule;
  state_instance_t *first_state;
  rule_instance_t  *next;
  int32_t           state_instances;
  time_t            creation_date;
  timeval_t         new_creation_date;
  timeval_t         new_last_act;
  wait_thread_t    *queue_head;
  wait_thread_t    *queue_tail;
  state_instance_t *state_list;
  int32_t           max_depth;
  int32_t           threads;
  uint32_t          flags;
  /* List of state instance that have synchronisation locks */
  sync_lock_list_t *sync_lock_list;
};


/**
 ** @struct state_instance_s
 ** State instance structure.
 **/
/**   @var state_instance_s::first_child
 **     First child (the next state in the exec path.
 **/
/**   @var state_instance_s::next_sibling
 **     Next sibling. This is another 'thread' fork.
 **/
/**   @var state_instance_s::parent
 **     Parent state instance (for issdl_dumpstack()).
 **/
/**   @var state_instance_s::event
 **     Event that permit to reach this state.
 **/
/**   @var state_instance_s::flags
 **     State flags. (re-trigger, ...).
 **/
/**   @var state_instance_s::state
 **     Reference to state declaration.
 **/
/**   @var state_instance_s::rule_instance
 **     Reference to the rule instance that contain this state instance.
 **/
/**   @var state_instance_s::inherit_env
 **     Environment: cumulative inherited environment for all past states.
 **/
/**   @var state_instance_s::current_env
 **     Environment: value allocated by actions in this state instance.
 **/
/**   @var state_instance_s::global_next
 **     Global state instance list by inverse creation order.
 **     (add a heap or a priority queue for future scheduling).
 **/
/**   @var state_instance_s::retrig_next
 **     Re-trigger list for THIS rule instance only.
 **/
/**   @var state_instance_s::depth
 **     Depth of this state
 **/
struct state_instance_s
{
  state_instance_t *first_child;
  state_instance_t *next_sibling;
  state_instance_t *parent;
  active_event_t   *event;
  int32_t           event_level;
  uint32_t          flags;
  state_t          *state;
  rule_instance_t  *rule_instance;
  ovm_var_t       **inherit_env;
  ovm_var_t       **current_env;
  state_instance_t *global_next; /* XXX: UNUSED */
  state_instance_t *retrig_next;
  int32_t           depth; /* XXX: UNUSED (only in create_state_instance()) */
  wait_thread_t    *thread_list;
  state_instance_t *next_report_elmt;
};

/* ------------------------------------------------------ */

/**
 ** @struct wait_thread_s
 **   This is a (virtual, not system) thread which wait for
 **   an event to pass a given transition.
 **/
/**   @var wait_thread_s::next
 **     Next thread in queue.
 **/
/**   @var wait_thread_s::prev
 **     Previous thread in queue.
 **/
/**   @var wait_thread_s::rule_instance
 **     A reference to the rule instance.
 **/
/**   @var wait_thread_s::trans
 **     A pointer to transition to pass.
 **/
/**   @var wait_thread_s::state_instance
 **     A reference to the state instance.
 **/
/**   @var wait_thread_s::flags
 **     Flags.
 **/
/**   @var wait_thread_s::pass
 **     Pass count.
 **/
struct wait_thread_s
{
  wait_thread_t    *next;
  wait_thread_t    *prev; /* XXX: UNUSED */
  rule_instance_t  *rule_instance; /* XXX: UNUSED (state_instance->rule_instance is used instead) */
  transition_t     *trans;
  state_instance_t *state_instance;
  unsigned long     flags;
  unsigned int      pass;
  wait_thread_t    *next_in_state_instance; /* XXX: UNUSED */
  time_t            timeout;
};


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


/**
 ** @struct rule_compiler_s
 ** The main rule compiler context.
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
/**   @var rule_compiler_s::static_1_res_id
 **     Ressource id for const int 1 (used for boolean)
 **/
/**   @var rule_compiler_s::static_0_res_id
 **     Ressource id for const int 0 (used for boolean)
 **/
/**   @var rule_compiler_s::static_null_res_id
 **     Ressource id for const null (used for undefined vars / fields)
 **/
/**   @var rule_compiler_s::static_param_error_res_id
 **     Ressource id for const null (with errno set to param error)
 **/
/**   @var rule_compiler_s::static_regex_error_res_id
 **     Ressource id for const null (with errno set to regex error)
 **/
struct rule_compiler_s
{
  char             *currfile;
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

  int		static_1_res_id;
  int		static_0_res_id;
  int		static_null_res_id;
  int		static_param_error_res_id;
  int		static_regex_error_res_id;
};


/**
 ** @typedef ovm_func_t
 ** ISSDL built-in function type declaration
 **/
typedef void (*ovm_func_t)(orchids_t *ctx, state_instance_t *state);


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
/**   @var issdl_function_s::desc
 **     Function description (for a little help).
 **/
typedef struct issdl_function_s issdl_function_t;
struct issdl_function_s
{
  ovm_func_t func;
  int32_t    id;
  char      *name;
  int32_t    args_nb;
  char      *desc;
};


typedef struct mod_entry_s mod_entry_t;

/**
 ** @typedef dissect_t
 **   Dissector function pointer implemented in modules and registered
 **   into the module manager.
 **/
typedef int (*dissect_t)(orchids_t *ctx, mod_entry_t *mod, struct event_s *e, void *data);

/**
 ** @struct conditional_dissector_record_s
 **   This structure is used to wrap a conditional dissector,
 **   the pointer to module responsible for the dissection and
 **   the abstract data for the dissector.  This structure was added
 **   during the modularization of Orchids, more precisely when Orchids
 **   start to build modules as shared objects.  A module configuration was
 **   initially held in the ::input_module_s module structure.  This was
 **   incompatible with security measures in newer OS.  See for example:
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
/**   @var orchids_s::poll_handler_list
 **     List of handler for polled data.
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
/**   @var orchids_s::active_events
 **     Number of active events.
 **/
/**   @var orchids_s::rule_instances
 **     Number of total rule instance.
 **/
/**   @var orchids_s::state_instances
 **     Number of total state instance.
 **/
/**   @var orchids_s::threads
 **     Total number of threads.
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
/**   @var orchids_s::last_evt_act
 **     Last event activity: the last time when an event was kept.
 **/
/**   @var orchids_s::last_rule_act
 **     Last rule activity: the last time when a rule was inserted or removed.
 **/
/**   @var orchids_s::last_ruleinst_act
 **     Last rule instance activity: the last time when a rule instance
 **     was created or removed.
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
  timeval_t    start_time;
  char        *config_file;
  mod_entry_t  mods[MAX_MODULES];
  int32_t      loaded_modules;
  polled_input_t     *poll_handler_list;
  time_t              last_poll;
  realtime_input_t   *realtime_handler_list;
  int                 maxfd;
  fd_set_t            fds;
  config_directive_t *cfg_tree;
  int32_t             num_fields;
  field_record_t     *global_fields;
  uint32_t            events;
  rule_compiler_t    *rule_compiler;
  timeval_t           poll_period;
  rulefile_t         *rulefile_list;
  rulefile_t         *last_rulefile;
  rule_instance_t    *first_rule_instance;
  rule_instance_t    *last_rule_instance;
  state_instance_t   *retrig_list;
  uint32_t            active_events;
  uint32_t            rule_instances;
  uint32_t            state_instances;
  uint32_t            threads;
  lifostack_t        *ovm_stack;
  issdl_function_t   *vm_func_tbl;
  int32_t             vm_func_tbl_sz;
  int32_t             off_line_mode;
  char               *off_line_input_file;
  bool_t              daemon;

  wait_thread_t  *current_tail;
  wait_thread_t  *cur_retrig_qh;
  wait_thread_t  *cur_retrig_qt;
  wait_thread_t  *new_qh;
  wait_thread_t  *new_qt;
  wait_thread_t  *retrig_qh;
  wait_thread_t  *retrig_qt;

  active_event_t *active_event_head;
  active_event_t *active_event_tail;
  active_event_t *active_event_cur;

  timeval_t  preconfig_time;
  timeval_t  postconfig_time;
  timeval_t  compil_time;
  timeval_t  postcompil_time;

  FILE *evt_fb_fp;

  /* global temporal information container */
  /* XXX: Move this into mod_period */
  strhash_t        *temporal;

  char *runtime_user;

  rusage_t ru;

  pid_t pid;

  uint32_t reports;

  timeval_t last_evt_act;
  timeval_t last_mod_act; /* XXX: UNUSED: last time when a mod was ins or rem */
  timeval_t last_rule_act;
  timeval_t last_ruleinst_act;

  DLIST_HEAD(evtlist, rtaction_t) rtactionlist;

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
typedef void (*dir_handler_t)(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir);


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
struct input_module_s
{
  uint32_t               magic;
  uint32_t               version;
  char                  *name;
  char                  *license;
  char                 **dependencies;
  mod_cfg_cmd_t         *cfg_cmds;
  pre_config_t           pre_config;
  post_config_t          post_config;
  post_compil_t          post_compil;
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
 ** @struct polled_input_s
 **   Contains informations on modules using polled events...
 **/
/**   @var polled_input_s::next
 **     A pointer to the next polled input.
 **/
/**   @var polled_input_s::last_call
 **     Date of last call. Used to multiplex w/ select() when events occur.
 **/
/**   @var polled_input_s::data
 **     Additional module specific data.
 **/
/**   @var polled_input_s::cb
 **     Module callback supposed to poll data. (and post events).
 **/
/**   @var polled_input_s::mod
 **     The module which registered this polled input.
 **/
struct polled_input_s
{
  polled_input_t        *next;
  time_t                 last_call;
  void                  *data;
  poll_callback_t        cb;
  mod_entry_t           *mod;
};

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
/**   @var field_s::desc
 **     Field description.
 **/
typedef struct field_s field_t;
struct field_s
{
  char    *name;
  int32_t  type;
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

/* ochids_cfg.c */

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
unsigned int
crc32(unsigned int crc, char *buf, int len);

/* string_util.c */
size_t
my_strspn(const char *pos, const char *eot, size_t n);

size_t
get_next_int(const char *istr, long *i, size_t n);

size_t
get_next_token(const char *pos, int c, size_t n);


/* issdl.l */
void
set_lexer_context(rule_compiler_t *ctx);

/**
 ** ISSDL lexer entry-point.
 **/
int
issdllex(void); /* yylex() */

void
issdlerror(char *s); /* yyerror() */

/* issdl.y */
void
set_yaccer_context(rule_compiler_t *ctx);

/**
 ** ISSDL parser (yaccer) entry-point.
 **
 ** @return The error code.
 **/
int
issdlparse(void); /* yyparse() */

/* ovm.h */

/**
 ** Orchids virtual machine entry point.
 **
 ** @param ctx       Orchids application context.
 ** @param s         State instance for the execution context.
 ** @param bytecode  Byte code to execute.
 **
 ** @return  0 for a normal exit (OP_END) or the error code.
 **/
int
ovm_exec(orchids_t *ctx, state_instance_t *s, bytecode_t *bytecode);

/**
 ** Convert an ovm opcode into the mnemonic name.
 **
 ** @param opcode  The numeric opcode to convert.
 **
 ** @return  The corresponding mnemonic name string.
 **/
const char *
get_opcode_name(bytecode_t opcode);

/**
 ** Disassemble and display an ovm byte code.
 ** @param fp        Output stream.
 ** @param bytecode  Byte code to disassemble.
 **/
void
fprintf_bytecode(FILE *fp, bytecode_t *bytecode);

void
fprintf_bytecode_short(FILE *fp, bytecode_t *bytecode);


/**
 ** Display a raw byte code dump in hexadecimal.
 **
 ** @param fp    Output stream.
 ** @param code  Byte code to display.
 **/
void
fprintf_bytecode_dump(FILE *fp, bytecode_t *code);


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
 ** Register a new function in the Orchids language.
 ** This function may be used by loadable module to register
 ** new functions.
 ** @param ctx   A pointer to the Orchids application context.
 ** @param func  A pointer to the C function which handle this
 **              Orchids function.
 ** @param name  The name of the function in the Orchids language.
 ** @param arity The number of arguments of the function.
 ** @param desc  A short description of the function.
 **/
void
register_lang_function(orchids_t *ctx,
                       ovm_func_t func,
                       const char *name,
                       int arity,
                       const char *desc);

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
** Debugging stuffs...
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

#endif /* ORCHID_H */



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
