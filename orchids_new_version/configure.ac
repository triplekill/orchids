dnl
dnl a tester:
dnl * prog binaire
dnl  - ok - HAVE_SWI_PROLOG = true si pl ET plld existent (pour automake)
dnl  - ok - HAVE_GRAPHVIZ_DOT = true si dot est la
dnl  - ok - HAVE_EPSTOPDF si epstopdf est la
dnl  - ok - HAVE_IMAGEMAGICK_CONVERT si convert est la
dnl  - ok - HAVE_DOXYGEN si doxygen est la (pour Automake)
dnl  - ok - HAVE_GNUPLOT si gnuplot est la
dnl
dnl * bibliotheque
dnl  - ok - HAVE_OPENSSL_CRYPTO = true si la libcrypto existe et link
dnl  - ok - HAVE_OPENSSL = true si la libssl ET la libcrypto existent
dnl  - ok - HAVE_NETSNMP = true (a determiner)
dnl  - ok - HAVE_LIBXML2 = true si la libxml est la
dnl  - ok - HAVE_DMALLOC si dmalloc est la (puis ENABLE_DMALLOC apres)
dnl  - ok - HAVE_LIBPCAP
dnl  - HAVE_LIBNIDS
dnl  - HAVE_LIBNET
dnl
dnl comportment des libs:
dnl *rien* active en cherchant dans les chemins par defaut.
dnl --with-lib comme par defaut
dnl --without-lib, --with-lib=no desactive la lib (comme si elle n'etait pas la)
dnl --with-lib=/path/to/lib
dnl


AC_INIT([Orchids],[1.2],[orchids-bug-report@lsv.ens-cachan.fr])
AC_CONFIG_HEADERS(config.h)
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE

dnl
dnl Libtool
dnl
AC_DISABLE_STATIC
AC_LIBTOOL_DLOPEN
AM_PROG_LIBTOOL
AC_SEARCH_LIBS(dlopen,c dl,,)
AC_LIB_LTDL
AC_CHECK_HEADERS(dlfcn.h)
AC_CHECK_HEADERS(ltdl.h)

dnl AM_MAINTAINER_MODE

dnl Checks for programs
AC_PROG_CC
AM_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_MAKE_SET



AC_C_BIGENDIAN

dnl Obsolete
dnl AC_ISC_POSIX

AC_PROG_LEX
if test "$LEX" != flex ; then
   LEX="$SHELL $missing_dir/missing flex"
   AC_SUBST(LEX_OUTPUT_ROOT, lex.yy)
   AC_SUBST(LEXLIB, '')
fi

AC_PROG_YACC
if test "$YACC" != "bison -y" ; then
   YACC="$SHELL $missing_dir/missing bison"
else
   AC_SUBST(YACC, bison)
fi

dnl AC_PATH_PROG(GNUPLOT, gnuplot)
dnl AM_CONDITIONAL(HAVE_GNUPLOT, test x$HAVE_GNUPLOT = xyes)
dnl AC_DEFINE([HAVE_GNUPLOT], 1, [Set to 1 if Gnuplot is present])
dnl AC_DEFINE_UNQUOTED([PATH_TO_GNUPLOT], "$GNUPLOT", [Path to GnuPlot])

dnl AC_PATH_PROG(DOT, dot)
dnl AM_CONDITIONAL(HAVE_DOT, test x$HAVE_DOT = xyes)
dnl AC_DEFINE([HAVE_DOT], 1, [Set to 1 if GraphViz is present])
dnl AC_DEFINE_UNQUOTED([PATH_TO_DOT], "$DOT", [Path to Dot])


dnl AC_ARG_ENABLE(debug,
dnl               [  --enable-debug    Turn on debugging],
dnl               [case "${enableval}" in
dnl                 yes) debug=true ;;
dnl                 no)  debug=false ;;
dnl                 *) AC_MSG_ERROR(bad value ${enableval} for --enable-debug) ;;
dnl                 esac], [debug=false]
dnl )
dnl AM_CONDITIONAL(ORCHIDS_DEBUG, test x$debug = xtrue)


AC_ARG_ENABLE(dmalloc,
AS_HELP_STRING([--enable-dmalloc], [Turn on dmalloc]),
              [case "${enableval}" in
                yes) dmalloc=true ;;
                no)  dmalloc=false ;;
                *) AC_MSG_ERROR(bad value ${enableval} for --enable-dmalloc) ;;
                esac], [dmalloc=false]
)
AM_CONDITIONAL(ORCHIDS_DMALLOC, test x$dmalloc = xtrue)
if test "$dmalloc" = "true" ; then
  AC_CHECK_HEADERS([dmalloc.h])
  AC_CHECK_LIB(dmalloc, malloc, [], [])
  if test "$ac_cv_header_dmalloc_h" = "yes" \
       -a "$ac_cv_lib_dmalloc_malloc" = "yes" ; then
     AC_DEFINE([HAVE_DMALLOC], 1, [Set to 1 if dmalloc is present])
  fi
fi

AC_ARG_ENABLE(actmon,
AS_HELP_STRING([--enable-actmon], [enable activity monitor (default is off)]),
[case "${enableval}" in
    yes) orchids_actmon=true ;;
    no)  orchids_actmon=false ;;
    *)   AC_MSG_ERROR(bad value ${enableval} for --enable-actmon) ;;
esac],
[orchids_actmon=false]
)
if test "$orchids_actmon" = "true" ; then
   AC_DEFINE([ENABLE_ACTMON], 1, [Set to 1 if ACTMON is requested])
fi

AC_ARG_ENABLE(demo,
AS_HELP_STRING([--enable-demo], [enable demo mode (default is off)]),
[case "${enableval}" in
    yes) orchids_demo=true ;;
    no)  orchids_demo=false ;;
    *)   AC_MSG_ERROR(bad value ${enableval} for --enable-demo) ;;
esac],
[orchids_demo=false]
)
if test "$orchids_demo" = "true" ; then
   AC_DEFINE([ORCHIDS_DEMO], 1, [Set to 1 if DEMO is requested])
fi

AC_ARG_ENABLE(preproc,
AS_HELP_STRING([--enable-preproc], [enable preprocessor (default is on)]),
[case "${enableval}" in
    yes) orchids_preproc=true ;;
    no)  orchids_preproc=false ;;
    *)   AC_MSG_ERROR(bad value ${enableval} for --enable-preproc) ;;
esac],
[orchids_preproc=true]
)
if test "$orchids_preproc" = "true" ; then
   AC_DEFINE([ENABLE_PREPROC], 1, [Set to 1 if PREPROC is requested])
fi

AC_ARG_ENABLE(debug,
AS_HELP_STRING([--enable-debug], [enable debugging (default is off)]),
[case "${enableval}" in
    yes) orchids_debug=true ;;
    no)  orchids_debug=false ;;
    *)   AC_MSG_ERROR(bad value ${enableval} for --enable-debug) ;;
esac],
[orchids_debug=false]
)
if test "$orchids_debug" = "true" ; then
   AC_DEFINE([ENABLE_DEBUG], 1, [Set to 1 if DEBUG is requested])
   AC_DEFINE([ORCHIDS_DEBUG], 1, [Set to 1 if DEBUG is requested])
   AC_DEFINE([ENABLE_DEBUGLOG], 1, [Set to 1 if DEBUG is requested])
   CFLAGS="$CFLAGS -g"
fi

AC_ARG_ENABLE(pcre,
AS_HELP_STRING([--enable-pcre], [enable pcre regex (default is off)]),
[case "${enableval}" in
    yes) orchids_pcre=true ;;
    no)  orchids_pcre=false ;;
    *)   AC_MSG_ERROR(bad value ${enableval} for --enable-pcre) ;;
esac],
[orchids_pcre=false]
)
if test "$orchids_pcre" = "true" ; then
   AC_DEFINE([ORCHIDS_PCRE], 1, [Set to 1 if PCRE is requested])
   CFLAGS="$CFLAGS -DPCREPOSIX"
   LDFLAGS="$LDFLAGS -lpcreposix -lpcre"
fi




AC_ARG_ENABLE(prelude,
AS_HELP_STRING([--enable-prelude], [enable prelude module (default is off)]),
[case "${enableval}" in
    yes) orchids_prelude=true ;;
    no)  orchids_prelude=false ;;
    *)   AC_MSG_ERROR(bad value ${enableval} for --enable-prelude) ;;
esac],
[orchids_prelude=false]
)
if test "$orchids_prelude" = "true" ; then
   AC_DEFINE([ORCHIDS_PRELUDE], 1, [Set to 1 if PRELUDE is requested])

   AC_PATH_PROG(LIBPRELUDE_CONFIG, libprelude-config)
   AC_CHECK_HEADERS([libprelude/prelude.h])
   AC_CHECK_HEADERS([libprelude/prelude-client.h])
   AC_CHECK_HEADERS([libprelude/idmef.h])

   AC_PATH_PROG(LIBPRELUDEDB_CONFIG, libpreludedb-config)
   AC_CHECK_HEADERS([libpreludedb/preludedb.h])

   if test "$LIBPRELUDE_CONFIG" != ""  \
      -a "$ac_cv_header_libprelude_prelude_h" = "yes"       \
      -a "$ac_cv_header_libprelude_prelude_client_h" = "yes" \
      -a "$ac_cv_header_libprelude_idmef_h" = "yes" ; then
      LIBPRELUDE_LDFLAGS=$($LIBPRELUDE_CONFIG --libs)
      LIBPRELUDE_CFLAGS=$($LIBPRELUDE_CONFIG --cflags)
      AM_CONDITIONAL(USE_PRELUDE, true)
   else
      AM_CONDITIONAL(USE_PRELUDE, false)
   fi
   AC_SUBST(LIBPRELUDE_CFLAGS)
   AC_SUBST(LIBPRELUDE_LDFLAGS)


   if test "$LIBPRELUDEDB_CONFIG" != ""  \
      -a "$ac_cv_header_libpreludedb_preludedb_h" = "yes" ; then
      LIBPRELUDEDB_LDFLAGS=$($LIBPRELUDEDB_CONFIG --libs)
      LIBPRELUDEDB_CFLAGS=$($LIBPRELUDEDB_CONFIG --cflags)
      AM_CONDITIONAL(USE_PRELUDEDB, true)
   else
      AM_CONDITIONAL(USE_PRELUDEDB, false)
   fi
   AC_SUBST(LIBPRELUDEDB_CFLAGS)
   AC_SUBST(LIBPRELUDEDB_LDFLAGS)
else
   AM_CONDITIONAL(USE_PRELUDE, false)
   AM_CONDITIONAL(USE_PRELUDEDB, false)
fi




AC_ARG_ENABLE(xml,
AS_HELP_STRING([--enable-xml], [enable xml,idmef,iodef modules (default is off)]),
[case "${enableval}" in
    yes) orchids_xml=true ;;
    no)  orchids_xml=false ;;
    *)   AC_MSG_ERROR(bad value ${enableval} for --enable-xml) ;;
esac],
[orchids_xml=false]
)
if test "$orchids_xml" = "true" ; then
   AC_PATH_PROG(XML2_CONFIG, xml2-config)
   if test "$XML2_CONFIG" != "" ; then
      XML2_LDFLAGS=$($XML2_CONFIG --libs)
      XML2_CFLAGS=$($XML2_CONFIG --cflags)
      AC_DEFINE([HAVE_XML2], 1, [Set to 1 if libxml2 is present])
      AM_CONDITIONAL(HAVE_XML2, true)
   else
      AM_CONDITIONAL(HAVE_XML2, false)
   fi
   AC_SUBST(XML2_CFLAGS)
   AC_SUBST(XML2_LDFLAGS)
else
   AM_CONDITIONAL(HAVE_XML2, false)
fi



AC_ARG_ENABLE(snmptrap,
AS_HELP_STRING([--enable-snmptrap], [enable snmptrap modules (default is off)]),
[case "${enableval}" in
    yes) orchids_snmptrap=true ;;
    no)  orchids_snmptrap=false ;;
    *)   AC_MSG_ERROR(bad value ${enableval} for --enable-snmptrap) ;;
esac],
[orchids_snmptrap=false]
)
if test "$orchids_snmptrap" = "true" ; then
   AC_CHECK_HEADERS([net-snmp/net-snmp-config.h])
   AC_PATH_PROG(NETSNMP_CONFIG, net-snmp-config)
   if test "$NETSNMP_CONFIG" != "" \
      -a "$ac_cv_header_net_snmp_net_snmp_config_h" = "yes" ; then
      NETSNMP_LDFLAGS=$($NETSNMP_CONFIG --libs)
      NETSNMP_CFLAGS=$($NETSNMP_CONFIG --cflags)
      AC_DEFINE([HAVE_NETSNMP], 1, [Set to 1 if libnetsnmp is present])
      AM_CONDITIONAL(USE_NETSNMP, true)
   else
      AM_CONDITIONAL(USE_NETSNMP, false)
   fi
   AC_SUBST(NETSNMP_CFLAGS)
   AC_SUBST(NETSNMP_LDFLAGS)
else
   AM_CONDITIONAL(USE_NETSNMP, false)
fi


dnl Activate Debug log library
AC_DEFINE([ENABLE_DEBUGLOG], 1, [Set to 1 if DEBUG is requested])

AC_CHECK_LIBPCAP
AC_CHECK_GNUPLOT
AC_CHECK_GRAPHVIZ_DOT
AC_CHECK_EPSTOPDF
AC_CHECK_CONVERT
AC_CHECK_SWI_PROLOG
AC_CHECK_DOXYGEN

AC_WITH_RUNTIME_USER

dnl Checks for libraries
dnl AC_HEADER_STDC
dnl AC_INCLUDES_DEFAULT

dnl test

dnl check for openssl libcrypto
dnl AC_CHECK_HEADERS([openssl/des.h openssl/md5.h openssl/sha.h openssl/rsa.h openssl/dsa.h])
dnl AC_CHECK_LIB(crypto, SSLeay_version, [], [])
dnl if test "$ac_cv_header_openssl_des_h" = "yes" \
dnl      -a "$ac_cv_header_openssl_md5_h" = "yes" \
dnl      -a "$ac_cv_header_openssl_sha_h" = "yes" \
dnl      -a "$ac_cv_header_openssl_rsa_h" = "yes" \
dnl      -a "$ac_cv_header_openssl_dsa_h" = "yes" \
dnl      -a "$ac_cv_lib_crypto_SSLeay_version" = "yes" ; then
dnl    AC_DEFINE([HAVE_OPENSSL_CRYPTO], 1, [Set to 1 if openssl libcrypto is present])
dnl fi

dnl check for openssl libssl
dnl AC_CHECK_HEADERS([openssl/ssl.h])
dnl AC_CHECK_LIB(ssl, SSL_library_init, [], [])
dnl if test "$ac_cv_header_openssl_ssl_h" = "yes" \
dnl      -a "$ac_cv_lib_ssl_SSL_library_init" = "yes"; then
dnl    AC_DEFINE([HAVE_OPENSSL], 1, [Set to 1 if openssl libssl is present])
dnl fi

dnl AC_CHECK_HEADERS([pcap.h])
dnl AC_CHECK_LIB(pcap, pcap_loop, [], [])
dnl if test "$ac_cv_header_pcap_h" = "yes" \
dnl      -a "$ac_cv_lib_pcap_pcap_loop" = "yes" ; then
dnl    AC_DEFINE([HAVE_PCAP], 1, [Set to 1 if libpcap is present])
dnl fi




AC_PATH_PROG(PCRE_CONFIG, pcre-config)
if test "$PCRE_CONFIG" != "" ; then
   PCRE_LDFLAGS=$($PCRE_CONFIG --libs)
   PCRE_CFLAGS=$($PCRE_CONFIG --cflags)
   AC_DEFINE([HAVE_PCRE], 1, [Set to 1 if pcre is present])
fi
AC_SUBST(PCRE_CFLAGS)
AC_SUBST(PCRE_LDFLAGS)


AC_CHECK_HEADERS(libaudit.h)

CFLAGS="$CFLAGS -Wall -DPKGDATADIR=\\\"$datadir\\\" -DSYSCONFDIR=\\\"$sysconfdir\\\" -DLOCALSTATEDIR=\\\"$localstatedir\\\" -DLIBDIR=\\\"$libdir\\\""

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/modules/Makefile
                 dist/Makefile
                 dist/conf.d/Makefile
                 dist/rules/Makefile
                 dist/htmlstate/Makefile
                 dist/iodef/Makefile
])
AC_OUTPUT
