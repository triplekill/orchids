/**
 ** @file mod_wifi.c
 ** Listen to events on 802.11 device. Dissect frames to Orchids fields.
 **
 **
 ** @author Romdhane BEN YOUNES <ben_younes.romdhane@courrier.uqam.ca>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 ** @date  Started on: Thu Jun 22 00:00:00 2006
 **/


#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <time.h> /* for strftime() and localtime() */
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pcap.h>

#include "orchids.h"
#include "orchids_api.h"

#include "compat.h"
#include "mod_wifi.h"


#define IEEE80211_FIELDS 13
#define F_TYPE            0
#define F_SUBTYPE         1
#define F_TIME            2
#define F_DA              3
#define F_SA              4
#define F_BSSID           5
#define F_TA              6
#define F_RA              7
#define F_DIR             8
#define F_SEQNUM          9
#define F_ISWEP          10
#define F_BODYLENGTH     11
#define F_RSSI           12

input_module_t mod_wifi;

int datalink_type = 0;

typedef struct ieee80211_header_s* ieee80211_header_t;
typedef struct prism2_header_s* prism2_header_t;


#define ADDR_STR_SZ  17 /* 6 bytes of 2 nibbles plus 5 ':' */
#define GETADDR(addr, ovmvar)                                           \
  do {                                                                  \
  val = ovm_str_new(gc_ctx, ADDR_STR_SZ+1);				\
  STRLEN(val) = sprintf(STR(ovmvar),					\
			"%.2x:%.2x:%.2x:%.2x:%.2x:%.2x",		\
			(addr)[0], (addr)[1], (addr)[2],		\
			(addr[3]), (addr[4]), (addr)[5]);		\
  GC_TOUCH (gc_ctx, (ovmvar) = val);					\
  } while (0)

typedef struct wifi_pcap_s {
  gc_t *gc_ctx;
  ovm_var_t **attr;
} wifi_pcap_t;

void wifi_pcap_callback(u_char* av, const struct pcap_pkthdr * pkthdr,
			const u_char * pkt)
{
  ieee80211_header_t mac_header;
  prism2_header_t prism2_header;
  u_int8_t dir, type, subtype;
  wifi_pcap_t *wpt;
  ovm_var_t **attr;
  gc_t *gc_ctx;
  ovm_var_t *val;

  DebugLog(DF_MOD, DS_TRACE, "wifi_pcap_callback()\n");

  wpt = (wifi_pcap_t *)av;
  gc_ctx = wpt->gc_ctx;
  attr = wpt->attr;

  if (datalink_type == DLT_PRISM_HEADER)
    {
      prism2_header = (prism2_header_t) pkt;
      mac_header = (ieee80211_header_t) (pkt + 144);

      val = ovm_int_new (gc_ctx, (&(prism2_header->rssi))->data);
      GC_TOUCH (gc_ctx, attr[F_RSSI] = val);
    }
  else
    return;

  /* fixation du champ .wifi.time */
  val = ovm_timeval_new (gc_ctx);
  gettimeofday(&(TIMEVAL(val)), NULL);
  GC_TOUCH (gc_ctx, attr[F_TIME] = val);

  type = mac_header->fc[0] & 0x0c;
  subtype = mac_header->fc[0] & SUBTYPE_MASK;
  dir = mac_header->fc[1] & DIR_MASK;

  switch (type)
    {
      /******************** Dans le cas d'une trame de gestion */
    case TYPE_MGT:
      val = ovm_vstr_new (gc_ctx, NULL);
      VSTR(val) = "management";
      VSTRLEN(val) = 10;
      GC_TOUCH (gc_ctx, attr[F_TYPE] = val);

      /* fixation du champ .wifi.seqnum */
      val = ovm_int_new (gc_ctx,
			 (int) (mac_header->seqnum & SEQ_MASK)	\
			 >> SEQ_SHIFT);
      GC_TOUCH (gc_ctx, attr[F_SEQNUM] = val);

      /* addr1 = DA
         addr2 = SA
         addr3 = BSSID */

      GETADDR(mac_header->addr1, attr[F_DA]);
      GETADDR(mac_header->addr2, attr[F_SA]);
      GETADDR(mac_header->addr3, attr[F_BSSID]);

      switch(subtype)
      {
        case SUBTYPE_ASSOC_REQ:
	  val = ovm_vstr_new (gc_ctx, NULL);
	  VSTR(val) = "assocreq";
	  VSTRLEN(val) = 8;
	  GC_TOUCH (gc_ctx, attr[F_SUBTYPE] = val);
          break;

        case SUBTYPE_ASSOC_RESP:
	  val = ovm_vstr_new (gc_ctx, NULL);
	  VSTR(val) = "assocresp";
	  VSTRLEN(val) = 9;
	  GC_TOUCH (gc_ctx, attr[F_SUBTYPE] = val);
          break;

        case SUBTYPE_PROBE_REQ:
	  val = ovm_vstr_new (gc_ctx, NULL);
	  VSTR(val) = "probereq";
	  VSTRLEN(val) = 8;
	  GC_TOUCH (gc_ctx, attr[F_SUBTYPE] = val);
          break;

        case SUBTYPE_PROBE_RESP:
	  val = ovm_vstr_new (gc_ctx, NULL);
	  VSTR(val) = "proberesp";
	  VSTRLEN(val) = 9;
	  GC_TOUCH (gc_ctx, attr[F_SUBTYPE] = val);
          break;

        case SUBTYPE_BEACON:
	  val = ovm_vstr_new (gc_ctx, NULL);
	  VSTR(val) = "beacon";
	  VSTRLEN(val) = 6;
	  GC_TOUCH (gc_ctx, attr[F_SUBTYPE] = val);
          break;

        case SUBTYPE_DISASSOC:
	  val = ovm_vstr_new (gc_ctx, NULL);
	  VSTR(val) = "disassoc";
	  VSTRLEN(val) = 8;
	  GC_TOUCH (gc_ctx, attr[F_SUBTYPE] = val);
          break;

        case SUBTYPE_AUTH:
	  val = ovm_vstr_new (gc_ctx, NULL);
	  VSTR(val) = "auth";
	  VSTRLEN(val) = 4;
	  GC_TOUCH (gc_ctx, attr[F_SUBTYPE] = val);
          break;

        case SUBTYPE_DEAUTH:
	  val = ovm_vstr_new (gc_ctx, NULL);
	  VSTR(val) = "deauth";
	  VSTRLEN(val) = 6;
	  GC_TOUCH (gc_ctx, attr[F_SUBTYPE] = val);
          break;

        default:
          DebugLog(DF_MOD, DS_DEBUG, "unknown_80211_mgmt_subtype_0x%.2x\n", subtype);
	  val = ovm_vstr_new (gc_ctx, NULL);
	  VSTR(val) = "unknown_mgmt_subtype";
	  VSTRLEN(val) = 20;
	  GC_TOUCH (gc_ctx, attr[F_SUBTYPE] = val);
      }  /* end switch(subtype) */
      break;


    /******************** Dans le cas d'une trame de controle */
    case TYPE_CTL:
      val = ovm_vstr_new (gc_ctx, NULL);
      VSTR(val) = "control";
      VSTRLEN(val) = 7;
      GC_TOUCH (gc_ctx, attr[F_TYPE] = val);

      switch(subtype)
      {
        case SUBTYPE_RTS:
	  val = ovm_vstr_new (gc_ctx, NULL);
	  VSTR(val) = "rts";
	  VSTRLEN(val) = 3;
	  GC_TOUCH (gc_ctx, attr[F_SUBTYPE] = val);

          GETADDR(mac_header->addr1, attr[F_RA]);
          GETADDR(mac_header->addr2, attr[F_TA]);
          break;

        case SUBTYPE_CTS:
	  val = ovm_vstr_new (gc_ctx, NULL);
	  VSTR(val) = "cts";
	  VSTRLEN(val) = 3;
	  GC_TOUCH (gc_ctx, attr[F_SUBTYPE] = val);

          GETADDR(mac_header->addr1, attr[F_RA]);
          break;

        case SUBTYPE_ACK:
	  val = ovm_vstr_new (gc_ctx, NULL);
	  VSTR(val) = "ack";
	  VSTRLEN(val) = 3;
	  GC_TOUCH (gc_ctx, attr[F_SUBTYPE] = val);

          GETADDR(mac_header->addr1, attr[F_RA]);
          break;

        default:
          DebugLog(DF_MOD, DS_DEBUG, "unknown_80211_ctl_subtype_0x%.2x\n", subtype);
	  val = ovm_vstr_new (gc_ctx, NULL);
	  VSTR(val) = "unknown_ctl_subtype";
	  VSTRLEN(val) = 19;
	  GC_TOUCH (gc_ctx, attr[F_SUBTYPE] = val);
	  break;
      }; /* end switch(subtype) */
      break;

    /******************** Dans le cas d'une trame de donnees */
    case TYPE_DATA:
      val = ovm_vstr_new (gc_ctx, NULL);
      VSTR(val) = "data";
      VSTRLEN(val) = 4;
      GC_TOUCH (gc_ctx, attr[F_TYPE] = val);

      /* fixation du champ .wifi.iswep */
      val = ovm_int_new (gc_ctx, (mac_header->fc[1] & PROT) / 64);
      GC_TOUCH (gc_ctx, attr[F_ISWEP] = val);

      /* fixation du champ .wifi.seqnum */
      val = ovm_int_new (gc_ctx,
			 (int) (mac_header->seqnum & SEQ_MASK)	\
			 >> SEQ_SHIFT);
      GC_TOUCH (gc_ctx, attr[F_SEQNUM] = val);

      switch(subtype)
      {
        case SUBTYPE_DATA:
	  val = ovm_vstr_new (gc_ctx, NULL);
	  VSTR(val) = "data";
	  VSTRLEN(val) = 4;
	  GC_TOUCH (gc_ctx, attr[F_SUBTYPE] = val);
          break;
        case SUBTYPE_CF_ACK:
	  val = ovm_vstr_new (gc_ctx, NULL);
	  VSTR(val) = "datacfack";
	  VSTRLEN(val) = 9;
	  GC_TOUCH (gc_ctx, attr[F_SUBTYPE] = val);
          break;
        case SUBTYPE_CF_POLL:
	  val = ovm_vstr_new (gc_ctx, NULL);
	  VSTR(val) = "datacfpoll";
	  VSTRLEN(val) = 10;
	  GC_TOUCH (gc_ctx, attr[F_SUBTYPE] = val);
          break;
        case SUBTYPE_CF_ACPL:
	  val = ovm_vstr_new (gc_ctx, NULL);
	  VSTR(val) = "datacfacpl";
	  VSTRLEN(val) = 10;
	  GC_TOUCH (gc_ctx, attr[F_SUBTYPE] = val);
          break;
        case SUBTYPE_NODATA:
	  val = ovm_vstr_new (gc_ctx, NULL);
	  VSTR(val) = "nodata";
	  VSTRLEN(val) = 6;
	  GC_TOUCH (gc_ctx, attr[F_SUBTYPE] = val);
          break;
        default:
          DebugLog(DF_MOD, DS_DEBUG, "unknown_80211_data_subtype_0x%.2x\n", subtype);
	  val = ovm_vstr_new (gc_ctx, NULL);
	  VSTR(val) = "unknown_data_subtype";
	  VSTRLEN(val) = 20;
	  GC_TOUCH (gc_ctx, attr[F_SUBTYPE] = val);
      } /* end switch(subtype) */

      switch(dir)
	{
        case DIR_NODS:
	  val = ovm_vstr_new (gc_ctx, NULL);
	  VSTR(val) = "nods";
	  VSTRLEN(val) = 4;
	  GC_TOUCH (gc_ctx, attr[F_DIR] = val);

	  val = ovm_int_new (gc_ctx, pkthdr->caplen - 144 - 28);
          GC_TOUCH (gc_ctx, attr[F_BODYLENGTH] = val);

	  /* addr1 = DA
             addr2 = SA
             addr3 = BSSID */

          GETADDR(mac_header->addr1, attr[F_DA]);
          GETADDR(mac_header->addr2, attr[F_SA]);
          GETADDR(mac_header->addr3, attr[F_BSSID]);
          break;

        case DIR_TODS:
	  val = ovm_vstr_new (gc_ctx, NULL);
	  VSTR(val) = "tods";
	  VSTRLEN(val) = 4;
	  GC_TOUCH (gc_ctx, attr[F_DIR] = val);

	  val = ovm_int_new (gc_ctx, pkthdr->caplen - 144 - 28);
          GC_TOUCH (gc_ctx, attr[F_BODYLENGTH] = val);

	  /* addr1 = BSSID
             addr2 = SA
             addr3 = DA */

          GETADDR(mac_header->addr1, attr[F_BSSID]);
          GETADDR(mac_header->addr2, attr[F_SA]);
          GETADDR(mac_header->addr3, attr[F_DA]);
          break;

        case DIR_FROMDS:
	  val = ovm_vstr_new (gc_ctx, NULL);
	  VSTR(val) = "fromds";
	  VSTRLEN(val) = 6;
	  GC_TOUCH (gc_ctx, attr[F_DIR] = val);

	  val = ovm_int_new (gc_ctx, pkthdr->caplen - 144 - 28);
          GC_TOUCH (gc_ctx, attr[F_BODYLENGTH] = val);

	  /* addr1 = DA
             addr2 = BSSID
             addr3 = SA */

          GETADDR(mac_header->addr1, attr[F_DA]);
          GETADDR(mac_header->addr2, attr[F_BSSID]);
          GETADDR(mac_header->addr3, attr[F_SA]);
          break;

        case DIR_DSTODS:
	  val = ovm_vstr_new (gc_ctx, NULL);
	  VSTR(val) = "dstods";
	  VSTRLEN(val) = 6;
	  GC_TOUCH (gc_ctx, attr[F_DIR] = val);

	  val = ovm_int_new (gc_ctx, pkthdr->caplen - 144 - 34);
          GC_TOUCH (gc_ctx, attr[F_BODYLENGTH] = val);

	  /* addr1 = RA
             addr2 = TA
             addr3 = DA
             addr4 = SA */

          GETADDR(mac_header->addr1, attr[F_RA]);
          GETADDR(mac_header->addr2, attr[F_TA]);
          GETADDR(mac_header->addr3, attr[F_DA]);
          GETADDR(mac_header->addr4, attr[F_SA]);
          break;

        default:
	  DebugLog(DF_MOD, DS_DEBUG, "unknown_80211_frame_direction_0x%.2x\n", dir);
	  break;
	} /* end switch(dir) */
      break;

    default:
      DebugLog(DF_MOD, DS_DEBUG, "unknown_80211_frame_type_0x%.2x\n", type);
      break;
    } /* end switch(type) */
}



static int wifi_callback(orchids_t *ctx, mod_entry_t *mod, int fd, void *cap)
{
  int ret = 0;
  gc_t *gc_ctx = ctx->gc_ctx;
  GC_START(gc_ctx, IEEE80211_FIELDS+1);
  wifi_pcap_t data = { gc_ctx, (ovm_var_t **)GC_DATA() };


  DebugLog(DF_MOD, DS_TRACE, "wifi_callback()\n");

  if ( pcap_dispatch(cap, 1, wifi_pcap_callback, (u_char *)&data) == -1 )
    {
      DebugLog(DF_MOD, DS_ERROR, "pcap_dispatch error: %s\n", pcap_geterr(cap));
      ret = -1;
    }
  else
    {
      REGISTER_EVENTS(ctx, mod, IEEE80211_FIELDS, 0);
      if (GC_LOOKUP(IEEE80211_FIELDS)==NULL)
	ret = -1;
    }
  GC_END(gc_ctx);
  return ret;

}

static field_t wifi_fields[] = {
  { "wifi.type",       &t_str, MONO_UNKNOWN,      "frame type"                             },
  { "wifi.subtype",    &t_str, MONO_UNKNOWN,      "frame subtype"                          },
  { "wifi.time",       &t_timeval, MONO_MONO,  "reception time"                         },
  { "wifi.da",         &t_str, MONO_UNKNOWN,      "destination address"                    },
  { "wifi.sa",         &t_str, MONO_UNKNOWN,      "source address"                         },
  { "wifi.bssid",      &t_str, MONO_UNKNOWN,      "basic service set identifier"           },
  { "wifi.ta",         &t_str, MONO_UNKNOWN,      "transmitter address"                    },
  { "wifi.ra",         &t_str, MONO_UNKNOWN,      "receiver address"                       },
  { "wifi.dir",        &t_str, MONO_UNKNOWN,      "direction of the frame"                 },
  { "wifi.seqnum",     &t_int, MONO_UNKNOWN,      "sequence number"                        },
  { "wifi.iswep",      &t_int, MONO_UNKNOWN,      "1 if it is an encrypted frame, 0 else." },
  { "wifi.bodylength", &t_int, MONO_UNKNOWN,      "body length"                            },
  { "wifi.rssi",       &t_int, MONO_UNKNOWN,      "rssi"                                   }
};

static void *
wifi_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  DebugLog(DF_MOD, DS_DEBUG, "load() wifi@%p\n", (void *) &mod_wifi);

  register_fields(ctx, mod, wifi_fields, IEEE80211_FIELDS);

  return (NULL);
}

static void
add_device(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{

  /*  dev: the network device to open
      capd: a packet capture descriptor
   */

  int fd;
  char* dev;
  char errbuf[PCAP_ERRBUF_SIZE]="";
  pcap_t* capd = NULL;

  dev = dir->args;
  DebugLog(DF_MOD, DS_INFO, "Add 802.11 listen device %s\n", dev);

  capd = pcap_open_live(dev, 65535, 1, 1, errbuf);

  if ( strlen(errbuf) != 0 ) {
    DebugLog(DF_MOD, DS_ERROR, "pcap_open_live error: %s\n", errbuf);
    return;
  }

  pcap_setnonblock(capd, 1, NULL);
  fd = pcap_fileno(capd);
  datalink_type = pcap_datalink(capd);

  add_input_descriptor(ctx, mod, wifi_callback, fd, capd);
}

static mod_cfg_cmd_t wifi_dir[] = 
{
  { "AddDevice", add_device, "Add a device to listen to" },
  { NULL, NULL, NULL }
};

input_module_t mod_wifi = {
  MOD_MAGIC,
  ORCHIDS_VERSION,
  0,			    /* flags */
  "wifi",
  NULL,
  NULL,
  wifi_dir,
  wifi_preconfig,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};



/* End-of-file */
