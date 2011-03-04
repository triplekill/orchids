/**
 ** @file mod_prism2.h
 ** Definitions for mod_prism2.c
 ** 
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 ** 
 ** @version 0.1.0
 ** 
 ** @date  Started on: Fri Jun 15 09:17:58 2007
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef MODPRISM2_H
#define MODPRISM2_H

#define PRISM_STATUS_PRESENT 0

#define PRISM_VAL_PRESENT(v) \
  ( ((v)->status) == PRISM_STATUS_PRESENT )

#define PRISM_VAL(v) \
  ( (v)->data )

#define DECODE_PRISM_VALUE(field, value)        \
  do {                                          \
    prismval_t *pv;                             \
    pv = (value);                               \
    if ( PRISM_VAL_PRESENT(pv) ) {              \
      (field) = ovm_uint_new();                 \
      UINT(field) = PRISM_VAL(pv);              \
    }                                           \
  } while (0)

typedef struct prismval_s prismval_t;
struct prismval_s
{
  u_int32_t did;
  u_int16_t status;
  u_int16_t len;
  u_int32_t data;
};

typedef struct prism2_hdr_s prism2_hdr_t;
struct prism2_hdr_s
{
  u_int32_t msgcode;
  u_int32_t msglen;
  u_int8_t devname[16];
  prismval_t hosttime;
  prismval_t mactime;
  prismval_t channel;
  prismval_t rssi;
  prismval_t sq;
  prismval_t signal;
  prismval_t noise;
  prismval_t rate;
  prismval_t istx;
  prismval_t frmlen;
};

/*
** Copyright (c) 2002-2007 by Julien OLIVAIN, Laboratoire Spécification
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


#endif /* MODPRISM2_H */
