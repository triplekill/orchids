/**
 ** @file mod_textfile.c
 ** textfile input module, for text log files.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 ** @date  Started on: Wed Jan 15 17:07:26 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef MOD_TEXTFILE_H
#define MOD_TEXTFILE_H

#define TF_FIELDS  3
#define F_LINE_NUM 0
#define F_FILE     1
#define F_LINE     2

// Static buffer size
#define BUFF_SZ	   1024
// Maximum line size
#define MAX_LINE_SZ 10000

/*
** module flags:
** the two LSBs identify integrity check method
** the third bit control file safety checks
*/
#define TXTFILE_CHK_NONE    0x00
#define TXTFILE_CHK_CRC32   0x01
#define TXTFILE_CHK_MD5     0x02
#define TXTFILE_CHK_SHA1    0x03
#define TXTFILE_SAFE_REOPEN 0x04

#define HASH_SIZE 32

#define DEFAULT_MODTEXT_POLL_PERIOD 10
#define INITIAL_MODTEXT_POLL_DELAY  0

typedef struct textfile_s textfile_t;
struct textfile_s
{
  struct textfile_s *next;
  char *filename;
  size_t filename_len;
  FILE *fd;
  struct stat file_stat;
  unsigned int line;
  unsigned char hash[HASH_SIZE];
  unsigned char eof;
};

typedef struct textfile_config_s textfile_config_t;
struct textfile_config_s
{
  int flags;
  int process_all_data;
  int exit_process_all_data;
  int poll_period;
  struct textfile_s *file_list;
};


typedef struct textsock_s textsock_t;
struct textsock_s
{
  size_t buf_sz;
  char *buf;
#define TEXTSOCK_LINE_TOO_LONG 0x1
#define TEXTSOCK_ISSOCK 0x2
  int flags;
  unsigned int line;
  off_t read_off, write_off;
  char *filename;
  size_t filename_len;
  int fd;
};

static void
textfile_buildevent(orchids_t *ctx, mod_entry_t *mod, textfile_t *tf, char *buf);


static int
textfile_callback(orchids_t *ctx, mod_entry_t *mod, void *dummy);


static void
add_input_file(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir);


static void *
textfile_preconfig(orchids_t *ctx, mod_entry_t *mod);


static void
textfile_postconfig(orchids_t *ctx, mod_entry_t *mod);


static void
textfile_postcompil(orchids_t *ctx, mod_entry_t *mod);


static void
add_input_file(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir);


static void
set_process_all(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir);


static void
set_poll_period(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir);


static int
rtaction_read_files(orchids_t *ctx, rtaction_t *e);


#endif /* MOD_TEXTFILE_H */

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
