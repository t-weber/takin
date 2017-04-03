#!/bin/bash
#
# internal dev script: link to other project dirs
# @author Tobias Weber <tobias.weber@tum.de>
# @license GPLv2
#

function link_tlibs
{
	echo -e "Linking tlibs..."
	ln -sf ../tlibs
}

function link_tdata
{
	echo -e "Linking takin-data..."
	ln -sf ../takin-data
}

link_tlibs
link_tdata
