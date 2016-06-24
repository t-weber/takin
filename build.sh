#!/bin/bash

MAKE=make
GCC=$(which gcc 2>/dev/null)

if [ "$GCC" != "" ]	# don't check version if only e.g. clang is available
then
	GCC_VER=$($GCC -dumpversion)
	echo -e "GCC is version $GCC_VER"

	GCC_VER=(${GCC_VER//./ })

	if [ ${GCC_VER[0]} -le 4 ] && [ ${GCC_VER[1]} -lt 8 ]
	then
    		echo -e "\t... which is not compatible with Takin.\n\t\tAt least version 4.8 is needed."
	        exit -1;
	fi
fi




if [ ! -d tlibs ] || [ ! -f tlibs/AUTHORS ]
then
	echo -e "Error: tlibs not installed. Use ./setup_tlibs.sh"
	exit -1;
fi




echo -e "Prebuilding..."
if ! ./prebuild.sh
then
	echo -e "Error: Prebuild failed";
	exit -1;
fi




NPROC=$(which nproc 2>/dev/null)
if [ "$NPROC" == "" ]; then NPROC="/usr/bin/nproc"; fi


if [ ! -x $NPROC ]
then
	CPUCNT=2
else
	CPUCNT=$($NPROC --ignore=1)
fi

echo -e "\nBuilding using $CPUCNT processes..."
${MAKE} -j${CPUCNT} -f themakefile
