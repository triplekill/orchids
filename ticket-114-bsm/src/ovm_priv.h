/**
 ** @file ovm_priv.h
 ** Private definitions for ovm.c.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 1.0
 ** @ingroup ovm
 **
 ** @date  Started on: Thu Dec  4 12:06:04 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */


#ifndef _OVM_PRIVATE_H_
#define _OVM_PRIVATE_H_

/**
 ** @struct isn_param_s
 **   A structure which wrap all necessary information for an instruction.
 **/
/** @var isn_param_s::ip
 **   The instruction pointer in the current isn_param_s::bytecode sequence.
 **/
/** @var isn_param_s::bytecode
 **   The current bytecode sequence being executed.
 **/
/** @var isn_param_s::state
 **   A pointer to the state instance in which the bytecode is executed.
 **/
/** @var isn_param_s::ctx
 **   A pointer to the Orchids application context.
 **/
typedef struct isn_param_s isn_param_t;
struct isn_param_s
{
  bytecode_t *ip;
  bytecode_t *bytecode;
  state_instance_t *state;
  orchids_t *ctx;
};

static int
ovm_nop(isn_param_t *param);

/**
 ** Push a flexible variable on the current stack.
 **/
static int
ovm_push(isn_param_t *param);

static int
ovm_pop(isn_param_t *param);

static int
ovm_pushstatic(isn_param_t *param);

static int
ovm_pushfield(isn_param_t *param);

static int
ovm_trash(isn_param_t *param);

static int
ovm_call(isn_param_t *param);

static int
ovm_add(isn_param_t *param);

static int
ovm_sub(isn_param_t *param);

static int
ovm_mul(isn_param_t *param);

static int
ovm_div(isn_param_t *param);

static int
ovm_mod(isn_param_t *param);

static int
ovm_inc(isn_param_t *param);

static int
ovm_dec(isn_param_t *param);

static int
ovm_and(isn_param_t *param);

static int
ovm_or(isn_param_t *param);

static int
ovm_xor(isn_param_t *param);

static int
ovm_neg(isn_param_t *param);

static int
ovm_not(isn_param_t *param);

static int
ovm_jmp(isn_param_t *param);

static int
ovm_popcjmp(isn_param_t *param);

static int
ovm_ceq(isn_param_t *param);

static int
ovm_cneq(isn_param_t *param);

static int
ovm_crm(isn_param_t *param);

static int
ovm_cnrm(isn_param_t *param);

static int
ovm_clt(isn_param_t *param);

static int
ovm_cgt(isn_param_t *param);

static int
ovm_cle(isn_param_t *param);

static int
ovm_cge(isn_param_t *param);

static int
ovm_regsplit(isn_param_t *param);

static int
ovm_cesv(isn_param_t *param);

static int
ovm_past(isn_param_t *param);

/**
 ** The type of a function implementing a virtual machine instruction.
 **/
typedef int (*ovm_insn_t)(isn_param_t *param);


/**
 ** @struct ovm_insn_rec_s
 **   A virtual machine instruction record.  These records are used to
 **   build a table which is used, for example, in the bytecode
 **   execution function ovm_exec() and the disassembly function
 **   fprintf_bytecode().  An instruction value correspond to the
 **   offset in this table.
 **/
/** @var ovm_insn_rec_s::insn
 **   A pointer to the C function which implement the isntruction.
 **/
/** @var ovm_insn_rec_s::name
 **   The name (mnemonic) of the instruction.
 **/
typedef struct ovm_insn_rec_s ovm_insn_rec_t;
struct ovm_insn_rec_s
{
  ovm_insn_t insn;
  int        insn_sz; /* XXX: UNUSED */
  char      *name;
  void      *padding; /* XXX: UNUSED: no longer needed if insn_sz is removed */
};

/**
 ** Opcode jumptable.
 **/
static ovm_insn_rec_t ops_g[];

#endif /* _OVM_PRIVATE_H_ */

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
