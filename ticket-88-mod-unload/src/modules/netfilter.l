%{

/**
 ** @file netfilter.l
 ** Parser for Netfilter/Iptables.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 ** @date  Started on: Mon Jan 27 17:21:41 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

/*
 * grep NF_ netfilter.l | sed 's/^[^(]*(\(NF_[A-Z]*\))[^)]*$/\1/g'
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h> /* for open() modes */

typedef struct string_s string_t;
struct string_s {
  char  *str;
  int    pos;
  size_t len;
};

#include "netfilter.tab.h"

  /* #include "netfilter.h" */

#include "orchids.h"

#define USE_FLEX_SCANBYTES

int netfilterlex(void);
void netfiltererror(char *s);

extern void netfilter_set_str(char *str, size_t s);
extern void netfilter_set_attrs(ovm_var_t **attr_fields);
extern int netfilterparse(void);
extern void netfilter_reset(void);

static int input_offset_g = 0;
#ifdef USE_FLEX_SCANBYTES
static YY_BUFFER_STATE state_g;
#endif

#define COUNT_OFFSET() \
  do { \
    input_offset_g += netfilterleng; \
  } while (0) 

#ifndef USE_FLEX_SCANBYTES
static char *input_string_g = NULL;
static int   input_string_sz_g = 0;
static char *input_string_ptr_g = NULL;

int my_yyinput(char *buf, int max_size);

#define YY_INPUT(b, r, ms) (r = my_yyinput(b, ms))
#endif

%}

%option nounput never-interactive

%%

 /* reserved keywords and operators */
"IN"      { COUNT_OFFSET(); return (NF_IN);      }
"PHYSIN"  { COUNT_OFFSET(); return (NF_PHYSIN);      }
"OUT"     { COUNT_OFFSET(); return (NF_OUT);       }
"PHYSOUT" { COUNT_OFFSET(); return (NF_PHYSOUT);    }
"MAC"     { COUNT_OFFSET(); return (NF_MAC);    }

"SRC" { COUNT_OFFSET(); return (NF_SRC); }
"DST" { COUNT_OFFSET(); return (NF_DST); }
"LEN" { COUNT_OFFSET(); return (NF_LEN); }
"TOS" { COUNT_OFFSET(); return (NF_TOS); }
"PREC" { COUNT_OFFSET(); return (NF_PREC); }
"TTL" { COUNT_OFFSET(); return (NF_TTL); }
"ID" { COUNT_OFFSET(); return (NF_ID); }
"CE" { COUNT_OFFSET(); return (NF_CE); }
"DF" { COUNT_OFFSET(); return (NF_DF); }
"MF" { COUNT_OFFSET(); return (NF_MF); }
"FRAG" { COUNT_OFFSET(); return (NF_FRAG); }
"OPT" { COUNT_OFFSET(); return (NF_OPT); }

"PROTO" { COUNT_OFFSET(); return (NF_PROTO); }
"INCOMPLETE" { COUNT_OFFSET(); return (NF_INCOMPLETE); }

"TCP" { COUNT_OFFSET(); return (NF_TCP); }
"SPT" { COUNT_OFFSET(); return (NF_SPT); }
"DPT" { COUNT_OFFSET(); return (NF_DPT); }
"SEQ" { COUNT_OFFSET(); return (NF_SEQ); }
"ACK" { COUNT_OFFSET(); return (NF_ACK); }
"WINDOW" { COUNT_OFFSET(); return (NF_WINDOW); }
"RES" { COUNT_OFFSET(); return (NF_RES); }

"CWR" { COUNT_OFFSET(); return (NF_CWR); }
"ECE" { COUNT_OFFSET(); return (NF_ECE); }
"URG" { COUNT_OFFSET(); return (NF_URG); }
"PSH" { COUNT_OFFSET(); return (NF_PSH); }
"RST" { COUNT_OFFSET(); return (NF_RST); }
"SYN" { COUNT_OFFSET(); return (NF_SYN); }
"FIN" { COUNT_OFFSET(); return (NF_FIN); }

"URGP" { COUNT_OFFSET(); return (NF_URGP); }

"UDP" { COUNT_OFFSET(); return (NF_UDP); }

"ICMP" { COUNT_OFFSET(); return (NF_ICMP); }
"TYPE" { COUNT_OFFSET(); return (NF_TYPE); }
"CODE" { COUNT_OFFSET(); return (NF_CODE); }
"PARAMETER" { COUNT_OFFSET(); return (NF_PARAM); }
"GATEWAY" { COUNT_OFFSET(); return (NF_GATEWAY); }
"MTU" { COUNT_OFFSET(); return (NF_MTU); }

"bytes" { COUNT_OFFSET(); return (NF_BYTES); }

[0-9]+ {
  netfilterlval.integer = atoi(netfiltertext);
  COUNT_OFFSET();
  return (INTEGER);
}
[a-zA-Z][a-zA-Z0-9_\-]+ {
/*   netfilterlval.string = strdup(netfiltertext); */
  netfilterlval.new_string.pos = input_offset_g;
  netfilterlval.new_string.len = netfilterleng;
/*   printf("STRING=%s pos=%i len=%i\n", */
/*          netfiltertext, input_offset_g, netfilterleng); */
  COUNT_OFFSET();
  return (STRING);
}
0x[0-9A-F][0-9A-F] {
  sscanf(netfiltertext, "0x%02X", &netfilterlval.integer);
  COUNT_OFFSET();
  return (HEXBYTE);
}
([0-9]{1,3}\.){3}[0-9]{1,3} {
  netfilterlval.ipv4addr.s_addr = inet_addr(netfiltertext);
  COUNT_OFFSET();
  return (INETADDR);
}
([0-9a-f]{2}:)+[0-9a-f]{2} {
  int mac_len;
#if 0
  int digit;
  int i;
#endif

  mac_len = (netfilterleng + 1) / 3;
  DPRINTF( ("MACINFO sz=%i \n", mac_len) );
  netfilterlval.mac_info = NULL;
#if 0
  netfilterlval.mac_info = malloc(mac_len);
  for (i = 0; i < mac_len; i++) {
    netfiltertext[ i * 3 + 2 ] = '\0';
    sscanf(&netfiltertext[ i * 3 ], "%02x", &digit);
    netfilterlval.mac_info[i] = (unsigned char) digit;
  }
#endif

  COUNT_OFFSET();
  return (MACINFO);
}
\n /* skip new lines */;

. {   COUNT_OFFSET(); return (netfiltertext[0]); }


%%

/* <ADDRESS>([0-9]{1,3}\.){3}[0-9]{1,3} { snarelval.ipv4addr.s_addr = inet_addr(snaretext); return (INETADDR); } */
/* <ADDRESS>[0-9]+ { snarelval.integer = atoi(snaretext); return (INTEGER); } */
/* <ADDRESS>\t { BEGIN INITIAL; return (snaretext[0]); } */
/* <ADDRESS>. { return (snaretext[0]); } */

/*
** yywrap()
*/

#ifndef USE_FLEX_SCANBYTES
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
#endif

void
netfilter_reset(void)
{
#ifdef USE_FLEX_SCANBYTES
  netfilter_delete_buffer(state_g);
#else
  netfilter_flush_buffer(YY_CURRENT_BUFFER);
/*   BEGIN(INITIAL); */
#endif
}

void
netfilter_set_str(char *str, size_t s)
{
  input_offset_g = 0;
#ifdef USE_FLEX_SCANBYTES
  state_g = netfilter_scan_bytes(str, s);
#else
  input_string_g = str;
  input_string_sz_g = s;
  input_string_ptr_g = str;
#endif
}

int
netfilterwrap(void)
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
