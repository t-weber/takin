#!/bin/bash
#
# @date jan-19
# @author Tobias Weber <tobias.weber@tum.de>
# @license GPLv2
#
# copy application into disk image
#

echo -e "Removing any old file ..."
rm -f takin.dmg

echo -e "\nCreating a writable image ..."
if ! hdiutil create takin.dmg -srcfolder takin.app -fs UDF -format "UDRW" -volname "takin"; then
	echo -e "Cannot create image file."
	exit -1
fi

echo -e "\n--------------------------------------------------------------------------------"

echo -e "Mounting the image ..."
if ! hdiutil attach takin.dmg -readwrite; then
	echo -e "Cannot mount image file."
	exit -1
fi

echo -e "\n--------------------------------------------------------------------------------"

echo -e "Creating additional files/links in the image ..."
ln -sf /Applications /Volumes/takin/Install_by_dragging_Takin_here

echo -e "\n--------------------------------------------------------------------------------"

echo -e "Unmounting the image ..."
if ! hdiutil detach /Volumes/takin; then
	echo -e "Cannot detach image file."
	exit -1
fi

echo -e "\n--------------------------------------------------------------------------------"

echo -e "Converting the image to read-only and compressing ..."
if ! hdiutil convert takin.dmg -o takin_final.dmg -format "UDBZ"; then
	echo -e "Cannot convert image file to UDBZ."
	exit -1
fi

echo -e "\n--------------------------------------------------------------------------------"

echo -e "Copying file ..."
mv -v takin_final.dmg takin.dmg

echo -e "\nSuccessfully created takin.dmg."
