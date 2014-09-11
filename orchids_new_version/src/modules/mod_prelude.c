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

#define RETURN_OVM_INT(x)			\
 do {						\
    res = ovm_int_new ();			\
    INT(res) = (x);				\
    return res;					\
 } while (0)


#define RETURN_OVM_UINT(x)						\
  do {									\
    res = ovm_uint_new ();						\
    UINT(res) = idmef_value_get_int8(value);				\
    return res;								\
  } while(0)

field_t prelude_fields[MAX_PRELUDE_FIELDS] = {
{ "prelude.ptr",		    T_EXTERNAL,"prelude alert ptr"           }
};

static ovm_var_t*
ovm_var_from_prelude_value(idmef_value_t	*value,
			   int			ovm_var_type)
{
  idmef_value_type_id_t	value_type;
  ovm_var_t		*res;

  value_type = idmef_value_get_type(value);

  switch (ovm_var_type)
  {
    case T_STR:
      if (value_type != IDMEF_VALUE_TYPE_STRING)
      {
	prelude_string_t *prelude_str = NULL;
	size_t		 prelude_str_len = 0;
	const char* str = NULL;
	prelude_string_new(&prelude_str);
	idmef_value_to_string(value, prelude_str);
	prelude_str_len =  prelude_string_get_len(prelude_str);
	str = prelude_string_get_string(prelude_str);
	res = ovm_str_new (prelude_str_len);
	memcpy (STR(res), str, prelude_str_len);
	return res;
      }
      else
      {
	prelude_string_t *prelude_str = idmef_value_get_string(value);
	size_t		 prelude_str_len =  prelude_string_get_len(prelude_str);
	res = ovm_str_new (prelude_str_len);
	memcpy (STR(res), prelude_string_get_string(prelude_str), prelude_str_len);
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
	       "Bad conversion from prelude type (%i) to orchids type (%i)\n", value_type,
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
	       "Bad conversion from prelude type (%i) to orchids type (%i)\n", value_type,
	       ovm_var_type);
      return NULL;
  default:
    DebugLog(DF_MOD, DS_ERROR,
	     "Cannot convert prelude type (%i) to orchids type (%i)\n", value_type,
	     ovm_var_type);
    return NULL;
  }
}

static void
get_field_from_idmef_value(orchids_t	*ctx,
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
      var = ovm_var_from_prelude_value(value, field_type);
      if (var)
	attr[field_id] = var;
    }
  }
}

static void
process_idmef_alert(orchids_t *ctx,
		    mod_entry_t	*mod,
		    idmef_message_t	*message)
{
  ovm_var_t		*attr[MAX_PRELUDE_FIELDS];
  event_t *event;
  int		c;

  memset(attr, 0, sizeof(attr));

  attr[F_PTR] = ovm_extern_new();
  EXTPTR(attr[F_PTR]) = message;

  for (c = 1; c < prelude_data->nb_fields; c++)
  {
    get_field_from_idmef_value(ctx, attr, message,
			       prelude_data->field_xpath[c],
			       c, T_STR);
  }

  event = NULL;
  add_fields_to_event(ctx, mod, &event, attr, prelude_data->nb_fields);
  post_event(ctx, mod, event);
}

static int
rtaction_recv_idmef(orchids_t *ctx, rtaction_t *e)
{
  int		ret;
  idmef_message_t   *idmef_message = NULL;

  DebugLog(DF_MOD, DS_INFO,
           "Real-time action: Checking prelude alert...\n");

  ret = prelude_client_recv_idmef(prelude_data->client, 0, &idmef_message);
  if ( ret < 0 ) {
    prelude_perror(ret, "no recv_idmef");
    DebugLog(DF_MOD, DS_FATAL, "prelude Unable to recv idmef");
    exit(EXIT_FAILURE);
  }

  if (idmef_message)
  {
    switch (idmef_message_get_type(idmef_message))
    {
      case IDMEF_MESSAGE_TYPE_ALERT:
	//idmef_message_print(idmef_message, prelude_data->prelude_io);
	process_idmef_alert(ctx, e->data, idmef_message);
	break;
      case IDMEF_MESSAGE_TYPE_ERROR :
      case IDMEF_MESSAGE_TYPE_HEARTBEAT :
      default:
	// Nothing to do
	break;
    }
  }

  e->date.tv_sec += prelude_data->poll_period;

  register_rtaction(ctx, e);

  return (0);
}

static void
issdl_idmef_message_new (orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *message;
  idmef_message_t	*idmef;
  int		ret;

  ret = idmef_message_new(&idmef);
  if ( ret < 0 ) {
    prelude_perror(ret, "Unable to create the IDMEF message");
    ISSDL_RETURN_PARAM_ERROR(ctx, state);
    return;
  }

  message = ovm_extern_new();
  EXTPTR(message) = idmef;
  stack_push(ctx->ovm_stack, message);
}

static void
issdl_idmef_message_set(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *idmef;
  ovm_var_t *path;
  ovm_var_t *value;
  int		ret = 0;

  idmef = stack_pop(ctx->ovm_stack);
  path = stack_pop(ctx->ovm_stack);
  value = stack_pop(ctx->ovm_stack);

  if ((TYPE(idmef) != T_EXTERNAL) || (TYPE(path) != T_STR) ||
      (TYPE(value) == T_NULL)) {
    DebugLog(DF_ENG, DS_ERROR, "parameter type error\n");
    ISSDL_RETURN_PARAM_ERROR(ctx, state);
    return ;
  }

  switch (TYPE(value))
  {
    case T_INT:
      ret = idmef_message_set_number(EXTPTR(idmef), STR(path), INT(value));
      break;
    case T_UINT:
      ret = idmef_message_set_number(EXTPTR(idmef), STR(path), UINT(value));
      break;
    case T_DOUBLE:
      ret = idmef_message_set_number(EXTPTR(idmef), STR(path), DOUBLE(value));
      break;
    case T_STR:
      ret = idmef_message_set_string(EXTPTR(idmef), STR(path), STR(value));
      break;
    case T_VSTR:
      ret = idmef_message_set_string(EXTPTR(idmef), STR(path), VSTR(value));
      break;
    case T_CTIME:
    {
      char		buff[128];
      struct tm time = *localtime (&(CTIME(value)));
      int len;

      len = strftime(buff, 128 * sizeof (char), "%Y-%m-%dT%H:%M:%S%z", &time);

      if ((buff[len - 5] == '+') ||
	  (buff[len - 5] == '-'))
      {
	buff[len + 1] = 0;
	buff[len] = buff[len - 1];
	buff[len - 1] = buff[len - 2];
	buff[len - 2] = ':';
      }

      idmef_message_set_string(EXTPTR(idmef), STR(path), buff);
      break;
    }

    default:
      DebugLog(DF_ENG, DS_ERROR, "issdl_idmef_message_set not implemented for type (%i)\n",
	       TYPE(value));
      ret = 0;
  }
  if (ret < 0)
    prelude_perror(ret, "idmef_message_set_string error\n");

}

static void
issdl_idmef_message_send(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *idmef;
  int	    ret;

  idmef = stack_pop(ctx->ovm_stack);
  if ((TYPE(idmef) != T_EXTERNAL) || (EXTPTR(idmef) == NULL)) {
    DebugLog(DF_ENG, DS_ERROR, "parameter type error\n");
    ISSDL_RETURN_FALSE(ctx, state);
  }

  if (prelude_data->mode == PRELUDE_MODE_PREWIKKA)
  {
    ret = preludedb_insert_message(prelude_data->db, EXTPTR(idmef));
    if ( ret < 0 ) {
      DebugLog(DF_MOD, DS_ERROR, "could not insert message into database %s\n", preludedb_strerror(ret));
      ISSDL_RETURN_FALSE(ctx, state);
    }
    ISSDL_RETURN_TRUE(ctx, state);
  }
  else
  {
    prelude_client_send_idmef(prelude_data->client, EXTPTR(idmef));
    ISSDL_RETURN_TRUE(ctx, state);
  }
  idmef_message_destroy(EXTPTR(idmef));
}

static void
issdl_idmef_message_print(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t *idmef;

  idmef = stack_pop(ctx->ovm_stack);
  if ((TYPE(idmef) != T_EXTERNAL) || (EXTPTR(idmef) == NULL)) {
    DebugLog(DF_ENG, DS_ERROR, "parameter type error\n");
    ISSDL_RETURN_FALSE(ctx, state);
  }

  idmef_message_print(EXTPTR(idmef), prelude_data->prelude_io);
  ISSDL_RETURN_TRUE(ctx, state);
}

static void
issdl_idmef_message_get_string(orchids_t *ctx, state_instance_t *state)
{
  ovm_var_t	*idmef;
  ovm_var_t	*path;
  char		*str;
  size_t	str_len;
  ovm_var_t	*res;

  idmef = stack_pop(ctx->ovm_stack);
  path = stack_pop(ctx->ovm_stack);

  if ((TYPE(idmef) != T_EXTERNAL) || (TYPE(path) != T_STR))
  {
    DebugLog(DF_ENG, DS_ERROR, "parameter type error\n");
    ISSDL_RETURN_FALSE(ctx, state);
    return ;
  }

  if (idmef_message_get_string(EXTPTR(idmef), STR(path), &str)
      < 0)
     ISSDL_RETURN_FALSE(ctx, state);
  else
  {
    str_len = strlen(str);
    res = ovm_str_new(str_len);
    memcpy (STR(res), str, str_len);
    stack_push(ctx->ovm_stack, res);
  }
}

static void *
mod_prelude_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  DebugLog(DF_MOD, DS_INFO, "load() mod_prelude@%p\n", (void *) &mod_prelude);

  prelude_data = Xzmalloc(sizeof (modprelude_t));
  prelude_data->mode = PRELUDE_MODE_SENSOR;
  prelude_data->profile = "orchids";
  prelude_data->nb_fields = 1;

  return (prelude_data);
}


static void
mod_prelude_postconfig(orchids_t *ctx, mod_entry_t *mod)
{
  int			argc = 0;
  char*			argv[2] = {"orchids", NULL};
  int			ret;
  prelude_client_t	*client;

  register_fields(ctx, mod, prelude_fields, prelude_data->nb_fields);

  ret = prelude_init(&argc, argv);
  if ( ret < 0 ) {
    DebugLog(DF_MOD, DS_FATAL, "unable to initialize the prelude library \n");
    exit(EXIT_FAILURE);;
  }

  if (prelude_data->mode == PRELUDE_MODE_PREWIKKA)
  {
    // configure database
    preludedb_sql_t *sql;
    preludedb_sql_settings_t *sql_settings;

    ret = preludedb_init();
    if ( ret < 0 ) {
      DebugLog(DF_MOD, DS_FATAL, "error initializing libpreludedb");
      exit(EXIT_FAILURE);
    }

    ret = preludedb_sql_settings_new_from_string(&sql_settings,
      prelude_data->prelude_db_settings);
    if ( ret < 0 ) {
      DebugLog(DF_MOD, DS_FATAL, "Error loading database settings: %s.\n", preludedb_strerror(ret));
      exit(EXIT_FAILURE);
    }

    ret = preludedb_sql_new(&sql, NULL, sql_settings);
    if ( ret < 0 ) {
      DebugLog(DF_MOD, DS_FATAL, "Error creating database interface: %s.\n", preludedb_strerror(ret));
      preludedb_sql_settings_destroy(sql_settings);
      exit(EXIT_FAILURE);
    }

    ret = preludedb_new(&(prelude_data->db), sql, NULL, NULL, 0);
    if ( ret < 0 ) {
      DebugLog(DF_MOD, DS_FATAL, "could not initialize database '%s': %s\n", prelude_data->prelude_db_settings, preludedb_strerror(ret));
      preludedb_sql_destroy(sql);
      exit(EXIT_FAILURE);
    }
  }
  else
  {
    // configure prelude client

    ret = prelude_client_new(&client, prelude_data->profile);
    if ( ! client ) {
      DebugLog(DF_MOD, DS_FATAL,  "Unable to create a prelude client object");
      exit(EXIT_FAILURE);
    }

    // May need to call prelude_client_set_config_filename()

    if (prelude_data->mode == PRELUDE_MODE_SENSOR)
      prelude_client_set_required_permission(client, PRELUDE_CONNECTION_PERMISSION_IDMEF_WRITE);
    else
      prelude_client_set_required_permission(client, PRELUDE_CONNECTION_PERMISSION_IDMEF_READ|PRELUDE_CONNECTION_PERMISSION_IDMEF_WRITE);

    ret = prelude_client_start(client);
    if ( ret < 0 ) {
      prelude_perror(ret, "unable to initialize the prelude library");
      DebugLog(DF_MOD, DS_FATAL,  "Unable to start prelude client");
      exit(EXIT_FAILURE);
    }

    ret = prelude_client_set_flags(client, PRELUDE_CLIENT_FLAGS_ASYNC_SEND|PRELUDE_CLIENT_FLAGS_ASYNC_TIMER);
    if ( ret < 0 ) {
      DebugLog(DF_MOD, DS_FATAL, "Unable to set asynchronous send and timer.\n");
      exit(EXIT_FAILURE);
    }

    prelude_data->client = client;
    prelude_data->poll_period = DEFAULT_MODPRELUDE_POLL_PERIOD;

  }

  ret = prelude_io_new(&(prelude_data->prelude_io));
  if ( ret < 0 ) {
    prelude_perror(ret, "no prelude_io");
    DebugLog(DF_MOD, DS_FATAL,  "prelude Unable to prelude_io");
    exit(EXIT_FAILURE);
  }
  prelude_io_set_file_io(prelude_data->prelude_io, stdout);


  if (prelude_data->mode == PRELUDE_MODE_ANALYZER)
  {
    register_rtcallback(ctx,
			rtaction_recv_idmef,
			mod,
			INITIAL_MODPRELUDE_POLL_DELAY);
  }

  register_lang_function(ctx,
                         issdl_idmef_message_new,
                         "idmef_message_new", 0,
                         "Create a new idmef report");

  register_lang_function(ctx,
                         issdl_idmef_message_set,
                         "idmef_message_set", 3,
                         "Set a value in the idmef report");

  register_lang_function(ctx,
                         issdl_idmef_message_send,
                         "idmef_message_send", 1,
                         "send message to the prelude manager");

  register_lang_function(ctx,
                         issdl_idmef_message_get_string,
                         "idmef_message_get_string", 2,
                         "get a string from an idmef message using an xpath request");

  register_lang_function(ctx,
                         issdl_idmef_message_print,
                         "idmef_message_print", 2,
                         "Debug function : print the idmef alert on stderr");

}


static void
dir_set_prelude_mode(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
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


static void
dir_set_prelude_profile(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  DebugLog(DF_MOD, DS_INFO, "setting prelude profile to %s\n", dir->args);

  prelude_data->profile = dir->args;
}

static void
dir_set_prelude_poll_period(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  int value;

  DebugLog(DF_MOD, DS_INFO, "setting PollPeriod to %s\n", dir->args);

  value = atoi(dir->args);
  if (value < 0) {
    value = 0;
  }

  prelude_data->poll_period = value;
}


static void
add_field(char* field_name, char* xpath)
{
  if (prelude_data->nb_fields == MAX_PRELUDE_FIELDS)
  {
    DebugLog(DF_MOD, DS_WARN, "Max number of prelude fields reached, cannot add %s %s\n", field_name , xpath);
    return;
  }

  prelude_fields[prelude_data->nb_fields].name = Xzmalloc((9 + strlen(field_name))
							  * sizeof(char));
  strcat(prelude_fields[prelude_data->nb_fields].name, "prelude.");
  strcat(prelude_fields[prelude_data->nb_fields].name, field_name);

  prelude_fields[prelude_data->nb_fields].type = T_STR;
  prelude_fields[prelude_data->nb_fields].desc = xpath;

  DebugLog(DF_MOD, DS_INFO, "add new field %s : %s\n", prelude_fields[prelude_data->nb_fields].name , xpath);
  prelude_data->field_xpath[prelude_data->nb_fields++] = xpath;
}

static void
dir_add_prelude_field(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  char* pos;

  pos = strchr(dir->args, ' ');
  if (!pos)
  {
    DebugLog(DF_MOD, DS_ERROR, "Error when parsing directiv str_field\n");
    return;
  }
  *pos = '\0';

  add_field(dir->args, pos + 1);
}


static void
dir_set_prelude_db_settings(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
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
  "prelude",
  "CeCILL2",
  NULL,
  prelude_dir,
  mod_prelude_preconfig,
  mod_prelude_postconfig,
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
