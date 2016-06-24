#!/bin/sh

TAKINDIR=$(dirname $0)
echo -e "Takin directory: ${TAKINDIR}"
LD_LIBRARY_PATH=./lib:${TAKINDIR}/lib:$LD_LIBRARY_PATH ${TAKINDIR}/bin/takin
