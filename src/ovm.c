/**
 ** @file ovm.c
 ** Orchids 'virtual machine'
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.3
 ** @ingroup ovm
 **
 ** @date  Started on: Wed Mar 12 12:14:53 2003
 ** @date Last update: Tue Dec  6 17:02:22 2005
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "orchids.h"
#include "stack.h"

#include "ovm.h"

#include "ovm_private.h"


int
ovm_exec(orchids_t *ctx, state_instance_t *s, bytecode_t *bytecode)
{
  isn_param_t isn_param;
  int ret;

  isn_param.bytecode = bytecode;
  isn_param.ip = bytecode;
  isn_param.state = s;
  isn_param.ctx = ctx;

  while (*isn_param.ip != OP_END) {
    if (*isn_param.ip >= OPCODE_NUM) {
      DebugLog(DF_OVM, DS_ERROR, "unknown opcode 0x%02lx\n", *isn_param.ip);
      return (1);
    }

    ret = ops_g[ *isn_param.ip ].insn(&isn_param);

    if (ret) {
      DebugLog(DF_OVM, DS_INFO, "OVM exeption\n");
      return (ret);
    }
  }

  return (0);
}


void
fprintf_bytecode(FILE *fp, bytecode_t *bytecode)
{
  bytecode_t *code;
  int offset;

  offset = 0;
  code = bytecode;
  while (1)
    {
      switch (*code)
        {
        case OP_END:
          fprintf(fp, "0x%04x: %08x             | end\n", offset, OP_END);
          return ;

        case OP_NOP:
          fprintf(fp, "0x%04x: %08x             | nop\n", offset, OP_NOP);
          offset += 1;
          break ;

        case OP_PUSH:
          fprintf(fp, "0x%04x: %08x %08lx    | push [%lu]\n",
                  offset, OP_PUSH, code[1], code[1]);
          offset += 2;
          break ;

        case OP_POP:
          fprintf(fp, "0x%04x: %08x %08lx    | pop [%lu]\n",
                  offset, OP_POP, code[1], code[1]);
          offset += 2;
          break ;

        case OP_PUSHSTATIC:
          fprintf(fp, "0x%04x: %08x %08lx    | pushstatic [%lu]\n",
                  offset, OP_PUSHSTATIC, code[1], code[1]);
          offset += 2;
          break ;

        case OP_PUSHFIELD:
          fprintf(fp, "0x%04x: %08x %08lx    | pushfield [%lu]\n",
                  offset, OP_PUSHFIELD, code[1], code[1]);
          offset += 2;
          break ;

        case OP_ALOAD:
          fprintf(fp, "OP_ALOAD not yet implemented\n");
          return ;

        case OP_ASTORE:
          fprintf(fp, "OP_ASTORE not yet implemented\n");
          return ;

        case OP_ALENGTH:
          fprintf(fp, "OP_ALENGTH not yet implemented\n");
          return ;

        case OP_CALL:
          fprintf(fp, "0x%04x: %08x %08lx    | call [%lu]\n",
                  offset, OP_CALL, code[1], code[1]);
          offset += 2;
          break ;

        case OP_ADD:
          fprintf(fp, "0x%04x: %08lx             | add\n", offset, *code);
          offset += 1;
          break ;

        case OP_SUB:
          fprintf(fp, "0x%04x: %08lx             | add\n", offset, *code);
          offset += 1;
          break ;

        case OP_MUL:
          fprintf(fp, "0x%04x: %08lx             | mul\n", offset, *code);
          offset += 1;
          break ;

        case OP_DIV:
          fprintf(fp, "0x%04x: %08lx             | div\n", offset, *code);
          offset += 1;
          break ;

        case OP_MOD:
          fprintf(fp, "0x%04x: %08lx             | mod\n", offset, *code);
          offset += 1;
          break ;

        case OP_INC:
          fprintf(fp, "not yet implemented\n");
          return ;

        case OP_DEC:
          fprintf(fp, "not yet implemented\n");
          return ;

        case OP_AND:
          fprintf(fp, "not yet implemented\n");
          return ;

        case OP_OR:
          fprintf(fp, "not yet implemented\n");
          return ;

        case OP_XOR:
          fprintf(fp, "not yet implemented\n");
          return ;

        case OP_NEG:
          fprintf(fp, "not yet implemented\n");
          return ;

        case OP_NOT:
          fprintf(fp, "not yet implemented\n");
          return ;

        case OP_JMP:
          fprintf(fp, "not yet implemented\n");
          return ;

        case OP_JE:
          fprintf(fp, "not yet implemented\n");
          return ;

        case OP_JNE:
          fprintf(fp, "not yet implemented\n");
          return ;

        case OP_JRM:
          fprintf(fp, "not yet implemented\n");
          return ;

        case OP_JNRM:
          fprintf(fp, "not yet implemented\n");
          return ;

        case OP_JLT:
          fprintf(fp, "not yet implemented\n");
          return ;

        case OP_JGT:
          fprintf(fp, "not yet implemented\n");
          return ;

        case OP_JLE:
          fprintf(fp, "not yet implemented\n");
          return ;

        case OP_JGE:
          fprintf(fp, "not yet implemented\n");
          return ;

        case OP_CEQ:
          fprintf(fp, "0x%04x: %08x             | ceq\n", offset, OP_CEQ);
          offset += 1;
          break ;

        case OP_CNEQ:
          fprintf(fp, "0x%04x: %08x             | cneq\n", offset, OP_CNEQ);
          offset += 1;
          break ;

        case OP_CRM:
          fprintf(fp, "0x%04x: %08x             | crm\n", offset, OP_CRM);
          offset += 1;
          break ;

        case OP_CNRM:
          fprintf(fp, "0x%04x: %08x             | cnrm\n", offset, OP_CNRM);
          offset += 1;
          break ;

        case OP_CLT:
          fprintf(fp, "0x%04x: %08x             | clt\n", offset, OP_CLT);
          offset += 1;
          break ;

        case OP_CGT:
          fprintf(fp, "0x%04x: %08x             | cgt\n", offset, OP_CGT);
          offset += 1;
          break ;

        case OP_CLE:
          fprintf(fp, "0x%04x: %08x             | cle\n", offset, OP_CLE);
          offset += 1;
          break ;

        case OP_CGE:
          fprintf(fp, "0x%04x: %08x             | cge\n", offset, OP_CGE);
          offset += 1;
          break ;

        case OP_REGSPLIT:
          fprintf(fp, "0x%04x: %08x             | regsplit\n", offset, OP_REGSPLIT);
          offset += 1;
          break ;

        case OP_CESV:
          fprintf(fp, "0x%04x: %08x             | cesv\n", offset, OP_REGSPLIT);
          offset += 1;
          break ;

        default:
          DebugLog(DF_OVM, DS_ERROR, "unknown opcode %lu\n", *code);
          return ;
          /* exit(EXIT_FAILURE); */
        }
      code = bytecode + offset;
    }

  return ;
}

void
fprintf_bytecode_short(FILE *fp, bytecode_t *bytecode)
{
  bytecode_t *code;
  int offset;

  offset = 0;
  code = bytecode;
  while (1)
    {
      switch (*code)
        {
        case OP_END:
          fprintf(fp, "%04x: %02x       | end\n", offset, OP_END);
          return ;

        case OP_NOP:
          fprintf(fp, "%04x: %02x       | nop\n", offset, OP_NOP);
          offset += 1;
          break ;

        case OP_PUSH:
          fprintf(fp, "%04x: %02x %02lx    | push [%lu]\n",
                  offset, OP_PUSH, code[1], code[1]);
          offset += 2;
          break ;

        case OP_POP:
          fprintf(fp, "%04x: %02x %02lx    | pop [%lu]\n",
                  offset, OP_POP, code[1], code[1]);
          offset += 2;
          break ;

        case OP_PUSHSTATIC:
          fprintf(fp, "%04x: %02x %02lx    | pushstatic [%lu]\n",
                  offset, OP_PUSHSTATIC, code[1], code[1]);
          offset += 2;
          break ;

        case OP_PUSHFIELD:
          fprintf(fp, "%04x: %02x %02lx    | pushfield [%lu]\n",
                  offset, OP_PUSHFIELD, code[1], code[1]);
          offset += 2;
          break ;

        case OP_ALOAD:
          fprintf(fp, "OP_ALOAD not yet implemented\n");
          return ;

        case OP_ASTORE:
          fprintf(fp, "OP_ASTORE not yet implemented\n");
          return ;

        case OP_ALENGTH:
          fprintf(fp, "OP_ALENGTH not yet implemented\n");
          return ;

        case OP_CALL:
          fprintf(fp, "%04x: %02x %02lx    | call [%lu]\n",
                  offset, OP_CALL, code[1], code[1]);
          offset += 2;
          break ;

        case OP_ADD:
          fprintf(fp, "%04x: %02lx       | add\n", offset, *code);
          offset += 1;
          break ;

        case OP_SUB:
          fprintf(fp, "%04x: %02lx       | sub\n", offset, *code);
          offset += 1;
          break ;

        case OP_MUL:
          fprintf(fp, "%04x: %02lx       | mul\n", offset, *code);
          offset += 1;
          break ;

        case OP_DIV:
          fprintf(fp, "%04x: %02lx       | div\n", offset, *code);
          offset += 1;
          break ;

        case OP_MOD:
          fprintf(fp, "%04x: %02lx       | mod\n", offset, *code);
          offset += 1;
          break ;

        case OP_INC:
          fprintf(fp, "not yet implemented\n");
          return ;

        case OP_DEC:
          fprintf(fp, "not yet implemented\n");
          return ;

        case OP_AND:
          fprintf(fp, "not yet implemented\n");
          return ;

        case OP_OR:
          fprintf(fp, "not yet implemented\n");
          return ;

        case OP_XOR:
          fprintf(fp, "not yet implemented\n");
          return ;

        case OP_NEG:
          fprintf(fp, "not yet implemented\n");
          return ;

        case OP_NOT:
          fprintf(fp, "not yet implemented\n");
          return ;

        case OP_JMP:
          fprintf(fp, "not yet implemented\n");
          return ;

        case OP_JE:
          fprintf(fp, "not yet implemented\n");
          return ;

        case OP_JNE:
          fprintf(fp, "not yet implemented\n");
          return ;

        case OP_JRM:
          fprintf(fp, "not yet implemented\n");
          return ;

        case OP_JNRM:
          fprintf(fp, "not yet implemented\n");
          return ;

        case OP_JLT:
          fprintf(fp, "not yet implemented\n");
          return ;

        case OP_JGT:
          fprintf(fp, "not yet implemented\n");
          return ;

        case OP_JLE:
          fprintf(fp, "not yet implemented\n");
          return ;

        case OP_JGE:
          fprintf(fp, "not yet implemented\n");
          return ;

        case OP_CEQ:
          fprintf(fp, "%04x: %02x       | ceq\n", offset, OP_CEQ);
          offset += 1;
          break ;

        case OP_CNEQ:
          fprintf(fp, "%04x: %02x       | cneq\n", offset, OP_CNEQ);
          offset += 1;
          break ;

        case OP_CRM:
          fprintf(fp, "%04x: %02x       | crm\n", offset, OP_CRM);
          offset += 1;
          break ;

        case OP_CNRM:
          fprintf(fp, "%04x: %02x       | cnrm\n", offset, OP_CNRM);
          offset += 1;
          break ;

        case OP_CLT:
          fprintf(fp, "%04x: %02x       | clt\n", offset, OP_CLT);
          offset += 1;
          break ;

        case OP_CGT:
          fprintf(fp, "%04x: %02x       | cgt\n", offset, OP_CGT);
          offset += 1;
          break ;

        case OP_CLE:
          fprintf(fp, "%04x: %02x       | cle\n", offset, OP_CLE);
          offset += 1;
          break ;

        case OP_CGE:
          fprintf(fp, "%04x: %02x       | cge\n", offset, OP_CGE);
          offset += 1;
          break ;

        case OP_REGSPLIT:
          fprintf(fp, "%04x: %02x       | regsplit\n", offset, OP_REGSPLIT);
          offset += 1;
          break ;

        case OP_CESV:
          fprintf(fp, "%04x: %02x       | cesv\n", offset, OP_REGSPLIT);
          offset += 1;
          break ;

        default:
          DebugLog(DF_OVM, DS_ERROR, "unknown opcode %lu\n", *code);
          return ;
          /* exit(EXIT_FAILURE); */
        }
      code = bytecode + offset;
    }

  return ;
}

void
fprintf_bytecode_dump(FILE *fp, bytecode_t *code)
{
  fprintf(fp, "bytecode dump: ");
  while (*code)
    fprintf(fp, "0x%02lX ", *code++);
  fprintf(fp, "\n");
}



static int
ovm_nop(isn_param_t *param)
{
  DebugLog(DF_OVM, DS_ERROR,
           "OP_NOP (why ? compiler doesn't produce OP_NOPs !)\n");

  param->ip += 1;

  return (0);
}

static int
ovm_push(isn_param_t *param)
{
  DebugLog(DF_OVM, DS_DEBUG,
           "OP_PUSH [%02lx] ($%s)\n",
            param->ip[1], param->state->state->rule->var_name[ param->ip[1] ]);

  /* XXX: optimize env checking and resolution */
  if (param->state->current_env && param->state->current_env[ param->ip[1] ])
    stack_push(param->ctx->ovm_stack, param->state->current_env[ param->ip[1] ]);
  else if (param->state->inherit_env && param->state->inherit_env[ param->ip[1] ])
    stack_push(param->ctx->ovm_stack, param->state->inherit_env[ param->ip[1] ]);
  else {
    DebugLog(DF_OVM, DS_ERROR, "env error\n");
    return (-1);
  }
  param->ip += 2;

  return (0);
}

static int
ovm_pop(isn_param_t *param)
{
  ovm_var_t **var;

  DebugLog(DF_OVM, DS_DEBUG, "OP_POP [%02lx] ($%s)\n",
            param->ip[1],
            param->state->state->rule->var_name[ param->ip[1] ]);

  var = &param->state->current_env[ param->ip[1] ];

  /* if a temp value is already bounded to this var, free it. */
  if (*var && CAN_FREE_VAR(*var)) {
    Xfree(*var);
  }

  *var = stack_pop(param->ctx->ovm_stack);

  /* mark value as bounded */
  if (*var)
    FLAGS(*var) &= ~TYPE_NOTBOUND;

  /* XXX: if *var can be freed, we must clone it. (ref can be dup) */

  param->ip += 2;

  return (0);
}

static int
ovm_pushstatic(isn_param_t *param)
{
  DebugLog(DF_OVM, DS_DEBUG, "OP_PUSHSTATIC [%02lx]\n", param->ip[1]);

  /* XXX: check boundary ?!... */
  stack_push(param->ctx->ovm_stack,
             param->state->state->rule->static_env[ param->ip[1] ]);
  param->ip += 2;

  return (0);
}

static int
ovm_pushfield(isn_param_t *param)
{
  DebugLog(DF_OVM, DS_DEBUG,
           "OP_PUSHFIELD [%02lx] (.%s)\n",
           param->ip[1], param->ctx->global_fields[ param->ip[1] ].name);

  /* XXX: check boundary ?!... */
  stack_push(param->ctx->ovm_stack,
             param->ctx->global_fields[ param->ip[1] ].val);
  param->ip += 2;

  return (0);
}

static int
ovm_aload(isn_param_t *param)
{
  DPRINTF( ("OP_ALOAD\n") );
  DPRINTF( ("not yet implemented\n") );

  return (-1);
}

static int
ovm_astore(isn_param_t *param)
{
  DPRINTF( ("OP_LOAD\n") );
  DPRINTF( ("not yet implemented\n") );

  return (-1);
}

static int
ovm_alength(isn_param_t *param)
{
  DPRINTF( ("OP_ALENGTH\n") );
  DPRINTF( ("not yet implemented\n") );

  return (-1);
}

static int
ovm_call(isn_param_t *param)
{
  DebugLog(DF_OVM, DS_DEBUG, "OP_CALL [%02lx] (%s)\n", param->ip[1],
            param->ctx->vm_func_tbl[ param->ip[1] ].name);
  /* check call table boundary */
  param->ctx->vm_func_tbl[ param->ip[1] ].func(param->ctx, param->state);
  param->ip += 2;

  return (0);
}

static int
ovm_add(isn_param_t *param)
{
  ovm_var_t *op1;
  ovm_var_t *op2;
  ovm_var_t *res;

  DebugLog(DF_OVM, DS_DEBUG, "OP_ADD\n");

  op2 = stack_pop(param->ctx->ovm_stack);
  op1 = stack_pop(param->ctx->ovm_stack);

  /* XXX: REMOVE THIS */
  if ((op1 == NULL) || (op2 == NULL)) {
    DebugLog(DF_OVM, DS_DEBUG, "op error\n");
    return (1);
  }

  res = issdl_add(op1, op2);
  stack_push(param->ctx->ovm_stack, res);

  /* of op1 and/or op2 was tmpvars, free them */
  if ( IS_NOT_BOUND(op1) ) {
    DebugLog(DF_OVM, DS_TRACE, "OP_ADD: free operand 1\n");
    Xfree(op1);
  }
  if ( IS_NOT_BOUND(op2) ) {
    DebugLog(DF_OVM, DS_TRACE, "OP_ADD: free operand 2\n");
    Xfree(op2);
  }

  param->ip += 1;

  return (0);
}

#if 0
static int
ovm_add_old(isn_param_t *param)
{
  ovm_var_t *op1;
  ovm_var_t *op2;
  ovm_var_t *res;

  DPRINTF( ("OP_ADD\n") );

  op2 = stack_pop(param->ctx->ovm_stack);
  op1 = stack_pop(param->ctx->ovm_stack);

  /* XXX: REMOVE THIS */
  if ((op1 == NULL) || (op2 == NULL))
    {
      DPRINTF( ("op error\n") );
      return (1);
    }

  if (op1->type != op2->type)
    {
      DPRINTF( ("incompatible type\n") );
      return (1);
    }

  /* switch/case on type
  ** implement special optimized comparisons
  ** and default will be a regular memcmp() */
  switch (op1->type)
    {
    case T_INT:
      res = (ovm_var_t *) new_integer();
      INT(res) = INT(op1) + INT(op2);
      stack_push(param->ctx->ovm_stack, res);
      break ;

    case T_STR:
      res = (ovm_var_t *) new_string(STRLEN(op1) + STRLEN(op2));
      memcpy(STR(res), STR(op1), STRLEN(op1));
      memcpy(STR(res) + STRLEN(op1), STR(op2), STRLEN(op2));
      stack_push(param->ctx->ovm_stack, res);
      break;

    default:
      DPRINTF( ("type not implemented\n") );
    }

  /* of op1 and/or op2 was tmpvars, free them */
  if ( IS_NOT_BOUND(op1) )
    {
      DPRINTF( ("free(op1);") );
      Xfree(op1);
    }

  if ( IS_NOT_BOUND(op2) )
    {
      DPRINTF( ("free(op2);") );
      Xfree(op2);
    }

  param->ip += 1;

  return (0);
}
#endif

static int
ovm_sub(isn_param_t *param)
{
  ovm_var_t *op1;
  ovm_var_t *op2;
  ovm_var_t *res;

  DebugLog(DF_OVM, DS_DEBUG,"OP_SUB\n");

  op2 = stack_pop(param->ctx->ovm_stack);
  op1 = stack_pop(param->ctx->ovm_stack);

  /* XXX: REMOVE THIS */
  if ((op1 == NULL) || (op2 == NULL)) {
    DebugLog(DF_OVM, DS_DEBUG,"op error\n");
    return (1);
  }

  res = issdl_sub(op1, op2);
  stack_push(param->ctx->ovm_stack, res);

  /* of op1 and/or op2 was tmpvars, free them */
  if ( IS_NOT_BOUND(op1) ) {
      DebugLog(DF_OVM, DS_DEBUG,"free(op1);\n");
      Xfree(op1);
  }

  if ( IS_NOT_BOUND(op2) ) {
    DebugLog(DF_OVM, DS_DEBUG, "free(op2);\n");
    Xfree(op2);
  }

  param->ip += 1;

  return (0);
}

static int
ovm_mul(isn_param_t *param)
{
  ovm_var_t *op1;
  ovm_var_t *op2;
  ovm_var_t *res;

  DPRINTF( ("OP_MUL\n") );

  op2 = stack_pop(param->ctx->ovm_stack);
  op1 = stack_pop(param->ctx->ovm_stack);

  /* XXX: REMOVE THIS */
  if ((op1 == NULL) || (op2 == NULL)) {
    DebugLog(DF_OVM, DS_DEBUG, "op error\n");
    return (1);
  }

  res = issdl_mul(op1, op2);
  stack_push(param->ctx->ovm_stack, res);

  /* of op1 and/or op2 was tmpvars, free them */
  if ( IS_NOT_BOUND(op1) ) {
      DebugLog(DF_OVM, DS_DEBUG, "free(op1);\n");
      Xfree(op1);
  }

  if ( IS_NOT_BOUND(op2) ) {
    DebugLog(DF_OVM, DS_DEBUG, "free(op2);\n");
    Xfree(op2);
  }

  param->ip += 1;

  return (0);
}

static int
ovm_div(isn_param_t *param)
{
  ovm_var_t *op1;
  ovm_var_t *op2;
  ovm_var_t *res;

  DebugLog(DF_OVM, DS_DEBUG, "OP_DIV\n");

  op2 = stack_pop(param->ctx->ovm_stack);
  op1 = stack_pop(param->ctx->ovm_stack);

  /* XXX: REMOVE THIS */
  if ((op1 == NULL) || (op2 == NULL)) {
    DebugLog(DF_OVM, DS_DEBUG, "op error\n");
    return (1);
  }

  res = issdl_div(op1, op2);
  stack_push(param->ctx->ovm_stack, res);

  /* of op1 and/or op2 was tmpvars, free them */
  if ( IS_NOT_BOUND(op1) ) {
      DebugLog(DF_OVM, DS_DEBUG, "free(op1);\n");
      Xfree(op1);
  }

  if ( IS_NOT_BOUND(op2) ) {
    DebugLog(DF_OVM, DS_DEBUG, "free(op2);\n");
    Xfree(op2);
  }

  param->ip += 1;

  return (0);
}

static int
ovm_mod(isn_param_t *param)
{
  ovm_var_t *op1;
  ovm_var_t *op2;
  ovm_var_t *res;

  DebugLog(DF_OVM, DS_DEBUG, "OP_MOD\n");

  op2 = stack_pop(param->ctx->ovm_stack);
  op1 = stack_pop(param->ctx->ovm_stack);

  /* XXX: REMOVE THIS */
  if ((op1 == NULL) || (op2 == NULL)) {
    DebugLog(DF_OVM, DS_DEBUG, "op error\n");
    return (1);
  }

  res = issdl_mod(op1, op2);
  stack_push(param->ctx->ovm_stack, res);

  /* of op1 and/or op2 was tmpvars, free them */
  if ( IS_NOT_BOUND(op1) ) {
    DebugLog(DF_OVM, DS_DEBUG, "free(op1);\n");
    Xfree(op1);
  }

  if ( IS_NOT_BOUND(op2) ) {
    DebugLog(DF_OVM, DS_DEBUG, "free(op2);\n");
    Xfree(op2);
  }

  param->ip += 1;

  return (0);
}

static int
ovm_inc(isn_param_t *param)
{
  DPRINTF( ("OP_INC\n") );
  DPRINTF( ("not yet implemented\n") );

  return (-1);
}

static int
ovm_dec(isn_param_t *param)
{
  DPRINTF( ("OP_DEC\n") );
  DPRINTF( ("not yet implemented\n") );

  return (-1);
}

static int
ovm_and(isn_param_t *param)
{
  DPRINTF( ("OP_AND\n") );
  DPRINTF( ("not yet implemented\n") );

  return (-1);
}

static int
ovm_or(isn_param_t *param)
{
  DPRINTF( ("OP_OR\n") );
  DPRINTF( ("not yet implemented\n") );

  return (-1);
}

static int
ovm_xor(isn_param_t *param)
{
  DPRINTF( ("OP_XOR\n") );
  DPRINTF( ("not yet implemented\n") );

  return (-1);
}

static int
ovm_neg(isn_param_t *param)
{
  DPRINTF( ("OP_NEG\n") );
  DPRINTF( ("not yet implemented\n") );

  return (-1);
}

static int
ovm_not(isn_param_t *param)
{
  DPRINTF( ("OP_NOT\n") );
  DPRINTF( ("not yet implemented\n") );

  return (-1);
}

static int
ovm_jmp(isn_param_t *param)
{
  DPRINTF( ("OP_JMP\n") );
  DPRINTF( ("not yet implemented\n") );

  return (-1);
}

static int
ovm_je(isn_param_t *param)
{
  DPRINTF( ("OP_JE\n") );
  DPRINTF( ("not yet implemented\n") );

  return (-1);
}

static int
ovm_jne(isn_param_t *param)
{
  DPRINTF( ("OP_JNE\n") );
  DPRINTF( ("not yet implemented\n") );

  return (-1);
}

static int
ovm_jrm(isn_param_t *param)
{
  DPRINTF( ("OP_JRM\n") );
  DPRINTF( ("not yet implemented\n") );

  return (-1);
}

static int
ovm_jnrm(isn_param_t *param)
{
  DPRINTF( ("OP_JNRM\n") );
  DPRINTF( ("not yet implemented\n") );

  return (-1);
}

static int
ovm_jlt(isn_param_t *param)
{
  DPRINTF( ("OP_JLT\n") );
  DPRINTF( ("not yet implemented\n") );

  return (-1);
}

static int
ovm_jgt(isn_param_t *param)
{
  DPRINTF( ("OP_JGT\n") );
  DPRINTF( ("not yet implemented\n") );

  return (-1);
}

static int
ovm_jle(isn_param_t *param)
{
  DPRINTF( ("OP_JLE\n") );
  DPRINTF( ("not yet implemented\n") );

  return (-1);
}

static int
ovm_jge(isn_param_t *param)
{
  DPRINTF( ("OP_JGE\n") );
  DPRINTF( ("not yet implemented\n") );

  return (-1);
}

static int
ovm_ceq(isn_param_t *param)
{
  ovm_var_t *op1;
  ovm_var_t *op2;
  int ret;

  DebugLog(DF_OVM, DS_DEBUG, "OP_CEQ\n");

  op1 = stack_pop(param->ctx->ovm_stack);
  op2 = stack_pop(param->ctx->ovm_stack);

  if ((op1 == NULL) || (op2 == NULL)) {
    DebugLog(DF_OVM, DS_ERROR, "op error\n");
    return (1);
  }

  ret = issdl_cmp(op1, op2);

  /* of op1 and/or op2 was tmpvars, free them */
  if ( IS_NOT_BOUND(op1) ) {
    DebugLog(DF_OVM, DS_TRACE, "OP_CEQ: free operand 1\n");
    Xfree(op1);
  }

  if ( IS_NOT_BOUND(op2) ) {
    DebugLog(DF_OVM, DS_TRACE, "OP_CEQ: free operand 2\n");
    Xfree(op2);
  }

  if (ret)
    return (1);

  param->ip += 1;

  DebugLog(DF_OVM, DS_DEBUG, "OP_CEQ (true)\n");
  
  return (0);
}

#if 0
static int
ovm_ceq_old(isn_param_t *param)
{
  ovm_var_t *op1;
  ovm_var_t *op2;

  DPRINTF( ("OP_CEQ\n") );

  op2 = stack_pop(param->ctx->ovm_stack);
  op1 = stack_pop(param->ctx->ovm_stack);

  if ((op1 == NULL) || (op2 == NULL))
    {
      DPRINTF( ("op error\n") );
      return (1);
    }

  /* switch on operand1 type */
  switch (op1->type)
    {
    case T_INT:
      if (op2->type != T_INT)
        return (1);
      if (INT(op1) != INT(op2))
        return (1);
      break;
      
    case T_STR:
      if (op2->type == T_STR)
        {
          if (STRLEN(op1) != STRLEN(op2))
            return (1);
          if (strncmp(STR(op1), STR(op2), STRLEN(op1)))
            return (1);
        }
      else if (op2->type == T_VSTR)
        {
          if (STRLEN(op1) != VSTRLEN(op2))
            return (1);
          if (strncmp(STR(op1), VSTR(op2), STRLEN(op1)))
            return (1);
        }
      else
        return (1);
      break;
      
    case T_VSTR:
      if (op2->type == T_STR)
        {
          if (VSTRLEN(op1) != STRLEN(op2))
            return (1);
          if (strncmp(VSTR(op1), STR(op2), VSTRLEN(op1)))
            return (1);
        }
      else if (op2->type == T_VSTR)
        {
          if (VSTRLEN(op1) != VSTRLEN(op2))
            return (1);
          if (strncmp(VSTR(op1), VSTR(op2), VSTRLEN(op1)))
            return (1);
        }
      else
        return (1);
      break;
      
    default:
      do {
        size_t op1_len, op2_len;
        
        if (op1->type != op2->type)
          return (1);
        op1_len = issdl_get_data_len(op1);
        op2_len = issdl_get_data_len(op2);
        if (op1_len != op2_len)
          return (1);
        if (memcmp(issdl_get_data(op1),
                   issdl_get_data(op2),
                   op1_len))
          return (1);
      } while (0);
    }

  param->ip += 1;

  DPRINTF( ("OP_CEQ (true)\n") );
  
  return (0);
}
#endif 

static int
ovm_cneq(isn_param_t *param)
{
  ovm_var_t *op1;
  ovm_var_t *op2;
  int ret;

  DebugLog(DF_OVM, DS_DEBUG, "OP_CNEQ\n");

  op2 = stack_pop(param->ctx->ovm_stack);
  op1 = stack_pop(param->ctx->ovm_stack);

  if ((op1 == NULL) || (op2 == NULL)) {
    DebugLog(DF_OVM, DS_DEBUG, "op error\n");
    return (1);
  }

  ret = issdl_cmp(op1, op2);

  if ( IS_NOT_BOUND(op1) ) {
    DebugLog(DF_OVM, DS_TRACE, "OP_CNEQ: free operand 1\n");
    Xfree(op1);
  }

  if ( IS_NOT_BOUND(op2) ) {
    DebugLog(DF_OVM, DS_TRACE, "OP_CNEQ: free operand 2\n");
    Xfree(op2);
  }

  if (!ret)
    return (1);

  param->ip += 1;

  DebugLog(DF_OVM, DS_DEBUG, "OP_CNEQ (true)\n");

  return (0);
}

#if 0
static int
ovm_cneq_old(isn_param_t *param)
{
  ovm_var_t *op1;
  ovm_var_t *op2;

  DPRINTF( ("OP_CNEQ\n") );

  op2 = stack_pop(param->ctx->ovm_stack);
  op1 = stack_pop(param->ctx->ovm_stack);
    
  /* XXX: REMOVE THIS */
  if ((op1 == NULL) || (op2 == NULL))
    {
      DPRINTF( ("op error\n") );
      return (1);
    }

  /* special comparisons */
  switch (op1->type)
    {
    case T_INT:
      /* check type of op2 here */
      if (INT(op1) == INT(op2))
        return (1);
      break;

    default:
      DPRINTF( ("type comparison not implemented\n") );
    }

  /* of op1 and/or op2 was tmpvars, free them */

  param->ip += 1;

  DPRINTF( ("OP_CNEQ (true)\n") );

  return (0);
}
#endif

static int
ovm_crm(isn_param_t *param)
{
  ovm_var_t *string;
  ovm_var_t *regex;
  char *s;
  int ret;

  DebugLog(DF_OVM, DS_DEBUG, "OP_CRM\n");

  regex = stack_pop(param->ctx->ovm_stack);
  string = stack_pop(param->ctx->ovm_stack);

  if ((regex == NULL) || (string == NULL)) {
    DebugLog(DF_OVM, DS_ERROR, "op error\n");
    return (1);
  }

  if ((TYPE(regex) != T_REGEX) ||
      ((TYPE(string) != T_STR) && TYPE(string) != T_VSTR)) {
    DebugLog(DF_OVM, DS_DEBUG, "op type error\n");
    FREE_IF_NEEDED(string);
    FREE_IF_NEEDED(regex);
    return (1);
  }

  s = ovm_strdup(string);

  DebugLog(DF_OVM, DS_DEBUG, "OP_CRM str=\"%s\" regex=\"%s\"\n", s, REGEXSTR(regex));

  ret = regexec(&REGEX(regex), s, 0, NULL, 0);
  Xfree(s);
  if (ret == REG_NOMATCH) {
    DebugLog(DF_OVM, DS_DEBUG, "OP_CRM (false)\n");
    FREE_IF_NEEDED(string);
    FREE_IF_NEEDED(regex);
    return (1);
  }
  else if (ret != 0) {
    char err_buf[64];
    regerror(ret, &(REGEX(regex)), err_buf, sizeof (err_buf));
    DebugLog(DF_OVM, DS_DEBUG, "regexec error (%s)\n", err_buf);
  }

  DebugLog(DF_OVM, DS_DEBUG, "OP_CRM (true)\n");

  param->ip += 1;

  FREE_IF_NEEDED(string);
  FREE_IF_NEEDED(regex);

  return (0);
}

static int
ovm_cnrm(isn_param_t *param)
{
  ovm_var_t *string;
  ovm_var_t *regex;
  char *s;
  int ret;

  DebugLog(DF_OVM, DS_DEBUG, "OP_CNRM\n");

  regex = stack_pop(param->ctx->ovm_stack);
  string = stack_pop(param->ctx->ovm_stack);

  if ((regex == NULL) || (string == NULL)) {
    DebugLog(DF_OVM, DS_ERROR, "op error\n");
    return (1);
  }

  if ((TYPE(regex) != T_REGEX) ||
      ((TYPE(string) != T_STR) && TYPE(string) != T_VSTR)) {
    DebugLog(DF_OVM, DS_DEBUG, "op type error\n");
    FREE_IF_NEEDED(string);
    FREE_IF_NEEDED(regex);
    return (1);
  }

  s = ovm_strdup(string);

  DebugLog(DF_OVM, DS_DEBUG, "OP_CNRM str=\"%s\" regex=\"%s\"\n", s, REGEXSTR(regex));

  ret = regexec(&REGEX(regex), s, 0, NULL, 0);
  Xfree(s);
  if (ret == 0) {
    DebugLog(DF_OVM, DS_DEBUG, "OP_CNRM (false)\n");
    FREE_IF_NEEDED(string);
    FREE_IF_NEEDED(regex);
    return (1);
  }
  else if (ret != REG_NOMATCH) {
    char err_buf[64];
    regerror(ret, &(REGEX(regex)), err_buf, sizeof (err_buf));
    DebugLog(DF_OVM, DS_DEBUG, "regexec error (%s)\n", err_buf);
  }

  DebugLog(DF_OVM, DS_DEBUG, "OP_CNRM (true)\n");

  param->ip += 1;
  FREE_IF_NEEDED(string);
  FREE_IF_NEEDED(regex);

  return (0);
}

static int
ovm_clt(isn_param_t *param)
{
  ovm_var_t *op1;
  ovm_var_t *op2;
  int ret;

  DebugLog(DF_OVM, DS_DEBUG, "OP_CLT\n");

  op2 = stack_pop(param->ctx->ovm_stack);
  op1 = stack_pop(param->ctx->ovm_stack);

  if ((op1 == NULL) || (op2 == NULL)) {
    DebugLog(DF_OVM, DS_ERROR, "op error\n");
    return (1);
  }

  ret = issdl_cmp(op1, op2);

  /* of op1 and/or op2 was tmpvars, free them */
  if ( IS_NOT_BOUND(op1) ) {
    DebugLog(DF_OVM, DS_TRACE, "OP_CLT: free operand 1\n");
    Xfree(op1);
  }

  if ( IS_NOT_BOUND(op2) ) {
    DebugLog(DF_OVM, DS_TRACE, "OP_CLT: free operand 2\n");
    Xfree(op2);
  }

  if (ret >= 0) {
    DebugLog(DF_OVM, DS_DEBUG, "OP_CLT (false ret=%i)\n", ret);
    return (1);
  }

  param->ip += 1;

  DebugLog(DF_OVM, DS_DEBUG, "OP_CLT (true ret=%i)\n", ret);
  
  return (0);
}

static int
ovm_cgt(isn_param_t *param)
{
  ovm_var_t *op1;
  ovm_var_t *op2;
  int ret;

  DebugLog(DF_OVM, DS_DEBUG, "OP_CGT\n");

  op2 = stack_pop(param->ctx->ovm_stack);
  op1 = stack_pop(param->ctx->ovm_stack);

  if ((op1 == NULL) || (op2 == NULL)) {
    DebugLog(DF_OVM, DS_ERROR, "op error\n");
    return (1);
  }

  ret = issdl_cmp(op1, op2);

  /* of op1 and/or op2 was tmpvars, free them */
  if ( IS_NOT_BOUND(op1) ) {
    DebugLog(DF_OVM, DS_TRACE, "OP_CGT: free operand 1\n");
    Xfree(op1);
  }

  if ( IS_NOT_BOUND(op2) ) {
    DebugLog(DF_OVM, DS_TRACE, "OP_CGT: free operand 2\n");
    Xfree(op2);
  }

  if (ret <= 0) {
    DebugLog(DF_OVM, DS_DEBUG, "OP_CGT (false ret=%i)\n", ret);
    return (1);
  }

  param->ip += 1;

  DebugLog(DF_OVM, DS_DEBUG, "OP_CGT (true ret=%i)\n", ret);
  
  return (0);
}

static int
ovm_cle(isn_param_t *param)
{
  ovm_var_t *op1;
  ovm_var_t *op2;
  int ret;

  DebugLog(DF_OVM, DS_DEBUG, "OP_CLE\n");

  op2 = stack_pop(param->ctx->ovm_stack);
  op1 = stack_pop(param->ctx->ovm_stack);

  if ((op1 == NULL) || (op2 == NULL)) {
    DebugLog(DF_OVM, DS_ERROR, "op error\n");
    return (1);
  }

  ret = issdl_cmp(op1, op2);

  /* of op1 and/or op2 was tmpvars, free them */
  if ( IS_NOT_BOUND(op1) ) {
    DebugLog(DF_OVM, DS_TRACE, "OP_CLE: free operand 1\n");
    Xfree(op1);
  }

  if ( IS_NOT_BOUND(op2) ) {
    DebugLog(DF_OVM, DS_TRACE, "OP_CLE: free operand 2\n");
    Xfree(op2);
  }

  if (ret > 0) {
    DebugLog(DF_OVM, DS_DEBUG, "OP_CLE (false ret=%i)\n", ret);
    return (1);
  }

  param->ip += 1;

  DebugLog(DF_OVM, DS_DEBUG, "OP_CLE (true ret=%i)\n", ret);
  
  return (0);
}

static int
ovm_cge(isn_param_t *param)
{
  ovm_var_t *op1;
  ovm_var_t *op2;
  int ret;

  DebugLog(DF_OVM, DS_DEBUG, "OP_CGE\n");

  op2 = stack_pop(param->ctx->ovm_stack);
  op1 = stack_pop(param->ctx->ovm_stack);

  if ((op1 == NULL) || (op2 == NULL)) {
    DebugLog(DF_OVM, DS_ERROR, "op error\n");
    return (1);
  }

  ret = issdl_cmp(op1, op2);

  /* of op1 and/or op2 was tmpvars, free them */
  if ( IS_NOT_BOUND(op1) ) {
    DebugLog(DF_OVM, DS_TRACE, "OP_CGE: free operand 1\n");
    Xfree(op1);
  }

  if ( IS_NOT_BOUND(op2) ) {
    DebugLog(DF_OVM, DS_TRACE, "OP_CGE: free operand 2\n");
    Xfree(op2);
  }

  if (ret < 0) {
    DebugLog(DF_OVM, DS_DEBUG, "OP_CGE (false ret=%i)\n", ret);
    return (1);
  }

  param->ip += 1;

  DebugLog(DF_OVM, DS_DEBUG, "OP_CGE (true ret=%i)\n", ret);

  return (0);
}

static int
ovm_regsplit(isn_param_t *param)
{
  ovm_var_t *string;
  ovm_var_t *pattern;
  int ret;
  int i;
  regmatch_t match[10];

  DebugLog(DF_OVM, DS_DEBUG, "OP_REGSPLIT\n");

  pattern = stack_pop(param->ctx->ovm_stack);
  string = stack_pop(param->ctx->ovm_stack);

  /* XXX: REMOVE THIS */
  if ((pattern == NULL) || (string == NULL)) {
    DebugLog(DF_OVM, DS_DEBUG, "op error\n");
    return (1);
  }

  if ((TYPE(pattern) != T_SREGEX) || (TYPE(string) != T_STR)) {
    DebugLog(DF_OVM, DS_DEBUG, "op type error\n");
    return (1);
  }

/*   DPRINTF( ("  regsplit string  = %s\n", STR(string)) ); */
/*   DPRINTF( ("  regsplit pattern = %s\n", SREGEXSTR(pattern)) ); */
/*   DPRINTF( ("  regsplit output  = %i\n", SREGEXNUM(pattern)) ); */

  ret = regexec(&SREGEX(pattern), STR(string), SREGEXNUM(pattern) + 1, match, 0);
  if (ret)
    {
      char err_buf[64];
      regerror(ret, &(SREGEX(pattern)), err_buf, sizeof (err_buf));
      DebugLog(DF_OVM, DS_DEBUG, "regexec error (%s)\n", err_buf);
    }

  for (i = SREGEXNUM(pattern); i > 0; i--)
    {
      size_t res_sz;
      ovm_var_t *res;

/*       DPRINTF( ("rm_so[%i]=%i rm_eo[%i]=%i\n", i, match[i].rm_so, i, match[i].rm_eo) ); */
      res_sz = match[i].rm_eo - match[i].rm_so;
      res = ovm_str_new(res_sz);
      memcpy(STR(res), &(STR(string)[match[i].rm_so]), res_sz);
      stack_push(param->ctx->ovm_stack, res);
    }

  param->ip += 1;

  return (0);
}

static int
ovm_cesv(isn_param_t *param)
{
  DPRINTF( ("OP_CESV\n") );
  DPRINTF( ("not yet implemented\n") );

  return (-1);
}

static int
ovm_past(isn_param_t *param)
{
  DPRINTF( ("OP_PAST\n") );
  DPRINTF( ("not yet implemented\n") );

  return (-1);
}



static ovm_insn_rec_t ops_g[] = {
  { NULL,           1, "end"        },
  { ovm_nop,        1, "nop"        },
  { ovm_push,       2, "push"       },
  { ovm_pop,        2, "pop"        },
  { ovm_pushstatic, 2, "pushstatic" },
  { ovm_pushfield,  2, "pushfield"  },
  { ovm_aload,      0, "aload"      },
  { ovm_astore,     0, "astore"     },
  { ovm_alength,    0, "alength"    },
  { ovm_call,       0, "call"       },
  { ovm_add,        0, "add"        },
  { ovm_sub,        0, "sub"        },
  { ovm_mul,        0, "mul"        },
  { ovm_div,        0, "div"        },
  { ovm_mod,        0, "mod"        },
  { ovm_inc,        0, "inc"        },
  { ovm_dec,        0, "dec"        },
  { ovm_and,        0, "and"        },
  { ovm_or,         0, "or"         },
  { ovm_xor,        0, "xor"        },
  { ovm_neg,        0, "neg"        },
  { ovm_not,        0, "not"        },
  { ovm_jmp,        0, "jmp"        },
  { ovm_je,         0, "je"         },
  { ovm_jne,        0, "jne"        },
  { ovm_jrm,        0, "jrm"        },
  { ovm_jnrm,       0, "jnrm"       },
  { ovm_jlt,        0, "jlt"        },
  { ovm_jgt,        0, "jgt"        },
  { ovm_jle,        0, "jle"        },
  { ovm_jge,        0, "jge"        },
  { ovm_ceq,        0, "ceq"        },
  { ovm_cneq,       0, "cneq"       },
  { ovm_crm,        0, "crm"        },
  { ovm_cnrm,       0, "cnrm"       },
  { ovm_clt,        0, "clt"        },
  { ovm_cgt,        0, "cgt"        },
  { ovm_cle,        0, "cle"        },
  { ovm_cge,        0, "cge"        },
  { ovm_regsplit,   0, "regsplit"   },
  { ovm_cesv,       0, "cesrv"      },
  { ovm_past,       0, "past"       },
  { NULL,           0, NULL         }
};


/*
** generated by:
** grep -E 'define OP_[A-Z]+' ovm.h |
** sed 's/^\#define OP_\([A-Z]*\) [0-9]*$/\"\1\",/' |
** tr A-Z a-z
*/

static char *op_name[] = {
  "end",
  "nop",
  "push",
  "pop",
  "pushstatic",
  "pushfield",
  "aload",
  "astore",
  "alength",
  "call",
  "add",
  "sub",
  "mul",
  "div",
  "mod",
  "inc",
  "dec",
  "and",
  "or",
  "xor",
  "neg",
  "not",
  "jmp",
  "je",
  "jne",
  "jrm",
  "jnrm",
  "jlt",
  "jgt",
  "jle",
  "jge",
  "ceq",
  "cneq",
  "crm",
  "cnrm",
  "clt",
  "cgt",
  "cle",
  "cge",
  "regsplit",
  "csev",
  "past",
  NULL
};

const char *
get_opcode_name(bytecode_t opcode)
{
  if (opcode >= OPCODE_NUM)
    return (NULL);

  return (op_name[opcode]);
}



#if 0
int
ovm_exec_old(orchids_t *ctx, state_instance_t *s, bytecode_t *bytecode)
{
  lifostack_t *ovm_stack;

  /* global vm stack should be clean at ovm_exec() call */

  ovm_stack = ctx->ovm_stack;
  while (1)
    {
      switch (*bytecode)
        {
        case OP_END:
          /* XXX: check that stack is really empty ? */
          DPRINTF( ("OP_END\n") );
          return (0); /* normal exit */

        case OP_NOP:
          DPRINTF( ("OP_NOP (why ? compiler doesn't produce OP_NOPs !)\n") );
          bytecode += 1;
          break;

        case OP_PUSH:
          DPRINTF( ("OP_PUSH [%02lx]\n", bytecode[1]) );
          /* optimize env checking and resolution */
          if (s->current_env && s->current_env[ bytecode[1] ])
            stack_push(ovm_stack, s->current_env[ bytecode[1] ]);
          else if (s->inherit_env && s->inherit_env[ bytecode[1] ])
            stack_push(ovm_stack, s->inherit_env[ bytecode[1] ]);
          else
            {
              DPRINTF( ("env error\n") );
              return (-1);
            }
          bytecode += 2;
          break ;

        case OP_POP:
          DPRINTF( ("OP_POP [%02lx]\n", bytecode[1]) );
          s->current_env[ bytecode[1] ] = stack_pop(ovm_stack);
/*           s->current_env[ bytecode[1] ]->flags &= ~TEMP; */
          /* if (var->flags & TEMP) assoc and remove the TEMP flags
          ** else clone */
          /* do a runtime checking for affectation */
          bytecode += 2;
          break ;

        case OP_PUSHSTATIC:
          DPRINTF( ("OP_PUSHSTATIC [%02lx]\n", bytecode[1]) );
          /* XXX: check boundary ?!... */
          stack_push(ovm_stack, s->state->rule->static_env[ bytecode[1] ]);
          bytecode += 2;
          break ;

        case OP_PUSHFIELD:
          DPRINTF( ("OP_PUSHFIELD [%02lx]\n", bytecode[1]) );
          /* XXX: check boundary ?!... */
          stack_push(ovm_stack, ctx->global_fields[ bytecode[1] ].val);
          bytecode += 2;
          break ;

        case OP_ALOAD:
          DPRINTF( ("not yet implemented\n") );
          return (-1);

        case OP_ASTORE:
          DPRINTF( ("not yet implemented\n") );
          return (-1);

        case OP_ALENGTH:
          DPRINTF( ("not yet implemented\n") );
          return (-1);

        case OP_CALL:
          DPRINTF( ("OP_CALL [%02lx] (%s)\n", bytecode[1],
                    ctx->vm_func_tbl[ bytecode[1] ].name) );
          /* check call table boundary */
          ctx->vm_func_tbl[ bytecode[1] ].func(ctx, s);
          bytecode += 2;
          break ;

        case OP_ADD:
          DPRINTF( ("OP_ADD\n") );
          do {
            ovm_var_t *op1;
            ovm_var_t *op2;
            ovm_var_t *res;

            op2 = stack_pop(ovm_stack);
            op1 = stack_pop(ovm_stack);

            /* XXX: REMOVE THIS */
            if ((op1 == NULL) || (op2 == NULL))
              {
                DPRINTF( ("op error\n") );
                return (1);
              }

            if (op1->type != op2->type)
              {
                DPRINTF( ("incompatible type\n") );
                return (1);
              }

            /* switch/case on type
            ** implement special optimized comparisons
            ** and default will be a regular memcmp() */
            switch (op1->type)
              {
              case T_INT:
                res = (ovm_var_t *) new_integer();
                INT(res) = INT(op1) + INT(op2);
                stack_push(ovm_stack, res);
                break ;

              case T_STR:
                res = (ovm_var_t *) new_string(STRLEN(op1) + STRLEN(op2));
                memcpy(STR(res), STR(op1), STRLEN(op1));
                memcpy(STR(res) + STRLEN(op1), STR(op2), STRLEN(op2));
                stack_push(ovm_stack, res);
                break;

              default:
                DPRINTF( ("type not implemented\n") );
              }

            /* of op1 and/or op2 was tmpvars, free them */
          } while (0);
          bytecode += 1;
          break ;

        case OP_SUB:
          DPRINTF( ("not yet implemented\n") );
          return (-1);

        case OP_MUL:
          DPRINTF( ("not yet implemented\n") );
          return (-1);

        case OP_DIV:
          DPRINTF( ("not yet implemented\n") );
          return (-1);

        case OP_MOD:
          DPRINTF( ("not yet implemented\n") );
          return (-1);

        case OP_INC:
          DPRINTF( ("not yet implemented\n") );
          return (-1);

        case OP_DEC:
          DPRINTF( ("not yet implemented\n") );
          return (-1);

        case OP_AND:
          DPRINTF( ("not yet implemented\n") );
          return (-1);

        case OP_OR:
          DPRINTF( ("not yet implemented\n") );
          return (-1);

        case OP_XOR:
          DPRINTF( ("not yet implemented\n") );
          return (-1);

        case OP_NEG:
          DPRINTF( ("not yet implemented\n") );
          return (-1);

        case OP_NOT:
          DPRINTF( ("not yet implemented\n") );
          return (-1);

        case OP_JMP:
          DPRINTF( ("not yet implemented\n") );
          return (-1);

        case OP_JE:
          DPRINTF( ("not yet implemented\n") );
          return (-1);

        case OP_JNE:
          DPRINTF( ("not yet implemented\n") );
          return (-1);

        case OP_JRM:
          DPRINTF( ("not yet implemented\n") );
          return (-1);

        case OP_JNRM:
          DPRINTF( ("not yet implemented\n") );
          return (-1);

        case OP_JLT:
          DPRINTF( ("not yet implemented\n") );
          return (-1);

        case OP_JGT:
          DPRINTF( ("not yet implemented\n") );
          return (-1);

        case OP_JLE:
          DPRINTF( ("not yet implemented\n") );
          return (-1);

        case OP_JGE:
          DPRINTF( ("not yet implemented\n") );
          return (-1);

        case OP_CEQ:
          DPRINTF( ("OP_CEQ\n") );
          do {
            ovm_var_t *op1;
            ovm_var_t *op2;

            op2 = stack_pop(ovm_stack);
            op1 = stack_pop(ovm_stack);

            /* XXX: REMOVE THIS */
            if ((op1 == NULL) || (op2 == NULL))
              {
                DPRINTF( ("op error\n") );
                return (1);
              }

            /* switch on operand1 type */
            switch (op1->type)
              {
              case T_INT:
                if (op2->type != T_INT)
                  return (1);
                if (INT(op1) != INT(op2))
                  return (1);
                break;

              case T_STR:
                if (op2->type == T_STR)
                  {
                    if (STRLEN(op1) != STRLEN(op2))
                      return (1);
                    if (strncmp(STR(op1), STR(op2), STRLEN(op1)))
                      return (1);
                  }
                else if (op2->type == T_VSTR)
                  {
                    if (STRLEN(op1) != VSTRLEN(op2))
                      return (1);
                    if (strncmp(STR(op1), VSTR(op2), STRLEN(op1)))
                      return (1);
                  }
                else
                  return (1);
                break;

              case T_VSTR:
                if (op2->type == T_STR)
                  {
                    if (VSTRLEN(op1) != STRLEN(op2))
                      return (1);
                    if (strncmp(VSTR(op1), STR(op2), VSTRLEN(op1)))
                      return (1);
                  }
                else if (op2->type == T_VSTR)
                  {
                    if (VSTRLEN(op1) != VSTRLEN(op2))
                      return (1);
                    if (strncmp(VSTR(op1), VSTR(op2), VSTRLEN(op1)))
                      return (1);
                  }
                else
                  return (1);
                break;

              default:
                do {
                  size_t op1_len, op2_len;

                  if (op1->type != op2->type)
                    return (1);
                  op1_len = issdl_get_data_len(op1);
                  op2_len = issdl_get_data_len(op2);
                  if (op1_len != op2_len)
                    return (1);
                  if (memcmp(issdl_get_data(op1),
                             issdl_get_data(op2),
                             op1_len))
                    return (1);
                } while (0);
              }

            /* of op1 and/or op2 was tmpvars, free them */
          } while (0);
          bytecode += 1;
          break ;

        case OP_CNEQ:
          DPRINTF( ("OP_CNEQ\n") );
          do {
            ovm_var_t *op1;
            ovm_var_t *op2;

            op1 = stack_pop(ovm_stack);
            op2 = stack_pop(ovm_stack);

            /* XXX: REMOVE THIS */
            if ((op1 == NULL) || (op2 == NULL))
              {
                DPRINTF( ("op error\n") );
                return (1);
              }

            /* special comparisons */
            switch (op1->type)
              {
              case T_INT:
                /* check type of op2 here */
                if (INT(op1) == INT(op2))
                  return (1);
                break;

              default:
                DPRINTF( ("type comparison not implemented\n") );
              }

            /* of op1 and/or op2 was tmpvars, free them */
          } while (0);
          bytecode += 1;
          break ;

        case OP_CRM:
          DPRINTF( ("not yet implemented\n") );
          return (-1);

        case OP_CLT:
          DPRINTF( ("not yet implemented\n") );
          return (-1);

        case OP_CGT:
          DPRINTF( ("OP_CGT\n") );
          do {
            ovm_var_t *op1;
            ovm_var_t *op2;

            op2 = stack_pop(ovm_stack);
            op1 = stack_pop(ovm_stack);

            /* XXX: REMOVE THIS */
            if ((op1 == NULL) || (op2 == NULL))
              {
                DPRINTF( ("op error\n") );
                return (1);
              }

            /* special comparisons */
            switch (op1->type)
              {
              case T_INT:
                /* check type of op2 here */
                if (INT(op1) <= INT(op2))
                  return (1);
                break;

              default:
                DPRINTF( ("type comparison not implemented\n") );
              }

            /* of op1 and/or op2 was tmpvars, free them */
          } while (0);
          bytecode += 1;
          break ;

        case OP_CLE:
          DPRINTF( ("OP_CLE\n") );
          do {
            ovm_var_t *op1;
            ovm_var_t *op2;

            op2 = stack_pop(ovm_stack);
            op1 = stack_pop(ovm_stack);

            /* XXX: REMOVE THIS */
            if ((op1 == NULL) || (op2 == NULL))
              {
                DPRINTF( ("op error\n") );
                return (1);
              }

            /* special comparisons */
            switch (op1->type)
              {
              case T_INT:
                /* check type of op2 here */
                if (INT(op1) > INT(op2))
                  return (1);
                break;

              default:
                DPRINTF( ("type comparison not implemented\n") );
              }

            /* of op1 and/or op2 was tmpvars, free them */
          } while (0);
          bytecode += 1;
          break ;

        case OP_CGE:
          DPRINTF( ("not yet implemented\n") );
          return (-1);

        case OP_REGSPLIT:
          DPRINTF( ("OP_REGSPLIT\n") );
          do {
            ovm_var_t *string;
            ovm_var_t *pattern;
            int ret;
            int i;
            regmatch_t match[10];

            pattern = stack_pop(ovm_stack);
            string = stack_pop(ovm_stack);

            /* XXX: REMOVE THIS */
            if ((pattern == NULL) || (string == NULL))
              {
                DPRINTF( ("op error\n") );
                return (1);
              }

            if ((TYPE(pattern) != T_SPLITREGEX) || (TYPE(string) != T_STR))
              {
                DPRINTF( ("op type error\n") );
                return (1);
              }

/*             DPRINTF( ("  regsplit string  = %s\n", STR(string)) ); */
/*             DPRINTF( ("  regsplit pattern = %s\n", SREGEXSTR(pattern)) ); */
/*             DPRINTF( ("  regsplit output  = %i\n", SREGEXNUM(pattern)) ); */

            ret = regexec(&SREGEX(pattern), STR(string), SREGEXNUM(pattern) + 1, match, 0);
            if (ret)
              {
                char err_buf[64];
                regerror(ret, &(SREGEX(pattern)), err_buf, sizeof (err_buf));
                DPRINTF( ("regexec error (%s)\n", err_buf) );
              }

            for (i = SREGEXNUM(pattern); i > 0; i--)
              {
                size_t res_sz;
                issdl_string_t *res;

/*                 DPRINTF( ("rm_so[%i]=%i rm_eo[%i]=%i\n", i, match[i].rm_so, i, match[i].rm_eo) ); */
                res_sz = match[i].rm_eo - match[i].rm_so;
                res = new_string(res_sz);
                memcpy(STR(res), &(STR(string)[match[i].rm_so]), res_sz);
                stack_push(ctx->ovm_stack, res);
              }

          } while (0);
          bytecode += 1;
          break ;

        case OP_CESV:
          DPRINTF( ("not yet implemented\n") );
          return (-1);

        default:
          DPRINTF( ("bytecode 0x%02lX not supported\n", *bytecode) );
          return (1);
        }
    }

  /* this should never be executed... only for shutting gcc out ;-) */
  return (1);
}
#endif


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
