#! /bin/sh

SOURCES='cisco-ios-122sb-sysmsg-urls.txt cisco-ios-122sr-sysmsg-urls.txt cisco-ios-122s-sysmsg-urls.txt cisco-ios-124-sysmsg-urls.txt'


for s in $SOURCES ; do
  cat $s | while read u ; do
    w3m -dump -cols 1000 $u
  done
done | grep -E "^ *%[A-Z][A-Z0-9_]+" | sed 's/^ *//;s/ : /: /' | sort -u
