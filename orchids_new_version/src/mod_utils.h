/**
 ** @file mod_utils.h
 ** Definitions for mod_utils.c
 **
 ** @author Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup core
 **
 ** @date  Started on: Mer  1 oct 2014 09:28:22 UTC
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef MOD_UTILS_H
#define MOD_UTILS_H

/* Utility functions to create subdissectors for streams
   of data coming typically from network connections in binary format
*/

typedef struct blox_hook_s blox_hook_t;

/* Blox states, as used in compute_length_fun() */
#define BLOX_INIT 0
#define BLOX_NOT_ALIGNED 1
#define BLOX_FINAL 2

#define BLOX_NSTATES 3

typedef size_t (*compute_length_fun) (unsigned char *first_bytes,
				      size_t n_first_bytes,
				      int *state, /* pointer to blox state */
				      void *sd_data);
typedef void (*subdissect_fun) (orchids_t *ctx, mod_entry_t *mod,
				event_t *event,
				ovm_var_t *delegate, /* for stream, stream_len */
				unsigned char *stream, size_t stream_len,
				void *sd_data);

struct blox_hook_s {
  size_t n_first_bytes; /* number of bytes to read off records to be able
			   to compute length of record in state BLOX_INIT */
  size_t n_bytes;
  int state;
  compute_length_fun compute_length;
  subdissect_fun subdissect;
  ovm_var_t *remaining; /* of type T_BSTR or T_VBSTR; this should be
			 a root of the GC. */
  event_t *event; /* should also be a root of the GC */
  mod_entry_t *mod;
  void *sd_data;
};


blox_hook_t *init_blox_hook(orchids_t *ctx,
			    mod_entry_t *mod,
			    size_t n_first_bytes,
			    compute_length_fun compute_length,
			    subdissect_fun subdissect,
			    void *sd_data
			    );
int blox_dissect(orchids_t *ctx, mod_entry_t *mod, event_t *event,
		 void *data);

#endif /* MOD_UTILS_H */

/*
** Copyright (c) 2014 by Jean GOUBAULT-LARRECQ, Laboratoire Spécification
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