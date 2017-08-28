#!/bin/bash
#
# creates a distro for mingw
# @author Tobias Weber <tobias.weber@tum.de>
# @license GPLv2
#


# installation directory
INSTDIR=~/.wine/drive_c/takin


# main programs
mkdir ${INSTDIR}
cp -v bin/takin.exe		${INSTDIR}/
cp -v bin/convofit.exe	${INSTDIR}/
cp -v bin/convoseries.exe	${INSTDIR}/
cp -v COPYING			${INSTDIR}/
cp -v LICENSES			${INSTDIR}/
cp -v LITERATURE		${INSTDIR}/
cp -v AUTHORS			${INSTDIR}/



# resources
mkdir ${INSTDIR}/res
cp -rv res/* 	${INSTDIR}/res/
gunzip -v ${INSTDIR}/res/data/*



# libraries
cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libstdc++-6.dll		${INSTDIR}/
cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libwinpthread-1.dll	${INSTDIR}/
cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libpng16-16.dll		${INSTDIR}/
cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libgcc_s_sjlj-1.dll	${INSTDIR}/
cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libgcc_s_seh-1.dll		${INSTDIR}/
cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libbz2-1.dll			${INSTDIR}/
cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/zlib1.dll				${INSTDIR}/

cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libboost_regex-mt.dll				${INSTDIR}/
cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libboost_system-mt.dll				${INSTDIR}/
cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libboost_iostreams-mt.dll			${INSTDIR}/
cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libboost_filesystem-mt.dll			${INSTDIR}/
cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libboost_program_options-mt.dll	${INSTDIR}/

cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/QtCore4.dll		${INSTDIR}/
cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/QtGui4.dll			${INSTDIR}/
cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/QtOpenGL4.dll		${INSTDIR}/
cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/QtSvg4.dll			${INSTDIR}/
cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/QtXml4.dll			${INSTDIR}/
cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/qwt.dll			${INSTDIR}/
cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libfreetype-6.dll	${INSTDIR}/



# qt plugins
mkdir -p ${INSTDIR}/lib/plugins/iconengines/
mkdir -p ${INSTDIR}/lib/plugins/imageformats

cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/lib/qt4/plugins/iconengines/qsvgicon4.dll	${INSTDIR}/lib/plugins/iconengines/
cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/lib/qt4/plugins/imageformats/qsvg4.dll		${INSTDIR}/lib/plugins/imageformats/



# stripping
strip -v ${INSTDIR}/*.exe
strip -v ${INSTDIR}/*.dll
strip -v ${INSTDIR}/lib/plugins/iconengines/*.dll
strip -v ${INSTDIR}/lib/plugins/imageformats/*.dll
