/**
 ** @file ovm.c
 ** Orchids 'virtual machine'
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 ** @author Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
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
#include "db.h"

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

#ifdef OBSOLETE
int ovm_exec_expr(orchids_t *ctx, state_instance_t *s, bytecode_t *bytecode)
{
  ovm_var_t *res;

  ovm_exec_base(ctx, s, bytecode);
  res = POP_VALUE(ctx);
  ovm_flush(ctx);
  if ((!(IS_NULL(res))) && TYPE(res) == T_INT)
    return INT(res);
  else
    return 0;
}
#endif

int ovm_exec_trans_cond (orchids_t *ctx, state_instance_t *s, bytecode_t *bytecode)
{
  ovm_var_t *res;

  ovm_exec_base(ctx, s, bytecode);
  res = POP_VALUE(ctx); /* res is guaranteed to be a T_INT */
  ovm_flush(ctx);
  return (IS_NULL(res))?0:INT(res);
}

int review_bytecode(bytecode_t *bytecode, size_t length,
		    int (*review) (bytecode_t *bc, size_t sz,
				   bytecode_t *start,
				   void *data),
		    void *data)
{
  bytecode_t *code, *end;
  size_t sz;
  unsigned long nfields;
  int res;

  code = bytecode;
  end = bytecode + length;
  while (code < end)
    {
      switch (*code)
	{
	case OP_END:
	  /*FALLTHROUGH*/
	case OP_NOP:
	case OP_PUSHZERO:
	case OP_PUSHMINUSONE:
	case OP_PUSHONE:
	case OP_PUSHNULL:
	case OP_TRASH:
	case OP_TRASH2:
	case OP_ADD:
	case OP_SUB:
	case OP_OPP:
	case OP_PLUS:
	case OP_MUL:
	case OP_DIV:
	case OP_MOD:
	case OP_AND:
	case OP_OR:
	case OP_XOR:
	case OP_NOT:
	case OP_CEQ:
	case OP_CNEQ:
	case OP_CRM:
	case OP_CNRM:
	case OP_CLT:
	case OP_CGT:
	case OP_CLE:
	case OP_CGE:
	case OP_REGSPLIT:
	case OP_CESV:
	case OP_EMPTY_EVENT:
	  sz = 1;
	  break;
	case OP_PUSH:
	case OP_POP:
	case OP_PUSHSTATIC:
	case OP_PUSHFIELD:
	case OP_CALL:
	case OP_JMP:
	case OP_POPCJMP:
	case OP_CEQJMP:
	case OP_CEQJMP_OPPOSITE:
	case OP_CNEQJMP:
	case OP_CNEQJMP_OPPOSITE:
	case OP_CRMJMP:
	case OP_CRMJMP_OPPOSITE:
	case OP_CNRMJMP:
	case OP_CNRMJMP_OPPOSITE:
	case OP_CGTJMP:
	case OP_CGTJMP_OPPOSITE:
	case OP_CLTJMP:
	case OP_CLTJMP_OPPOSITE:
	case OP_CGEJMP:
	case OP_CGEJMP_OPPOSITE:
	case OP_CLEJMP:
	case OP_CLEJMP_OPPOSITE:
	case OP_ADD_EVENT:
	case OP_DB_SINGLE:
	case OP_DUP:
	  sz = 2;
	  break;
	case OP_DB_FILTER:
	  if (code+2 >= end)
	    return -1;
	  nfields = code[2];
	  sz = 4+nfields;
	  break;
	case OP_DB_JOIN:
	  if (code+2 >= end)
	    return -1;
	  nfields = code[2];
	  sz = 3+nfields;
	  break;
	case OP_DB_PROJ:
	  if (code+1 >= end)
	    return -1;
	  nfields = code[1];
	  sz = 3+nfields;
	  break;
	case OP_DB_MAP:
	  sz = 3;
	  break;
	case OP_INC:
	case OP_DEC:
	case OP_NEG:
	  /* not implemented */
	default:
	  return -1; /* unknown opcode */
	}
      if (code+sz > end)
	return -1;
      res = (*review) (code, sz, bytecode, data);
      if (res)
	return res;
      code += sz;
    }
  return 0;
}

static int do_review_fprintf (bytecode_t *bc, size_t sz,
			      bytecode_t *start, void *data)
{
  FILE *fp = data;

  switch (sz)
    {
    case 1:
      fprintf (fp, "0x%04lx: %08lx            | %s\n", bc-start, bc[0],
	       ops_g[bc[0]].name);
      break;
    case 2:
      fprintf(fp, "0x%04lx: %08lx %08lx    | %s [%lu] \n",
	      bc-start, bc[0], bc[1], ops_g[bc[0]].name, 
	      bc[1]);
      break;
    default:
      switch (bc[0])
	{
	case OP_DB_FILTER:
	  {
	    int nfields = bc[2];
	    unsigned long val;
	    int i;
	    char *delim;

	    fprintf(fp, "0x%04lx: %08x ...         | "
		    "db_filter [nfields_res=%lu, nfields=%lu, nconsts=%lu] [",
		    bc-start, OP_DB_FILTER, bc[1], bc[2], bc[3]);
	    delim = "";
	    for (i=0; i<nfields; i++)
	      {
		fputs (delim, fp);
		val = bc[4+i];
		if (val==DB_VAR_NONE)
		  fprintf (fp, "_");
		else if (DB_IS_CST(val))
		  fprintf (fp, "cst %lu", DB_CST(val));
		else fprintf (fp, "var %lu", DB_VAR(val));
		delim = ",";
	      }
	    fputs ("]\n", fp);
	    break;
	  }
	case OP_DB_JOIN:
	  {
	    int nfields2 = bc[2];
	    int i;
	    char *delim;
	    unsigned long val;

	    fprintf(fp, "0x%04lx: %08x ...         | "
		    "db_join [nfields1=%lu, nfields2=%lu] [",
	      bc-start, OP_DB_JOIN, bc[1], bc[2]);
	    delim = "";
	    for (i=0; i<nfields2; i++)
	      {
		fputs (delim, fp);
		val = bc[3+i];
		if (val==DB_VAR_NONE)
		  fprintf (fp, "_");
		else if (DB_IS_CST(val))
		  fprintf (fp, "cst %lu", DB_CST(val));
		else fprintf (fp, "var %lu", DB_VAR(val));
		delim = ",";
	      }
	    fputs ("]\n", fp);
	    break;
	  }
	case OP_DB_PROJ:
	  {
	    int nfields_res = bc[1];
	    unsigned long val;
	    int i;
	    char *delim;

	    fprintf(fp, "0x%04lx: %08x ...         | "
		    "db_proj [nfields_res=%lu, nconsts=%lu] [",
		    bc-start, OP_DB_PROJ, bc[1], bc[2]);
	    delim = "";
	    for (i=0; i<nfields_res; i++)
	      {
		fputs (delim, fp);
		val = bc[3+i];
		if (val==DB_VAR_NONE)
		  fprintf (fp, "_");
		else if (DB_IS_CST(val))
		  fprintf (fp, "cst %lu", DB_CST(val));
		else fprintf (fp, "var %lu", DB_VAR(val));
		delim = ",";
	      }
	    fputs ("]\n", fp);
	    break;
	  }
	case OP_DB_MAP:
	  fprintf(fp, "0x%04lx: %08x %08lx %08lx | db_map [%lu] [%lu]\n",
		  bc-start, OP_DB_MAP, bc[1], bc[2], bc[1], bc[2]);
	  break;
	default:
	  return -1;
	}
      break;
    }
  return 0;
}

void fprintf_bytecode(FILE *fp, bytecode_t *bytecode, size_t length)
{
  (void) review_bytecode (bytecode, length, do_review_fprintf, fp);
}

static int do_review_fprintf_short (bytecode_t *bc, size_t sz,
				    bytecode_t *start, void *data)
{
  FILE *fp = data;

  switch (sz)
    {
    case 1:
      fprintf (fp, "0x%04lx: %02lx       | %s\n", bc-start, bc[0],
	       ops_g[bc[0]].name);
      break;
    case 2:
      fprintf(fp, "0x%04lx: %02lx %02lx    | %s [%lu] \n",
	      bc-start, bc[0], bc[1], ops_g[bc[0]].name, 
	      bc[1]);
      break;
    default:
      switch (bc[0])
	{
	case OP_DB_FILTER:
	  {
	    int nfields = bc[2];
	    unsigned long val;
	    int i;
	    char *delim;

	    fprintf(fp, "0x%04lx: %02x ...   | "
		    "db_filter [nfields_res=%lu, nfields=%lu, nconsts=%lu] [",
		    bc-start, OP_DB_FILTER, bc[1], bc[2], bc[3]);
	    delim = "";
	    for (i=0; i<nfields; i++)
	      {
		fputs (delim, fp);
		val = bc[4+i];
		if (val==DB_VAR_NONE)
		  fprintf (fp, "_");
		else if (DB_IS_CST(val))
		  fprintf (fp, "cst %lu", DB_CST(val));
		else fprintf (fp, "var %lu", DB_VAR(val));
		delim = ",";
	      }
	    fputs ("]\n", fp);
	    break;
	  }
	case OP_DB_JOIN:
	  {
	    int nfields2 = bc[2];
	    int i;
	    char *delim;
	    unsigned long val;

	    fprintf(fp, "0x%04lx: %02x ...   | "
		    "db_join [nfields1=%lu, nfields2=%lu] [",
	      bc-start, OP_DB_JOIN, bc[1], bc[2]);
	    delim = "";
	    for (i=0; i<nfields2; i++)
	      {
		fputs (delim, fp);
		val = bc[3+i];
		if (val==DB_VAR_NONE)
		  fprintf (fp, "_");
		else if (DB_IS_CST(val))
		  fprintf (fp, "cst %lu", DB_CST(val));
		else fprintf (fp, "var %lu", DB_VAR(val));
		delim = ",";
	      }
	    fputs ("]\n", fp);
	    break;
	  }
	case OP_DB_PROJ:
	  {
	    int nfields_res = bc[1];
	    unsigned long val;
	    int i;
	    char *delim;

	    fprintf(fp, "0x%04lx: %02x ...   | "
		    "db_proj [nfields_res=%lu, nconsts=%lu] [",
		    bc-start, OP_DB_PROJ, bc[1], bc[2]);
	    delim = "";
	    for (i=0; i<nfields_res; i++)
	      {
		fputs (delim, fp);
		val = bc[3+i];
		if (val==DB_VAR_NONE)
		  fprintf (fp, "_");
		else if (DB_IS_CST(val))
		  fprintf (fp, "cst %lu", DB_CST(val));
		else fprintf (fp, "var %lu", DB_VAR(val));
		delim = ",";
	      }
	    fputs ("]\n", fp);
	    break;
	  }
	case OP_DB_MAP:
	  fprintf(fp, "0x%04lx: %02x %02lx %02lx | db_map [%lu] [%lu]\n",
		  bc-start, OP_DB_MAP, bc[1], bc[2], bc[1], bc[2]);
	  break;
	default:
	  return -1;
	}
      break;
    }
  return 0;
}

void fprintf_bytecode_short(FILE *fp, bytecode_t *bytecode, size_t length)
{
  (void) review_bytecode (bytecode, length, do_review_fprintf_short, fp);
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
           "OP_PUSH [%02lx] (%s)\n",
            op, state->q->rule->var_name[op]);

  res = ovm_read_value (state->env, op);
  PUSH_VALUE(param->ctx,res);
  param->ip += 2;
  return 0;
}

static int ovm_pop(isn_param_t *param)
{
  ovm_var_t *env;
  ovm_var_t *val;
  state_instance_t *state = param->state;
  bytecode_t op = param->ip[1];
  orchids_t *ctx = param->ctx;
  gc_t *gc_ctx = ctx->gc_ctx;

  DebugLog(DF_OVM, DS_DEBUG, "OP_POP [%02lx] (%s)\n",
            op, state->q->rule->var_name[op]);
  val = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  env = ovm_write_value (gc_ctx, state->env, op, val);
  GC_TOUCH (gc_ctx, state->env = env);
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

static int ovm_push_minus_one(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;

  DebugLog(DF_OVM, DS_DEBUG, "OP_PUSHMINUSONE\n");
  PUSH_RETURN_MINUS_ONE(ctx);
  param->ip += 1;
  return 0;
}

static int ovm_pushstatic(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  state_instance_t *state = param->state;
  bytecode_t op = param->ip[1];

  DebugLog(DF_OVM, DS_DEBUG, "OP_PUSHSTATIC [%lu]\n", op);
  PUSH_VALUE (ctx, state->q->rule->static_env[op]);
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
  issdl_function_t *f;

  f = &ctx->vm_func_tbl[op];
  DebugLog(DF_OVM, DS_DEBUG, "OP_CALL [%02lx] (%s)\n", param->ip[1],
	   f->name);
  /* Check call table boundary */
  (*f->func) (ctx, state, f->data);
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
	    param->ip += param->ip[1]+2;
	    return 0;
	  }
	break;
      case T_UINT:
	if (UINT(res)!=0)
	  {
	    param->ip += param->ip[1]+2;
	    return 0;
	  }
	break;
      default:
	param->ip += param->ip[1]+2;
	return 0;
      }
#else
  /* Type-checker guarantees res is NULL or a T_INT */
  if (res!=NULL && INT(res)!=0)
    {
      param->ip += param->ip[1]+2;
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
      ret = issdl_cmp(op1, op2, CMP_LEQ_MASK | CMP_GEQ_MASK);
      if (CMP_EQUAL(ret))
	{
	  res = TRUE_VAR;
	  DebugLog(DF_OVM, DS_TRACE, "OP_CEQ: true\n");
	}
      else res = FALSE_VAR;
    }
  else
    res = NULL;
  STACK_DROP(ctx->ovm_stack, 2);
  PUSH_VALUE(ctx,res);
  return 0;
}

static int ovm_ceqjmp(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  ovm_var_t *op1;
  ovm_var_t *op2;
  int ret = 1; // False by default

  DebugLog(DF_OVM, DS_DEBUG, "OP_CEQJMP\n");
  op2 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  op1 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  if (!IS_NULL(op1) && !IS_NULL(op2))
    {
      ret = issdl_cmp(op1, op2, CMP_LEQ_MASK | CMP_GEQ_MASK);
      if (CMP_EQUAL(ret))
	param->ip += param->ip[1]+2;
      else param->ip += 2;
    }
  else
    param->ip += 2;
  STACK_DROP(ctx->ovm_stack, 2);
  return 0;
}

static int ovm_ceqjmp_opposite(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  ovm_var_t *op1;
  ovm_var_t *op2;
  int ret = 1; // False by default

  DebugLog(DF_OVM, DS_DEBUG, "OP_CEQJMP_OPPOSITE\n");
  op2 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  op1 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  if (!IS_NULL(op1) && !IS_NULL(op2))
    {
      ret = issdl_cmp(op1, op2, CMP_LEQ_MASK | CMP_GEQ_MASK);
      if (CMP_EQUAL(ret))
	param->ip += 2;
      else param->ip += param->ip[1]+2;
    }
  else
    param->ip += param->ip[1]+2;
  STACK_DROP(ctx->ovm_stack, 2);
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
      ret = issdl_cmp(op1, op2, CMP_LEQ_MASK | CMP_GEQ_MASK);
      if (CMP_DIFFERENT(ret))
	{
	  res = TRUE_VAR;
	  DebugLog(DF_OVM, DS_TRACE, "OP_CNEQ: true\n");
	}
      else res = FALSE_VAR;
    }
  else
    res = NULL;
  STACK_DROP(ctx->ovm_stack, 2);
  PUSH_VALUE(ctx,res);
  return 0;
}

static int ovm_cneqjmp(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  ovm_var_t *op1;
  ovm_var_t *op2;
  int ret = 1; // False by default

  DebugLog(DF_OVM, DS_DEBUG, "OP_CNEQJMP\n");
  op2 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  op1 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  if (!IS_NULL(op1) && !IS_NULL(op2))
    {
      ret = issdl_cmp(op1, op2, CMP_LEQ_MASK | CMP_GEQ_MASK);
      if (CMP_EQUAL(ret))
	param->ip += 2;
      else param->ip += param->ip[1]+2;
    }
  else
    param->ip += 2;
  STACK_DROP(ctx->ovm_stack, 2);
  return 0;
}

static int ovm_cneqjmp_opposite(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  ovm_var_t *op1;
  ovm_var_t *op2;
  int ret = 1; // False by default

  DebugLog(DF_OVM, DS_DEBUG, "OP_CNEQJMP_OPPOSITE\n");
  op2 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  op1 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  if (!IS_NULL(op1) && !IS_NULL(op2))
    {
      ret = issdl_cmp(op1, op2, CMP_LEQ_MASK | CMP_GEQ_MASK);
      if (CMP_EQUAL(ret))
	param->ip += param->ip[1]+2;
      else param->ip += 2;
    }
  else
    param->ip += param->ip[1]+2;
  STACK_DROP(ctx->ovm_stack, 2);
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

  switch (TYPE(v))
    {
    case T_STR: s = STR(v); len = STRLEN(v); break;
    case T_VSTR: s = VSTR(v); len = VSTRLEN(v); break;
    default: return 0;
    }
  ret = regnexec(preg, s, len, nmatch, pmatch, eflags);
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

static int ovm_crmjmp(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  ovm_var_t *string;
  ovm_var_t *regex;
  int ret = 1; // False by default
  regmatch_t match[1];

  DebugLog(DF_OVM, DS_DEBUG, "OP_CRMJMP\n");
  regex = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  string = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  if (!IS_NULL(regex) && !IS_NULL(string) && TYPE(regex)==T_REGEX &&
      (TYPE(string)==T_STR || TYPE(string)==T_VSTR))
    {
      ret = my_regvexec(ctx->gc_ctx, &REGEX(regex), string, 0, match, 0);
      if (ret == REG_NOMATCH)
	param->ip += 2;
      else if (ret != 0) /* regexec error */
	{
	  char err_buf[64];

	  regerror(ret, &(REGEX(regex)), err_buf, sizeof (err_buf));
	  DebugLog(DF_OVM, DS_ERROR, "regexec error (%s)\n", err_buf);
	  param->ip += 2;
	}
      else
	param->ip += param->ip[1]+2;
    }
  else
    param->ip += 2;
  STACK_DROP(ctx->ovm_stack, 2);
  return 0;
}

static int ovm_crmjmp_opposite(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  ovm_var_t *string;
  ovm_var_t *regex;
  int ret = 1; // False by default
  regmatch_t match[1];

  DebugLog(DF_OVM, DS_DEBUG, "OP_CRMJMP_OPPOSITE\n");
  regex = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  string = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  if (!IS_NULL(regex) && !IS_NULL(string) && TYPE(regex)==T_REGEX &&
      (TYPE(string)==T_STR || TYPE(string)==T_VSTR))
    {
      ret = my_regvexec(ctx->gc_ctx, &REGEX(regex), string, 0, match, 0);
      if (ret == REG_NOMATCH)
	param->ip += param->ip[1]+2;
      else if (ret != 0) /* regexec error */
	{
	  char err_buf[64];

	  regerror(ret, &(REGEX(regex)), err_buf, sizeof (err_buf));
	  DebugLog(DF_OVM, DS_ERROR, "regexec error (%s)\n", err_buf);
	  param->ip += param->ip[1]+2;
	}
      else
	param->ip += 2;
    }
  else
    param->ip += param->ip[1]+2;
  STACK_DROP(ctx->ovm_stack, 2);
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
	  res = NULL;
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

static int ovm_cnrmjmp(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  ovm_var_t *string;
  ovm_var_t *regex;
  int ret = 1; // False by default
  regmatch_t match[1];

  DebugLog(DF_OVM, DS_DEBUG, "OP_CNRMJMP\n");
  regex = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  string = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  if (!IS_NULL(regex) && !IS_NULL(string) && TYPE(regex)==T_REGEX &&
      (TYPE(string)==T_STR || TYPE(string)==T_VSTR))
    {
      ret = my_regvexec(ctx->gc_ctx, &REGEX(regex), string, 0, match, 0);
      if (ret == REG_NOMATCH)
	param->ip += param->ip[1]+2;
      else if (ret != 0) /* regexec error */
	{
	  char err_buf[64];

	  regerror(ret, &(REGEX(regex)), err_buf, sizeof (err_buf));
	  DebugLog(DF_OVM, DS_ERROR, "regexec error (%s)\n", err_buf);
	  param->ip += 2;
	}
      else
	param->ip += 2;
    }
  else
    param->ip += 2;
  STACK_DROP(ctx->ovm_stack, 2);
  return 0;
}

static int ovm_cnrmjmp_opposite(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  ovm_var_t *string;
  ovm_var_t *regex;
  int ret = 1; // False by default
  regmatch_t match[1];

  DebugLog(DF_OVM, DS_DEBUG, "OP_CNRMJMP\n");
  regex = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  string = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  if (!IS_NULL(regex) && !IS_NULL(string) && TYPE(regex)==T_REGEX &&
      (TYPE(string)==T_STR || TYPE(string)==T_VSTR))
    {
      ret = my_regvexec(ctx->gc_ctx, &REGEX(regex), string, 0, match, 0);
      if (ret == REG_NOMATCH)
	param->ip += 2;
      else if (ret != 0) /* regexec error */
	{
	  char err_buf[64];

	  regerror(ret, &(REGEX(regex)), err_buf, sizeof (err_buf));
	  DebugLog(DF_OVM, DS_ERROR, "regexec error (%s)\n", err_buf);
	  param->ip += param->ip[1]+2;
	}
      else
	param->ip += param->ip[1]+2;
    }
  else
    param->ip += param->ip[1]+2;
  STACK_DROP(ctx->ovm_stack, 2);
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
      ret = issdl_cmp(op1, op2, CMP_LEQ_MASK | CMP_GEQ_MASK);
      if (CMP_LESS(ret))
	{
	  DebugLog(DF_OVM, DS_DEBUG, "OP_CLT (true ret=%i)\n", ret);
	  res = TRUE_VAR;
	}
      else
	{
	  DebugLog(DF_OVM, DS_DEBUG, "OP_CLT (false ret=%i)\n", ret);
	  res = FALSE_VAR;
	}
    }
  else
    res = NULL;
  STACK_DROP(ctx->ovm_stack, 2);
  PUSH_VALUE(ctx,res);
  return 0;
}

static int ovm_cltjmp(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  ovm_var_t *op1;
  ovm_var_t *op2;
  int ret = 0; //False by default

  DebugLog(DF_OVM, DS_DEBUG, "OP_CLTJMP\n");
  op2 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  op1 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  if (!IS_NULL(op1) && !IS_NULL(op2))
    {
      ret = issdl_cmp(op1, op2, CMP_LEQ_MASK | CMP_GEQ_MASK);
      if (CMP_LESS(ret))
	param->ip += param->ip[1]+2;
      else param->ip += 2;
    }
  else
    param->ip += 2;
  STACK_DROP(ctx->ovm_stack, 2);
  return 0;
}

static int ovm_cltjmp_opposite(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  ovm_var_t *op1;
  ovm_var_t *op2;
  int ret = 0; //False by default

  DebugLog(DF_OVM, DS_DEBUG, "OP_CLTJMP\n");
  op2 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  op1 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  if (!IS_NULL(op1) && !IS_NULL(op2))
    {
      ret = issdl_cmp(op1, op2, CMP_LEQ_MASK | CMP_GEQ_MASK);
      if (CMP_LESS(ret))
	param->ip += 2;
      else param->ip += param->ip[1]+2;
    }
  else
    param->ip += param->ip[1]+2;
  STACK_DROP(ctx->ovm_stack, 2);
  return 0;
}

static int ovm_cgt(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  ovm_var_t *op1;
  ovm_var_t *op2;
  ovm_var_t *res;
  int ret = 0; //False by default

  DebugLog(DF_OVM, DS_DEBUG, "OP_CGT\n");
  op2 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  op1 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  param->ip += 1;
  if (!IS_NULL(op1) && !IS_NULL(op2))
    {
      ret = issdl_cmp(op1, op2, CMP_LEQ_MASK | CMP_GEQ_MASK);
      if (CMP_GREATER(ret))
	{
	  DebugLog(DF_OVM, DS_DEBUG, "OP_CGT (true ret=%i)\n", ret);
	  res = TRUE_VAR;
	}
      else
	{
	  DebugLog(DF_OVM, DS_DEBUG, "OP_CGT (false ret=%i)\n", ret);
	  res = FALSE_VAR;
	}
    }
  else
    res = NULL;
  STACK_DROP(ctx->ovm_stack, 2);
  PUSH_VALUE(ctx,res);
  return 0;
}

static int ovm_cgtjmp(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  ovm_var_t *op1;
  ovm_var_t *op2;
  int ret = 0; // False by default

  DebugLog(DF_OVM, DS_DEBUG, "OP_CGTJMP\n");
  op2 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  op1 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  if (!IS_NULL(op1) && !IS_NULL(op2))
    {
      ret = issdl_cmp(op1, op2, CMP_LEQ_MASK | CMP_GEQ_MASK);
      if (CMP_GREATER(ret))
	param->ip += param->ip[1]+2;
      else param->ip += 2;
    }
  else param->ip += 2;
  STACK_DROP(ctx->ovm_stack, 2);
  return 0;
}

static int ovm_cgtjmp_opposite(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  ovm_var_t *op1;
  ovm_var_t *op2;
  int ret = 0; // False by default

  DebugLog(DF_OVM, DS_DEBUG, "OP_CGTJMP_OPPOSITE\n");
  op2 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  op1 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  if (!IS_NULL(op1) && !IS_NULL(op2))
    {
      ret = issdl_cmp(op1, op2, CMP_LEQ_MASK | CMP_GEQ_MASK);
      if (CMP_GREATER(ret))
	param->ip += 2;
      else param->ip += param->ip[1]+2;
    }
  else param->ip += param->ip[1]+2;
  STACK_DROP(ctx->ovm_stack, 2);
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
      ret = issdl_cmp(op1, op2, CMP_LEQ_MASK);
      /* Don't need CMP_GEQ_MASK here, since anyway
	 whether this flag will be set or unset will be irrelevant.
	 This is important for databases, where just the inclusion
	 of op1 in op2 will be tested, using this mask;
	 using the CMP_LEQ_MASK | CMP_GEQ_MASK would also
	 test whether op2 is included in op1, wasting time for nothing
      */
      if (CMP_LEQ(ret))
	{
	  DebugLog(DF_OVM, DS_DEBUG, "OP_CLE (true ret=%i)\n", ret);
	  res = TRUE_VAR;
	}
      else
	{
	  DebugLog(DF_OVM, DS_DEBUG, "OP_CLE (false ret=%i)\n", ret);
	  res = FALSE_VAR;
	}
    }
  else
    res = NULL;
  STACK_DROP(ctx->ovm_stack, 2);
  PUSH_VALUE(ctx,res);
  return 0;
}

static int ovm_clejmp(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  ovm_var_t *op1;
  ovm_var_t *op2;
  int ret = 1; // False by default

  DebugLog(DF_OVM, DS_DEBUG, "OP_CLEJMP\n");
  op2 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  op1 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  if (!IS_NULL(op1) && !IS_NULL(op2))
    {
      ret = issdl_cmp(op1, op2, CMP_LEQ_MASK);
      /* Don't need CMP_GEQ_MASK here, since anyway
	 whether this flag will be set or unset will be irrelevant.
	 This is important for databases, where just the inclusion
	 of op1 in op2 will be tested, using this mask;
	 using the CMP_LEQ_MASK | CMP_GEQ_MASK would also
	 test whether op2 is included in op1, wasting time for nothing
      */
      if (CMP_LEQ(ret))
	param->ip += param->ip[1]+2;
      else param->ip += 2;
    }
  else param->ip += 2;
  STACK_DROP(ctx->ovm_stack, 2);
  return 0;
}

static int ovm_clejmp_opposite(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  ovm_var_t *op1;
  ovm_var_t *op2;
  int ret = 1; // False by default

  DebugLog(DF_OVM, DS_DEBUG, "OP_CLEJMP_OPPOSITE\n");
  op2 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  op1 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  if (!IS_NULL(op1) && !IS_NULL(op2))
    {
      ret = issdl_cmp(op1, op2, CMP_LEQ_MASK);
      /* Don't need CMP_GEQ_MASK here, since anyway
	 whether this flag will be set or unset will be irrelevant.
	 This is important for databases, where just the inclusion
	 of op1 in op2 will be tested, using this mask;
	 using the CMP_LEQ_MASK | CMP_GEQ_MASK would also
	 test whether op2 is included in op1, wasting time for nothing
      */
      if (CMP_LEQ(ret))
	param->ip += 2;
      else param->ip += param->ip[1]+2;
    }
  else param->ip += param->ip[1]+2;
  STACK_DROP(ctx->ovm_stack, 2);
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
      ret = issdl_cmp(op1, op2, CMP_GEQ_MASK);
      /* Don't need CMP_LEQ_MASK here, since anyway
	 whether this flag will be set or unset will be irrelevant.
	 This is important for databases, where just the inclusion
	 of op2 in op1 will be tested, using this mask;
	 using the CMP_LEQ_MASK | CMP_GEQ_MASK would also
	 test whether op1 is included in op2, wasting time for nothing
      */
      if (CMP_GEQ(ret))
	{
	  DebugLog(DF_OVM, DS_DEBUG, "OP_CGE (true ret=%i)\n", ret);
	  res = TRUE_VAR;
	}
      else
	{
	  DebugLog(DF_OVM, DS_DEBUG, "OP_CGE (false ret=%i)\n", ret);
	  res = FALSE_VAR;
	}
    }
  else
    res = NULL;
  STACK_DROP(ctx->ovm_stack, 2);
  PUSH_VALUE(ctx,res);
  return 0;
}

static int ovm_cgejmp(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  ovm_var_t *op1;
  ovm_var_t *op2;
  int ret = -1; // False by default

  DebugLog(DF_OVM, DS_DEBUG, "OP_CGEJMP\n");
  op2 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  op1 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  if (!IS_NULL(op1) && !IS_NULL(op2))
    {
      ret = issdl_cmp(op1, op2, CMP_GEQ_MASK);
      /* Don't need CMP_LEQ_MASK here, since anyway
	 whether this flag will be set or unset will be irrelevant.
	 This is important for databases, where just the inclusion
	 of op2 in op1 will be tested, using this mask;
	 using the CMP_LEQ_MASK | CMP_GEQ_MASK would also
	 test whether op1 is included in op2, wasting time for nothing
      */
      if (CMP_GEQ(ret))
	param->ip += param->ip[1]+2;
      else param->ip += 2;
    }
  else param->ip += 2;
  STACK_DROP(ctx->ovm_stack, 2);
  return 0;
}

static int ovm_cgejmp_opposite(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  ovm_var_t *op1;
  ovm_var_t *op2;
  int ret = -1; // False by default

  DebugLog(DF_OVM, DS_DEBUG, "OP_CGEJMP_OPPOSITE\n");
  op2 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  op1 = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 2);
  if (!IS_NULL(op1) && !IS_NULL(op2))
    {
      ret = issdl_cmp(op1, op2, CMP_GEQ_MASK);
      /* Don't need CMP_LEQ_MASK here, since anyway
	 whether this flag will be set or unset will be irrelevant.
	 This is important for databases, where just the inclusion
	 of op2 in op1 will be tested, using this mask;
	 using the CMP_LEQ_MASK | CMP_GEQ_MASK would also
	 test whether op1 is included in op2, wasting time for nothing
      */
      if (CMP_GEQ(ret))
	param->ip += 2;
      else param->ip += param->ip[1]+2;
    }
  else param->ip += param->ip[1]+2;
  STACK_DROP(ctx->ovm_stack, 2);
  return 0;
}

static int ovm_trash2(isn_param_t *param)
{
  DebugLog(DF_OVM, DS_DEBUG, "OP_TRASH2\n");
  STACK_DROP(param->ctx->ovm_stack, 2);
  param->ip += 1;
  return (0);
}

static int ovm_plus(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  ovm_var_t *op;
  ovm_var_t *res = NULL;

  DebugLog(DF_OVM, DS_DEBUG, "OP_PLUS\n");
  op = (ovm_var_t *)STACK_ELT(ctx->ovm_stack, 1);
  param->ip += 1;
  if (!IS_NULL(op))
    res = issdl_plus(ctx->gc_ctx, op);
  STACK_DROP(ctx->ovm_stack, 1);
  PUSH_VALUE(ctx, res);
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
	  GC_PROTECT_PROGRESSIVE(ctx->gc_ctx, string);
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
  param->ip++;
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

static int ovm_db_filter(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  bytecode_t *ip;
  int i, nconsts;
  struct db_filter_spec spec;
  db_map *m, *res;

  DebugLog(DF_OVM, DS_DEBUG, "OP_DB_FILTER\n");
  ip = param->ip+1;
  spec.nfields_res = *ip++;
  spec.nfields = *ip++;
  nconsts  = *ip++;
  spec.vals = ip;
  ip += spec.nfields;
  param->ip = ip;
  m = (db_map *)STACK_ELT(ctx->ovm_stack, nconsts+1);
  if (m==NULL)
    res = NULL;
  else
    {
      spec.constants = (ovm_var_t **)&STACK_ELT(ctx->ovm_stack, nconsts);
      spec.hash = gc_base_malloc (ctx->gc_ctx, nconsts*sizeof(unsigned long));
      for (i=0; i<nconsts; i++)
	spec.hash[i] = issdl_hash(spec.constants[i]);
      res = db_filter (ctx->gc_ctx, &spec, m);
      gc_base_free(spec.hash);
    }
  STACK_DROP(ctx->ovm_stack, nconsts+1);
  PUSH_VALUE(ctx,res);
  return 0;
}

static int ovm_db_join(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  bytecode_t *ip;
  struct db_join_spec spec;
  db_map *m1, *m2, *res;

  DebugLog(DF_OVM,DS_DEBUG, "OP_DB_JOIN\n");
  ip = param->ip+1;
  spec.nfields1 = *ip++;
  spec.nfields2 = *ip++;
  spec.vals = ip;
  ip += spec.nfields2;
  param->ip = ip;
  m1 = (db_map *)STACK_ELT(ctx->ovm_stack, 2);
  m2 = (db_map *)STACK_ELT(ctx->ovm_stack, 1);
  if (m1==NULL || m2==NULL)
    res = NULL;
  else
    res = db_join (ctx->gc_ctx, &spec, m1, m2);
  STACK_DROP(ctx->ovm_stack, 2);
  PUSH_VALUE(ctx,res);
  return 0;
}

static int ovm_db_proj(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  bytecode_t *ip;
  int i, nconsts;
  struct db_proj_spec spec;
  db_map *m, *res;

  DebugLog(DF_OVM,DS_DEBUG, "OP_DB_PROJ\n");
  ip = param->ip+1;
  spec.nsubfields = *ip++;
  nconsts = *ip++;
  spec.head = ip;
  ip += spec.nsubfields;
  param->ip = ip;
  m = (db_map *)STACK_ELT(ctx->ovm_stack, nconsts+1);
  if (m==NULL)
    res = NULL;
  else
    {
      spec.constants = (ovm_var_t **)&STACK_ELT(ctx->ovm_stack, nconsts);
      spec.hash = gc_base_malloc (ctx->gc_ctx, nconsts*sizeof(unsigned long));
      for (i=0; i<nconsts; i++)
	spec.hash[i] = issdl_hash(spec.constants[i]);
      res = db_proj (ctx->gc_ctx, &spec, m);
      gc_base_free(spec.hash);
    }
  STACK_DROP(ctx->ovm_stack, nconsts+1);
  PUSH_VALUE(ctx,res);
  return 0;
}

struct ovm_db_map_data {
  orchids_t *ctx;
  state_instance_t *si;
  bytecode_t *ip;
};

static db_map *do_ovm_db_map (gc_t *gc_ctx,
			      int nfields_res,
			      int nfields,
			      db_map *singleton,
			      void *data)
{
  struct ovm_db_map_data *vdata = data;
  orchids_t *ctx;
  ovm_var_t **tuple;
  int i;

  tuple = singleton->what.tuples.table->tuple;
  ctx = vdata->ctx;
  for (i=0; i<nfields; i++)
    {
      PUSH_VALUE(ctx, tuple[i]);
    }
  ovm_exec_base (ctx, vdata->si, vdata->ip);
  return (db_map *)POP_VALUE(ctx);
}

static int ovm_db_map(isn_param_t *param)
{
  bytecode_t *ip;
  int nfields;
  db_map *m, *res;
  struct ovm_db_map_data data;

  DebugLog(DF_OVM,DS_DEBUG, "OP_DB_MAP\n");
  ip = param->ip;
  nfields = ip[2];
  param->ip += ip[1]+2;
  data.ctx = param->ctx;
  data.si = param->state;
  data.ip = ip+3;
  m = (db_map *)STACK_ELT(data.ctx->ovm_stack, 1);
  if (m==NULL)
    res = NULL;
  else
    res = db_collect_lazy (data.ctx->gc_ctx, nfields, m, do_ovm_db_map, &data);
  STACK_DROP(data.ctx->ovm_stack, 1);
  PUSH_VALUE(data.ctx, res);
  return 0;
}

static int ovm_db_single(isn_param_t *param)
{
  orchids_t *ctx = param->ctx;
  int nconsts;
  db_map *res;

  DebugLog(DF_OVM,DS_DEBUG, "OP_DB_SINGLE\n");
  nconsts = param->ip[1];
  param->ip += 2;
  res = db_singleton (ctx->gc_ctx,
		      (ovm_var_t **)&STACK_ELT(ctx->ovm_stack, nconsts),
		      nconsts);
  STACK_DROP(ctx->ovm_stack, nconsts);
  PUSH_VALUE(ctx,res);
  return 0;
}

static int ovm_dup(isn_param_t *param)
{
  ovm_var_t *val;

  DebugLog(DF_OVM, DS_DEBUG, "OP_DUP\n");
  val = (ovm_var_t *)STACK_ELT(param->ctx->ovm_stack, param->ip[1]);
  param->ip += 2;
  PUSH_VALUE(param->ctx, val);
  return 0;
}

static int ovm_push_null(isn_param_t *param)
{
  DebugLog(DF_OVM, DS_DEBUG, "OP_PUSHNULL\n");
  PUSH_VALUE(param->ctx, NULL);
  return 0;
}

static ovm_insn_rec_t ops_g[] = {
  { NULL,           "end"        },
  { ovm_nop,        "nop"        },
  { ovm_push,       "push"       },
  { ovm_pop,        "pop"        },
  { ovm_push_zero,  "pushzero"   },
  { ovm_push_one,   "pushone"    },
  { ovm_pushstatic, "pushstatic" },
  { ovm_pushfield,  "pushfield"  },
  { ovm_trash,      "trash"      },
  { ovm_call,       "call"       },
  { ovm_add,        "add"        },
  { ovm_sub,        "sub"        },
  { ovm_opp,        "opp"        },
  { ovm_mul,        "mul"        },
  { ovm_div,        "div"        },
  { ovm_mod,        "mod"        },
  { ovm_inc,        "inc"        },
  { ovm_dec,        "dec"        },
  { ovm_and,        "and"        },
  { ovm_or,         "or"         },
  { ovm_xor,        "xor"        },
  { ovm_neg,        "neg"        },
  { ovm_not,        "not"        },
  { ovm_jmp,        "jmp"        },
  { ovm_popcjmp,    "popcjmp"    },
  { ovm_ceq,        "ceq"        },
  { ovm_cneq,       "cneq"       },
  { ovm_crm,        "crm"        },
  { ovm_cnrm,       "cnrm"       },
  { ovm_clt,        "clt"        },
  { ovm_cgt,        "cgt"        },
  { ovm_cle,        "cle"        },
  { ovm_cge,        "cge"        },
  { ovm_regsplit,   "regsplit"   },
  { ovm_cesv,       "cesrv"      },
  { ovm_empty_event, "emptyevent" },
  { ovm_add_event,  "addevent" },
  { ovm_db_filter,  "db_filter" },
  { ovm_db_join,    "db_join" },
  { ovm_db_proj,    "db_proj" },
  { ovm_db_map,     "db_map" },
  { ovm_db_single,  "db_single" },
  { ovm_dup,        "dup" },
  { ovm_push_null,  "pushnull" },
  { ovm_push_minus_one, "pushminusone" },
  { ovm_ceqjmp,     "ceqjmp" },
  { ovm_ceqjmp_opposite, "ceqjmp_opp" },
  { ovm_cneqjmp,    "cneqjmp" },
  { ovm_cneqjmp_opposite, "cneqjmp_opp" },
  { ovm_crmjmp,     "crmjmp" },
  { ovm_crmjmp_opposite, "crmjmp_opp" },
  { ovm_cnrmjmp,    "cnrmjmp" },
  { ovm_cnrmjmp_opposite, "cnrmjmp_opp" },
  { ovm_cgtjmp,     "cgtjmp" },
  { ovm_cgtjmp_opposite, "cgtjmp_opp" },
  { ovm_cltjmp,     "cltjmp" },
  { ovm_cltjmp_opposite, "cltjmp_opp" },
  { ovm_cgejmp,     "cgejmp" },
  { ovm_cgejmp_opposite, "cgejmp_opp" },
  { ovm_clejmp,     "clejmp" },
  { ovm_clejmp_opposite, "clejmp_opp" },
  { ovm_trash2,     "trash2" },
  { ovm_plus,       "plus" },
  { NULL,           NULL         }
};


/*
** generated by:
** grep -E 'define OP_[A-Z]+' ovm.h |
** sed 's/^\#define OP_\([A-Z]*\) [0-9]*$/\"\1\",/' |
** tr A-Z a-z
*/

const char *get_opcode_name(bytecode_t opcode)
{
  if (opcode >= OPCODE_NUM)
    return NULL;
  return ops_g[opcode].name;
}


/*
** Copyright (c) 2002-2005 by Julien OLIVAIN, Laboratoire Spécification
** et Vérification (LSV), CNRS UMR 8643 & ENS Cachan.
** Copyright (c) 2014-2016 by Jean GOUBAULT-LARRECQ, Laboratoire Spécification
** et Vérification (LSV), CNRS UMR 8643 & ENS Cachan.
**
** Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
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
