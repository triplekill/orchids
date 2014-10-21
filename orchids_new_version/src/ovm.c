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

#define TRUE_VAR (ctx->one)
#define FALSE_VAR (ctx->zero)


static void ovm_flush(orchids_t *ctx)
{
  STACK_DROP (ctx->ovm_stack, STACK_DEPTH(ctx->ovm_stack));
}

static
void ovm_exec_base(orchids_t *ctx, state_instance_t *s, bytecode_t *bytecode)
{
  isn_param_t isn_param;

  isn_param.bytecode = bytecode;
  isn_param.ip = bytecode;
  isn_param.state = s;
  isn_param.ctx = ctx;

  while (*isn_param.ip != OP_END)
    {
      if (*isn_param.ip >= OPCODE_NUM)
	{
	  DebugLog(DF_OVM, DS_ERROR, "unknown opcode 0x%02lx\n", *isn_param.ip);
	  return;
	}
      (void) (*ops_g[*isn_param.ip].insn) (&isn_param);
    }
}

void ovm_exec_stmt(orchids_t *ctx, state_instance_t *s, bytecode_t *bytecode)
{
  ovm_exec_base(ctx, s, bytecode);
  ovm_flush(ctx);
}

int ovm_exec_expr(orchids_t *ctx, state_instance_t *s, bytecode_t *bytecode)
{
  ovm_var_t *res;

  ovm_exec_base(ctx, s, bytecode);
  res = POP_VALUE(ctx);
  ovm_flush(ctx);
  if ((!(IS_NULL(res))) && TYPE(res) == T_INT)
    return (!(INT(res)));
  else
    return 1;
}

void fprintf_bytecode(FILE *fp, bytecode_t *bytecode)
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
	case OP_PUSHZERO:
	  fprintf(fp, "0x%04x: %08x             | pushzero\n",
		  offset, OP_PUSHZERO);
	  offset += 1;
	  break;
	case OP_PUSHONE:
	  fprintf(fp, "0x%04x: %08x             | pushone\n",
		  offset, OP_PUSHONE);
	  offset += 1;
	  break;
	case OP_PUSHSTATIC:
	  fprintf(fp, "0x%04x: %08x %08lx    | pushstatic [%lu]\n",
		  offset, OP_PUSHSTATIC, code[1], code[1]);
	  offset += 2;
	  break;
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
	  fprintf(fp, "0x%04x: %08lx             | sub\n", offset, *code);
	  offset += 1;
	  break ;
	case OP_OPP:
	  fprintf(fp, "0x%04x: %08lx             | opp\n", offset, *code);
	  offset += 1;
	  break;
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
	  fprintf(fp, "0x%04x: %08lx             | and\n", offset, *code);
	  return ;
	case OP_OR:
	  fprintf(fp, "0x%04x: %08lx             | or\n", offset, *code);
	  return ;
	case OP_XOR:
	  fprintf(fp, "0x%04x: %08lx             | xor\n", offset, *code);
	  return ;
	case OP_NEG:
	  fprintf(fp, "unused opcode. See OP_OPP\n");
	  return ;
	case OP_NOT:
	  fprintf(fp, "0x%04x: %08lx             | not\n", offset, *code);
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
	  fprintf(fp, "0x%04x: %08x             | cesv\n", offset, OP_CESV);
	  offset += 1;
	  break ;
	case OP_EMPTY_EVENT:
	  fprintf(fp, "0x%04x: %08lx             | emptyevent\n",
		  offset, *code);
	  offset += 1;
	  break ;
	case OP_ADD_EVENT:
	  fprintf(fp, "0x%04x: %08x %08lx    | addevent [%lu]\n",
		  offset, OP_ADD_EVENT, code[1], code[1]);
	  offset += 2;
	  break ;
	default:
	  DebugLog(DF_OVM, DS_ERROR, "unknown opcode %lu\n", *code);
	  return ;
	  /* exit(EXIT_FAILURE); */
	}
      code = bytecode + offset;
    }
  return;
}

void fprintf_bytecode_short(FILE *fp, bytecode_t *bytecode)
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
	case OP_PUSHZERO:
	  fprintf(fp, "0x%04x: %02x       | pushzero\n",
		  offset, OP_PUSHZERO);
	  offset += 1;
	  break;
	case OP_PUSHONE:
	  fprintf(fp, "0x%04x: %02x       | pushone\n",
		  offset, OP_PUSHONE);
	  offset += 1;
	  break;
	case OP_PUSHSTATIC:
	  fprintf(fp, "0x%04x: %02x %02lx    | pushstatic [%lu]\n",
		  offset, OP_PUSHSTATIC, code[1], code[1]);
	  offset += 2;
	  break;
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
	case OP_OPP:
	  fprintf(fp, "%04x: %02lx       | opp\n", offset, *code);
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
	  fprintf(fp, "%04x: %02lx       | and\n", offset, *code);
	  return ;
	case OP_OR:
	  fprintf(fp, "%04x: %02lx       | or\n", offset, *code);
	  return ;
	case OP_XOR:
	  fprintf(fp, "%04x: %02lx       | xor\n", offset, *code);
	  return ;
	case OP_NEG:
	  fprintf(fp, "unused opcode, see OP_OPP\n");
	  return ;
	case OP_NOT:
	  fprintf(fp, "%04x: %02lx       | not\n", offset, *code);
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
	  fprintf(fp, "%04x: %02x       | cesv\n", offset, OP_CESV);
	  offset += 1;
	  break ;
	case OP_EMPTY_EVENT:
	  fprintf(fp, "%04x: %02x       | emptyevent\n",
		  offset, OP_EMPTY_EVENT);
	  offset += 1;
	  break ;
	case OP_ADD_EVENT:
	  fprintf(fp, "%04x: %02x %02lx  | addevent [%lu]\n",
		  offset, OP_ADD_EVENT, code[1], code[1]);
	  offset += 2;
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

void fprintf_bytecode_dump(FILE *fp, bytecode_t *code)
{
  fprintf(fp, "bytecode dump: ");
  while (*code)
    fprintf(fp, "0x%02lX ", *code++);
  fprintf(fp, "\n");
}

static int ovm_nop(isn_param_t *param)
{
  DebugLog(DF_OVM, DS_ERROR,
           "OP_NOP (why ? Compiler doesn't produce OP_NOPs !)\n");
  param->ip += 1;
  return 0;
}

static int ovm_push(isn_param_t *param)
{
  ovm_var_t *res;
  state_instance_t *state = param->state;
  bytecode_t op = param->ip[1];

  DebugLog(DF_OVM, DS_DEBUG,
           "OP_PUSH [%02lx] ($%s)\n",
            op, state->state->rule->var_name[op]);

  res = ovm_read_value (state->env, op);
  PUSH_VALUE(param->ctx,res);
  param->ip += 2;
  return 0;
}

static void env_bind_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  env_bind_t *bind = (env_bind_t *)p;

  GC_TOUCH (gc_ctx, bind->val);
}

static void env_bind_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  return;
}

static int env_bind_traverse (gc_traverse_ctx_t *gtc, gc_header_t *p,
			      void *data)
{
  env_bind_t *bind = (env_bind_t *)p;
  int err = 0;

  err = (*gtc->do_subfield) (gtc, (gc_header_t *)bind->val, data);
  return err;
}

static gc_class_t env_bind_class = {
  GC_ID('b','i','n','d'),
  env_bind_mark_subfields,
  env_bind_finalize,
  env_bind_traverse
};

static void env_split_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  env_split_t *split = (env_split_t *)p;

  GC_TOUCH (gc_ctx, split->left);
  GC_TOUCH (gc_ctx, split->right);
}

static void env_split_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  return;
}

static int env_split_traverse (gc_traverse_ctx_t *gtc, gc_header_t *p,
			      void *data)
{
  env_split_t *split = (env_split_t *)p;
  int err = 0;

  err = (*gtc->do_subfield) (gtc, (gc_header_t *)split->left, data);
  if (err)
    return err;
  err = (*gtc->do_subfield) (gtc, (gc_header_t *)split->right, data);
  return err;
}

static gc_class_t env_split_class = {
  GC_ID('s','p','l','t'),
  env_split_mark_subfields,
  env_split_finalize,
  env_split_traverse
};

static int ovm_pop(isn_param_t *param)
{
  ovm_var_t *val;
  state_instance_t *state = param->state;
  bytecode_t op = param->ip[1];
  orchids_t *ctx = param->ctx;
  gc_t *gc_ctx = ctx->gc_ctx;
  ovm_var_t *env;
  ovm_var_t *branch[32]; /*depth of environment is at most 32,
			   since indexed by int32_t's in general
			   (here by a bytecode_t, but see
			   static_env_sz in rule_s, orchids.h */
  ovm_var_t **branchp;
  bytecode_t mask, op2;
  ovm_var_t *env_left, *env_right;

  DebugLog(DF_OVM, DS_DEBUG, "OP_POP [%02lx] ($%s)\n",
            op, state->state->rule->var_name[op]);
  val = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  for (env = state->env, mask = 1L, branchp=branch; env!=NULL; mask <<= 1)
    if (TYPE(env)==T_BIND)
      break;
    else if (op & mask)
      {
	*branchp++ = ((env_split_t *)env)->left;
	env = ((env_split_t *)env)->right;
      }
    else
      {
	*branchp++ = ((env_split_t *)env)->right;
	env = ((env_split_t *)env)->left;
      }
  if (env!=NULL)
    {
      op2 = ((env_bind_t *)env)->var;
      if (op2!=op)
	for (;; mask<<=1)
	  {
	    if ((op2 & mask)==(op & mask))
	      *branchp++ = NULL;
	    else
	      {
		*branchp++ = env;
		mask <<= 1;
		break;
	      }
	  }
    }
  GC_START(gc_ctx, 1);
  env = gc_alloc (gc_ctx, sizeof(env_bind_t), &env_bind_class);
  env->gc.type = T_BIND;
  ((env_bind_t *)env)->var = op;
  GC_TOUCH (gc_ctx, ((env_bind_t *)env)->val = val);
  GC_UPDATE(gc_ctx, 0, env);
  for (; branchp > branch; )
    {
      mask >>= 1;
      if (op & mask)
	{
	  env_left = *--branchp;
	  env_right = env;
	}
      else
	{
	  env_left = env;
	  env_right = *--branchp;
	}
      env = gc_alloc (gc_ctx, sizeof(env_split_t), &env_split_class);
      env->gc.type = T_SPLIT;
      GC_TOUCH (gc_ctx, ((env_split_t *)env)->left = env_left);
      GC_TOUCH (gc_ctx, ((env_split_t *)env)->right = env_right);
      GC_UPDATE(gc_ctx, 0, env);
    }
  GC_TOUCH (gc_ctx, state->env = env);
  GC_END(gc_ctx);
  STACK_DROP(ctx->ovm_stack, 1);

  param->ip += 2;
  return 0;
}

static int ovm_trash(isn_param_t *param)
{
  DebugLog(DF_OVM, DS_DEBUG, "OP_TRASH\n");
  STACK_DROP(param->ctx->ovm_stack, 1);
  param->ip += 1;
  return (0);
}

static int ovm_push_zero(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;

  DebugLog(DF_OVM, DS_DEBUG, "OP_PUSHZERO\n");
  PUSH_RETURN_FALSE(ctx);
  param->ip += 1;
  return 0;
}

static int ovm_push_one(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;

  DebugLog(DF_OVM, DS_DEBUG, "OP_PUSHONE\n");
  PUSH_RETURN_TRUE(ctx);
  param->ip += 1;
  return 0;
}

static int ovm_pushstatic(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  state_instance_t *state = param->state;
  bytecode_t op = param->ip[1];

  DebugLog(DF_OVM, DS_DEBUG, "OP_PUSHSTATIC [%lu]\n", op);
  PUSH_VALUE (ctx, state->state->rule->static_env[op]);
  param->ip += 2;
  return 0;
}

static int ovm_pushfield(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  bytecode_t op = param->ip[1];

  DebugLog(DF_OVM, DS_DEBUG,
           "OP_PUSHFIELD [%02lx] (.%s)\n",
           op, ctx->global_fields->fields[op].name);
  /* XXX: check boundary ?!... */
  PUSH_VALUE(ctx, ctx->global_fields->fields[op].val);
  param->ip += 2;

  return (0);
}


static int ovm_call(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  state_instance_t *state = param->state;
  bytecode_t op = param->ip[1];

  DebugLog(DF_OVM, DS_DEBUG, "OP_CALL [%02lx] (%s)\n", param->ip[1],
	   ctx->vm_func_tbl[op].name);
  /* Check call table boundary */
  (*ctx->vm_func_tbl[op].func) (ctx, state);
  param->ip += 2;
  return (0);
}

static int ovm_add(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  ovm_var_t *op1;
  ovm_var_t *op2;
  ovm_var_t *res = NULL;

  DebugLog(DF_OVM, DS_DEBUG, "OP_ADD\n");
  op2 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  op1 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  param->ip += 1;
  res = issdl_add(ctx->gc_ctx, op1, op2);
  STACK_DROP(ctx->ovm_stack, 2);
  PUSH_VALUE(ctx, res);
  return 0;
}


static int ovm_sub(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  ovm_var_t *op1;
  ovm_var_t *op2;
  ovm_var_t *res = NULL;

  DebugLog(DF_OVM, DS_DEBUG, "OP_SUB\n");
  op2 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  op1 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  param->ip += 1;
  res = issdl_sub(ctx->gc_ctx, op1, op2);
  STACK_DROP(ctx->ovm_stack, 2);
  PUSH_VALUE(ctx, res);
  return 0;
}

static int ovm_opp(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  ovm_var_t *op;
  ovm_var_t *res = NULL;

  DebugLog(DF_OVM, DS_DEBUG, "OP_OPP\n");
  op = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  param->ip += 1;
  if (!IS_NULL(op))
    res = issdl_opp(ctx->gc_ctx, op);
  STACK_DROP(ctx->ovm_stack, 1);
  PUSH_VALUE(ctx, res);
  return 0;
}

static int ovm_mul(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  ovm_var_t *op1;
  ovm_var_t *op2;
  ovm_var_t *res = NULL;

  DebugLog(DF_OVM, DS_DEBUG, "OP_MUL\n");
  op2 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  op1 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  param->ip += 1;
  if (!IS_NULL(op1) && !IS_NULL(op2))
    res = issdl_mul(ctx->gc_ctx, op1, op2);
  STACK_DROP(ctx->ovm_stack, 2);
  PUSH_VALUE(ctx, res);
  return 0;
}

static int ovm_div(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  ovm_var_t *op1;
  ovm_var_t *op2;
  ovm_var_t *res = NULL;

  DebugLog(DF_OVM, DS_DEBUG, "OP_DIV\n");
  op2 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  op1 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  param->ip += 1;
  if (!IS_NULL(op1) && !IS_NULL(op2))
    res = issdl_div(ctx->gc_ctx, op1, op2);
  STACK_DROP(ctx->ovm_stack, 2);
  PUSH_VALUE(ctx, res);
  return 0;
}

static int ovm_mod(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  ovm_var_t *op1;
  ovm_var_t *op2;
  ovm_var_t *res = NULL;

  DebugLog(DF_OVM, DS_DEBUG, "OP_MOD\n");
  op2 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  op1 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  param->ip += 1;
  if (!IS_NULL(op1) && !IS_NULL(op2))
    res = issdl_mod(ctx->gc_ctx, op1, op2);
  STACK_DROP(ctx->ovm_stack, 2);
  PUSH_VALUE(ctx, res);
  return 0;
}

static int ovm_inc(isn_param_t *param)
{
  DPRINTF( ("OP_INC\n") );
  DPRINTF( ("not yet implemented\n") );
  return -1;
}

static int ovm_dec(isn_param_t *param)
{
  DPRINTF( ("OP_DEC\n") );
  DPRINTF( ("not yet implemented\n") );
  return -1;
}

static int ovm_and(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  ovm_var_t *op1;
  ovm_var_t *op2;
  ovm_var_t *res = NULL;

  DebugLog(DF_OVM, DS_DEBUG, "OP_AND\n");
  op2 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  op1 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  param->ip += 1;
  res = issdl_and(ctx->gc_ctx, op1, op2);
  STACK_DROP(ctx->ovm_stack, 2);
  PUSH_VALUE(ctx, res);
  return 0;
}

static int ovm_or(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  ovm_var_t *op1;
  ovm_var_t *op2;
  ovm_var_t *res = NULL;

  DebugLog(DF_OVM, DS_DEBUG, "OP_OR\n");
  op2 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  op1 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  param->ip += 1;
  res = issdl_or(ctx->gc_ctx, op1, op2);
  STACK_DROP(ctx->ovm_stack, 2);
  PUSH_VALUE(ctx, res);
  return 0;
}

static int ovm_xor(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  ovm_var_t *op1;
  ovm_var_t *op2;
  ovm_var_t *res = NULL;

  DebugLog(DF_OVM, DS_DEBUG, "OP_XOR\n");
  op2 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  op1 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  param->ip += 1;
  res = issdl_xor(ctx->gc_ctx, op1, op2);
  STACK_DROP(ctx->ovm_stack, 2);
  PUSH_VALUE(ctx, res);
  return 0;
}

static int ovm_neg(isn_param_t *param)
{
  DPRINTF( ("OP_NEG\n") );
  DPRINTF( ("unused opcode, see OP_OPP\n") );
  return -1;
}

static int ovm_not(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  ovm_var_t *op;
  ovm_var_t *res = NULL;

  DebugLog(DF_OVM, DS_DEBUG, "OP_NOT\n");
  op = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  param->ip += 1;
  res = issdl_not(ctx->gc_ctx, op);
  STACK_DROP(ctx->ovm_stack, 1);
  PUSH_VALUE(ctx, res);
  return 0;
}

static int ovm_jmp(isn_param_t *param)
{
  DebugLog(DF_OVM, DS_DEBUG, "OP_JMP\n");
  param->ip += param->ip[1]+2;
  return 0;
}

static int ovm_popcjmp(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  ovm_var_t *res;

  DebugLog(DF_OVM, DS_DEBUG, "OP_POPCJMP\n");
  res = POP_VALUE(ctx);
#if 0
  if (res!=NULL) /* Formerly, we used issdl_test(); inlining is faster */
    switch (TYPE(res))
      {
      case T_INT:
	if (INT(res)!=0)
	  {
	    param->ip += param->ip[1];
	    return 0;
	  }
	break;
      case T_UINT:
	if (UINT(res)!=0)
	  {
	    param->ip += param->ip[1];
	    return 0;
	  }
	break;
      default:
	param->ip += param->ip[1];
	return 0;
      }
#else
  /* Type-checker guarantees res is NULL or a T_INT */
  if (res!=NULL && INT(res)!=0)
    {
      param->ip += param->ip[1];
      return 0;
    }
#endif
  param->ip += 2;
  return 0;
}

static int ovm_ceq(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  ovm_var_t *op1;
  ovm_var_t *op2;
  ovm_var_t *res = NULL;
  int ret = 1; // False by default

  DebugLog(DF_OVM, DS_DEBUG, "OP_CEQ\n");
  op2 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  op1 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  param->ip += 1;
  if (!IS_NULL(op1) && !IS_NULL(op2))
    {
      ret = issdl_cmp(op1, op2);
      res = (ret) ? FALSE_VAR : TRUE_VAR;
      if (!ret)
	DebugLog(DF_OVM, DS_TRACE, "OP_CEQ: true\n");
    }
  else
    res = NULL;
  STACK_DROP(ctx->ovm_stack, 2);
  PUSH_VALUE(ctx,res);
  return 0;
}

static int ovm_cneq(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  ovm_var_t *op1;
  ovm_var_t *op2;
  ovm_var_t *res;
  int ret = 1;

  DebugLog(DF_OVM, DS_DEBUG, "OP_CNEQ\n");
  op2 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  op1 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  param->ip += 1;
  if (!IS_NULL(op1) && !IS_NULL(op2))
    {
      ret = issdl_cmp(op1, op2);
      res = (ret) ? TRUE_VAR : FALSE_VAR;
      if (ret)
	DebugLog(DF_OVM, DS_TRACE, "OP_CNEQ: true\n");
    }
  else
    res = NULL;
  STACK_DROP(ctx->ovm_stack, 2);
  PUSH_VALUE(ctx,res);
  return 0;
}

static int my_regvexec(gc_t *gc_ctx,
		       const regex_t *preg,
		       ovm_var_t *v,
		       size_t nmatch,
		       regmatch_t *pmatch,
		       int eflags)
{
  int ret;
#ifdef HAVE_REGNEXEC
  char *s;
  size_t len;

  switch (TYPE(string))
    {
    case T_STR: s = STR(v); len = STRLEN(v); break;
    case T_VSTR: s = VSTR(v); len = VSTRLEN(v); break;
    default: return 0;
    }
  ret = regnexec(preg, s, len, nmatch, eflags);
#else
  char *s;

  s = ovm_strdup (gc_ctx, v);
  if (s!=NULL)
    {
      ret = regexec(preg, s, nmatch, pmatch, eflags);
      gc_base_free (s);
    }
  else ret = REG_NOMATCH;
#endif
  return ret;
}

static int ovm_crm(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  ovm_var_t *string;
  ovm_var_t *regex;
  ovm_var_t *res;
  int ret;
  regmatch_t match[1];

  DebugLog(DF_OVM, DS_DEBUG, "OP_CRM\n");
  regex = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  string = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  param->ip += 1;
  if (IS_NULL(regex) || IS_NULL(string))
    res = NULL;
  else if ((TYPE(regex) != T_REGEX) ||
	   ((TYPE(string) != T_STR) && TYPE(string) != T_VSTR))
    res = NULL; /* PARAM_ERROR_VAR */
  else
    {
      DebugLog(DF_OVM, DS_DEBUG, "OP_CRM\n");
      ret = my_regvexec(ctx->gc_ctx, &REGEX(regex), string, 0, match, 0);
      if (ret == REG_NOMATCH)
	{
	  DebugLog(DF_OVM, DS_DEBUG, "OP_CRM (false)\n");
	  res = FALSE_VAR;
	}
      else if (ret != 0)
	{
	  char err_buf[64];

	  regerror(ret, &(REGEX(regex)), err_buf, sizeof (err_buf));
	  DebugLog(DF_OVM, DS_ERROR, "regexec error (%s)\n", err_buf);
	  res = NULL; /* REGEX_ERROR_VAR; */
	}
      else
	{
	  DebugLog(DF_OVM, DS_DEBUG, "OP_CRM (true)\n");
	  res = TRUE_VAR;
	}
    }
  STACK_DROP(ctx->ovm_stack, 2);
  PUSH_VALUE(ctx,res);
  return 0;
}

static int ovm_cnrm(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  ovm_var_t *string;
  ovm_var_t *regex;
  ovm_var_t *res;
  int ret;
  regmatch_t match[1];

  DebugLog(DF_OVM, DS_DEBUG, "OP_CNRM\n");
  regex = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  string = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  param->ip += 1;

  if (IS_NULL(regex) || IS_NULL(string))
    res = NULL;
  else if ((TYPE(regex) != T_REGEX) ||
	   ((TYPE(string) != T_STR) && TYPE(string) != T_VSTR))
    res = NULL; /* PARAM_ERROR_VAR */
  else
    {
      DebugLog(DF_OVM, DS_DEBUG, "OP_CNRM\n");
      ret = my_regvexec(ctx->gc_ctx, &REGEX(regex), string, 0, match, 0);
      if (ret == 0)
	{
	  DebugLog(DF_OVM, DS_DEBUG, "OP_CNRM (false)\n");
	  res = FALSE_VAR;
	}
      else if (ret != REG_NOMATCH)
	{
	  char err_buf[64];

	  regerror(ret, &(REGEX(regex)), err_buf, sizeof (err_buf));
	  DebugLog(DF_OVM, DS_ERROR, "regexec eror (%s)\n", err_buf);
	  res = NULL; /* REGEX_ERROR_VAR; */
	}
      else
	{
	  DebugLog(DF_OVM, DS_DEBUG, "OP_CNRM (true)\n");
	  res = TRUE_VAR;
	}
    }
  STACK_DROP(ctx->ovm_stack, 2);
  PUSH_VALUE(ctx,res);
  return 0;
}

static int ovm_clt(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  ovm_var_t *op1;
  ovm_var_t *op2;
  ovm_var_t *res;
  int ret = 0; //False by default

  DebugLog(DF_OVM, DS_DEBUG, "OP_CLT\n");
  op2 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  op1 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  param->ip += 1;
  if (!IS_NULL(op1) && !IS_NULL(op2))
    {
      ret = issdl_cmp(op1, op2);
      if (ret >= 0)
	{
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
    res = NULL;
  STACK_DROP(ctx->ovm_stack, 2);
  PUSH_VALUE(ctx,res);
  return 0;
}

static int ovm_cgt(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  ovm_var_t *op1;
  ovm_var_t *op2;
  ovm_var_t *res;
  int ret = 0; // False by default

  DebugLog(DF_OVM, DS_DEBUG, "OP_CGT\n");
  op2 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  op1 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  param->ip += 1;
  if (!IS_NULL(op1) && !IS_NULL(op2))
    {
      ret = issdl_cmp(op1, op2);
      if (ret <= 0)
	{
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
    res = NULL;
  STACK_DROP(ctx->ovm_stack, 2);
  PUSH_VALUE(ctx,res);
  return 0;
}

static int ovm_cle(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  ovm_var_t *op1;
  ovm_var_t *op2;
  ovm_var_t *res;
  int ret = 1; // False by default

  DebugLog(DF_OVM, DS_DEBUG, "OP_CLE\n");
  op2 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  op1 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  param->ip += 1;
  if (!IS_NULL(op1) && !IS_NULL(op2))
    {
      ret = issdl_cmp(op1, op2);
      if (ret > 0)
	{
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
    res = NULL;
  STACK_DROP(ctx->ovm_stack, 2);
  PUSH_VALUE(ctx,res);
  return 0;
}

static int ovm_cge(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  ovm_var_t *op1;
  ovm_var_t *op2;
  ovm_var_t *res;
  int ret = -1; // False by default

  DebugLog(DF_OVM, DS_DEBUG, "OP_CGE\n");
  op2 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  op1 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  param->ip += 1;
  if (!IS_NULL(op1) && !IS_NULL(op2))
    {
      ret = issdl_cmp(op1, op2);
      if (ret < 0)
	{
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
    res = NULL;
  STACK_DROP(ctx->ovm_stack, 2);
  PUSH_VALUE(ctx,res);
  return 0;
}

static int ovm_regsplit(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  ovm_var_t *string;
  ovm_var_t *pattern;
  int ret;
  int i;
  regmatch_t match[10];

  DebugLog(DF_OVM, DS_DEBUG, "OP_REGSPLIT\n");
  pattern = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  string = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  param->ip += 1;
  if (IS_NULL(pattern) && IS_NULL(string))
    {
      DebugLog(DF_OVM, DS_ERROR, "op error\n");
      STACK_DROP(ctx->ovm_stack, 2);
    }
  else if ((TYPE(pattern) != T_REGEX) ||
	   (TYPE(string) != T_STR && TYPE(string) != T_VSTR))
    {
      DebugLog(DF_OVM, DS_ERROR, "op type error\n");
      STACK_DROP(ctx->ovm_stack, 2);
    }
  else
    {
      ret = my_regvexec(ctx->gc_ctx, &REGEX(pattern), string,
			REGEXNUM(pattern) + 1, match, 0);
      if (ret)
	{
	  char err_buf[64];

	  regerror(ret, &(REGEX(pattern)), err_buf, sizeof (err_buf));
	  DebugLog(DF_OVM, DS_DEBUG, "regexec error (%s)\n", err_buf);
	  STACK_DROP(ctx->ovm_stack, 2);
	}
      else
	{
	  int n = REGEXNUM(pattern);
	  char *s = (TYPE(string)==T_STR)?STR(string):VSTR(string);

	  GC_START_PROGRESSIVE(ctx->gc_ctx, 1);
	  GC_PROTECT_PROGRESSIVE(ctx->gc_ctx, (gc_header_t *)string);
	  STACK_DROP(ctx->ovm_stack, 2);
	  for (i = n; i > 0; i--)
	    {
	      size_t res_sz;
	      ovm_var_t *res;

	      /*       DPRINTF( ("rm_so[%i]=%i rm_eo[%i]=%i\n", i, match[i].rm_so, i, match[i].rm_eo) ); */
	      res_sz = match[i].rm_eo - match[i].rm_so;
	      res = ovm_str_new(ctx->gc_ctx, res_sz);
	      memcpy(STR(res), &(s[match[i].rm_so]), res_sz);
	      PUSH_VALUE(param->ctx, res);
	    }
	  GC_END_PROGRESSIVE(ctx->gc_ctx);
	}
    }
  return 0;
}

static int ovm_cesv(isn_param_t *param)
{
  DPRINTF( ("OP_CESV\n") );
  DPRINTF( ("not yet implemented\n") );
  return -1;
}

static int ovm_empty_event(isn_param_t *param)
{
  DebugLog(DF_OVM, DS_DEBUG, "OP_EMPTY_EVENT\n");
  PUSH_VALUE(param->ctx, NULL);
  return 0;
}

static int ovm_add_event(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  bytecode_t op = param->ip[1];
  ovm_var_t *val;
  event_t *evt;
  event_t *res = NULL;

  DebugLog(DF_OVM, DS_DEBUG, "OP_ADD_EVENT\n");
  val = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  evt = (event_t *)STACK_ELT(ctx->ovm_stack, 2);
  param->ip += 2;
  if (evt!=NULL && TYPE(evt)!=T_EVENT)
    {
      DebugLog(DF_OVM, DS_DEBUG, "Type error\n");
    }
  else
    {
      res = new_event (ctx->gc_ctx, op, val, evt);
    }
  STACK_DROP(ctx->ovm_stack, 2);
  PUSH_VALUE(ctx, res);
  return 0;
}


static ovm_insn_rec_t ops_g[] = {
  { NULL,           1, "end"        },
  { ovm_nop,        1, "nop"        },
  { ovm_push,       2, "push"       },
  { ovm_pop,        2, "pop"        },
  { ovm_push_zero,  1, "pushzero"   },
  { ovm_push_one,   1, "pushone"    },
  { ovm_pushstatic, 2, "pushstatic" },
  { ovm_pushfield,  2, "pushfield"  },
  { ovm_trash,      0, "trash"      },
  { ovm_call,       0, "call"       },
  { ovm_add,        0, "add"        },
  { ovm_sub,        0, "sub"        },
  { ovm_opp,        0, "opp"        },
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
  { ovm_empty_event, 1, "emptyevent" },
  { ovm_add_event,  2, "addevent" },
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
  "pushzero",
  "pushone",
  "pushstatic",
  "pushfield",
  "trash",
  "call",
  "add",
  "sub",
  "opp",
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
  NULL
};


const char *get_opcode_name(bytecode_t opcode)
{
  if (opcode >= OPCODE_NUM)
    return NULL;
  return op_name[opcode];
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
