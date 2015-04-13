/**
 ** @file mod_prelude.c
 ** Prelude module (See http://www.prelude-technologies.com/)
 **
 ** @author Baptiste GOURDIN <gourdin@lsv.ens-cachan.fr>
 ** @version 0.1
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

#include "mod_prelude.h"

modprelude_t	*prelude_data;
input_module_t mod_prelude;

#define RETURN_OVM_INT(x) return ovm_int_new(gc_ctx, x)
#define RETURN_OVM_UINT(x) return ovm_uint_new(gc_ctx, x)

static type_t t_prelude = { "prelude", T_EXTERNAL }; /* convertible with xmldoc type */

field_t prelude_fields[MAX_PRELUDE_FIELDS] = {
{ "prelude.ptr",		   &t_prelude, "prelude alert pointer"           }
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
				       int		field_id,
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

static prelude_description = "prelude";

void prelude_extern_free (void *p)
{
  idmef_message_t *msg = (idmef_message_t *)p;

  if (msg!=NULL)
    idmef_message_destroy(msg);
}

static void process_idmef_alert(orchids_t *ctx,
				mod_entry_t	*mod,
				idmef_message_t	*message)
{
  event_t *event;
  int c;
  ovm_var_t *val;
  gc_t *gc_ctx = ctx->gc_ctx;
  GC_START(gc_ctx, MAX_PRELUDE_FIELDS+1);

  val = ovm_extern_new (gc_ctx, (void *)message,
			prelude_description,
			prelude_extern_free);
  GC_UPDATE(gc_ctx, F_PTR, val);

  for (c = 1; c < prelude_data->nb_fields; c++)
  {
    get_field_from_idmef_value(ctx, attr, message,
			       prelude_data->field_xpath[c],
			       c, T_STR);
  }
  add_fields_to_event(ctx, mod, (event_t **)&GC_LOOKUP(MAX_PRELUDE_FIELDS),
		      (ovm_var_t **)GC_DATA(), prelude_data->nb_fields);
  post_event(ctx, mod, (event_t *)GC_LOOKUP(MAX_PRELUDE_FIELDS));
  GC_END(gc_ctx);
}

static int rtaction_recv_idmef(orchids_t *ctx, heap_entry_t *he)
{
  int ret;
  idmef_message_t *idmef_message = NULL;

  DebugLog(DF_MOD, DS_INFO,
           "Real-time action: Checking prelude alert...\n");

  ret = prelude_client_recv_idmef(prelude_data->client, 0, &idmef_message);
  if ( ret < 0 )
    {
      DebugLog (DF_MOD, DS_ERROR, prelude_strerror(ret, "no recv_idmef"));
      gc_base_free(he);
      return ret;
    }

  if (idmef_message!=NULL)
    {
      switch (idmef_message_get_type(idmef_message))
	{
	case IDMEF_MESSAGE_TYPE_ALERT:
	  //idmef_message_print(idmef_message, prelude_data->prelude_io);
	  process_idmef_alert(ctx, he->data, idmef_message);
	  break;
	case IDMEF_MESSAGE_TYPE_ERROR :
	case IDMEF_MESSAGE_TYPE_HEARTBEAT :
	default:
	  // Nothing to do
	  break;
	}
    }
  he->date.tv_sec += prelude_data->poll_period;
  register_rtaction(ctx, he);
  return 0;
}

static void issdl_idmef_message_new (orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *message;
  idmef_message_t	*idmef;
  int		ret;

  ret = idmef_message_new(&idmef);
  if ( ret < 0 )
    {
      DebugLog (DF_MOD, DS_ERROR,
		prelude_strerror(ret, "Unable to create the IDMEF message"));
      PUSH_VALUE(ctx, NULL);
      return;
    }

  message = ovm_extern_new(ctx->gc_ctx, (void *)idmef,
			   prelude_description,
			   prelude_extern_free);
  PUSH_VALUE(ctx, message);
}

static void issdl_idmef_message_set(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *idmef;
  ovm_var_t *path;
  ovm_var_t *value;
  char *str;
  int ret = 0;

  idmef = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 3);
  path = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  value = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);

  if (idmef==NULL || TYPE(idmef)!=T_EXTERNAL ||
      EXTDESC(idmef)!=prelude_description)
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

  str = ovm_strdup (ctx->gc_ctx, path);
  switch (TYPE(value))
    {
    case T_INT:
      ret = idmef_message_set_number(EXTPTR(idmef), str, INT(value));
      break;
    case T_UINT:
      ret = idmef_message_set_number(EXTPTR(idmef), str, UINT(value));
      break;
    case T_DOUBLE:
      ret = idmef_message_set_number(EXTPTR(idmef), str, DOUBLE(value));
      break;
    case T_STR: case T_VSTR:
      {
	char *val_str = ovm_strdup (ctx->gc_ctx, value);

	ret = idmef_message_set_string(EXTPTR(idmef), str, val_str);
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
	ret = idmef_message_set_string(EXTPTR(idmef), str, buff);
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
      DebugLog(DF_MOD, DS_ERROR,
	       prelude_strerror(ret, "idmef_message_set_string error\n"));
    }
  STACK_DROP(ctx->ovm_stack, 3);
  if (res < 0)
    PUSH_RETURN_FALSE(ctx);
  else
    PUSH_RETURN_TRUE(ctx);
}

static void issdl_idmef_message_send(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *idmef;
  int ret = 0;

  idmef = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  if (idmef==NULL || TYPE(idmef)!=T_EXTERNAL ||
      EXTDESC(idmef)!=prelude_description)
    {
      DebugLog(DF_MOD, DS_ERROR, "parameter not a prelude idmef document");
      ret = -1;
    }
  else if (prelude_data->mode == PRELUDE_MODE_PREWIKKA)
    {
      ret = preludedb_insert_message(prelude_data->db, EXTPTR(idmef));
      if ( ret < 0 )
	{
	  DebugLog(DF_MOD, DS_ERROR,
		   "could not insert message into database %s\n",
		   preludedb_strerror(ret));
	}
    }
  else
    {
      prelude_client_send_idmef(prelude_data->client, EXTPTR(idmef));
    }
  STACK_DROP(ctx->ovm_stack, 1);
  if (res < 0)
    PUSH_RETURN_FALSE(ctx);
  else
    PUSH_RETURN_TRUE(ctx);
}

static void issdl_idmef_message_print(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *idmef;

  idmef = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  if (idmef==NULL || TYPE(idmef)!=T_EXTERNAL ||
      EXTDESC(idmef)!=prelude_description)
    {
      DebugLog(DF_MOD, DS_ERROR, "parameter not a prelude idmef document");
      STACK_DROP(ctx->ovm_stack, 1);
      PUSH_RETURN_FALSE(ctx);
      return;
    }
  idmef_message_print(EXTPTR(idmef), prelude_data->prelude_io);
  PUSH_RETURN_TRUE(ctx);
}

static void issdl_idmef_message_get_string(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t	*idmef;
  ovm_var_t	*path;
  char *path_str;
  char		*str;
  size_t	str_len;
  ovm_var_t	*res;

  idmef = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  path = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);

  if (idmef==NULL || TYPE(idmef)!=T_EXTERNAL ||
      EXTDESC(idmef)!=prelude_description)
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

  if (idmef_message_get_string(EXTPTR(idmef), path_str, &str) < 0)
    {
      STACK_DROP(ctx->ovm_stack, 2);
      PUSH_VALUE(ctx, NULL);
    }
  else
    {
      str_len = strlen(str);
      res = ovm_str_new(str_len);
      memcpy (STR(res), str, str_len);
      STACK_DROP(ctx->ovm_stack, 2);
      PUSH_VALUE(ctx, res);
    }
  gc_base_free(path_str);
}

static void *mod_prelude_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  int i;

  DebugLog(DF_MOD, DS_INFO, "load() mod_prelude@%p\n", (void *) &mod_prelude);

  prelude_data = gc_base_malloc (ctx->gc_ctx, sizeof (modprelude_t));
  prelude_data->client = NULL;
  prelude_data->prelude_io = NULL;
  prelude_data->db = NULL;
  prelude_data->mode = PRELUDE_MODE_SENSOR;
  prelude_data->profile = "orchids";
  prelude_data->prelude_db_settings = NULL;
  prelude_data->poll_period = 0;
  prelude_data->nb_fields = 1;
  for (i=0; i<MAX_PRELUDE_FIELDS; i++)
    prelude_data->field_xpath[i] = NULL;

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

static void mod_prelude_postconfig(orchids_t *ctx, mod_entry_t *mod)
{
  int			argc = 0;
  char*			argv[2] = {"orchids", NULL};
  int			ret;
  prelude_client_t	*client;

  register_fields(ctx, mod, prelude_fields, prelude_data->nb_fields);

  ret = prelude_init(&argc, argv);
  if ( ret < 0 )
    {
      DebugLog(DF_MOD, DS_FATAL,
	       "unable to initialize the prelude library \n");
      exit(EXIT_FAILURE);;
    }

  if (prelude_data->mode == PRELUDE_MODE_PREWIKKA)
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
						   prelude_data->prelude_db_settings);
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

      ret = preludedb_new(&prelude_data->db, sql, NULL, NULL, 0);
      if ( ret < 0 )
	{
	  DebugLog(DF_MOD, DS_FATAL,
		   "could not initialize database '%s': %s\n",
		   prelude_data->prelude_db_settings,
		   preludedb_strerror(ret));
	  preludedb_sql_destroy(sql);
	  exit(EXIT_FAILURE);
	}
    }
  else
    {
      // configure prelude client

      ret = prelude_client_new(&client, prelude_data->profile);
      if (client==NULL)
	{
	  DebugLog(DF_MOD, DS_FATAL,
		   "Unable to create a prelude client object");
	  exit(EXIT_FAILURE);
	}

      // May need to call prelude_client_set_config_filename()

      if (prelude_data->mode == PRELUDE_MODE_SENSOR)
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
    prelude_data->client = client;
    prelude_data->poll_period = DEFAULT_MODPRELUDE_POLL_PERIOD;
  }

  ret = prelude_io_new(&(prelude_data->prelude_io));
  if ( ret < 0 )
    {
      prelude_perror(ret, "no prelude_io");
      DebugLog(DF_MOD, DS_FATAL,  "prelude Unable to prelude_io");
      exit(EXIT_FAILURE);
    }
  prelude_io_set_file_io(prelude_data->prelude_io, stdout);

  if (prelude_data->mode == PRELUDE_MODE_ANALYZER)
    {
      register_rtcallback(ctx,
			  rtaction_recv_idmef,
			  NULL,
			  mod,
			  INITIAL_MODPRELUDE_POLL_DELAY);
    }

  register_lang_function(ctx,
                         issdl_idmef_message_new,
                         "prelude_message_new",
			 0, prelude_new_sigs,
                         "Create a new prelude idmef report");

  register_lang_function(ctx,
                         issdl_idmef_message_set,
                         "prelude_message_set",
			 3, prelude_set_sigs,
                         "set a value in the idmef report");

  register_lang_function(ctx,
                         issdl_idmef_message_send,
                         "prelude_message_send",
			 1, prelude_send_sigs,
                         "send message to the prelude manager");

  register_lang_function(ctx,
                         issdl_idmef_message_get_string,
                         "prelude_message_get_string",
			 2, prelude_get_sigs,
                         "get a string from an idmef message using an xpath request");

  register_lang_function(ctx,
                         issdl_idmef_message_print,
                         "prelude_message_print",
			 1, prelude_print_sigs,
                         "Debug function : print the idmef alert on stderr");

}


static void dir_set_prelude_mode(orchids_t *ctx, mod_entry_t *mod,
				 config_directive_t *dir)
{
  if (!strcmp(dir->args, "sensor"))
    {
      DebugLog(DF_MOD, DS_INFO, "setting prelude mode to sensor\n");
      prelude_data->mode = PRELUDE_MODE_SENSOR;
    }
  else if (!strcmp(dir->args, "analyzer"))
    {
      DebugLog(DF_MOD, DS_INFO, "setting prelude mode to analyzer\n");
      prelude_data->mode = PRELUDE_MODE_ANALYZER;
    }
  else if (!strcmp(dir->args, "prewikka"))
    {
      DebugLog(DF_MOD, DS_INFO, "setting prelude mode to prewikka\n");
      prelude_data->mode = PRELUDE_MODE_PREWIKKA;
    }
  else
    DebugLog(DF_MOD, DS_ERROR, "unknown prelude mode, setting sensor \n");
}


static void dir_set_prelude_profile(orchids_t *ctx, mod_entry_t *mod,
				    config_directive_t *dir)
{
  DebugLog(DF_MOD, DS_INFO, "setting prelude profile to %s\n", dir->args);
  prelude_data->profile = dir->args;
}

static void dir_set_prelude_poll_period(orchids_t *ctx, mod_entry_t *mod,
					config_directive_t *dir)
{
  int value;

  DebugLog(DF_MOD, DS_INFO, "setting PollPeriod to %s\n", dir->args);
  value = strtol(dir->args, (char **)NULL, 10);
  if (value < 0)
    value = 0;
  prelude_data->poll_period = value;
}


static void add_field(orchids_t *ctx, char* field_name, char* xpath)
{
  if (prelude_data->nb_fields == MAX_PRELUDE_FIELDS)
    {
      DebugLog(DF_MOD, DS_WARN,
	       "Max number of prelude fields reached, cannot add %s %s\n",
	       field_name , xpath);
      return;
    }

  {
    char *s;

    s = gc_base_malloc (ctx->gc_ctx, 9 + strlen(field_name));
    prelude_fields[prelude_data->nb_fields].name = s;
    memcpy (s, "prelude.", 8);
    strcpy (s+8, field_name);
  }
  prelude_fields[prelude_data->nb_fields].type = T_STR;
  prelude_fields[prelude_data->nb_fields].desc = xpath;

  DebugLog(DF_MOD, DS_INFO, "add new field %s : %s\n",
	   prelude_fields[prelude_data->nb_fields].name , xpath);
  prelude_data->field_xpath[prelude_data->nb_fields++] = xpath;
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
  add_field(ctx, dir->args, pos + 1);
}


static void dir_set_prelude_db_settings(orchids_t *ctx, mod_entry_t *mod,
					config_directive_t *dir)
{
  DebugLog(DF_MOD, DS_INFO, "setting PreludeDBSettings to %s\n", dir->args);
  prelude_data->prelude_db_settings = dir->args;
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
  NULL,
  NULL,
  NULL
};

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
