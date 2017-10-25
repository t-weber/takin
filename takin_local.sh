#!/bin/sh
#
# Takin
# @author Tobias Weber <tobias.weber@tum.de>
# @license GPLv2
#

arg=$1
LD_LIBRARY_PATH=/usr/local/lib:/usr/local/lib64:$LD_LIBRARY_PATH \
	/usr/local/bin/takin ${arg}
