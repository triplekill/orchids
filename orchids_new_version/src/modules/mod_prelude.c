/**
 ** @file mod_prelude.c
 ** Prelude module (See http://www.prelude-technologies.com/)
 **
 ** @author Baptiste GOURDIN <gourdin@lsv.ens-cachan.fr>
 ** @author Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
 **
 ** @version 0.2
 ** @ingroup modules
 **
 **
 ** @date  Started on:  Wed Jan  26 10:32:11 CET 2011
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */


#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <string.h>
#include "ovm.h"
#include "mod_prelude.h"

input_module_t mod_prelude;

#define RETURN_OVM_INT(x) return ovm_int_new(gc_ctx, x)
#define RETURN_OVM_UINT(x) return ovm_uint_new(gc_ctx, x)

static type_t t_prelude = { "prelude", T_UINT };

field_t prelude_fields[MAX_PRELUDE_FIELDS] = {
#ifdef OBSOLETE
{ "prelude.ptr",		   &t_prelude, "prelude alert pointer"           }
#endif
};

static ovm_var_t *ovm_var_from_prelude_value(gc_t *gc_ctx,
					     idmef_value_t *value,
					     int ovm_var_type)
{
  idmef_value_type_id_t	value_type;
  ovm_var_t *res;

  value_type = idmef_value_get_type(value);

  switch (ovm_var_type)
    {
    case T_STR:
      if (value_type != IDMEF_VALUE_TYPE_STRING)
	{
	  prelude_string_t *prelude_str = NULL;
	  size_t prelude_str_len = 0;
	  const char* str = NULL;

	  prelude_string_new(&prelude_str);
	  idmef_value_to_string(value, prelude_str);
	  prelude_str_len = prelude_string_get_len(prelude_str);
	  str = prelude_string_get_string(prelude_str);
	  res = ovm_str_new (gc_ctx, prelude_str_len);
	  memcpy (STR(res), str, prelude_str_len);
	  prelude_string_destroy(prelude_str);
	  return res;
	}
      else
	{
	  prelude_string_t *prelude_str = idmef_value_get_string(value);
	  size_t prelude_str_len =  prelude_string_get_len(prelude_str);
	  res = ovm_str_new (gc_ctx, prelude_str_len);
	  memcpy (STR(res), prelude_string_get_string(prelude_str),
		  prelude_str_len);
	  /* do we need to prelude_string_destroy(prelude_str)? */
	  return res;
	}
      return NULL;
    case T_INT:
      if (value_type == IDMEF_VALUE_TYPE_INT8)
	RETURN_OVM_INT (idmef_value_get_int8(value));
      else if (value_type == IDMEF_VALUE_TYPE_INT16)
	RETURN_OVM_INT (idmef_value_get_int16(value));
      else if (value_type != IDMEF_VALUE_TYPE_INT32)
	RETURN_OVM_INT (idmef_value_get_int32(value));
      DebugLog(DF_MOD, DS_ERROR,
	       "Bad conversion from prelude type (%i) to orchids type (%i)\n",
	       value_type,
	       ovm_var_type);
      return NULL;
    case T_UINT:
      if (value_type == IDMEF_VALUE_TYPE_UINT8)
	RETURN_OVM_UINT (idmef_value_get_uint8(value));
      else if (value_type == IDMEF_VALUE_TYPE_UINT16)
	RETURN_OVM_UINT (idmef_value_get_uint16(value));
      else if (value_type != IDMEF_VALUE_TYPE_UINT32)
	RETURN_OVM_UINT (idmef_value_get_uint32(value));
      DebugLog(DF_MOD, DS_ERROR,
	       "Bad conversion from prelude type (%i) to orchids type (%i)\n",
	       value_type,
	       ovm_var_type);
      return NULL;
    default:
      DebugLog(DF_MOD, DS_ERROR,
	       "Cannot convert prelude type (%i) to orchids type (%i)\n",
	       value_type,
	       ovm_var_type);
      return NULL;
    }
}

static void get_field_from_idmef_value(orchids_t	*ctx,
				       ovm_var_t	 **attr,
				       idmef_message_t	*message,
				       const char	*path,
				       size_t		field_id,
				       int		field_type)
{
  idmef_value_t	*value = NULL;
  ovm_var_t	*var;
  idmef_value_type_id_t	value_type;

  if (idmef_message_get_value(message, path, &value) < 0)
    {
      DebugLog(DF_MOD, DS_ERROR,
	       "Error getting value from path (%s)\n", path);
      return;
    };

  attr[field_id] = NULL;

  if (value != NULL)
    {
      value_type = idmef_value_get_type(value);
      if (value_type == IDMEF_VALUE_TYPE_LIST)
	{
	  DebugLog(DF_MOD, DS_ERROR,
		   "Cannot convert prelude list to orchids single elt\n");
	}
      else // value is not an array
	{
	  var = ovm_var_from_prelude_value(ctx->gc_ctx, value, field_type);
	  if (var!=NULL)
	    GC_TOUCH (ctx->gc_ctx, attr[field_id] = var);
	}
      /* destroy value? */
    }
}

static void *prelude_copy (gc_t *gc_ctx, void *obj)
{
  idmef_message_t *msg = (idmef_message_t *)obj;
  idmef_message_t *newmsg = NULL;

  if (msg==NULL)
    return NULL;
  if (idmef_message_clone (msg, &newmsg) < 0)
    return NULL;
  return newmsg;
}

static void prelude_free (void *obj)
{
  idmef_message_t *msg = (idmef_message_t *)obj;

  if (msg!=NULL)
    idmef_message_destroy(msg);
}

static int prelude_save (save_ctx_t *sctx, void *ptr)
{
  prelude_io_t *pio;
  int err;
  
  err = prelude_io_new (&pio);
  if (err) return err;
  funlockfile (sctx->f); /* To avoid any problems, we first unlock
			    the file, so that idmef_message_print()
			    has access to it */
  prelude_io_set_file_io (pio, sctx->f);
  errno = 0;
  idmef_message_print (ptr, pio);
  flockfile (sctx->f); /* and we re-lock it afterwards */
  if (errno) { err = errno; goto end; }
 end:
  prelude_io_destroy (pio);
  return err;
}

static void *prelude_restore (restore_ctx_t *rctx)
{
  prelude_msg_t *msg;
  prelude_io_t *pio;
  idmef_message_t *idmef = NULL;
  int err;

  err = prelude_io_new (&pio);
  if (err) { errno=err; return NULL; }
  funlockfile (rctx->f); /* To avoid any problems, we first unlock
			    the file, so that idmef_message_print()
			    has access to it */
  prelude_io_set_file_io (pio, rctx->f);
  msg = NULL;
  err = prelude_msg_read (&msg, pio);
  flockfile (rctx->f); /* and we re-lock it afterwards */
  if (err) goto end;
  err = idmef_message_new (&idmef);
  if (err) goto end_msg;
  err = idmef_message_read (idmef, msg);
  if (err) { idmef_message_destroy (idmef); idmef = NULL; }
 end_msg:
  prelude_msg_destroy (msg);
 end:
  prelude_io_destroy (pio);
  errno = err;
  return idmef;
}

static ovm_extern_class_t prelude_xclass = {
  "prelude",
  prelude_copy,
  prelude_free,
  prelude_save,
  prelude_restore
};

static void process_idmef_alert(orchids_t *ctx,
				mod_entry_t	*mod,
				idmef_message_t	*message,
				int dissection_level,
				modprelude_t *config)
{
  size_t c;
#ifdef OBSOLETE
  ovm_var_t *val;
#endif
  gc_t *gc_ctx = ctx->gc_ctx;
  GC_START(gc_ctx, MAX_PRELUDE_FIELDS+1);

#ifdef OBSOLETE
  val = ovm_extern_new (gc_ctx, (void *)message, &prelude_xclass);
  GC_UPDATE(gc_ctx, F_PTR, val);
#endif

  for (c = 0 /* OBSOLETE: 1*/; c < config->nb_fields; c++)
  {
    get_field_from_idmef_value(ctx, (ovm_var_t **)GC_DATA(), message,
			       config->field_xpath[c],
			       c, T_STR);
  }
  add_fields_to_event(ctx, mod, (event_t **)&GC_LOOKUP(MAX_PRELUDE_FIELDS),
		      (ovm_var_t **)GC_DATA(), config->nb_fields);
  post_event(ctx, mod, (event_t *)GC_LOOKUP(MAX_PRELUDE_FIELDS), dissection_level);
  GC_END(gc_ctx);
}

static int rtaction_recv_idmef(orchids_t *ctx, heap_entry_t *he)
{
  int ret;
  idmef_message_t *idmef_message = NULL;
  mod_entry_t *mod = (mod_entry_t *)he->data;
  modprelude_t *config = (modprelude_t *)mod->config;

  DebugLog(DF_MOD, DS_INFO,
           "Real-time action: Checking prelude alert...\n");

  ret = prelude_client_recv_idmef(config->client, 0, &idmef_message);
  if ( ret < 0 )
    {
      DebugLog (DF_MOD, DS_ERROR, prelude_strerror(ret));
      gc_base_free(he);
      return ret;
    }

  if (idmef_message!=NULL)
    {
      switch (idmef_message_get_type(idmef_message))
	{
	case IDMEF_MESSAGE_TYPE_ALERT:
	  //idmef_message_print(idmef_message, config->prelude_io);
	  process_idmef_alert(ctx, he->data, idmef_message, he->pri, config);
	  break;
	case IDMEF_MESSAGE_TYPE_ERROR :
	case IDMEF_MESSAGE_TYPE_HEARTBEAT :
	default:
	  // Nothing to do
	  break;
	}
    }
  gettimeofday(&he->date, NULL);
  he->date.tv_sec += config->poll_period;
  register_rtaction(ctx, he);
  return 0;
}

static void issdl_idmef_message_new (orchids_t *ctx, state_instance_t *state, void *data)
{
  ovm_var_t *message, *val;
  idmef_message_t	*idmef;
  int		ret;
  gc_t *gc_ctx = ctx->gc_ctx;
  uint16_t handle;

  ret = idmef_message_new(&idmef);
  if ( ret < 0 )
    {
      DebugLog (DF_MOD, DS_ERROR, prelude_strerror(ret));
      PUSH_VALUE(ctx, NULL);
      return;
    }

  GC_START (gc_ctx, 1);
  message = ovm_extern_new(gc_ctx, idmef, &prelude_xclass);
  GC_UPDATE(gc_ctx, 0, message);
  handle = create_fresh_handle (gc_ctx, state, message);
  val = ovm_uint_new (gc_ctx, handle);
  PUSH_VALUE(ctx, val);
  GC_END (gc_ctx);
}

static void issdl_idmef_message_set(orchids_t *ctx, state_instance_t *state, void *data)
{
  ovm_var_t *idmef;
  ovm_var_t *path;
  ovm_var_t *value;
  char *str;
  ovm_var_t *doc1;
  idmef_message_t *message;
  int ret = 0;

  idmef = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 3);
  path = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  value = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);

  if (idmef==NULL || TYPE(idmef)!=T_UINT) /* an uint handle to an idmef_message_t * */
    {
      DebugLog(DF_MOD, DS_ERROR, "first parameter not a prelude idmef document");
      STACK_DROP(ctx->ovm_stack, 3);
      PUSH_RETURN_FALSE(ctx);
      return;
    }
  if (path==NULL || (TYPE(path)!=T_STR && TYPE(path)!=T_VSTR))
    {
      DebugLog(DF_MOD, DS_ERROR, "second parameter not a string");
      STACK_DROP(ctx->ovm_stack, 3);
      PUSH_RETURN_FALSE(ctx);
      return;
    }
  if (value==NULL)
    {
      DebugLog(DF_MOD, DS_ERROR, "NULL third parameter");
      STACK_DROP(ctx->ovm_stack, 3);
      PUSH_RETURN_FALSE(ctx);
      return;
    }

  doc1 = handle_get_wr (ctx->gc_ctx, state, UINT(idmef));
  message = EXTPTR(doc1);

  str = ovm_strdup (ctx->gc_ctx, path);
  switch (TYPE(value))
    {
    case T_INT:
      ret = idmef_message_set_number(message, str, INT(value));
      break;
    case T_UINT:
      ret = idmef_message_set_number(message, str, UINT(value));
      break;
    case T_FLOAT:
      ret = idmef_message_set_number(message, str, FLOAT(value));
      break;
    case T_STR: case T_VSTR:
      {
	char *val_str = ovm_strdup (ctx->gc_ctx, value);

	ret = idmef_message_set_string(message, str, val_str);
	gc_base_free (val_str);
      }
      break;
    case T_CTIME:
      {
	char buff[128];
	struct tm time = *localtime (&(CTIME(value)));
	int len;

	len = strftime(buff, 128 * sizeof (char),
		       "%Y-%m-%dT%H:%M:%S%z", &time);
	if ((buff[len - 5] == '+') ||
	    (buff[len - 5] == '-'))
	  {
	    buff[len + 1] = 0;
	    buff[len] = buff[len - 1];
	    buff[len - 1] = buff[len - 2];
	    buff[len - 2] = ':';
	  }
	ret = idmef_message_set_string(message, str, buff);
	break;
      }
    default:
      DebugLog(DF_ENG, DS_ERROR,
	       "issdl_idmef_message_set not implemented for type (%i)\n",
	       TYPE(value));
      ret = -1;
      break;
    }
  gc_base_free(str);
  if (ret < 0)
    {
      DebugLog(DF_MOD, DS_ERROR, prelude_strerror(ret));
    }
  STACK_DROP(ctx->ovm_stack, 3);
  if (ret < 0)
    PUSH_RETURN_FALSE(ctx);
  else
    PUSH_RETURN_TRUE(ctx);
}

static void issdl_idmef_message_send(orchids_t *ctx, state_instance_t *state, void *data)
{
  ovm_var_t *idmef;
  ovm_var_t *doc1;
  idmef_message_t *message;
  int ret = 0;
  modprelude_t *config = (modprelude_t *)data;

  idmef = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  if (idmef==NULL || TYPE(idmef)!=T_UINT) /* an uint handle to an idmef_message_t * */
    {
      DebugLog(DF_MOD, DS_ERROR, "parameter not a prelude idmef document");
      ret = -1;
    }
  else
    {
      doc1 = handle_get_rd (ctx->gc_ctx, state, UINT(idmef));
      message = EXTPTR(doc1);
      if (config->mode == PRELUDE_MODE_PREWIKKA)
	{
	  ret = preludedb_insert_message(config->db, message);
	  if ( ret < 0 )
	    {
	      DebugLog(DF_MOD, DS_ERROR,
		       "could not insert message into database %s\n",
		       preludedb_strerror(ret));
	    }
	}
      else
	{
	  prelude_client_send_idmef(config->client, message);
	}
    }
  STACK_DROP(ctx->ovm_stack, 1);
  if (ret < 0)
    PUSH_RETURN_FALSE(ctx);
  else
    PUSH_RETURN_TRUE(ctx);
}

static void issdl_idmef_message_print(orchids_t *ctx, state_instance_t *state, void *data)
{
  ovm_var_t *idmef;
  ovm_var_t *doc1;
  idmef_message_t *message;
  modprelude_t *config = (modprelude_t *)data;

  idmef = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  if (idmef==NULL || TYPE(idmef)!=T_UINT) /* an uint handle to an idmef_message_t * */
    {
      DebugLog(DF_MOD, DS_ERROR, "parameter not a prelude idmef document");
      STACK_DROP(ctx->ovm_stack, 1);
      PUSH_RETURN_FALSE(ctx);
      return;
    }
  doc1 = handle_get_rd (ctx->gc_ctx, state, UINT(idmef));
  message = EXTPTR(doc1);
  idmef_message_print(message, config->prelude_io);
  PUSH_RETURN_TRUE(ctx);
}

static void issdl_idmef_message_get_string(orchids_t *ctx, state_instance_t *state, void *data)
{
  ovm_var_t	*idmef;
  ovm_var_t *doc1;
  idmef_message_t *message;
  ovm_var_t	*path;
  char *path_str;
  char		*str;
  size_t	str_len;
  ovm_var_t	*res;

  idmef = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  path = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);

  if (idmef==NULL || TYPE(idmef)!=T_UINT) /* an uint handle to an idmef_message_t * */
    {
      DebugLog(DF_MOD, DS_ERROR, "first parameter not a prelude idmef document");
      STACK_DROP(ctx->ovm_stack, 2);
      PUSH_VALUE(ctx, NULL);
      return;
    }
  if (path==NULL || (TYPE(path)!=T_STR && TYPE(path)!=T_VSTR))
    {
      DebugLog(DF_MOD, DS_ERROR, "second parameter not a string");
      STACK_DROP(ctx->ovm_stack, 2);
      PUSH_VALUE(ctx, NULL);
      return;
    }
  path_str = ovm_strdup (ctx->gc_ctx, path);

  doc1 = handle_get_rd (ctx->gc_ctx, state, UINT(idmef));
  message = EXTPTR(doc1);
  if (idmef_message_get_string(message, path_str, &str) < 0)
    {
      STACK_DROP(ctx->ovm_stack, 2);
      PUSH_VALUE(ctx, NULL);
    }
  else
    {
      str_len = strlen(str);
      res = ovm_str_new(ctx->gc_ctx, str_len);
      memcpy (STR(res), str, str_len);
      free (str);
      STACK_DROP(ctx->ovm_stack, 2);
      PUSH_VALUE(ctx, res);
    }
  gc_base_free(path_str);
}

static void *mod_prelude_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  int i;
  modprelude_t *prelude_data;

  DebugLog(DF_MOD, DS_INFO, "load() mod_prelude@%p\n", (void *) &mod_prelude);

  prelude_data = gc_base_malloc (ctx->gc_ctx, sizeof (modprelude_t));
  prelude_data->client = NULL;
  prelude_data->prelude_io = NULL;
  prelude_data->db = NULL;
  prelude_data->mode = PRELUDE_MODE_SENSOR;
  prelude_data->profile = "orchids";
  prelude_data->prelude_db_settings = NULL;
  prelude_data->poll_period = 0;
  prelude_data->nb_fields = 0; /* OBSOLETE: 1; */
  for (i=0; i<MAX_PRELUDE_FIELDS; i++)
    prelude_data->field_xpath[i] = NULL;
  register_extern_class (ctx, &prelude_xclass);
  return prelude_data;
}

static const type_t *prelude_new_sig[] = { &t_prelude };
static const type_t **prelude_new_sigs[] = { prelude_new_sig, NULL };

static const type_t *prelude_set_sig_int[] = { &t_int, &t_prelude, &t_str, &t_int };
static const type_t *prelude_set_sig_uint[] = { &t_int, &t_prelude, &t_str, &t_uint };
static const type_t *prelude_set_sig_float[] = { &t_int, &t_prelude, &t_str, &t_float };
static const type_t *prelude_set_sig_str[] = { &t_int, &t_prelude, &t_str, &t_str };
static const type_t *prelude_set_sig_ctime[] = { &t_int, &t_prelude, &t_str, &t_ctime };
static const type_t **prelude_set_sigs[] = { prelude_set_sig_int,
					     prelude_set_sig_uint,
					     prelude_set_sig_float,
					     prelude_set_sig_str,
					     prelude_set_sig_ctime,
					     NULL };

static const type_t *prelude_send_sig[] = { &t_int, &t_prelude };
static const type_t **prelude_send_sigs[] = { prelude_send_sig, NULL };

static const type_t *prelude_get_sig[] = { &t_str, &t_prelude, &t_str };
static const type_t **prelude_get_sigs[] = { prelude_get_sig, NULL };

static const type_t *prelude_print_sig[] = { &t_int, &t_prelude };
static const type_t **prelude_print_sigs[] = { prelude_print_sig, NULL };

struct node_expr_s;
monotony m_prelude_set (rule_compiler_t *ctx, struct node_expr_s *e, monotony m[])
{
  m[0] |= MONO_THRASH; /* thrashes the first argument (prelude document) */
  return MONO_UNKNOWN | MONO_THRASH;
}

static void mod_prelude_postconfig(orchids_t *ctx, mod_entry_t *mod)
{
  int			argc = 0;
  char*			argv[2] = {"orchids", NULL};
  int			ret;
  prelude_client_t	*client;
  modprelude_t *config = (modprelude_t *)mod->config;

  register_fields(ctx, mod, prelude_fields, config->nb_fields);

  ret = prelude_init(&argc, argv);
  if ( ret < 0 )
    {
      DebugLog(DF_MOD, DS_FATAL,
	       "unable to initialize the prelude library \n");
      exit(EXIT_FAILURE);;
    }

  if (config->mode == PRELUDE_MODE_PREWIKKA)
    {
      // configure database
      preludedb_sql_t *sql;
      preludedb_sql_settings_t *sql_settings;

      ret = preludedb_init();
      if ( ret < 0 )
	{
	  DebugLog(DF_MOD, DS_FATAL, "error initializing libpreludedb");
	  exit(EXIT_FAILURE);
	}

      ret = preludedb_sql_settings_new_from_string(&sql_settings,
						   config->prelude_db_settings);
      if ( ret < 0 )
	{
	  DebugLog(DF_MOD, DS_FATAL,
		   "Error loading database settings: %s.\n",
		   preludedb_strerror(ret));
	  exit(EXIT_FAILURE);
	}

      ret = preludedb_sql_new(&sql, NULL, sql_settings);
      if ( ret < 0 )
	{
	  DebugLog(DF_MOD, DS_FATAL,
		   "Error creating database interface: %s.\n",
		   preludedb_strerror(ret));
	  preludedb_sql_settings_destroy(sql_settings);
	  exit(EXIT_FAILURE);
	}

      ret = preludedb_new(&config->db, sql, NULL, NULL, 0);
      if ( ret < 0 )
	{
	  DebugLog(DF_MOD, DS_FATAL,
		   "could not initialize database '%s': %s\n",
		   config->prelude_db_settings,
		   preludedb_strerror(ret));
	  preludedb_sql_destroy(sql);
	  exit(EXIT_FAILURE);
	}
    }
  else
    {
      // configure prelude client

      ret = prelude_client_new(&client, config->profile);
      if (client==NULL)
	{
	  DebugLog(DF_MOD, DS_FATAL,
		   "Unable to create a prelude client object");
	  exit(EXIT_FAILURE);
	}

      // May need to call prelude_client_set_config_filename()

      if (config->mode == PRELUDE_MODE_SENSOR)
	prelude_client_set_required_permission(client,
					       PRELUDE_CONNECTION_PERMISSION_IDMEF_WRITE);
      else
	prelude_client_set_required_permission(client,
					       PRELUDE_CONNECTION_PERMISSION_IDMEF_READ|PRELUDE_CONNECTION_PERMISSION_IDMEF_WRITE);

      ret = prelude_client_start(client);
      if ( ret < 0 )
	{
	  prelude_perror(ret, "unable to initialize the prelude library");
	  DebugLog(DF_MOD, DS_FATAL,  "Unable to start prelude client");
	  exit(EXIT_FAILURE);
	}

      ret = prelude_client_set_flags(client,
				     PRELUDE_CLIENT_FLAGS_ASYNC_SEND|PRELUDE_CLIENT_FLAGS_ASYNC_TIMER);
    if ( ret < 0 )
      {
	DebugLog(DF_MOD, DS_FATAL, "Unable to set asynchronous send and timer.\n");
	exit(EXIT_FAILURE);
      }
    config->client = client;
    config->poll_period = DEFAULT_MODPRELUDE_POLL_PERIOD;
  }

  ret = prelude_io_new(&config->prelude_io);
  if ( ret < 0 )
    {
      prelude_perror(ret, "no prelude_io");
      DebugLog(DF_MOD, DS_FATAL,  "prelude Unable to prelude_io");
      exit(EXIT_FAILURE);
    }
  prelude_io_set_file_io(config->prelude_io, stdout);

  if (config->mode == PRELUDE_MODE_ANALYZER)
    {
      register_rtcallback(ctx,
			  rtaction_recv_idmef,
			  NULL,
			  mod,
			  INITIAL_MODPRELUDE_POLL_DELAY,
			  0);
    }

  register_lang_function(ctx,
                         issdl_idmef_message_new,
                         "prelude_message_new",
			 0, prelude_new_sigs,
			 m_random,
                         "Create a new prelude idmef report",
			 config);

  register_lang_function(ctx,
                         issdl_idmef_message_set,
                         "prelude_message_set",
			 3, prelude_set_sigs,
			 m_prelude_set,
                         "set a value in the idmef report",
			 config);

  register_lang_function(ctx,
                         issdl_idmef_message_send,
                         "prelude_message_send",
			 1, prelude_send_sigs,
			 m_random_thrash,
                         "send message to the prelude manager",
			 config);

  register_lang_function(ctx,
                         issdl_idmef_message_get_string,
                         "prelude_message_get_string",
			 2, prelude_get_sigs,
			 m_random,
                         "get a string from an idmef message using an xpath request",
			 config);

  register_lang_function(ctx,
                         issdl_idmef_message_print,
                         "prelude_message_print",
			 1, prelude_print_sigs,
			 m_random_thrash,
                         "Debug function : print the idmef alert on stderr",
			 config);
}


static void dir_set_prelude_mode(orchids_t *ctx, mod_entry_t *mod,
				 config_directive_t *dir)
{
  modprelude_t *config = (modprelude_t *)mod->config;
  
  if (!strcmp(dir->args, "sensor"))
    {
      DebugLog(DF_MOD, DS_INFO, "setting prelude mode to sensor\n");
      config->mode = PRELUDE_MODE_SENSOR;
    }
  else if (!strcmp(dir->args, "analyzer"))
    {
      DebugLog(DF_MOD, DS_INFO, "setting prelude mode to analyzer\n");
      config->mode = PRELUDE_MODE_ANALYZER;
    }
  else if (!strcmp(dir->args, "prewikka"))
    {
      DebugLog(DF_MOD, DS_INFO, "setting prelude mode to prewikka\n");
      config->mode = PRELUDE_MODE_PREWIKKA;
    }
  else
    DebugLog(DF_MOD, DS_ERROR, "unknown prelude mode, setting sensor \n");
}


static void dir_set_prelude_profile(orchids_t *ctx, mod_entry_t *mod,
				    config_directive_t *dir)
{
  modprelude_t *config = (modprelude_t *)mod->config;

  DebugLog(DF_MOD, DS_INFO, "setting prelude profile to %s\n", dir->args);
  config->profile = dir->args;
}

static void dir_set_prelude_poll_period(orchids_t *ctx, mod_entry_t *mod,
					config_directive_t *dir)
{
  int value;
  modprelude_t *config = (modprelude_t *)mod->config;

  DebugLog(DF_MOD, DS_INFO, "setting PollPeriod to %s\n", dir->args);
  value = strtol(dir->args, (char **)NULL, 10);
  if (value < 0)
    value = 0;
  config->poll_period = value;
}


static void add_field(orchids_t *ctx, char* field_name, char* xpath, modprelude_t *config)
{
  if (config->nb_fields == MAX_PRELUDE_FIELDS)
    {
      DebugLog(DF_MOD, DS_WARN,
	       "Max number of prelude fields reached, cannot add %s %s\n",
	       field_name , xpath);
      return;
    }

  {
    char *s;

    s = gc_base_malloc (ctx->gc_ctx, 9 + strlen(field_name));
    prelude_fields[config->nb_fields].name = s;
    memcpy (s, "prelude.", 8);
    strcpy (s+8, field_name);
  }
  prelude_fields[config->nb_fields].type = &t_str;
  prelude_fields[config->nb_fields].desc = xpath;

  DebugLog(DF_MOD, DS_INFO, "add new field %s : %s\n",
	   prelude_fields[config->nb_fields].name , xpath);
  config->field_xpath[config->nb_fields++] = xpath;
}

static void dir_add_prelude_field(orchids_t *ctx, mod_entry_t *mod,
				  config_directive_t *dir)
{
  char* pos;

  pos = strchr(dir->args, ' ');
  if (!pos)
  {
    DebugLog(DF_MOD, DS_ERROR, "Error when parsing directive str_field\n");
    return;
  }
  *pos = '\0';
  add_field(ctx, dir->args, pos + 1, (modprelude_t *)mod->config);
}


static void dir_set_prelude_db_settings(orchids_t *ctx, mod_entry_t *mod,
					config_directive_t *dir)
{
  modprelude_t *config = (modprelude_t *)mod->config;

  DebugLog(DF_MOD, DS_INFO, "setting PreludeDBSettings to %s\n", dir->args);
  config->prelude_db_settings = dir->args;
}


static mod_cfg_cmd_t prelude_dir[] =
{
  { "Mode", dir_set_prelude_mode, "Set prelude mode : sensor (default) / analyzer / display"},
  { "Profile", dir_set_prelude_profile, "Set prelude profile : orchids by default"},
  { "PollPeriod", dir_set_prelude_poll_period, "Set poll period for analyzer mode."},
  { "str_field", dir_add_prelude_field, "Add a new field corresponding to a XPath query."},
  { "PreludeDBSettings", dir_set_prelude_db_settings, "Setup configuration for prelude database connection"},
  { NULL, NULL }
};

input_module_t mod_prelude = {
  MOD_MAGIC,
  ORCHIDS_VERSION,
  0,			    /* flags */
  "prelude",
  "CeCILL2",
  NULL,
  prelude_dir,
  mod_prelude_preconfig,
  mod_prelude_postconfig,
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
** Copyright (c) 2013-2015 by Jean GOUBAULT-LARRECQ, Laboratoire Spécification
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
