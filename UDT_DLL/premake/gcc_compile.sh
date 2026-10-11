#!/bin/bash
set -e

cd ../.build/gmake

UDT_TARGET=""
options=("Release", "Debug")
PS3="Select the target"
select opt in "${options[@]}" "Quit"; do 
    case "$REPLY" in
    1 ) UDT_TARGET=release;;
    2 ) UDT_TARGET=debug;;
    $(( ${#options[@]}+1 )) ) echo "Goodbye!"; break;;
    *) echo "Invalid option. Try another one."; continue;;
    esac
	break
done

UDT_ARCH=x64

UDT_CONFIG="$UDT_TARGET"_"$UDT_ARCH"
echo Selected config: $UDT_CONFIG

UDT_OPTIONS=--jobs=8 config=$UDT_CONFIG

make clean
make UDT $UDT_OPTIONS
make UDT_c89 $UDT_OPTIONS
make UDT_cutter $UDT_OPTIONS
make UDT_splitter $UDT_OPTIONS
make UDT_timeshifter $UDT_OPTIONS
make UDT_merger $UDT_OPTIONS
make UDT_json $UDT_OPTIONS
make UDT_captures $UDT_OPTIONS
make UDT_converter $UDT_OPTIONS
make UDT_viewer $UDT_OPTIONS
make viewer_data_gen $UDT_OPTIONS
make tut_players $UDT_OPTIONS
make tut_multi_rail $UDT_OPTIONS

