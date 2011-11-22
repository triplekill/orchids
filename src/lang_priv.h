/**
 ** @file lang.c
 ** The intrusion scenario signature definition language.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 1.0
 ** @ingroup engine
 **
 ** @date  Started on: Mon Feb  3 18:11:19 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef LANG_PRIV_H
#define LANG_PRIV_H

#include "orchids.h"

#include "lang.h"

static void *
int_get_data(ovm_var_t *i);

static size_t
int_get_data_len(ovm_var_t *i);

static ovm_var_t *
int_clone(ovm_var_t *var);


static void *
uint_get_data(ovm_var_t *i);

static size_t
uint_get_data_len(ovm_var_t *i);


static void *
bytestr_get_data(ovm_var_t *str);

static size_t
bytestr_get_data_len(ovm_var_t *str);


static void *
vbstr_get_data(ovm_var_t *str);

static size_t
vbstr_get_data_len(ovm_var_t *str);

static ovm_var_t *
vbstr_clone(ovm_var_t *var);


static void *
string_get_data(ovm_var_t *str);

static size_t
string_get_data_len(ovm_var_t *str);

static ovm_var_t *
string_clone(ovm_var_t *var);


static void *
vstring_get_data(ovm_var_t *str);

static size_t
vstring_get_data_len(ovm_var_t *str);

static ovm_var_t *
vstr_clone(ovm_var_t *var);


static void *
counter_get_data(ovm_var_t *i);

static size_t
counter_get_data_len(ovm_var_t *i);

static int
counter_cmp(ovm_var_t *var1, ovm_var_t *var2);

static ovm_var_t *
counter_add(ovm_var_t *var1, ovm_var_t *var2);

static ovm_var_t *
counter_mul(ovm_var_t *var1, ovm_var_t *var2);

static ovm_var_t *
counter_clone(ovm_var_t *var);


static void *
regex_get_data(ovm_var_t *regex);

static size_t
regex_get_data_len(ovm_var_t *regex);

void
regex_destruct(ovm_var_t *regex);

static void *
splitregex_get_data(ovm_var_t *regex);

static size_t
splitregex_get_data_len(ovm_var_t *regex);

void
splitregex_destruct(ovm_var_t *regex);


static void *
address_get_data(ovm_var_t *address);

static size_t
address_get_data_len(ovm_var_t *address);


static int
str_cmp(ovm_var_t *var1, ovm_var_t *var2);

static ovm_var_t *
str_add(ovm_var_t *var1, ovm_var_t *var2);


static int
vstr_cmp(ovm_var_t *var1, ovm_var_t *var2);

static ovm_var_t *
vstr_add(ovm_var_t *var1, ovm_var_t *var2);


static int
int_cmp(ovm_var_t *var1, ovm_var_t *var2);

static ovm_var_t *
int_add(ovm_var_t *var1, ovm_var_t *var2);

static ovm_var_t *
int_sub(ovm_var_t *var1, ovm_var_t *var2);

static ovm_var_t *
int_mul(ovm_var_t *var1, ovm_var_t *var2);

static ovm_var_t *
int_div(ovm_var_t *var1, ovm_var_t *var2);

static ovm_var_t *
int_mod(ovm_var_t *var1, ovm_var_t *var2);


static int
uint_cmp(ovm_var_t *var1, ovm_var_t *var2);

static ovm_var_t *
uint_add(ovm_var_t *var1, ovm_var_t *var2);

static ovm_var_t *
uint_sub(ovm_var_t *var1, ovm_var_t *var2);

static ovm_var_t *
uint_mul(ovm_var_t *var1, ovm_var_t *var2);

static ovm_var_t *
uint_div(ovm_var_t *var1, ovm_var_t *var2);

static ovm_var_t *
uint_mod(ovm_var_t *var1, ovm_var_t *var2);

static ovm_var_t *
uint_clone(ovm_var_t *var);


static void *
ipv4_get_data(ovm_var_t *addr);

static size_t
ipv4_get_data_len(ovm_var_t *addr);

static int
ipv4_cmp(ovm_var_t *var1, ovm_var_t *var2);

static ovm_var_t *
ipv4_clone(ovm_var_t *var);


static void *
timeval_get_data(ovm_var_t *str);

static size_t
timeval_get_data_len(ovm_var_t *str);

static ovm_var_t *
timeval_add(ovm_var_t *var1, ovm_var_t *var2);

static ovm_var_t *
timeval_sub(ovm_var_t *var1, ovm_var_t *var2);

static ovm_var_t *
timeval_mul(ovm_var_t *var1, ovm_var_t *var2);

static ovm_var_t *
timeval_div(ovm_var_t *var1, ovm_var_t *var2);

static ovm_var_t *
timeval_mod(ovm_var_t *var1, ovm_var_t *var2);

static int
timeval_cmp(ovm_var_t *var1, ovm_var_t *var2);

static ovm_var_t *
timeval_clone(ovm_var_t *var);


static void *
float_get_data(ovm_var_t *i);

static size_t
float_get_data_len(ovm_var_t *i);

static int
float_cmp(ovm_var_t *var1, ovm_var_t *var2);

static ovm_var_t *
float_add(ovm_var_t *var1, ovm_var_t *var2);

static ovm_var_t *
float_sub(ovm_var_t *var1, ovm_var_t *var2);

static ovm_var_t *
float_mul(ovm_var_t *var1, ovm_var_t *var2);

static ovm_var_t *
float_div(ovm_var_t *var1, ovm_var_t *var2);

static ovm_var_t *
float_clone(ovm_var_t *var);


static ovm_var_t *
bstr_clone(ovm_var_t *var);


static void *
ctime_get_data(ovm_var_t *t);

static size_t
ctime_get_data_len(ovm_var_t *i);

static int
ctime_cmp(ovm_var_t *var1, ovm_var_t *var2);

static ovm_var_t *
ctime_add(ovm_var_t *var1, ovm_var_t *var2);

static ovm_var_t *
ctime_sub(ovm_var_t *var1, ovm_var_t *var2);

static ovm_var_t *
ctime_mul(ovm_var_t *var1, ovm_var_t *var2);

static ovm_var_t *
ctime_div(ovm_var_t *var1, ovm_var_t *var2);

static ovm_var_t *
ctime_mod(ovm_var_t *var1, ovm_var_t *var2);

static ovm_var_t *
ctime_clone(ovm_var_t *var);

static void *
extern_get_data(ovm_var_t *address);

static size_t
extern_get_data_len(ovm_var_t *address);

void
extern_destruct(ovm_var_t *var);

#endif /* LANG_PRIV_H */

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
