/**
 ** @file mod_pcap.c
 ** Network frame capture with libpcap
 ** 
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 ** 
 ** @version 0.1.0
 ** 
 ** @date  Started on: Fri May 25 14:29:20 2007
 **/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pcap.h>

#include "orchids.h"

#include "orchids_api.h"

#include "mod_pcap.h"

input_module_t mod_pcap;

static void
libpcap_callback(u_char *data,
                 const pcap_pkthdr_t *pkthdr,
                 const u_char *pkt)
{
  ovm_var_t *attr[PCAP_FIELDS];
  event_t *event;
  orchids_t *ctx;
  mod_entry_t *mod;
  mod_pcap_if_t *pcapif;

  DebugLog(DF_MOD, DS_TRACE, "pcap_callback()\n");

  event = NULL;
  memset(attr, 0, sizeof (attr));

  /* decode allocate fields here */

  ctx = ((pcap_cb_data_t *)data)->ctx;
  mod = ((pcap_cb_data_t *)data)->mod;
  pcapif = ((pcap_cb_data_t *)data)->pcapif;

  attr[F_TIME] = ovm_timeval_new();
  TIMEVAL(attr[F_TIME]) = pkthdr->ts;

  attr[F_LEN] = ovm_int_new();
  INT(attr[F_LEN]) = pkthdr->len;

  attr[F_CAPLEN] = ovm_int_new();
  INT(attr[F_CAPLEN]) = pkthdr->caplen;

  attr[F_INTERFACE] = ovm_vstr_new();
  VSTR(attr[F_INTERFACE]) = pcapif->name;
  VSTRLEN(attr[F_INTERFACE]) = strlen(pcapif->name);

  attr[F_DATALINK] = ovm_int_new();
  INT(attr[F_DATALINK]) = pcapif->datalink;

  attr[F_PACKET] = ovm_bstr_new(pkthdr->len);
  memcpy(BSTR(attr[F_PACKET]), pkt, pkthdr->len);

  add_fields_to_event(ctx, mod, &event, attr, PCAP_FIELDS);

  post_event(ctx, mod, event);
}


static int
modpcap_callback(orchids_t *ctx, mod_entry_t *mod, int fd, void *data)
{
  int ret;
  pcap_cb_data_t pcapdata;
  mod_pcap_if_t *pcapif;

  DebugLog(DF_MOD, DS_TRACE, "mod_callback()\n");

  pcapif = (mod_pcap_if_t *)data;

  pcapdata.ctx = ctx;
  pcapdata.mod = mod;
  pcapdata.pcapif = pcapif;

  ret = pcap_dispatch(pcapif->pcap, -1, libpcap_callback, (void*) &pcapdata);
  if (ret == -1) {
    DebugLog(DF_MOD, DS_ERROR,
             "pcap_dispatch error: %s\n",
             pcap_geterr(pcapif->pcap));
    return (-1);
  }

  return (0);
}


static field_t pcap_fields[] = {
  { "pcap.time", T_TIMEVAL, "Time of the reception of the frame"},
  { "pcap.len", T_INT, "Length of the packet on the wire"},
  { "pcap.caplen", T_INT, "Length of data captured" },
  { "pcap.interface", T_VSTR, "Interface name where the packet was captured"},
  { "pcap.datalink", T_INT, "Datalink type" },
  { "pcap.packet", T_BSTR, "The raw bytes of the captured frame"}
};


static void *
pcap_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  DebugLog(DF_MOD, DS_DEBUG, "load() pcap@%p\n", (void *) &mod_pcap);

  register_fields(ctx, mod, pcap_fields, PCAP_FIELDS);

  return (NULL);
}


static void
add_device(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  char errbuf[PCAP_ERRBUF_SIZE];
  mod_pcap_if_t *newif;

  DebugLog(DF_MOD, DS_INFO, "Add device %s\n", dir->args);

  newif = Xzmalloc(sizeof (mod_pcap_if_t));

  newif->name = dir->args;
  newif->promisc = MODPCAP_DEFAULT_PROMISC;
  newif->snaplen = MODPCAP_DEFAULT_SNAPLEN;

  newif->pcap = pcap_open_live(newif->name,
                               newif->snaplen,
                               newif->promisc,
                               MODPCAP_READ_TIMEOUT,
                               errbuf);

  if (newif->pcap == NULL) {
    DebugLog(DF_MOD, DS_ERROR, "pcap_open_live error: %s\n", errbuf);
    Xfree(newif);
    return;
  }

  newif->fd = pcap_get_selectable_fd(newif->pcap);
  if (newif->fd == -1) {
    DebugLog(DF_MOD, DS_ERROR,
             "pcap_get_selectable_fd error: "
             "can't get selectable descriptor on BPF interface '%s'.\n",
             newif->name);
    Xfree(newif);
    return;
  }

  newif->datalink = pcap_datalink(newif->pcap);

  DebugLog(DF_MOD, DS_INFO,
           "Device %s have fd=%i and datalink=%i\n",
           dir->args, newif->fd, newif->datalink);

  add_input_descriptor(ctx, mod, modpcap_callback, newif->fd, newif);
}

static mod_cfg_cmd_t pcap_dir[] = 
{
/*   { "Promiscuous", set_promisc, "Set promiscuous parameter" }, */
/*   { "CaptureLength", set_caplen, "Set the capture length" }, */
  { "AddDevice", add_device, "Add a device to listen to" },
  { NULL, NULL, NULL }
};

input_module_t mod_pcap = {
  MOD_MAGIC,
  ORCHIDS_VERSION,
  "pcap",
  NULL,
  NULL,
  pcap_dir,
  pcap_preconfig,
  NULL,
  NULL
};

/* End-of-file */
