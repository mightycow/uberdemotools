#!/bin/bash

BIN_PATH=../UDT_DLL/.bin/lin_gmake/x64/release
CHG_PATH=$(pwd)/..
AR_NAME=udt_con.tar.bz2
AR_PATH=$(pwd)/$AR_NAME

rm --force $AR_NAME
tar cvf "$AR_PATH" --bzip2 -p \
	-C "$BIN_PATH" UDT_cutter UDT_splitter UDT_timeshifter UDT_merger UDT_json UDT_captures UDT_converter \
	-C "$CHG_PATH" --xform s:_dll:: changelog_dll.txt
