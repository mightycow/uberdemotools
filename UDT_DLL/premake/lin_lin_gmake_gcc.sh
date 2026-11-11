#!/bin/bash
set -e

source "$(dirname "$0")/helpers/pick_functions.sh"

UDT_TOOLCHAIN=lin_lin_gmake_gcc

Generate() {
	./premake5 gmake --preset=$UDT_TOOLCHAIN
}

Test() {
	PickTarget
	pushd ../.bin/$UDT_TOOLCHAIN/x64/$UDT_TARGET
	./tests
	popd
}

Compile() {
	PickTarget
	PickClean
	UDT_CONFIG="$UDT_TARGET"_x64
	echo Selected config: $UDT_CONFIG
	# UDT_JOBS = max(core_count - 2, 1)
	UDT_JOBS="$(( (n = $(nproc) - 2) > 1 ? n : 1 ))"
	UDT_OPTIONS="--jobs=$UDT_JOBS config=$UDT_CONFIG"

	pushd ../.build/$UDT_TOOLCHAIN
	if [[ "$UDT_CLEAN" -eq 1 ]]; then
		make clean $UDT_OPTIONS
	fi
	make all $UDT_OPTIONS
	popd
}

action=""
if [[ -z "$action" ]]; then
	echo "Which action?"
	echo "G: Generate"
	echo "C: Compile"
	echo "T: Test"
	read -r -p "> " action
fi

case "${action,,}" in
	g) Generate ;;
	c) Compile ;;
	t) Test ;;
	*)
		echo "Invalid choice: '$action'" >&2
		exit 1
		;;
esac
