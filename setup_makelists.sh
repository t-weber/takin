#!/bin/bash

DLG=$(which dialog 2>/dev/null)
if [ "$DLG" == "" ]; then DLG="/usr/bin/dialog"; fi


if [ ! -x $DLG ]
then
	echo -e "Could not find dialog tool."
	exit -1
else
	dlg_selected=$(${DLG} --stdout --title "Makelists" \
		--menu "Setup Makelists" 12 50 4 \
			1 "Native, with dynamic tlibs" \
			2 "Native, with static tlibs" \
			3 "Mingw cross compile, with static tlibs" \
			4 "Minimal cli client, with static tlibs" \
		2>&1)
	if [ ! $? -eq 0 ]
	then
		echo -e "Aborting."
		exit 1
	fi

	case $dlg_selected in
		1 )
			rm -f CMakeLists.txt
			ln -sf CMakeLists-dynamic.txt CMakeLists.txt
			;;
		2 )
			rm -f CMakeLists.txt
			ln -sf CMakeLists-static.txt CMakeLists.txt
			;;
		3 )
			rm -f CMakeLists.txt
			ln -sf CMakeLists-static-mingw.txt CMakeLists.txt
			;;
		4 )
			rm -f CMakeLists.txt
			ln -sf CMakeLists-static-cli.txt CMakeLists.txt
			;;
	esac

fi
