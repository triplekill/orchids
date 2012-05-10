/**
 ** @file period.c
 ** Functions for frequencies and phase analysis.
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <math.h>

#include "orchids_types.h"
#include "period.h"

/*
  period-<context>.plot
  period-<context>.dat

  phase-<context>.plot
  phase-<context>-<name>.plot
  ...

*/

/* static int askdisplay_g = 0; */




static periodspec_t periodspec_g[] = {
  { "0s",    {                    0,          0 } },
  { "100us", {                    0,        100 } },
  { "1ms",   {                    0,       1000 } },
  { "10ms",  {                    0,  10 * 1000 } },
  { "100ms", {                    0, 100 * 1000 } },
  { "250ms", {                    0, 250 * 1000 } },
  { "500ms", {                    0, 500 * 1000 } },
  { "1s",    {                    1,          0 } },
  { "2s",    {                    2,          0 } },
  { "5s",    {                    5,          0 } },
  { "10s",   {                   10,          0 } },
  { "30s",   {                   30,          0 } },
  { "1m",    {                   60,          0 } },
  { "2m",    {               2 * 60,          0 } },
  { "5m",    {               5 * 60,          0 } },
  { "10m",   {              10 * 60,          0 } },
  { "15m",   {              15 * 60,          0 } },
  { "30m",   {              30 * 60,          0 } },
  { "45m",   {              45 * 60,          0 } },
  { "1h",    {              60 * 60,          0 } },
  { "2h",    {          2 * 60 * 60,          0 } },
  { "4h",    {          4 * 60 * 60,          0 } },
  { "8h",    {          8 * 60 * 60,          0 } },
  { "12h",   {         12 * 60 * 60,          0 } },
  { "1d",    {         24 * 60 * 60,          0 } },
  { "2d",    {     2 * 24 * 60 * 60,          0 } },
  { "3d",    {     3 * 24 * 60 * 60,          0 } },
  { "1w",    {     7 * 24 * 60 * 60,          0 } },
  { "2w",    { 2 * 7 * 24 * 60 * 60,          0 } },
  { "3w",    { 3 * 7 * 24 * 60 * 60,          0 } },
  /* Average month length: (365.25 / 12) * 24 * 60 * 60 = 2629800.00 */
  { "1M",    {              2629800,          0 } },
  { "2M",    {          2 * 2629800,          0 } },
  { "3M",    {          3 * 2629800,          0 } },
  { "6M",    {          6 * 2629800,          0 } },
  { "9M",    {          9 * 2629800,          0 } },
  { "1Y",    {         12 * 2629800,          0 } },
  { "2Y",    {     2 * 12 * 2629800,          0 } },
  { "3Y",    {     3 * 12 * 2629800,          0 } },
  { NULL,    {                    0,          0 } }
};

static phasespec_t phasespec_g[] = {
  { "1s-10ms",  P_SEC,   {            0,  10 * 1000 }, 100, "ms", 
    "( \"100\" 10, \"200\" 20, \"300\" 30, \"400\" 40, \"500\" 50, \"600\" 60, \"700\" 70, \"800\" 80, \"900\" 90 )" },
  { "1m-250ms", P_MIN,   {            0, 250 * 1000 }, 240, "sec", 
    "( \"0\" 0, \"5\" 20, \"10\" 40, \"15\" 60, \"20\" 80, \"25\" 100, \"30\" 120, \"35\" 140, \"40\" 160, \"45\" 180, \"50\" 200, \"55\" 220, \"60\" 240 )" },
  { "1h-10s",   P_HOUR,  {           10,          0 }, 360, "min",
    "( \"0\" 0, \"5\" 30, \"10\" 60, \"15\" 90, \"20\" 120, \"25\" 150, \"30\" 180, \"35\" 210, \"40\" 240, \"45\" 270, \"50\" 300, \"55\" 330, \"60\" 360 )" },
  { "1d-5m",    P_DAY,   {       5 * 60,          0 }, 288, "hour", 
    "( \"0\" 0, \"1\" 12, \"2\" 24, \"3\" 36, \"4\" 48, \"5\" 60, \"6\" 72, \"7\" 84, \"8\" 96, \"9\" 108, \"10\" 120, \"11\" 132, \"12\" 144, \"13\" 156, \"14\" 168, \"15\" 180, \"16\" 192, \"17\" 204, \"18\" 216, \"19\" 228, \"20\" 240, \"21\" 252, \"22\" 264, \"23\" 276, \"24\" 288 )" },
  { "1w-30m",   P_WEEK,  {      30 * 60,          0 }, 336, "day", 
    "( \"Mon\" 0, \"\" 24, \"Tue\" 48, \"\" 72, \"Wed\" 96, \"\" 120, \"Thu\" 144, \"\" 168, \"Fri\" 192, \"\" 216, \"Sat\" 240, \"\" 264, \"Sun\" 288, \"\" 312, \"\" 336 )" },
  { "1M-6h",    P_MONTH, {  3 * 60 * 60,          0 }, 248, "day",
    "( \"1\" 0, \"2\" 8, \"3\" 16, \"4\" 24, \"5\" 32, \"6\" 40, \"7\" 48, \"8\" 56, \"9\" 64, \"10\" 72, \"11\" 80, \"12\" 88, \"13\" 96, \"14\" 104, \"15\" 112, \"16\" 120, \"17\" 128, \"18\" 136, \"19\" 144, \"20\" 152, \"21\" 160, \"22\" 168, \"23\" 176, \"24\" 184, \"25\" 192, \"26\" 200, \"27\" 208, \"28\" 216, \"29\" 224, \"30\" 232, \"31\" 240, \"\"  248 )" },
  { "1Y-1d",    P_YEAR,  { 24 * 60 * 60,          0 }, 366, "month",
    "( \"Jan\" 0, \"Feb\" 31, \"Mar\" 59, \"Apr\" 90, \"May\" 120, \"Jun\" 151, \"Jul\" 181, \"Aug\" 212, \"Sep\" 243, \"Oct\" 273, \"Nov\" 304, \"Dec\" 334, \"\" 365 )" },

  /* some user defined crazy demos */
#define MARS_REVOLUTION 59355072 /* 686.98d * 24h * 60m * 60s */
  { "mars-2d", MARS_REVOLUTION, { 48 * 60 * 60, 0 }, 344, "day", 
    "( \"0\" 0, \"1\" 15, \"2\" 30, \"3\" 45, \"4\" 60, \"5\" 75, \"6\" 90, \"7\" 105, \"8\" 120, \"9\" 135, \"10\" 150, \"11\" 165, \"12\" 180, \"13\" 195, \"14\" 210, \"15\" 225, \"16\" 240, \"17\" 255, \"18\" 270, \"19\" 285, \"20\" 300, \"21\" 315, \"22\" 330 )" },
#define MOON_REVOLUTION (27 * 24 * 60 * 60 + 7 * 60 * 60 + 43 * 60)
  { "moon-2h", MOON_REVOLUTION, { 2 * 60 * 60, 0 }, 328, "day", 
    "( \"0\" 0, \"1\" 12, \"2\" 24, \"3\" 36, \"4\" 48, \"5\" 60, \"6\" 72, \"7\" 84, \"8\" 96, \"9\" 108, \"10\" 120, \"11\" 132, \"12\" 144, \"13\" 156, \"14\" 168, \"15\" 180, \"16\" 192, \"17\" 204, \"18\" 216,  \"19\" 228, \"20\" 240, \"21\" 252, \"22\" 264, \"23\" 276, \"24\" 288, \"25\" 300, \"26\" 312, \"27\" 324 )" }, 
  { NULL, 0, { 0, 0}, 0 }
};



static void fprintf_phase_plot(FILE *fp, phasectx_t *ctx, flags_t flags);
#ifdef UNUSED
static void fprintf_period_plot(FILE *fp, periodctx_t *ctx, flags_t flags);
#endif
#ifdef UNUSED
static void fprintf_period_data(FILE *fp, periodctx_t *ctx);
#endif
#ifdef UNUSED
static void fprintf_period_data_nowin(FILE *fp, periodctx_t *ctx);
#endif
static double get_phase_loops(phasespec_t *spec, struct timeval *first, struct timeval *last);
static int get_phase_slice(phasespec_t *spec, struct timeval *tv);
#ifdef UNUSED
static void fprintf_period_data_timewin(FILE *fp, periodctx_t *ctx);
#endif

double kullback_leibler(periodctx_t *ctx);
double kullback_leibler_time(periodctx_t *ctx);

static void
period_snprintf_uptime(char *str, size_t size, time_t uptime);





phasespec_t *
get_default_phasespec(void)
{
  return (phasespec_g);
}

periodspec_t *
get_default_periodspec(void)
{
  return (periodspec_g);
}

static int
tvdiv(const struct timeval *t1, const struct timeval *t2)
{
  uint64_t a, b;

  a = ((uint64_t) t1->tv_sec) * 1000000LL + t1->tv_usec;
  b = ((uint64_t) t2->tv_sec) * 1000000LL + t2->tv_usec;

  return (a / b);
}

static void
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

#ifdef UNUSED
static void
write_period_snapshot(periodctx_t *ctx)
{
  char filename[1024];
  FILE *fp;

  snprintf(filename, sizeof (filename), "%s-period-snapshot.plot", ctx->name);
  fp = fopen(filename, "a");
  fprintf_period_plot(fp, ctx, O_OUTPNG|O_VERBOSEPLOT|O_SNAPSHOT);
  fclose(fp);

  snprintf(filename, sizeof (filename), "%s-period-snapshot-%lu-%06lu.dat",
           ctx->name, ctx->last.tv_sec, (unsigned long)ctx->last.tv_usec);
  fp = fopen(filename, "w");
  fprintf_period_data_timewin(fp, ctx);
  fclose(fp);
}
#endif

#ifdef UNUSED
static void
write_period_file(periodctx_t *ctx)
{
  char filename[1024];
  FILE *fp;

  snprintf(filename, sizeof (filename), "%s-period.plot", ctx->name);
  fp = fopen(filename, "w");
  fprintf_period_plot(fp, ctx, O_OUTPNG|O_VERBOSEPLOT);
  fclose(fp);

  snprintf(filename, sizeof (filename), "%s-period.dat", ctx->name);
  fp = fopen(filename, "w");
  fprintf_period_data(fp, ctx);
  fclose(fp);
}
#endif

#ifdef UNUSED
static void
fprintf_period_plot(FILE *fp, periodctx_t *ctx, flags_t flags)
{
  int i;
  periodspec_t *p;
  char asc_time[32];
  time_t curtime;
  char basename[1024];

  if ( ! (flags & (O_OUTPNG|O_OUTEPS|O_OUTPDF)) )
    flags |= O_OUTPNG; /* default output */

  if (flags & O_SNAPSHOT) {
    snprintf(basename, sizeof (basename), "%s-period-snapshot-%lu-%06lu",
             ctx->name, ctx->last.tv_sec, (unsigned long)ctx->last.tv_usec);
  }
  else {
    snprintf(basename, sizeof (basename), "%s-period", ctx->name);
  }

  fprintf(fp, "#! /usr/bin/gnuplot -persist\n\n");
  curtime = time(NULL);
  strftime(asc_time, sizeof (asc_time), "%a %b %d %H:%M:%S %Z %Y",
           localtime(&curtime));
  fprintf(fp, "#\n");
  fprintf(fp, "# Generated by Period on %s.\n\n", asc_time);

  fprintf(fp, "set grid\n");
  fprintf(fp, "set style fill solid 0.25 border\n");
  fprintf(fp, "set xtics (");
  for (i = 0, p = ctx->periodspec; p->name; i++, p++) {
    fprintf(fp, "\"%s\" %i.5%s ", p[0].name, i, p[1].name ? ",":"");
  }
  fprintf(fp, ")\n");

  fprintf(fp, "set xlabel \"Time\"\n");
  fprintf(fp, "set ylabel \"Count\"\n");
  fprintf(fp, "set title "
              "\"Discrete event \\\"%s\\\" periodogram (%lu events)\"\n",
          ctx->name, ctx->total);

  if (flags & O_VERBOSEPLOT)
    fprintf(fp, "print \"Rendering '%s-period-count'\"\n", ctx->name);

  if (flags & O_OUTPNG) {
    fprintf(fp, "set term png font Vera 7\n");
    fprintf(fp, "set output \"%s-count.png\"\n", basename);
    fprintf(fp, "plot [] [0:] \"%s.dat\" using 1:2:(1.0) "
                "notitle with boxes, \\\n", basename);
    fprintf(fp, "             \"%s.dat\" using 1:2  lt 1 "
                "notitle with line smooth bezier\n\n\n", basename);
  }

  if (flags & O_OUTEPS) {
    fprintf(fp, "set term postscript eps enhanced 10\n");
    fprintf(fp, "set output \"%s-count.eps\"\n", basename);
    fprintf(fp, "plot [] [0:] \"%s.dat\" using 1:2:(1.0) "
                "notitle with boxes, \\\n", basename);
    fprintf(fp, "             \"%s.dat\" using 1:2  lt 1 "
                "notitle with line smooth bezier\n\n\n", basename);
  }

  if (flags & O_OUTPDF) {
    fprintf(fp, "set term pdf enhanced fname \"Helvetica\" fsize 6\n");
    fprintf(fp, "set output \"%s-count.pdf\"\n", basename);
    fprintf(fp, "plot [] [0:] \"%s.dat\" using 1:2:(1.0) "
                "notitle with boxes, \\\n", basename);
    fprintf(fp, "             \"%s.dat\" using 1:2  lt 1 "
                "notitle with line smooth bezier\n\n\n", basename);
  }

  fprintf(fp, "set xlabel \"Time\"\n");
  fprintf(fp, "set ylabel \"Time (s)\"\n");
  fprintf(fp, "set title \"Discrete event \\\"%s\\\" time spent by time interval (%lu events)\"\n", ctx->name, ctx->total);

  if (flags & O_VERBOSEPLOT)
    fprintf(fp, "print \"Rendering '%s-period-time'\"\n", ctx->name);

  if (flags & O_OUTPNG) {
    fprintf(fp, "set term png font Vera 7\n");
    fprintf(fp, "set output \"%s-time.png\"\n", basename);
    fprintf(fp, "plot [] [0:] \"%s.dat\" using 1:5:(0.5)   "
                "title \"max\" with boxes, \\\n", basename);
    fprintf(fp, "             \"%s.dat\" using 1:4:(1.0)   "
                "title \"avg\" with boxes, \\\n", basename);
    fprintf(fp, "             \"%s.dat\" using 1:3:(0.5)   "
                "title \"min\" with boxes, \\\n", basename);
    fprintf(fp, "             \"%s.dat\" using 1:7:(0.2)   "
                "title \"real\" with boxes, \\\n", basename);
    fprintf(fp, "             \"%s.dat\" using 1:5 lt 1 "
                "notitle with line smooth bezier, \\\n", basename);
    fprintf(fp, "             \"%s.dat\" using 1:4 lt 2 "
                "notitle with line smooth bezier, \\\n", basename);
    fprintf(fp, "             \"%s.dat\" using 1:3 lt 3 "
                "notitle with line smooth bezier, \\\n", basename);
    fprintf(fp, "             \"%s.dat\" using 1:7 lt 4 "
                "notitle with line smooth bezier\n", basename);
  }

  if (flags & O_OUTEPS) {
    fprintf(fp, "set term postscript eps enhanced 10\n");
    fprintf(fp, "set output \"%s-time.eps\"\n", basename);
    fprintf(fp, "plot [] [0:] \"%s.dat\" using 1:5:(0.5)   "
                "title \"max\" with boxes, \\\n", basename);
    fprintf(fp, "             \"%s.dat\" using 1:4:(1.0)   "
                "title \"avg\" with boxes, \\\n", basename);
    fprintf(fp, "             \"%s.dat\" using 1:3:(0.5)   "
                "title \"min\" with boxes, \\\n", basename);
    fprintf(fp, "             \"%s.dat\" using 1:7:(0.2)   "
                "title \"real\" with boxes, \\\n", basename);
    fprintf(fp, "             \"%s.dat\" using 1:5 lt 1 "
                "notitle with line smooth bezier, \\\n", basename);
    fprintf(fp, "             \"%s.dat\" using 1:4 lt 2 "
                "notitle with line smooth bezier, \\\n", basename);
    fprintf(fp, "             \"%s.dat\" using 1:3 lt 3 "
                "notitle with line smooth bezier, \\\n", basename);
    fprintf(fp, "             \"%s.dat\" using 1:7 lt 4 "
                "notitle with line smooth bezier\n", basename);
  }

  if (flags & O_OUTPDF) {
    fprintf(fp, "set term pdf enhanced fname \"Helvetica\" fsize 6\n");
    fprintf(fp, "set output \"%s-time.pdf\"\n", basename);
    fprintf(fp, "plot [] [0:] \"%s.dat\" using 1:5:(0.5)   "
                "title \"max\" with boxes, \\\n", basename);
    fprintf(fp, "             \"%s.dat\" using 1:4:(1.0)   "
                "title \"avg\" with boxes, \\\n", basename);
    fprintf(fp, "             \"%s.dat\" using 1:3:(0.5)   "
                "title \"min\" with boxes, \\\n", basename);
    fprintf(fp, "             \"%s.dat\" using 1:7:(0.2)   "
                "title \"real\" with boxes, \\\n", basename);
    fprintf(fp, "             \"%s.dat\" using 1:5 lt 1 "
                "notitle with line smooth bezier, \\\n", basename);
    fprintf(fp, "             \"%s.dat\" using 1:4 lt 2 "
                "notitle with line smooth bezier, \\\n", basename);
    fprintf(fp, "             \"%s.dat\" using 1:3 lt 3 "
                "notitle with line smooth bezier, \\\n", basename);
    fprintf(fp, "             \"%s.dat\" using 1:7 lt 4 "
                "notitle with line smooth bezier\n", basename);
  }

  fprintf(fp, "set xlabel \"Time gap\"\n");
  fprintf(fp, "set ylabel \"Prob\"\n");
  fprintf(fp, "set title "
              "\"Discrete event \\\"%s\\\" time gap distribution (%lu events)\"\n",
          ctx->name, ctx->total);

  if (flags & O_VERBOSEPLOT)
    fprintf(fp, "print \"Rendering '%s-period-gapdist'\"\n", ctx->name);

  if (flags & O_OUTPNG) {
    fprintf(fp, "set term png font Vera 7\n");
    fprintf(fp, "set output \"%s-gapdist.png\"\n", basename);
    fprintf(fp, "plot [] [0:] \"%s.dat\" using 1:8:(1.0) "
                "notitle with boxes, \\\n", basename);
    fprintf(fp, "             \"%s.dat\" using 1:8  lt 1 "
                "notitle with line smooth bezier\n\n\n", basename);
  }

  if (flags & O_OUTEPS) {
    fprintf(fp, "set term postscript eps enhanced 10\n");
    fprintf(fp, "set output \"%s-gapdist.eps\"\n", basename);
    fprintf(fp, "plot [] [0:] \"%s.dat\" using 1:8:(1.0) "
                "notitle with boxes, \\\n", basename);
    fprintf(fp, "             \"%s.dat\" using 1:8  lt 1 "
                "notitle with line smooth bezier\n\n\n", basename);
  }

  if (flags & O_OUTPDF) {
    fprintf(fp, "set term pdf enhanced fname \"Helvetica\" fsize 6\n");
    fprintf(fp, "set output \"%s-gapdist.pdf\"\n", basename);
    fprintf(fp, "plot [] [0:] \"%s.dat\" using 1:8:(1.0) "
                "notitle with boxes, \\\n", basename);
    fprintf(fp, "             \"%s.dat\" using 1:8  lt 1 "
                "notitle with line smooth bezier\n\n\n", basename);
  }
}
#endif

#ifdef UNUSED
static void
fprintf_period_data_nowin(FILE *fp, periodctx_t *ctx)
{
  int i;
  periodspec_t *p;
  double norm1, norm2, expect;
  char asc_time[128];
  time_t curtime;

  fprintf(fp, "# data\n");
  fprintf(fp, "# fields: timeslice count minspenttime maxspenttime countprob meantime timeprob\n");
  fprintf(fp, "#\n");
  fprintf(fp, "#          Infos:\n");
  fprintf(fp, "# ------------------------------------------\n");
  fprintf(fp, "#         events: %lu\n", ctx->total);
  fprintf(fp, "#    time_length: %lu\n", ctx->last.tv_sec - ctx->first.tv_sec);
  period_snprintf_uptime(asc_time, sizeof (asc_time),
                  ctx->last.tv_sec - ctx->first.tv_sec);
  fprintf(fp, "#     length_str: %s\n", asc_time);
  strftime(asc_time, sizeof (asc_time), "%a %b %d %H:%M:%S %Z %Y",
           localtime(&ctx->first.tv_sec));
  fprintf(fp, "#     first_date: %s\n", asc_time);
  strftime(asc_time, sizeof (asc_time), "%a %b %d %H:%M:%S %Z %Y",
           localtime(&ctx->last.tv_sec));
  fprintf(fp, "#      last_date: %s\n", asc_time);
  curtime = time(NULL);
  strftime(asc_time, sizeof (asc_time), "%a %b %d %H:%M:%S %Z %Y",
           localtime(&curtime));
  fprintf(fp, "#\n");
  fprintf(fp, "# Generated by Period on %s.\n\n", asc_time);

  for (i = 1, p = &ctx->periodspec[1]; p->name; p++, i++) {
    if (ctx->count_total[i] > 0) { 
      norm1 = p[-1].tv.tv_sec + (p[-1].tv.tv_usec / 1000000.0);
      norm2 = p[0].tv.tv_sec + (p[0].tv.tv_usec / 1000000.0);
      expect = ctx->count_total[i] / ((double) ctx->total);
      fprintf(fp, "%i\t%10lu\t%14.4f\t%14.4f\t%14.4f\t%10.8f\t%14.4f\t%10.8f\n",
              i,
              ctx->count_total[i],
              ctx->count_total[i] * norm1,
              ((ctx->count_total[i] * norm1) + (ctx->count_total[i] * norm2)) / 2.0,
              ctx->count_total[i] * norm2,
              expect,
              ctx->count_mtime[i],
              ctx->count_mtime[i] / (double)(ctx->last.tv_sec - ctx->first.tv_sec));
    }
  }
}
#endif


#ifdef UNUSED
static void
fprintf_period_data_timewin(FILE *fp, periodctx_t *ctx)
{
  int i;
  periodspec_t *p;
  double norm1, norm2, expect;
  char asc_time[128];
  time_t curtime;
  double mtime[128] = { 0.0, };
  unsigned long count[128] = { 0, };
  periodrec_t *rec;

  fprintf(fp, "# data\n");
  fprintf(fp, "# fields: timeslice count minspenttime maxspenttime countprob meantime timeprob\n");
  fprintf(fp, "#\n");
  fprintf(fp, "#          Infos:\n");
  fprintf(fp, "# ------------------------------------------\n");
  fprintf(fp, "#         events: %lu\n", ctx->total);
  fprintf(fp, "#    time_length: %lu\n", ctx->last.tv_sec - ctx->first.tv_sec);
  period_snprintf_uptime(asc_time, sizeof (asc_time),
                  ctx->last.tv_sec - ctx->first.tv_sec);
  fprintf(fp, "#     length_str: %s\n", asc_time);
  strftime(asc_time, sizeof (asc_time), "%a %b %d %H:%M:%S %Z %Y",
           localtime(&ctx->first.tv_sec));
  fprintf(fp, "#     first_date: %s\n", asc_time);
  strftime(asc_time, sizeof (asc_time), "%a %b %d %H:%M:%S %Z %Y",
           localtime(&ctx->last.tv_sec));
  fprintf(fp, "#      last_date: %s\n", asc_time);
  curtime = time(NULL);
  strftime(asc_time, sizeof (asc_time), "%a %b %d %H:%M:%S %Z %Y",
           localtime(&curtime));
  fprintf(fp, "#\n");
  fprintf(fp, "# Generated by Period on %s.\n\n", asc_time);

  for (rec = ctx->timewin_head; rec != ctx->timewin_tail; ) {
    count[ rec->period ]++;
    mtime[ rec->period ] += timer_float( &rec->lag );
    rec++;
    if (rec == ctx->timewin_end)
      rec = ctx->timewin;
  }

  for (i = 1, p = &ctx->periodspec[1]; p->name; p++, i++) {
    if (ctx->count_total[i] > 0) { 
      norm1 = p[-1].tv.tv_sec + (p[-1].tv.tv_usec / 1000000.0);
      norm2 = p[0].tv.tv_sec + (p[0].tv.tv_usec / 1000000.0);
      expect = count[i] / ((double) ctx->total);
      fprintf(fp, "%i\t%10lu\t%14.4f\t%14.4f\t%14.4f\t%10.8f\t%14.4f\t%10.8f\n",
              i,
              count[i],
              count[i] * norm1,
              ((count[i] * norm1) + (count[i] * norm2)) / 2.0,
              count[i] * norm2,
              expect,
              mtime[i],
              mtime[i] / (double)(ctx->timewin_tail->time.tv_sec - ctx->timewin_head->time.tv_sec));
    }
  }
}
#endif


#ifdef UNUSED
static void
fprintf_period_data(FILE *fp, periodctx_t *ctx)
{
  int i;
  periodspec_t *p;
  double norm1, norm2, expect;
  char asc_time[128];
  time_t curtime;

  fprintf(fp, "# data\n");
  fprintf(fp, "# fields: timeslice count minspenttime maxspenttime countprob meantime timeprob\n");
  fprintf(fp, "#\n");
  fprintf(fp, "#          Infos:\n");
  fprintf(fp, "# ------------------------------------------\n");
  fprintf(fp, "#         events: %lu\n", ctx->total);
  fprintf(fp, "#    time_length: %lu\n", ctx->last.tv_sec - ctx->first.tv_sec);
  period_snprintf_uptime(asc_time, sizeof (asc_time),
                  ctx->last.tv_sec - ctx->first.tv_sec);
  fprintf(fp, "#     length_str: %s\n", asc_time);
  strftime(asc_time, sizeof (asc_time), "%a %b %d %H:%M:%S %Z %Y",
           localtime(&ctx->first.tv_sec));
  fprintf(fp, "#     first_date: %s\n", asc_time);
  strftime(asc_time, sizeof (asc_time), "%a %b %d %H:%M:%S %Z %Y",
           localtime(&ctx->last.tv_sec));
  fprintf(fp, "#      last_date: %s\n", asc_time);
  curtime = time(NULL);
  strftime(asc_time, sizeof (asc_time), "%a %b %d %H:%M:%S %Z %Y",
           localtime(&curtime));
  fprintf(fp, "#\n");
  fprintf(fp, "# Generated by Period on %s.\n\n", asc_time);

  for (i = 1, p = &ctx->periodspec[1]; p->name; p++, i++) {
    if (ctx->count[i] > 0) { 
      norm1 = p[-1].tv.tv_sec + (p[-1].tv.tv_usec / 1000000.0);
      norm2 = p[0].tv.tv_sec + (p[0].tv.tv_usec / 1000000.0);
      expect = ctx->count[i] / ((double) DEFAULT_PERIOD_BACKLOG);
      fprintf(fp, "%i\t%10lu\t%14.4f\t%14.4f\t%14.4f\t%10.8f\t%14.4f\t%10.8f\n",
              i,
              ctx->count[i],
              ctx->count[i] * norm1,
              ((ctx->count[i] * norm1) + (ctx->count[i] * norm2)) / 2.0,
              ctx->count[i] * norm2,
              expect,
              ctx->count_timewin[i],
              ctx->count_timewin[i] / (double)(ctx->last.tv_sec - ctx->first.tv_sec));
    }
  }
}
#endif


static void
fprintf_phase_plot(FILE *fp, phasectx_t *ctx, flags_t flags)
{
  int i;
  phasespec_t *p;
  char basename[1024];
  int div;
  char asc_time[32];
  double loops;
  time_t curtime;
  int graphtype;

  if ( ! (flags & (O_OUTPNG|O_OUTEPS|O_OUTPDF)) )
    flags |= O_OUTPNG; /* default output */

  fprintf(fp, "#! /usr/bin/gnuplot -persist\n");
  curtime = time(NULL);
  strftime(asc_time, sizeof (asc_time), "%a %b %d %H:%M:%S %Z %Y",
           localtime(&curtime));
  fprintf(fp, "#\n");
  fprintf(fp, "# Generated by Period on %s\n\n", asc_time);

  for (i = 0, p = ctx->phasespec; p->name ; p++, i++) {

    snprintf(basename, sizeof (basename), "%s-phase-%i-%s",
             ctx->name, i, p->name);

    fprintf(fp, "#### Gnuplot Script for '%s' ####\n\n\n", basename);

    if (flags & O_VERBOSEPLOT)
      fprintf(fp, "print \"Rendering '%s'\"\n", basename);

    fprintf(fp, "set grid\n");
/*     fprintf(fp, "set style fill solid 0.25 border\n"); */
    fprintf(fp, "set rmargin 5\n");
    fprintf(fp, "set xlabel \"Phase slice (%s)\"\n", p->xtics_unit);

    graphtype = (flags & O_GRAPHTYPE) >> 6;
    switch (graphtype) {
    case 0:
      fprintf(fp, "set ylabel \"Count\"\n\n");
      break;

    case 1:
      fprintf(fp, "set ylabel \"Average count per loop\"\n\n");
      break;

    case 2:
      fprintf(fp, "set ylabel \"Raw probability\"\n\n");
      break;

    case 3:
      fprintf(fp, "set ylabel \"Corrected probability\"\n\n");
      break;

    default:
      printf("graph type error.\n");
      break;
    }

    if (flags & O_SHOWFIRST) {
      strftime(asc_time, sizeof (asc_time), "%d/%m/%Y\\n%H:%M:%S",
               localtime(&ctx->first.tv_sec));
      div = get_phase_slice(p, &ctx->first);
      fprintf(fp, "set label 1 \"First:\\n%s\" "
                  "at first %i,graph 0.88 center front\n",
              asc_time, div);
      fprintf(fp, "set arrow 1 from first %i, graph 0.79 "
                  "to first %i,graph 0 size graph 0.02,30 "
                  "filled lt 3 lw 2 front\n",
              div, div);
    }

    if (flags & O_SHOWLAST) {
      strftime(asc_time, sizeof (asc_time), "%d/%m/%Y\\n%H:%M:%S",
               localtime(&ctx->last.tv_sec));
      div = get_phase_slice(p, &ctx->last);
      fprintf(fp, "set label 2 \"Last:\\n%s\" "
                  "at first %i,graph 0.97 center front\n",
              asc_time, div);
      fprintf(fp, "set arrow 2 from first %i, graph 0.88 "
                  "to first %i,graph 0 size graph 0.02,30 "
                  "filled lt 3 lw 2 front\n\n\n",
              div, div);
    }

    loops = get_phase_loops(p, &ctx->first, &ctx->last);
    fprintf(fp, "set title \"Phase distribution \\\"%s\\\" for event \\\"%s\\\"\\n"
                "(%lu evts, %4.2f loops, %4.2f evts/loop)\"\n",
            p->name, ctx->name, ctx->total, loops, ctx->total / loops);

    if (p->xtics)
      fprintf(fp, "set xtics %s\n", p->xtics);
    else
      fprintf(fp, "set xtics autofreq\n");

    if (flags & O_OUTPNG) {
      fprintf(fp, "set term png font Vera 7\n");
      fprintf(fp, "set output \"%s.png\"\n", basename);
      fprintf(fp, "plot [0:] [0:] \"%s.dat\" using 1:%i "
                  "notitle with filledcurve x1, \\\n", basename, graphtype + 2);
      fprintf(fp, "               \"%s.dat\" using 1:%i "
                  "notitle with lines smooth bezier\n\n", basename, graphtype + 2);
    }

    if (flags & O_OUTEPS) {
      fprintf(fp, "set term postscript eps enhanced 10\n");
      fprintf(fp, "set output \"%s.eps\"\n", basename);
      fprintf(fp, "plot [0:] [0:] \"%s.dat\" using 1:%i "
                  "notitle with filledcurve x1, \\\n", basename, graphtype + 2);
      fprintf(fp, "               \"%s.dat\" using 1:%i "
                  "notitle with lines smooth bezier\n\n", basename, graphtype + 2);
    }

    if (flags & O_OUTPDF) {
      fprintf(fp, "set term pdf enhanced fname \"Helvetica\" fsize 6\n");
      fprintf(fp, "set output \"%s.pdf\"\n", basename);
      fprintf(fp, "plot [0:] [0:] \"%s.dat\" using 1:%i "
                  "notitle with filledcurve x1, \\\n", basename, graphtype + 2);
      fprintf(fp, "               \"%s.dat\" using 1:%i "
                  "notitle with lines smooth bezier\n\n", basename, graphtype + 2);
    }
  }
}

void
fprintf_phase_data(FILE *fp, phasectx_t *ctx, phasespec_t *spec, unsigned long *data)
{
  int i;
  size_t size;
  double loops;
  double loops_ceil;
  double loops_floor;
  double norm;
  double expect;
  double evts_per_loop;
  int first;
  int last;
  char asc_time[128];
  time_t curtime;

  size = spec->divs;
  loops = get_phase_loops(spec, &ctx->first, &ctx->last);
  evts_per_loop = ctx->total / loops;
  fprintf(fp, "#\n");
  fprintf(fp, "# Data for phase context '%s' and phasespec '%s'\n",
          ctx->name, spec->name);
  fprintf(fp, "#\n");
  fprintf(fp, "# field description:\n");
  fprintf(fp, "# timeslice rawcount loopavg rawprob prob\n");
  fprintf(fp, "#\n");
  fprintf(fp, "#          Infos:\n");
  fprintf(fp, "# ------------------------------------------\n");
  fprintf(fp, "#         events: %lu\n", ctx->total);
  fprintf(fp, "#          loops: %4.2f\n", loops);
  period_snprintf_uptime(asc_time, sizeof (asc_time),
                  ctx->last.tv_sec - ctx->first.tv_sec);
  fprintf(fp, "#    time_length: %s\n", asc_time);
  fprintf(fp, "#  evts_per_loop: %4.2f\n", ctx->total / loops);
  strftime(asc_time, sizeof (asc_time), "%a %b %d %H:%M:%S %Z %Y",
           localtime(&ctx->first.tv_sec));
  fprintf(fp, "#     first_date: %s\n", asc_time);
  fprintf(fp, "#    first_slice: %i\n", get_phase_slice(spec, &ctx->first));
  strftime(asc_time, sizeof (asc_time), "%a %b %d %H:%M:%S %Z %Y",
           localtime(&ctx->last.tv_sec));
  fprintf(fp, "#      last_date: %s\n", asc_time);
  fprintf(fp, "#     last_slice: %i\n", get_phase_slice(spec, &ctx->last));
  curtime = time(NULL);
  strftime(asc_time, sizeof (asc_time), "%a %b %d %H:%M:%S %Z %Y",
           localtime(&curtime));
  fprintf(fp, "#\n");
  fprintf(fp, "# Generated by Period on %s.\n\n", asc_time);


  if (loops > 1.0) {
    first = get_phase_slice(spec, &ctx->first);
    last = get_phase_slice(spec, &ctx->last);
    loops_ceil = (int)loops;
    loops_floor = loops_ceil + 1.0;

    if (first <= last) {
      for (i = 0; i < size; i++) {
        expect = data[i] / (double)ctx->total;
        if ((i >= first) && (i <= last))
          norm = data[i] / loops_floor;
        else
          norm = data[i] / loops_ceil;

        fprintf(fp, "%4i %10lu %15.4f %10.8f %10.8f\n",
                i, data[i], norm, expect, norm / evts_per_loop);
      }
    }
    else {
      for (i = 0; i < size; i++) {
        expect = data[i] / (double)ctx->total;
        if ((i > last) && (i < first))
          norm = data[i] / loops_ceil;
        else
          norm = data[i] / loops_floor;

        fprintf(fp, "%4i %10lu %15.4f %10.8f %10.8f\n",
                i, data[i], norm, expect, norm / evts_per_loop);
      }
    }
  }
  else {
    for (i = 0; i < size; i++) {
      expect = data[i] / (double)ctx->total;
      fprintf(fp, "%4i %10lu %15.4f %10.8f %10.8f\n",
              i, data[i], (double)data[i], expect, data[i] / evts_per_loop);
    }
  }
  fprintf(fp, "\n");
}

void
fprintf_phase_data_bak(FILE *fp, phasectx_t *ctx, phasespec_t *spec, unsigned long *data)
{
  int i;
  size_t size;

  size = spec->divs;
  fprintf(fp, "#\n# Data for phase context '%s' and phasespec '%s'\n#\n",
          ctx->name, spec->name);
  for (i = 0; i < size; i++) {
/*     if (data[i] > 0) */
      fprintf(fp, "%i %10lu\n", i, data[i]);
  }
  fprintf(fp, "\n");
}


void
write_phase_file(phasectx_t *ctx)
{
  char filename[1024];
  FILE *fp;
  phasespec_t *p;
  int i;

  snprintf(filename, sizeof (filename), "%s-phase.plot", ctx->name);
  fp = fopen(filename, "w");
  fprintf_phase_plot(fp, ctx, O_OUTPNG|O_VERBOSEPLOT|O_SHOWENDS|O_LOOPAVG);
  fclose(fp);

  for (i = 0, p = ctx->phasespec; p->name ; p++, i++) {
    snprintf(filename, sizeof (filename), "%s-phase-%i-%s.dat",
             ctx->name, i, p->name);
    fp = fopen(filename, "w");
    fprintf_phase_data(fp, ctx, p, ctx->count[i]);
    fclose(fp);
  }
}

static double
get_phase_loops(phasespec_t *spec, struct timeval *first, struct timeval *last)
{
  double loops;
  struct timeval diff_tv;
  double a, b;

  tvdiff(&diff_tv, last, first);

  a = diff_tv.tv_sec + (diff_tv.tv_usec / 1000000.0);
  b = spec->div_len.tv_sec + (spec->div_len.tv_usec / 1000000.0);
  loops = a / (b * spec->divs);

  return (loops);
}

static int
get_phase_slice(phasespec_t *spec, struct timeval *tv)
{
  struct tm *bdtime;
  struct timeval offset;
  int div;

  bdtime = gmtime(&tv->tv_sec);
  bdtime->tm_wday = (bdtime->tm_wday + 6) % 7;
  offset.tv_usec = tv->tv_usec;

  switch (spec->period) {

  case P_SEC:
    offset.tv_sec = 0;
    break;

  case P_MIN:
    offset.tv_sec = bdtime->tm_sec;
    break;

  case P_HOUR:
    offset.tv_sec = bdtime->tm_min * 60 + bdtime->tm_sec;
    break;

  case P_DAY:
    offset.tv_sec = bdtime->tm_hour * 3600 
      + bdtime->tm_min * 60 
      + bdtime->tm_sec;
    break;

  case P_WEEK:
    offset.tv_sec = ((bdtime->tm_wday + 6) % 7) * 24 * 60 * 60
      + bdtime->tm_hour * 60 * 60
      + bdtime->tm_min * 60 
      + bdtime->tm_sec;
    break;

  case P_MONTH:
    offset.tv_sec = (bdtime->tm_mday - 1) * 24 * 60 * 60
      + bdtime->tm_hour * 60 * 60
      + bdtime->tm_min * 60 
      + bdtime->tm_sec;
    break;

  case P_YEAR:
    offset.tv_sec = bdtime->tm_yday * 24 * 60 * 60
      + bdtime->tm_hour * 60 * 60
      + bdtime->tm_min * 60 
      + bdtime->tm_sec;
    break;

  default:
    if (spec->period <= 0) {
      fprintf(stderr, "error: unknown period type %i.\n", spec->period);
      exit(EXIT_FAILURE);
    }
    offset.tv_sec = tv->tv_sec % spec->period;
    offset.tv_usec = 0;
    break;
  }

  div = tvdiv(&offset, &spec->div_len);

  return (div);
}

#ifdef UNUSED
static void
phase(phasectx_t *phase_ctx, struct timeval *tv)
{
  struct tm *bdtime;
  struct timeval offset;
  int div;
  phasespec_t *spec;
  int i;

  if (phase_ctx->total == 0)
    phase_ctx->first = *tv;

  phase_ctx->last = *tv;

  bdtime = gmtime(&tv->tv_sec);
  bdtime->tm_wday = (bdtime->tm_wday + 6) % 7;

/*   fprintf(stderr, "phase["); */

  for (i = 0, spec = phase_ctx->phasespec; spec->name; spec++, i++) {

    offset.tv_usec = tv->tv_usec;

    switch (spec->period) {

    case P_SEC:
      offset.tv_sec = 0;
      break;

    case P_MIN:
      offset.tv_sec = bdtime->tm_sec;
      break;

    case P_HOUR:
      offset.tv_sec = bdtime->tm_min * 60 + bdtime->tm_sec;
      break;

    case P_DAY:
      offset.tv_sec = bdtime->tm_hour * 3600 
        + bdtime->tm_min * 60 
        + bdtime->tm_sec;
      break;

    case P_WEEK:
      offset.tv_sec = ((bdtime->tm_wday + 6) % 7) * 24 * 60 * 60
        + bdtime->tm_hour * 60 * 60
        + bdtime->tm_min * 60 
        + bdtime->tm_sec;
      break;

    case P_MONTH:
      offset.tv_sec = (bdtime->tm_mday - 1) * 24 * 60 * 60
        + bdtime->tm_hour * 60 * 60
        + bdtime->tm_min * 60 
        + bdtime->tm_sec;
      break;

    case P_YEAR:
      offset.tv_sec = bdtime->tm_yday * 24 * 60 * 60
        + bdtime->tm_hour * 60 * 60
        + bdtime->tm_min * 60 
        + bdtime->tm_sec;
      break;

    default:
      if (spec->period <= 0) {
        fprintf(stderr, "error: unknown period type %i.\n", spec->period);
        exit(EXIT_FAILURE);
      }
      offset.tv_sec = tv->tv_sec % spec->period;
      offset.tv_usec = 0;
      break;
    }

    div = tvdiv(&offset, &spec->div_len);
    phase_ctx->count[i][div]++;
/*     fprintf(stderr, "%s=%i ", spec->name, div); */
  }
  phase_ctx->total++;

/*   fprintf(stderr, "]"); */
}
#endif

void
fprintf_phases(FILE *fp, phasectx_t *ctx)
{
  phasespec_t *p;
  size_t size;
  int i, j;

  for (i = 0, p = ctx->phasespec; p->name ; p++, i++) {
    size = p->divs;
    fprintf(fp, "#\n# %s\n#\n", p->name);
    for (j = 0; j < size; j++) {
      if (ctx->count[i][j] > 0)
        fprintf(fp, "%i %lu\n", j, ctx->count[i][j]);
    }
    fprintf(fp, "\n");
  }
}

#ifdef UNUSED
static int
period_scroll_win(periodctx_t *ctx, struct timeval *t)
{
  struct timeval limit;
  int removed_events;

  timer_sub(&limit, t, &ctx->timewin_size);

  removed_events = 0;
  while ( timercmp(&ctx->timewin_head->time, &limit, < ) ) {
    removed_events++;
    if (ctx->timewin_head == ctx->timewin_tail) /* last evt */ {
      break;
    }
    ctx->timewin_head++;
    if (ctx->timewin_head == ctx->timewin_end) {
      ctx->timewin_head = ctx->timewin;
    }
  }

  ctx->evt_in_win -= removed_events;

  return (removed_events);
}
#endif

#ifdef UNUSED
static void
period_add_to_win(periodctx_t *ctx, struct timeval *t, struct timeval *l, int p)
{
  periodrec_t *rec;


  //fprintf(stderr, "window usage: %lu.%06lu %i\n", t->tv_sec, t->tv_usec, ctx->evt_in_win);

  /* 1st evt in win */
  if (   ctx->timewin_head == NULL
      || ctx->timewin_tail == NULL
      || ctx->evt_in_win == 0) {
    ctx->timewin_head = ctx->timewin;
    ctx->timewin_tail = ctx->timewin;
    ctx->evt_in_win = 1;
  }
  else {
    period_scroll_win(ctx, t);
    if (ctx->evt_in_win < DEFAULT_PERIOD_BACKLOG) {
      ctx->evt_in_win++;
    }
    else {
      //fprintf(stderr, "Warning, event window is full.\n");
      ctx->timewin_head++;
      if (ctx->timewin_head == ctx->timewin_end)
        ctx->timewin_head = ctx->timewin;
    }
    ctx->timewin_tail++;
    if (ctx->timewin_tail == ctx->timewin_end) {
      ctx->timewin_tail = ctx->timewin;
    }
  }

  rec = ctx->timewin_tail;
  rec->time = *t;
  rec->lag = *l;
  rec->period = p;

  do {
    struct timeval d;

    timer_sub(&d, &ctx->timewin_tail->time, &ctx->timewin_head->time);
    //fprintf(stderr, "window size: %lu.%06lu %lu.%06lu\n", t->tv_sec, t->tv_usec, d.tv_sec, d.tv_usec);
    //fprintf(stderr, "event speed: %lu.%06lu %f\n", t->tv_sec, t->tv_usec, ctx->evt_in_win / timer_float(&d) );
  } while (0);
}
#endif

#ifdef UNUSED
static void
period(periodctx_t *ctx, struct timeval *tv)
{
  struct timeval diff_tv;
  int i;
/*   double ddiff; */

  if (ctx->total == 0) {
    ctx->first.tv_usec = tv->tv_usec;
    ctx->first.tv_sec = tv->tv_sec;
    ctx->last.tv_usec = tv->tv_usec;
    ctx->last.tv_sec = tv->tv_sec;
    ctx->total++;

    return ;
  }

  if (ctx->total > DEFAULT_PERIOD_BACKLOG) {
    ctx->count[ *(ctx->backlog_pos) ]--;
    ctx->count_timewin[ *(ctx->backlog_pos) ] -= *(ctx->tbacklog_pos);
  }

  tvdiff(&diff_tv, tv, &ctx->last);
/*   printf("%i %6lu.%06lu %6lu.%06lu %f\n", */
/*          i, */
/*          ctx->last.tv_sec, ctx->last.tv_usec, */
/*          tv->tv_sec, tv->tv_usec, */
/*          diff_tv.tv_sec + (diff_tv.tv_usec / 1000000.0)); */
  ctx->last.tv_usec = tv->tv_usec;
  ctx->last.tv_sec = tv->tv_sec;

  i = 1;
  while (timercmp(&diff_tv, &ctx->periodspec[i].tv, >= )
         && ctx->periodspec[i].name)
    i++;

  period_add_to_win(ctx, tv, &diff_tv, i);


  *(ctx->backlog_pos) = i;
  ctx->backlog_pos++;
  if (ctx->backlog_pos == ctx->backlog_end)
    ctx->backlog_pos = ctx->backlog;

  *(ctx->tbacklog_pos) = diff_tv.tv_sec + (diff_tv.tv_usec / 1000000.0);
  ctx->tbacklog_pos++;
  if (ctx->tbacklog_pos == ctx->tbacklog_end)
    ctx->tbacklog_pos = ctx->tbacklog;

  ctx->count[i]++;
  ctx->count_timewin[i] += diff_tv.tv_sec + (diff_tv.tv_usec / 1000000.0);

  if (( !(ctx->flags & O_COUNTALERT) && (!(ALARM_IS_SET(ctx->flags))) )
      || (ctx->flags & O_COUNTALERT) ){
    ctx->count_total[i]++;
    ctx->total++;
    /* count mean time spent at a rate */
    ctx->count_mtime[i] += diff_tv.tv_sec + (diff_tv.tv_usec / 1000000.0);
/*     ddiff = diff_tv.tv_sec + (diff_tv.tv_usec / 1000000.0); */
/*     ctx->count_mtime[i] += (ddiff - ctx->count_mtime[i]) / (double) ctx->count_total[i]; */
  }
}
#endif

periodctx_t *
new_periodctx(periodspec_t *spec, char *name)
{
  periodctx_t *p;
  size_t spec_size;

  p = calloc(1, sizeof (periodctx_t));
  p->periodspec = spec;
  for (spec_size = 1; spec->name; spec++, spec_size++)
    ;
  p->spec_size = spec_size;
  p->count = calloc(spec_size, sizeof (unsigned long));
  p->count_mtime = calloc(spec_size, sizeof (double));
  p->count_timewin = calloc(spec_size, sizeof (double));
  p->count_total = calloc(spec_size, sizeof (unsigned long));
  p->name = name;
  p->backlog = malloc(DEFAULT_PERIOD_BACKLOG * sizeof (unsigned long));
  p->backlog_end = p->backlog + DEFAULT_PERIOD_BACKLOG;
  p->backlog_pos = p->backlog;
  p->tbacklog = malloc(DEFAULT_PERIOD_BACKLOG * sizeof (double));
  p->tbacklog_end = p->tbacklog + DEFAULT_PERIOD_BACKLOG;
  p->tbacklog_pos = p->tbacklog;
  p->timewin = malloc(DEFAULT_PERIOD_BACKLOG * sizeof (periodrec_t));
  p->timewin_end = p->timewin + DEFAULT_PERIOD_BACKLOG;

  p->timewin_size.tv_sec = 10800;

  return (p);
}

phasectx_t *
new_phasectx(phasespec_t *spec, char *name)
{
  phasectx_t *p;
  size_t spec_size;
  int i;

  p = calloc(1, sizeof (phasectx_t));
  p->phasespec = spec;
  p->name = name;

  for (spec_size = 0; spec[spec_size].name; spec_size++)
    ;
  p->count = calloc(spec_size, sizeof (unsigned long *));

  for (i = 0; i < spec_size; i++) {
    p->count[i] = calloc(spec[i].divs, sizeof (unsigned long));
  }

  return (p);
}


void
write_period_kullback_plot(periodctx_t *ctx)
{
  FILE *fp;
  char buffer[64];

  snprintf(buffer, sizeof (buffer), "%s-period-kullback.plot", ctx->name);
  fp = fopen(buffer, "w");
  fprintf(fp, "set grid\n");
  fprintf(fp, "set xlabel \"Event number\"\n");
  fprintf(fp, "set ylabel \"Kullback-Leibler distance\"\n");
  fprintf(fp, "set title \"Periodogram variation for \\\"%s\\\" event\\nWindow size = %ti\"\n",
          ctx->name, ctx->backlog_end - ctx->backlog);
  fprintf(fp, "print \"Rendering \'%s-period-kullback\'\"\n", ctx->name);
  fprintf(fp, "set term png font Vera 7\n");
  fprintf(fp, "set output \"%s-period-kullback.png\"\n", ctx->name);
  fprintf(fp, "plot [0:] [0:] \"%s-period-kullback.dat\" title \"Distance\" with lines, "
              "%10.8f title \"Attack threshold\"\n", ctx->name, ctx->threshold);
  fclose(fp);
}


#define BUFFER_SIZE 8192
#define TVUSEC_INCREMENT 2500

#if 0
int
main(int argc, char *argv[])
{
  char *retval;
  struct timeval tv;
  periodctx_t *periodctx;
  phasectx_t *phasectx;
  char buffer[BUFFER_SIZE];
  time_t last_time = 0;
  struct tm tm;
  FILE *file;
  int cur_year = 104;
  int line=0;
  FILE *kullback;
  double kl;

/*   signal(SIGINT, sigint_handler); */
/*   signal(SIGUSR1, sigusr1_handler); */

  if (argc != 3) {
    fprintf(stderr, "Usage: period <ctxname> <file>\n");
    exit(EXIT_FAILURE);
  }

  periodctx = new_periodctx(get_default_periodspec(), argv[1]);
  periodctx->threshold = 0.20;
  periodctx->flags |= O_COUNTALERT;
  phasectx = new_phasectx(get_default_phasespec(), argv[1]);

  if (!strcmp("-", argv[2]))
    file = stdin;
  else {
    file = fopen(argv[2], "r");    
    if (file == NULL) {
      perror("fopen()");
      exit(EXIT_FAILURE);
    }      
  }

  snprintf(buffer, sizeof (buffer), "%s-period-kullback.dat", argv[1]);
  kullback = fopen(buffer, "w");

  while (fgets(buffer, BUFFER_SIZE, file)) {
    line++;
/*     ret = regexec(&ctx->time_regex, buffer, 2, pmatch, 0); */
/*     strncpy(date_string, &buffer[ pmatch[1].rm_so ], pmatch[1].rm_eo - pmatch[1].rm_so); */
    memset(&tm, 0, sizeof (struct tm) );
    tm.tm_year = cur_year;
    retval = (char *)strptime(buffer, "%b %d %OH:%M:%S", &tm);
    if (retval == NULL) {
      fprintf(stderr, "Warning, time convertion failed!\n");
      continue ;
    }

/*     fprintf_tm(stdout, &tm); */

    tv.tv_sec = mktime(&tm);
    if (tv.tv_sec == last_time)
      tv.tv_usec += TVUSEC_INCREMENT;
    else if (tv.tv_sec < last_time) {
      fprintf(stderr, "Warning, assuming year change at line %i (%i -> %i)!\n",
              line, 1900 + cur_year, 1901 + cur_year);
      cur_year++;
      memset(&tm, 0, sizeof (struct tm) );
      tm.tm_year = cur_year;
      retval = (char *)strptime(buffer, "%b %d %OH:%M:%S", &tm);
      if (retval == NULL) {
        fprintf(stderr, "Warning, time convertion failed!\n");
        continue ;
      }
      tv.tv_sec = mktime(&tm);
      tv.tv_usec = 0;
    }
    else
      tv.tv_usec = 0;
    last_time = tv.tv_sec;

    period(periodctx, &tv);
    phase(phasectx, &tv);

    kl = kullback_leibler(periodctx);
    if ((kl > periodctx->threshold)
        && !(periodctx->flags & 1)) {
      periodctx->flags |= 1;
/*       fprintf(stderr, "ALARM '%s' (thres=%6.4f) from event %i ", */
/*               periodctx->name, periodctx->threshold, line); */
      fprintf(stderr, "%i,", line);
      fflush(stderr);
    }

    if ((kl <= periodctx->threshold)
        && (periodctx->flags & 1)) {
      periodctx->flags &= ~1;
/*       fprintf(stderr, "to %i\n", line); */
      fprintf(stderr, "%ip; ", line);
      fflush(stderr);
    }


    if (line > 0 && !(line % 1000)) {
      write_period_snapshot(periodctx);
/*       printf("%6i %lu.%06lu %12.10f\n", line, tv.tv_sec, tv.tv_usec, kullback_leibler(periodctx)); */
      fprintf(kullback, "%6i %12.10f\n", line, kullback_leibler_time(periodctx));
    }
  }

  write_phase_file(phasectx);
  write_period_file(periodctx);
  write_period_kullback_plot(periodctx);

  fclose(kullback);

#if 0
  for ( ; ; ) {
    memcpy(&tmpfds, &rfds, sizeof (fd_set));
    retval = select(1, &tmpfds, NULL, NULL, NULL);

    if (retval) {
      read(0, buff, 1024);
      gettimeofday(&tv, NULL);

      /* estimate period */
      period(periodctx, &tv);

      /* estimate phases */
      phase(phasectx, &tv);
    }

    if (askdisplay_g) {
      fprintf(stderr, "Rendering files...\n");
      write_phase_file(phasectx);
      write_period_file(periodctx);
      askdisplay_g = 0;
    }
  }
#endif

  fprintf(stderr, "\n");

  return 0;
}
#endif

void
fprintf_tm(FILE *fp, const struct tm *tm)
{
  fprintf(fp, "-----------------------------\n");
  fprintf(fp, "          is dst : %s\n", tm->tm_isdst ? "yes" : "no");
  fprintf(fp, " day of the year : %i\n", tm->tm_yday);
  fprintf(fp, " day of the week : %i\n", tm->tm_wday);
  fprintf(fp, "            year : %i\n", tm->tm_year);
  fprintf(fp, "           month : %i\n", tm->tm_mon);
  fprintf(fp, "day of the month : %i\n", tm->tm_mday);
  fprintf(fp, "            hour : %i\n", tm->tm_hour);
  fprintf(fp, "          minute : %i\n", tm->tm_min);
  fprintf(fp, "             sec : %i\n", tm->tm_sec);
}



# define log2of10 3.32192809488736234787
# define log2(x)     (log2of10 * log10(x))

double
kullback_leibler_time(periodctx_t *ctx)
{
  double dist;
  int size;
  int i;
  double p, q;
  double window_size;
  double context_size;
  double mtime[128] = { 0.0, };
/*   unsigned long count[128] = { 0, }; */
  periodrec_t *rec;
  struct timeval d;

  for (rec = ctx->timewin_head; rec != ctx->timewin_tail; ) {
    mtime[ rec->period ] += timer_float( &rec->lag );
    rec++;
    if (rec == ctx->timewin_end)
      rec = ctx->timewin;
  }

  size = ctx->spec_size;
  dist = 0.0;
  window_size = timer_float(&ctx->timewin_size);
  timer_sub(&d, &ctx->last, &ctx->first);
  context_size = timer_float( &d );

  for (i = 0; i < size; i++) {
    p = mtime[i] / window_size;
    q = ctx->count_mtime[i] / context_size;
    if (p > 0.0 && q > 0.0) {
      dist += p * log2( p / q );
    }
  }

  return (dist);
}


double
kullback_leibler(periodctx_t *ctx)
{
  double dist;
  int size;
  int i;
  double p, q;
  double window_size;

  size = ctx->spec_size;
  dist = 0.0;
  window_size = ctx->backlog_end - ctx->backlog;
  if (ctx->total < window_size)
    window_size = ctx->total;
  for (i = 0; i < size; i++) {
    p = ctx->count[i] / window_size;
    q = ctx->count_total[i] / (double)ctx->total;
    if (p > 0.0 && q > 0.0)
      dist += p * log2( p / q );
  }

  return (dist);
}




static void
period_snprintf_uptime(char *str, size_t size, time_t uptime)
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


  return ;
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











#if 0
static void
sigusr1_handler(int sig)
{
  askdisplay_g = 1;
}

static void
sigint_handler(int sig)
{
  fprintf(stderr, "\nExiting...\n");
  exit(EXIT_SUCCESS);
}


static void
init_phasespec(phasespec_t *p)
{
  struct timeval tv;
  int size;
/*   size_t alloc = 0; */

  size = 0;
  for ( ; p->name ; p++ ) {

    switch (p->period) {

    case P_SEC:
      tv.tv_sec = 1;
      tv.tv_usec = 0;
      break;

    case P_MIN:
      tv.tv_sec = 60;
      tv.tv_usec = 0;
      break;

    case P_HOUR:
      tv.tv_sec = 60 * 60;
      tv.tv_usec = 0;
      break;

    case P_DAY:
      tv.tv_sec = 24 * 60 * 60;
      tv.tv_usec = 0;
      break;

    case P_WEEK:
      tv.tv_sec = 7 * 24 * 60 * 60;
      tv.tv_usec = 0;
      break;

    case P_MONTH:
      tv.tv_sec = 31 * 24 * 60 * 60;
      tv.tv_usec = 0;
      break;

    case P_YEAR:
      tv.tv_sec = 366 * 24 * 60 * 60;
      tv.tv_usec = 0;
      break;

    default:
      if (p->period <= 0) {
        fprintf(stderr, "error: unknown period type %i.\n", p->period);
        exit(EXIT_FAILURE);
      }
      tv.tv_sec = p->period;
      tv.tv_usec = 0;
      break;
    }
    p->divs = tvdiv(&tv, &p->div_len);
    fprintf(stderr, "%s size=%i\n", p->name, p->divs);
/*     p->count = calloc(p->divs, sizeof (unsigned long)); */
/*     /\* XXX *\/ alloc += p->divs * sizeof (unsigned long); */
  }
/*   fprintf(stderr, "allocated %i bytes\n", alloc); */
}
#endif


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
