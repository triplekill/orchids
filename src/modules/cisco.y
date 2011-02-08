%{
/**
 ** @file cisco.y
 ** Cisco log yaccer.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 ** @date  Started on: Mon Jan 27 17:32:49 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#include <stdlib.h>
#include <stdio.h>

/* #include <fcntl.h> */
/* #include <sys/syscall.h> */
/* #include <sys/stat.h> */
#include <netinet/in.h>

#include "orchids.h"

extern int ciscolex(void);
extern void ciscoerror(char *s);

extern char *ciscotext;

%}

%union {
  int integer;
  char *string;
  struct in_addr ipv4addr;
}

%token CISCO_SEC

%token CISCO_IPACCESSLOGDP CISCO_IPACCESSLOGNP CISCO_IPACCESSLOGRL
%token CISCO_IPACCESSLOGP CISCO_IPACCESSLOGRP CISCO_IPACCESSLOGS

%token CISCO_LIST

%token CISCO_DENIED CISCO_PERMITTED

%token CISCO_ICMP CISCO_IGMP CISCO_OSPF CISCO_TCP CISCO_UDP

%token CISCO_ARROW CISCO_PACKETS

%token CISCO_RLTEXT

%token <string> STRING
%token <integer> INTEGER
%token <ipv4addr> INETADDR

%%

ciscoline_list:
  ciscoline_list ciscoline
| ciscoline
;

ciscoline:
 '%' CISCO_IPACCESSLOGDP ':' ' ' CISCO_LIST ' ' INTEGER ' '
  action ' ' CISCO_ICMP ' ' INETADDR ' ' CISCO_ARROW ' ' INETADDR ' '
  '(' INTEGER '/' INTEGER ')' ',' ' ' INTEGER ' ' CISCO_PACKETS
    { DPRINTF( ("Log Datagram Protocol type=%i code=%i %i packet(s)\n",
                $20, $22, $26) ); }
| '%' CISCO_IPACCESSLOGNP ':' ' ' CISCO_LIST ' ' INTEGER ' '
   action ' ' INTEGER ' ' INETADDR ' ' CISCO_ARROW ' ' INETADDR ',' ' '
   INTEGER ' ' CISCO_PACKETS
     { DPRINTF( ("Log No Protocol\n") ); }
| '%' CISCO_IPACCESSLOGP ':' ' ' CISCO_LIST ' ' INTEGER ' '
  action ' ' proto_tcpudp ' ' INETADDR '(' INTEGER ')' ' ' CISCO_ARROW ' '
  INETADDR '(' INTEGER ')' ',' ' ' INTEGER ' ' CISCO_PACKETS
    { DPRINTF( ("Log TCP/UDP Peer (Ipaddr/port)\n") ); }
| '%' CISCO_IPACCESSLOGRL ':' ' ' CISCO_RLTEXT ' ' INTEGER ' ' CISCO_PACKETS
    { DPRINTF( ("Log Rate-limit\n") ); }
| '%' CISCO_IPACCESSLOGRP ':' ' ' CISCO_LIST ' ' INTEGER ' '
  action ' ' proto_multicast ' ' INETADDR ' ' CISCO_ARROW ' ' INETADDR ',' ' '
  INTEGER ' ' CISCO_PACKETS
    { DPRINTF( ("Log (Multicast) Router Protocols\n") ); }
| '%' CISCO_IPACCESSLOGS ':' ' ' CISCO_LIST ' ' INTEGER ' '
  action ' ' INETADDR ' ' INTEGER ' ' CISCO_PACKETS
  { DPRINTF( ("IP Access Log Self\n") );  }
;

action:
  CISCO_PERMITTED
| CISCO_DENIED
;

proto_tcpudp:
  CISCO_TCP
| CISCO_UDP
;

proto_multicast:
  CISCO_OSPF
| CISCO_IGMP
;

%%

void
ciscoerror(char *s)
{
  printf("%s at '%s'.\n", s, ciscotext);
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
