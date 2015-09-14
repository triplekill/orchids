/**
 ** @file mod_binfile.h
 ** binary file input module, for binary log files.
 **
 ** @author Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 ** @date  Started on: Sam  9 mai 2015 16:51:40 UTC
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef MOD_BINFILE_H
#define MOD_BINFILE_H

#define BF_FIELDS  2
#define BF_FILE     0
#define BF_BLOCK    1

// Static buffer size
#define BIN_BUFF_SIZE	   1024

#define DEFAULT_MODBIN_POLL_PERIOD 10
#define INITIAL_MODBIN_POLL_DELAY  0

typedef struct binfile_s binfile_t;
struct binfile_s
{
  struct binfile_s *next;
  ovm_var_t *file_name;
  int fd;
  struct stat file_stat;
  unsigned char eof;
};

typedef struct binfile_config_s binfile_config_t;
struct binfile_config_s
{
  int flags;
  int process_all_data;
  int exit_process_all_data;
  int poll_period;
  struct binfile_s *file_list;
};


typedef struct binsock_s binsock_t;
struct binsock_s
{
#define BINSOCK_LINE_TOO_LONG 0x1
#define BINSOCK_ISSOCK 0x2
  int flags;
  ovm_var_t *file_name;
  int fd;
};

static int binfile_callback(orchids_t *ctx, mod_entry_t *mod, void *dummy);
static void add_input_binfile(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir);
static void *binfile_preconfig(orchids_t *ctx, mod_entry_t *mod);
static void binfile_postconfig(orchids_t *ctx, mod_entry_t *mod);
static void binfile_postcompil(orchids_t *ctx, mod_entry_t *mod);


static void add_input_binfile(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir);
static void binfile_set_process_all(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir);
static void binfile_set_exit_process_all(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir);
static void binfile_set_poll_period(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir);

static int rtaction_read_binfiles(orchids_t *ctx, heap_entry_t *he);


#endif /* MOD_BINFILE_H */

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
