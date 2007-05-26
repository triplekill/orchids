INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/src/util

BUILT_SOURCES = cisco.tab.c cisco.tab.h cisco.yy.c \
                netfilter.tab.c netfilter.tab.h netfilter.yy.c \
                snare.tab.c snare.tab.h snare.yy.c

EXTRA_DIST = cisco.l cisco.y \
             netfilter.l netfilter.y \
             snare.l snare.y

pkglib_LTLIBRARIES = \
  mod_autohtml.la \
  mod_cisco.la \
  mod_clocks.la \
  mod_consoles.la \
  mod_generic.la \
  mod_netfilter.la \
  mod_rawsnare.la \
  mod_remoteadm.la \
  mod_snare.la \
  mod_snmptrap.la \
  mod_sockunix.la \
  mod_sunbsm.la \
  mod_syslog.la \
  mod_textfile.la \
  mod_udp.la \
  mod_win32evt.la \
  mod_period.la

if ENABLE_HTMLOUTPUT
pkglib_LTLIBRARIES += mod_htmlstate.la
endif

if USE_SWIPROLOG
pkglib_LTLIBRARIES += mod_prolog.la
endif

if HAVE_LIBPCAP
pkglib_LTLIBRARIES += mod_pcap.la
pkglib_LTLIBRARIES += mod_wifi.la
endif

#  mod_stats.la
#  mod_snmp.la
#  mod_template.la
#  mod_test2.la
#  mod_test.la

mod_autohtml_la_SOURCES = mod_autohtml.c
mod_autohtml_la_LDFLAGS = -module -avoid-version

mod_cisco_la_SOURCES = mod_cisco.c cisco.tab.c cisco.tab.h cisco.yy.c
mod_cisco_la_LDFLAGS = -module -avoid-version

$(srcdir)/cisco.tab.c $(srcdir)/cisco.tab.h: $(srcdir)/cisco.y
	$(YACC) -p cisco -d -b cisco -o $(srcdir)/cisco.tab.c $(srcdir)/cisco.y

$(srcdir)/cisco.yy.c: $(srcdir)/cisco.l
	$(LEX) -f -Pcisco -o$(srcdir)/cisco.yy.c $(srcdir)/cisco.l

mod_clocks_la_SOURCES = mod_clocks.c
mod_clocks_la_LDFLAGS = -module -avoid-version

mod_consoles_la_SOURCES = mod_consoles.c
mod_consoles_la_LDFLAGS = -module -avoid-version

mod_generic_la_SOURCES = mod_generic.c
mod_generic_la_LDFLAGS = -module -avoid-version

mod_netfilter_la_SOURCES = mod_netfilter.c netfilter.tab.c netfilter.tab.h netfilter.yy.c
mod_netfilter_la_LDFLAGS = -module -avoid-version

$(srcdir)/netfilter.tab.c $(srcdir)/netfilter.tab.h: $(srcdir)/netfilter.y
	$(YACC) -p netfilter -d -b netfilter -o $(srcdir)/netfilter.tab.c $(srcdir)/netfilter.y

$(srcdir)/netfilter.yy.c: $(srcdir)/netfilter.l
	$(LEX) -f -Pnetfilter -o$(srcdir)/netfilter.yy.c $(srcdir)/netfilter.l

mod_period_la_SOURCES = mod_period.c
mod_period_la_LDFLAGS = -module -avoid-version

if HAVE_LIBPCAP
mod_pcap_la_SOURCES = mod_pcap.c
mod_pcap_la_LDFLAGS = -module -avoid-version
mod_pcap_la_LIBADD = $(PCAPLIB)
mod_pcap_la_CFLAGS = $(PCAPINC)

mod_wifi_la_SOURCES = mod_wifi.c mod_wifi.h compat.h
mod_wifi_la_LDFLAGS = -module -avoid-version
mod_wifi_la_LIBADD = $(PCAPLIB)
mod_wifi_la_CFLAGS = $(PCAPINC)
endif

if USE_SWIPROLOG
mod_prolog_la_SOURCES = mod_prolog.c
mod_prolog_la_LDFLAGS = -module -avoid-version $(SWIPL_LDFLAGS)
mod_prolog_la_LIBADD = $(SWIPL_LDADD)
mod_prolog_la_CFLAGS = $(SWIPL_CFLAGS)
mod_prolog_la_DEPENDENCIES = mod_prolog.prc

pkglib_DATA = mod_prolog.prc

# original line was:
# pl -f none -F none -g true \
# -t "consult([]),qsave_program('mod_prolog.prc',[toplevel=prolog,initfile=none])"
#

mod_prolog.prc:
	$(SWIPL) -f none -F none -g true \
-t "consult([]),qsave_program('mod_prolog.prc',[toplevel=prolog])"
endif

mod_rawsnare_la_SOURCES = mod_rawsnare.c rawsnare.h
mod_rawsnare_la_LDFLAGS = -module -avoid-version

mod_remoteadm_la_SOURCES = mod_remoteadm.c
mod_remoteadm_la_LDFLAGS = -module -avoid-version

mod_snare_la_SOURCES = mod_snare.c snare.tab.c snare.tab.h snare.yy.c
mod_snare_la_LDFLAGS = -module -avoid-version

$(srcdir)/snare.tab.c $(srcdir)/snare.tab.h: $(srcdir)/snare.y
	$(YACC) -p snare -d -b snare -o $(srcdir)/snare.tab.c $(srcdir)/snare.y

$(srcdir)/snare.yy.c: $(srcdir)/snare.l
	$(LEX) -f -Psnare -o$(srcdir)/snare.yy.c $(srcdir)/snare.l


# mod_snmp_la_SOURCES = mod_snmp.c
# mod_snmp_la_LDFLAGS = -module -avoid-version

if USE_NETSNMP
mod_snmptrap_la_SOURCES = mod_snmptrap.c
mod_snmptrap_la_CFLAGS = $(NETSNMP_CFLAGS)
mod_snmptrap_la_LDFLAGS = -module -avoid-version
mod_snmptrap_la_LIBADD = $(NETSNMP_LDFLAGS)
endif

mod_sockunix_la_SOURCES = mod_sockunix.c
mod_sockunix_la_LDFLAGS = -module -avoid-version

# mod_stats_la_SOURCES = mod_stats.c
# mod_stats_la_LDFLAGS = -module -avoid-version

mod_sunbsm_la_SOURCES = mod_sunbsm.c
mod_sunbsm_la_LDFLAGS = -module -avoid-version

mod_syslog_la_SOURCES = mod_syslog.c
mod_syslog_la_LDFLAGS = -module -avoid-version

# mod_template_la_SOURCES = mod_template.c
# mod_template_la_LDFLAGS = -module -avoid-version

# mod_test2_la_SOURCES = mod_test2.c
# mod_test2_la_LDFLAGS = -module -avoid-version

# mod_test_la_SOURCES = mod_test.c
# mod_test_la_LDFLAGS = -module -avoid-version

mod_textfile_la_SOURCES = mod_textfile.c
mod_textfile_la_LDFLAGS = -module -avoid-version

mod_udp_la_SOURCES = mod_udp.c
mod_udp_la_LDFLAGS = -module -avoid-version

mod_win32evt_la_SOURCES = mod_win32evt.c
mod_win32evt_la_LDFLAGS = -module -avoid-version

if ENABLE_HTMLOUTPUT
mod_htmlstate_la_SOURCES = mod_htmlstate.c html_output.c file_cache.c
mod_htmlstate_la_LDFLAGS = -module -avoid-version
endif


# End-of-file
