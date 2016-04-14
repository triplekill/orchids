#!/bin/sh
# usage: source distpatch.sh ${prefix} $(localstatedir) $(sysconfdir) $(libdir) $(ORCHIDS_RUNTIME_USER) input
vardir=`echo $2 | sed -e "s#^$1#:#"`
etcdir=`echo $3 | sed -e "s#^$1#:#"`
libdir=`echo $4 | sed -e "s#^$1#:#"`
sed -e "s#@@VARDIR@@#$vardir#g" \
    -e "s#@@ETCDIR@@#$etcdir#g" \
    -e "s#@@LIBDIR@@#$libdir#g" \
    -e "s#@@RUNUSER@@#$5#g" $6
