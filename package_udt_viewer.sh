#!/bin/bash

PackageViewer () {
  rm --force udt_viewer_"$1".tar.bz2
  BIN_PATH=UDT_DLL/.bin/gmake/"$1"/release/
  tar cvf udt_viewer_"$1".tar.bz2 --bzip2 -p --xform s:$BIN_PATH:: "$BIN_PATH"UDT_viewer --xform s:changelog_viewer:changelog: changelog_viewer.txt viewer_data/blender_icons.png viewer_data/deja_vu_sans.ttf viewer_data/map_aliases.txt --xform s:/maps:: viewer_data/maps/*.png --xform s:__temp/:: __temp/viewer_data
}

rm --force --recursive --dir __temp/viewer_data
mkdir --parent __temp/viewer_data
./UDT_DLL/.bin/gmake/x64/release/viewer_data_gen -o=__temp/viewer_data viewer_data
PackageViewer x86
PackageViewer x64
rm --force --recursive --dir __temp/viewer_data

