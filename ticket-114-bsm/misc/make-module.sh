#! /bin/sh

if [ "$1" = "" ] ; then
    echo "Usage $0 <new-module-name>"
    exit 1
fi

TR=tr
SED=sed
MOD_NAME=$1
UPPER_MOD_NAME=$(echo ${MOD_NAME} | ${TR} a-z A-Z)

echo "Creating the module ${MOD_NAME}..." > /dev/stderr

${SED} \
  -e "s/@@MOD_TEMPLATE@@/${MOD_NAME}/g" \
  -e "s/@@UPPER_MOD_TEMPLATE@@/${UPPER_MOD_NAME}/g;" \
  mod_template.c.in > mod_${MOD_NAME}.c

${SED} \
  -e "s/@@MOD_TEMPLATE@@/${MOD_NAME}/g" \
  -e "s/@@UPPER_MOD_TEMPLATE@@/${UPPER_MOD_NAME}/g;" \
  mod_template.h.in > mod_${MOD_NAME}.h

echo "Wrote mod_${MOD_NAME}.c and mod_${MOD_NAME}.h" > /dev/stderr

# End-of-file
