/**
 ** @file ovm.c
 ** Orchids 'virtual machine'
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 1.0
 ** @ingroup ovm
 **
 ** @date  Started on: Wed Mar 12 12:14:53 2003
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
#include "ovm_priv.h"

#define NULL_VAR (param->state->state->rule->static_env[	\
		    param->ctx->rule_compiler->static_null_res_id	\
		    ])

#define PARAM_ERROR_VAR (param->state->state->rule->static_env[		\
			   param->ctx->rule_compiler->static_param_error_res_id \
			   ])

#define REGEX_ERROR_VAR (param->state->state->rule->static_env[		\
			   param->ctx->rule_compiler->static_regex_error_res_id \
			   ])

#define TRUE_VAR (param->state->state->rule->static_env[		\
		    param->ctx->rule_compiler->static_1_res_id \
		    ])
#define FALSE_VAR (param->state->state->rule->static_env[		\
		     param->ctx->rule_compiler->static_0_res_id		\
		     ])


static void
ovm_flush(orchids_t *ctx)
{
   ovm_var_t   *res;

   while ((res = stack_pop(ctx->ovm_stack)) != NULL)
     FREE_IF_NEEDED(res);
}

int
ovm_exec(orchids_t *ctx, state_instance_t *s, bytecode_t *bytecode)
{
  isn_param_t isn_param;
  int	      ret;
  ovm_var_t   *res;

  isn_param.bytecode = bytecode;
  isn_param.ip = bytecode;
  isn_param.state = s;
  isn_param.ctx = ctx;

  while (*isn_param.ip != OP_END) {
    if (*isn_param.ip >= OPCODE_NUM) {
      DebugLog(DF_OVM, DS_ERROR, "unknown opcode 0x%02lx\n", *isn_param.ip);
      ovm_flush(ctx);
      return (1);
    }

    ret = ops_g[ *isn_param.ip ].insn(&isn_param);
  }

  res = stack_pop(ctx->ovm_stack);
  ovm_flush(ctx);
  if ((!(IS_NULL(res))) && TYPE(res) == T_INT)
    return (!(INT(res)));
  else
    return (1);
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

      case OP_TRASH:
	fprintf(fp, "0x%04x: %08x             | trash\n", offset, OP_TRASH);
	offset += 1;
	break ;

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
	fprintf(fp, "0x%04x: %08lx             | jmp [%lu]\n", offset,
		*code, code[1]);
	offset += 2;
	break ;

      case OP_POPCJMP:
	fprintf(fp, "0x%04x: %08lx             | popcjmp [%lu] \n", offset,
		*code, code[1]);
	offset += 2;
	break ;

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

      case OP_TRASH:
	fprintf(fp, "0x%04x: %08x        | trash\n", offset, OP_TRASH);
	offset += 1;
	break ;

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
	fprintf(fp, "0x%04x: %08lx             | jmp [%lu]\n", offset,
		*code, code[1]);
	offset += 2;
	break ;

      case OP_POPCJMP:
	fprintf(fp, "0x%04x: %08lx             | popcjmp [%lu] \n", offset,
		*code, code[1]);
	offset += 2;
	break ;

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
           "OP_NOP (why ? Compiler doesn't produce OP_NOPs !)\n");

  param->ip += 1;

  return (0);
}

static int
ovm_push(isn_param_t *param)
{
  ovm_var_t *res;
  DebugLog(DF_OVM, DS_DEBUG,
           "OP_PUSH [%02lx] ($%s)\n",
            param->ip[1], param->state->state->rule->var_name[ param->ip[1] ]);

  /* XXX: optimize env checking and resolution */
  if (param->state->current_env && param->state->current_env[ param->ip[1] ])
    res = param->state->current_env[ param->ip[1] ];
  else if (param->state->inherit_env && param->state->inherit_env[ param->ip[1] ])
    res = param->state->inherit_env[ param->ip[1] ];
  else
  {
    res = NULL_VAR;
  }

  stack_push(param->ctx->ovm_stack, res);

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
ovm_trash(isn_param_t *param)
{
  ovm_var_t *var;

  DebugLog(DF_OVM, DS_DEBUG, "OP_TRASH\n");

  var = stack_pop(param->ctx->ovm_stack);
  if (var && CAN_FREE_VAR(var)) {
    Xfree(var);
  }

  param->ip += 1;

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
ovm_call(isn_param_t *param)
{
  DebugLog(DF_OVM, DS_DEBUG, "OP_CALL [%02lx] (%s)\n", param->ip[1],
            param->ctx->vm_func_tbl[ param->ip[1] ].name);
  /* Check call table boundary */
  param->ctx->vm_func_tbl[ param->ip[1] ].func(param->ctx, param->state);
  param->ip += 2;

  return (0);
}

static int
ovm_add(isn_param_t *param)
{
  ovm_var_t *op1;
  ovm_var_t *op2;
  ovm_var_t *res = NULL;

  DebugLog(DF_OVM, DS_DEBUG, "OP_ADD\n");

  op2 = stack_pop(param->ctx->ovm_stack);
  op1 = stack_pop(param->ctx->ovm_stack);

  param->ip += 1;

  if (!IS_NULL(op1) && !IS_NULL(op2))
    res = issdl_add(op1, op2);
  if (res == NULL)
    res = NULL_VAR;

  stack_push(param->ctx->ovm_stack, res);

  /* If op1 and/or op2 was temp vars, free them */
  if ( IS_NOT_BOUND(op1) ) {
    DebugLog(DF_OVM, DS_TRACE, "OP_ADD: free operand 1\n");
    Xfree(op1);
  }
  if ( IS_NOT_BOUND(op2) ) {
    DebugLog(DF_OVM, DS_TRACE, "OP_ADD: free operand 2\n");
    Xfree(op2);
  }

  return (0);
}


static int
ovm_sub(isn_param_t *param)
{
  ovm_var_t *op1;
  ovm_var_t *op2;
  ovm_var_t *res = NULL;

  DebugLog(DF_OVM, DS_DEBUG,"OP_SUB\n");

  op2 = stack_pop(param->ctx->ovm_stack);
  op1 = stack_pop(param->ctx->ovm_stack);

  param->ip += 1;

  if (!IS_NULL(op1) && !IS_NULL(op2))
    res = issdl_sub(op1, op2);
  if (res == NULL)
    res = NULL_VAR;

  stack_push(param->ctx->ovm_stack, res);

  /* If op1 and/or op2 was temp vars, free them */
  if ( IS_NOT_BOUND(op1) ) {
      DebugLog(DF_OVM, DS_DEBUG,"free(op1);\n");
      Xfree(op1);
  }

  if ( IS_NOT_BOUND(op2) ) {
    DebugLog(DF_OVM, DS_DEBUG, "free(op2);\n");
    Xfree(op2);
  }

  return (0);
}

static int
ovm_mul(isn_param_t *param)
{
  ovm_var_t *op1;
  ovm_var_t *op2;
  ovm_var_t *res = NULL;

  DPRINTF( ("OP_MUL\n") );

  op2 = stack_pop(param->ctx->ovm_stack);
  op1 = stack_pop(param->ctx->ovm_stack);

  param->ip += 1;

  if (!IS_NULL(op1) && !IS_NULL(op2))
    res = issdl_mul(op1, op2);
  if (res == NULL)
    res = NULL_VAR;

  stack_push(param->ctx->ovm_stack, res);

  /* If op1 and/or op2 was temp vars, free them */
  if ( IS_NOT_BOUND(op1) ) {
      DebugLog(DF_OVM, DS_DEBUG, "free(op1);\n");
      Xfree(op1);
  }

  if ( IS_NOT_BOUND(op2) ) {
    DebugLog(DF_OVM, DS_DEBUG, "free(op2);\n");
    Xfree(op2);
  }

  return (0);
}

static int
ovm_div(isn_param_t *param)
{
  ovm_var_t *op1;
  ovm_var_t *op2;
  ovm_var_t *res = NULL;

  DebugLog(DF_OVM, DS_DEBUG, "OP_DIV\n");

  op2 = stack_pop(param->ctx->ovm_stack);
  op1 = stack_pop(param->ctx->ovm_stack);

  param->ip += 1;

  if (!IS_NULL(op1) && !IS_NULL(op2))
    res = issdl_div(op1, op2);
  if (res == NULL)
    res = NULL_VAR;

  stack_push(param->ctx->ovm_stack, res);

  /* If op1 and/or op2 was temp vars, free them */
  if ( IS_NOT_BOUND(op1) ) {
      DebugLog(DF_OVM, DS_DEBUG, "free(op1);\n");
      Xfree(op1);
  }

  if ( IS_NOT_BOUND(op2) ) {
    DebugLog(DF_OVM, DS_DEBUG, "free(op2);\n");
    Xfree(op2);
  }

  return (0);
}

static int
ovm_mod(isn_param_t *param)
{
  ovm_var_t *op1;
  ovm_var_t *op2;
  ovm_var_t *res = NULL;

  DebugLog(DF_OVM, DS_DEBUG, "OP_MOD\n");

  op2 = stack_pop(param->ctx->ovm_stack);
  op1 = stack_pop(param->ctx->ovm_stack);

  param->ip += 1;

  if (!IS_NULL(op1) && !IS_NULL(op2))
    res = issdl_mod(op1, op2);
  if (res == NULL)
    res = NULL_VAR;


  stack_push(param->ctx->ovm_stack, res);

  /* If op1 and/or op2 was temp vars, free them */
  if ( IS_NOT_BOUND(op1) ) {
    DebugLog(DF_OVM, DS_DEBUG, "free(op1);\n");
    Xfree(op1);
  }

  if ( IS_NOT_BOUND(op2) ) {
    DebugLog(DF_OVM, DS_DEBUG, "free(op2);\n");
    Xfree(op2);
  }

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
  DPRINTF( ("OP_AND\n") );
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
  DebugLog(DF_OVM, DS_DEBUG, "OP_JMP\n");

  param->ip += *(param->ip + 1);
  param->ip += 2;

  return (0);
}

static int
ovm_popcjmp(isn_param_t *param)
{
  ovm_var_t *res;

  DebugLog(DF_OVM, DS_DEBUG, "OP_POPCJMP\n");

  res = stack_pop(param->ctx->ovm_stack);

  if (issdl_test(res))
    param->ip += *(param->ip + 1);
  param->ip += 2;

  return (0);
}

static int
ovm_ceq(isn_param_t *param)
{
  ovm_var_t *op1;
  ovm_var_t *op2;
  ovm_var_t *res = NULL;
  int ret = 1; // False by default

  DebugLog(DF_OVM, DS_DEBUG, "OP_CEQ\n");

  op1 = stack_pop(param->ctx->ovm_stack);
  op2 = stack_pop(param->ctx->ovm_stack);

  param->ip += 1;

  if (!IS_NULL(op1) && !IS_NULL(op2))
  {
    ret = issdl_cmp(op1, op2);
    res = (ret) ? FALSE_VAR : TRUE_VAR;
    if (!ret)
      DebugLog(DF_OVM, DS_TRACE, "OP_CEQ: true\n");
  }
  else
  {
    res = NULL_VAR;
  }
  stack_push(param->ctx->ovm_stack, res);

  FREE_IF_NEEDED(op1);
  FREE_IF_NEEDED(op2);

  return (0);
}


static int
ovm_cneq(isn_param_t *param)
{
  ovm_var_t *op1;
  ovm_var_t *op2;
  ovm_var_t *res;
  int ret = 1;

  DebugLog(DF_OVM, DS_DEBUG, "OP_CNEQ\n");

  op2 = stack_pop(param->ctx->ovm_stack);
  op1 = stack_pop(param->ctx->ovm_stack);

  param->ip += 1;

  if (!IS_NULL(op1) && !IS_NULL(op2))
  {
    ret = issdl_cmp(op1, op2);
    res = (ret) ? TRUE_VAR : FALSE_VAR;
    if (ret)
      DebugLog(DF_OVM, DS_TRACE, "OP_CNEQ: true\n");
  }
  else
  {
    res = NULL_VAR;
  }

  stack_push(param->ctx->ovm_stack, res);

  FREE_IF_NEEDED(op1);
  FREE_IF_NEEDED(op2);

  return (0);
}


static int
ovm_crm(isn_param_t *param)
{
  ovm_var_t *string;
  ovm_var_t *regex;
  ovm_var_t *res;
  char *s;
  int ret;

  DebugLog(DF_OVM, DS_DEBUG, "OP_CRM\n");

  regex = stack_pop(param->ctx->ovm_stack);
  string = stack_pop(param->ctx->ovm_stack);

  param->ip += 1;

  if (IS_NULL(regex) || IS_NULL(string))
  {
    res = NULL_VAR;
  }
  else if ((TYPE(regex) != T_REGEX) ||
	   ((TYPE(string) != T_STR) && TYPE(string) != T_VSTR)) {
    res = PARAM_ERROR_VAR;
  }
  else
  {
    s = ovm_strdup(string);

    DebugLog(DF_OVM, DS_DEBUG, "OP_CRM str=\"%s\" regex=\"%s\"\n",
	     s, REGEXSTR(regex));

    ret = regexec(&REGEX(regex), s, 0, NULL, 0);
    Xfree(s);
    if (ret == REG_NOMATCH) {
      DebugLog(DF_OVM, DS_DEBUG, "OP_CRM (false)\n");
      res = FALSE_VAR;
    }
    else if (ret != 0) {
      char err_buf[64];
      regerror(ret, &(REGEX(regex)), err_buf, sizeof (err_buf));
      DebugLog(DF_OVM, DS_ERROR, "regexec error (%s)\n", err_buf);
      res = PARAM_ERROR_VAR;
    }
    else
    {
      DebugLog(DF_OVM, DS_DEBUG, "OP_CRM (true)\n");
      res = TRUE_VAR;
    }
  }

  stack_push(param->ctx->ovm_stack, res);
  FREE_IF_NEEDED(string);
  FREE_IF_NEEDED(regex);

  return (0);
}

static int
ovm_cnrm(isn_param_t *param)
{
  ovm_var_t *string;
  ovm_var_t *regex;
  ovm_var_t *res;
  char *s;
  int ret;

  DebugLog(DF_OVM, DS_DEBUG, "OP_CNRM\n");

  regex = stack_pop(param->ctx->ovm_stack);
  string = stack_pop(param->ctx->ovm_stack);

  param->ip += 1;

  if (IS_NULL(regex) || IS_NULL(string))
  {
    res = NULL_VAR;
  }
  else if ((TYPE(regex) != T_REGEX) ||
	   ((TYPE(string) != T_STR) && TYPE(string) != T_VSTR)) {
    res = PARAM_ERROR_VAR;
  }
  else
  {
    s = ovm_strdup(string);

    DebugLog(DF_OVM, DS_DEBUG, "OP_CNRM str=\"%s\" regex=\"%s\"\n", s, REGEXSTR(regex));

    ret = regexec(&REGEX(regex), s, 0, NULL, 0);
    Xfree(s);
    if (ret == 0) {
      DebugLog(DF_OVM, DS_DEBUG, "OP_CNRM (false)\n");
      res = FALSE_VAR;
    }
    else if (ret != REG_NOMATCH) {
      char err_buf[64];
      regerror(ret, &(REGEX(regex)), err_buf, sizeof (err_buf));
      DebugLog(DF_OVM, DS_ERROR, "regexec eror (%s)\n", err_buf);
      res = REGEX_ERROR_VAR;
    }
    else
    {
      DebugLog(DF_OVM, DS_DEBUG, "OP_CNRM (true)\n");
      res = TRUE_VAR;
    }
  }
  stack_push(param->ctx->ovm_stack, res);
  FREE_IF_NEEDED(string);
  FREE_IF_NEEDED(regex);

  return (0);
}

static int
ovm_clt(isn_param_t *param)
{
  ovm_var_t *op1;
  ovm_var_t *op2;
  ovm_var_t *res;
  int ret = 0; //False by default

  DebugLog(DF_OVM, DS_DEBUG, "OP_CLT\n");

  op2 = stack_pop(param->ctx->ovm_stack);
  op1 = stack_pop(param->ctx->ovm_stack);

  param->ip += 1;

  if (!IS_NULL(op1) && !IS_NULL(op2))
  {
    ret = issdl_cmp(op1, op2);
    if (ret >= 0) {
      DebugLog(DF_OVM, DS_DEBUG, "OP_CLT (false ret=%i)\n", ret);
      res = FALSE_VAR;
    }
    else
    {
      DebugLog(DF_OVM, DS_DEBUG, "OP_CLT (true ret=%i)\n", ret);
      res = TRUE_VAR;
    }
  }
  else
  {
    res = NULL_VAR;
  }

  FREE_IF_NEEDED(op1);
  FREE_IF_NEEDED(op2);
  stack_push(param->ctx->ovm_stack, res);

  return (0);
}

static int
ovm_cgt(isn_param_t *param)
{
  ovm_var_t *op1;
  ovm_var_t *op2;
  ovm_var_t *res;
  int ret = 0; // False by default

  DebugLog(DF_OVM, DS_DEBUG, "OP_CGT\n");

  op2 = stack_pop(param->ctx->ovm_stack);
  op1 = stack_pop(param->ctx->ovm_stack);

  param->ip += 1;

  if (!IS_NULL(op1) && !IS_NULL(op2))
  {
    ret = issdl_cmp(op1, op2);
    if (ret <= 0) {
      DebugLog(DF_OVM, DS_DEBUG, "OP_CGT (false ret=%i)\n", ret);
      res = FALSE_VAR;
    }
    else
    {
      DebugLog(DF_OVM, DS_DEBUG, "OP_CGT (true ret=%i)\n", ret);
      res = TRUE_VAR;
    }
  }
  else
  {
    res = NULL_VAR;
  }

  FREE_IF_NEEDED(op1);
  FREE_IF_NEEDED(op2);
  stack_push(param->ctx->ovm_stack, res);

  return (0);
}

static int
ovm_cle(isn_param_t *param)
{
  ovm_var_t *op1;
  ovm_var_t *op2;
  ovm_var_t *res;
  int ret = 1; // False by default

  DebugLog(DF_OVM, DS_DEBUG, "OP_CLE\n");

  op2 = stack_pop(param->ctx->ovm_stack);
  op1 = stack_pop(param->ctx->ovm_stack);

  param->ip += 1;

  if (!IS_NULL(op1) && !IS_NULL(op2))
  {
    ret = issdl_cmp(op1, op2);
    if (ret > 0) {
      DebugLog(DF_OVM, DS_DEBUG, "OP_CLE (false ret=%i)\n", ret);
      res = FALSE_VAR;
    }
    else
    {
      DebugLog(DF_OVM, DS_DEBUG, "OP_CLE (true ret=%i)\n", ret);
      res = TRUE_VAR;
    }
  }
 else
  {
    res = NULL_VAR;
  }

  FREE_IF_NEEDED(op1);
  FREE_IF_NEEDED(op2);
  stack_push(param->ctx->ovm_stack, res);

  return (0);
}

static int
ovm_cge(isn_param_t *param)
{
  ovm_var_t *op1;
  ovm_var_t *op2;
  ovm_var_t *res;
  int ret = -1; // False by default

  DebugLog(DF_OVM, DS_DEBUG, "OP_CGE\n");

  op2 = stack_pop(param->ctx->ovm_stack);
  op1 = stack_pop(param->ctx->ovm_stack);

  param->ip += 1;

  if (!IS_NULL(op1) && !IS_NULL(op2))
  {
    ret = issdl_cmp(op1, op2);
    if (ret < 0) {
      DebugLog(DF_OVM, DS_DEBUG, "OP_CGE (false ret=%i)\n", ret);
      res = FALSE_VAR;
    }
    else
    {
      DebugLog(DF_OVM, DS_DEBUG, "OP_CGE (true ret=%i)\n", ret);
      res = TRUE_VAR;
    }
  }
  else
  {
    res = NULL_VAR;
  }

  FREE_IF_NEEDED(op1);
  FREE_IF_NEEDED(op2);
  stack_push(param->ctx->ovm_stack, res);

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

  param->ip += 1;

  if (IS_NULL(pattern) && IS_NULL(string))
  {
    DebugLog(DF_OVM, DS_ERROR, "op error\n");
  }
  else if ((TYPE(pattern) != T_SREGEX) || (TYPE(string) != T_STR)) {
    DebugLog(DF_OVM, DS_ERROR, "op type error\n");
  }
  else
  {
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
    else
    {
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
    }
  }

  FREE_IF_NEEDED(pattern);
  FREE_IF_NEEDED(string);

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
  { ovm_trash,      0, "trash"      },
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
  { ovm_popcjmp,    0, "popcjmp"    },
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
  "trash",
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
  "popcjmp",
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
