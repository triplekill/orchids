/**
 ** @file mod_win32evt.c
 ** A win32evt for new modules.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 ** @date  Started on: Fri Feb  7 11:07:42 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef MOD_WIN32EVT_H
#define MOD_WIN32EVT_H

/*
typedef unsigned long DWORD;
typedef unsigned short WORD;
*/

#define MSEVT_RESERVED 0x654C664C

#define MSEVT_MAGIC1 0x11111111UL
#define MSEVT_MAGIC2 0x22222222UL
#define MSEVT_MAGIC3 0x33333333UL
#define MSEVT_MAGIC4 0x44444444UL

#define WIN32EVT_FIELDS 7
#define F_RECNUM        0
#define F_GENTIME       1
#define F_WRITIME       2
#define F_EVENTID       3
#define F_EVENTTYPE     4
#define F_EVENTCAT      5
#define F_USERSID       6
#define F_DATA          7


typedef struct event_log_header_s event_log_header_t;
struct event_log_header_s
{
  unsigned long length; /* length of header record == 48 */
  unsigned long reserved; /* magic number == MSEVT_RESERVER */
  unsigned long val0; /* ??? usually == 1 */
  unsigned long val1; /* ??? usually == 1 */
  unsigned long first_record_offset; /* offset of the first event record */
  unsigned long trailer_record_offset; /* offset of the trailer record */
  unsigned long number_of_records; /* number of records (events + 1 for header)*/
  unsigned long val5; /* ??? usually == 1 */
  unsigned long used_file_size; /* file size */
  unsigned long pending_events; /* event writed on file (at the trailer_record_offset) */
  unsigned long time_limit;
  unsigned long length_end;
};


typedef struct event_log_trailer_s event_log_trailer_t;
struct event_log_trailer_s
{
  unsigned long length;
  unsigned long magic1;
  unsigned long magic2;
  unsigned long magic3;
  unsigned long magic4;
  unsigned long first_record_new_offset;
  unsigned long trailer_record_new_offset;
  unsigned long new_number_of_records;
  unsigned long val4;
  unsigned long length_end;
};

#define EVENT_HEADER_SIZE 56


typedef struct event_log_record_s event_log_record_t;
struct event_log_record_s
{
  unsigned long length;
  unsigned long reserved;
  unsigned long record_number;
  unsigned long time_generated;
  unsigned long time_written;
  unsigned long event_id;
  unsigned short event_type;
  unsigned short num_strings;
  unsigned short event_category;
  unsigned short reserved_flags;
  unsigned long closing_record_number;
  unsigned long string_offset;
  unsigned long user_sid_length;
  unsigned long user_sid_offset;
  unsigned long data_length;
  unsigned long data_offset;

  unsigned char extra_data[0]; /* NOT ISO C89 ! */

/*   wchar_t source_name[]; */
/*   wchar_t computername[]; */
/*   unsigned char user_sid[]; */
/*   wchar_t strings[]; */
/*   unsigned char data[]; */
/*   char pad[]; */
/*   unsigned int pad_len; */
/*   unsigned long length_end; */

};

typedef struct win32evt_config_s win32evt_config_t;
struct win32evt_config_s
{
  void *files;
};


#ifdef UNUSED
static void
fprintf_event_log_header(FILE *fp, event_log_header_t *hdr);
#endif

#ifdef UNUSED
static void
fprintf_event_log_trailer(FILE *fp, event_log_trailer_t *trailer);
#endif

#ifdef UNUSED
static void
fprintf_event_log_record(FILE *fp, event_log_record_t *event);
#endif


#ifdef UNUSED
static int
get_next_event_log_record_2(FILE *fp, event_log_record_t *e, size_t elen);
#endif


#ifdef UNUSED
static event_log_record_t *
get_next_event_log_record(FILE *fp);
#endif

#ifdef UNUSED
static void
read_record(const char *file);
#endif


static void *
win32evt_preconfig(orchids_t *ctx, mod_entry_t *mod);


static void
win32evt_postconfig(orchids_t *ctx, mod_entry_t *mod);


static void
win32evt_postcompil(orchids_t *ctx, mod_entry_t *mod);





#endif /* MOD_WIN32EVT_H */

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
