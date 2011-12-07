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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <wchar.h>
#include <string.h>

#include "orchids.h"

#include "orchids_api.h"

#include "mod_win32evt.h"

input_module_t mod_win32evt;


#ifdef UNUSED
static void
fprintf_event_log_header(FILE *fp, event_log_header_t *hdr)
{
  fprintf(fp, "---------------[ log header ]-------------\n");
  fprintf(fp, "               length: %lu\n", hdr->length);
  fprintf(fp, "       magic reserved: 0x%08lx\n", hdr->reserved);
  fprintf(fp, "                val 0: 0x%08lx (%lu) (?)\n",
          hdr->val0, hdr->val0);
  fprintf(fp, "                val 1: 0x%08lx (%lu) (?)\n",
          hdr->val1, hdr->val1);
  fprintf(fp, "  first_record_offset: 0x%08lx (%lu)\n",
          hdr->first_record_offset, hdr->first_record_offset);
  fprintf(fp, "trailer_record_offset: 0x%08lx (%lu)\n",
          hdr->trailer_record_offset, hdr->trailer_record_offset);
  fprintf(fp, "    number_of_records: 0x%08lx (%lu)\n",
          hdr->number_of_records, hdr->number_of_records);
  fprintf(fp, "                val 5: 0x%08lx (%lu) (pending data ?)\n",
          hdr->val5, hdr->val5);
  fprintf(fp, "       used_file_size: 0x%08lx (%lu)\n",
          hdr->used_file_size, hdr->used_file_size);
  fprintf(fp, "       pending_events: 0x%08lx (%lu)\n",
          hdr->pending_events, hdr->pending_events);
  fprintf(fp, "           time_limit: 0x%08lx (%lu s)\n",
          hdr->time_limit, hdr->time_limit);
  fprintf(fp, "           length_end: 0x%08lx (%lu)\n",
          hdr->length_end, hdr->length_end);
}
#endif


#ifdef UNUSED
static void
fprintf_event_log_trailer(FILE *fp, event_log_trailer_t *trailer)
{
  fprintf(fp, "---------------[ log trailer ]-------------\n");
  fprintf(fp, "                   length: %lu\n", trailer->length);
  fprintf(fp, "                  magic 1: 0x%08lx (should be 0x11111111)\n",
          trailer->magic1);
  fprintf(fp, "                  magic 2: 0x%08lx (should be 0x22222222)\n",
          trailer->magic2);
  fprintf(fp, "                  magic 3: 0x%08lx (should be 0x33333333)\n",
          trailer->magic3);
  fprintf(fp, "                  magic 4: 0x%08lx (should be 0x44444444)\n",
          trailer->magic4);
  fprintf(fp, "  first_record_new_offset: 0x%08lx (%lu)\n",
          trailer->first_record_new_offset, trailer->first_record_new_offset);
  fprintf(fp, "trailer_record_new_offset: 0x%08lx (%lu)\n",
          trailer->trailer_record_new_offset,
          trailer->trailer_record_new_offset);
  fprintf(fp, "    new_number_of_records: 0x%08lx (%lu)\n",
          trailer->new_number_of_records, trailer->new_number_of_records);
  fprintf(fp, "                    val 4: 0x%08lx (%lu)\n",
          trailer->val4, trailer->val4);
  fprintf(fp, "               length_end: 0x%08lx (%lu)\n",
          trailer->length_end, trailer->length_end);
}
#endif


#ifdef UNUSED
static void
fprintf_event_log_record(FILE *fp, event_log_record_t *event)
{
  char asc_time[32];
  char *str;

  fprintf(fp, "---------------[ event ]-------------\n");
  fprintf(fp, "               length: %lu\n", event->length);
  fprintf(fp, "             reserved: 0x%08lx\n", event->reserved);
  fprintf(fp, "        record_number: %lu\n", event->record_number);
  strftime(asc_time, 32, "%a %b %d %H:%M:%S %Y",
           localtime((time_t *)&event->time_generated));
  fprintf(fp, "       time_generated: %lu (%.32s)\n",
          event->time_generated, asc_time);
  strftime(asc_time, 32, "%a %b %d %H:%M:%S %Y",
           localtime((time_t *)&event->time_written));
  fprintf(fp, "         time_written: %lu (%.32s)\n",
          event->time_written, asc_time);
  fprintf(fp, "             event_id: %lu\n", event->event_id);
  fprintf(fp, "           event_type: %hu\n", event->event_type);
  fprintf(fp, "          num_strings: %hu\n", event->num_strings);
  fprintf(fp, "       event_category: %hu\n", event->event_category);
  fprintf(fp, "       reserved_flags: 0x%04hx\n", event->reserved_flags);
  fprintf(fp, "closing_record_number: %lu\n", event->closing_record_number);
  fprintf(fp, "        string_offset: %lu\n", event->string_offset);
  fprintf(fp, "      user_sid_length: %lu\n", event->user_sid_length);
  fprintf(fp, "      user_sid_offset: %lu\n", event->user_sid_offset);
  fprintf(fp, "          data_length: %lu\n", event->data_length);
  fprintf(fp, "          data_offset: %lu\n", event->data_offset);

  /* source name */
  fprintf(fp, "          source_name: ");
  str = (char *)event->extra_data;
  while (*str) {
    fprintf(fp, "%c", *str);
    str += 2;
  }
  fprintf(fp, "\n");

  /* computer name */
  fprintf(fp, "         computername: ");
  str += 2;
  while (*str) {
    fprintf(fp, "%c", *str);
    str += 2;
  }
  fprintf(fp, "\n");

  /* user security identifier */
  if (event->user_sid_length) {
    int i;

    fprintf(fp, "             user_sid: ");
    for (i = 0; i < event->user_sid_length; ++i)
      fprintf(fp, "%02hhx", ((unsigned char *)event)[event->user_sid_offset + i] );
    fprintf(fp, "\n");
  }
  else {
    fprintf(fp, "             user_sid: none\n");
  }

  /* strings  */
  if (event->num_strings) {
    int s;

    str = (char *) event + event->string_offset;
    for (s = 0; s < event->num_strings; ++s) {
      fprintf(fp, "         string %5i: ", s);
      while (*str) {
        fprintf(fp, "%c", *str);
        str += 2;
      }
      str += 2;
      fprintf(fp, "\n");
    }
  }
  else {
    fprintf(fp, "              strings: none\n");
  }

  /* data */
  if (event->data_length) {
    int i;

    fprintf(fp, "                 data: \n");
    str = (char *) event + event->data_offset;

    for (i = 0; i < event->data_length; ++i) {
      if (i && !(i % 4))
        printf(" ");
      if (i && !(i % 16))
        printf("\n");
      printf("%02hhx ", (unsigned char) str[i]);
    }
    printf("\n");

    for (i = 0; i < event->data_length; ++i) {
      if (isprint(str[i]) || str[i] == '\n')
        fprintf(fp, "%c", str[i]);
      else if (str[i] != '\r')
        fprintf(fp, ".");
    }
    fprintf(fp, "\n");
  }
  else {
    fprintf(fp, "                 data: <no data>\n");
  }
  
  fprintf(fp, "\n");
}
#endif


#ifdef UNUSED
static int
get_next_event_log_record_2(FILE *fp, event_log_record_t *e, size_t elen)
{
  int sz;

  if (elen < EVENT_HEADER_SIZE) {
    printf("alloc\n");
    return (-1);
  }

  sz = fread(e, EVENT_HEADER_SIZE, 1, fp); /* first read event header */
  if (sz != 1)
    {
      printf("read error\n");
      return (-1);
    }
  if (e->reserved != MSEVT_RESERVED)
    {
      printf("corrupted event record (%08lx/%08x)\n", e->reserved, MSEVT_RESERVED);
      return (-1);
    }

  if (e->length > EVENT_HEADER_SIZE) /* if there is extra data... */
    {
      if ((e->length - EVENT_HEADER_SIZE) <= elen) /* ...and enought space */
        sz = fread(e->extra_data, e->length - EVENT_HEADER_SIZE, 1, fp);
      else
        {
          printf("mem error\n");
          return (-1);
        }
    }

  return (0);
}
#endif

#ifdef UNUSED
static event_log_record_t *
get_next_event_log_record(FILE *fp)
{
  event_log_record_t header;

  /* first read event header */
  fread(&header, 1, 1, fp);

  return (NULL);
}
#endif

#ifdef UNUSED
static void
read_record(const char *file)
{
  FILE *fp;
  /* unsigned char header[48]; */
  event_log_header_t hdr;
  event_log_trailer_t trailer;
  int i;
  unsigned char data[65336];
  long off;

  fp = fopen(file, "r");
  if (fp == NULL) {
    perror("fopen()");
    exit(EXIT_FAILURE);
  }

  memset(data, 0, sizeof(data));

  /* read first header record (sz = 48 instead 56) */
  fread(&hdr, 48, 1, fp);
  printf("header:\n");
  for (i = 0; i < 48; ++i) {
    if (i && !(i % 4))
      printf(" ");
    if (i && !(i % 16))
      printf("\n");
    printf("%02hhx ", ((unsigned char *)&hdr)[i]);
  }
  printf("\n");
  fprintf_event_log_header(stdout, &hdr);

  fseek(fp, hdr.first_record_offset, SEEK_SET);

  /**/
/*   fseek(fp, hdr.trailer_record_offset, SEEK_SET); */
/*   fread(&trailer, 40, 1, fp); */

/*   printf("trailer:\n"); */
/*   for (i = 0; i < 40; ++i) */
/*     { */
/*       if (i && !(i % 4)) */
/*         printf(" "); */
/*       if (i && !(i % 16)) */
/*         printf("\n"); */
/*       printf("%02hhx ", ((unsigned char *)&trailer)[i]); */
/*     } */
/*   printf("\n"); */
/*   fprintf_event_log_trailer(stdout, &trailer); */
/*   return (0); */
  /**/

/*   while (!get_next_event_log_record_2(fp, (event_log_record_t *) data, sizeof(data))) */
/*     fprintf_event_log_record(stdout, (event_log_record_t *) data); */

  if (hdr.number_of_records > 1) {
    /* try to get the first event */
    if (get_next_event_log_record_2(fp, (event_log_record_t *) data, sizeof(data))) {
      /* recover */

      fprintf(stderr, "entry point error... trying to recover from trailer record...\n");
      for (i=48; !feof(fp); i++) {
        unsigned char ref[16] = { 0x11, 0x11, 0x11, 0x11, 0x22, 0x22, 0x22, 0x22, 0x33, 0x33, 0x33, 0x33, 0x44, 0x44, 0x44, 0x44 };
        unsigned char buf[16];

        fseek(fp, i, SEEK_SET);
        fread(buf, 16, 1, fp);
        if (!memcmp(ref, buf, 16)) {
          fprintf(stderr, "found trailer record at 0x%08x\n", i);
          break ;
        }
      }
      fseek(fp, i - 4, SEEK_SET);
      fread(&trailer, sizeof(trailer), 1, fp);
      fprintf_event_log_trailer(stdout, &trailer);
      hdr.number_of_records = trailer.new_number_of_records;
      hdr.first_record_offset = trailer.first_record_new_offset;
      hdr.trailer_record_offset = trailer.trailer_record_new_offset;

      fseek(fp, hdr.first_record_offset, SEEK_SET);

      if (get_next_event_log_record_2(fp, (event_log_record_t *) data, sizeof(data))) {
        fprintf(stderr, "recover error\n");
        exit(1);
      }
      fprintf_event_log_record(stdout, (event_log_record_t *) data);
    }

    for (i = 1; i < hdr.number_of_records - 1; i++) {
      if (get_next_event_log_record_2(fp, (event_log_record_t *) data, sizeof(data))) {
        fprintf(stderr, "read error 1\n");
        exit(EXIT_FAILURE);
      }
      fprintf_event_log_record(stdout, (event_log_record_t *) data);
    }
  }

  off = ftell(fp);
  if (off != hdr.trailer_record_offset) {
    fprintf(stderr, "file corrupted (%li/%lu)\n", off, hdr.trailer_record_offset);
    exit(EXIT_FAILURE);
  }
  else {
    fprintf(stderr, "*** events records OK. ***\n");
  }

  /* read pending events */
  for (i = 0; i < hdr.pending_events; i++) {
    if (get_next_event_log_record_2(fp, (event_log_record_t *) data, sizeof(data))) {
      fprintf(stderr, "read error 1\n");
      exit(EXIT_FAILURE);
    }
    fprintf_event_log_record(stdout, (event_log_record_t *) data);
  }

  /* seek to entry point */
  /* fseek(fp, hdr.trailer_record_offset, SEEK_SET); */

  printf("trailer pos = %lu\n", ftell(fp));

  /* normally, we should be at the trailer pos */
  fread(&trailer, 40, 1, fp);

  printf("trailer:\n");
  for (i = 0; i < 40; ++i) {
    if (i && !(i % 4))
      printf(" ");
    if (i && !(i % 16))
      printf("\n");
    printf("%02hhx ", ((unsigned char *)&trailer)[i]);
  }
  printf("\n");
  fprintf_event_log_trailer(stdout, &trailer);
}
#endif


#if WIN32EVT
static int
win32evt_dissector(orchids_t *ctx, mod_entry_t *mod, event_t *e, void *data)
{
  DebugLog(DF_MOD, DS_TRACE, "win32evt_dissector()\n");

  /* dissect event top attribute here, and add them to it */

  /* then, post resulting event */
  post_event(ctx, mod, e);

  return (0);
}
#endif

static field_t win32evt_fields[] = {
  { "win32evt.rec_num",    T_INT,     "Event record number"      },
  { "win32evt.gen_time",   T_CTIME,   "Generation date"          },
  { "win32evt.wri_time",   T_CTIME,   "Writing date"             },
  { "win32evt.event_id",   T_INT,     "Event identifier"         },
  { "win32evt.event_type", T_INT,     "Event type"               },
  { "win32evt.event_cat",  T_INT,     "Event category"           },
  { "win32evt.user_sid",   T_VBSTR,   "User security identifier" },
  { "win32evt.data",       T_VBSTR,   "Event data"               },
};


static void *
win32evt_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  win32evt_config_t *cfg;

  DebugLog(DF_MOD, DS_INFO, "load() win32evt@%p\n", &mod_win32evt);

  /* allocate some memory for module configuration
  ** and initialize default configuration. */
  cfg = Xzmalloc(sizeof (win32evt_config_t));

  register_fields(ctx, mod, win32evt_fields, WIN32EVT_FIELDS);

  /* return config structure, for module manager */
  return (cfg);
}


static void
win32evt_postconfig(orchids_t *ctx, mod_entry_t *mod)
{
  /* Do all thing needed _AFTER_ module configuration.
  ** (register configurable callbacks for examples) */
}


static void
win32evt_postcompil(orchids_t *ctx, mod_entry_t *mod)
{
  /* Do all thing needed _AFTER_ rule compilation. */
}


static mod_cfg_cmd_t win32evt_config_commands[] = {
  { NULL, NULL, NULL }
};


input_module_t mod_win32evt = {
  MOD_MAGIC,                /* Magic number */
  ORCHIDS_VERSION,          /* Module version */
  "win32evt",               /* module name */
  "CeCILL2",                /* module license */
  NULL,                     /* module dependencies */
  win32evt_config_commands, /* module configuration commands,
                               for core config parser */
  win32evt_preconfig,       /* called just after module registration */
  win32evt_postconfig,      /* called after all mods preconfig,
                               and after all module configuration*/
  win32evt_postcompil
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
