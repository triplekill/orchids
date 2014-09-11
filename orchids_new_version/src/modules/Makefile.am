INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/src/util

BUILT_SOURCES = cisco.tab.c cisco.tab.h cisco.yy.c \
                netfilter.tab.c netfilter.tab.h netfilter.yy.c \
                snare.tab.c snare.tab.h snare.yy.c

EXTRA_DIST = cisco.l cisco.y \
             netfilter.l netfilter.y \
             snare.l snare.y

pkglib_LTLIBRARIES = \
  mod_cisco.la \
  mod_clocks.la \
  mod_consoles.la \
  mod_generic.la \
  mod_netfilter.la \
  mod_rawsnare.la \
  mod_remoteadm.la \
  mod_snare.la \
  mod_sockunix.la \
  mod_sunbsm.la \
  mod_syslog.la \
  mod_textfile.la \
  mod_udp.la \
  mod_win32evt.la \
  mod_period.la \
  mod_sharedvars.la \
  mod_timeout.la \
  mod_ruletrace.la\
  mod_auditd.la \
  mod_htmlstate.la \
  mod_metaevent.la \
  mod_mark.la



if USE_SWIPROLOG
pkglib_LTLIBRARIES += mod_prolog.la
pkglib_LTLIBRARIES += mod_prolog_history.la
endif

if HAVE_LIBPCAP
pkglib_LTLIBRARIES += mod_pcap.la
pkglib_LTLIBRARIES += mod_prism2.la
pkglib_LTLIBRARIES += mod_wifi.la
endif


if USE_PRELUDE
pkglib_LTLIBRARIES += mod_prelude.la
endif


#  mod_stats.la

mod_cisco_la_SOURCES =     \
  mod_cisco.c mod_cisco.h  \
  cisco.tab.c cisco.tab.h  \
  cisco.yy.c
mod_cisco_la_LDFLAGS = -module -avoid-version

$(srcdir)/cisco.tab.c $(srcdir)/cisco.tab.h: $(srcdir)/cisco.y
	$(YACC) -p cisco -d -b cisco -o $(srcdir)/cisco.tab.c $(srcdir)/cisco.y

$(srcdir)/cisco.yy.c: $(srcdir)/cisco.l
	$(LEX) -f -Pcisco -o$(srcdir)/cisco.yy.c $(srcdir)/cisco.l

mod_clocks_la_SOURCES = mod_clocks.c mod_clocks.h
mod_clocks_la_LDFLAGS = -module -avoid-version

mod_consoles_la_SOURCES = mod_consoles.c mod_consoles.h
mod_consoles_la_LDFLAGS = -module -avoid-version

mod_generic_la_SOURCES = mod_generic.c mod_generic.h
mod_generic_la_LDFLAGS = -module -avoid-version

mod_netfilter_la_SOURCES = mod_netfilter.c netfilter.tab.c netfilter.tab.h netfilter.yy.c
mod_netfilter_la_LDFLAGS = -module -avoid-version

$(srcdir)/netfilter.tab.c $(srcdir)/netfilter.tab.h: $(srcdir)/netfilter.y
	$(YACC) -p netfilter -d -b netfilter -o $(srcdir)/netfilter.tab.c $(srcdir)/netfilter.y

$(srcdir)/netfilter.yy.c: $(srcdir)/netfilter.l
	$(LEX) -f -Pnetfilter -o$(srcdir)/netfilter.yy.c $(srcdir)/netfilter.l

mod_period_la_SOURCES = mod_period.c mod_period.h period.c period.h
mod_period_la_LDFLAGS = -module -avoid-version
mod_period_la_LIBADD = -lm

if HAVE_LIBPCAP
mod_pcap_la_SOURCES = mod_pcap.c mod_pcap.h
mod_pcap_la_LDFLAGS = -module -avoid-version
mod_pcap_la_LIBADD = $(PCAPLIB)
mod_pcap_la_CFLAGS = $(PCAPINC)

mod_prism2_la_SOURCES = mod_prism2.c mod_prism2.h
mod_prism2_la_LDFLAGS = -module -avoid-version
mod_prism2_la_LIBADD = $(PCAPLIB)
mod_prism2_la_CFLAGS = $(PCAPINC)

mod_wifi_la_SOURCES = mod_wifi.c mod_wifi.h compat.h
mod_wifi_la_LDFLAGS = -module -avoid-version
mod_wifi_la_LIBADD = $(PCAPLIB)
mod_wifi_la_CFLAGS = $(PCAPINC)
endif

if USE_SWIPROLOG
mod_prolog_la_SOURCES = mod_prolog.c mod_prolog.h
mod_prolog_la_LDFLAGS = -module -avoid-version $(SWIPL_LDFLAGS)
mod_prolog_la_LIBADD = $(SWIPL_LDADD)
mod_prolog_la_CFLAGS = $(SWIPL_CFLAGS)
mod_prolog_la_DEPENDENCIES = mod_prolog.prc

mod_prolog_history_la_SOURCES = mod_prolog_history.c mod_prolog_history.h
mod_prolog_history_la_LDFLAGS = -module -avoid-version

pkglib_DATA = mod_prolog.prc

# original line was:
# pl -f none -F none -g true \
# -t "consult([]),qsave_program('mod_prolog.prc',[toplevel=prolog,initfile=none])"
#

mod_prolog.prc:
	$(SWIPL) -f none -F none -g true \
-t "consult([]),qsave_program('mod_prolog.prc',[toplevel=prolog])"


endif

mod_rawsnare_la_SOURCES = mod_rawsnare.c mod_rawsnare.h rawsnare.h
mod_rawsnare_la_LDFLAGS = -module -avoid-version

mod_remoteadm_la_SOURCES = mod_remoteadm.c mod_remoteadm.h
mod_remoteadm_la_LDFLAGS = -module -avoid-version

mod_snare_la_SOURCES = mod_snare.c mod_snare.h snare.tab.c snare.tab.h snare.yy.c
mod_snare_la_LDFLAGS = -module -avoid-version

$(srcdir)/snare.tab.c $(srcdir)/snare.tab.h: $(srcdir)/snare.y
	$(YACC) -p snare -d -b snare -o $(srcdir)/snare.tab.c $(srcdir)/snare.y

$(srcdir)/snare.yy.c: $(srcdir)/snare.l
	$(LEX) -f -Psnare -o$(srcdir)/snare.yy.c $(srcdir)/snare.l


mod_sockunix_la_SOURCES = mod_sockunix.c mod_sockunix.h
mod_sockunix_la_LDFLAGS = -module -avoid-version


mod_auditd_la_SOURCES = mod_auditd.c mod_auditd.h auditd_queue.h
mod_auditd_la_LDFLAGS = -module -avoid-version
mod_auditd_la_LIBADD = -lm

# mod_stats_la_SOURCES = mod_stats.c
# mod_stats_la_LDFLAGS = -module -avoid-version

mod_sunbsm_la_SOURCES = mod_sunbsm.c
mod_sunbsm_la_LDFLAGS = -module -avoid-version

mod_syslog_la_SOURCES = mod_syslog.c mod_syslog.h
mod_syslog_la_LDFLAGS = -module -avoid-version

mod_textfile_la_SOURCES = mod_textfile.c mod_textfile.h
mod_textfile_la_LDFLAGS = -module -avoid-version

mod_udp_la_SOURCES = mod_udp.c mod_udp.h
mod_udp_la_LDFLAGS = -module -avoid-version

mod_win32evt_la_SOURCES = mod_win32evt.c mod_win32evt.h
mod_win32evt_la_LDFLAGS = -module -avoid-version

mod_htmlstate_la_SOURCES = \
  mod_htmlstate.c mod_htmlstate.h \
  html_output.c html_output.h \
  file_cache.c file_cache.h file_cache_priv.h
mod_htmlstate_la_LDFLAGS = -module -avoid-version

mod_sharedvars_la_SOURCES = mod_sharedvars.c mod_sharedvars.h
mod_sharedvars_la_LDFLAGS = -module -avoid-version

mod_timeout_la_SOURCES = mod_timeout.c mod_timeout.h
mod_timeout_la_LDFLAGS = -module -avoid-version

mod_ruletrace_la_SOURCES = mod_ruletrace.c mod_ruletrace.h
mod_ruletrace_la_LDFLAGS = -module -avoid-version

if USE_PRELUDE
mod_prelude_la_SOURCES = mod_prelude.c mod_prelude.h
mod_prelude_la_CFLAGS = $(LIBPRELUDE_CFLAGS) $(LIBPRELUDEDB_CFLAGS)
mod_prelude_la_LDFLAGS = -module -avoid-version
mod_prelude_la_LIBADD = $(LIBPRELUDE_LDFLAGS) $(LIBPRELUDEDB_LDFLAGS)
endif

if HAVE_XML2

mod_iodef_la_SOURCES = mod_iodef.c mod_iodef.h
mod_iodef_la_CFLAGS = $(XML2_CFLAGS)
mod_iodef_la_LDFLAGS = -module -avoid-version
mod_iodef_la_LIBADD = $(XML2_LDFLAGS)

mod_idmef_la_SOURCES = mod_idmef.c mod_idmef.h
mod_idmef_la_CFLAGS = $(XML2_CFLAGS)
mod_idmef_la_LDFLAGS = -module -avoid-version
mod_idmef_la_LIBADD = $(XML2_LDFLAGS)

mod_xml_la_SOURCES = mod_xml.c mod_xml.h
mod_xml_la_CFLAGS = $(XML2_CFLAGS)
mod_xml_la_LDFLAGS = -module -avoid-version
mod_xml_la_LIBADD = $(XML2_LDFLAGS)

pkglib_LTLIBRARIES += mod_idmef.la mod_iodef.la mod_xml.la
endif

mod_metaevent_la_SOURCES = mod_metaevent.c mod_metaevent.h
mod_metaevent_la_LDFLAGS = -module -avoid-version

mod_mark_la_SOURCES = mod_mark.c mod_mark.h
mod_mark_la_LDFLAGS = -module -avoid-version

if USE_NETSNMP
mod_snmptrap_la_SOURCES = mod_snmptrap.c mod_snmptrap.h
mod_snmptrap_la_CFLAGS = $(NETSNMP_CFLAGS)
mod_snmptrap_la_LDFLAGS = -module -avoid-version
mod_snmptrap_la_LIBADD = $(NETSNMP_LDFLAGS)
pkglib_LTLIBRARIES += mod_snmptrap.la
endif

# End-of-file
