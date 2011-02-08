/**
 ** @file mod_wifi.h
 ** 
 ** 
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 ** 
 ** @version 0.1.0
 ** 
 ** @date  Started on: Fri Jun 15 09:05:21 2007
 **/

#ifndef MODWIFI_H
#define MODWIFI_H

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
