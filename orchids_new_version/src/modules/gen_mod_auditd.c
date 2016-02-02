/**
 ** @file gen_mod_auditd.c
 ** Functions generating part of the source of the auditd module.
 **
 ** @author Jean Goubault-Larrecq <goubault@lsv.ens-cachan.fr>
 ** @version 0.1
 ** @ingroup modules
 ** 
 **
 ** @date  Started on: Lun  1 fév 2016 17:53:18 UTC
 **/

#include <stdlib.h>
#include <stdio.h>

int main (int argc, char *argv[])
{
  int i;
  int max_args = 128;
  FILE *defs, *fields;

  defs = fopen ("defs_auditd.h", "w");
  if (defs==NULL)
    {
      fprintf (stderr, "cannot open defs_auditd.h\n");
      exit (0);
    }

  fprintf(defs, "// Generated automatically by gen_mod_auditd.c: do not touch!\n");
  fprintf(defs, "// Copyright, license: see file gen_mod_auditd.c.\n");
  for (i=0; i<=max_args; i++)
    {
      fprintf(defs, "  { \"auditd.a%d\", &t_uint, MONO_UNKNOWN,"
	     " \"auditd syscall argument %d\" },\n",
	     i, i);
      fprintf(defs, "  { \"auditd.s%d\", &t_str, MONO_UNKNOWN,"
	     " \"auditd execve argument %d\" },\n",
	     i, i);
    }
  fflush(defs);
  fclose (defs);

  fields = fopen ("fields_auditd.h", "w");
  if (fields==NULL)
    {
      fprintf (stderr, "cannot open fields_auditd.h\n");
      exit (0);
    }
  fprintf(fields, "// Generated automatically by gen_mod_auditd.c: do not touch!\n");
  fprintf(fields, "// Copyright, license: see file gen_mod_auditd.c.\n");
  fprintf(defs, "#define AUDITD_MAX_ARGS %d\n", max_args);
  fprintf(defs, "#define F_AUDITD_ARG_START F_AUDITD_REGULAR_END\n");
  for (i=0; i<max_args; i++)
    {
      fprintf(fields, "  { \"a%d=\", F_AUDITD_ARG_START+%d, action_doer_a0 },\n",
	      i, 2*i);
    }
  fprintf(defs, "#define F_AUDITD_ARG_END (F_AUDITD_ARG_START+%d)\n", 2*max_args);
  fflush (fields);
  fclose (fields);
  return 0;
}

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */



/*
** Copyright (c) 2016 by Jean GOUBAULT-LARRECQ, Laboratoire Spécification
** et Vérification (LSV), CNRS UMR 8643 & ENS Cachan.
**
** Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
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
