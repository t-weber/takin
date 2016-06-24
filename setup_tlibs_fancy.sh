#!/bin/bash

TLIBS=tlibs-master.tar.bz2
REPO=http://forge.frm2.tum.de/cgit/cgit.cgi/frm2/mira/tlibs.git/snapshot
TLIBS_DIR=tlibs
DLG=$(which dialog 2>/dev/null)

if [ "$DLG" == "" ]; then DLG="/usr/bin/dialog"; fi

function dl_tlibs
{
	echo -e "Downloading tlibs..."
	rm -f ${TLIBS}

	if ! wget ${REPO}/${TLIBS}
	then
		echo -e "Error: Cannot download tlibs.";
		exit -1;
	fi

	if ! tar -xjvf ${TLIBS}
	then
		echo -e "Error: Cannot extract tlibs.";
		exit -1;
	fi

	mv tlibs-master ${TLIBS_DIR}
	rm -f ${TLIBS}
}

function link_tlibs
{
	echo -e "Linking tlibs..."
	ln -sf ../tlibs
}


if [ -L ${TLIBS_DIR} ]
then
	echo -e "Error: Symbolic link \"${TLIBS_DIR}\" already exists. Aborting.";
	exit -1;
fi


rm -rf ${TLIBS_DIR}


if [ ! -x $DLG ]
then
	dl_tlibs
else
	dlg_selected=$(${DLG} --stdout --title "tlibs" \
		--menu "Setup tlibs" 10 40 2 \
			1 "Download tlibs" \
			2 "Link to ../tlibs" \
		2>&1)
	if [ ! $? -eq 0 ]
	then
		echo -e "Aborting."
		exit 1
	fi

	case $dlg_selected in
		1 )
			dl_tlibs;;
		2 )
			link_tlibs;;
	esac

fi
