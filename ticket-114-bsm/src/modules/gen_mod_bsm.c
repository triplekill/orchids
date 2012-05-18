/**
 ** @file gen_mod_bsm.c
 ** Functions generating part of the source of the bsm module.
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
#include <bsm/libbsm.h>

int
main (int argc, char *argv[])
{
  int i;

  printf("// Generated automatically by gen_mod_bsm.c: do not touch!\n");
  printf("// Copyright, license: see file gen_mod.bsm.c.\n");
  printf("  { \"bsm.groups.len\", T_INT,"
	 " \"bsm groups: number of groups\" },\n");
  for (i=0; ++i<=AUDIT_MAX_GROUPS; )
    printf("  { \"bsm.groups.elt%d\", T_INT,"
	   " \"bsm groups: element no. %d\" },\n",
	   i, i);
  printf("  { \"bsm.newgroups.len\", T_INT,"
	 " \"bsm newgroups: number of groups\" },\n");
  for (i=0; ++i<=AUDIT_MAX_GROUPS; )
    printf("  { \"bsm.newgroups.elt%d\", T_INT,"
	   " \"bsm newgroups: element no. %d\" },\n",
	   i, i);
  printf("  { \"bsm.execargs.len\", T_INT,"
	 " \"bsm execargs: number of arguments\" },\n");
  for (i=0; ++i<=AUDIT_MAX_ARGS; )
    printf("  { \"bsm.execargs.elt%d\", T_VSTR,"
	   " \"bsm execargs: element no. %d\" },\n",
	   i, i);
  printf("  { \"bsm.execenv.len\", T_INT,"
	 " \"bsm execenv: number of environment variables\" },\n");
  for (i=0; ++i<=AUDIT_MAX_ENV; )
    printf("  { \"bsm.execenv.elt%d\", T_VSTR,"
	   " \"bsm execenv: element no. %d\" },\n",
	   i, i);
  for (i=0; ++i<=AUDIT_MAX_ARGS; )
    printf("  { \"bsm.arg%d\", T_INT,"
	   " \"bsm argument %d\" },\n",
	   i, i);
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
