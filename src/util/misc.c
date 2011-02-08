/**
 ** @file misc.c
 ** Misc functions.
 ** 
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 ** 
 ** @version 0.1.0
 ** @ingroup util
 ** 
 ** @date  Started on: Fri Sep 10 18:01:39 2004
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>

#include "orchids.h"

void
tvdiff(struct timeval *dst, const struct timeval *ref1, const struct timeval *ref2)
{
  dst->tv_usec = ref1->tv_usec - ref2->tv_usec;
  if (dst->tv_usec < 0) {
    dst->tv_usec += 1000000;
    dst->tv_sec = ref1->tv_sec - ref2->tv_sec - 1;
  } else {
    dst->tv_sec = ref1->tv_sec - ref2->tv_sec;
  }
}



void
fprintf_uptime(FILE *fp, time_t uptime)
{
  struct tm *tm_val;
  char tmp[128];
  char buf[128];

  tm_val = gmtime(&uptime);
  tm_val->tm_year -= 70;

  buf[0] = '\0';

  if (tm_val->tm_year > 0) {
    if (tm_val->tm_year == 1)
      snprintf(tmp, sizeof (tmp), "%i year", tm_val->tm_year);
    else
      snprintf(tmp, sizeof (tmp), "%i years", tm_val->tm_year);

    strcat(buf, tmp);
  }

  if (tm_val->tm_yday > 0) {
    if (buf[0] != '\0')
      strcat(buf, ", ");

    if (tm_val->tm_yday == 1)
      snprintf(tmp, sizeof (tmp), "%i day", tm_val->tm_yday);
    else
      snprintf(tmp, sizeof (tmp), "%i days", tm_val->tm_yday);

    strcat(buf, tmp);
  }

  if (tm_val->tm_hour > 0) {
    if (buf[0] != '\0')
      strcat(buf, ", ");

    if (tm_val->tm_hour == 1)
      snprintf(tmp, sizeof (tmp), "%i hour", tm_val->tm_hour);
    else
      snprintf(tmp, sizeof (tmp), "%i hours", tm_val->tm_hour);

    strcat(buf, tmp);
  }

  if (tm_val->tm_min > 0) {
    if (buf[0] != '\0')
      strcat(buf, ", ");

    if (tm_val->tm_min == 1)
      snprintf(tmp, sizeof (tmp), "%i minute", tm_val->tm_min);
    else
      snprintf(tmp, sizeof (tmp), "%i minutes", tm_val->tm_min);

    strcat(buf, tmp);
  }

  if (tm_val->tm_sec > 0) {
    if (buf[0] != '\0')
      strcat(buf, ", ");

    if (tm_val->tm_sec == 1)
      snprintf(tmp, sizeof (tmp), "%i second", tm_val->tm_sec);
    else
      snprintf(tmp, sizeof (tmp), "%i seconds", tm_val->tm_sec);

    strcat(buf, tmp);
  }

  if (buf[0] != '\0')
    strcat(buf, ".");
  else
    strcat(buf, "Uptime is null.");

  fprintf(fp, "%s", buf);
}

int
snprintf_uptime(char *str, size_t size, time_t uptime)
{
  struct tm *tm_val;
  char tmp[128];
  char buf[128];
  int ret;

  tm_val = gmtime(&uptime);
  tm_val->tm_year -= 70;

  buf[0] = '\0';

  if (tm_val->tm_year > 0) {
    if (tm_val->tm_year == 1)
      snprintf(tmp, sizeof (tmp), "%i year", tm_val->tm_year);
    else
      snprintf(tmp, sizeof (tmp), "%i years", tm_val->tm_year);

    strcat(buf, tmp);
  }

  if (tm_val->tm_yday > 0) {
    if (buf[0] != '\0')
      strcat(buf, ", ");

    if (tm_val->tm_yday == 1)
      snprintf(tmp, sizeof (tmp), "%i day", tm_val->tm_yday);
    else
      snprintf(tmp, sizeof (tmp), "%i days", tm_val->tm_yday);

    strcat(buf, tmp);
  }

  if (tm_val->tm_hour > 0) {
    if (buf[0] != '\0')
      strcat(buf, ", ");

    if (tm_val->tm_hour == 1)
      snprintf(tmp, sizeof (tmp), "%i hour", tm_val->tm_hour);
    else
      snprintf(tmp, sizeof (tmp), "%i hours", tm_val->tm_hour);

    strcat(buf, tmp);
  }

  if (tm_val->tm_min > 0) {
    if (buf[0] != '\0')
      strcat(buf, ", ");

    if (tm_val->tm_min == 1)
      snprintf(tmp, sizeof (tmp), "%i minute", tm_val->tm_min);
    else
      snprintf(tmp, sizeof (tmp), "%i minutes", tm_val->tm_min);

    strcat(buf, tmp);
  }

  if (tm_val->tm_sec > 0) {
    if (buf[0] != '\0')
      strcat(buf, ", ");

    if (tm_val->tm_sec == 1)
      snprintf(tmp, sizeof (tmp), "%i second", tm_val->tm_sec);
    else
      snprintf(tmp, sizeof (tmp), "%i seconds", tm_val->tm_sec);

    strcat(buf, tmp);
  }

  if (buf[0] != '\0')
    strcat(buf, ".");
  else
    strcat(buf, "Uptime is null.");

  ret = snprintf(str, size, "%s", buf);

  return (ret);
}

int
snprintf_uptime_short(char *str, size_t size, time_t uptime)
{
  struct tm *tm_val;
  char tmp[128];
  char buf[128];
  int ret;

  tm_val = gmtime(&uptime);
  tm_val->tm_year -= 70;

  buf[0] = '\0';

  if (tm_val->tm_year > 0) {
    snprintf(tmp, sizeof (tmp), "%iy", tm_val->tm_year);
    strcat(buf, tmp);
  }

  if (tm_val->tm_yday > 0) {
    if (buf[0] != '\0')
      strcat(buf, " ");
    snprintf(tmp, sizeof (tmp), "%id", tm_val->tm_yday);
    strcat(buf, tmp);
  }

  if (tm_val->tm_hour > 0) {
    if (buf[0] != '\0')
      strcat(buf, " ");
    snprintf(tmp, sizeof (tmp), "%ih", tm_val->tm_hour);
    strcat(buf, tmp);
  }

  if (tm_val->tm_min > 0) {
    if (buf[0] != '\0')
      strcat(buf, " ");
    snprintf(tmp, sizeof (tmp), "%im", tm_val->tm_min);
    strcat(buf, tmp);
  }

  if (tm_val->tm_sec > 0) {
    if (buf[0] != '\0')
      strcat(buf, " ");

    snprintf(tmp, sizeof (tmp), "%is", tm_val->tm_sec);
    strcat(buf, tmp);
  }

  if (buf[0] == '\0')
    strcat(buf, "Uptime is null.");

  ret = snprintf(str, size, "%s", buf);

  return (ret);
}


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
