#!/bin/bash
# internal dev script: link to other project dirs

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
