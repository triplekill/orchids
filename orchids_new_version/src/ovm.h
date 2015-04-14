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

#define PUSH_RETURN_TRUE(ctx)					\
  STACK_PUSH((ctx)->gc_ctx, (ctx)->ovm_stack,			\
	     (gc_header_t *)ctx->one)

#define PUSH_RETURN_FALSE(ctx)					\
  STACK_PUSH((ctx)->gc_ctx, (ctx)->ovm_stack,			\
	     (gc_header_t *)ctx->zero)

#define PUSH_RETURN_EMPTY(ctx)					\
  STACK_PUSH((ctx)->gc_ctx, (ctx)->ovm_stack,			\
	     (gc_header_t *)ctx->empty_string)

#define PUSH_VALUE(ctx,value) \
  STACK_PUSH((ctx)->gc_ctx, (ctx)->ovm_stack, (gc_header_t *)value)
#define POP_VALUE(ctx) \
  (ovm_var_t *)(STACK_POP((ctx)->gc_ctx, (ctx)->ovm_stack))

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
** () OP_CALL [p] ..., arg1, arg2, ..., argn => ..., retval
** call the p-th procedure. procedure implementation should only have a
** reference of the current stack.
**
*/

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
 * Push zero (false) onto the stack.
 **/
#define OP_PUSHZERO 4

/**
 * Push one (true) onto the stack.
 **/
#define OP_PUSHONE 5

/**
 * Push data from static zone onto the stack.
 **/
#define OP_PUSHSTATIC 6

/**
 * Put a field value on the stack.
 **/
#define OP_PUSHFIELD 7

/**
 * Trash the value on the top of the stack (free it if necessary)
 **/
#define OP_TRASH 8

/**
 * Call an ISSDL built-in function.
 **/
#define OP_CALL 9


/* Arithmetic */

/*
** OP_xxx [] ..., op2, op1 => ..., result
*/

/**
 * Addition.
 **/
#define OP_ADD 10

/**
 * Subtraction
 **/
#define OP_SUB 11
#define OP_OPP 12

/**
 * Multiplication
 **/
#define OP_MUL 13

/**
 * Division
 **/
#define OP_DIV 14

/**
 * Modulo
 **/
#define OP_MOD 15

/**
 * Increase
 **/
#define OP_INC 16

/**
 * Decrease
 **/
#define OP_DEC 17

/**
 * Logical AND
 **/
#define OP_AND 18

/**
 * Logical OR
 **/
#define OP_OR 19

/**
 * Logical XOR
 **/
#define OP_XOR 20

/**
 * Unused opcode. See OP_OPP for negation.
 **/
#define OP_NEG 21

/**
 * Logical NOT
 **/
#define OP_NOT 22



/* instruction flow control (jump if) */

/*
** OP_Jxx [offset] ..., value2, value1 => ...
*/

/**
 * JuMP
 **/
#define OP_JMP 23

/**
 * POP the last value and jump if this one is true
 **/
#define OP_POPCJMP 24


/* fast real-time evaluation for transition condition
   (continue if <> else HALT) */

/*
** OP_Cxx [] ..., value2, value1 => ...
*/

/**
 * Continue if EQual
 **/
#define OP_CEQ 25

/**
 * Continue if Not EQual
 **/
#define OP_CNEQ 26

/**
 * Continue if Regexp Match
 **/
#define OP_CRM 27

/**
 * Continue if Not Regexp Match
 **/
#define OP_CNRM 28

/**
 * Continue if Less Than
 **/
#define OP_CLT 29

/**
 * Continue if Greater Than
 **/
#define OP_CGT 30

/**
 * Continue if Less or Equal
 **/
#define OP_CLE 31

/**
 * Continue if Greater or Equal
 **/
#define OP_CGE 32

/**
 * Split a string into substrings using a regular expression.
 * The resulting substrings are pushed on the stack.
 * A complete regsplit operation consists of PUSHing the source
 * string and the split pattern, execute an OP_REGSPLIT operation
 * and POP the resulting substrings into corresponding variables.
 * (ocode) OP_REGSPLIT []   ..., src, pat => ..., subn ... sub2, sub1
 * compil ex: /"abc:def"/"^\([a-z]*\):\([a-z]*\)"/$a/$b;
 * PUSH "abc:def"
 * PUSH "^\([a-z]*\):\([a-z]*\)"
 * REGSPLIT
 * POP $a
 * POP $b
 **/
#define OP_REGSPLIT 33


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
#define OP_CESV 34

/**
 * Build an empty metaevent
 * OP_EMPTY_EVENT    ==> NULL
 **/
#define OP_EMPTY_EVENT 35

/**
 * Add binding to metaevent
 * OP_ADD_EVENT [field_id]   event, val ==> event'
 **/
#define OP_ADD_EVENT 36

/**
 * Filter a database db by equality and constant fields
 * OP_DB_FILTER [nfields_res] [nfields] [nconsts] [vals[0] ... vals[nfields-1]]
 * db, cst[0], ..., cst[nconsts-1] ==> new-db
 **/
#define OP_DB_FILTER 37

/**
 * Join two databases
 * OP_DB_JOIN [nfields1] [nfields2] [vals[0] ... vals[nfields2-1]]
 *  db1, db2 ==> new-db
 **/
#define OP_DB_JOIN 38

/**
 * Project a database onto subfields, possibly adding constant fields
 * OP_DB_PROJ [nfields_res] [nconsts] [vals[0] ... vals[nfields_res-1]]
 * db, cst[0], ..., cst[nconsts-1] ==> new-db
 **/
#define OP_DB_PROJ 39

/**
 * Take a union of databases indexed by another one
 * OP_DB_MAP [offset] [nfields]
 * followed by some OP_END terminated bytecode sequence (with the effect
 * ... val1, ..., valn => ..., dbb
 * where n==nfields)
 * will have the effect:
 * ..., db => ..., db'
 * where db' is the union of all the databases dbb, when
 * the tuple val1, ..., valn ranges over the database db;
 * then proceeds offset places later inside the bytecode (just like OP_JMP)
 **/
#define OP_DB_MAP 40


/**
 * Create a database with a single tuple
 * OP_DB_SINGLE [nconsts]
 * cst[0], ..., cst[nconsts-1] ==> new-db
 **/
#define OP_DB_SINGLE 41

/**
 * Copy a value already on the stack
 **/
#define OP_DUP 42

/**
 * Push NULL
 **/
#define OP_PUSHNULL 43

/**
 ** Number of total opcodes.
 **/
#define OPCODE_NUM 44

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
