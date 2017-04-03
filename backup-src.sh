#!/bin/zsh
#
# quick backup of Takin source files
# @author Tobias Weber <tobias.weber@tum.de>
# @license GPLv2
# @date 18-jun-2016
#

thetime=${${${$(date --iso-8601='ns')//:/-}//,/_}//+/p}
thefile="takin_src-${thetime}.txz"

find . \( -name "*.cpp" -type f \) -o \
	\( -name "*.h" -type f \) -o \
	\( -name "*.py" -type f \) -o \
	\( -name "*.scr" -type f \) -o \
	\( -name "*.sh" -type f \) -o \
	\( -name "*.ui" -type f \) \
	| xargs tar -Jvcf ${thefile}

echo -e "\n$(tput bold)Wrote ${thefile}$(tput sgr0)\n"
