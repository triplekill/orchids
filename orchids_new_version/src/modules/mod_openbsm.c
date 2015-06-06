/**
 ** @file mod_openbsm.c
 ** Dissection module for open BSM
 **
 ** @author Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 ** @date  Started on: Jeu  2 oct 2014 17:38:33 UTC
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

#include "orchids.h"

#include "orchids_api.h"
#include "mod_mgr.h"
#include "mod_utils.h"

#include "mod_openbsm.h"

input_module_t mod_openbsm;

#define AUT_HEADER32 0x14
#define AUT_HEADER32_EX 0x15
#define AUT_HEADER64 0x74
#define AUT_HEADER64_EX 0x79
#define AUT_OTHER_FILE32 0x11
#define AUT_TRAILER 0x13
#define AUT_ARG32 0x2d
#define AUT_ARG64 0x71
#define AUT_DATA 0x21
#define AUT_ATTR32 0x3e
#define AUT_ATTR64 0x73
#define AUT_EXIT 0x52
#define AUT_EXEC_ARGS 0x3c
#define AUT_EXEC_ENV 0x3d
#define AUT_NEWGROUPS 0x3b
#define AUT_IN_ADDR 0x2a
#define AUT_IN_ADDR_EX 0x7e
#define AUT_IP 0x2b
#define AUT_IPC 0x22
#define AUT_IPC_PERM 0x32
#define AUT_IPORT 0x2c
#define AUT_OPAQUE 0x29
#define AUT_PATH 0x23
#define AUT_PROCESS32 0x26
#define AUT_PROCESS64 0x77
#define AUT_PROCESS32_EX 0x7b
#define AUT_PROCESS64_EX 0x7d
#define AUT_RETURN32 0x27
#define AUT_RETURN64 0x72
#define AUT_SEQ 0x2f
#define AUT_SOCKET 0x2e
#define AUT_SOCKET_EX 0x7f
#define AUT_SUBJECT32 0x24
#define AUT_SUBJECT32_EX 0x7a
#define AUT_SUBJECT64 0x75
#define AUT_SUBJECT64_EX 0x7c
#define AUT_TEXT 0x28
#define AUT_ZONENAME 0x60
// The following from Apple BSM, not present in Sun BSM:
#define AUT_SOCKINET32 0x80
#define AUT_SOCKINET128 0x81
#define AUT_SOCKUNIX 0x82

/* Read int32 and other data types in network byte order (=big-endian)
   We do not use htonl(), htons(), ntohl(), ntohs() since the latter
   may assume aligned data
 */
/*
#define INT64_FROM_CHARS(bytes)						\
  ((((uint64_t)INT32_FROM_CHARS(bytes))<<32) | (uint64_t)INT32_FROM_CHARS(bytes+4))
*/

#define INT64_FROM_CHARS(bytes)	(					\
  (((uint64_t)(unsigned char)(bytes)[0])<<56) |			\
  (((uint64_t)(unsigned char)(bytes)[1])<<48) |			\
  (((uint64_t)(unsigned char)(bytes)[2])<<40) |			\
  (((uint64_t)(unsigned char)(bytes)[3])<<32) |			\
  (((uint64_t)(unsigned char)(bytes)[4])<<24) |			\
  (((uint64_t)(unsigned char)(bytes)[5])<<16) |			\
  (((uint64_t)(unsigned char)(bytes)[6])<<8) |			\
   ((uint64_t)(unsigned char)(bytes)[7])				\
   )
#define INT32_FROM_CHARS(bytes)	(					\
  (((uint32_t)(unsigned char)(bytes)[0])<<24) |			\
  (((uint32_t)(unsigned char)(bytes)[1])<<16) |			\
  (((uint32_t)(unsigned char)(bytes)[2])<<8) |			\
   ((uint32_t)(unsigned char)(bytes)[3])				\
   )
#define INT16_FROM_CHARS(bytes)	(					\
   (((uint16_t)(unsigned char)(bytes)[0])<<8) | \
    ((uint16_t)(unsigned char)(bytes)[1])				\
   )

#define STATE_HEADER BLOX_NSTATES+1
#define STATE_FILE_EXPECT_FILENAMELEN BLOX_NSTATES+2

static size_t openbsm_compute_length (unsigned char *first_bytes,
				      size_t n_first_bytes,
				      size_t available_bytes,
				      /* available_bytes is ignored here */
				      int *state,
				      void *sd_data)
{ /* n_first_bytes should be 5: 1 AUT_HEADER* byte + 4 bytes of length, in big-endian format */
  int c;

  switch (*state)
    {
    case BLOX_INIT: /* then n_first_bytes==1 */
      c = (int)(unsigned int)first_bytes[0];
      switch (c)
	{
	case AUT_HEADER32:
	  /* AUT_HEADER32 (1 byte), size (4 bytes), version (1 byte),
	     e_type (2 bytes), e_mod (2 bytes), s (4 bytes), ms (4 bytes):
	     need to read 5 bytes to obtain size */
	case AUT_HEADER32_EX:
	  /* AUT_HEADER32_EX (1 byte), size (4 bytes), version (1 byte),
	     e_type (2 bytes), e_mod (2 bytes), ad_type (4 bytes, value==4 for IPV4, 16 for IPV6),
	     addr (ad_type bytes), s (4 bytes), ms (4 bytes):
	     need to read 5 bytes to obtain size */
	case AUT_HEADER64:
	  /* AUT_HEADER64 (1 byte), size (4 bytes), version (1 byte),
	     e_type (2 bytes), e_mod (2 bytes), s (8 bytes), ms (8 bytes):
	     need to read 5 bytes to obtain size */
	case AUT_HEADER64_EX:
	  /* AUT_HEADER32_EX (1 byte), size (4 bytes), version (1 byte),
	     e_type (2 bytes), e_mod (2 bytes), ad_type (4 bytes, value==4 for IPV4, 16 for IPV6),
	     addr (ad_type bytes), s (8 bytes), ms (8 bytes):
	     need to read 5 bytes to obtain size */
	  *state = STATE_HEADER;
	  return 5;
	case AUT_OTHER_FILE32:
	  /* AUT_OTHER_FILE32 (1 byte), sec (4 bytes), msec (4 bytes),
	     filenamelen (2 bytes), filename (filenamelen bytes):
	     need to read 11 bytes to obtain filnamelen */
	  *state = STATE_FILE_EXPECT_FILENAMELEN;
	  return 11;
	default:
	  *state = BLOX_NOT_ALIGNED;
	  return 1;
	}
    case STATE_HEADER: /* then n_first_bytes==5 */
      *state = BLOX_FINAL;
      return INT32_FROM_CHARS(&first_bytes[1]);
    case STATE_FILE_EXPECT_FILENAMELEN:
      /* See case AUT_OTHER_FILE32 above.
	 We have n_first_bytes==11 here.  The last two bytes are the file name length.
	 The final size is 11 plus the file name length.
       */
      *state = BLOX_FINAL;
      return 11+INT16_FROM_CHARS(&first_bytes[9]);
    default:
      DebugLog(DF_MOD,DS_ERROR, "Impossible blox state");
      *state = BLOX_INIT;
      return 0;
    }
}

static field_t openbsm_fields[] = {
  { "openbsm.kind", &t_uint, MONO_UNKNOWN, "header kind" },
  { "openbsm.version", &t_uint, MONO_UNKNOWN, "version (in header)" },
  { "openbsm.type", &t_uint, MONO_UNKNOWN, "type (in header)" },
  { "openbsm.modifier", &t_uint, MONO_UNKNOWN, "modifier (in header)" },
  { "openbsm.time", &t_timeval, MONO_MONO, "time (in header)" },
  { "openbsm.ip", &t_ipv6, MONO_UNKNOWN, "IPv6 (or IPv4, encoded as IPv6) address (in header)" },
  { "openbsm.file", &t_str, MONO_UNKNOWN, "file name (in header)" },
  { "openbsm.data", &t_str, MONO_UNKNOWN, "raw data, printed as a string (in data token)" },
  { "openbsm.file_access_mode", &t_uint, MONO_UNKNOWN, "file access mode (in attr token)" },
  { "openbsm.owner_uid", &t_uint, MONO_UNKNOWN, "owner user id (in attr token)" },
  { "openbsm.owner_gid", &t_uint, MONO_UNKNOWN, "owner group id (in attr token)" },
  { "openbsm.fsid", &t_uint, MONO_UNKNOWN, "file system id (in attr token)" },
  { "openbsm.nid", &t_uint, MONO_UNKNOWN, "node id (in attr token)" }, // !!! should really be 64 bits!
  { "openbsm.dev", &t_uint, MONO_UNKNOWN, "device id (in attr token)" },
  { "openbsm.exit_status", &t_uint, MONO_UNKNOWN, "exit status (in exit token)" },
  { "openbsm.exit_value", &t_uint, MONO_UNKNOWN, "exit return value (in exit token)" },
  { "openbsm.inaddr", &t_ipv4, MONO_UNKNOWN, "IPv4 address (in in_addr token)" },
  { "openbsm.inaddr6", &t_ipv6, MONO_UNKNOWN, "IPv6 (or IPv4, encoded as IPv6) address (in in_addr_ex token)" },
  { "openbsm.ip_version", &t_uint, MONO_UNKNOWN, "IP header: version (in ip token)" },
  { "openbsm.ip_tos", &t_uint, MONO_UNKNOWN, "IP header: type of service (in ip token)" },
  { "openbsm.ip_len", &t_uint, MONO_UNKNOWN, "IP header: length (in ip token)" },
  { "openbsm.ip_id", &t_uint, MONO_UNKNOWN, "IP header: identifier (in ip token)" },
  { "openbsm.ip_offset", &t_uint, MONO_UNKNOWN, "IP header: offset (in ip token)" },
  { "openbsm.ip_ttl", &t_uint, MONO_UNKNOWN, "IP header: time to live (in ip token)" },
  { "openbsm.ip_protocol", &t_uint, MONO_UNKNOWN, "IP header: protocol (in ip token)" },
  { "openbsm.ip_checksum", &t_uint, MONO_UNKNOWN, "IP header: checksum (in ip token)" },
  { "openbsm.ip_source", &t_ipv4, MONO_UNKNOWN, "IP header: source address (in ip token)" },
  { "openbsm.ip_dest", &t_ipv4, MONO_UNKNOWN, "IP header: destination address (in ip token)" },
  { "openbsm.ipc_type", &t_uint, MONO_UNKNOWN, "object type (in ipc token)" },
  { "openbsm.ipc_id", &t_uint, MONO_UNKNOWN, "object identifier (in ipc token)" },
  { "openbsm.ipcperm_uid", &t_uint, MONO_UNKNOWN, "owner user id (in ipcperm token)" },
  { "openbsm.ipcperm_gid", &t_uint, MONO_UNKNOWN, "owner group id (in ipcperm token)" },
  { "openbsm.ipcperm_puid", &t_uint, MONO_UNKNOWN, "creator user id (in ipcperm token)" },
  { "openbsm.ipcperm_pgid", &t_uint, MONO_UNKNOWN, "creator group id (in ipcperm token)" },
  { "openbsm.ipcperm_mode", &t_uint, MONO_UNKNOWN, "access mode (in ipcperm token)" },
  { "openbsm.ipcperm_seq", &t_uint, MONO_UNKNOWN, "slot sequence number (in ipcperm token)" },
  { "openbsm.ipcperm_key", &t_uint, MONO_UNKNOWN, "key (in ipcperm token)" },
  { "openbsm.iport", &t_uint, MONO_UNKNOWN, "IP port (in iport token)" },
  { "openbsm.opaque", &t_bstr, MONO_UNKNOWN, "opaque data (in opaque token)" },
  { "openbsm.path", &t_str, MONO_UNKNOWN, "path (in path token)" },
  { "openbsm.proc_auid", &t_uint, MONO_UNKNOWN, "audit id (in process token)" },
  { "openbsm.proc_euid", &t_uint, MONO_UNKNOWN, "effective user id (in process token)" },
  { "openbsm.proc_egid", &t_uint, MONO_UNKNOWN, "effective group id (in process token)" },
  { "openbsm.proc_ruid", &t_uint, MONO_UNKNOWN, "real user id (in process token)" },
  { "openbsm.proc_rgid", &t_uint, MONO_UNKNOWN, "real group id (in process token)" },
  { "openbsm.proc_pid", &t_uint, MONO_UNKNOWN, "process id (in process token)" },
  { "openbsm.proc_sid", &t_uint, MONO_UNKNOWN, "session id (in process token)" },
  { "openbsm.proc_port", &t_uint, MONO_UNKNOWN, "port id (in process token)" },
  { "openbsm.proc_addr", &t_ipv6, MONO_UNKNOWN, "machine id, as an IPv6 (or IPv4, encoded as IPv6) address (in process token)" },
  { "openbsm.subj_auid", &t_uint, MONO_UNKNOWN, "audit id (in subject token)" },
  { "openbsm.subj_euid", &t_uint, MONO_UNKNOWN, "effective user id (in subject token)" },
  { "openbsm.subj_egid", &t_uint, MONO_UNKNOWN, "effective group id (in subject token)" },
  { "openbsm.subj_ruid", &t_uint, MONO_UNKNOWN, "real user id (in subject token)" },
  { "openbsm.subj_rgid", &t_uint, MONO_UNKNOWN, "real group id (in subject token)" },
  { "openbsm.subj_pid", &t_uint, MONO_UNKNOWN, "process id (in subject token)" },
  { "openbsm.subj_sid", &t_uint, MONO_UNKNOWN, "session id (in subject token)" },
  { "openbsm.subj_port", &t_uint, MONO_UNKNOWN, "port id (in subject token)" },
  { "openbsm.subj_addr", &t_ipv6, MONO_UNKNOWN, "machine id, as an IPv6 (or IPv4, encoded as IPv6) address (in subject token)" },
  { "openbsm.return_status", &t_uint, MONO_UNKNOWN, "return status (in return token)" },
  { "openbsm.return_value", &t_uint, MONO_UNKNOWN, "return value (in return token)" },
  { "openbsm.seqno", &t_uint, MONO_UNKNOWN, "sequence number (in seq token)" },
  { "openbsm.sock_family", &t_uint, MONO_UNKNOWN, "socket family (in sockinet and sockunix tokens)" },
  { "openbsm.sock_port", &t_uint, MONO_UNKNOWN, "socket local port (in sockinet token)" },
  { "openbsm.sock_addr", &t_ipv6, MONO_UNKNOWN, "socket address (in sockinet token)" },
  { "openbsm.sock_path", &t_str, MONO_UNKNOWN, "socket path (in sockunix token)" },
  { "openbsm.socket_domain", &t_uint, MONO_UNKNOWN, "socket domain (in socket token)" },
  { "openbsm.socket_type", &t_uint, MONO_UNKNOWN, "socket type (in socket token)" },
  { "openbsm.socket_lport", &t_uint, MONO_UNKNOWN, "socket local port (in socket token)" },
  { "openbsm.socket_laddr", &t_ipv6, MONO_UNKNOWN, "socket IPv6 (or IPv4 encoded as IPv6) local address (in socket token)" },
  { "openbsm.socket_rport", &t_uint, MONO_UNKNOWN, "socket remote port (in socket token)" },
  { "openbsm.socket_raddr", &t_ipv6, MONO_UNKNOWN, "socket IPv6 (or IPv4 encoded as IPv6) remote address (in socket token)" },
  { "openbsm.text", &t_str, MONO_UNKNOWN, "text (in text token)" },
  { "openbsm.zonename", &t_str, MONO_UNKNOWN, "zone name (in zonename token)" },
  // The following file generated automatically by gen_mod_openbsm.c
  // handles the bsm.arg<n> fields, etc.
#include "defs_openbsm.h"
  // Do not add new fields after this line

};

static void openbsm_subdissect (orchids_t *ctx, mod_entry_t *mod,
				event_t *event,
				ovm_var_t *delegate,
				unsigned char *stream, size_t stream_len,
				void *sd_data,
				int dissector_level)
{
  gc_t *gc_ctx = ctx->gc_ctx;
  unsigned char *stream_end = stream+stream_len;
  ovm_var_t *val;
  event_t **evtp;
  int c, num;
  int ad_type;
  int with_size_n; /* do not use: will be handled through WITH_SIZE() macro */
  size_t size;
  size_t bu, uc;
  size_t width;
  char *format;
  char howtoprint;
  size_t maxlen;
  size_t i;
  char *s, *t;
  int space;

  GC_START(gc_ctx, 1);
  GC_UPDATE(gc_ctx, 0, event);
  evtp = (event_t **)&GC_LOOKUP(0);
#define F_WRAP(type) add_fields_to_event_stride(ctx, mod, evtp, (ovm_var_t **)GC_DATA(), \
						F_OPENBSM_ ## type ## _START, \
						F_OPENBSM_ ## type ## _END);
#define WITH_SIZE(n,lab)			\
    if (stream+(n)>stream_end) goto lab;	\
    for (with_size_n=1; with_size_n; stream += (n), with_size_n=0)
  while (stream < stream_end)
    {
      c = (int)(unsigned int)*stream++;
      switch (c)
	{
	case AUT_HEADER32:
	case AUT_HEADER32_EX:
	case AUT_HEADER64:
	case AUT_HEADER64_EX:
	  WITH_SIZE(4,hdr_wrap); /* skip uint32_t size */
	  GC_START(gc_ctx, F_OPENBSM_HDR_SIZE);
	  GC_UPDATE(gc_ctx, F_OPENBSM_HDR_KIND,
		    ovm_uint_new (gc_ctx, (unsigned int)c));
	  WITH_SIZE(1,hdr_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_HDR_VERSION,
			ovm_uint_new (gc_ctx, (unsigned int)stream[0]));
	    }
	  WITH_SIZE(2,hdr_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_HDR_TYPE,
			ovm_uint_new (gc_ctx, INT16_FROM_CHARS(stream)));
	    }
	  WITH_SIZE(2,hdr_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_HDR_MODIFIER,
			ovm_uint_new (gc_ctx, INT16_FROM_CHARS(stream)));
	    }
	  if (c==AUT_HEADER32_EX || c==AUT_HEADER64_EX)
	    {
	      WITH_SIZE(4,hdr_wrap)
		ad_type = INT32_FROM_CHARS(stream);
	      val = ovm_ipv6_new(gc_ctx);
	      switch (ad_type)
		{
		case 4: /* IPv4 */
		  /* convert IPv4 address to IPv6: */
		  WITH_SIZE(4,hdr_wrap)
		    {
		      memcpy (IPV6(val).s6_addr+12, stream, 4);
		      IPV6(val).s6_addr[11] = 0xff;
		      IPV6(val).s6_addr[10] = 0xff;
		      memset(IPV6(val).s6_addr, 0, 10);
		    }
		  break;
		case 16: /* IPv6 */
		  WITH_SIZE(16,hdr_wrap)
		    memcpy (IPV6(val).s6_addr, stream, 16);
		  break;
		default: goto hdr_wrap;
		}
	      GC_UPDATE(gc_ctx, F_OPENBSM_HDR_IP, val);
	    }
	  GC_UPDATE(gc_ctx, F_OPENBSM_HDR_TIME, val = ovm_timeval_new (gc_ctx));
	  if (c==AUT_HEADER64 || c==AUT_HEADER64_EX)
	    {
	      WITH_SIZE(16,hdr_wrap)
		{
		  TIMEVAL(val).tv_sec = INT32_FROM_CHARS(stream+4);
		  /* ignore leading 4 bytes of 8 byte integer */
		  TIMEVAL(val).tv_usec = INT32_FROM_CHARS(stream+12) * 1000;
		  /* ignore leading 4 bytes of 8 byte integer */
		}
	    }
	  else
	    {
	      WITH_SIZE(8,hdr_wrap)
		{
		  TIMEVAL(val).tv_sec = INT32_FROM_CHARS(stream);
		  TIMEVAL(val).tv_usec = INT32_FROM_CHARS(stream+4) * 1000;
		}
	    }
	hdr_wrap:
	  F_WRAP(HDR);
	  GC_END(gc_ctx);
	  break;
	case AUT_OTHER_FILE32:
	  GC_START(gc_ctx, F_OPENBSM_HDR_SIZE);
	  GC_UPDATE(gc_ctx, F_OPENBSM_HDR_KIND,
		    ovm_uint_new (gc_ctx, (unsigned int)c));
	  WITH_SIZE(8,other_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_HDR_TIME, val = ovm_timeval_new (gc_ctx));
	      TIMEVAL(val).tv_sec = INT32_FROM_CHARS(stream);
	      TIMEVAL(val).tv_usec = INT32_FROM_CHARS(stream+4) * 1000;
	    }
	  WITH_SIZE(2,other_wrap)
	    {
	      size = INT16_FROM_CHARS(stream);
	    }
	  WITH_SIZE(size,other_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_HDR_FILE, val = ovm_vstr_new (gc_ctx, delegate));
	      VSTR(val) = (char *)stream;
	      VSTRLEN(val) = size;
	      /* XXX I don't know whether the file name is meant to be NUL-terminated.
		 If it is not, the following lines are useless.
		 It it is, one should write size-1 instead of size above and remove
		 the following lines */
	      if (size>0 && stream[size-1]=='\0')
		VSTRLEN(val)--;
	    }
	other_wrap:
	  F_WRAP(HDR);
	  GC_END(gc_ctx);
	  break;
	case AUT_TRAILER: /* normally at the very end of an audit record */
	  /* magic (2 bytes, ==0xb105?), size (4 bytes, repeats size given in header) */
	  stream += 6;
	  break;
	case AUT_ARG32:
	case AUT_ARG64:
	  /* argument number (1 byte, in [1, 128]), argument value (4 bytes/8 bytes),
	     text length (2 bytes), text (length bytes, including final NUL byte);
	     I assume this text is the name of the argument.
	  */
	  WITH_SIZE(1,arg_end)
	    num = (int)(unsigned int)stream[0];
	  if (num<=0 || num>OPENBSM_MAX_ARGS)
	    goto arg_end;
	  GC_START(gc_ctx, 2);
	  if (c==AUT_ARG64)
	    {
	      WITH_SIZE(8,arg_wrap)
		{
		  GC_UPDATE(gc_ctx, 0, ovm_uint_new(gc_ctx, INT64_FROM_CHARS(stream)));
		}
	    }
	  else
	    {
	      WITH_SIZE(4,arg_wrap)
		{
		  GC_UPDATE(gc_ctx, 0, ovm_uint_new(gc_ctx, INT32_FROM_CHARS(stream)));
		}
	    }
	  WITH_SIZE(2,arg_wrap)
	    size = INT16_FROM_CHARS(stream);
	  if (size==0)
	    goto arg_wrap;
	  WITH_SIZE(size,arg_wrap)
	    {
	      GC_UPDATE(gc_ctx, 1, val = ovm_vstr_new (gc_ctx, delegate));
	      VSTR(val) = (char *)stream;
	      VSTRLEN(val) = size-1; /* remove final NUL byte */
	    }
	arg_wrap:
	  add_fields_to_event_stride(ctx, mod, evtp, (ovm_var_t **)GC_DATA(),
				     F_OPENBSM_ARG_START+2*num-2,
				     F_OPENBSM_ARG_START+2*num-1);
	  GC_END(gc_ctx);
	arg_end:
	  break;
	case AUT_DATA:
	  /* how-to-print (1 byte), basic-unit (1 byte), unit-count (1 byte),
	     data (unit-count groups of datasize bytes, where datasize==1 if
	     basic-unit==0, 2 if basic-unit==1, 4 if basic-unit==2,
	     8 if basic-unit==3).
	     how-to-print can be 0 (binary), 1 (octal), 2 (decimal), 3 (hex), 4 (string).
	     Although that might be a bit costly, we use all that information to
	     print data according to specification.  Orchids can then extract
	     whatever it requires by regex matching, and conversions from strings
	     to numbers, say.
!!! int_from_string() should also recognize octal and hex!
	  */
	  WITH_SIZE(3,aut_end)
	    {
	      howtoprint = stream[0];
	      bu = (size_t)(unsigned char)stream[1];
	      uc = (size_t)(unsigned char)stream[2];
	    }
	  if (bu>3) goto aut_end;
	  width = 1L << bu;
	  switch (howtoprint)
	    {
	    case 1: format = "0%o"; maxlen = 1+3*width; space = 1; break;
	    case 2: format = "%u"; maxlen = 3*width; space = 1; break;
	    case 3: format = "0x%x"; maxlen = 2*width; space = 1; break;
	    case 0:
	    case 4:
	    default: format = "%c"; maxlen = 1; space = 0; break;
	    }
	  size = uc * width;
	  WITH_SIZE(size,aut_end)
	    {
	      GC_START(gc_ctx, F_OPENBSM_DATA_SIZE);
	      GC_UPDATE(gc_ctx, F_OPENBSM_DATA_DATA,
			val = ovm_str_new (gc_ctx, uc * maxlen + (space?(uc-1):0)));
	      for (i=0, s=STR(val); i<uc; i++)
		{
		  if (i!=0 && space)
		    *s++ = ' ';
		  switch (bu)
		    {
		    case 0: s += sprintf(s, format, stream+i); break;
		    case 1: s += sprintf(s, format, (uint16_t)INT16_FROM_CHARS(stream+2*i)); break;
		    case 2: s += sprintf(s, format, (uint32_t)INT32_FROM_CHARS(stream+4*i)); break;
		    case 3: s += sprintf(s, format, (uint64_t)INT64_FROM_CHARS(stream+8*i)); break;
		    default: break; /* Cannot happen */
		    }
		}
	      STRLEN(val) = s - STR(val);
	      F_WRAP(DATA);
	      GC_END(gc_ctx);
	    }
	aut_end:
	  break;
	case AUT_ATTR32:
	case AUT_ATTR64:
	  /* file access mode (4 bytes), owner uid (4 bytes), owner gid (4 bytes),
	     file system id (4 bytes), node id (8 bytes), device (4 bytes)
	  */
	  GC_START(gc_ctx, F_OPENBSM_ATTR_SIZE);
	  WITH_SIZE(4,attr_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_ATTR_FILE_ACCESS_MODE,
			ovm_uint_new (gc_ctx, INT32_FROM_CHARS(stream)));
	    }
	  WITH_SIZE(4,attr_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_ATTR_OWNER_UID,
			ovm_uint_new (gc_ctx, INT32_FROM_CHARS(stream)));
	    }
	  WITH_SIZE(4,attr_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_ATTR_OWNER_GID,
			ovm_uint_new (gc_ctx, INT32_FROM_CHARS(stream)));
	    }
	  WITH_SIZE(4,attr_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_ATTR_FSID,
			ovm_uint_new (gc_ctx, INT32_FROM_CHARS(stream)));
	    }
	  WITH_SIZE(8,attr_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_ATTR_NID,
			ovm_uint_new (gc_ctx, INT64_FROM_CHARS(stream)));
	    }
	  if (c==AUT_ATTR64)
	    {
	      WITH_SIZE(8,attr_wrap)
		{
		  GC_UPDATE(gc_ctx, F_OPENBSM_ATTR_DEV,
			    ovm_uint_new (gc_ctx, INT64_FROM_CHARS(stream)));
		}
	    }
	  else
	    {
	      WITH_SIZE(4,attr_wrap)
		{
		  GC_UPDATE(gc_ctx, F_OPENBSM_ATTR_DEV,
			    ovm_uint_new (gc_ctx, INT32_FROM_CHARS(stream)));
		}
	    }
	attr_wrap:
	  F_WRAP(ATTR);
	  GC_END(gc_ctx);
	  break;
	case AUT_EXIT:
	  /* status (4 bytes), return value (4 bytes) */
	  GC_START(gc_ctx, F_OPENBSM_EXIT_SIZE);
	  WITH_SIZE(4,exit_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_EXIT_STATUS,
			ovm_uint_new (gc_ctx, INT32_FROM_CHARS(stream)));
	    }
	  WITH_SIZE(4,exit_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_EXIT_RETVAL,
			ovm_uint_new (gc_ctx, INT32_FROM_CHARS(stream)));
	    }
	exit_wrap:
	  F_WRAP(ATTR);
	  GC_END(gc_ctx);
	  break;
	case AUT_EXEC_ARGS:
	  /* count (4 bytes), 'count' NUL-terminated strings,
	   which we shall put into fields .openbsm.execarg1, .openbsm.execarg2, etc.
	   (not really an ideal situation, but Orchids does not have arrays or
	   means to explore arrays)
	  */
	  WITH_SIZE(4,execarg_end)
	    size = INT32_FROM_CHARS(stream);
	  if (size>OPENBSM_MAX_ARGS)
	    size = OPENBSM_MAX_ARGS;
	  GC_START(gc_ctx, OPENBSM_MAX_ARGS+1); /* really only size+1 */
	  for (i=0; i<size; )
	    {
	      for (s=(char *)stream; s<(char *)stream_end && *s!='\0'; s++)
		;
              width = s-(char *)stream;
	      GC_UPDATE(gc_ctx, i+1, val = ovm_str_new (gc_ctx, width));
	      memcpy (STR(val), stream, width);
	      STRLEN(val) = width;
	      i++;
	      if (s>=(char *)stream_end)
		break;
	      stream = (unsigned char *)s+1;
	    }
	  GC_UPDATE(gc_ctx, F_OPENBSM_EXECARG_NUM, ovm_uint_new (gc_ctx, i));
	  add_fields_to_event_stride(ctx, mod, evtp, (ovm_var_t **)GC_DATA(),
				     F_OPENBSM_EXECARG_START,
				     F_OPENBSM_EXECARG_START+size+1);
	  GC_END(gc_ctx);
	execarg_end:
	  break;
	case AUT_EXEC_ENV:
	  /* count (4 bytes), 'count' NUL-terminated strings,
	   which we shall put into fields .openbsm.execenv1, .openbsm.execenv2, etc.
	   (not really an ideal situation, but Orchids does not have arrays or
	   means to explore arrays)
	  */
	  WITH_SIZE(4,execenv_end)
	    size = INT32_FROM_CHARS(stream);
	  if (size>OPENBSM_MAX_ENV)
	    size = OPENBSM_MAX_ENV;
	  GC_START(gc_ctx, OPENBSM_MAX_ENV+1); /* really size+1 */
	  for (i=0; i<size;)
	    {
	      for (s=(char *)stream; s<(char *)stream_end && *s!='\0'; s++);
	      width = s-(char *)stream;
	      GC_UPDATE(gc_ctx, i+1, val = ovm_str_new (gc_ctx, width));
	      memcpy (STR(val), stream, width);
	      STRLEN(val) = width;
	      i++;
	      if (s>=(char *)stream_end)
		break;
	      stream = (unsigned char *)s+1;
	    }
	  GC_UPDATE(gc_ctx, F_OPENBSM_EXECENV_NUM, ovm_uint_new (gc_ctx, i));
	  add_fields_to_event_stride(ctx, mod, evtp, (ovm_var_t **)GC_DATA(),
				     F_OPENBSM_EXECARG_START,
				     F_OPENBSM_EXECARG_START+size+1);
	  GC_END(gc_ctx);
	execenv_end:
	  break;
	case AUT_NEWGROUPS:
	  /* number of groups (2 bytes), group list (number of groups * 4 bytes) */
	  WITH_SIZE(2,newgroups_end)
	    size = INT16_FROM_CHARS(stream);
	  GC_START(gc_ctx, AUT_NEWGROUPS+1); /* really size+1 */
	  for (i=0; i<size; i++)
	    {
	      WITH_SIZE(4, newgroups_wrap)
		{
		  GC_UPDATE(gc_ctx, i+1, ovm_uint_new(gc_ctx, INT32_FROM_CHARS(stream)));
		}
	    }
	newgroups_wrap:
	  GC_UPDATE(gc_ctx, F_OPENBSM_NEWGROUPS_NUM, ovm_uint_new (gc_ctx, i));
	  add_fields_to_event_stride(ctx, mod, evtp, (ovm_var_t **)GC_DATA(),
				     F_OPENBSM_NEWGROUPS_START,
				     F_OPENBSM_NEWGROUPS_START+size+1);
	  GC_END(gc_ctx);
	newgroups_end:
	  break;
	case AUT_IN_ADDR:
	  /* IPV4 address (4 bytes) */
	  WITH_SIZE(4,inaddr_end)
	    {
	      GC_START(gc_ctx, F_OPENBSM_INADDR_SIZE);
	      GC_UPDATE(gc_ctx, F_OPENBSM_INADDR_ADDR, val = ovm_ipv4_new (gc_ctx));
	      IPV4(val).s_addr = INT32_FROM_CHARS(stream);
	      F_WRAP(INADDR);
	      GC_END(gc_ctx);
	    }
	inaddr_end:
	  break;
	case AUT_IN_ADDR_EX:
	  /* ad_type (4 bytes: 4 for IPv4, 16 for IPv6), address (ad_type bytes);
	     I will always encode them as IPv6 addresses */
	  WITH_SIZE(4,inaddr6_end)
	    ad_type = INT32_FROM_CHARS(stream);
	  GC_START(gc_ctx, F_OPENBSM_INADDR6_SIZE);
	  val = ovm_ipv6_new(gc_ctx);
	  switch (ad_type)
	    {
	    case 4: /* IPv4 */
	      /* convert IPv4 address to IPv6: */
	      WITH_SIZE(4,inaddr6_wrap)
		{
		  memcpy (IPV6(val).s6_addr+12, stream, 4);
		  IPV6(val).s6_addr[11] = 0xff;
		  IPV6(val).s6_addr[10] = 0xff;
		  memset(IPV6(val).s6_addr, 0, 10);
		}
	      break;
	    case 16: /* IPv6 */
	      WITH_SIZE(16,inaddr6_wrap)
		memcpy (IPV6(val).s6_addr, stream, 16);
	      break;
	    default: goto inaddr6_wrap;
	    }
	  GC_UPDATE(gc_ctx, F_OPENBSM_INADDR6_ADDR, val);
	inaddr6_wrap:
	  F_WRAP(INADDR6);
	  GC_END(gc_ctx);
	inaddr6_end:
	  break;
	case AUT_IP:
	  /* version (1 byte), tos (1 byte), length (2 bytes), id (2 bytes),
	     offset (2 bytes), ttl (1 byte), protocol (1 byte), checksum (2 bytes),
	     source ip (4 bytes), dest ip (4 bytes) */
	  GC_START(gc_ctx, F_OPENBSM_IP_SIZE);
	  WITH_SIZE(1,ip_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_IP_VERSION,
			ovm_uint_new (gc_ctx, (unsigned int)stream[0]));
	    }
	  WITH_SIZE(1,ip_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_IP_TOS,
			ovm_uint_new (gc_ctx, (unsigned int)stream[0]));
	    }
	  WITH_SIZE(2,ip_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_IP_LEN,
			ovm_uint_new (gc_ctx, INT16_FROM_CHARS(stream)));
	    }
	  WITH_SIZE(2,ip_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_IP_ID,
			ovm_uint_new (gc_ctx, INT16_FROM_CHARS(stream)));
	    }
	  WITH_SIZE(2,ip_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_IP_OFFSET,
			ovm_uint_new (gc_ctx, INT16_FROM_CHARS(stream)));
	    }
	  WITH_SIZE(1,ip_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_IP_TTL,
			ovm_uint_new (gc_ctx, (unsigned int)stream[0]));
	    }
	  WITH_SIZE(1,ip_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_IP_PROTOCOL,
			ovm_uint_new (gc_ctx, (unsigned int)stream[0]));
	    }
	  WITH_SIZE(2,ip_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_IP_CHECKSUM,
			ovm_uint_new (gc_ctx, INT16_FROM_CHARS(stream)));
	    }
	  WITH_SIZE(4,ip_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_IP_SOURCE, val = ovm_ipv4_new (gc_ctx));
	      IPV4(val).s_addr = INT32_FROM_CHARS(stream);
	    }
	  WITH_SIZE(4,ip_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_IP_DEST, val = ovm_ipv4_new (gc_ctx));
	      IPV4(val).s_addr = INT32_FROM_CHARS(stream);
	    }
	ip_wrap:
	  F_WRAP(IP);
	  GC_END(gc_ctx);
	  break;
	case AUT_IPC:
	  /* object type (1 byte), object identifier (4 bytes) */
	  GC_START(gc_ctx, F_OPENBSM_IPC_SIZE);
	  WITH_SIZE(1,ipc_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_IPC_TYPE,
			ovm_uint_new (gc_ctx, (unsigned int)stream[0]));
	    }
	  WITH_SIZE(4,ipc_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_IPC_ID,
			ovm_uint_new (gc_ctx, INT32_FROM_CHARS(stream)));
	    }
	ipc_wrap:
	  F_WRAP(IPC);
	  GC_END(gc_ctx);
	  break;
	case AUT_IPC_PERM:
	  /* owner user id (4 bytes), owner group id (4 bytes),
	     creator user id (4 bytes), creater group id (4 bytes),
	     access mode (4 bytes), slot sequence number (4 bytes),
	     key (4 bytes) */
	  GC_START(gc_ctx, F_OPENBSM_IPCPERM_SIZE);
	  WITH_SIZE(4,ipcperm_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_IPCPERM_UID,
			ovm_uint_new (gc_ctx, INT32_FROM_CHARS(stream)));
	    }
	  WITH_SIZE(4,ipcperm_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_IPCPERM_GID,
			ovm_uint_new (gc_ctx, INT32_FROM_CHARS(stream)));
	    }
	  WITH_SIZE(4,ipcperm_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_IPCPERM_PUID,
			ovm_uint_new (gc_ctx, INT32_FROM_CHARS(stream)));
	    }
	  WITH_SIZE(4,ipcperm_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_IPCPERM_PGID,
			ovm_uint_new (gc_ctx, INT32_FROM_CHARS(stream)));
	    }
	  WITH_SIZE(4,ipcperm_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_IPCPERM_MODE,
			ovm_uint_new (gc_ctx, INT32_FROM_CHARS(stream)));
	    }
	  WITH_SIZE(4,ipcperm_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_IPCPERM_SEQ,
			ovm_uint_new (gc_ctx, INT32_FROM_CHARS(stream)));
	    }
	  WITH_SIZE(4,ipcperm_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_IPCPERM_KEY,
			ovm_uint_new (gc_ctx, INT32_FROM_CHARS(stream)));
	    }
	ipcperm_wrap:
	  F_WRAP(IPCPERM);
	  GC_END(gc_ctx);
	  break;
	case AUT_IPORT:
	  /* IP port (2 bytes) */
	  GC_START(gc_ctx, F_OPENBSM_IPORT_SIZE);
	  WITH_SIZE(2,iport_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_IPORT_ADDR,
			ovm_uint_new (gc_ctx, INT16_FROM_CHARS(stream)));
	    }
	iport_wrap:
	  F_WRAP(IPORT);
	  GC_END(gc_ctx);
	  break;
	case AUT_OPAQUE:
	  /* size (2 bytes), data (size bytes) */
	  WITH_SIZE(2,opaque_end)
	    size = INT16_FROM_CHARS(stream);
	  WITH_SIZE(size,opaque_end)
	    {
	      GC_START(gc_ctx, F_OPENBSM_OPAQUE_SIZE);
	      GC_UPDATE(gc_ctx, F_OPENBSM_OPAQUE_DATA,
			val = ovm_vbstr_new (gc_ctx, delegate));
	      VBSTR(val) = stream;
	      VBSTRLEN(val) = size;
	      F_WRAP(OPAQUE);
	      GC_END(gc_ctx);
	    }
	opaque_end:
	  break;
	case AUT_PATH:
	  /* size (2 bytes), path (size bytes, including final NUL byte) */
	  WITH_SIZE(2,path_end)
	    size = INT16_FROM_CHARS(stream);
	  if (size==0)
	    goto path_end;
	  WITH_SIZE(size,path_end)
	    {
	      GC_START(gc_ctx, F_OPENBSM_PATH_SIZE);
	      GC_UPDATE(gc_ctx, F_OPENBSM_PATH_DATA,
			val = ovm_vstr_new (gc_ctx, delegate));
	      VSTR(val) = (char *)stream;
	      VSTRLEN(val) = size-1; /* remove final NUL byte */
	      F_WRAP(PATH);
	      GC_END(gc_ctx);
	    }
	path_end:
	  break;
	case AUT_PROCESS32:
	case AUT_PROCESS64:
	case AUT_PROCESS32_EX:
	case AUT_PROCESS64_EX:
	  /* audit id (4 bytes),
	     euid (4 bytes), egid (4 bytes), ruid (4 bytes), rgid (4 bytes),
	     pid (4 bytes), session id (4 bytes),
	     port id (4 or 8 bytes), machine address (4 bytes in the non *_EX cases;
	     or ad_type [4 bytes] follows by ad_type bytes of IPv4/v6 address in
	     the *_EX cases)
	  */
	  GC_START(gc_ctx,F_OPENBSM_PROCESS_SIZE);
	  WITH_SIZE(4,process_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_PROCESS_AUID,
			ovm_uint_new (gc_ctx, INT32_FROM_CHARS(stream)));
	    }
	  WITH_SIZE(4,process_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_PROCESS_EUID,
			ovm_uint_new (gc_ctx, INT32_FROM_CHARS(stream)));
	    }
	  WITH_SIZE(4,process_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_PROCESS_EGID,
			ovm_uint_new (gc_ctx, INT32_FROM_CHARS(stream)));
	    }
	  WITH_SIZE(4,process_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_PROCESS_RUID,
			ovm_uint_new (gc_ctx, INT32_FROM_CHARS(stream)));
	    }
	  WITH_SIZE(4,process_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_PROCESS_RGID,
			ovm_uint_new (gc_ctx, INT32_FROM_CHARS(stream)));
	    }
	  WITH_SIZE(4,process_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_PROCESS_PID,
			ovm_uint_new (gc_ctx, INT32_FROM_CHARS(stream)));
	    }
	  WITH_SIZE(4,process_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_PROCESS_SID,
			ovm_uint_new (gc_ctx, INT32_FROM_CHARS(stream)));
	    }
	  if (c==AUT_PROCESS64 || c==AUT_PROCESS64_EX)
	    {
	      WITH_SIZE(8,process_wrap)
		{
		  GC_UPDATE(gc_ctx, F_OPENBSM_PROCESS_TID_PORT,
			    ovm_uint_new (gc_ctx, INT64_FROM_CHARS(stream)));
		}
	    }
	  else
	    {
	      WITH_SIZE(4,process_wrap)
		{
		  GC_UPDATE(gc_ctx, F_OPENBSM_PROCESS_TID_PORT,
			    ovm_uint_new (gc_ctx, INT32_FROM_CHARS(stream)));
		}
	    }
	  if (c==AUT_PROCESS32 || c==AUT_PROCESS64)
	    ad_type = 4;
	  else
	    {
	      WITH_SIZE(4,process_wrap)
		ad_type = INT32_FROM_CHARS(stream);
	    }
	  val = ovm_ipv6_new (gc_ctx);
	  switch (ad_type)
	    {
	    case 4: /* IPv4 */
	      /* convert IPv4 address to IPv6: */
	      WITH_SIZE(4,process_wrap)
		{
		  memcpy (IPV6(val).s6_addr+12, stream, 4);
		  IPV6(val).s6_addr[11] = 0xff;
		  IPV6(val).s6_addr[10] = 0xff;
		  memset(IPV6(val).s6_addr, 0, 10);
		}
	      break;
	    case 16: /* IPv6 */
	      WITH_SIZE(16,process_wrap)
		memcpy (IPV6(val).s6_addr, stream, 16);
	      break;
	    default: goto process_wrap;
	    }
	  GC_UPDATE(gc_ctx, F_OPENBSM_PROCESS_TID_ADDR, val);
	process_wrap:
	  F_WRAP(PROCESS);
	  GC_END(gc_ctx);
	  break;
	case AUT_RETURN32:
	case AUT_RETURN64:
	  /* return status (1 byte), return value (4 bytes) */
	  GC_START(gc_ctx, F_OPENBSM_RETURN_SIZE);
	  WITH_SIZE(1,ret_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_RETURN_ERRNO,
			ovm_uint_new (gc_ctx, (unsigned int)stream[0]));
	    }
	  if (c==AUT_RETURN64)
	    {
	      WITH_SIZE(8,ret_wrap)
		{
		  GC_UPDATE(gc_ctx, F_OPENBSM_RETURN_VALUE,
			    ovm_uint_new (gc_ctx, INT64_FROM_CHARS(stream)));
		}
	    }
	  else
	    {
	      WITH_SIZE(4,ret_wrap)
		{
		  GC_UPDATE(gc_ctx, F_OPENBSM_RETURN_VALUE,
			    ovm_uint_new (gc_ctx, INT32_FROM_CHARS(stream)));
		}
	    }
	ret_wrap:
	  F_WRAP(RETURN);
	  GC_END(gc_ctx);
	  break;
	case AUT_SEQ:
	  /* sequence number (4 bytes) */
	  GC_START(gc_ctx, F_OPENBSM_SEQ_SIZE);
	  WITH_SIZE(4,seq_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_SEQ_NUM,
			ovm_uint_new (gc_ctx, INT32_FROM_CHARS(stream)));
	    }
	seq_wrap:
	  F_WRAP(SEQ);
	  GC_END(gc_ctx);
	  break;
	case AUT_SOCKINET32:
	case AUT_SOCKINET128:
	  /* socket family (2 bytes), local port (2 bytes), socket address (4 or 8 bytes) */
	  GC_START(gc_ctx, F_OPENBSM_SOCK_SIZE);
	  WITH_SIZE(2,sockinet_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_SOCK_FAMILY,
			ovm_uint_new (gc_ctx, INT16_FROM_CHARS(stream)));
	    }
	  WITH_SIZE(2,sockinet_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_SOCK_PORT,
			ovm_uint_new (gc_ctx, INT16_FROM_CHARS(stream)));
	    }
	  val = ovm_ipv6_new (gc_ctx);
	  if (c==AUT_SOCKINET128)
	    {
	      WITH_SIZE(16,sockinet_wrap)
		{
		  memcpy (IPV6(val).s6_addr, stream, 16);
		}
	    }
	  else
	    {
	      WITH_SIZE(4,sockinet_wrap)
		{
		  memcpy (IPV6(val).s6_addr+12, stream, 4);
		  IPV6(val).s6_addr[11] = 0xff;
		  IPV6(val).s6_addr[10] = 0xff;
		  memset(IPV6(val).s6_addr, 0, 10);
		}
	    }
	  GC_UPDATE(gc_ctx, F_OPENBSM_SOCK_ADDR, val);
	sockinet_wrap:
	  F_WRAP(SOCK);
	  GC_END(gc_ctx);
	  break;
	case AUT_SOCKUNIX:
	  /* socket family (2 bytes), path (no length given, NUL-terminated, up to 104 chars) */
	  GC_START(gc_ctx, F_OPENBSM_SOCK_SIZE);
	  WITH_SIZE(2,sockunix_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_SOCK_FAMILY,
			ovm_uint_new (gc_ctx, INT16_FROM_CHARS(stream)));
	    }
	  GC_UPDATE(gc_ctx, F_OPENBSM_SOCK_PATH, val = ovm_vstr_new (gc_ctx, delegate));
	  VSTR(val) = (char *)stream;
	  t = (char *)stream+104;
	  if (t>(char *)stream_end)
	    t = (char *)stream_end;
	  s = memchr(stream, '\0', t-(char *)stream);
	  if (s==NULL)
	    {
	      stream = (unsigned char *)t;
	      VSTRLEN(val) = t-(char *)stream;
	    }
	  else
	    {
	      stream = (unsigned char *)s+1; /* skip final NUL byte */
	      VSTRLEN(val) = s-(char *)stream;
	    }
	sockunix_wrap:
	  F_WRAP(SOCK);
	  GC_END(gc_ctx);
	  break;
	case AUT_SOCKET:
	  /* socket type (2 bytes), local port (2 bytes), local address (4 bytes),
	     remote port (2 bytes), remote address (4 bytes) */
	  GC_START(gc_ctx, F_OPENBSM_SOCKET_SIZE);
	  WITH_SIZE(2,socket_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_SOCKET_TYPE,
			ovm_uint_new (gc_ctx, INT16_FROM_CHARS(stream)));
	    }
	  WITH_SIZE(2,socket_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_SOCKET_LPORT,
			ovm_uint_new (gc_ctx, INT16_FROM_CHARS(stream)));
	    }
	  WITH_SIZE(4,socket_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_SOCKET_LADDR, val = ovm_ipv6_new (gc_ctx));
	      memcpy (IPV6(val).s6_addr+12, stream, 4);
	      IPV6(val).s6_addr[11] = 0xff;
	      IPV6(val).s6_addr[10] = 0xff;
	      memset(IPV6(val).s6_addr, 0, 10);
	    }
	  WITH_SIZE(2,socket_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_SOCKET_RPORT,
			ovm_uint_new (gc_ctx, INT16_FROM_CHARS(stream)));
	    }
	  WITH_SIZE(4,socket_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_SOCKET_RADDR, val = ovm_ipv6_new (gc_ctx));
	      memcpy (IPV6(val).s6_addr+12, stream, 4);
	      IPV6(val).s6_addr[11] = 0xff;
	      IPV6(val).s6_addr[10] = 0xff;
	      memset(IPV6(val).s6_addr, 0, 10);
	    }
	socket_wrap:
	  F_WRAP(SOCKET);
	  GC_END(gc_ctx);
	  break;
	case AUT_SOCKET_EX:
	  /* socket domain (2 bytes),
	     socket type (2 bytes), ad_type (2 bytes),
	     local port (2 bytes), local address (ad_type bytes),
	     remote port (2 bytes), remote address (ad_type bytes) */
	  GC_START(gc_ctx, F_OPENBSM_SOCKET_SIZE);
	  WITH_SIZE(2,socketex_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_SOCKET_DOMAIN,
			ovm_uint_new (gc_ctx, INT16_FROM_CHARS(stream)));
	    }
	  WITH_SIZE(2,socketex_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_SOCKET_TYPE,
			ovm_uint_new (gc_ctx, INT16_FROM_CHARS(stream)));
	    }
	  WITH_SIZE(2,socketex_wrap)
	    ad_type = INT16_FROM_CHARS(stream);
	  WITH_SIZE(2,socketex_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_SOCKET_LPORT,
			ovm_uint_new (gc_ctx, INT16_FROM_CHARS(stream)));
	    }
	  val = ovm_ipv6_new (gc_ctx);
	  switch (ad_type)
	    {
	    case 4: /* IPv4 */
	      /* convert IPv4 address to IPv6: */
	      WITH_SIZE(4,socketex_wrap)
		{
		  memcpy (IPV6(val).s6_addr+12, stream, 4);
		  IPV6(val).s6_addr[11] = 0xff;
		  IPV6(val).s6_addr[10] = 0xff;
		  memset(IPV6(val).s6_addr, 0, 10);
		}
	      break;
	    case 16: /* IPv6 */
	      WITH_SIZE(16,socketex_wrap)
		memcpy (IPV6(val).s6_addr, stream, 16);
	      break;
	    default: goto socketex_wrap;
	    }
	  GC_UPDATE(gc_ctx, F_OPENBSM_SOCKET_LADDR, val);

	  WITH_SIZE(2,socketex_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_SOCKET_RPORT,
			ovm_uint_new (gc_ctx, INT16_FROM_CHARS(stream)));
	    }
	  val = ovm_ipv6_new (gc_ctx);
	  switch (ad_type)
	    {
	    case 4: /* IPv4 */
	      /* convert IPv4 address to IPv6: */
	      WITH_SIZE(4,socketex_wrap)
		{
		  memcpy (IPV6(val).s6_addr+12, stream, 4);
		  IPV6(val).s6_addr[11] = 0xff;
		  IPV6(val).s6_addr[10] = 0xff;
		  memset(IPV6(val).s6_addr, 0, 10);
		}
	      break;
	    case 16: /* IPv6 */
	      WITH_SIZE(16,socketex_wrap)
		memcpy (IPV6(val).s6_addr, stream, 16);
	      break;
	    default: goto socketex_wrap;
	    }
	  GC_UPDATE(gc_ctx, F_OPENBSM_SOCKET_RADDR, val);
	socketex_wrap:
	  F_WRAP(SOCKET);
	  GC_END(gc_ctx);
	  break;
	case AUT_SUBJECT32:
	case AUT_SUBJECT32_EX:
	case AUT_SUBJECT64:
	case AUT_SUBJECT64_EX:
	  /* audit id (4 bytes),
	     euid (4 bytes), egid (4 bytes), ruid (4 bytes), rgid (4 bytes),
	     pid (4 bytes), session id (4 bytes),
	     port id (4 or 8 bytes), machine address (4 bytes in the non *_EX cases;
	     or ad_type [4 bytes] follows by ad_type bytes of IPv4/v6 address in
	     the *_EX cases)
	  */
	  GC_START(gc_ctx,F_OPENBSM_SUBJECT_SIZE);
	  WITH_SIZE(4,subject_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_SUBJECT_AUID,
			ovm_uint_new (gc_ctx, INT32_FROM_CHARS(stream)));
	    }
	  WITH_SIZE(4,subject_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_SUBJECT_EUID,
			ovm_uint_new (gc_ctx, INT32_FROM_CHARS(stream)));
	    }
	  WITH_SIZE(4,subject_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_SUBJECT_EGID,
			ovm_uint_new (gc_ctx, INT32_FROM_CHARS(stream)));
	    }
	  WITH_SIZE(4,subject_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_SUBJECT_RUID,
			ovm_uint_new (gc_ctx, INT32_FROM_CHARS(stream)));
	    }
	  WITH_SIZE(4,subject_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_SUBJECT_RGID,
			ovm_uint_new (gc_ctx, INT32_FROM_CHARS(stream)));
	    }
	  WITH_SIZE(4,subject_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_SUBJECT_PID,
			ovm_uint_new (gc_ctx, INT32_FROM_CHARS(stream)));
	    }
	  WITH_SIZE(4,subject_wrap)
	    {
	      GC_UPDATE(gc_ctx, F_OPENBSM_SUBJECT_SID,
			ovm_uint_new (gc_ctx, INT32_FROM_CHARS(stream)));
	    }
	  if (c==AUT_SUBJECT64 || c==AUT_SUBJECT64_EX)
	    {
	      WITH_SIZE(8,subject_wrap)
		{
		  GC_UPDATE(gc_ctx, F_OPENBSM_SUBJECT_TID_PORT,
			    ovm_uint_new (gc_ctx, INT64_FROM_CHARS(stream)));
		}
	    }
	  else
	    {
	      WITH_SIZE(4,subject_wrap)
		{
		  GC_UPDATE(gc_ctx, F_OPENBSM_SUBJECT_TID_PORT,
			    ovm_uint_new (gc_ctx, INT32_FROM_CHARS(stream)));
		}
	    }
	  if (c==AUT_SUBJECT32 || c==AUT_SUBJECT64)
	    ad_type = 4;
	  else
	    {
	      WITH_SIZE(4,subject_wrap)
		ad_type = INT32_FROM_CHARS(stream);
	    }
	  val = ovm_ipv6_new (gc_ctx);
	  switch (ad_type)
	    {
	    case 4: /* IPv4 */
	      /* convert IPv4 address to IPv6: */
	      WITH_SIZE(4,subject_wrap)
		{
		  memcpy (IPV6(val).s6_addr+12, stream, 4);
		  IPV6(val).s6_addr[11] = 0xff;
		  IPV6(val).s6_addr[10] = 0xff;
		  memset(IPV6(val).s6_addr, 0, 10);
		}
	      break;
	    case 16: /* IPv6 */
	      WITH_SIZE(16,subject_wrap)
		memcpy (IPV6(val).s6_addr, stream, 16);
	      break;
	    default: goto subject_wrap;
	    }
	  GC_UPDATE(gc_ctx, F_OPENBSM_SUBJECT_TID_ADDR, val);
	subject_wrap:
	  F_WRAP(PROCESS);
	  GC_END(gc_ctx);
	  break;
	case AUT_TEXT:
	  /* size (2 bytes), data (size bytes, including final NUL byte) */
	  WITH_SIZE(2,text_end)
	    size = INT16_FROM_CHARS(stream);
	  if (size==0)
	    goto text_end;
	  WITH_SIZE(size,text_end)
	    {
	      GC_START(gc_ctx, F_OPENBSM_TEXT_SIZE);
	      GC_UPDATE(gc_ctx, F_OPENBSM_TEXT_TEXT,
			val = ovm_vstr_new (gc_ctx, delegate));
	      VSTR(val) = (char *)stream;
	      VSTRLEN(val) = size-1; /* remove final NUL byte */
	      F_WRAP(TEXT);
	      GC_END(gc_ctx);
	    }
	text_end:
	  break;
	case AUT_ZONENAME:
	  /* size (2 bytes), data (size bytes, including final NUL byte) */
	  WITH_SIZE(2,zone_end)
	    size = INT16_FROM_CHARS(stream);
	  if (size==0)
	    goto zone_end;
	  WITH_SIZE(size,zone_end)
	    {
	      GC_START(gc_ctx, F_OPENBSM_ZONENAME_SIZE);
	      GC_UPDATE(gc_ctx, F_OPENBSM_ZONENAME_TEXT,
			val = ovm_vstr_new (gc_ctx, delegate));
	      VSTR(val) = (char *)stream;
	      VSTRLEN(val) = size-1; /* remove final NUL byte */
	      F_WRAP(ZONENAME);
	      GC_END(gc_ctx);
	    }
	zone_end:
	  break;
	default:
	  /* invalid */
	  stream++;
	  break;
	}
    }
  post_event(ctx, mod, *evtp, dissector_level);
  GC_END(gc_ctx);
}

static int openbsm_dissect (orchids_t *ctx, mod_entry_t *mod,
			    event_t *event, void *data, int dissector_level)
{
  return blox_dissect (ctx, mod, event, data, dissector_level);
}

static void *openbsm_predissect(orchids_t *ctx, mod_entry_t *mod,
				char *parent_modname,
				char *cond_param_str,
				int cond_param_size)
{
  blox_hook_t *hook;

  hook = init_blox_hook (ctx, mod->config);
  return hook;
}

static void *openbsm_preconfig(orchids_t *ctx, mod_entry_t *mod)
{
  blox_config_t *bcfg;

  register_fields(ctx, mod, openbsm_fields, OPENBSM_FIELDS);
  bcfg = init_blox_config (ctx, mod, 1,
			   openbsm_compute_length,
			   openbsm_subdissect,
			   NULL);
  return bcfg;
}

input_module_t mod_openbsm = {
  MOD_MAGIC,
  ORCHIDS_VERSION,
  0,			    /* flags */
  "openbsm",
  "CeCILL2",
  NULL,
  NULL,
  openbsm_preconfig,
  NULL,
  NULL,
  openbsm_predissect,
  openbsm_dissect,
  &t_bstr		    /* type of fields it expects to dissect */
};

/*
** Copyright (c) 2014 by Jean GOUBAULT-LARRECQ, Laboratoire Spécification
** et Vérification (LSV), CNRS UMR 8643 & ENS Cachan.
**
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
