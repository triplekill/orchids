%{
/**
 ** @file cisco.l
 ** cisco log lexer.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 ** @date  Started on: Mon Jan 27 12:21:47 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h> /* for open() modes */

#include "cisco.tab.h"

#include "orchids.h"

int ciscolex(void);
void ciscoerror(char *s);

static char *input_string_g = NULL;
static int   input_string_sz_g = 0;
static char *input_string_ptr_g = NULL;

int my_yyinput(char *buf, int max_size);

//#define YY_INPUT(b, r, ms) (r = my_yyinput(b, ms))

%}

%option nounput

%%

 /* reserved keywords and operators */
"SEC-6-IPACCESSLOGDP" { return (CISCO_IPACCESSLOGDP); }
"SEC-6-IPACCESSLOGNP" { return (CISCO_IPACCESSLOGNP); }
"SEC-6-IPACCESSLOGP" { return (CISCO_IPACCESSLOGP); }
"SEC-6-IPACCESSLOGRP" { return (CISCO_IPACCESSLOGRP); }
"SEC-6-IPACCESSLOGS" {return (CISCO_IPACCESSLOGS); }

"list" { return (CISCO_LIST); }

"denied"    { return (CISCO_DENIED); }
"permitted" { return (CISCO_PERMITTED); }

"icmp" { return (CISCO_ICMP); }
"igmp" { return (CISCO_IGMP); }
"ospf" { return (CISCO_OSPF); }
"tcp"  { return (CISCO_TCP);  }
"udp"  { return (CISCO_UDP);  }

"->" { return (CISCO_ARROW); }

"access-list logging rate-limited or missed" { return (CISCO_RLTEXT); }

packets? { return (CISCO_PACKETS); }

[0-9]+ { ciscolval.integer = atoi(ciscotext); return (INTEGER); }
[a-zA-Z][a-zA-Z0-9_]+ {
  ciscolval.string = strdup(ciscotext);
  DPRINTF( ("string\n") );
  return (STRING);
}
([0-9]{1,3}\.){3}[0-9]{1,3} {
  ciscolval.ipv4addr.s_addr = inet_addr(ciscotext);
  return (INETADDR);
}

\n /* skip new lines */;

. { return (ciscotext[0]); }


%%

/* <ADDRESS>([0-9]{1,3}\.){3}[0-9]{1,3} { snarelval.ipv4addr.s_addr = inet_addr(snaretext); return (INETADDR); } */
/* <ADDRESS>[0-9]+ { snarelval.integer = atoi(snaretext); return (INTEGER); } */
/* <ADDRESS>\t { BEGIN INITIAL; return (snaretext[0]); } */
/* <ADDRESS>. { return (snaretext[0]); } */

/*
** yywrap()
*/

int
my_yyinput(char *buf, int max_size)
{
  int n;

  DPRINTF( ("my_yyinput(%p, %i)\n", buf, max_size) );

  if (max_size > (input_string_g + input_string_sz_g - input_string_ptr_g))
    n = (input_string_g + input_string_sz_g - input_string_ptr_g);
  else
    n = max_size;

  if (n > 0)
    {
      memcpy(buf, input_string_ptr_g, n);
      input_string_ptr_g += n;
    }

  return (n);
}

void
set_lexer_str(char *str, size_t s)
{
  input_string_g = str;
  input_string_sz_g = s;
  input_string_ptr_g = str;
}

int
ciscowrap(void)
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
