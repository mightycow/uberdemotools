#!/bin/bash
set -e

UDT_TARGET=""
options=("Release", "Debug")
PS3="Select the target: "
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

UDT_OPTIONS="--jobs=8 config=$UDT_CONFIG"

pushd ../.build/lin_gmake
make clean $UDT_OPTIONS
make all $UDT_OPTIONS
popd
