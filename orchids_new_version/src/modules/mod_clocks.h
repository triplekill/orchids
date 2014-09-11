/**
 ** @file mod_clocks.c
 ** A module for handling clock uncertainty.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1.0
 ** @ingroup modules
 **
 ** @date  Started on: Mon Dec 01 00:57:32 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef MOD_CLOCKS
#define MOD_CLOCKS

#define INITIAL_CLOCK_HASH_SIZE 1024

#ifdef USE_DOUBLE
typedef double timefloat_t;
typedef double prob_t;
#else
typedef float timefloat_t;
typedef float prob_t;
#endif

typedef unsigned long sequence_t;

typedef struct myclock_s myclock_t;
struct myclock_s {
  char *name;
  timefloat_t prec;
  timefloat_t sync;
};

typedef struct clocktime_s clocktime_t;
struct clocktime_s {
  myclock_t *clock;
  sequence_t seq;
  timefloat_t time;
};

typedef struct clockctx_s clockctx_t;
struct clockctx_s {
  strhash_t *clocks;
};

#define TIMER_ADD(r, a, b) \
  do { \
    (r)->tv_usec = ((a)->tv_usec + (b)->tv_usec) % 1000000; \
    (r)->tv_sec  =  (a)->tv_sec +  (b)->tv_sec \
                   + (((a)->tv_usec + (b)->tv_usec) / 1000000); \
  } while (0)

#define TIMER_SUB(r, a, b) \
  do { \
    if ((a)->tv_usec < (b)->tv_usec) { \
        (r)->tv_usec = 1000000 + (a)->tv_usec - (b)->tv_usec; \
        (r)->tv_sec = (a)->tv_sec - (b)->tv_sec - 1; \
      } else { \
        (r)->tv_usec = (a)->tv_usec - (b)->tv_usec; \
        (r)->tv_sec  = (a)->tv_sec  - (b)->tv_sec; \
      } \
  } while (0)

#define TIMER_TO_FLOAT(t) ( (t)->tv_sec + ((t)->tv_usec / 1000000.0 ) )

#ifdef USE_DOUBLE
#define FLOAT_TO_TIMER(t, f) (t)->tv_usec = 1000000.0 * modf((f), &(t)->tv_sec)
#else
#define FLOAT_TO_TIMER(t, f) \
do { \
  timefloat_t i; \
  (t)->tv_usec = 1000000.0 * modff((f), &i); \
  (t)->tv_sec = i; \
  } while (0)
#endif

#define MIN(a, b) ((a) < (b)) ? (a) : (b)
#define MAX(a, b) ((a) > (b)) ? (a) : (b)

#define CLOCKTIME_MIN_MAX(min, max, ct)         \
                                                \
  do {                                          \
    timefloat_t t, t_prec, sync;                \
                                                \
    t = (ct)->time;                             \
    t_prec = t + (ct)->clock->prec;             \
    sync = (ct)->clock->sync;                   \
    (min) = MIN(t, t_prec) - sync;              \
    (max) = MAX(t, t_prec) + sync;              \
                                                \
  } while (0)

#ifdef UNUSED
static void
clock_add(clockctx_t *ctx,
          char *name,
          timeval_t *prec,
          timeval_t *sync);
#endif

static void
clock_add_float(clockctx_t *ctx,
                char *name,
                timefloat_t prec,
                timefloat_t sync);


#ifdef UNUSED
static prob_t
prob_is_before(clockctx_t *ctx, clocktime_t *t1, clocktime_t *t2);
#endif

#ifdef UNUSED
static prob_t
prob_is_after(clockctx_t *ctx, clocktime_t *t1, clocktime_t *t2);
#endif

#ifdef UNUSED
static prob_t
prob_is_before_fast(clockctx_t *ctx, clocktime_t *t1, clocktime_t *t2);
#endif

#ifdef UNUSED
static int
make_clocktime(clockctx_t *ctx,
               clocktime_t *clocktime,
               timeval_t *time,
               sequence_t seq,
               char *clock_name);
#endif

#ifdef UNUSED
static int
make_clocktime_float(clockctx_t *ctx,
                     clocktime_t *clocktime,
                     timefloat_t time,
                     sequence_t seq,
                     char *clock_name);
#endif

#ifdef UNUSED
static clockctx_t *
new_clocks_ctx(void);
#endif

static void
add_clock(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir);


static int
qsort_clockcmp(const void *a, const void *b);

static int
clocks_htmloutput(orchids_t *ctx, mod_entry_t *mod, FILE *menufp, html_output_cfg_t *htmlcfg);




#endif /* MOD_CLOCKS */

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
