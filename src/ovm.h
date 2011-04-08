/**
 ** @file ovm.h
 ** Orchids 'virtual machine' bytecode definition.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup ovm
 **
 ** @date  Started on: Wed Mar 12 12:20:17 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

/**
 ** @defgroup ovm Orchids Virtual Machine
 **/

#ifndef OVM_H
#define OVM_H

#define PUSH_RETURN_TRUE(ctx,state)					\
    stack_push(ctx->ovm_stack,						\
	       state->state->rule->static_env[ ctx->rule_compiler->static_1_res_id]);

#define PUSH_RETURN_FALSE(ctx,state)					\
    stack_push(ctx->ovm_stack,					\
	       state->state->rule->static_env[ ctx->rule_compiler->static_0_res_id]);

/*
** Orchids VM Op Codes Definition
*/

/*
**
** (0) OP_END [] ... => ...
**
** () OP_NOP [] ... => ...
** No Operation instruction, do nothing.
**
** () OP_PUSH [n] ... => ..., value
** Put the n-th element from the local variable array on th current stack
**
** () OP_POP [n] ..., value => ...
** Set the n-th element of local variable array to the top stack value.
**
** () OP_ALOAD [] ..., a, n => ..., value
** Put the n-th element of array a on the stack.
**
** () OP_ASTORE [] ..., a, n, value => ...
** Set the n-th element of array a to value.
**
** () OP_ALENGTH [] ..., a => ..., array_length
** returns the array a length on the stack. (may be implemented as a OP_CALL)
**
** () OP_CALL [p] ..., argn, ..., arg2, arg1 => ..., retval
** call the p-th procedure. procedure implementation should only have a
** reference of the current stack.
**
*/

/**
 ** Number of total opcode.
 **/
#define OPCODE_NUM 40

/**
 ** End of program. Stop the program execution and return a normal exit.
 **/
#define OP_END 0

/**
 * No OPeration.
 **/
#define OP_NOP 1

/**
 * Put a dynamic variable on the stack.
 **/
#define OP_PUSH 2

/**
 * Load the value on top of the stack into a dynamic environment value.
 **/
#define OP_POP 3

/**
 * Put a static variable on the stack.
 **/
#define OP_PUSHSTATIC 4

/**
 * Put a field value on the stack.
 **/
#define OP_PUSHFIELD 5

/**
 * Trash the value on the top of the stack (free it if necessary)
 **/
#define OP_TRASH 6

/**
 * Call an ISSDL built-in function.
 **/
#define OP_CALL 7



/* Arithmetic */

/*
** OP_xxx [] ..., op2, op1 => ..., result
*/

/**
 * Addition.
 **/
#define OP_ADD 8

/**
 * Subtraction
 **/
#define OP_SUB 9

/**
 * Multiplication
 **/
#define OP_MUL 10

/**
 * Division
 **/
#define OP_DIV 11

/**
 * Modulo
 **/
#define OP_MOD 12

/**
 * Increase
 **/
#define OP_INC 13

/**
 * Decrease
 **/
#define OP_DEC 14

/**
 * Logical AND
 **/
#define OP_AND 15

/**
 * Logical OR
 **/
#define OP_OR 16

/**
 * Logical XOR
 **/
#define OP_XOR 17

/**
 * Logical NEG
 **/
#define OP_NEG 18

/**
 * Logical NOT
 **/
#define OP_NOT 19



/* instruction flow control (jump if) */

/*
** OP_Jxx [offset] ..., value2, value1 => ...
*/

/**
 * JuMP
 **/
#define OP_JMP 20

/**
 * POP the last value and jump if this one is true
 **/
#define OP_POPCJMP 21


/* fast real-time evaluation for transition condition
   (continue if <> else HALT) */

/*
** OP_Cxx [] ..., value2, value1 => ...
*/

/**
 * Continue if EQual
 **/
#define OP_CEQ 22

/**
 * Continue if Not EQual
 **/
#define OP_CNEQ 23

/**
 * Continue if Regexp Match
 **/
#define OP_CRM 24

/**
 * Continue if Not Regexp Match
 **/
#define OP_CNRM 25

/**
 * Continue if Less Than
 **/
#define OP_CLT 26

/**
 * Continue if Greater Than
 **/
#define OP_CGT 27

/**
 * Continue if Less or Equal
 **/
#define OP_CLE 28

/**
 * Continue if Greater or Equal
 **/
#define OP_CGE 29

/**
 * Split a string into substrings using a regular expression.
 * The resultant substrings are pushed on the stack.
 * A complete regsplit operation consist of PUSHing the source
 * string and the split pattern, execute an OP_REGSPLIT operation
 * and POP resultant substrings into corresponding variables.
 * (ocode) OP_REGSPLIT []   ..., src, pat => ..., subn ... sub2, sub1
 * compil ex: /"abc:def"/"^\([a-z]*\):\([a-z]*\)"/$a/$b;
 * PUSH "abc:def"
 * PUSH "^\([a-z]*\):\([a-z]*\)"
 * REGSPLIT
 * POP $a
 * POP $b
 **/
#define OP_REGSPLIT 30


/**
 * Continue if Equal or Set Rigid/correlation Variable
 * CESV [idx]  ..., val ==> ...
 * if [idx] is defined then
 *   if val == [idx] then
 *     continue
 *   else
 *     stop
 * else
 * set [idx] to val
 * endif
 **/
#define OP_CESV 31

/* XXX Add Past Instruction */

#endif /* OVM_H */


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
