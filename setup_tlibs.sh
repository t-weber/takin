#!/bin/bash
#
# downloads tlibs and takin-data
# @author Tobias Weber <tobias.weber@tum.de>
# @license GPLv2
#


if [ $# -ge 1  -a  "$1" == "latest" ]; then
	TLIBS=tlibs-master
	TDATA=takin-data-master
else
	TLIBS=tlibs-0.8.3
	TLIBS=tlibs-master	# override for non-tagged version

	TDATA=takin-data-master
fi


REPO=http://forge.frm2.tum.de/cgit/cgit.cgi/frm2/mira/tlibs.git/snapshot
REPO_DATA=http://forge.frm2.tum.de/cgit/cgit.cgi/frm2/mira/takin-data.git/snapshot

TLIBS_DIR=tlibs
TDATA_DIR=takin-data

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

function dl_tdata
{
	echo -e "Downloading takin-data..."
	rm -f ${TDATA}${THEARCH}

	if ! wget ${REPO_DATA}/${TDATA}${THEARCH}
	then
		echo -e "Error: Cannot download takin-data.";
		exit -1;
	fi

	if ! tar -xjvf ${TDATA}${THEARCH}
	then
		echo -e "Error: Cannot extract takin-data.";
		exit -1;
	fi

	mv ${TDATA} ${TDATA_DIR}
	rm -f ${TDATA}${THEARCH}
}



echo -e "\n\n"

if [ -L ${TLIBS_DIR} ]
then
	echo -e "Error: Symbolic link \"${TLIBS_DIR}\" already exists. Skipping.";
else
	rm -rf ${TLIBS_DIR}
	dl_tlibs
fi

echo -e "\n\n"

if [ -L ${TDATA_DIR} ]
then
	echo -e "Error: Symbolic link \"${TDATA_DIR}\" already exists. Skipping.";
else
	rm -rf ${TDATA_DIR}
	dl_tdata
fi

echo -e "\n\n"
