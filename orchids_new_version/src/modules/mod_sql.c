/**
 ** @file mod_sql.c
 ** SQL database load module.
 **
 ** @author Pierre-Arnaud SENTUCQ <pierre-arnaud.sentucq@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 ** @date  Started on: Wed Apr  1 16:59:54 CEST 2015
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "orchids.h"
#include "ovm.h"
#include "mod_sql.h"
#include "lang.h"

#include <string.h>
#include <stddef.h>
#include <arpa/inet.h>
#include <ctype.h>

/* sql librairies */
#ifdef HAVE_SQLITE
#include <sqlite3.h>
#endif
#ifdef HAVE_MYSQL
#include <mysql/mysql.h>
#endif

input_module_t mod_sql;

static sql_config_t *mod_sql_cfg;

extern gc_class_t db_small_table_class;
extern gc_class_t db_tuples_class;


static char *str_from_stack_arg(orchids_t *ctx, int arg_n, int args_total_n)
{
  ovm_var_t *var;
  char *ret = NULL;
 
  var = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, arg_n); 
  if (var == NULL)
  {
    DebugLog(DF_ENG, DS_ERROR, "issdl_load_sql(): param error\n");
    STACK_DROP(ctx->ovm_stack, args_total_n);
    PUSH_VALUE(ctx, NULL);
  }
  else
  {
    switch (TYPE(var))
    {
      case T_STR:
        ret = gc_base_malloc(ctx->gc_ctx, STRLEN(var) + 1);
        memcpy(ret, STR(var), STRLEN(var));
	STR(var)[STRLEN(var)] = '\0';
        break;
      case T_VSTR:
        ret = gc_base_malloc(ctx->gc_ctx, VSTRLEN(var) + 1);
        memcpy(ret, VSTR(var), VSTRLEN(var));
	VSTR(var)[VSTRLEN(var)] = '\0';
        break;
      default:
        DebugLog(DF_ENG, DS_ERROR, "issdl_load_sql(): param error\n");
        STACK_DROP(ctx->ovm_stack, args_total_n);
        PUSH_VALUE(ctx, NULL);
    }
  }
  return ret; 
}

/* Doubly-linked list manipulation functions */

dlist *dlist_new(gc_t *gc_ctx)
{
  dlist *dl = gc_base_malloc(gc_ctx, sizeof (*dl));
  
  dl->length = 0;
  dl->head = NULL;
  dl->tail = NULL;
  return dl;
}

size_t dlist_length(dlist *dl)
{
  size_t len = 0;
  if (dl != NULL)
    len = dl->length;
  return len;
}

node *dlist_find(dlist *dl, char *name)
{
  node *tmp = NULL;

  if (dl != NULL)
    {
      for (tmp = dl->head; tmp != NULL; tmp=tmp->next)
	{
	  if (tmp->name==name)
	    break;
	}
    }
  return tmp;
}

dlist *dlist_prepend(gc_t *gc_ctx, dlist *dl, char *name, db_map *db)
{
  if (dl != NULL)
  {
    node *new_node = gc_base_malloc (gc_ctx, sizeof (*new_node));
    
    new_node->name = name;
    new_node->db = db;
    new_node->prev = NULL;      
    if (dl->tail == NULL)
      {
        new_node->next = NULL;
        dl->head = new_node;
        dl->tail = new_node;
      }
    else
      { 
        dl->head->prev = new_node;
        new_node->next = dl->head;
        dl->head = new_node;
      }
    dl->length++;
  }
  return dl;
}

dlist *dlist_remove_last(dlist *dl)
{
  if (dl != NULL)
    {
      node *tmp = dl->tail;
      
      if (tmp != NULL)
	{
	  if ((dl->tail = tmp->prev)!=NULL)
	    dl->tail->next = NULL;
	  gc_base_free (tmp);
	  dl->length--;
	}
    }
  return dl;
}

dlist *dlist_move_into_lead(dlist *dl, node *n)
{
  if (dl != NULL && n != NULL && dl->head!=NULL && dl->head!=n)
    {
      n->prev->next = n->next;
      if (n == dl->tail)
	dl->tail = n->prev;
      else
	n->next->prev = n->prev;
      n->prev = NULL;
      n->next = dl->head;
      dl->head->prev = n;
      dl->head = n;
    }
  return dl;
}

/* MySQL database management */

#ifdef HAVE_MYSQL
db_map *db_from_mysql(orchids_t *ctx, char *domain, char *user, char *pwd,
                      char *dbname, char *sql_query)
{ 
  db_small_table *t;
  db_map *db = NULL;
  db_map *s = NULL;
  OBJTYPE obj;
  MYSQL mysql;

  GC_START(ctx->gc_ctx, 2);
  mysql_init(&mysql);
  mysql_options(&mysql, MYSQL_READ_DEFAULT_GROUP, "option");
  
  if (mysql_real_connect(&mysql, domain, user, pwd, dbname, 0, NULL, 0))
  {
    mysql_query(&mysql, sql_query);
    MYSQL_RES *res = NULL;
    MYSQL_ROW row;
    int i = 0;
    unsigned int ncols = 0;

    res = mysql_use_result(&mysql);
    ncols = mysql_num_fields(res);
    t = gc_alloc(ctx->gc_ctx, DBST_SIZE(ncols), &db_small_table_class);   
    t->gc.type = T_DB_SMALL_TABLE;
    t->nfields = ncols;
    t->next = NULL;
    GC_UPDATE(ctx->gc_ctx, 0, t);

    while((row = mysql_fetch_row(res)))
    {  
      unsigned long *len = NULL;
      len = mysql_fetch_lengths(res);
      for (i=0; i<ncols; i++)
      {
        obj = ovm_str_new(ctx->gc_ctx, len[i]);
        memcpy(STR(obj), row[i] ? row[i] : "NULL", STRLEN(obj));
	      GC_TOUCH(ctx->gc_ctx, t->tuple[i] = obj);
      }
      
      s = gc_alloc(ctx->gc_ctx, DB_TUPLE_SIZE(ncols), &db_tuples_class);
      GC_TOUCH(ctx->gc_ctx, s->what.tuples.table = t);
      GC_UPDATE(ctx->gc_ctx, 1, s); 
      for (i=0; i<ncols; i++)
      {
        if (t->tuple[i]==NULL)
    	    s->what.tuples.hash[i] = 0;
        else 
          s->what.tuples.hash[i] = issdl_hash(t->tuple[i]);
      } 
      db = db_union (ctx->gc_ctx, ncols, db, s);
    }

    mysql_free_result(res);
    mysql_close(&mysql);
  }
  else
    DebugLog(DF_ENG, DS_ERROR, "can't load MySQL database\n");

  GC_END(ctx->gc_ctx);
  return db;
}

static void issdl_load_mysql(orchids_t *ctx, state_instance_t *state)
{
  char *domain;
  char *user;
  char *pwd;
  char *dbname;
  char *tab;
  char *sql_query;
  node *n;
  size_t i, l;

  domain = str_from_stack_arg(ctx, 5, 5);
  if (domain==NULL) goto err_domain;
  user = str_from_stack_arg(ctx, 4, 5);
  if (user==NULL) goto err_user;
  pwd = str_from_stack_arg(ctx, 3, 5);
  if (pwd==NULL) goto err_pwd;
  dbname = str_from_stack_arg(ctx, 2, 5);
  if (dbname==NULL) goto err_dbname;
  tab = str_from_stack_arg(ctx, 1, 5);
  if (tab==NULL) goto err_tab;

  l = strlen(tab);
  for (i = 0; i<l; i++)
  {
    if (!isalnum(tab[i]))
      {
	DebugLog(DF_ENG, DS_ERROR,
		 "Database table name must only contain alphanumerical characters");
	STACK_DROP(ctx->ovm_stack, 5);
	PUSH_VALUE(ctx, NULL);
	goto err_format;
      }
  }

  sql_query = gc_base_malloc(ctx->gc_ctx, strlen(tab) + 15);
  strncpy(sql_query, "SELECT * FROM ", 15);
  strncat(sql_query, tab, strlen(tab));

  n = dlist_find(mod_sql_cfg->dl, dbname);
  if (n != NULL)
    {
      mod_sql_cfg->dl = dlist_move_into_lead(mod_sql_cfg->dl, n);
      STACK_DROP(ctx->ovm_stack, 5);
      PUSH_VALUE(ctx, n->db);
    }
  else
    {
      if (dlist_length(mod_sql_cfg->dl) >= mod_sql_cfg->max_db)
	mod_sql_cfg->dl = dlist_remove_last(mod_sql_cfg->dl);
        
      mod_sql_cfg->dl = dlist_prepend(ctx->gc_ctx, mod_sql_cfg->dl, dbname,
				      db_from_mysql(ctx, domain, 
						    user, pwd, dbname,
						    sql_query));
      STACK_DROP(ctx->ovm_stack, 5);
      PUSH_VALUE(ctx, mod_sql_cfg->dl->head->db);
    }

  gc_base_free(sql_query);
 err_format:
  gc_base_free(tab);
 err_tab:
  gc_base_free(dbname);
 err_dbname:
  gc_base_free(pwd);
 err_pwd:
  gc_base_free(user);
 err_user:
  gc_base_free(domain);
 err_domain:
  return;
}
#endif

/* SQLite3 databases management */

#ifdef HAVE_SQLITE
int db_from_sqlite_3_callback (void *data, int ncols,
			       char **cols, char **colnames)
{
  db_from_sqlite3_data *sqdata = data;
  gc_t *gc_ctx = sqdata->ctx->gc_ctx;
  db_small_table *t;
  int i;
  db_map *s;
  char *str;
  OBJTYPE obj;

  GC_START(gc_ctx, 2);

  if (sqdata->nfields<0)
    sqdata->nfields = ncols;
  else if (sqdata->nfields!=ncols)
  {
    sqdata->error = 1;
    return -1; /* stop: number of fields is inconsistent from row to row */
  }
  
  t = gc_alloc(gc_ctx, DBST_SIZE(ncols), &db_small_table_class);
  t->gc.type = T_DB_SMALL_TABLE;
  t->nfields = 0;
  t->next = NULL;
  GC_UPDATE(gc_ctx, 0, t);
  
  for (i=0; i<ncols; )
    {
      str = cols[i];
      if (str==NULL)
	t->tuple[i] = NULL;
      else
	{
	  obj = ovm_str_new(gc_ctx, strlen(str));
	  memcpy(STR(obj), str, STRLEN(obj));
	  GC_TOUCH(gc_ctx, t->tuple[i] = obj);
	}
      t->nfields = ++i;
    }

  s = gc_alloc(gc_ctx, DB_TUPLE_SIZE(ncols), &db_tuples_class);
  GC_TOUCH(gc_ctx, s->what.tuples.table = t);
  GC_UPDATE(gc_ctx, 1, s);
  for (i=0; i<ncols; i++)
    {
      obj = t->tuple[i];
      if (obj==NULL)
    	s->what.tuples.hash[i] = 0; /* NULL has hash code 0 */
      else 
	s->what.tuples.hash[i] = issdl_hash (obj);
    }
  
  GC_TOUCH(gc_ctx, *sqdata->db = db_union (gc_ctx, ncols, *sqdata->db, s));
  GC_END(gc_ctx);
  return 0; /* proceed with other rows */ 
}

db_map *db_from_sqlite3(orchids_t *ctx, char *filename, char *sql_query)
{
  sqlite3 *db3;
  db_map *dbm;
  db_from_sqlite3_data data;
  int err;
  char *errmsg;

  GC_START(ctx->gc_ctx, 1);
  err = sqlite3_open_v2(filename, &db3, SQLITE_OPEN_READONLY, NULL);  
  if (err == SQLITE_OK)
  {
    data.db = (db_map **)&GC_LOOKUP(0);
    data.nfields = -1; /* unknown yet */
    data.error = 0;
    data.ctx = ctx;

    sqlite3_exec(db3, sql_query, db_from_sqlite_3_callback, &data, &errmsg);
    if (errmsg != NULL)
    {
      DebugLog(DF_ENG, DS_ERROR, "SQL query failed ; %%s\n", errmsg);
      sqlite3_free(errmsg);
    }
  }
  else
  {
    DebugLog(DF_ENG, DS_ERROR, "Could not open sqlite3 database %s: %s\n",
	     filename, sqlite3_errmsg(db3));
  }
  sqlite3_close(db3);
  dbm = (db_map *)GC_LOOKUP(0);
  GC_END(ctx->gc_ctx);
  return dbm;
}

static void issdl_load_sqlite3(orchids_t *ctx, state_instance_t *state)
{
  node *n;
  char *path;
  char *tab;
  char *sql_query;
  size_t i, len;

  path = str_from_stack_arg(ctx, 2, 2);
  if (path==NULL) goto err_path;
  tab = str_from_stack_arg(ctx, 1,2);
  if (tab==NULL) goto err_tab;

  len = strlen (tab);
  for (i = 0; i<len; i++)
  {
    if (!isalnum(tab[i]))
      {
	DebugLog(DF_ENG, DS_ERROR,
		 "Database table name must only contain alphanumerical characters");
	STACK_DROP(ctx->ovm_stack, 2);
	PUSH_VALUE(ctx, NULL);
	goto err_format;
    }
  }

  sql_query = gc_base_malloc(ctx->gc_ctx, strlen(tab) + 15);
  strncpy(sql_query, "select * from ", 15);
  strncat(sql_query, tab, strlen(tab));

  n = dlist_find(mod_sql_cfg->dl, path);
  if (n != NULL)
    {
      mod_sql_cfg->dl = dlist_move_into_lead(mod_sql_cfg->dl, n);
      STACK_DROP(ctx->ovm_stack, 2);
      PUSH_VALUE(ctx, n->db);
    }
  else
    {
      if (dlist_length(mod_sql_cfg->dl) >= mod_sql_cfg->max_db)
        mod_sql_cfg->dl = dlist_remove_last(mod_sql_cfg->dl);
      
      mod_sql_cfg->dl = dlist_prepend(ctx->gc_ctx, mod_sql_cfg->dl, path,
				      db_from_sqlite3(ctx, path, sql_query));
      STACK_DROP(ctx->ovm_stack, 2);
      PUSH_VALUE(ctx, mod_sql_cfg->dl->head->db);
    }
  gc_base_free(sql_query);
  
 err_format:
  gc_base_free(tab);
 err_tab:
  gc_base_free(path);
 err_path:
  return;
}
#endif

#ifdef HAVE_MYSQL
static const type_t *load_mysql_sig[] = { &t_db_any, &t_str, &t_str, &t_str, &t_str, &t_str };
static const type_t **load_mysql_sigs[] = { load_mysql_sig, NULL };
#endif

#ifdef HAVE_SQLITE
static const type_t *load_sqlite3_sig[] = { &t_db_any, &t_str, &t_str };
static const type_t **load_sqlite3_sigs[] = { load_sqlite3_sig, NULL };
#endif

static void *mod_sql_preconfig(orchids_t *ctx, mod_entry_t *mod)
{

  DebugLog(DF_MOD, DS_INFO, "load() sql@%p\n", &mod_sql);

#ifdef HAVE_MYSQL
  register_lang_function(ctx,
			 issdl_load_mysql,
			 "load_mysql",
			 5, load_mysql_sigs,
			 m_random,
			 "load a MySQL database");
#endif

#ifdef HAVE_SQLITE
  register_lang_function(ctx,
			 issdl_load_sqlite3,
			 "load_sqlite3",
			 2, load_sqlite3_sigs,
			 m_random,
			 "load a SQLite3 database");
#endif

  /* this doubly-linked list will allow us to know if a database is already 
     loaded in the orchids virtual machine memory. Databases loading is
     managed in a Least Recently Used style. 
  */
  dlist *dl = dlist_new(ctx->gc_ctx);
  /* issdl_* primitives don't have a mod_entry_t as argument. Hence
     we need to use a global variable mod_sql_cfg.
   */
  mod_sql_cfg = gc_base_malloc(ctx->gc_ctx, sizeof (sql_config_t));
  mod_sql_cfg->max_db = 10;
  mod_sql_cfg->dl = dl;
  return mod_sql_cfg;
}

static void sql_set_max_db(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  long value;

  DebugLog(DF_MOD, DS_INFO, "setting MaxDatabases to %s\n", dir->args);

  value = strtol(dir->args, (char**)NULL, 10);
  if (value < 0)
    value = 10;

  // mod_sql_cfg->max_db = value;
  ((sql_config_t *)mod->config)->max_db = value;
}

static mod_cfg_cmd_t sql_dir[] = 
{
  {"MaxDatabases", sql_set_max_db, "set the maximum loading databases number"},
  {NULL, NULL, NULL}
};

input_module_t mod_sql = {
  MOD_MAGIC,
  ORCHIDS_VERSION,
  0,
  "sql",
  "CeCILL2",
  NULL,
  sql_dir,
  mod_sql_preconfig,
  NULL, /* postconfig */
  NULL, /* postcompil */
  NULL, /* predissect */
  NULL, /* dissect*/
  NULL, /* dissect type */
  NULL, /* save */
  NULL  /*restore */
};

/*
** Copyright (c) 2015 by Pierre-Arnaud SENTUCQ, Laboratoire Spécification
** et Vérification (LSV), CNRS UMR 8643 & ENS Cachan.
**
** Pierre-Arnaud SENTUCQ <pierre-arnaud.sentucq@lsv.ens-cachan.fr>
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
