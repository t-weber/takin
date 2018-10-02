#!/bin/bash
#
# @date 12-apr-17
# @author Tobias Weber <tobias.weber@tum.de>
# @license GPLv2
#
# copy framework libs
#
# pack with: hdiutil create takin.dmg -srcfolder takin.app -fs UDF -format "UDBZ" -volname "takin"
#


PRG="takin.app"
OS_BIN="$BIN_DIR"	# set accordingly

PLUGIN_DIR="/usr/local/opt/qt5/plugins/"
DST_DIR="${PRG}/Contents/Frameworks/"
DST_PLUGIN_DIR="${PRG}/Contents/PlugIns/"


# -----------------------------------------------------------------------------
# frameworks
declare -a SRC_LIBS=(
	"/usr/local/opt/qt5/lib/QtCore.framework"
	"/usr/local/opt/qt5/lib/QtGui.framework"
	"/usr/local/opt/qt5/lib/QtWidgets.framework"
	"/usr/local/opt/qt5/lib/QtOpenGL.framework"
	"/usr/local/opt/qt5/lib/QtConcurrent.framework"
	"/usr/local/opt/qt5/lib/QtXml.framework"
	"/usr/local/opt/qt5/lib/QtSvg.framework"
	"/usr/local/opt/qt5/lib/QtPrintSupport.framework"
	"/usr/local/opt/qt5/lib/QtDBus.framework"
	"/usr/local/opt/qwt/lib/qwt.framework"
	"/usr/local/opt/minuit2/lib/libMinuit2.0.dylib"
	"/usr/local/opt/boost/lib/libboost_system.dylib"
	"/usr/local/opt/boost/lib/libboost_filesystem.dylib"
	"/usr/local/opt/boost/lib/libboost_iostreams.dylib"
	"/usr/local/opt/boost/lib/libboost_regex.dylib"
	"/usr/local/opt/boost/lib/libboost_program_options.dylib"
	"/usr/local/opt/boost-python/lib/libboost_python27.dylib"
	"/usr/local/opt/freetype/lib/libfreetype.6.dylib"
	"/usr/local/opt/libpng/lib/libpng16.16.dylib"
	"/usr/local/opt/libjpeg/lib/libjpeg.9.dylib"
	"/usr/local/opt/libtiff/lib/libtiff.5.dylib"
)


# qt plugins
declare -a SRC_PLUGINS=(
	"printsupport/libcocoaprintersupport.dylib"
	"imageformats/libqsvg.dylib"
	"imageformats/libqicns.dylib"
	"imageformats/libqjpeg.dylib"
	"iconengines/libqsvgicon.dylib"
	"styles/libqmacstyle.dylib"
	"platforms/libqcocoa.dylib"
	"platforms/libqminimal.dylib"
)
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# create dirs
mkdir -pv ${DST_DIR}
mkdir -pv "${DST_PLUGIN_DIR}/printsupport"
mkdir -pv "${DST_PLUGIN_DIR}/imageformats"
mkdir -pv "${DST_PLUGIN_DIR}/iconengines"
mkdir -pv "${DST_PLUGIN_DIR}/platforms"
mkdir -pv "${DST_PLUGIN_DIR}/platformthemes"
mkdir -pv "${DST_PLUGIN_DIR}/styles"
mkdir -pv "${PRG}/Contents/${OS_BIN}"
# -----------------------------------------------------------------------------



# -----------------------------------------------------------------------------
# copy libs
for srclib in ${SRC_LIBS[@]}; do
	echo -e "Copying library \"${srclib}\"..."
	cp -rv ${srclib} ${DST_DIR}
done


# copy plugins
for srclib in ${SRC_PLUGINS[@]}; do
	echo -e "Copying plugin \"${srclib}\"..."
	cp -v "${PLUGIN_DIR}${srclib}" "${DST_PLUGIN_DIR}${srclib}"
	chmod a+rx "${DST_PLUGIN_DIR}${srclib}"
done


# copy binaries
cp -v bin/takin "${PRG}/Contents/${OS_BIN}/"
cp -v bin/convofit "${PRG}/Contents/${OS_BIN}/"
cp -v bin/convoseries "${PRG}/Contents/${OS_BIN}/"
cp -v bin/sfact "${PRG}/Contents/${OS_BIN}/"
cp -v bin/polextract "${PRG}/Contents/${OS_BIN}/"
# -----------------------------------------------------------------------------



# -----------------------------------------------------------------------------
# attributes
chmod -R -u+rwa+r ${DST_DIR}
chmod -R -u+rwa+r ${DST_PLUGIN_DIR}

find {DST_DIR} -type d -print0 | xargs -0 chmod a+x
find {DST_PLUGIN_DIR} -type d -print0 | xargs -0 chmod a+x
# -----------------------------------------------------------------------------



# -----------------------------------------------------------------------------
# delete unnecessary files
find "${DST_DIR}" -type d -name "Headers" -print0 | xargs -0 rm -rfv
find "${DST_DIR}" -type d -name "Current" -print0 | xargs -0 rm -rfv

declare -a QT_FW_LIBS=("QtCore" "QtGui" "QtWidgets" "QtConcurrent" "QtOpenGL"
	"QtSvg" "QtXml" "QtDBus" "QtPrintSupport" "qwt")

for qlib in ${QT_FW_LIBS[@]}; do
	rm -rfv ${DST_DIR}/${qlib}.framework/Resources
	rm -rfv ${DST_DIR}/${qlib}.framework/${qlib}*
done
# -----------------------------------------------------------------------------
