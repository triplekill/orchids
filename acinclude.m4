dnl My m4 include


AC_DEFUN([AC_CHECK_GNUPLOT],
[
AC_ARG_WITH(gnuplot,
AS_HELP_STRING([--with-gnuplot], [use gnuplot (default is yes)]),
[
   if test "$withval" = yes ; then
      AC_PATH_PROG(GNUPLOT, gnuplot)
      if test "$GNUPLOT" != "" ; then
         AC_DEFINE([HAVE_GNUPLOT], 1, [Set to 1 if Gnuplot is present])
         AC_DEFINE_UNQUOTED([PATH_TO_GNUPLOT], "$GNUPLOT", [Path to GnuPlot])
      fi
      else if test "$withval" = no ; then
         AC_MSG_CHECKING([for gnuplot])
         AC_MSG_RESULT(no)
      else
         if test "$withval" != "" ; then
            AC_MSG_CHECKING([for gnuplot])
            GNUPLOT=$withval
            AC_DEFINE([HAVE_GNUPLOT], 1, [Set to 1 if Gnuplot is present])
            AC_DEFINE_UNQUOTED([PATH_TO_GNUPLOT], "$GNUPLOT", [Path to GnuPlot])
            AC_MSG_RESULT([$GNUPLOT])
            if test ! -f $GNUPLOT ; then
               AC_MSG_WARN([gnuplot binary "$GNUPLOT" doesn't exist]);
            fi
         fi
      fi
   fi
],
[
    dnl default action (if no --with-xxx)
    AC_PATH_PROG(GNUPLOT, gnuplot)
    if test "$GNUPLOT" != "" ; then
       AC_DEFINE([HAVE_GNUPLOT], 1, [Set to 1 if Gnuplot is present])
       AC_DEFINE_UNQUOTED([PATH_TO_GNUPLOT], "$GNUPLOT", [Path to GnuPlot])
    fi
])
])


AC_DEFUN([AC_CHECK_GRAPHVIZ_DOT],
[
AC_ARG_WITH(graphviz-dot,
AS_HELP_STRING([--with-graphviz-dot], [use GraphViz dot (default is yes)]),
[
   if test "$withval" = yes ; then
      AC_PATH_PROG(DOT, dot)
      if test "$DOT" != "" ; then
         AC_DEFINE([HAVE_DOT], 1, [Set to 1 if GraphViz dot is present])
         AC_DEFINE_UNQUOTED([PATH_TO_DOT], "$DOT", [Path to GraphViz dot])
      fi
      else if test "$withval" = no ; then
         AC_MSG_CHECKING([for dot])
         AC_MSG_RESULT(no)
      else
         if test "$withval" != "" ; then
            AC_MSG_CHECKING([for dot])
            DOT=$withval
            AC_DEFINE([HAVE_DOT], 1, [Set to 1 if GraphViz dot is present])
            AC_DEFINE_UNQUOTED([PATH_TO_DOT], "$DOT", [Path to GraphViz dot])
            AC_MSG_RESULT([$DOT])
            if test ! -f $DOT ; then
               AC_MSG_WARN([GraphViz dot binary "$DOT" doesn't exist]);
            fi
         fi
      fi
   fi
],
[
   dnl default action (if no --with-xxx)
   AC_PATH_PROG(DOT, dot)
   if test "$DOT" != "" ; then
      AC_DEFINE([HAVE_DOT], 1, [Set to 1 if GraphViz dot is present])
      AC_DEFINE_UNQUOTED([PATH_TO_DOT], "$DOT", [Path to GraphViz dot])
   fi
])
])



AC_DEFUN([AC_CHECK_EPSTOPDF],
[
AC_ARG_WITH(epstopdf,
AS_HELP_STRING([--with-epstopdf], [use epstopdf (default is yes)]),
[
   if test "$withval" = yes ; then
      AC_PATH_PROG(EPSTOPDF, epstopdf)
      if test "$GNUPLOT" != "" ; then
         AC_DEFINE([HAVE_EPSTOPDF], 1, [Set to 1 if epstopdf is present])
         AC_DEFINE_UNQUOTED([PATH_TO_EPSTOPDF], "$EPSTOPDF", [Path to epstopdf])
      fi
      else if test "$withval" = no ; then
         AC_MSG_CHECKING([for epstopdf])
         AC_MSG_RESULT(no)
      else
         if test "$withval" != "" ; then
            AC_MSG_CHECKING([for epstopdf])
            EPSTOPDF=$withval
            AC_DEFINE([HAVE_EPSTOPDF], 1, [Set to 1 if epstopdf is present])
            AC_DEFINE_UNQUOTED([PATH_TO_EPSTOPDF], "$EPSTOPDF", [Path to epstopdf])
            AC_MSG_RESULT([$EPSTOPDF])
            if test ! -f $EPSTOPDF ; then
               AC_MSG_WARN([epstopdf binary "$EPSTOPDF" doesn't exist]);
            fi
         fi
      fi
   fi
],
[
    dnl default action (if no --with-xxx)
    AC_PATH_PROG(EPSTOPDF, epstopdf)
    if test "$EPSTOPDF" != "" ; then
       AC_DEFINE([HAVE_EPSTOPDF], 1, [Set to 1 if epstopdf is present])
       AC_DEFINE_UNQUOTED([PATH_TO_EPSTOPDF], "$EPSTOPDF", [Path to epstopdf])
    fi
])
])


AC_DEFUN([AC_CHECK_CONVERT],
[
AC_ARG_WITH(convert,
AS_HELP_STRING([--with-convert], [use ImageMagick convert (default is yes)]),
[
   if test "$withval" = yes ; then
      AC_PATH_PROG(CONVERT, convert)
      if test "$CONVERT" != "" ; then
         AC_DEFINE([HAVE_CONVERT], 1, [Set to 1 if ImageMagick convert is present])
         AC_DEFINE_UNQUOTED([PATH_TO_CONVERT], "$CONVERT", [Path to ImageMagick convert])
      fi
      else if test "$withval" = no ; then
         AC_MSG_CHECKING([for convert])
         AC_MSG_RESULT(no)
      else
         if test "$withval" != "" ; then
            AC_MSG_CHECKING([for convert])
            CONVERT=$withval
            AC_DEFINE([HAVE_CONVERT], 1, [Set to 1 if ImageMagick convert is present])
            AC_DEFINE_UNQUOTED([PATH_TO_CONVERT], "$CONVERT", [Path to ImageMagick convert])
            AC_MSG_RESULT([$CONVERT])
            if test ! -f $CONVERT ; then
               AC_MSG_WARN([ImageMagick convert binary "$CONVERT" doesn't exist]);
            fi
         fi
      fi
   fi
],
[
    dnl default action (if no --with-xxx)
    AC_PATH_PROG(CONVERT, convert)
    if test "$CONVERT" != "" ; then
       AC_DEFINE([HAVE_CONVERT], 1, [Set to 1 if ImageMagick convert is present])
       AC_DEFINE_UNQUOTED([PATH_TO_CONVERT], "$CONVERT", [Path to ImageMagick convert])
    fi
])
])


AC_DEFUN([AC_CHECK_SWI_PROLOG],
[
AC_ARG_WITH(swiprolog,
AS_HELP_STRING([--with-swiprolog], [use SWI Prolog (default is yes)]),
[
   if test "$withval" = yes ; then
      AC_PATH_PROGS(SWIPL, [pl swipl prolog])
      AC_PATH_PROGS(SWIPLLD, [plld swipl-ld])
      if test "$SWIPL" != "" -a "$SWIPLLD" != ""; then
         AC_DEFINE([HAVE_SWIPROLOG], 1, [Set to 1 if SWI Prolog is present])
      fi
      AM_CONDITIONAL(USE_SWIPROLOG, true)
   else if test "$withval" = no ; then
      AC_MSG_CHECKING([for pl])
      AC_MSG_RESULT(no)
      AC_MSG_CHECKING([for plld])
      AC_MSG_RESULT(no)
      AM_CONDITIONAL(USE_SWIPROLOG, false)
   else
      if test "$withval" != "" ; then
         AC_MSG_CHECKING([for pl])
         SWIPL="$ac_cv_path_SWIPL"
	 dnl "$withval/bin/pl"
         AC_MSG_RESULT([$SWIPL])
         if test ! -f "$SWIPL" ; then
            AC_MSG_WARN([SWI Prolog binary "$SWIPL" doesn't exist]);
         fi

         AC_MSG_CHECKING([for plld])
         SWIPLLD="$ac_cv_path_SWIPLLD"
	 dnl "$withval/bin/plld"
         AC_MSG_RESULT([$SWIPLLD])
         if test ! -f "$SWIPLLD" ; then
            AC_MSG_WARN([SWI Prolog linker binary "$SWIPLLD" doesn't exist]);
         fi
         AC_DEFINE([HAVE_SWIPROLOG], 1, [Set to 1 if SWI Prolog is present])
         AM_CONDITIONAL(USE_SWIPROLOG, true)
      else
         AM_CONDITIONAL(USE_SWIPROLOG, false)
      fi
   fi
fi
],
[
   dnl default action (if no --with-xxx)
   AC_PATH_PROGS(SWIPL, [pl swipl prolog])
dnl   if test "$SWIPL" == "" ; then
dnl      AC_PATH_PROG(SWIPL, swipl)
dnl   fi
   AC_PATH_PROGS(SWIPLLD, [plld swipl-ld])
   if test "$SWIPL" != "" -a "$SWIPLLD" != ""; then
      AC_DEFINE([HAVE_SWIPROLOG], 1, [Set to 1 if SWI Prolog is present])
      eval `$SWIPL -dump-runtime-variables | sed 's/^CC/PLCC/'`
      SWIPL_LDFLAGS="$PLLDFLAGS"
      SWIPL_LDADD="-L$PLBASE/lib/$PLARCH $PLLIB $PLLIBS"
      SWIPL_CFLAGS="-D__SWI_PROLOG__ -D__SWI_EMBEDDED__ -I$PLBASE/include"
      if test "$PLTHREADS" == "yes" ; then
        SWIPL_CFLAGS="-D_REENTRANT $SWIPL_CFLAGS"
      fi
      AC_SUBST(SWIPL_LDFLAGS)
      AC_SUBST(SWIPL_LDADD)
      AC_SUBST(SWIPL_CFLAGS)
      AC_SUBST(SWIPL)
      AM_CONDITIONAL(USE_SWIPROLOG, true)
   else
      AM_CONDITIONAL(USE_SWIPROLOG, false)
   fi
])
])

dnl doxygen
AC_DEFUN([AC_CHECK_DOXYGEN],
[
AC_ARG_WITH(doxygen,
AS_HELP_STRING([--with-doxygen], [use doxygen (default is yes)]),
[
   if test "$withval" = yes ; then
      AC_PATH_PROG(DOXYGEN, doxygen)
      else if test "$withval" = no ; then
         AC_MSG_CHECKING([for doxygen])
         AC_MSG_RESULT(no)
      else
         if test "$withval" != "" ; then
            AC_MSG_CHECKING([for doxygen])
            CONVERT=$withval
            AC_MSG_RESULT([$CONVERT])
            if test ! -f "$DOXYGEN" ; then
               AC_MSG_WARN([doxygen binary "$DOXYGEN" doesn't exist]);
            fi
         fi
      fi
   fi
],
[
    dnl default action (if no --with-xxx)
    AC_PATH_PROG(DOXYGEN, doxygen)
])
])

AC_DEFUN([AC_WITH_RUNTIME_USER],
[
AC_ARG_WITH(runtime-user,
AS_HELP_STRING([--with-runtime-user], [user id to use for runtime (default is nobody)]),
[
  if test "$withval" != "" ; then
    ORCHIDS_RUNTIME_USER=$withval
    AC_SUBST(ORCHIDS_RUNTIME_USER)
    id $ORCHIDS_RUNTIME_USER > /dev/null
    RETVAL=$?
    if test $RETVAL != 0 ; then
      AC_MSG_WARN([user "$ORCHIDS_RUNTIME_USER" doesn't exist]);
    fi
  fi
],
[
  dnl default action (if no --with-xxx)
  ORCHIDS_RUNTIME_USER=nobody
  AC_SUBST(ORCHIDS_RUNTIME_USER)
])
])

AC_DEFUN([AC_WITH_LOG_DIR],
[
AC_ARG_WITH(log-dir,
AS_HELP_STRING([--with-log-dir], [sets the log directory (default is ${prefix}/var/orchids/log)]),
[
  if test "$withval" != "" ; then
    AC_DEFINE_UNQUOTED([ORCHIDS_LOG_DIR], "$withval", [Path to log directory])
  fi
],
[
  dnl default action (if no --with-xxx)
  AC_DEFINE_UNQUOTED([ORCHIDS_LOG_DIR], "$prefix/var/orchids/log", [Path to system config directory])
])
])


AC_DEFUN([AC_CHECK_LIBPCAP],
[
AC_MSG_CHECKING(for libpcap)
AC_ARG_WITH(libpcap, AS_HELP_STRING(--with-libpcap=DIR,use libpcap in DIR),
[ case "$withval" in
  no)
     AC_MSG_RESULT(no)
     AC_MSG_ERROR([libpcap not found.])
     ;;
  *)
     if test -f $withval/pcap.h; then
        PCAPINC="-I$withval"
        PCAPLIB="-L$withval -lpcap"
        AC_DEFINE([HAVE_LIBPCAP], 1, [Set to 1 if libpcap is present])
        AM_CONDITIONAL(HAVE_LIBPCAP, true)
        AC_SUBST(PCAPINC)
        AC_SUBST(PCAPLIB)
        AC_MSG_RESULT($withval)
     elif test -f $withval/include/pcap.h; then
        PCAPINC="-I$withval/include"
        PCAPLIB="-L$withval/lib -lpcap"
        AC_DEFINE([HAVE_LIBPCAP], 1, [Set to 1 if libpcap is present])
        AM_CONDITIONAL(HAVE_LIBPCAP, true)
        AC_SUBST(PCAPINC)
        AC_SUBST(PCAPLIB)
        AC_MSG_RESULT($withval)
     else
        AC_MSG_RESULT(no)
        AC_MSG_ERROR([pcap.h not found in $withval])
     fi
     ;;
  esac ],
[ if test -f /usr/include/pcap/pcap.h; then
     PCAPINC="-I/usr/include/pcap"
     PCAPLIB="-lpcap"
     AC_DEFINE([HAVE_LIBPCAP], 1, [Set to 1 if libpcap is present])
     AM_CONDITIONAL(HAVE_LIBPCAP, true)
     AC_SUBST(PCAPINC)
     AC_SUBST(PCAPLIB)
     AC_MSG_RESULT(yes)
  elif test -f /usr/include/pcap.h; then
     PCAPLIB="-lpcap"
     AC_DEFINE([HAVE_LIBPCAP], 1, [Set to 1 if libpcap is present])
     AM_CONDITIONAL(HAVE_LIBPCAP, true)
     AC_SUBST(PCAPINC)
     AC_SUBST(PCAPLIB)
     AC_MSG_RESULT(yes)
  elif test -f /usr/local/include/pcap.h; then
     PCAPINC="-I/usr/local/include"
     PCAPLIB="-lpcap"
     AC_DEFINE([HAVE_LIBPCAP], 1, [Set to 1 if libpcap is present])
     AM_CONDITIONAL(HAVE_LIBPCAP, true)
     AC_SUBST(PCAPINC)
     AC_SUBST(PCAPLIB)
     AC_MSG_RESULT(yes)
  else
     AM_CONDITIONAL(HAVE_LIBPCAP, false)
     AC_MSG_RESULT(no)
  fi ]
)
])


AC_DEFUN([MY_ARG_WITH],
[AC_ARG_WITH([$1],
             AS_HELP_STRING([--with-$1], [use $1 (default is $2)]),
                            ac_cv_use_$1=$withval, ac_cv_use_$1=no),
 AC_CACHE_CHECK(whether to use $1, ac_cv_use_$1, ac_cv_use_$1=$2)])
