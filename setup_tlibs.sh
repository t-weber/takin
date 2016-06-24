#!/bin/bash

if [ $# -ge 1  -a  "$1" == "latest" ]; then
	TLIBS=tlibs-master
else
	#TLIBS=tlibs-0.5.8
	TLIBS=tlibs-master	# override for non-tagged version
fi


REPO=http://forge.frm2.tum.de/cgit/cgit.cgi/frm2/mira/tlibs.git/snapshot

TLIBS_DIR=tlibs
THEARCH=.tar.bz2

function dl_tlibs
{
	echo -e "Downloading tlibs..."
	rm -f ${TLIBS}${THEARCH}

	if ! wget ${REPO}/${TLIBS}${THEARCH}
	then
		echo -e "Error: Cannot download tlibs.";
		exit -1;
	fi

	if ! tar -xjvf ${TLIBS}${THEARCH}
	then
		echo -e "Error: Cannot extract tlibs.";
		exit -1;
	fi

	mv ${TLIBS} ${TLIBS_DIR}
	rm -f ${TLIBS}${THEARCH}
}


if [ -L ${TLIBS_DIR} ]
then
	echo -e "Error: Symbolic link \"${TLIBS_DIR}\" already exists. Aborting.";
	exit -1;
fi


rm -rf ${TLIBS_DIR}
dl_tlibs
