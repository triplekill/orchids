/**
 ** @file gen_mod_openbsm.c
 ** Functions generating part of the source of the openbsm module.
 **
 ** @author Jean Goubault-Larrecq <goubault@lsv.ens-cachan.fr>
 ** @version 0.1
 ** @ingroup modules
 ** 
 **
 ** @date  Started on: Dim  5 f�v 2012 19:39:49
 **/

#include <stdlib.h>
#include <stdio.h>
// #include <bsm/libbsm.h>

int main (int argc, char *argv[])
{
  int i;
  int max_args = 128;
  int max_env = 128;
  int max_groups = 16;

  printf("// Generated automatically by gen_mod_openbsm.c: do not touch!\n");
  printf("// Copyright, license: see file gen_mod_openbsm.c.\n");
  printf("#define OPENBSM_MAX_ARGS %d\n", max_args);
  printf("#define OPENBSM_MAX_ENV %d\n", max_env);
  printf("#define OPENBSM_MAX_GROUPS %d\n", max_groups);

  printf("#define F_OPENBSM_ARG_START F_OPENBSM_REGULAR_END\n");
  for (i=0; ++i<=max_args; )
    {
      printf("  { \"openbsm.arg%d\", &t_uint,"
	     " \"openbsm argument %d\" },\n",
	     i, i);
      printf("  { \"openbsm.argname%d\", &t_str,"
	     " \"openbsm argument name %d\" },\n",
	     i, i);
    }
  printf("#define F_OPENBSM_ARG_END (F_OPENBSM_ARG_START+%d)\n", 2*max_args);

  printf("#define F_OPENBSM_EXECARG_START F_OPENBSM_ARG_END\n");
  printf("  { \"bsm.execarg_num\", &t_uint,"
	 " \"bsm exec argument number\" },\n");
  printf("#define F_OPENBSM_EXECARG_NUM 0\n");
  for (i=0; ++i<=max_args; )
    printf("  { \"bsm.execarg%d\", &t_str,"
	   " \"openbsm exec argument %d\" },\n",
	   i, i);
  printf("#define F_OPENBSM_EXECARG_END (F_OPENBSM_EXECARG_START+%d)\n",
	 max_args+1);

  printf("#define F_OPENBSM_EXECENV_START F_OPENBSM_EXECARG_END\n");
  printf("  { \"bsm.execenv_num\", &t_uint,"
	 " \"bsm exec number of environment variables\" },\n");
  printf("#define F_OPENBSM_EXECENV_NUM 0\n");
  for (i=0; ++i<=max_env; )
    printf("  { \"bsm.execenv%d\", &t_str,"
	   " \"openbsm exec environment variable %d\" },\n",
	   i, i);
  printf("#define F_OPENBSM_EXECENV_END (F_OPENBSM_EXECENV_START+%d)\n",
	 max_env+1);

  printf("#define F_OPENBSM_NEWGROUPS_START F_OPENBSM_EXECENV_END\n");
  printf("  { \"bsm.newgroups_num\", &t_uint,"
	 " \"bsm number of new groups\" },\n");
  printf("#define F_OPENBSM_NEWGROUPS_NUM 0\n");
  for (i=0; ++i<=max_groups; )
    printf("  { \"bsm.newgroup%d\", &t_uint,"
	   " \"openbsm new group %d\" },\n",
	   i, i);
  printf("#define F_OPENBSM_NEWGROUPS_END (F_OPENBSM_NEWGROUPS_START+%d)\n",
	 max_groups+1);
  printf("#define OPENBSM_FIELDS F_OPENBSM_NEWGROUPS_END\n");
  fflush(stdout);
  return 0;
}

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */


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
