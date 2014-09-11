%{

/**
 ** @file issdl.l
 ** issdl.yy.c ISSDL lexer.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 1.0
 **
 ** @date  Started on: Tue Feb 25 18:51:38 2003
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
#include "tree.h"
#include "rule_compiler.h"
#include "dlist.h"

#include "issdl.tab.h"

/*
** Since the maxline limit is usually 4096 and quoted string can't
** be defined in multiple lines, we can't have a quoted string
** bigger than 4096.
*/
#define QSTRMAX 4096

#define QSTR_PUTC(c) \
  do { \
    if (qstr_sz_g >= (QSTRMAX - 1)) { \
      printf("%24s:%-4d: quoted string too long\n", __FILE__, __LINE__); \
      exit(EXIT_FAILURE); \
     } \
     qstr_sz_g++; \
     *qstr_ptr_g++ = (c); \
  } while (0)

unsigned int issdllineno_g = 1;
static rule_compiler_t *compiler_ctx_g = NULL;
char *issdlcurrentfile_g = NULL;
SLIST_HEAD(filestack, filestack_t) issdlfilestack_g =
  SLIST_HEAD_INITIALIZER(filestack);

/* Static values for escape character interpretation and
** quoted string construction. */
static char   qstr_buf_g[QSTRMAX];
static char  *qstr_ptr_g;
static size_t qstr_sz_g;

/* Add this line with BYacc */
/* extern int issdllval; **/

%}

/* eXclusive state COMMENT */
%x COMMENT QSTR

%option nounput never-interactive

%%

[\t ]+ /* Ignore whitespace */ ;
"\n" { ++issdllineno_g; }

 /* Reserved keywords and operators */
"rule"     { return (RULE);     }
"state"    { return (STATE);    }
"synchronize" { return (SYNCHRONIZE); }
"init" {
  issdllval.sym.file = issdlcurrentfile_g;
  issdllval.sym.line = issdllineno_g;
  issdllval.sym.name = strdup(issdltext);
  return (INIT);
}
"goto"     { return (GOTO);     }
"if"       { return (IF);       }
"else"     { return (ELSE);     }
"expect"   { return (EXPECT);   }
"{"        { return (O_BRACE);  }
"}"        { return (C_BRACE);  }
"("        { return (O_PARENT); }
")"        { return (C_PARENT); }
"="        { return (EQ);       }
";"        { return (SEMICOLUMN); }
","        { return (COMMA);    }
"||"       { return (OROR);     }
"&&"       { return (ANDAND);   }
"=="       { return (EQCOMPARE); }
"!="       { return (NOTEQUAL); }
"+"        { return (O_PLUS);   }
"-"        { return (O_MINUS);  }
"*"        { return (O_TIMES); }
"/"        { return (O_DIV); }
"%"        { return (O_MOD); }
"=~"       { return (REGEX_MATCH); }
"!~"       { return (REGEX_NOTMATCH); }
">"        { return (GREATER);  }
"<"        { return (LESS);     }
">="       { return (GREATER_EQ); }
"<="       { return (LESS_EQ);  }
"=:="      { return (EQBIND); }
"_CTIME"   { return (KW_CTIME); }
"_IPV4"    { return (KW_IPV4); }
"_TIMEVAL" { return (KW_TIMEVAL); }
"_COUNTER" { return (KW_COUNTER); }
"_REGEX"   { return (KW_REGEX); }

 /* Handle comments */
"/*"       { BEGIN COMMENT; }
<COMMENT>. ;
<COMMENT>\n   { ++issdllineno_g; }
<COMMENT>"*/" { BEGIN INITIAL; }
<COMMENT>"/*" { DPRINTF( ("warning! '/*' in comment.\n") ); }

 /* Handle output of CPP line control */
^"# "[0-9]+" \"".+"\""(" "[1-4]){0,4}\n {
  int line, flag1, flag2, flag3, flag4;
  char file[1024];
  int ret;
  filestack_t *fs;

  /*
    CPP output line control code:
    1: This code indicates the start of a new file.
    2: This code indicates returning to a file
       (after having included another file).
    3: This code indicates that the following text comes from a
       system header file, so certain warnings should be suppressed.
    4: This code indicates that the following text should be treated as being
       wrapped in an implicit `extern "C"' block.
  */
  flag1 = 0;
  flag2 = 0;
  flag3 = 0;
  flag4 = 0;
  line = 0;
  file[0] = '\0';
  ret = sscanf(issdltext, "# %i \"%1024[^\"]\" %i %i %i %i",
               &line, file, &flag1, &flag2, &flag3, &flag4);
  DebugLog(DF_OLC, DS_DEBUG, ">>> CPP sscanf(\"%s\") = %i\n", issdltext, ret);

  if ((ret >= 2) && (ret <= 6)) {
    /* Handle flags */
    if (flag1 == 1) {
      /* If 1 is set, push new file and set line counter */
      fs = Xmalloc(sizeof (filestack_t));
      fs->file = strdup(issdlcurrentfile_g);
      fs->line = issdllineno_g;
      SLIST_INSERT_HEAD(&issdlfilestack_g, fs, elmt);
    }
    else if (flag1 == 2) {
      /* If 2 is set, pop val, set new file value and line number */
      if ( ! SLIST_IS_EMPTY(&issdlfilestack_g) ) {
        fs = SLIST_FIRST(&issdlfilestack_g);
        SLIST_REMOVE_HEAD(&issdlfilestack_g, elmt);
        Xfree(fs->file);
        Xfree(fs);
      }
      else {
        fprintf(stderr, "warning: cpp file inclusion underflow.\n");
      }
    }
    /* Other flags are specific to C */

    if (issdlcurrentfile_g)
      Xfree(issdlcurrentfile_g);
    issdlcurrentfile_g = strdup(file);
    issdllineno_g = line;

  } else {
    fprintf(stderr, "warning: syntax error in cpp line control syntax.\n");
  }
}

 /* Variable name */
$[A-Za-z_][A-Za-z0-9_\.]* {
  issdllval.string = strdup(issdltext + 1);
  /* DPRINTF( ("VARIABLE %s\n", issdltext) ); */
  return (VARIABLE);
}

 /* Field name */
\.[a-z]+\.[a-z_]+ {
  issdllval.string = strdup(issdltext + 1); /* +1 skip the dot */
  /* DPRINTF( ("FIELD %s\n", issdltext) ); */
  return(FIELD);
}

 /* Symbol names (same as a C declaration) */
[A-Za-z][A-Za-z0-9_]* {
  issdllval.sym.file = strdup(issdlcurrentfile_g);
  issdllval.sym.line = issdllineno_g;
  issdllval.sym.name = strdup(issdltext);
  DebugLog(DF_OLC, DS_DEBUG,
           "sym file=%s line=%i name=%s\n",
           issdllval.sym.file,
           issdllval.sym.line,
           issdllval.sym.name);
  return (SYMNAME);
}

 /* Floating point numbers */
([0-9]*\.[0-9]+)([eE][-+]?[0-9]+)? {
  issdllval.fp_double = atof(issdltext);
  /* printf("FPDOUBLE %f\n", issdllval.fp_double); */
  return (FPDOUBLE);
}

 /* Integer numbers */
[0-9]+ {
  issdllval.integer = atoi(issdltext);
  /* printf("NUMBER %i\n", issdllval.integer); */
  return (NUMBER);
}

 /* Quoted string handling */
 /* A double quote is the begin of a quoted string */
\" {
  qstr_sz_g = 0;
  qstr_ptr_g = qstr_buf_g;
  BEGIN QSTR;
}

  /* End of the string */
<QSTR>\" {
  BEGIN INITIAL;
  *qstr_ptr_g = '\0';
  issdllval.string = strdup(qstr_buf_g);
  return (STRING);
}

  /* Newline in qstr must be specified as \n */
<QSTR>\n {
  DPRINTF( ("ERROR: newline in multiline quoted strings must be escaped.\n") );
  return (0);
}

  /* Octal sequence handling */
<QSTR>\\[0-7]{1,3} {
  unsigned int res;

  sscanf(yytext + 1, "%o", &res);
  if (res > 0xFF)
    {
      DPRINTF( ("ERROR: bad octal value (%i)\n", res) );
      return (0);
    }
  QSTR_PUTC(res);
}

  /* Other illegal values */
<QSTR>\\[0-9]+ {
  DPRINTF( ("ERROR: bad octal value (%s)\n", yytext) );
  return (0);
}

  /* New line */
<QSTR>\\n { QSTR_PUTC('\n'); }

  /* Tabulation */
<QSTR>\\t { QSTR_PUTC('\t'); }

  /* Carriage return */
<QSTR>\\r { QSTR_PUTC('\r'); }

  /* Backspace */
<QSTR>\\b { QSTR_PUTC('\b'); }

  /* Form feed */
<QSTR>\\f { QSTR_PUTC('\f'); }

  /* Other escaped characters (\", \\n, ...) */
<QSTR>\\(.|\n) { QSTR_PUTC(yytext[1]); }

  /* Traditional string */
<QSTR>[^\\\n\"]+ {
  char *yptr;

  yptr = yytext;
  while (*yptr)
    QSTR_PUTC(*yptr++);
}

 /* If we are here, it means that something went wrong
  * (this is a catch all rule)... */
. {
  DPRINTF( ("error.\n") );
  return (issdltext[0]);
}

%%

/*
** yywrap()
*/

void
set_lexer_context(rule_compiler_t *ctx)
{
  compiler_ctx_g = ctx;
}

int
issdlwrap(void)
{
  return (1);
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
