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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "orchids.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>
#include <string.h>

#include "safelib.h"
#include "strhash.h"
#include "file_cache.h"
#include "html_output.h"

#include "mod_clocks.h"

#ifdef UNUSED
static void
clock_add(clockctx_t *ctx,
          char *name,
          timeval_t *prec,
          timeval_t *sync)
{
  timefloat_t fprec;
  timefloat_t fsync;

  fprec = TIMER_TO_FLOAT(prec);
  fsync = TIMER_TO_FLOAT(sync);

  clock_add_float(ctx, name, fprec, fsync);
}
#endif


static void
clock_add_float(clockctx_t *ctx,
                char *name,
                timefloat_t prec,
                timefloat_t sync)
{
  myclock_t *clock;

  clock = Xzmalloc(sizeof (myclock_t));
  clock->name = name;
  clock->prec = prec;
  clock->sync = sync;

  strhash_add(ctx->clocks, clock, name);
}


static prob_t
prob_is_before(clockctx_t *ctx, clocktime_t *t1, clocktime_t *t2)
{
  timefloat_t t1_min;
  timefloat_t t1_max;
  timefloat_t t2_min;
  timefloat_t t2_max;
  prob_t p;

  CLOCKTIME_MIN_MAX(t1_min, t1_max, t1);
  CLOCKTIME_MIN_MAX(t2_min, t2_max, t2);

  /* case 1 */
  if (t2_max < t1_min)
    return (0.0);

  /* case 2 */
  if (t1_max < t2_min)
    return (1.0);

  /* case 3 */
  if ((t2_min < t1_min) &&
      (t1_min <= t2_max) &&
      (t2_max < t1_max)) {
    p = (  (0.5 * t2_max * t2_max)
         + (0.5 * t1_min * t1_min)
         - (t2_max * t1_min) )
      / ( (t1_max - t1_min) * (t2_max - t2_min));

    return (p);
  }

  /* case 4 */
  if ((t1_min <= t2_min) &&
      (t2_min < t2_max) &&
      (t2_max <= t1_max)) {
    p = (t2_min - t1_min) / (t1_max - t1_min);
    p += (   (0.5 * t2_max * t2_max)
           + (0.5 * t2_min * t2_min)
           - (t2_max * t2_min) )
         / ( (t1_max - t1_min) * (t2_max - t2_min) );

    return (p);
  }

  /* case 5 */
  if ((t1_min < t2_min) &&
      (t2_min <= t1_max) &&
      (t1_max < t2_max)) {
    p = (t2_min - t1_min) / (t1_max - t1_min);
    p += (   (t2_max * t1_max)
           - (t2_max * t2_min)
           + (0.5 * t2_min * t2_min)
           - (0.5 * t1_max * t1_max) )
         / ( (t1_max - t1_min) * (t2_max - t2_min) );

    return (p);
  }

  /* case 6 */
  if ((t2_min <= t1_min) &&
      (t1_min < t1_max) &&
      (t1_max <= t2_max)) {
    p = (  (  t2_max * t1_max)
           - (t2_max * t1_min)
           + (0.5 * t1_min * t1_min)
           - (0.5 * t1_max * t1_max) )
      / ( (t1_max - t1_min) * (t2_max - t2_min) );

    return (p);
  }

  return (-1.0); /* Shouldn't not append */
}


#ifdef UNUSED
static prob_t
prob_is_after(clockctx_t *ctx, clocktime_t *t1, clocktime_t *t2)
{
  return ( prob_is_before(ctx, t2, t1) );
}
#endif


static prob_t
prob_is_before_fast(clockctx_t *ctx, clocktime_t *t1, clocktime_t *t2)
{
  timefloat_t t1_min;
  timefloat_t t1_max;
  timefloat_t t2_min;
  timefloat_t t2_max;
  prob_t p;

  CLOCKTIME_MIN_MAX(t1_min, t1_max, t1);
  CLOCKTIME_MIN_MAX(t2_min, t2_max, t2);

  /* case 1 */
  if (t2_max < t1_min)
    return (0.0);

  /* case 2 */
  if (t1_max < t2_min)
    return (1.0);

  p = (t2_max - t1_min) / (t1_max - t1_min + t2_max - t2_min);

  return (p);
}

#ifdef UNUSED
static prob_t
prob_is_after_fast(clockctx_t *ctx, clocktime_t *t1, clocktime_t *t2)
{
  return ( prob_is_before_fast(ctx, t2, t1) );
}
#endif


#ifdef UNUSED
static int
make_clocktime(clockctx_t *ctx,
               clocktime_t *clocktime,
               timeval_t *time,
               sequence_t seq,
               char *clock_name)
{
  myclock_t *clock;

  clock = strhash_get(ctx->clocks, clock_name);
  if (clock == NULL)
    return (-1);
  clocktime->clock = clock;
  clocktime->seq = seq;
  clocktime->time = TIMER_TO_FLOAT(time);

  return (0);
}
#endif

#ifdef UNUSED
static int
make_clocktime_float(clockctx_t *ctx,
                     clocktime_t *clocktime,
                     timefloat_t time,
                     sequence_t seq,
                     char *clock_name)
{
  myclock_t *clock;

  clock = strhash_get(ctx->clocks, clock_name);
  if (clock == NULL)
    return (-1);
  clocktime->clock = clock;
  clocktime->seq = seq;
  clocktime->time = time;

  return (0);
}
#endif

#ifdef UNUSED
static clockctx_t *
new_clocks_ctx(void)
{
  clockctx_t *ctx;

  ctx = Xmalloc(sizeof (clockctx_t));
  ctx->clocks = new_strhash(1021);

  return (ctx);
}
#endif

static void
add_clock(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  config_directive_t *clock_dir;
  clockctx_t *modcfg;
  char *clock_name;
  timefloat_t prec;
  timefloat_t sync;

  clock_name = strdup(dir->args);
  clock_name[ strlen(clock_name) - 1 ] = '\0';
  modcfg = mod->config;
  prec = 0.0;
  sync = 0.0;

  for (clock_dir = dir->first_child; clock_dir; clock_dir = clock_dir->next) {
    if (!strcmp(clock_dir->directive, "Precision")) {
      prec = atof(clock_dir->args);
      DebugLog(DF_MOD, DS_INFO,
               "Adding prec=%f for clock '%s'\n",prec,clock_name);
      if (prec <= 0.0) {
	DebugLog(DF_MOD, DS_ERROR,
		 "Precision for clock '%s' must be positive (prec=%f)\n",
		 clock_name, prec);
	return ;
      }
    }
   else if (!strcmp(clock_dir->directive, "Synchronization")) {
      sync = atof(clock_dir->args);
      if (sync <= 0.0) {
	DebugLog(DF_MOD, DS_ERROR,
		 "Synchronization for clock '%s' must be positive (sync=%f)\n",
		 clock_name, sync);
	return ;
      }
    }
    else {
      DebugLog(DF_MOD, DS_ERROR,
	       "Unknown sub-directive '%s' for clock '%s'\n",
	       clock_dir->next, clock_name);
    }
  }

  if (prec == 0.0) {
    DebugLog(DF_MOD, DS_ERROR, "Undefined precision for clock '%s'\n", clock_name);
    return ;
  }

  if (sync == 0.0) {
    DebugLog(DF_MOD, DS_ERROR, "Undefined synchronization for clock '%s'\n", clock_name);
    return ;
  }

  clock_add_float(modcfg, clock_name, prec, sync);
}


static int
qsort_clockcmp(const void *a, const void *b)
{
  return ( strcmp(((myclock_t*)a)->name, ((myclock_t *)b)->name) );
}


static int
clocks_htmloutput(orchids_t *ctx, mod_entry_t *mod, FILE *menufp, html_output_cfg_t *htmlcfg)
{
  FILE *fp;
/*   strhash_elmt_t *helmt; */
  int i;
/*  strhash_t *clocks; */
/*   int nb_clocks; */
  size_t ctx_array_sz;
  myclock_t **ctx_array;
  clockctx_t *modcfg;

  modcfg = mod->config;

  fp = create_html_file(htmlcfg, "orchids-clocks.html", NO_CACHE);
  fprintf_html_header(fp, "Orchids statistics");

  fprintf(fp, "<center><h1>Clocks<h1></center>\n");

  fprintf(fp, "<center>\n");
  fprintf(fp, "<table border=\"0\" cellpadding=\"3\" width=\"600\">\n");
  fprintf(fp, "<tr class=\"h\"><th colspan=\"3\">Clocks</th></tr>\n");
  fprintf(fp, "<tr class=\"hh\"><th>Clock name</th><th>Precision</th><th>Synchronization</th></tr>\n");

  ctx_array = strhash_to_array(modcfg->clocks);
  ctx_array_sz = modcfg->clocks->elmts;
  qsort(ctx_array, ctx_array_sz, sizeof (myclock_t *), qsort_clockcmp);

  for (i = 0; i < ctx_array_sz; i++) {
    fprintf(fp,
            "<tr><td class=\"e%i\">%s</td><td class=\"v%i\">%f</td><td class=\"v%i\">%f</td>\n",
            i%2, ctx_array[i]->name,
            i%2, ctx_array[i]->prec,
            i%2, ctx_array[i]->sync);
  }

  Xfree(ctx_array);

  fprintf(fp, "</table>\n");
  fprintf(fp, "</center>\n");

  fprintf_html_trailer(fp);

  Xfclose(fp);

  fprintf(menufp,
	  "<a href=\"orchids-clocks.html\" "
          "target=\"main\">Clocks</a><br/>\n");

  return (0);
}


#if 0
static void
issdl_prob_before(orchids_t *ctx, state_instance_t *state)
{
/*
  ovm_var_t *clock1_name;
  ovm_var_t *clock2_name;
  ovm_var_t *clock1_date;
  ovm_var_t *clock2_date;
  ovm_var_t *seq1_date;
  ovm_var_t *seq2_date;
*/

  /* type check here */
}
#endif


static void *
clocks_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  clockctx_t *modcfg;

  modcfg = Xzmalloc(sizeof (clockctx_t));
  modcfg->clocks = new_strhash(INITIAL_CLOCK_HASH_SIZE);

  /* register html output */
  html_output_add_menu_entry(ctx, mod, clocks_htmloutput);

  /* register language functions */

  return (modcfg);
}

static mod_cfg_cmd_t clocks_config_commands[] =
{
  { "<clock", add_clock, "Add a new clock definition" },
  { NULL, NULL, NULL }
};

input_module_t mod_clocks = {
  MOD_MAGIC,
  ORCHIDS_VERSION,
  "clocks",
  "CeCILL2",
  NULL,
  clocks_config_commands,
  clocks_preconfig,
  NULL,
  NULL
};







#if 0

int
main(int argc, char *argv[])
{
  clockctx_t *ctx;
  clocktime_t ct1, ct2;
  int i;
  prob_t p;

  if (argc != 2) {
    fprintf(stderr, "usage %s <configfile>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  ctx = new_clocks_ctx();
  parse_config(ctx, argv[1]);

/*   make_clocktime_float(ctx, &ct1, 10.123, "host1-kernel"); */
/*   make_clocktime_float(ctx, &ct2, 10.2, "host2-kernel"); */

  prob_is_before_fast(ctx, &ct1, &ct2);

  main_loop(ctx);

  return (0);
}

void
main_loop(clockctx_t *ctx)
{
  char buffer[4096];
  char clock1[256], clock2[256];
  sequence_t seq1, seq2;
  timefloat_t t1, t2;
  int test;
  int ret;
  clocktime_t ct1, ct2;
  prob_t p;

  while (fgets(buffer, sizeof (buffer), stdin)) {
    clock1[0] = '\0';
    clock2[0] = '\0';
    ret = fscanf(stdin, "(%256s %lu %f) %c (%256s %lu %f)",
           clock1, &seq1, &t1,
           &test,
           clock2, &seq2, &t2);
    if (ret == 7) {
      printf("clock1=%s seq1=%lu t1=%f\n%c\nclock2=%s seq2=%lu t2=%f\n",
             clock1, seq1, t1,
             test,
             clock2, seq2, t2);
      if (make_clocktime_float(ctx, &ct1, t1, seq1, clock1) == -1) {
        fprintf(stderr, "clocktime error\n");
        continue ;
      }
      if (make_clocktime_float(ctx, &ct2, t2, seq2, clock2) == -1) {
        fprintf(stderr, "clocktime error\n");
        continue ;
      }
      if ((char)test == '<')
        p = prob_is_before(ctx, &ct1, &ct2);
      else if ((char)test == '>')
        p = prob_is_after(ctx, &ct1, &ct2);
      else {
        fprintf(stderr, "< or > expected (%c)\n", test);
        continue ;
      }
      printf("p(t1 %c t2)=%10.8f\n", test, p);
    }
    else {
      fprintf(stderr, "syntax error\n");
    }
  }
}

void
parse_config(clockctx_t *ctx, char *file)
{
  char buffer[4096];
  char clock_name[256];
  timefloat_t prec;
  timefloat_t sync;
  FILE *fp;
  int ret;

  fp = Xfopen(file, "r");
  while (!feof(fp)) {
    ret = fscanf(fp, "%256s %f %f", &clock_name, &prec, &sync);
    if (ret == 3) {
      clock_add_float(ctx, strdup(clock_name), prec, sync);
    }
  }
}


#endif /* 0 */


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
