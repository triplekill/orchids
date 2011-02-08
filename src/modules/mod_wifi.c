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
    char buf_mac[ ADDR_STR_SZ + 1 ];                                    \
    (ovmvar) = ovm_str_new( ADDR_STR_SZ );                              \
    sprintf(buf_mac,                                                    \
            "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x",                            \
            (addr)[0], (addr)[1], (addr)[2],                            \
            (addr[3]), (addr[4]), (addr)[5]);                           \
    memcpy(STR(ovmvar), buf_mac, ADDR_STR_SZ);                          \
  } while (0)


void
wifi_pcap_callback(u_char* av, const struct pcap_pkthdr * pkthdr, const u_char * pkt)
{
  ieee80211_header_t mac_header;
  prism2_header_t prism2_header;
  u_int8_t dir, type, subtype;
  ovm_var_t **attr;

  DebugLog(DF_MOD, DS_TRACE, "wifi_pcap_callback()\n");

  attr = (ovm_var_t **) av;

  if (datalink_type == DLT_PRISM_HEADER) {
    prism2_header = (prism2_header_t) pkt;
    mac_header = (ieee80211_header_t) (pkt + 144);

    attr[F_RSSI] = ovm_int_new();
    INT(attr[F_RSSI]) = (&(prism2_header->rssi))->data;
  } else {
      return ;
  }

  /* fixation du champ .wifi.time */
  attr[F_TIME] = ovm_timeval_new();
  attr[F_TIME]->flags |= TYPE_MONO;
  gettimeofday( &(TIMEVAL(attr[F_TIME])) , NULL);

  type = mac_header->fc[0] & 0x0c;
  subtype = mac_header->fc[0] & SUBTYPE_MASK;
  dir = mac_header->fc[1] & DIR_MASK;

  switch (type) {
    /******************** Dans le cas d'une trame de gestion */
    case TYPE_MGT:

      attr[F_TYPE] = ovm_str_new(strlen("management"));
      memcpy(STR(attr[F_TYPE]), "management", STRLEN(attr[F_TYPE]));

      /* fixation du champ .wifi.seqnum */
      attr[F_SEQNUM] = ovm_int_new();
      INT(attr[F_SEQNUM]) = (int) (mac_header->seqnum & SEQ_MASK) \
                               >> SEQ_SHIFT;

      /* addr1 = DA
         addr2 = SA
         addr3 = BSSID */

      GETADDR(mac_header->addr1, attr[F_DA]);
      GETADDR(mac_header->addr2, attr[F_SA]);
      GETADDR(mac_header->addr3, attr[F_BSSID]);

      switch(subtype)
      {
        case SUBTYPE_ASSOC_REQ:
          attr[F_SUBTYPE] = ovm_str_new(strlen("assocreq"));
          memcpy(STR(attr[F_SUBTYPE]), "assocreq", STRLEN(attr[F_SUBTYPE]));
          break;

        case SUBTYPE_ASSOC_RESP:
          attr[F_SUBTYPE] = ovm_str_new(strlen("assocresp"));
          memcpy(STR(attr[F_SUBTYPE]), "assocresp", STRLEN(attr[F_SUBTYPE]));
          break;

        case SUBTYPE_PROBE_REQ:
          attr[F_SUBTYPE] = ovm_str_new(strlen("probereq"));
          memcpy(STR(attr[F_SUBTYPE]), "probereq", STRLEN(attr[F_SUBTYPE]));
          break;

        case SUBTYPE_PROBE_RESP:
          attr[F_SUBTYPE] = ovm_str_new(strlen("proberesp"));
          memcpy(STR(attr[F_SUBTYPE]), "proberesp", STRLEN(attr[F_SUBTYPE]));
          break;

        case SUBTYPE_BEACON:
          attr[F_SUBTYPE] = ovm_str_new(strlen("beacon"));
          memcpy(STR(attr[F_SUBTYPE]), "beacon", STRLEN(attr[F_SUBTYPE]));
          break;

        case SUBTYPE_DISASSOC:
          attr[F_SUBTYPE] = ovm_str_new(strlen("disassoc"));
          memcpy(STR(attr[F_SUBTYPE]), "disassoc", STRLEN(attr[F_SUBTYPE]));
          break;

        case SUBTYPE_AUTH:
          attr[F_SUBTYPE] = ovm_str_new(strlen("auth"));
          memcpy(STR(attr[F_SUBTYPE]), "auth", STRLEN(attr[F_SUBTYPE]));
          break;

        case SUBTYPE_DEAUTH:
          attr[F_SUBTYPE] = ovm_str_new(strlen("deauth"));
          memcpy(STR(attr[F_SUBTYPE]), "deauth", STRLEN(attr[F_SUBTYPE]));
          break;

        default:
          DebugLog(DF_MOD, DS_DEBUG, "unknown_80211_mgmt_subtype_0x%.2x\n", subtype);
          attr[F_SUBTYPE] = ovm_str_new(strlen("unknown_mgmt_subtype"));
          memcpy(STR(attr[F_SUBTYPE]), "unknown_mgmt_subtype", STRLEN(attr[F_SUBTYPE]));
      }  /* end switch(subtype) */

      break;



    /******************** Dans le cas d'une trame de controle */
    case TYPE_CTL:
      attr[F_TYPE] = ovm_str_new(strlen("control"));
      memcpy(STR(attr[F_TYPE]), "control", STRLEN(attr[F_TYPE]));

      switch(subtype)
      {
        case SUBTYPE_RTS:
          attr[F_SUBTYPE] = ovm_str_new(strlen("rts"));
          memcpy(STR(attr[F_SUBTYPE]), "rts", STRLEN(attr[F_SUBTYPE]));

          GETADDR(mac_header->addr1, attr[F_RA]);
          GETADDR(mac_header->addr2, attr[F_TA]);

          break;

        case SUBTYPE_CTS:
          attr[F_SUBTYPE] = ovm_str_new(strlen("cts"));
          memcpy(STR(attr[F_SUBTYPE]), "cts", STRLEN(attr[F_SUBTYPE]));

          GETADDR(mac_header->addr1, attr[F_RA]);

          break;

        case SUBTYPE_ACK:
          attr[F_SUBTYPE] = ovm_str_new(strlen("ack"));
          memcpy(STR(attr[F_SUBTYPE]), "ack", STRLEN(attr[F_SUBTYPE]));

          GETADDR(mac_header->addr1, attr[F_RA]);

          break;

        default:
          DebugLog(DF_MOD, DS_DEBUG, "unknown_80211_ctl_subtype_0x%.2x\n", subtype);
          attr[F_SUBTYPE] = ovm_str_new(strlen("unknown_ctl_subtype"));
          memcpy(STR(attr[F_SUBTYPE]), "unknown_ctl_subtype", STRLEN(attr[F_SUBTYPE]));

      }; /* end switch(subtype) */

      break;



    /******************** Dans le cas d'une trame de donnees */
    case TYPE_DATA:

      attr[F_TYPE] = ovm_str_new(strlen("data"));
      memcpy(STR(attr[F_TYPE]), "data", STRLEN(attr[F_TYPE]));

      /* fixation du champ .wifi.iswep */
      attr[F_ISWEP] = ovm_int_new();
      INT(attr[F_ISWEP]) = (mac_header->fc[1] & PROT) / 64;

      /* fixation du champ .wifi.seqnum */
      attr[F_SEQNUM] = ovm_int_new();
      INT(attr[F_SEQNUM]) = (int) (mac_header->seqnum & SEQ_MASK) \
                               >> SEQ_SHIFT;

      switch(subtype)
      {
        case SUBTYPE_DATA:
          attr[F_SUBTYPE] = ovm_str_new(strlen("data"));
          memcpy(STR(attr[F_SUBTYPE]), "data", STRLEN(attr[F_SUBTYPE]));

          break;
        case SUBTYPE_CF_ACK:
          attr[F_SUBTYPE] = ovm_str_new(strlen("datacfack"));
          memcpy(STR(attr[F_SUBTYPE]), "datacfack", STRLEN(attr[F_SUBTYPE]));

          break;
        case SUBTYPE_CF_POLL:
          attr[F_SUBTYPE] = ovm_str_new(strlen("datacfpoll"));
          memcpy(STR(attr[F_SUBTYPE]), "datacfpoll", STRLEN(attr[F_SUBTYPE]));

          break;
        case SUBTYPE_CF_ACPL:
          attr[F_SUBTYPE] = ovm_str_new(strlen("datacfacpl"));
          memcpy(STR(attr[F_SUBTYPE]), "datacfacpl", STRLEN(attr[F_SUBTYPE]));

          break;
        case SUBTYPE_NODATA:
          attr[F_SUBTYPE] = ovm_str_new(strlen("nodata"));
          memcpy(STR(attr[F_SUBTYPE]), "nodata", STRLEN(attr[F_SUBTYPE]));

          break;

        default:
          DebugLog(DF_MOD, DS_DEBUG, "unknown_80211_data_subtype_0x%.2x\n", subtype);
          attr[F_SUBTYPE] = ovm_str_new(strlen("unknown_data_subtype"));
          memcpy(STR(attr[F_SUBTYPE]), "unknown_data_subtype", STRLEN(attr[F_SUBTYPE]));

      } /* end switch(subtype) */

      switch(dir)
      {
        case DIR_NODS:

          attr[F_DIR] = ovm_str_new(strlen("nods"));
          memcpy(STR(attr[F_DIR]), "nods", STRLEN(attr[F_DIR]));

          attr[F_BODYLENGTH] = ovm_int_new();
          INT(attr[F_BODYLENGTH]) = pkthdr->caplen - 144 - 28 ;

	  /* addr1 = DA
             addr2 = SA
             addr3 = BSSID */

          GETADDR(mac_header->addr1, attr[F_DA]);
          GETADDR(mac_header->addr2, attr[F_SA]);
          GETADDR(mac_header->addr3, attr[F_BSSID]);

          break;


        case DIR_TODS:

          attr[F_DIR] = ovm_str_new(strlen("tods"));
          memcpy(STR(attr[F_DIR]), "tods", STRLEN(attr[F_DIR]));

          attr[F_BODYLENGTH] = ovm_int_new();
          INT(attr[F_BODYLENGTH]) = pkthdr->caplen - 144 - 28 ;

	  /* addr1 = BSSID
             addr2 = SA
             addr3 = DA */

          GETADDR(mac_header->addr1, attr[F_BSSID]);
          GETADDR(mac_header->addr2, attr[F_SA]);
          GETADDR(mac_header->addr3, attr[F_DA]);

          break;


        case DIR_FROMDS:

          attr[F_DIR] = ovm_str_new(strlen("fromds"));
          memcpy(STR(attr[F_DIR]), "fromds", STRLEN(attr[F_DIR]));

          attr[F_BODYLENGTH] = ovm_int_new();
          INT(attr[F_BODYLENGTH]) = pkthdr->caplen - 144 - 28 ;

	  /* addr1 = DA
             addr2 = BSSID
             addr3 = SA */

          GETADDR(mac_header->addr1, attr[F_DA]);
          GETADDR(mac_header->addr2, attr[F_BSSID]);
          GETADDR(mac_header->addr3, attr[F_SA]);

          break;



        case DIR_DSTODS:

          attr[F_DIR] = ovm_str_new(strlen("dstods"));
          memcpy(STR(attr[F_DIR]), "fromds", STRLEN(attr[F_DIR]));

          attr[F_BODYLENGTH] = ovm_int_new();
          INT(attr[F_BODYLENGTH]) = pkthdr->caplen - 144 - 34 ;

	  /* addr1 = RA
             addr2 = TA
             addr3 = DA
             addr4 = SA */

          GETADDR(mac_header->addr1, attr[F_RA]);
          GETADDR(mac_header->addr2, attr[F_TA]);
          GETADDR(mac_header->addr3, attr[F_DA]);
          GETADDR(mac_header->addr4, attr[F_SA]);

          break;

        default: DebugLog(DF_MOD, DS_DEBUG, "unknown_80211_frame_direction_0x%.2x\n", dir);

      } /* end switch(dir) */

      break;

    default: DebugLog(DF_MOD, DS_DEBUG, "unknown_80211_frame_type_0x%.2x\n", type);

  } /* end switch(type) */

}



static int
wifi_callback(orchids_t *ctx, mod_entry_t *mod, int fd, void *cap)
{
  event_t *event;
  ovm_var_t *attr[IEEE80211_FIELDS];


  DebugLog(DF_MOD, DS_TRACE, "wifi_callback()\n");

  memset(attr, 0, sizeof(attr));

  if ( pcap_dispatch(cap, 1, wifi_pcap_callback, (u_char*) attr) == -1 )
  {
    DebugLog(DF_MOD, DS_ERROR, "pcap_dispatch error: %s\n", pcap_geterr(cap));
    return (-1);
  }

  event = NULL;

  add_fields_to_event(ctx, mod, &event, attr, IEEE80211_FIELDS);

  if (event == NULL)
    return (-1);

  post_event(ctx, mod, event);

  return (0);

}

static field_t wifi_fields[] = {
  { "wifi.type",       T_STR,      "frame type"                             },
  { "wifi.subtype",    T_STR,      "frame subtype"                          },
  { "wifi.time",       T_TIMEVAL,  "reception time"                         },
  { "wifi.da",         T_STR,      "destination address"                    },
  { "wifi.sa",         T_STR,      "source address"                         },
  { "wifi.bssid",      T_STR,      "basic service set identifier"           },
  { "wifi.ta",         T_STR,      "transmitter address"                    },
  { "wifi.ra",         T_STR,      "receiver address"                       },
  { "wifi.dir",        T_STR,      "direction of the frame"                 },
  { "wifi.seqnum",     T_INT,      "sequence number"                        },
  { "wifi.iswep",      T_INT,      "1 if it is an encrypted frame, 0 else." },
  { "wifi.bodylength", T_INT,      "body length"                            },
  { "wifi.rssi",       T_INT,      "rssi"                                   }
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
  "wifi",
  NULL,
  NULL,
  wifi_dir,
  wifi_preconfig,
  NULL,
  NULL
};



/* End-of-file */
