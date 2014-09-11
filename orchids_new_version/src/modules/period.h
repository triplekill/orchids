/**
 ** @file period.c
 ** Header for frequencies and phase analysis.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 ** 
 **
 ** @date  Started on: Fri Feb  7 11:07:42 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef PERIOD_H
#define PERIOD_H

#define timer_add(r, a, b) \
  do { \
    (r)->tv_usec = ((a)->tv_usec + (b)->tv_usec) % 1000000; \
    (r)->tv_sec  =  (a)->tv_sec +  (b)->tv_sec \
                   + (((a)->tv_usec + (b)->tv_usec) / 1000000); \
  } while (0)

#define timer_sub(r, a, b) \
  do { \
    if ((a)->tv_usec < (b)->tv_usec) { \
        (r)->tv_usec = 1000000 + (a)->tv_usec - (b)->tv_usec; \
        (r)->tv_sec = (a)->tv_sec - (b)->tv_sec - 1; \
      } else { \
        (r)->tv_usec = (a)->tv_usec - (b)->tv_usec; \
        (r)->tv_sec  = (a)->tv_sec  - (b)->tv_sec; \
      } \
  } while (0)

#define timer_float(t) ( (t)->tv_sec + ((t)->tv_usec / 1000000.0 ) )

#define DEFAULT_PERIOD_BACKLOG 100000

#if __WORDSIZE != 64
typedef unsigned long long uint64_t;
#endif

typedef unsigned long flags_t;

typedef struct periodspec_s periodspec_t;
struct periodspec_s
{
  char *name;
  struct timeval tv;
/*   unsigned long count; */
};

typedef struct periodrec_s periodrec_t;
struct periodrec_s
{
  struct timeval time;
  struct timeval lag;
  int period;
};

typedef struct periodctx_s periodctx_t;
struct periodctx_s
{
  periodspec_t *periodspec;
  int spec_size;
  struct timeval first;
  struct timeval last;
  double *count_mtime;
  double *count_timewin;
  unsigned long *count_total;
  unsigned long *count;
  char *name;
  unsigned long total;

  unsigned long *backlog;
  unsigned long *backlog_pos;
  unsigned long *backlog_end;
  double *tbacklog;
  double *tbacklog_pos;
  double *tbacklog_end;

  periodrec_t *timewin;
  periodrec_t *timewin_head;
  periodrec_t *timewin_tail;
  periodrec_t *timewin_end;
  struct timeval timewin_size;
  int evt_in_win;
  flags_t flags;
  double threshold;
};


/*
- period, subdivs

- 1s / 100  (10ms)
- 1m / 60   (1s)
- 1h / 360  (10s)
- 1d / 288  (5m)
- 1w / 336  (30m)
- 1M / 124  (6h)
- 1Y / 366  (1d)

P_SEC -1 (div_len in microseconds)
P_MIN -2
P_HOUR -3
P_DAY -4
P_WEEK -5
P_MONTH -6
P_YEAR -7
'raw user defined in sec'
*/

#define P_SEC   -1
#define P_MIN   -2
#define P_HOUR  -3
#define P_DAY   -4
#define P_WEEK  -5
#define P_MONTH -6
#define P_YEAR  -7

typedef struct phasespec_s phasespec_t;
struct phasespec_s {
  char *name;
  int period;
  struct timeval div_len;
  size_t divs;
  char *xtics_unit;
  char *xtics;
};

typedef struct phasectx_s phasectx_t;
struct phasectx_s {
  phasespec_t *phasespec;
  unsigned long **count;
  char *name;
  unsigned long total;
  struct timeval first;
  struct timeval last;
};


/* options for GnuPlot script generation */
#define O_OUTPNG      (1 << 0)
#define O_OUTEPS      (1 << 1)
#define O_OUTPDF      (1 << 2)
#define O_SHOWFIRST   (1 << 3)
#define O_SHOWLAST    (1 << 4)
#define O_SHOWENDS    ((O_SHOWFIRST)|(O_SHOWLAST))
#define O_VERBOSEPLOT (1 << 5)
#define O_GRAPHTYPE   (3 << 6)
#define O_RAWCOUNT    (0 << 6)
#define O_LOOPAVG     (1 << 6)
#define O_RAWPROB     (2 << 6)
#define O_PROB        (3 << 6)
#define O_SNAPSHOT    (1 << 8)

#define F_ALARM      (1 << 0)
#define ALARM_IS_SET(flag)  ((flag) & F_ALARM)
#define SET_ALARM(flag) do { (flag) |= F_ALARM; } while (0)
#define RESET_ALARM(flag) do { (flag) &= ~F_ALARM; } while (0)
#define O_COUNTALERT (1 << 1)


#endif /* PERIOD_H */

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
