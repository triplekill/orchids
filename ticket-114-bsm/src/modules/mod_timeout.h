/**
 ** @file mod_timeout.h
 ** Definitions for mod_timeout.c
 ** 
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 ** 
 ** @version 0.1.0
 ** 
 ** @date  Started on: Fri Jun 15 10:53:53 2007
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef MOD_TIMEOUT_H
#define MOD_TIMEOUT_H

#define TIMEOUT_FIELDS  7
#define F_DATE          0
#define F_REGDATE       1
#define F_NAME          2
#define F_RULE          3
#define F_RULEINST      4
#define F_STATE         5
#define F_STATEINST     6


static int
timeout_rtcallback(orchids_t *ctx, rtaction_t *e);


static void
issdl_timeout(orchids_t *ctx, state_instance_t *state);


static void *
timeout_preconfig(orchids_t *ctx, mod_entry_t *mod);


#endif /* MOD_TIMEOUT_H */
