#!/bin/bash


if [[ $# -ge 3 ]]; then
	UIC="$1"
	MOC="$2"

	TAKINROOT="$3"
	FORCE=0
else
	UIC="$(which uic-qt4)"
	MOC="$(which moc-qt4)"

#	UIC="$(which uic-qt5)"
#	MOC="$(which moc-qt5)"

	if [ $? -ne 0 ]; then
		UIC="$(which uic)"
		MOC="$(which moc)"
	fi

	TAKINROOT=.
	FORCE=1		# force rebuild when manually called
fi


if [ -z "$UIC" ] || [ -z "${MOC}" ]; then
	echo "Error: moc/uic tools not found!";
	exit -1;
fi

echo -e "Using UIC: ${UIC}\nUsing MOC: ${MOC}\n"



echo -e "--------------------------------------------------------------------------------"
echo -e "building uis..."

cd "${TAKINROOT}/ui"
for file in *.ui; do
	outfile=ui_${file%\.ui}.h

	if [ ! -e ${outfile} -o "${FORCE}" -eq "1" ]; then
		echo -e "${file} -> ${outfile}"
		${UIC} ${file} -o ${outfile}
	fi
done
cd ..


echo -e "--------------------------------------------------------------------------------"
echo -e "\n"




echo -e "--------------------------------------------------------------------------------"
echo -e "building mocs..."

declare -a hfiles=(
        "${TAKINROOT}/tools/taz/taz.h"
        "${TAKINROOT}/tools/taz/scattering_triangle.h"
        "${TAKINROOT}/tools/taz/real_lattice.h"
        "${TAKINROOT}/tools/taz/proj_lattice.h"
        "${TAKINROOT}/tools/taz/tas_layout.h"
        "${TAKINROOT}/tools/taz/tof_layout.h"
        "${TAKINROOT}/tools/taz/recip3d.h"
        "${TAKINROOT}/tools/taz/net.h"
        "${TAKINROOT}/tools/taz/nicos.h"
        "${TAKINROOT}/tools/taz/sics.h"
        "${TAKINROOT}/tools/res/ResoDlg.h"
        "${TAKINROOT}/tools/scanviewer/scanviewer.h"
	"${TAKINROOT}/tools/monteconvo/ConvoDlg.h"
	"${TAKINROOT}/tools/monteconvo/SqwParamDlg.h"
	"${TAKINROOT}/tools/sglist/SgListDlg.h"
        "${TAKINROOT}/dialogs/EllipseDlg.h"
        "${TAKINROOT}/dialogs/EllipseDlg3D.h"
        "${TAKINROOT}/dialogs/RecipParamDlg.h"
        "${TAKINROOT}/dialogs/RealParamDlg.h"
        "${TAKINROOT}/dialogs/SpurionDlg.h"
        "${TAKINROOT}/dialogs/NeutronDlg.h"
        "${TAKINROOT}/dialogs/SrvDlg.h"
        "${TAKINROOT}/dialogs/GotoDlg.h"
        "${TAKINROOT}/dialogs/AtomsDlg.h"
        "${TAKINROOT}/dialogs/NetCacheDlg.h"
	"${TAKINROOT}/dialogs/PowderDlg.h"
	"${TAKINROOT}/dialogs/SettingsDlg.h"
	"${TAKINROOT}/dialogs/DWDlg.h"
	"${TAKINROOT}/dialogs/DynPlaneDlg.h"
	"${TAKINROOT}/dialogs/FilePreviewDlg.h"
	"${TAKINROOT}/dialogs/FormfactorDlg.h"
	"${TAKINROOT}/dialogs/AboutDlg.h"
	"${TAKINROOT}/dialogs/DispDlg.h"
)

for hfile in ${hfiles[@]}; do
	mocfile=${hfile%\.h}.moc

	if [ ! -e ${mocfile} -o "${FORCE}" -eq "1" ]; then
		echo -e "${hfile} -> ${mocfile}"
		${MOC} ${hfile} -o ${mocfile}
	fi
done

echo -e "--------------------------------------------------------------------------------"
echo -e "\n"


echo -e "--------------------------------------------------------------------------------"
echo -e "building docs..."

qcollectiongenerator ${TAKINROOT}/doc/takin.qhcp -o ${TAKINROOT}/doc/takin.qhc
cp -v ${TAKINROOT}/doc/takin.qhc ${TAKINROOT}/res/
cp -v ${TAKINROOT}/doc/takin.qch ${TAKINROOT}/res/

echo -e "--------------------------------------------------------------------------------"


exit 0
