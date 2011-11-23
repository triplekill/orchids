/**
 ** @file string_util.c
 ** Some string functions.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup util
 **
 ** @date  Started on: Tue Feb 18 17:15:32 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>

#include "orchids.h"

size_t
get_next_token(const char *pos, int c, size_t n)
{
  size_t token_size;

  token_size = 0;
  while ((token_size <= n) && (*pos++ != (char)c))
    token_size++;

  return (token_size);
}

size_t
get_next_int(const char *istr, long *i, size_t n)
{
  size_t sz;

  *i = 0;
  sz = 0;
  while ((*istr >= '0') && (*istr <= '9') && (sz <= n))
    {
      *i *= 10;
      *i += *istr - '0';
      ++istr;
      ++sz;
    }

  return (sz);
}

size_t
my_strspn(const char *pos, const char *eot, size_t n)
{
  size_t token_size;
  const char *r;

  for (token_size = 0; token_size < n; ++token_size, ++pos)
    {
      for (r = eot; *r != '\0'; ++r)
        if (*pos == *r)
          return (token_size);
    }

  return (token_size);
}

bool_t
fnmatch_test(const char *pattern)
{
  int nesting;

  nesting = 0;

  while (*pattern) {
    switch (*pattern) {

    case '?':
    case '*':
      return (TRUE);

    case '\\':
      if (*pattern++ == '\0')
        return (FALSE);
      break;

    /* '[' is only a glob if it has a matching ']' */
    case '[':       
      ++nesting;
      break;

    case ']':
      if (nesting)
        return (TRUE);
      break;
    }
    ++pattern;
  }

  return (FALSE);
}



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
