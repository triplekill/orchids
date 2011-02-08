/**
 ** @file timer.h
 ** Macro for timers.
 ** 
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 ** 
 ** @version 0.1.0
 ** @ingroup util
 ** 
 ** @date  Started on: Web Jan 22 16:53:54 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef TIMER_H
#define TIMER_H

#define Timer_Add(r, a, b) \
  do { \
    (r)->tv_usec = ((a)->tv_usec + (b)->tv_usec) % 1000000; \
    (r)->tv_sec  =  (a)->tv_sec +  (b)->tv_sec \
                   + (((a)->tv_usec + (b)->tv_usec) / 1000000); \
  } while (0)

#define Timer_Sub(r, a, b) \
  do { \
    if ((a)->tv_usec < (b)->tv_usec) { \
        (r)->tv_usec = 1000000 + (a)->tv_usec - (b)->tv_usec; \
        (r)->tv_sec = (a)->tv_sec - (b)->tv_sec - 1; \
      } else { \
        (r)->tv_usec = (a)->tv_usec - (b)->tv_usec; \
        (r)->tv_sec  = (a)->tv_sec  - (b)->tv_sec; \
      } \
  } while (0)

#define Timer_Float(t) ( (t)->tv_sec + ((t)->tv_usec / 1000000.0 ) )

#define  JAN_1970        0x83aa7e80      /* 2208988800 1970 - 1900 in seconds */
#define  TWO_POW_32    4294967296.0      /* 2^32 in doudle */
#define  MICRO_SEC           1.0e-6      /* one micro-second */

#define Timer_to_NTP(t, ntph, ntpl) \
do { \
  (ntph) = (t)->tv_sec + JAN_1970; \
  (ntpl) = (t)->tv_usec * TWO_POW_32 * MICRO_SEC; \
} while (0)

#endif /* TIMER_H */


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
