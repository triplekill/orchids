/**
 ** @file orchids_types.h
 ** Types definition.
 ** 
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 ** 
 ** @version 0.1
 ** @ingroup core
 ** 
 ** @date  Started on: Fri Nov 28 15:32:53 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef ORCHIDS_TYPES
#define ORCHIDS_TYPES

#include <sys/time.h>
#include <sys/types.h>
#include <stdint.h>
#include <unistd.h>

/* standard types unification */
typedef struct timeval timeval_t;
typedef fd_set fd_set_t;
typedef struct rusage rusage_t;

/* custom types */
typedef int bool_t;

#ifndef HAVE_STDINT
typedef   signed char  int8_t;
typedef unsigned char uint8_t;

typedef   signed short  int16_t;
typedef unsigned short uint16_t;

typedef   signed int  int32_t;
typedef unsigned int uint32_t;

# if __WORDSIZE == 64
typedef   signed long  int64_t;
typedef unsigned long uint64_t;
#else
typedef   signed long long  int64_t;
typedef unsigned long long uint64_t;
# endif

#endif /* HAVE_STDINT */


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


#endif /* ORCHIDS_TYPES */
