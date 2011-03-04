/*-
 * Copyright (c) 2001 Atsushi Onoe
 * Copyright (c) 2002-2005 Sam Leffler, Errno Consulting
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id: ieee80211.h 1712 2006-09-15 01:13:53Z kelmo $
 * This file was provided in the madwifi-ng-r1747-20060929 package
 */
#ifndef MODWIFI_H
#define MODWIFI_H

struct item_t {
        u_int32_t did;
        u_int16_t status;
        u_int16_t len;
        u_int32_t data;
};

struct prism2_header_s {
        u_int32_t msgcode;
        u_int32_t msglen;
        u_int8_t devname[16];
        struct item_t hosttime;
        struct item_t mactime;
        struct item_t channel;
        struct item_t rssi;
        struct item_t sq;
        struct item_t signal;
        struct item_t noise;
        struct item_t rate;
        struct item_t istx;
        struct item_t frmlen;
};

struct ieee80211_header_s {
        u_int8_t  fc[2];
        u_int8_t  duration[2];
        u_int8_t  addr1[6];
        u_int8_t  addr2[6];
        u_int8_t  addr3[6];
        u_int16_t seqnum;
        u_int8_t  addr4[6];
};


#define	TYPE_MGT		0x00
#define	TYPE_CTL		0x04
#define	TYPE_DATA		0x08

#define	SUBTYPE_MASK		0xf0

/* TYPE_MGT */
#define	SUBTYPE_ASSOC_REQ	0x00
#define	SUBTYPE_ASSOC_RESP	0x10
#define	SUBTYPE_PROBE_REQ	0x40
#define	SUBTYPE_PROBE_RESP	0x50
#define	SUBTYPE_BEACON		0x80
#define	SUBTYPE_DISASSOC	0xa0
#define	SUBTYPE_AUTH		0xb0
#define	SUBTYPE_DEAUTH		0xc0

/* TYPE_CTL */
#define	SUBTYPE_RTS		0xb0
#define	SUBTYPE_CTS		0xc0
#define	SUBTYPE_ACK		0xd0

/* TYPE_DATA */
#define	SUBTYPE_DATA		0x00
#define	SUBTYPE_CF_ACK		0x10
#define	SUBTYPE_CF_POLL		0x20
#define	SUBTYPE_CF_ACPL		0x30
#define	SUBTYPE_NODATA		0x40

#define	DIR_MASK		0x03
#define	DIR_NODS		0x00	/* STA -> STA */
#define	DIR_TODS		0x01	/* STA -> AP  */
#define	DIR_FROMDS		0x02	/* AP  -> STA */
#define	DIR_DSTODS		0x03	/* AP  -> AP  */

#define	PROT			0x40

#define	SEQ_MASK		0xfff0
#define	SEQ_SHIFT		4

#define	CRC_LEN			4

#endif /* MODWIFI_H */
