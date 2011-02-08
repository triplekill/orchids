/**
 ** @file linux_process_info.h
 ** Linux process info header.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup util
 **
 ** @date  Started on: Wed Jun 11 13:33:08 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef _LIN_PROC_INFO_H_
#define _LIN_PROC_INFO_H_

/*
** linux (2.4) process informations in /proc/<pid>/stat
** see manpage proc(5) more informations
*/
typedef struct linux_process_info_s linux_process_info_t;
struct linux_process_info_s
{
  pid_t         pid;
  char          comm[16];
  char          pad_for_state[3];
  char          state;
  pid_t         ppid;
  pid_t         pgrp;
  pid_t         session;
  int           tty_nr;
  pid_t         tpgid;
  unsigned long flags;
  unsigned long minflt;
  unsigned long cminflt;
  unsigned long majflt;
  unsigned long cmajflt;
  unsigned long utime;
  unsigned long stime;
  long          cutime;
  long          cstime;
  long          priority;
  long          nice;
  long          itrealvalue;
  unsigned long starttime;
  unsigned long vsize;
  long          rss;
  unsigned long rlim;
  unsigned long startcode;
  unsigned long endcode;
  unsigned long startstack;
  unsigned long kstkesp;
  unsigned long kstkeip;
  unsigned long signal;
  unsigned long blocked;
  unsigned long sigignore;
  unsigned long sigcatch;
  unsigned long wchan;
  unsigned long nswap;
  unsigned long cnswap;
  int           exit_signal;
  int           processor;
};

/*
** functions
*/

void fprintf_linux_process_info(FILE *fp, linux_process_info_t *p);
void fprintf_linux_process_summary(FILE *fp, linux_process_info_t *p);
void get_linux_process_info(linux_process_info_t *p, pid_t pid);
void fprintf_linux_process_html_summary(FILE *fp, linux_process_info_t *p);

#endif /* _LIN_PROC_INFO_H_ */



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
