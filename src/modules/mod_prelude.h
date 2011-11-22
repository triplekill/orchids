#ifndef MOD_PRELUDE_H_
# define MOD_PRELUDE_H_

#include <libprelude/prelude.h>
#include <libprelude/prelude-client.h>
#include <libprelude/idmef.h>
#include <libprelude/idmef-message-print.h>
#include <libpreludedb/preludedb.h>
#include <libpreludedb/preludedb-sql.h>

#include "orchids.h"

#include "orchids_api.h"
#include "evt_mgr.h"

#define INITIAL_MODPRELUDE_POLL_DELAY  1
#define DEFAULT_MODPRELUDE_POLL_PERIOD 1

#define MAX_PRELUDE_FIELDS	32

#define F_PTR		0

enum prelude_mode_e {
  PRELUDE_MODE_SENSOR,
  PRELUDE_MODE_ANALYZER,
  PRELUDE_MODE_PREWIKKA
};

typedef struct modprelude_s modprelude_t;
struct modprelude_s
{
    prelude_client_t *	client;
    prelude_io_t	*prelude_io;
    preludedb_t		*db;
    enum prelude_mode_e	mode;
    const char*		profile;
    const char*		prelude_db_settings;
    int			poll_period;
    unsigned int       	nb_fields;
    const char*		field_xpath[MAX_PRELUDE_FIELDS];
};

#endif /* !MOD_PRELUDE_H_ */
