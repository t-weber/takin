#!/bin/bash
#
# @date oct-17
# @author Tobias Weber <tobias.weber@tum.de>
# @license GPLv2
#
# create icon files
#

FILE=res/icons/takin.svg


echo "svg -> png (512)"
convert -resize 512x512 -background none -alpha on -transparent-color white $FILE ${FILE%\.svg}.png

echo "png -> icns"
makeicns -in ${FILE%\.svg}.png -out ${FILE%\.svg}.icns


echo "svg -> png (128)"
convert -resize 128x128 -background none -alpha on -transparent-color white $FILE ${FILE%\.svg}.png

echo "png -> ico"
convert ${FILE%\.svg}.png ${FILE%\.svg}.ico


# remove temporary file
rm ${FILE%\.svg}.png
