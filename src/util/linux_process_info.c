/**
 ** @file linux_process_info.c
 ** get process info from /proc/[0-9]+/stat.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup util
 **
 ** @date  Started on: Thu Jun  5 18:57:14 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include "linux_process_info.h"

#if 0 /* if compiler isn't ISO C89 compliant */

void
fprintf_linux_process_info(FILE *fp, linux_process_info_t *p)
{
  fprintf(fp,
          "        pid: %d\n"
          "       comm: %s\n"
          "      state: %c\n"
          "       ppid: %d\n"
          "       pgrp: %d\n"
          "    session: %d\n"
          "     tty_nr: %d\n"
          "      tpgid: %d\n"
          "      flags: 0x%08lx\n"
          "     minflt: %lu\n"
          "    cminflt: %lu\n"
          "     majflt: %lu\n"
          "    cmajflt: %lu\n"
          "      utime: %lu jiffie(s)\n"
          "      stime: %lu jiffie(s)\n"
          "     cutime: %ld jiffie(s)\n"
          "     cstime: %ld jiffie(s)\n"
          "   priority: %ld\n"
          "       nice: %ld\n"
          "itrealvalue: %ld jiffie(s)\n"
          "  starttime: %lu jiffie(s) after system boot\n"
          "      vsize: %lu bytes (%5.3f Mb)\n"
          "        rss: %ld pages (%5.3f Mb)\n"
          "       rlim: %lu bytes\n"
          "  startcode: 0x%08lx\n"
          "    endcode: 0x%08lx\n"
          " startstack: 0x%08lx\n"
          "    kstkesp: 0x%08lx\n"
          "    kstkeip: 0x%08lx\n"
          "     signal: 0x%08lx\n"
          "    blocked: 0x%08lx\n"
          "  sigignore: 0x%08lx\n"
          "   sigcatch: 0x%08lx\n"
          "      wchan: %lu\n"
          "      nswap: %lu pages\n"
          "     cnswap: %lu pages\n"
          "exit_signal: %d\n"
          "  processor: %d\n",
          p->pid, p->comm, p->state, p->ppid, p->pgrp, p->session, p->tty_nr,
          p->tpgid, p->flags, p->minflt, p->cminflt, p->majflt, p->cmajflt,
          p->utime, p->stime, p->cutime, p->cstime, p->priority, p->nice,
          p->itrealvalue, p->starttime, p->vsize, p->vsize / 1048576.0,
          p->rss, (p->rss * 4) / 1024.0 ,p->rlim,
          p->startcode, p->endcode, p->startstack, p->kstkesp, p->kstkeip,
          p->signal, p->blocked, p->sigignore, p->sigcatch, p->wchan,
          p->nswap, p->cnswap, p->exit_signal, p->processor);
}

#else

void
fprintf_linux_process_info(FILE *fp, linux_process_info_t *p)
{
  fprintf(fp, "        pid: %d\n", p->pid);
  fprintf(fp, "       comm: %s\n", p->comm);
  fprintf(fp, "      state: %c\n", p->state);
  fprintf(fp, "       ppid: %d\n", p->ppid);
  fprintf(fp, "       pgrp: %d\n", p->pgrp);
  fprintf(fp, "    session: %d\n", p->session);
  fprintf(fp, "     tty_nr: %d\n", p->tty_nr);
  fprintf(fp, "      tpgid: %d\n", p->tpgid);
  fprintf(fp, "      flags: 0x%08lx\n", p->flags);
  fprintf(fp, "     minflt: %lu\n", p->minflt);
  fprintf(fp, "    cminflt: %lu\n", p->cminflt);
  fprintf(fp, "     majflt: %lu\n", p->majflt);
  fprintf(fp, "    cmajflt: %lu\n", p->cmajflt);
  fprintf(fp, "      utime: %lu jiffie(s)\n", p->utime);
  fprintf(fp, "      stime: %lu jiffie(s)\n", p->stime);
  fprintf(fp, "     cutime: %ld jiffie(s)\n", p->cutime);
  fprintf(fp, "     cstime: %ld jiffie(s)\n", p->cstime);
  fprintf(fp, "   priority: %ld\n", p->priority);
  fprintf(fp, "       nice: %ld\n", p->nice);
  fprintf(fp, "itrealvalue: %ld jiffie(s)\n", p->itrealvalue);
  fprintf(fp, "  starttime: %lu jiffie(s) after system boot\n", p->starttime);
  fprintf(fp, "      vsize: %lu bytes (%5.3f Mb)\n", p->vsize, p->vsize / 1048576.0);
  fprintf(fp, "        rss: %ld pages (%5.3f Mb)\n", p->rss, (p->rss * 4) / 1024.0);
  fprintf(fp, "       rlim: %lu bytes\n", p->rlim);
  fprintf(fp, "  startcode: 0x%08lx\n", p->startcode);
  fprintf(fp, "    endcode: 0x%08lx\n", p->endcode);
  fprintf(fp, " startstack: 0x%08lx\n", p->startstack);
  fprintf(fp, "    kstkesp: 0x%08lx\n", p->kstkesp);
  fprintf(fp, "    kstkeip: 0x%08lx\n", p->kstkeip);
  fprintf(fp, "     signal: 0x%08lx\n", p->signal);
  fprintf(fp, "    blocked: 0x%08lx\n", p->blocked);
  fprintf(fp, "  sigignore: 0x%08lx\n", p->sigignore);
  fprintf(fp, "   sigcatch: 0x%08lx\n", p->sigcatch);
  fprintf(fp, "      wchan: %lu\n", p->wchan);
  fprintf(fp, "      nswap: %lu pages\n", p->nswap);
  fprintf(fp, "     cnswap: %lu pages\n", p->cnswap);
  fprintf(fp, "exit_signal: %d\n", p->exit_signal);
  fprintf(fp, "  processor: %d\n", p->processor);
}

#endif /* 0 */

#ifndef ORCHIDS_DEMO

void
fprintf_linux_process_summary(FILE *fp, linux_process_info_t *p)
{
  fprintf(fp, "                pid : %d\n", p->pid);
  fprintf(fp, "               ppid : %d\n", p->ppid);
  fprintf(fp, "             tty_nr : %d\n", p->tty_nr);
  fprintf(fp, "           priority : %ld\n", p->priority);
  fprintf(fp, "               nice : %ld\n", p->nice);
  fprintf(fp, "              vsize : %lu bytes (%5.3f MB)\n", p->vsize, p->vsize / 1048576.0);
  fprintf(fp, "                rss : %ld pages (%5.3f MB)\n", p->rss, (p->rss * 4) / 1024.0);
  fprintf(fp, "               rlim : %lu bytes\n", p->rlim);
  fprintf(fp, "            blocked : 0x%08lx\n", p->blocked);
  fprintf(fp, "          sigignore : 0x%08lx\n", p->sigignore);
  fprintf(fp, "           sigcatch : 0x%08lx\n", p->sigcatch);
  fprintf(fp, "          processor : %d\n", p->processor);
}

void
fprintf_linux_process_html_summary(FILE *fp, linux_process_info_t *p)
{
  fprintf(fp, "<tr> <td class=\"e0\"> pid </td> <td class=\"v0\"> %d </td> </tr>\n", p->pid);
  fprintf(fp, "<tr> <td class=\"e1\"> ppid </td> <td class=\"v1\"> %d </td> </tr>\n", p->ppid);
  fprintf(fp, "<tr> <td class=\"e0\"> tty_nr </td> <td class=\"v0\"> %d </td> </tr>\n", p->tty_nr);
  fprintf(fp, "<tr> <td class=\"e1\"> priority </td> <td class=\"v1\"> %ld </td> </tr>\n", p->priority);
  fprintf(fp, "<tr> <td class=\"e0\"> nice </td> <td class=\"v0\"> %ld </td> </tr>\n", p->nice);
  fprintf(fp, "<tr> <td class=\"e1\"> vsize </td> <td class=\"v1\"> %lu bytes (%5.3f MB) </td> </tr>\n", p->vsize, p->vsize / 1048576.0);
  fprintf(fp, "<tr> <td class=\"e0\"> rss </td> <td class=\"v0\"> %ld pages (%5.3f MB) </td> </tr>\n", p->rss, (p->rss * 4) / 1024.0);
  fprintf(fp, "<tr> <td class=\"e1\"> rlim </td> <td class=\"v1\"> %lu bytes </td> </tr>\n", p->rlim);
  fprintf(fp, "<tr> <td class=\"e0\"> blocked </td> <td class=\"v0\"> 0x%08lx </td> </tr>\n", p->blocked);
  fprintf(fp, "<tr> <td class=\"e1\"> sigignore </td> <td class=\"v1\"> 0x%08lx </td> </tr>\n", p->sigignore);
  fprintf(fp, "<tr> <td class=\"e0\"> sigcatch </td> <td class=\"v0\"> 0x%08lx </td> </tr>\n", p->sigcatch);
  fprintf(fp, "<tr> <td class=\"e1\"> processor </td> <td class=\"v1\"> %d </td> </tr>\n", p->processor);
}


#else /* ORCHIDS_DEMO */

void
fprintf_linux_process_summary(FILE *fp, linux_process_info_t *p)
{
  fprintf(fp, "                pid : %d\n", p->pid);
  fprintf(fp, "           priority : %ld\n", p->priority);
  fprintf(fp, "              vsize : %lu bytes (%5.3f MB)\n", p->vsize, p->vsize / 1048576.0);
  fprintf(fp, "                rss : %ld pages (%5.3f MB)\n", p->rss, (p->rss * 4) / 1024.0);
}

void
fprintf_linux_process_html_summary(FILE *fp, linux_process_info_t *p)
{
  fprintf(fp, "<tr> <td class=\"e0\"> pid </td> <td class=\"v0\"> %d </td> </tr>\n", p->pid);
  fprintf(fp, "<tr> <td class=\"e1\"> priority </td> <td class=\"v1\"> %ld </td> </tr>\n", p->priority);
  fprintf(fp, "<tr> <td class=\"e0\"> vsize </td> <td class=\"v0\"> %lu bytes (%5.3f MB) </td> </tr>\n", p->vsize, p->vsize / 1048576.0);
  fprintf(fp, "<tr> <td class=\"e1\"> rss </td> <td class=\"v1\"> %ld pages (%5.3f MB) </td> </tr>\n", p->rss, (p->rss * 4) / 1024.0);
}

#endif /* ORCHIDS_DEMO */


/*
** WARNING: doesn't work if the process comm name contains ")" or " "
*/
void
get_linux_process_info(linux_process_info_t *p, pid_t pid)
{
  char stat_path[20]; /* max len is '/proc/12345/stat\0' (17) */
  FILE *fp;

  if (pid == 0) {
    strcpy(stat_path, "/proc/self/stat");
  }
  else {
    snprintf(stat_path, 20, "/proc/%d/stat", pid);
  }

  fp = fopen(stat_path, "r");
  if (fp == NULL)
    {
      fprintf(stderr, "fopen(%s): %s\n", stat_path, strerror(errno));
      exit(EXIT_FAILURE);
    }

  fscanf(fp,
         "%d "  /* pid */
         "%s "  /* comm -- BUG: doen't work if comm contains ')' or ' ' */
         "%c "  /* state */
         "%d "  /* ppid */
         "%d "  /* pgrp */
         "%d "  /* session */
         "%d "  /* tty_nr */
         "%d "  /* tpgid */
         "%lu " /* flags */
         "%lu " /* minflt */
         "%lu " /* cminflt */ 
         "%lu " /* majflt */
         "%lu " /* cmajflt */
         "%lu " /* utime */
         "%lu " /* stime */
         "%ld " /* cutime */
         "%ld " /* cstime */
         "%ld " /* priority */
         "%ld " /* nice */
         "%*d " /* 0 */
         "%ld " /* itrealvalue */
         "%lu " /* starttime */
         "%lu " /* vsize */
         "%ld " /* rss */
         "%lu " /* rlim */
         "%lu " /* startcode */
         "%lu " /* endcode */
         "%lu " /* startstack */
         "%lu " /* kstkesp */
         "%lu " /* kstkeip */
         "%lu " /* signal */
         "%lu " /* blocked */
         "%lu " /* sigignore */
         "%lu " /* sigcatch */
         "%lu " /* wchan */
         "%lu " /* nswap */
         "%lu " /* cnswap */
         "%d "  /* exit_signal */
         "%d ", /* processor */
         &p->pid, p->comm, &p->state, &p->ppid, &p->pgrp, &p->session,
         &p->tty_nr, &p->tpgid, &p->flags, &p->minflt, &p->cminflt,
         &p->majflt, &p->cmajflt,
         &p->utime, &p->stime, &p->cutime, &p->cstime, &p->priority, &p->nice,
         &p->itrealvalue, &p->starttime, &p->vsize, &p->rss, &p->rlim,
         &p->startcode, &p->endcode, &p->startstack, &p->kstkesp, &p->kstkeip,
         &p->signal, &p->blocked, &p->sigignore, &p->sigcatch, &p->wchan,
         &p->nswap, &p->cnswap, &p->exit_signal, &p->processor);

  if (fclose(fp))
    {
      fprintf(stderr, "fclose(%s): %s\n", stat_path, strerror(errno));
      exit(EXIT_FAILURE);
    }
}

#if 0
int
main(int argc, char *argv[])
{
  linux_process_info_t pinfo;

  get_linux_process_info(&pinfo, getpid());

  fprintf_linux_process_info(stdout, &pinfo);

  return (0);
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
