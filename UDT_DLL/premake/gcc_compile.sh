#!/bin/bash
cd ../.build/gmake
make clean

UDT_ARCH=""
options=("x86", "x64")
PS3="Select the architecture"
select opt in "${options[@]}" "Quit"; do 
    case "$REPLY" in
    1 ) UDT_ARCH=x32;;
    2 ) UDT_ARCH=x64;;
    $(( ${#options[@]}+1 )) ) echo "Goodbye!"; break;;
    *) echo "Invalid option. Try another one."; continue;;
    esac
	break
done

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

UDT_CONFIG="$UDT_TARGET"_"$UDT_ARCH"
echo Selected config: $UDT_CONFIG

make UDT config=$UDT_CONFIG
make UDT_c89 config=$UDT_CONFIG
make UDT_cutter config=$UDT_CONFIG
make UDT_splitter config=$UDT_CONFIG
make UDT_timeshifter config=$UDT_CONFIG
make UDT_merger config=$UDT_CONFIG
make UDT_json config=$UDT_CONFIG
make UDT_captures config=$UDT_CONFIG
make UDT_converter config=$UDT_CONFIG
