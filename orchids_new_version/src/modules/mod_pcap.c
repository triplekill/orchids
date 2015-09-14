/**
 ** @file mod_pcap.c
 ** Network frame capture with libpcap
 ** 
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 ** @author Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
 ** 
 ** @version 0.1
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

static void libpcap_callback(u_char *data,
			     const pcap_pkthdr_t *pkthdr,
			     const u_char *pkt);

static int modpcap_callback(orchids_t *ctx, mod_entry_t *mod,
			    int fd, void *data);

static void *pcap_preconfig(orchids_t *ctx, mod_entry_t *mod);

static void add_device(orchids_t *ctx, mod_entry_t *mod,
		       config_directive_t *dir);


#ifdef OBSOLETE
static void mod_pcap_if_mark_subfields (gc_t *gc_ctx, gc_header_t *p)
{
  mod_pcap_if_t *pcapif = (mod_pcap_if_t *)p;

  GC_TOUCH (gc_ctx, pcapif->name);
}

static void mod_pcap_if_finalize (gc_t *gc_ctx, gc_header_t *p)
{
  pcap_t *pcap = ((mod_pcap_if_t *)p)->pcap;

  if (pcap!=NULL)
    pcap_close (pcap);
}

static int mod_pcap_if_traverse (gc_traverse_ctx_t *gtc, gc_header_t *p,
				 void *data)
{
  mod_pcap_if_t *pcapif = (mod_pcap_if_t *)p;
  int err = 0;

  if (pcapif->name!=NULL)
    err = (*gtc->do_subfield) (gtc, (gc_header_t *)pcapif->name, data);
  return err;
}

static gc_class_t mod_pcap_if_class = {
  GC_ID('p','c','a','p'),
  mod_pcap_if_mark_subfields,
  mod_pcap_if_finalize,
  mod_pcap_if_traverse,
  NULL, /* it is in principle not legal to leave the save and restore
	   hooks to NULL, but mod_pcap_if_class objects are not meant to be
	   saved or restored */
  NULL
};
#endif

static void libpcap_callback(u_char *data,
			     const pcap_pkthdr_t *pkthdr,
			     const u_char *pkt)
{
  ovm_var_t *var;
  orchids_t *ctx = ((pcap_cb_data_t *)data)->ctx;
  gc_t *gc_ctx = ctx->gc_ctx;
  mod_entry_t *mod = ((pcap_cb_data_t *)data)->mod;
  mod_pcap_if_t *pcapif = ((pcap_cb_data_t *)data)->pcapif;

  GC_START(gc_ctx, PCAP_FIELDS+1);
  /* PCAP_FIELDS fields, plus the event to be built */

  DebugLog(DF_MOD, DS_TRACE, "pcap_callback()\n");

  var = ovm_timeval_new (gc_ctx);
  TIMEVAL(var) = pkthdr->ts;
  GC_UPDATE(gc_ctx, F_TIME, var);

  var = ovm_int_new (gc_ctx, pkthdr->len);
  GC_UPDATE(gc_ctx, F_LEN, var);

  var = ovm_int_new (gc_ctx, pkthdr->caplen);
  GC_UPDATE(gc_ctx, F_CAPLEN, var);

  GC_UPDATE(gc_ctx, F_INTERFACE, pcapif->name);

  var = ovm_int_new (gc_ctx, pcapif->datalink);
  GC_UPDATE(gc_ctx, F_DATALINK, var);

  var = ovm_bstr_new(gc_ctx, pkthdr->len);
  memcpy(BSTR(var), pkt, pkthdr->len);
  GC_UPDATE(gc_ctx, F_PACKET, var);

  REGISTER_EVENTS(ctx, mod, PCAP_FIELDS, 0);
  GC_END(gc_ctx);
}


static int modpcap_callback(orchids_t *ctx, mod_entry_t *mod, int fd, void *data)
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
    return -1;
  }
  return 0;
}

static field_t pcap_fields[] = {
  { "pcap.time", &t_timeval, MONO_MONO, "Time of the reception of the frame"},
  { "pcap.len", &t_int, MONO_UNKNOWN, "Length of the packet on the wire"},
  { "pcap.caplen", &t_int, MONO_UNKNOWN, "Length of data captured" },
  { "pcap.interface", &t_str, MONO_UNKNOWN, "Interface name where the packet was captured"},
  { "pcap.datalink", &t_int, MONO_UNKNOWN, "Datalink type" },
  { "pcap.packet", &t_bstr, MONO_UNKNOWN, "The raw bytes of the captured frame"}
};


static void *pcap_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  DebugLog(DF_MOD, DS_DEBUG, "load() pcap@%p\n", (void *) &mod_pcap);
  register_fields(ctx, mod, pcap_fields, PCAP_FIELDS);
  return NULL;
}

static void add_device(orchids_t *ctx, mod_entry_t *mod, config_directive_t *dir)
{
  char errbuf[PCAP_ERRBUF_SIZE];
  mod_pcap_if_t *newif;
  gc_t *gc_ctx = ctx->gc_ctx;
  ovm_var_t *name;
  size_t len;

  DebugLog(DF_MOD, DS_INFO, "Add device %s\n", dir->args);

  GC_START(gc_ctx, 1);
  len = strlen(dir->args);
  name = ovm_str_new(gc_ctx, len);
  memcpy (STR(name), dir->args, len);
  GC_UPDATE(gc_ctx, 0, name);

  newif = gc_base_malloc (gc_ctx, sizeof (mod_pcap_if_t));

  newif->promisc = MODPCAP_DEFAULT_PROMISC;
  newif->snaplen = MODPCAP_DEFAULT_SNAPLEN;
  newif->pcap = pcap_open_live(dir->args,
                               newif->snaplen,
                               newif->promisc,
                               MODPCAP_READ_TIMEOUT,
                               errbuf);

  if (newif->pcap == NULL)
    {
      DebugLog(DF_MOD, DS_ERROR, "pcap_open_live error: %s\n", errbuf);
    }
  else
    {
      newif->fd = pcap_get_selectable_fd(newif->pcap);
      if (newif->fd == -1)
	{
	  DebugLog(DF_MOD, DS_ERROR,
		   "pcap_get_selectable_fd error: "
		   "can't get selectable descriptor on BPF interface '%s'.\n",
		   dir->args);
	  pcap_close (newif->pcap);
	  gc_base_free (newif);
	}
      else
	{
	  newif->datalink = pcap_datalink(newif->pcap);
	  DebugLog(DF_MOD, DS_INFO,
		   "Device %s have fd=%i and datalink=%i\n",
		   dir->args, newif->fd, newif->datalink);
	  GC_TOUCH (gc_ctx, newif->name = name);
	  gc_add_root(gc_ctx, (gc_header_t **)&newif->name);

	  add_input_descriptor(ctx, mod, modpcap_callback, newif->fd, newif);
	}
    }
  GC_END(gc_ctx);
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
  MODULE_DISSECTABLE,
  "pcap",
  NULL,
  NULL,
  pcap_dir,
  pcap_preconfig,
  NULL, /* postconfig */
  NULL, /* postcompil */
  NULL, /* predissect */
  NULL, /* dissect */
  NULL, /* dissect type */
  NULL, /* save */
  NULL, /* restore */
};

/*
** Copyright (c) 2007 by Julien OLIVAIN, Laboratoire Spécification
** et Vérification (LSV), CNRS UMR 8643 & ENS Cachan.
** Copyright (c) 2014-2015 by Jean GOUBAULT-LARRECQ, Laboratoire Spécification
** et Vérification (LSV), CNRS UMR 8643 & ENS Cachan.
**
** Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
** Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
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
