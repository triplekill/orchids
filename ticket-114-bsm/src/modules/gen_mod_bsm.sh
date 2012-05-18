#!/bin/sh
#
# @file gen_mod_bsm.sh
# Functions generating part of the source of the bsm module.
#
# @author Jean Goubault-Larrecq <goubault@lsv.ens-cachan.fr>
# @version 0.1
# @ingroup modules
#
#
# @date  Started on: Ven 18 mai 2012 22:59:23 CEST
BASE=$(dirname $0)
CC=$1
cd ${BASE}
${CC} -o gen_mod_bsm gen_mod_bsm.c
./gen_mod_bsm >defs_bsm.h

