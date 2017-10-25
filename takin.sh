#!/bin/sh
#
# calling takin with local libs
# @author Tobias Weber <tobias.weber@tum.de>
# @license GPLv2
#

TAKINDIR=$(dirname $0)
echo -e "Takin directory: ${TAKINDIR}"

arg=$1
LD_LIBRARY_PATH=./lib:${TAKINDIR}/lib:/usr/local/lib:/usr/local/lib64:$LD_LIBRARY_PATH \
	${TAKINDIR}/bin/takin ${arg}
