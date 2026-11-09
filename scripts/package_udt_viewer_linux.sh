#!/bin/bash

BIN_PATH=../UDT_DLL/.bin/lin_gmake/x64/release
AR_NAME=udt_viewer.tar.bz2
AR_PATH=$(pwd)/$AR_NAME

# prepare temp folder
rm --force --recursive --dir __temp/viewer_data
mkdir --parent __temp/viewer_data
rm --force $AR_NAME

# populate temp folder with everything
$BIN_PATH/viewer_data_gen -o=__temp/viewer_data ../viewer_data
cp $BIN_PATH/UDT_viewer __temp
cp ../changelog_viewer.txt __temp/changelog.txt
cp ../viewer_data/map_aliases.txt __temp/viewer_data
cp ../viewer_data/deja_vu_sans.ttf __temp/viewer_data
cp ../viewer_data/blender_icons.png __temp/viewer_data
cp ../viewer_data/maps/*.png __temp/viewer_data

# temp -> archive
pushd __temp
tar cvf "$AR_PATH" --bzip2 -p *
popd

# clean up
rm --force --recursive --dir __temp/viewer_data
rm --force --recursive --dir __temp
