#!/bin/bash

PackageBinaries () {
  BIN_PATH=UDT_DLL/.bin/gmake/"$1"/release/
  tar cvf udt_con_"$1".tar.bz2 --bzip2 -p --xform s:$BIN_PATH:: "$BIN_PATH"UDT_cutter "$BIN_PATH"UDT_splitter "$BIN_PATH"UDT_timeshifter "$BIN_PATH"UDT_merger "$BIN_PATH"UDT_json "$BIN_PATH"UDT_captures "$BIN_PATH"UDT_converter --xform s:_dll:: changelog_dll.txt
}

PackageBinaries x86
PackageBinaries x64

