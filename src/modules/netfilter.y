%{
/**
 ** @file netfilter.y
 ** Netfilter yaccer.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 ** @date  Started on: Mon Jan 27 17:37:12 2003
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

/* #include <fcntl.h> */
/* #include <sys/syscall.h> */
/* #include <sys/stat.h> */
#include <netinet/in.h>

  /* #include "netfilter.h" */

#include "orchids.h"

typedef struct string_s string_t;
struct string_s {
  char *str;
  size_t pos;
  size_t len;
};

static ovm_var_t **fields_g;
static char *vstr_base_g;
extern char *netfiltertext;

extern int netfilterlex(void);
extern void netfiltererror(char *s);
void netfilter_set_attrs(ovm_var_t **attr_fields);

static char *ip_proto_g[5];

#define NETFILTER_FIELDS 32
#define F_IN           0
#define F_PHYSIN       1
#define F_OUT          2
#define F_PHYSOUT      3
#define F_MAC          4
#define F_SRC          5
#define F_DST          6
#define F_IPLEN        7
#define F_TOS          8
#define F_PREC         9
#define F_TTL         10
#define F_IPID        11
#define F_IPFLAGS     12
#define F_FRAG        13
#define F_IPOPTS      14
#define F_PROTO       15
#define F_SPT         16
#define F_DPT         17
#define F_SEQ         18
#define F_ACK         19
#define F_WINDOW      20
#define F_RES         21
#define F_TCPFLAGS    22
#define F_URGP        23
#define F_UDPLEN      24
#define F_ICMPTYPE    25
#define F_ICMPCODE    26
#define F_ICMPID      27
#define F_ICMPSEQ     28
#define F_ICMPPARAM   29
#define F_ICMPGATEWAY 30
#define F_ICMPMTU     31

#define IP_PROTO_TCP   0
#define IP_PROTO_UDP   1
#define IP_PROTO_ICMP  2
#define IP_PROTO_OTHER 3

#define IP_FLAG_CE 0x01
#define IP_FLAG_MF 0x02
#define IP_FLAG_DF 0x04

#define TCP_FLAG_CWR 0x01
#define TCP_FLAG_ECE 0x02
#define TCP_FLAG_URG 0x04
#define TCP_FLAG_ACK 0x08
#define TCP_FLAG_PSH 0x10
#define TCP_FLAG_RST 0x20
#define TCP_FLAG_SYN 0x40
#define TCP_FLAG_FIN 0x80

%}

%union {
  int integer;
  char *string;
  string_t new_string;
  unsigned int ip_flags;
  unsigned int tcp_flags;
  unsigned char *mac_info;
  struct in_addr ipv4addr;
}

%token NF_IN NF_PHYSIN NF_OUT NF_PHYSOUT NF_MAC
%token NF_SRC NF_DST NF_LEN NF_TOS NF_PREC NF_TTL NF_ID 
%token NF_CE NF_DF NF_MF NF_FRAG NF_OPT

%token NF_PROTO NF_INCOMPLETE

%token NF_TCP NF_SPT NF_DPT NF_SEQ NF_ACK NF_WINDOW NF_RES
%token NF_CWR NF_ECE NF_URG NF_PSH NF_RST NF_SYN NF_FIN NF_URGP

%token NF_UDP NF_ICMP NF_TYPE NF_CODE NF_PARAM NF_GATEWAY NF_MTU

%token NF_BYTES

%token <new_string> STRING
%token <integer> INTEGER HEXBYTE
%token <ipv4addr> INETADDR
%token <mac_info> MACINFO

%type <ip_flags> ecn_congestion_experienced dont_fragment more_fragments
%type <tcp_flags> cwr ece urg ack psh rst syn fin
%type <new_string> interface

%%

nfline:
  NF_IN  '=' interface ' ' physin
  NF_OUT '=' interface ' ' physout
  macinfo
  NF_SRC '=' INETADDR ' ' NF_DST '=' INETADDR ' '
  NF_LEN '=' INTEGER ' '
  NF_TOS '=' HEXBYTE ' '
  NF_PREC '=' HEXBYTE ' '
  NF_TTL '=' INTEGER ' '
  NF_ID '=' INTEGER ' '
  ip_flags
  frag_offset
  ip_options
  proto_specifics
    {
      DPRINTF( ("LEN=%i TOS=0x%02X PREC=0x%02X TTL=%i ID=%i\n",
                $22, $26, $30, $34, $38) );

      if ($3.len) {
        fields_g[F_IN] = ovm_vstr_new();
        VSTR(fields_g[F_IN]) = vstr_base_g + $3.pos;
        VSTRLEN(fields_g[F_IN]) = $3.len;
      }

      if ($8.len) {
        fields_g[F_OUT] = ovm_vstr_new();
        VSTR(fields_g[F_OUT]) = vstr_base_g + $8.pos;
        VSTRLEN(fields_g[F_OUT]) = $8.len;
      }

      fields_g[F_SRC] = ovm_ipv4_new();
      IPV4(fields_g[F_SRC]) = $14;

      fields_g[F_DST] = ovm_ipv4_new();
      IPV4(fields_g[F_DST]) = $18;

      fields_g[F_IPLEN] = ovm_int_new();
      INT(fields_g[F_IPLEN]) = $22;

      fields_g[F_TOS] = ovm_int_new();
      INT(fields_g[F_TOS]) = $26;

      fields_g[F_PREC] = ovm_int_new();
      INT(fields_g[F_PREC]) = $30;

      fields_g[F_TTL] = ovm_int_new();
      INT(fields_g[F_TTL]) = $34;

      fields_g[F_IPID] = ovm_int_new();
      INT(fields_g[F_IPID]) = $38;

      DPRINTF( ("*** good line ***\n") );
    }
;

interface:
  /* empty */
    { $$.len = 0; }
| STRING
    { $$ = $1; }
;

physin:
  /* empty */
    { /* do nothing */ ; }
| NF_PHYSIN '=' STRING ' '
    {
      fields_g[F_PHYSIN] = ovm_vstr_new();
      VSTR(fields_g[F_PHYSIN]) = vstr_base_g + $3.pos;
      VSTRLEN(fields_g[F_PHYSIN]) = $3.len;
    }
;

physout:
  /* empty */
    { /* do nothing */ ; }
| NF_PHYSOUT '=' STRING ' '
    {
      fields_g[F_PHYSOUT] = ovm_vstr_new();
      VSTR(fields_g[F_PHYSOUT]) = vstr_base_g + $3.pos;
      VSTRLEN(fields_g[F_PHYSOUT]) = $3.len;
    }
;

macinfo:
  /* empty */
    { DPRINTF( ("no MAC info\n") ); }
| NF_MAC '=' mac_header ' '
    { DPRINTF( ("MAC=\n") ); }
;

mac_header:
  /* empty */
    { DPRINTF( ("no MAC header\n") ); }
| MACINFO
    { DPRINTF( ("MAC header\n") ); }
;

ip_flags:
  ecn_congestion_experienced dont_fragment more_fragments
    {
      fields_g[F_IPFLAGS] = ovm_int_new();
      INT( fields_g[F_IPFLAGS] ) = $1 | $2 | $3;
    }
;

  /* bit 7 is in the TOS ip field, not in IP-flags (see RFC2481) */
ecn_congestion_experienced:
  /* empty */
    { $$ = 0; }
| NF_CE ' '
    { $$ = IP_FLAG_CE; }
;

dont_fragment:
  /* empty */
    { $$ = 0; }
| NF_DF ' '
    { $$ = IP_FLAG_DF; }
;

more_fragments:
  /* empty */
    { $$ = 0; }
| NF_MF ' '
    { $$ = IP_FLAG_MF; }
;

frag_offset:
  /* empty */
    { /* do nothing */ ; }
| NF_FRAG '=' INTEGER ' '
    {
      fields_g[F_FRAG] = ovm_int_new();
      INT(fields_g[F_FRAG]) = $3;
    }
;

ip_options:
  /* empty */
    { /* do nothing */ ; }
| NF_OPT ' ' '(' STRING ')' ' '
    { DPRINTF( ("OPT\n") ); }
;

proto_specifics:
  proto_tcp
    {
      DPRINTF( ("PROTO=TCP\n") );
    }
| proto_udp
    {
      DPRINTF( ("PROTO=UDP\n") );
    }
| proto_icmp
    {
      DPRINTF( ("PROTO=ICMP\n") );
    }
| proto_other
    {
      DPRINTF( ("PROTO=OTHER\n") );
    }
;


/* -----  TCP ------ */

proto_tcp:
  NF_PROTO '=' NF_TCP ' '
| NF_PROTO '=' NF_TCP ' ' NF_INCOMPLETE ' ' '[' INTEGER ' ' NF_BYTES ']'
| NF_PROTO '=' NF_TCP ' '
  NF_SPT '=' INTEGER ' ' NF_DPT '=' INTEGER ' '
  sequence_numbers
  NF_WINDOW '=' INTEGER ' '
  NF_RES '=' HEXBYTE ' '
  tcp_flags
  NF_URGP '=' INTEGER ' '
  /* XXX ajouter les OPTions tcp */
    {
      DPRINTF( ("SPT=%i DPT=%i WINDOWS=%i URGPP=%i\n", $7, $11, $16, $25) );
      fields_g[F_PROTO] = ovm_vstr_new();
      VSTR(fields_g[F_PROTO]) = ip_proto_g[IP_PROTO_TCP];
      VSTRLEN(fields_g[F_PROTO]) = strlen(ip_proto_g[IP_PROTO_TCP]);

      fields_g[F_SPT] = ovm_int_new();
      INT(fields_g[F_SPT]) = $7;

      fields_g[F_DPT] = ovm_int_new();
      INT(fields_g[F_DPT]) = $11;

      fields_g[F_WINDOW] = ovm_int_new();
      INT(fields_g[F_WINDOW]) = $16;

      fields_g[F_RES] = ovm_int_new();
      INT(fields_g[F_RES]) = $20;

      fields_g[F_URGP] = ovm_int_new();
      INT(fields_g[F_URGP]) = $25;
    }
;

sequence_numbers:
  /* empty */
    { /* do nothing */ ; }
| NF_SEQ '=' INTEGER ' ' NF_ACK '=' INTEGER ' '
    {
      DPRINTF( ("SEQ=%i ACK=%i\n", $3, $7) );

      fields_g[F_SEQ] = ovm_uint_new();
      UINT(fields_g[F_SEQ]) = $3;

      fields_g[F_ACK] = ovm_uint_new();
      UINT(fields_g[F_ACK]) = $7;
    }
;

tcp_flags:
  cwr ece urg ack psh rst syn fin
    {
      fields_g[F_TCPFLAGS] = ovm_int_new();
      INT( fields_g[F_TCPFLAGS] ) = $1 | $2 | $3 | $4 | $5 | $6 | $7 | $8;
    }
;

 /* Congestion Window Reduced */
cwr:
  /* empty */
    { $$ = 0; }
| NF_CWR ' '
    { $$ = TCP_FLAG_CWR; }
;

ece:
  /* empty */
    { $$ = 0; }
| NF_ECE ' '
    { $$ = TCP_FLAG_ECE; }
;

urg:
  /* empty */
    { $$ = 0; }
| NF_URG ' '
    { $$ = TCP_FLAG_URG; }
;

ack:
  /* empty */
    { $$ = 0; }
| NF_ACK ' '
    { $$ = TCP_FLAG_ACK; }
;

psh:
  /* empty */
    { $$ = 0; }
| NF_PSH ' '
    { $$ = TCP_FLAG_PSH; }
;

rst:
  /* empty */
    { $$ = 0; }
| NF_RST ' '
    { $$ = TCP_FLAG_RST; }
;

syn:
  /* empty */
    { $$ = 0; }
| NF_SYN ' '
    { $$ = TCP_FLAG_SYN; }
;

fin:
  /* empty */
    { $$ = 0; }
| NF_FIN ' '
    { $$ = TCP_FLAG_FIN; }
;



/* UDP */

proto_udp:
  NF_PROTO '=' NF_UDP ' '
| NF_PROTO '=' NF_UDP ' ' NF_INCOMPLETE ' ' '[' INTEGER ' ' NF_BYTES ']'
| NF_PROTO '=' NF_UDP ' '
  NF_SPT '=' INTEGER ' ' NF_DPT '=' INTEGER ' '
  NF_LEN '=' INTEGER ' '
    {
      fields_g[F_SPT] = ovm_int_new();
      INT(fields_g[F_SPT]) = $7;

      fields_g[F_DPT] = ovm_int_new();
      INT(fields_g[F_DPT]) = $11;

      fields_g[F_UDPLEN] = ovm_int_new();
      INT(fields_g[F_UDPLEN]) = $15;
    }
;



/* ICMP */

proto_icmp:
  NF_PROTO '=' NF_ICMP ' '
| NF_PROTO '=' NF_ICMP ' ' NF_INCOMPLETE ' ' '[' INTEGER ' ' NF_BYTES ']'
| NF_PROTO '=' NF_ICMP ' '
  NF_TYPE '=' INTEGER ' '
  NF_CODE '=' INTEGER ' '
  icmp_specific
    {
      fields_g[F_ICMPTYPE] = ovm_int_new();
      INT( fields_g[F_ICMPTYPE] ) = $7;

      fields_g[F_ICMPCODE] = ovm_int_new();
      INT( fields_g[F_ICMPCODE] ) = $11;
    }
;

icmp_specific:
  icmp_echo
| icmp_parameter
| icmp_redirect
| icmp_other
;

icmp_echo:
  NF_ID '=' INTEGER ' ' NF_SEQ '=' INTEGER ' '
    {
      fields_g[F_ICMPID] = ovm_int_new();
      INT( fields_g[F_ICMPID] ) = $3;

      fields_g[F_ICMPSEQ] = ovm_int_new();
      INT( fields_g[F_ICMPSEQ] ) = $7;
    }
;

icmp_parameter:
  NF_PARAM '=' INTEGER ' '
    { DPRINTF( ("PARAMETER\n") ); }
;

icmp_redirect:
  NF_GATEWAY '=' INETADDR ' '
    { DPRINTF( ("GATEWAY\n") ); }
;

  /* we have a level of recursion here !... */
icmp_other:
  '[' nfline ']'
  mtu
    { DPRINTF( ("IP-in-ICMP\n") ); }
;

mtu:
  /* empty */
| NF_MTU '=' INTEGER ' '
;

proto_other:
  NF_PROTO '=' INTEGER ' '
    { DPRINTF( ("PROTO=%i\n", $3) ); }
;

%%

void
netfiltererror(char *s)
{
  DebugLog(DF_MOD, DS_ERROR, "%s at '%s'.\n",
           s, netfiltertext);
}

void
netfilter_set_attrs(ovm_var_t **attr_fields)
{
  fields_g = attr_fields;
}

void
netfilter_set_vstr_base(char *str)
{
  vstr_base_g = str;
}

static char *ip_proto_g[5] = {
  "TCP",
  "UDP",
  "ICMP",
  "OTHER",
  NULL
};


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
