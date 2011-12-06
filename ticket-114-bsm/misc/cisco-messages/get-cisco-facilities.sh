#! /bin/sh

sed 's/^%\([A-Z][A-Za-z0-9_]*\)-.*$/\1/' | sort -u
